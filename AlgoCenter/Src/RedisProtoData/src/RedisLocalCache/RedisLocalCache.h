#pragma once
#include <atomic>
#include <thread>
#include "Common/CommonCache.h"

// 给Redis数据做本地缓存, 定时执行请求进行更新
class RedisLocalCache
{
public:
    // 初始化接口
    static int Init(const std::string &strCacheConfPath);
    static void ShutDown();

protected:
    static std::shared_ptr<CacheInterface> GetCahceInstance(const std::string &name);

    // 定时更新函数
    static void TimerUpdateFunc();
    static std::atomic<bool> g_init;
    static std::thread g_workThread;

private:
    RedisLocalCache() = delete;
    ~RedisLocalCache() = delete;
    RedisLocalCache(const RedisLocalCache &) = delete;
    RedisLocalCache &operator=(const RedisLocalCache &) = delete;
};
