#include "SingleInvertIndexCache.h"
#include <thread>
#include <future>
#include "glog/logging.h"
#include "TDRedis/TDRedis.h"
#include "TDRedis/TDRedisConnPool.h"

static const std::vector<RPD_Common::RPD_Type> VecSingleInvertIndexCacheType = {
    RPD_Common::RPD_Type::RPD_ResTypeHotInvertIndex,
    RPD_Common::RPD_Type::RPD_ResTypeQualityInvertIndex,
    RPD_Common::RPD_Type::RPD_ResTypeSurgeInvertIndex,
    RPD_Common::RPD_Type::RPD_ResTypeCTCVRInvertIndex,
    RPD_Common::RPD_Type::RPD_ResTypeDLRInvertIndex,
};

int SingleInvertIndexCache::Init(const CacheParam &cacheParam)
{
    // 初始化对象, 更新数据
    for (auto type : VecSingleInvertIndexCacheType)
    {
        m_typeCachePtr[type] = new SingleInvertIndexCacheData();
    }

    return Refresh();
}

int SingleInvertIndexCache::Refresh()
{
    std::unordered_map<RPD_Common::RPD_Type, std::future<int>> type_refreshFutures;
    for (auto type : VecSingleInvertIndexCacheType)
    {
        type_refreshFutures[type] =
            std::async(&SingleInvertIndexCache::RefreshSingleInvertIndexData, this, type);
    }

    int retErr = Common::Error::OK;
    for (auto &item : type_refreshFutures)
    {
        int err = item.second.get();
        if (err != Common::Error::OK)
        {
            LOG(WARNING) << "Refresh() RefreshSingleInvertIndexData Failed"
                         << ", type = " << item.first << ", err = " << err;

            retErr = err;
        }
    }

    return retErr;
}

int SingleInvertIndexCache::RefreshIncr()
{
    return Common::Error::OK;
}

int SingleInvertIndexCache::ShutDown()
{
    for (auto &item : m_typeCachePtr)
    {
        if (item.second != nullptr)
        {
            delete item.second;
            item.second = nullptr;
        }
    }
    m_typeCachePtr.clear();
    return Common::Error::OK;
}

int SingleInvertIndexCache::RefreshSingleInvertIndexData(RPD_Common::RPD_Type type)
{
    // 获取缓存指针
    SingleInvertIndexCacheData *pCacheData = nullptr;
    auto iter = m_typeCachePtr.find(type);
    if (iter != m_typeCachePtr.end())
    {
        pCacheData = static_cast<decltype(pCacheData)>(iter->second);
    }
    if (pCacheData == nullptr)
    {
        LOG(ERROR) << "RefreshSingleInvertIndexData() Invalid RPD_Common::RPD_Type, type = " << type;
        return Common::Error::RPDCache_InvalidCacheType;
    }

    // 获取Redis指针
    const std::string &redisName = RPD_Common::g_InvertIndexData;
    auto tdRedisPtr = TDRedisConnPool::GetInstance()->GetConnect(redisName);
    if (tdRedisPtr == nullptr)
    {
        LOG(ERROR) << "RefreshSingleInvertIndexData() GetRedisConnPtr Failed, RedisName = " << redisName;
        return Common::Error::RPD_GetConnFailed;
    }

    // 获取本地时间戳
    long localTimeStamp = pCacheData->GetCacheTimeStamp();

    // 获取Redis时间戳
    long redisTimeStamp = 0;
    {
        std::string key = RPD_Common::GetBasicKey(RPD_Common::RPD_Type::RPD_InvertIndexTimeStamp);
        std::string field = RPD_Common::GetBasicKey(type);
        std::string retVal;
        TDRedis::Error err = tdRedisPtr->HGET(key, field, retVal);
        if (err != TDRedis::Error::OK)
        {
            LOG(ERROR) << "RefreshSingleInvertIndexData() HGET Failed"
                       << ", type = " << type << ", key = " << key
                       << ", field = " << field << ", err= " << err;
            return Common::Error::RPD_RequestFailed;
        }
        redisTimeStamp = atol(retVal.c_str());
    }

    // 判断更新数据条件
    if (redisTimeStamp != 0 &&
        localTimeStamp != 0 &&
        localTimeStamp == redisTimeStamp)
    {
        // 数据已经是最新, 直接返回OK即可
        return Common::Error::OK;
    }

    long cursor = 0;
    std::string key = RPD_Common::GetBasicKey(type);
    std::string key_pattern = key + "*";
    std::vector<std::string> data_keys_list;
    while (1)
    {
        std::vector<std::string> tmp_keys;
        TDRedis::Error err = tdRedisPtr->SCAN(cursor, key_pattern, 1000, cursor, tmp_keys);
        if (err != TDRedis::Error::OK)
        {
            LOG(ERROR) << "RefreshSingleInvertIndexData() SCAN Failed"
                       << ", key_pattern = " << key_pattern
                       << ", cursor = " << cursor;
            return err;
        }

        for (auto &item : tmp_keys)
        {
            data_keys_list.push_back(item);
        }

        if (cursor == 0)
        {
            break;
        }
    }

    std::unordered_map<std::string, std::string> mSingleIndexDatas;
    for (auto &data_key : data_keys_list)
    {
        // 发起请求
        std::string proto_data;
        TDRedis::Error err = tdRedisPtr->GET(data_key, proto_data);
        if (err != TDRedis::Error::OK)
        {
            LOG(ERROR) << "RefreshSingleInvertIndexData() GET Failed."
                       << " type = " << type << ", key = " << data_key << ", err= " << err;
            return Common::Error::RPD_RequestFailed;
        }

        mSingleIndexDatas[data_key] = proto_data;
    }

    // 将数据写入本地缓存
    pCacheData->AddDBufCacheData(std::move(mSingleIndexDatas));

    // 更新本地缓存时间戳
    pCacheData->SetCacheTimeStamp(redisTimeStamp);

    return Common::Error::OK;
}

int SingleInvertIndexCache::GetSingleInvertIndexCache(
    const RPD_Common::RPD_Type &type,
    const Key &slice,
    Value &protoData) const
{
    SingleInvertIndexCacheData *pCacheData = nullptr;
    auto iter = m_typeCachePtr.find(type);
    if (iter != m_typeCachePtr.end())
    {
        pCacheData = static_cast<decltype(pCacheData)>(iter->second);
    }
    if (pCacheData == nullptr)
    {
        LOG(ERROR) << "GetSingleInvertIndexCache() Invalid RPD_Type, type = " << type;
        return Common::Error::RPDCache_InvalidCacheType;
    }

    std::string data_key = RPD_Common::GetBasicKey(type) + slice;
    if (!pCacheData->GetCacheData(data_key, protoData))
    {
        protoData.clear();
    }

    return Common::Error::OK;
}
