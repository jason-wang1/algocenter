#include "ItemFeatureCache.h"
#include "glog/logging.h"
#include "TDRedis/TDRedis.h"
#include "TDRedis/TDRedisConnPool.h"

static const std::vector<RPD_Common::RPD_Type> VecItemBucketCacheType = {
    RPD_Common::RPD_Type::RPD_ItemFeatureBasic,
    RPD_Common::RPD_Type::RPD_ItemFeatureStatis,
    RPD_Common::RPD_Type::RPD_ItemFeatureIndex,
};

int ItemFeatureCache::Init(const CacheParam &cacheParam) // 初始化
{
    // 初始化存储对象
    m_bucketCachePtr.resize(RPD_Common::s_ItemFeatureBucketNum);
    for (int idx = 0; idx < RPD_Common::s_ItemFeatureBucketNum; idx++)
    {
        m_bucketCachePtr[idx] = new ItemFeatureCacheData();
    }

    return Refresh();
}

// 完全更新函数
int ItemFeatureCache::Refresh()
{
    return Common::Error::OK;
}

// 缓冲刷新函数
int ItemFeatureCache::RefreshIncr()
{
    static int bucket = 0;
    std::unordered_set<Key> setItemKeys;
    GetBucketItemKeys(bucket, setItemKeys);
    RefreshBucket(bucket, setItemKeys);
    bucket = (bucket + 1) % RPD_Common::s_ItemFeatureBucketNum;
    return Common::Error::OK;
}

int ItemFeatureCache::ShutDown()
{
    for (auto &item : m_bucketCachePtr)
    {
        if (item != nullptr)
        {
            delete item;
            item = nullptr;
        }
    }
    m_bucketCachePtr.clear();
    return Common::Error::OK;
}

