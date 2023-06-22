#pragma once
#include "Common/CommonCache.h"
#include "Common/Singleton.h"
#include "../RPD_Common.hpp"

class SingleInvertIndexCache
    : public CacheInterface,
      public Singleton<SingleInvertIndexCache>
{
    using Key = std::string;
    using Value = std::string;
    using SingleInvertIndexCacheData = HashCache<Key, Value>;

public:
    virtual int Init(const CacheParam &cacheParam) override;
    virtual int Refresh() override;
    virtual int RefreshIncr() override;
    virtual int ShutDown() override;

public:
    SingleInvertIndexCache(token) {}
    virtual ~SingleInvertIndexCache() {}
    SingleInvertIndexCache(const SingleInvertIndexCache &) = delete;
    SingleInvertIndexCache &operator=(const SingleInvertIndexCache &) = delete;

public:
    // 获取倒排索引数据
    int GetSingleInvertIndexCache(
        const RPD_Common::RPD_Type &type,
        const Key &slice,
        Value &protoData) const;

protected:
    // 更新数据函数
    int RefreshSingleInvertIndexData(RPD_Common::RPD_Type type);

protected:
    std::unordered_map<RPD_Common::RPD_Type, SingleInvertIndexCacheData *> m_typeCachePtr;
};
