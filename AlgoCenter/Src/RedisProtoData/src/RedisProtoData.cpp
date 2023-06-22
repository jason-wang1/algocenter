#include "include/RedisProtoData.h"
#include <map>
#include "glog/logging.h"
#include "RPD_Common.hpp"
#include <unordered_map>
#include "TDRedis/TDRedis.h"
#include "TDRedis/TDRedisConnPool.h"

#include "RedisLocalCache/RedisLocalCache.h"
#include "RedisLocalCache/ItemFeatureCache.h"
#include "RedisLocalCache/SingleInvertIndexCache.h"
#include "RedisLocalCache/MultiInvertIndexCache.h"

bool RedisProtoData::AddConnectPool(
    const std::string &redisName,
    const std::string &ip,
    const int port,
    const bool crypto,
    const std::string &password,
    const int index,
    const int initPoolSize)
{
    return TDRedisConnPool::GetInstance()->AddConnPool(
        redisName, ip, port,
        crypto, password,
        index, initPoolSize);
}

// 停止连接池任务
void RedisProtoData::ShutDownConnectPool()
{
    TDRedisConnPool::GetInstance()->ShutDown();
}

int RedisProtoData::InitCache(const std::string &strCacheConfPath)
{
    return RedisLocalCache::Init(strCacheConfPath);
}

void RedisProtoData::ShutDownCache()
{
    RedisLocalCache::ShutDown();
    LOG(INFO) << "RedisLocalCache::ShutDown() finish.";
}

int RedisProtoData::GetUserFeatureProto(
    const long user_id,
    const std::vector<RPD_Common::RPD_Type> &vec_type,
    RPDType2SPProto &type_proto) noexcept
{
    const std::string &RedisName = RPD_Common::g_UserFeatureData;
    auto tdRedisPtr = TDRedisConnPool::GetInstance()->GetConnect(RedisName);
    if (tdRedisPtr == nullptr)
    {
        LOG(ERROR) << "GetUserFeatureProto() GetConnect Failed"
                   << ", RedisName = " << RedisName;
        return Common::Error::RPD_GetConnFailed;
    }

    // 使用PipeLine依次请求需要更新的数据
    for (auto type : vec_type)
    {
        std::string key = RPD_Common::GetUserKey(type, user_id);
        std::string field = RPD_Common::GetUserField(type, user_id);
        TDRedis::Error err = tdRedisPtr->PipeHGET(key, field);
        if (err != TDRedis::Error::OK)
        {
            LOG(ERROR) << "GetUserFeatureProto() PipeHGET Failed"
                       << ", type = " << type
                       << ", key = " << key
                       << ", field = " << field
                       << ", err = " << err;
            return Common::Error::RPD_RequestFailed;
        }
    }

    // 依次获取返回值
    for (auto type : vec_type)
    {
        std::string protoData;
        TDRedis::Error err = tdRedisPtr->PipeHGETRet(protoData);
        if (err != TDRedis::Error::OK)
        {
            LOG(ERROR) << "GetUserFeatureProto() PipeHGETRet Failed"
                       << ", type = " << type
                       << ", user_id = " << user_id
                       << ", err = " << err;
            return Common::Error::RPD_RequestFailed;
        }

        if (!protoData.empty())
        {
            // 解析Proto数据
            auto spProto = RPD_Common::GetSPProto(type);
            if (spProto == nullptr)
            {
                LOG(ERROR) << "GetUserFeatureProto() GetSPProto is nullptr"
                           << ", type = " << type
                           << ", user_id = " << user_id;
                return Common::Error::RPD_GetProtoFailed;
            }

            if (!spProto->ParseFromString(protoData))
            {
                LOG(ERROR) << "GetUserFeatureProto() ParseFromString Failed"
                           << ", type = " << type
                           << ", user_id = " << user_id;
                return Common::Error::RPD_ProtoParseFailed;
            }

            type_proto[type] = spProto;
        }
        else
        {
            type_proto[type] = nullptr;

#ifdef DEBUG
            // DEBUG 模式下判断是否获取到数据
            LOG(WARNING) << "[DEBUG] GetUserFeatureProto() GetProtoDataEmpty"
                         << ", type = " << type
                         << ", key = " << RPD_Common::GetUserKey(type, user_id)
                         << ", field = " << RPD_Common::GetUserField(type, user_id);
#endif
        }
    }
    return Common::Error::OK;
}

int RedisProtoData::GetItemFeature(
    const std::vector<std::string> &vecItemKeys,
    std::unordered_map<std::string, RPDType2SPProto> &item_type_proto) noexcept
{
    if (vecItemKeys.empty())
    {
        return Common::Error::OK;
    }

    return ItemFeatureCache::GetInstance()->GetCacheProtoData(vecItemKeys, item_type_proto);
}

int RedisProtoData::GetResTypeHotInvertIndex(
    const int res_type,
    std::string &protoData) noexcept
{
    std::string slice = "_" + std::to_string(res_type);
    return SingleInvertIndexCache::GetInstance()->GetSingleInvertIndexCache(
        RPD_Common::RPD_Type::RPD_ResTypeHotInvertIndex, slice, protoData);
}

int RedisProtoData::GetResTypeQualityInvertIndex(
    const int res_type,
    std::string &protoData) noexcept
{
    std::string slice = "_" + std::to_string(res_type);
    return SingleInvertIndexCache::GetInstance()->GetSingleInvertIndexCache(
        RPD_Common::RPD_Type::RPD_ResTypeQualityInvertIndex, slice, protoData);
}

int RedisProtoData::GetResTypeSurgeInvertIndex(
    const int res_type,
    std::string &protoData) noexcept
{
    std::string slice = "_" + std::to_string(res_type);
    return SingleInvertIndexCache::GetInstance()->GetSingleInvertIndexCache(
        RPD_Common::RPD_Type::RPD_ResTypeSurgeInvertIndex, slice, protoData);
}

int RedisProtoData::GetResTypeCTCVRInvertIndex(
    const int res_type,
    std::string &protoData) noexcept
{
    std::string slice = "_" + std::to_string(res_type);
    return SingleInvertIndexCache::GetInstance()->GetSingleInvertIndexCache(
        RPD_Common::RPD_Type::RPD_ResTypeCTCVRInvertIndex, slice, protoData);
}

int RedisProtoData::GetResTypeDLRInvertIndex(
    const int res_type,
    std::string &protoData) noexcept
{
    std::string slice = "_" + std::to_string(res_type);
    return SingleInvertIndexCache::GetInstance()->GetSingleInvertIndexCache(
        RPD_Common::RPD_Type::RPD_ResTypeDLRInvertIndex, slice, protoData);
}
