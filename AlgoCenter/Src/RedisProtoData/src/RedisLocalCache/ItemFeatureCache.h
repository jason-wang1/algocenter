#pragma once
#include "Common/CommonCache.h"
#include "Common/Singleton.h"
#include "../RPD_Common.hpp"

class ItemFeatureCache final
    : public CacheInterface,
      public Singleton<ItemFeatureCache>
{
    using Key = std::string;
    using Value = RPDType2SPProto;
    using ItemFeatureCacheData = HashCache<Key, Value>;

public:
    virtual int Init(const CacheParam &cacheParam) override;
    virtual int Refresh() override;
    virtual int RefreshIncr() override;
    virtual int ShutDown() override;

public:
    ItemFeatureCache(token) {}
    virtual ~ItemFeatureCache() {}
    ItemFeatureCache(const ItemFeatureCache &) = delete;
    ItemFeatureCache &operator=(const ItemFeatureCache &) = delete;

public:
    int GetCacheProtoData(
        const std::vector<Key> &vecItemKeys,
        std::unordered_map<Key, Value> &featureProtoData) const;

protected:
    int GetBucketItemKeys(int bucket, std::unordered_set<Key> &setItemKeys);
    int RefreshBucket(int bucket, std::unordered_set<Key> &setItemKeys);

private:
    std::vector<ItemFeatureCacheData *> m_bucketCachePtr;
};
