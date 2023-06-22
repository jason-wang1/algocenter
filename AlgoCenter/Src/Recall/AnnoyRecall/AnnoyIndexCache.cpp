#include "AnnoyIndexCache.h"
#include <future>
#include <chrono>
#include "Eigen/Dense"
#include "glog/logging.h"
#include "TDRedis/TDRedis.h"
#include "TDRedis/TDRedisConnPool.h"
#include "Common/Function.h"
#include "Protobuf/other/annoy_file_data.pb.h"
#include "Protobuf/other/local_file.pb.h"

#include "AlgoCenter/Define.h"
#include "AlgoCenter/Config.h"

bool AnnoyIndexCache::Init(
    const std::string &basic_path,
    const size_t emb_dimension,
    const int search_node_num)
{
    m_BasicPath = basic_path;
    m_EmbDimension = emb_dimension;
    m_SearchNodeNum = search_node_num;
    if (m_BasicPath.empty() || m_SearchNodeNum <= 0 || m_SearchNodeNum <= 0)
    {
        LOG(ERROR) << "Init() param error"
                   << ", basic_path = " << basic_path
                   << ", emb_dimension = " << emb_dimension
                   << ", search_node_num = " << search_node_num;
        return false;
    }

    m_CacheDataPtr = std::make_shared<CacheData>();
    if (m_CacheDataPtr == nullptr)
    {
        LOG(ERROR) << "Init() make shared CacheData failed.";
        return false;
    }

    int err = Refresh();
    if (err != Common::Error::OK)
    {
        LOG(ERROR) << "Init() call Refresh failed, err = " << err;
        return false;
    }

    // 初始化成功, 拉起线程定时调用更新函数
    g_Init = true;
    g_WorkThreadPtr = std::make_shared<std::thread>(std::bind(&AnnoyIndexCache::OnTimer, this));

    LOG(INFO) << "Init() g_WorkThreadPtr start success.";

    return true;
}

void AnnoyIndexCache::ShutDown()
{
    if (g_Init)
    {
        g_Init = false;
        if (g_WorkThreadPtr->joinable())
        {
            g_WorkThreadPtr->join();
            LOG(INFO) << "ShutDown() g_WorkThreadPtr.join() finish.";
        }
    }
}

