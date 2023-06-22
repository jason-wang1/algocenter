#pragma once
#include "Common/Singleton.h"
#include "Filter_Interface.h"

// 历史曝光过滤策略
class DownloadFilter
    : public Singleton<DownloadFilter>,
      public Filter_Interface
{
public:
    // CallStrategy 调用过滤策略接口
    virtual int CallStrategy(
        const RequestData &requestData,
        const FilterParam &filterParams,
        ResponseData &responseData) override;

public:
    DownloadFilter(token) {}
    ~DownloadFilter() {}
    DownloadFilter(DownloadFilter &) = delete;
    DownloadFilter &operator=(const DownloadFilter &) = delete;
};
