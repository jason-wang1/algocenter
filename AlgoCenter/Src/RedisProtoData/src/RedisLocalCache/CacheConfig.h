#pragma once
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include "Common/Singleton.h"
#include "Common/CommonCache.h"
#include "rapidjson/document.h"

struct CacheConfigData
{
public:
    // 缓存配置
    std::vector<CacheParam> vecCacheConfig;

    // UpdateTime
    long UpdateTimeMinSleep;         // 刷新时间间隔
    long UpdateTimeMaxLoop;          // 最大更新循环点
    std::vector<long> vecUpdateTime; // 刷新时间点
};

class CacheConfig : public Singleton<CacheConfig>
{
public:
    typedef enum CacheConfigError
    {
        OK = 0,
        JsonDataEmpty = 1,
        ParseError,
        DecodeCacheParamError,
        DecodeUpdateTimeError,
    } Error;

    int Init(const std::string &filename);

    std::vector<CacheParam> GetCacheParam() const noexcept
    {
        return m_data[m_dataIdx].vecCacheConfig;
    }

    long GetUpdateTimeMinSleep() const noexcept
    {
        return m_data[m_dataIdx].UpdateTimeMinSleep;
    }

    long GetUpdateTimeMaxLoop() const noexcept
    {
        return m_data[m_dataIdx].UpdateTimeMaxLoop;
    }

    std::vector<long> GetUpdateTime() const noexcept
    {
        return m_data[m_dataIdx].vecUpdateTime;
    }

public:
    CacheConfig(token) { m_dataIdx = 0; }
    virtual ~CacheConfig() {}
    CacheConfig(CacheConfig &) = delete;
    CacheConfig &operator=(const CacheConfig &) = delete;

protected:
    int DecodeJsonData(const std::string &jsonData);
    int DecodeCacheParam(rapidjson::Document &doc, int dataIdx);
    int DecodeUpdateTime(rapidjson::Document &doc, int dataIdx);

private:
    std::atomic<int> m_dataIdx;
    CacheConfigData m_data[2];
};