int32_t AnnoyIndexCache::AnnoyRetrieval(
    const std::string &slice,
    const std::string &item_key,
    const std::vector<std::string> &vec_keynames,
    const int top_k,
    std::vector<std::string> &vec_near_items,
    std::vector<float> &vec_distances) const
{
    AnnoyIndexData slice_data;
    if (!m_CacheDataPtr->GetDBufData(slice, slice_data))
    {
        LOG(ERROR) << "AnnoyRetrieval() slice data is empty, slice = " << slice;
        return Common::Error::Recall_Annoy_DataIsEmpty;
    }

    if (slice_data.spAnnoyIndex == nullptr ||
        slice_data.spItemDict == nullptr ||
        slice_data.spItemVector == nullptr ||
        slice_data.spKeynameVector == nullptr)
    {
        LOG(ERROR) << "AnnoyRetrieval() slice data not init, slice = " << slice;
        return Common::Error::Recall_Annoy_NotInit;
    }

    std::vector<int> vec_cache_dict;
    vec_cache_dict.reserve(top_k);

    std::vector<float> vec_cache_distances;
    vec_cache_distances.reserve(top_k);

    auto item_iter = slice_data.spItemVector->find(item_key);
    if (item_iter != slice_data.spItemVector->end())
    {
        if (item_iter->second.size() != slice_data.iEmbDimension)
        {
            LOG(ERROR) << "AnnoyRetrieval() slice data not init, slice = " << slice;
            return Common::Error::Recall_Annoy_NotInit;
        }

        slice_data.spAnnoyIndex->get_nns_by_vector(
            item_iter->second.data(), top_k, slice_data.iSearchNodeNum, &vec_cache_dict, &vec_cache_distances);
    }
    else
    {
        std::vector<std::vector<float>> vec_keynames_vector;
        vec_keynames_vector.reserve(vec_keynames.size());
        for (const auto &keyname : vec_keynames)
        {
            auto keyname_iter = slice_data.spKeynameVector->find(keyname);
            if (keyname_iter != slice_data.spItemVector->end())
            {
                if (keyname_iter->second.size() == slice_data.iEmbDimension)
                {
                    vec_keynames_vector.push_back(keyname_iter->second);
                }
            }
        }

        size_t keynames_vector_size = vec_keynames_vector.size();
        if (keynames_vector_size == 0)
        {
            LOG(WARNING) << "AnnoyRetrieval() item vector not find, and keynames mean vector not generated.";
            return Common::Error::OK;
        }

        Eigen::MatrixXf matrix_xf(keynames_vector_size, slice_data.iEmbDimension);
        for (size_t idx = 0; idx < keynames_vector_size; idx++)
        {
            matrix_xf.row(idx) =
                Eigen::Map<Eigen::RowVectorXf>(vec_keynames_vector[idx].data(), 1, vec_keynames_vector[idx].size());
        }

        Eigen::MatrixXf mean_vector = matrix_xf.colwise().mean();
        slice_data.spAnnoyIndex->get_nns_by_vector(
            mean_vector.data(), top_k, slice_data.iSearchNodeNum, &vec_cache_dict, &vec_cache_distances);
    }

    vec_near_items.reserve(top_k);
    vec_distances.reserve(top_k);
    size_t result_num = std::min(vec_cache_dict.size(), vec_cache_distances.size());
    for (size_t idx = 0; idx < result_num; idx++)
    {
        int item_dict = vec_cache_dict[idx];
        auto iter = slice_data.spItemDict->find(item_dict);
        if (iter == slice_data.spItemDict->end())
        {
            LOG(WARNING) << "AnnoyRetrieval() annoy dict find error, item_dict = " << item_dict;
        }
        else
        {
            vec_near_items.push_back(iter->second);
            vec_distances.push_back(vec_cache_distances[idx]);
        }
    }

    return Common::Error::OK;
}

