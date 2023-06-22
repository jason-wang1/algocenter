#include "RedisLocalCache.h"
#include <future> // std::async, std::future
#include <chrono>
#include "glog/logging.h"
#include "CacheConfig.h"
#include "CacheType.h"

#include "ItemFeatureCache.h"
#include "SingleInvertIndexCache.h"
#include "MultiInvertIndexCache.h"

std::atomic<bool> RedisLocalCache::g_init(false);
std::thread RedisLocalCache::g_workThread;

std::shared_ptr<CacheInterface> RedisLocalCache::GetCahceInstance(const std::string &name)
{
    if (RPD_Common::CacheType_ItemFeature == name)
    {
        return ItemFeatureCache::GetInstance();
    }
    else if (RPD_Common::CacheType_SingleInvertIndex == name)
    {
        return SingleInvertIndexCache::GetInstance();
    }
    else if (RPD_Common::CacheType_MultiInvertIndex == name)
    {
        return MultiInvertIndexCache::GetInstance();
    }

    return nullptr;
}

int RedisLocalCache::Init(const std::string &strCacheConfPath)
{
    // 加载配置文件
    if (CacheConfig::Error::OK != CacheConfig::GetInstance()->Init(strCacheConfPath))
    {
        LOG(ERROR) << "RedisLocalCache::Init() Load Config Failed.";
        return Common::Error::RPD_ConfigInitError;
    }

    // 这里定义lambda函数是因为async绑定不了纯虚函数, 所以需要通过lambda函数来实现
    // 需要注意传引用 与 参数捕捉的区别
    auto lambda_init = [](std::shared_ptr<CacheInterface> spCacheInst,
                          const CacheParam &cacheParam) -> int
    { return spCacheInst->Init(cacheParam); };

    // 拉起线程初始化数据
    // 1. 通过名称获取 shared_ptr<CacheInterface> -> CacheInstance
    // 2. 调用async自动拉取线程更新数据
    // 3. 将future放入vector, 循环遍历获取更新结果
    std::unordered_map<std::string, std::future<int>> Name2Futures;
    auto vecCacheParam = CacheConfig::GetInstance()->GetCacheParam();
    for (const auto &cacheParam : vecCacheParam)
    {
        const std::string name = cacheParam.GetCacheName();
        auto spCacheInst = GetCahceInstance(name);
        if (spCacheInst != nullptr)
        {
            Name2Futures[name] = std::async(lambda_init, spCacheInst, cacheParam);
        }
        else
        {
            LOG(ERROR) << "RedisLocalCache::Init() Get Instance Failed. name = " << name;
            return Common::Error::RPDCache_InvalidCacheType;
        }
    }

    // 获取更新结果
    int retErr = Common::Error::OK;
    for (auto &item : Name2Futures)
    {
        int err = item.second.get();
        if (err != Common::Error::OK)
        {
#ifdef DEBUG
            LOG(ERROR) << "[DEBUG] RedisLocalCache::Init() Init "
                       << item.first << " Failed, err = " << err;
#endif
            retErr = err;
        }
        else
        {
            LOG(INFO) << "Cache Data Init Ok, name =  " << item.first;
        }
    }

    if (retErr == Common::Error::OK)
    {
        // 初始化成功, 拉起线程定时调用更新函数
        g_init = true;
        g_workThread = std::thread(RedisLocalCache::TimerUpdateFunc);
    }
    return retErr;
}

void RedisLocalCache::ShutDown()
{
    // 通知退出, 等待工作线程退出
    if (g_init)
    {
        g_init = false;
        if (g_workThread.joinable())
        {
            g_workThread.join();
            LOG(INFO) << "ShutDown() g_workThread.join() finish.";
        }

        auto vecCacheParam = CacheConfig::GetInstance()->GetCacheParam();
        for (auto &cacheParam : vecCacheParam)
        {
            const std::string name = cacheParam.GetCacheName();
            auto spCacheInst = GetCahceInstance(name);
            if (spCacheInst != nullptr)
            {
                spCacheInst->ShutDown();
                LOG(INFO) << "ShutDown() cache shutdown finish. name = " << name;
            }
        }
    }
}

void RedisLocalCache::TimerUpdateFunc()
{
    // 增加判断条件, 没有需要刷新的缓存, 那直接退出线程即可, 没必要空跑
    auto vecCacheParam = CacheConfig::GetInstance()->GetCacheParam();
    if (vecCacheParam.empty())
    {
        return;
    }

    const long UpdateTimeMinSleep = CacheConfig::GetInstance()->GetUpdateTimeMinSleep();
    if (UpdateTimeMinSleep <= 0)
    {
        LOG(ERROR) << "RedisLocalCache::TimerUpdateFunc() UpdateTimeMinSleep <= 0";
        return;
    }

    const long UpdateTimeMaxLoop = CacheConfig::GetInstance()->GetUpdateTimeMaxLoop();
    if (UpdateTimeMaxLoop <= 0)
    {
        LOG(ERROR) << "RedisLocalCache::TimerUpdateFunc() UpdateTimeMaxLoop <= 0";
        return;
    }

    const std::vector<long> vecRefreshTime = CacheConfig::GetInstance()->GetUpdateTime();
    if (vecRefreshTime.empty())
    {
        LOG(ERROR) << "RedisLocalCache::TimerUpdateFunc() vecRefreshTime is Empty.";
        return;
    }

    auto lambda_refresh = [](const long time) -> void
    {
        auto vecCacheParam = CacheConfig::GetInstance()->GetCacheParam();
        for (const auto &cacheParam : vecCacheParam)
        {
            if (!g_init)
            {
                break;
            }

            // 获取缓存实例
            const std::string name = cacheParam.GetCacheName();
            auto spCacheInst = GetCahceInstance(name);
            if (spCacheInst == nullptr)
            {
                continue;
            }

            // 比对时间, 如果对应则调用
            if (cacheParam.GetRefreshIncrTime() == time)
            {
                spCacheInst->RefreshIncr();
            }

            if (cacheParam.GetRefreshTime() == time)
            {
                spCacheInst->Refresh();
            }
        }
        return;
    };

    static long loop = 1; // 从1开始, 这样启动时不执行刷新
    while (g_init)
    {
        std::this_thread::sleep_for(std::chrono::seconds(UpdateTimeMinSleep));

        if (!g_init)
        {
            break;
        }

        for (const auto time : vecRefreshTime)
        {
            // 当前时间为time, 如果匹配中了则调用刷新函数
            if ((loop % time) == 0)
            {
                lambda_refresh(time);
            }

            // 随时判断退出情况, 及时退出
            if (!g_init)
            {
                break;
            }
        }

        // 每小时归零
        loop = (loop + 1) % UpdateTimeMaxLoop;
    }
}
