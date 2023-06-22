#pragma once
#include "Common/Singleton.h"
#include "Filter_Interface.h"

// 无特征过滤策略
class UnfeaturedFilter
    : public Singleton<UnfeaturedFilter>,
      public Filter_Interface
{
public:
    // CallStrategy 调用过滤策略接口
    virtual int CallStrategy(
        const RequestData &requestData,
        const FilterParam &filterParams,
        ResponseData &responseData) override;

public:
    UnfeaturedFilter(token) {}
    ~UnfeaturedFilter() {}
    UnfeaturedFilter(UnfeaturedFilter &) = delete;
    UnfeaturedFilter &operator=(const UnfeaturedFilter &) = delete;
};