int32_t AnnoyIndexCache::Refresh()
{
    std::string latest_version;
    if (!GetLatestVersion(m_BasicPath, latest_version))
    {
        LOG(ERROR) << "Refresh() call GetLatestVersion failed"
                   << ", m_BasicPath = " << m_BasicPath
                   << ", latest_version = " << latest_version;
        return Common::Error::Recall_Annoy_ParamError;
    }

    long latest_timestamp = atol(latest_version.c_str());
    long cache_timestamp = m_CacheDataPtr->GetTimestamp();

    // 判断更新数据条件
    if (latest_timestamp != 0 &&
        cache_timestamp != 0 &&
        latest_timestamp == cache_timestamp)
    {
        // 数据已经是最新, 直接返回OK即可
        return Common::Error::OK;
    }

    std::string version_path = m_BasicPath + "/" + latest_version;
    std::vector<std::string> vec_slices;
    std::string result;
    if (!GetLatestSlice(version_path, vec_slices, result))
    {
        LOG(ERROR) << "Refresh() call GetLatestSlice failed"
                   << ", version_path = " << version_path
                   << ", result = " << result;
        return Common::Error::Recall_Annoy_ParamError;
    }

    std::unordered_map<std::string, AnnoyIndexData> data;
    for (const auto &slice : vec_slices)
    {
        AnnoyIndexData slice_data;
        slice_data.iEmbDimension = m_EmbDimension;
        slice_data.iSearchNodeNum = m_SearchNodeNum;

        std::string slice_path = version_path + "/" + slice;

        {
            std::string annoy_index_path = slice_path + "/annoy_index";
            if (!std::filesystem::is_regular_file(annoy_index_path))
            {
                LOG(ERROR) << "Refresh() no regular file, annoy_index_path = " << annoy_index_path;
                continue;
            }

            char *p_error[1024];

            slice_data.spAnnoyIndex =
                std::make_shared<AnnoyIndex<int, float, Angular, Kiss32Random, AnnoyIndexMultiThreadedBuildPolicy>>(slice_data.iEmbDimension);
            if (!slice_data.spAnnoyIndex->load(annoy_index_path.c_str(), false, p_error))
            {
                LOG(ERROR) << "Refresh() load annoy index failed, annoy_index_path = " << annoy_index_path;
                LOG(ERROR) << "Refresh() load annoy index failed, p_error = " << std::string(p_error[0]);
                continue;
            }

            if (slice_data.spAnnoyIndex->get_n_items() <= 0)
            {
                LOG(ERROR) << "Refresh() annoy index is empty.";
                continue;
            }
        }

        {
            std::string item_vector_path = slice_path + "/item_vector";
            std::string item_vector_buffer;
            if (Common::FastReadFile(item_vector_path, item_vector_buffer, false) == false)
            {
                LOG(ERROR) << "Refresh() call FastReadFile failed, item_vector_path = " << item_vector_path;
                continue;
            }

            slice_data.spItemDict = std::make_shared<std::unordered_map<int, std::string>>();
            slice_data.spItemVector = std::make_shared<std::unordered_map<std::string, std::vector<float>>>();

            RSP_LocalFileData::FileData file_data;
            if (!file_data.ParseFromString(item_vector_buffer))
            {
                LOG(ERROR) << "Refresh() FileData ParseFromString failed.";
                continue;
            }

            const auto &file_data_protos = file_data.protos();
            for (const auto &proto_data : file_data_protos)
            {
                RSP_AnnoyFileData::AnnoyItemVector aiv;
                if (!aiv.ParseFromString(proto_data))
                {
                    LOG(ERROR) << "Refresh() AnnoyItemVector ParseFromString failed.";
                    continue;
                }

                const auto &aiv_vector_data = aiv.vector_data();
                size_t aiv_vector_size = aiv_vector_data.size();
                if (aiv_vector_size != slice_data.iEmbDimension)
                {
                    LOG(WARNING) << "Refresh() emb dimension error"
                                 << ", item_vector_path = " << item_vector_path
                                 << ", aiv_vector_size = " << aiv_vector_size
                                 << ", iEmbDimension = " << slice_data.iEmbDimension;
                    continue;
                }

                std::string item_key = std::to_string(aiv.item_id()) + "_" + std::to_string(aiv.res_type());

                auto &ItemDict = *slice_data.spItemDict.get();
                ItemDict[aiv.item_pos()] = item_key;

                auto &ItemVector = *slice_data.spItemVector.get();
                auto &vec_vector = ItemVector[item_key];

                vec_vector.reserve(aiv_vector_size);
                for (const auto &vector_data : aiv_vector_data)
                {
                    vec_vector.push_back(vector_data);
                }
            }

            if (slice_data.spItemDict->empty() || slice_data.spItemVector->empty())
            {
                LOG(ERROR) << "Refresh() item vector is empty.";
                continue;
            }
        }

        {
            std::string keyname_vector_path = slice_path + "/keyname_vector";
            std::string keyname_vector_buffer;
            if (Common::FastReadFile(keyname_vector_path, keyname_vector_buffer, false) == false)
            {
                LOG(ERROR) << "Refresh() call FastReadFile failed, keyname_vector_path = " << keyname_vector_path;
                continue;
            }

            slice_data.spKeynameVector = std::make_shared<std::unordered_map<std::string, std::vector<float>>>();

            RSP_LocalFileData::FileData file_data;
            if (!file_data.ParseFromString(keyname_vector_buffer))
            {
                LOG(ERROR) << "Refresh() FileData ParseFromString failed.";
                continue;
            }

            const auto &file_data_protos = file_data.protos();
            for (const auto &proto_data : file_data_protos)
            {
                RSP_AnnoyFileData::AnnoyKeynameVector akv;
                if (!akv.ParseFromString(proto_data))
                {
                    LOG(ERROR) << "Refresh() AnnoyKeynameVector ParseFromString Failed.";
                    continue;
                }

                const auto &akv_vector_data = akv.vector_data();
                size_t akv_vector_size = akv_vector_data.size();
                if (akv_vector_size != slice_data.iEmbDimension)
                {
                    LOG(ERROR) << "Refresh() emb dimension Error"
                               << ", keyname_vector_path = " << keyname_vector_path
                               << ", akv_vector_size = " << akv_vector_size
                               << ", iEmbDimension = " << slice_data.iEmbDimension;
                    continue;
                }

                auto &KeynameVector = *slice_data.spKeynameVector.get();
                auto &vec_vector = KeynameVector[akv.keyname()];

                vec_vector.reserve(akv_vector_size);
                for (const auto &vector_data : akv_vector_data)
                {
                    vec_vector.push_back(vector_data);
                }
            }

            if (slice_data.spKeynameVector->empty())
            {
                LOG(ERROR) << "Refresh() Keyname Vector is empty.";
                continue;
            }
        }

        data[slice] = std::move(slice_data);
    }

    // 将数据写入本地缓存
    if (!data.empty())
    {
        m_CacheDataPtr->SetDBufData(std::move(data));
        m_CacheDataPtr->SetTimestamp(latest_timestamp);
    }

    return Common::Error::OK;
}