// 批量获取接口
int ItemFeatureCache::GetCacheProtoData(
    const std::vector<Key> &vecItemKeys,
    std::unordered_map<Key, Value> &featureProtoData) const
{
    if (m_bucketCachePtr.size() != RPD_Common::s_ItemFeatureBucketNum)
    {
        LOG(ERROR) << "GetCacheProtoData() Cache Not Init!!!";
        return Common::Error::RPDCache_InvalidCacheType;
    }

    std::unordered_set<std::string> missItemKeys; // 未命中的Id, 需要Pipe获取

    for (const auto &item_key : vecItemKeys)
    {
        long item_id = atol(item_key.substr(0, item_key.find_first_of("_")).c_str());
        int bucket = item_id % RPD_Common::s_ItemFeatureBucketNum;
        ItemFeatureCacheData *pCacheData = m_bucketCachePtr[bucket];
        if (pCacheData == nullptr)
        {
            LOG(ERROR) << "GetCacheProtoData() Invalid Bucket Cache, bucket = " << bucket;
            return Common::Error::RPDCache_InvalidCacheType;
        }

        auto &protoData = featureProtoData[item_key];
        if (!pCacheData->GetCacheData(item_key, protoData))
        {
            missItemKeys.insert(item_key);
        }
    }

    // 没有缓存不命中的情况, 返回OK
    if (missItemKeys.empty())
    {
        return Common::Error::OK;
    }

    // 缓存不命中, 获取Id所有缓存, 写入缓存中, 再返回回去

    // 将Id分桶
    std::vector<std::vector<Key>> vec_bucket_items;
    vec_bucket_items.resize(RPD_Common::s_ItemFeatureBucketNum);
    for (const auto &item_key : missItemKeys)
    {
        long item_id = atol(item_key.substr(0, item_key.find_first_of("_")).c_str());
        int bucket = item_id % RPD_Common::s_ItemFeatureBucketNum;
        vec_bucket_items[bucket].emplace_back(item_key);
    }

    // 获取Redis指针
    const std::string RedisName = RPD_Common::g_ItemFeatureData;
    auto tdRedisPtr = TDRedisConnPool::GetInstance()->GetConnect(RedisName);
    if (tdRedisPtr == nullptr)
    {
        LOG(ERROR) << "GetCacheProtoData() GetConnect Failed"
                   << ", RedisName = " << RedisName;
        return Common::Error::RPD_GetConnFailed;
    }

    // 针对每个类型, 分桶请求
    for (int bucket = 0; bucket < RPD_Common::s_ItemFeatureBucketNum; bucket++)
    {
        const auto &bucket_items = vec_bucket_items[bucket];
        const int bucket_items_size = bucket_items.size();
        if (bucket_items_size == 0)
        {
            continue;
        }

        for (const auto type : VecItemBucketCacheType)
        {
            std::vector<std::string> vec_fields(bucket_items_size);
            for (int idx = 0; idx < bucket_items_size; idx++)
            {
                const std::string &item_key = bucket_items[idx];
                long item_id = atol(item_key.substr(0, item_key.find_first_of("_")).c_str());
                std::string res_type = item_key.substr(item_key.find_first_of("_") + 1);
                vec_fields[idx] = RPD_Common::GetItemField(type, item_id, res_type);
            }

            const std::string key = RPD_Common::GetItemKey(type, bucket);
            const int err = tdRedisPtr->PipeHMGET(key, vec_fields);
            if (err != Common::Error::OK)
            {
                LOG(ERROR) << "GetCacheProtoData() PipeHMGET Failed"
                           << ", key = " << key
                           << ", err = " << err;
                return Common::Error::RPD_RequestFailed;
            }
        }
    }

    // 组织好数据, 放到不同的桶内
    for (int bucket = 0; bucket < RPD_Common::s_ItemFeatureBucketNum; bucket++)
    {
        const auto &bucket_items = vec_bucket_items[bucket];
        const int bucket_items_size = bucket_items.size();
        if (bucket_items_size == 0)
        {
            continue;
        }

        // Pipe 获取返回值
        std::unordered_map<Key, Value> bucketSPProto;
        for (const auto type : VecItemBucketCacheType)
        {
            // 获取返回结果
            std::vector<std::string> bucket_proto_data;
            const int err = tdRedisPtr->PipeHMGETRet(bucket_proto_data);
            if (err != Common::Error::OK)
            {
                LOG(ERROR) << "GetCacheProtoData() PipeHMGETRet Failed"
                           << ", err = " << err;
                return Common::Error::RPD_RequestFailed;
            }

            // 确定返回数据没问题
            const int bucket_proto_data_size = bucket_proto_data.size();
            if (bucket_proto_data_size != bucket_items_size)
            {
                LOG(ERROR) << "GetCacheProtoData() check (bucket_proto_data_size != bucket_items_size)"
                           << ", type = " << type
                           << ", idx = " << bucket;
                return Common::Error::RPD_Error;
            }

            // 将返回数据写入
            for (int idx = 0; idx < bucket_proto_data_size; idx++)
            {
                const Key &item_key = bucket_items[idx];
                const std::string &proto_data = bucket_proto_data[idx];
                if (!proto_data.empty())
                {
                    auto spProto = RPD_Common::GetSPProto(type);
                    if (spProto == nullptr)
                    {
                        LOG(ERROR) << "GetCacheProtoData() GetSPProto is nullptr"
                                   << ", type = " << type;
                        return Common::Error::RPD_ProtoParseFailed;
                    }

                    if (!spProto->ParseFromString(proto_data))
                    {
                        LOG(ERROR) << "GetCacheProtoData() ParseFromString Failed"
                                   << ", type = " << type
                                   << ", item_key = " << item_key;
                        return Common::Error::RPD_ProtoParseFailed;
                    }

                    bucketSPProto[item_key][type] = spProto;
                    featureProtoData[item_key][type] = spProto;
                }
                else
                {
                    bucketSPProto[item_key][type] = nullptr;
                    featureProtoData[item_key][type] = nullptr;
                }
            }
        }

        // 写入到缓存中
        auto cache = m_bucketCachePtr[bucket];
        cache->AddRWBufCacheData(bucketSPProto);
    }

    return Common::Error::OK;
}

// 获取一个桶内需要刷新的Id
int ItemFeatureCache::GetBucketItemKeys(
    int bucket, std::unordered_set<Key> &setItemKeys)
{
    auto cache = m_bucketCachePtr[bucket];
    cache->RefreshCache();
    setItemKeys = cache->GetContainKey();
    return Common::Error::OK;
}

