#pragma once
#include <set>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "RedisProtoData_Def.h"

class RedisProtoData
{
public:
    // 初始化Redis连接池
    static bool AddConnectPool(
        const std::string &redisName,
        const std::string &ip,
        const int port,
        const bool crypto,
        const std::string &password,
        const int index,
        const int initPoolSize);

    // 停止连接池任务
    static void ShutDownConnectPool();

    // 初始化缓存
    static int InitCache(const std::string &strCacheConfPath);

    // 停止缓存任务
    static void ShutDownCache();

public:
    // 获取用户特征Proto
    static int GetUserFeatureProto(
        const long user_id,
        const std::vector<RPD_Common::RPD_Type> &vec_type,
        RPDType2SPProto &type_proto) noexcept;

public:
    // 获取物料特征Proto
    static int GetItemFeature(
        const std::vector<std::string> &vecItemKeys,
        std::unordered_map<std::string, RPDType2SPProto> &item_type_proto) noexcept;

public:
    // 获取类目热门倒排索引数据
    static int GetResTypeHotInvertIndex(
        const int res_type,
        std::string &protoData) noexcept;

    // 获取类目优质倒排索引数据
    static int GetResTypeQualityInvertIndex(
        const int res_type,
        std::string &protoData) noexcept;

    // 获取类目飙升倒排索引数据
    static int GetResTypeSurgeInvertIndex(
        const int res_type,
        std::string &protoData) noexcept;

    // 获取类目高点击率倒排索引数据
    static int GetResTypeCTCVRInvertIndex(
        const int res_type,
        std::string &protoData) noexcept;

    // 获取类目高下载率倒排索引数据
    static int GetResTypeDLRInvertIndex(
        const int res_type,
        std::string &protoData) noexcept;

private:
    // 禁止构造、析构、拷贝、赋值
    RedisProtoData() = delete;
    ~RedisProtoData() = delete;
    RedisProtoData(const RedisProtoData &) = delete;
    RedisProtoData &operator=(const RedisProtoData &) = delete;
};
