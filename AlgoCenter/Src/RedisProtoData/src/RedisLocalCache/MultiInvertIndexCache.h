#pragma once
#include "Common/CommonCache.h"
#include "Common/Singleton.h"
#include "../RPD_Common.hpp"

class MultiInvertIndexCache
    : public CacheInterface,
      public Singleton<MultiInvertIndexCache>
{
    using Key = std::string;
    using Value = std::string;
    using MultiInvertIndexCacheData = HashCache<Key, Value>;

public:
    virtual int Init(const CacheParam &cacheParam) override;
    virtual int Refresh() override;
    virtual int RefreshIncr() override;
    virtual int ShutDown() override;

public:
    MultiInvertIndexCache(token) {}
    virtual ~MultiInvertIndexCache() {}
    MultiInvertIndexCache(const MultiInvertIndexCache &) = delete;
    MultiInvertIndexCache &operator=(const MultiInvertIndexCache &) = delete;

public:
    // 批量获取倒排索引数据
    int BatchGetMultiInvertIndexCache(
        const RPD_Common::RPD_Type &type,
        const std::vector<Key> &vecSliceId,
        std::vector<Value> &vecProtoData) const;

protected:
    // 更新数据函数
    int RefreshMultiInvertIndexData(RPD_Common::RPD_Type type);

protected:
    std::unordered_map<RPD_Common::RPD_Type, MultiInvertIndexCacheData *> m_typeCachePtr;
};
