#pragma once
#include <vector>
#include <unordered_map>

#include "Common/Singleton.h"

#include "FilterConfig.h"

class RequestData;
class ResponseData;
class Filter_Interface;

class Filter : public Singleton<Filter>
{
public:
    const FilterParamData *GetFilterParamData(
        const RequestData &requestData,
        ResponseData &responseData);

    // 过滤统一入口
    // [in] requestData 请求数据
    // [out] responseData 返回数据
    int CallFilter(
        const FilterParamData *pFilterParamData,
        const RequestData &requestData,
        ResponseData &responseData);

protected:
    static std::shared_ptr<Filter_Interface> GetFilterInstance(
        const std::string &filterType);

public:
    Filter(token) {}
    ~Filter() {}
    Filter(Filter &) = delete;
    Filter &operator=(const Filter &) = delete;
};