bool AnnoyIndexCache::GetLatestVersion(
    const std::string &basic_path,
    std::string &result)
{
    auto store_basic_path = std::filesystem::path(basic_path);
    bool find = false;
    int latest_version = 0;

    // 如果不存在, 返回错误
    if (!std::filesystem::exists(store_basic_path))
    {
        result = "store_basic_path " + store_basic_path.string() + " not exists";
        return false;
    }

    // 如果存在, 但是不是文件夹, 那只能返回错误
    if (!std::filesystem::is_directory(store_basic_path))
    {
        // 路径必须是文件夹
        result = "store_basic_path" + store_basic_path.string() + " must be a folder";
        return false;
    }

    // 遍历文件夹，获取最大的版本号
    long now_time = Common::get_timestamp();
    for (auto &store_iter : std::filesystem::directory_iterator(store_basic_path))
    {
        std::string filename = store_iter.path().filename();
        if (Common::IsDigit(filename))
        {
            long version = atol(filename.c_str());
            if (now_time - version > 600)
            {
                if (!find || version > latest_version)
                {
                    // 版本号大于等于新的版本号
                    latest_version = version;
                    find = true;
                }
            }
        }
    }

    // 如果没有找到
    if (!find)
    {
        result = "store_basic_path" + store_basic_path.string() + " is empty";
        return false;
    }

    // 返回结果
    result = std::to_string(latest_version);
    return true;
}

bool AnnoyIndexCache::GetLatestSlice(
    const std::string &version_path,
    std::vector<std::string> &vec_slices,
    std::string &result)
{
    auto store_version_path = std::filesystem::path(version_path);

    // 如果不存在, 返回错误
    if (!std::filesystem::exists(store_version_path))
    {
        result = "store_version_path " + store_version_path.string() + " not exists";
        return false;
    }

    // 如果存在, 但是不是文件夹, 那只能返回错误
    if (!std::filesystem::is_directory(store_version_path))
    {
        // 路径必须是文件夹
        result = "store_version_path" + store_version_path.string() + " must be a folder";
        return false;
    }

    static const std::string slice_prefix = "res_type_";
    size_t slice_prefix_size = slice_prefix.size();

    // 遍历文件夹
    vec_slices.clear();
    for (auto &store_iter : std::filesystem::directory_iterator(store_version_path))
    {
        std::string filename = store_iter.path().filename();
        if (filename.size() > slice_prefix_size)
        {
            if (filename.substr(0, slice_prefix_size) == slice_prefix)
            {
                vec_slices.push_back(filename);
            }
        }
    }

    return true;
}

void AnnoyIndexCache::OnTimer()
{
    static const long sleep_ms = 100;            // 每次睡 100 (ms)
    static const long interval_loop_count = 600; // 600*sleep_ms (ms)

    static long loop = 1; // 从1开始, 这样启动时不执行刷新
    while (g_Init)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));

        if (!g_Init)
        {
            break;
        }

        loop = (loop + 1) % interval_loop_count;
        if (loop == (interval_loop_count - 1))
        {
            Refresh();
        }

        if (!g_Init)
        {
            break;
        }
    }
}
