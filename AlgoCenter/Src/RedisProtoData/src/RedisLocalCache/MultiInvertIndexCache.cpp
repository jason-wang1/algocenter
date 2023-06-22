#include "MultiInvertIndexCache.h"
#include <thread>
#include <future>
#include "glog/logging.h"
#include "TDRedis/TDRedis.h"
#include "TDRedis/TDRedisConnPool.h"
#include "Common/Function.h"

static const std::vector<RPD_Common::RPD_Type> VecMultiInvertIndexType = {};

int MultiInvertIndexCache::Init(const CacheParam &cacheParam)
{
    // 初始化对象, 更新数据
    for (auto type : VecMultiInvertIndexType)
    {
        m_typeCachePtr[type] = new MultiInvertIndexCacheData();
    }

    return Refresh();
}

int MultiInvertIndexCache::Refresh()
{
    std::unordered_map<RPD_Common::RPD_Type, std::future<int>> type_refreshFutures;
    for (auto type : VecMultiInvertIndexType)
    {
        type_refreshFutures[type] =
            std::async(&MultiInvertIndexCache::RefreshMultiInvertIndexData, this, type);
    }

    int retErr = Common::Error::OK;
    for (auto &item : type_refreshFutures)
    {
        int err = item.second.get();
        if (err != Common::Error::OK)
        {
            LOG(WARNING) << "Refresh() RefreshMultiInvertIndexData Failed"
                         << ", type = " << item.first << ", err = " << err;
            retErr = err;
        }
    }

    return retErr;
}

int MultiInvertIndexCache::RefreshIncr()
{
    return Common::Error::OK;
}

int MultiInvertIndexCache::ShutDown()
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

int MultiInvertIndexCache::RefreshMultiInvertIndexData(RPD_Common::RPD_Type type)
{
    // 获取缓存指针
    MultiInvertIndexCacheData *pCacheData = nullptr;
    auto iter = m_typeCachePtr.find(type);
    if (iter != m_typeCachePtr.end())
    {
        pCacheData = static_cast<decltype(pCacheData)>(iter->second);
    }
    if (pCacheData == nullptr)
    {
        LOG(ERROR) << "RefreshMultiInvertIndexData() Invalid RPD_Common::RPD_Type, type = " << type;
        return Common::Error::RPDCache_InvalidCacheType;
    }

    // 获取Redis指针
    const std::string &redisName = RPD_Common::g_InvertIndexData;
    auto tdRedisPtr = TDRedisConnPool::GetInstance()->GetConnect(redisName);
    if (tdRedisPtr == nullptr)
    {
        LOG(ERROR) << "RefreshMultiInvertIndexData() GetRedisConnPtr Failed"
                   << ", RedisName = " << redisName;
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
            LOG(ERROR) << "RefreshMultiInvertIndexData() HGET Failed"
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
            LOG(ERROR) << "RefreshMultiInvertIndexData() SCAN Failed"
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

    // 查询
    std::unordered_map<std::string, std::string> mMultiIndexDatas;
    for (auto &data_key : data_keys_list)
    {
        cursor = 0;
        std::vector<std::string> tmp_fields;
        std::vector<std::string> tmp_values;
        while (1)
        {
            TDRedis::Error err = tdRedisPtr->HSCAN(data_key, cursor, "", 1000, cursor, tmp_fields, tmp_values);
            if (err != TDRedis::Error::OK)
            {
                LOG(ERROR) << "RefreshMultiInvertIndexData() HSCAN Failed"
                           << ", key = " << data_key
                           << ", cursor = " << cursor;
                return err;
            }

            for (size_t idx = 0; idx < tmp_fields.size() && idx < tmp_values.size(); idx++)
            {
                mMultiIndexDatas[data_key + "_" + tmp_fields[idx]] = tmp_values[idx];
            }

            if (cursor == 0)
            {
                break;
            }
        }
    }

    // 将数据写入本地缓存
    pCacheData->AddDBufCacheData(std::move(mMultiIndexDatas));

    // 更新本地缓存时间戳
    pCacheData->SetCacheTimeStamp(redisTimeStamp);

    return Common::Error::OK;
}

// 批量获取风格倒排索引接口
int MultiInvertIndexCache::BatchGetMultiInvertIndexCache(
    const RPD_Common::RPD_Type &type,
    const std::vector<Key> &vecSliceId,
    std::vector<Value> &vecProtoData) const
{
    MultiInvertIndexCacheData *pCacheData = nullptr;
    auto iter = m_typeCachePtr.find(type);
    if (iter != m_typeCachePtr.end())
    {
        pCacheData = static_cast<decltype(pCacheData)>(iter->second);
    }
    if (pCacheData == nullptr)
    {
        LOG(ERROR) << "BatchGetMultiInvertIndexCache() Invalid RPD_Type, type = " << type;
        return Common::Error::RPDCache_InvalidCacheType;
    }

    size_t slice_id_size = vecSliceId.size();
    vecProtoData.resize(slice_id_size);
    for (size_t idx = 0; idx < slice_id_size; idx++)
    {
        std::string data_key = RPD_Common::GetBasicKey(type) + vecSliceId[idx];
        auto &protoData = vecProtoData[idx];
        if (!pCacheData->GetCacheData(data_key, protoData))
        {
            protoData.clear();
        }
    }

    return Common::Error::OK;
}