int ItemFeatureCache::RefreshBucket(
    int bucket, std::unordered_set<Key> &setItemKeys)
{
    // 将Id分桶
    std::vector<Key> bucket_items;
    bucket_items.reserve(setItemKeys.size());
    for (const auto &item_key : setItemKeys)
    {
        long item_id = atol(item_key.substr(0, item_key.find_first_of("_")).c_str());
        int calc_bucket = item_id % RPD_Common::s_ItemFeatureBucketNum;
        if (calc_bucket == bucket)
        {
            bucket_items.push_back(item_key);
        }
    }

    const int bucket_items_size = bucket_items.size();
    if (bucket_items_size == 0)
    {
        return Common::Error::OK;
    }

    // 获取Redis指针
    const std::string RedisName = RPD_Common::g_ItemFeatureData;
    auto tdRedisPtr = TDRedisConnPool::GetInstance()->GetConnect(RedisName);
    if (tdRedisPtr == nullptr)
    {
        LOG(ERROR) << "GetCacheProtoData() GetConnect Failed"
                   << ", RedisName = " << RedisName;
        return Common::Error::RPD_GetConnFailed;
    }

    for (auto type : VecItemBucketCacheType)
    {
        std::vector<std::string> vec_fields(bucket_items_size);
        for (int idx = 0; idx < bucket_items_size; idx++)
        {
            const std::string &item_key = bucket_items[idx];
            long item_id = atol(item_key.substr(0, item_key.find_first_of("_")).c_str());
            std::string res_type = item_key.substr(item_key.find_first_of("_") + 1);
            vec_fields[idx] = RPD_Common::GetItemField(type, item_id, res_type);
        }

        std::string key = RPD_Common::GetItemKey(type, bucket);
        int err = tdRedisPtr->PipeHMGET(key, vec_fields);
        if (err != Common::Error::OK)
        {
            LOG(ERROR) << "RefreshBucketItem() PipeHMGET Failed"
                       << ", key = " << key
                       << ", err = " << err;
            return Common::Error::RPD_RequestFailed;
        }
    }

    // Pipe 获取返回值
    std::unordered_map<Key, Value> bucketSPProto;
    for (auto type : VecItemBucketCacheType)
    {
        // 获取返回结果
        std::vector<std::string> bucket_proto_data;
        int err = tdRedisPtr->PipeHMGETRet(bucket_proto_data);
        if (err != Common::Error::OK)
        {
            LOG(ERROR) << "RefreshBucketItem() PipeHMGETRet Failed"
                       << ", err = " << err;
            return Common::Error::RPD_RequestFailed;
        }

        // 确定返回数据没问题
        const int bucket_proto_data_size = bucket_proto_data.size();
        if (bucket_proto_data_size != bucket_items_size)
        {
            LOG(ERROR) << "GetCacheProtoData() check (bucket_proto_data_size != bucket_items_size)"
                       << ", type = " << type
                       << ", idx = " << bucket;
            return Common::Error::RPD_Error;
        }

        // 将返回数据写入
        for (int idx = 0; idx < bucket_proto_data_size; idx++)
        {
            const Key &item_key = bucket_items[idx];
            const std::string &proto_data = bucket_proto_data[idx];
            if (!proto_data.empty())
            {
                auto spProto = RPD_Common::GetSPProto(type);
                if (spProto == nullptr)
                {
                    LOG(ERROR) << "GetCacheProtoData() GetSPProto is nullptr"
                               << ", type = " << type;
                    return Common::Error::RPD_ProtoParseFailed;
                }

                if (!spProto->ParseFromString(proto_data))
                {
                    LOG(ERROR) << "GetCacheProtoData() ParseFromString Failed"
                               << ", type = " << type
                               << ", item_key = " << item_key;
                    return Common::Error::RPD_ProtoParseFailed;
                }

                bucketSPProto[item_key][type] = spProto;
            }
            else
            {
                bucketSPProto[item_key][type] = nullptr;
            }
        }
    }

    // 写入到缓存中
    auto cache = m_bucketCachePtr[bucket];
    cache->AddDBufCacheData(std::move(bucketSPProto));

    return Common::Error::OK;
}
