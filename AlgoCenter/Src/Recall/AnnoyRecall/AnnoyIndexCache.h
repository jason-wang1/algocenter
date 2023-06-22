#pragma once
#include <map>
#include <atomic>
#include <thread>

#define _CRT_SECURE_NO_WARNINGS
#define ANNOYLIB_MULTITHREADED_BUILD
#include "annoy/annoylib.h"
#include "annoy/kissrandom.h"

#include "Common/Singleton.h"

#include "Protobuf/other/annoy_file_data.pb.h"

using namespace Annoy;

class AnnoyIndexCache : public Singleton<AnnoyIndexCache>
{
    using AnnoyIndexPtr = std::shared_ptr<AnnoyIndex<int, float, Angular, Kiss32Random, AnnoyIndexMultiThreadedBuildPolicy>>;
    using ItemDictPtr = std::shared_ptr<std::unordered_map<int, std::string>>;
    using ItemVectorPtr = std::shared_ptr<std::unordered_map<std::string, std::vector<float>>>;
    using KeynameVectorPtr = std::shared_ptr<std::unordered_map<std::string, std::vector<float>>>;
    struct AnnoyIndexData
    {
        size_t iEmbDimension;
        int iSearchNodeNum;

        AnnoyIndexPtr spAnnoyIndex;
        ItemDictPtr spItemDict;
        ItemVectorPtr spItemVector;
        KeynameVectorPtr spKeynameVector;
    };

    class CacheData
    {
    public:
        bool GetDBufData(const std::string &key, AnnoyIndexData &slice_data) const noexcept
        {
            const auto spDBuf = m_dBufCache[m_dataIdx];
            if (spDBuf != nullptr)
            {
                const auto it = spDBuf->find(key);
                if (it != spDBuf->end())
                {
                    slice_data = it->second;
                    return true;
                }
            }
            return false;
        }

        void SetDBufData(std::unordered_map<std::string, AnnoyIndexData> &&data) noexcept
        {
            std::lock_guard<std::mutex> ul(m_updateLock);
            int newDataIdx = (m_dataIdx + 1) % 2;

            // 直接生成新的存储空间, 旧空间引用计数归零后释放
            m_dBufCache[newDataIdx] = std::make_shared<std::unordered_map<std::string, AnnoyIndexData>>();
            auto spNewBuf = m_dBufCache[newDataIdx];
            auto &newBuf = *spNewBuf.get();
            newBuf = std::move(data);
            data.clear();

            m_dataIdx = newDataIdx;
        }

    public:
        // 数据时间戳, 每次修改数据时修改
        long GetTimestamp() const noexcept { return m_timestamp; }
        void SetTimestamp(long time) noexcept { m_timestamp = time; }

    private:
        // 双缓冲
        std::mutex m_updateLock;
        std::atomic<int> m_dataIdx = 0;
        std::shared_ptr<std::unordered_map<std::string, AnnoyIndexData>> m_dBufCache[2];

        long m_timestamp = 0; // 当前缓存 unix时间戳
    };

public:
    AnnoyIndexCache(token) {}
    virtual ~AnnoyIndexCache() {}
    AnnoyIndexCache(const AnnoyIndexCache &) = delete;
    AnnoyIndexCache &operator=(const AnnoyIndexCache &) = delete;

public:
    bool Init(
        const std::string &basic_path,
        const size_t emb_dimension,
        const int search_node_num);

    void ShutDown();

    int32_t AnnoyRetrieval(
        const std::string &slice,
        const std::string &item_key,
        const std::vector<std::string> &vec_keynames,
        const int top_k,
        std::vector<std::string> &vec_near_items,
        std::vector<float> &vec_distances) const;

    int32_t Refresh();

private:
    bool GetLatestVersion(
        const std::string &basic_path,
        std::string &result);

    bool GetLatestSlice(
        const std::string &version_path,
        std::vector<std::string> &vec_slices,
        std::string &result);

private:
    void OnTimer();
    std::atomic<bool> g_Init = false;
    std::shared_ptr<std::thread> g_WorkThreadPtr;

private:
    std::string m_BasicPath;

    size_t m_EmbDimension = 0;
    int m_SearchNodeNum = 0;

    std::shared_ptr<CacheData> m_CacheDataPtr;
};
