#include "Filter.h"
#include <unordered_map>
#include "glog/logging.h"

#include "UnfeaturedFilter.h"
#include "DownloadFilter.h"

static std::unordered_map<std::string, std::shared_ptr<Filter_Interface>> TypeToStrategyList = {
    {Common::FilterType_Unfeatured, UnfeaturedFilter::GetInstance()},
    {Common::FilterType_Download, DownloadFilter::GetInstance()},
};

const FilterParamData *Filter::GetFilterParamData(
    const RequestData &requestData,
    ResponseData &responseData)
{
    return FilterConfig::GetInstance()->GetFilterParamData(
        requestData.userGroupList, requestData.apiType, requestData.filterExpID);
}

int Filter::CallFilter(
    const FilterParamData *pFilterParamData,
    const RequestData &requestData,
    ResponseData &responseData)
{
    // 遍历过滤参数列表, 调用过滤策略
    for (const auto &filterParams : pFilterParamData->vecFilterParams)
    {
        std::string filterType = filterParams.GetFilterType();
        if (filterType == Common::FilterType_ErrorType)
        {
            LOG(ERROR) << "Filter::CallFilter() Get Filter Type error"
                       << ", apiType" << requestData.apiType
                       << ", filterExpID = " << requestData.filterExpID;
            return Common::Error::Filter_Error;
        }

        auto filterInst = GetFilterInstance(filterType);
        if (filterInst == nullptr || filterInst.get() == nullptr)
        {
            LOG(ERROR) << "Filter::CallFilter() Get Filter Inst error"
                       << ", apiType" << requestData.apiType
                       << ", filterExpID = " << requestData.filterExpID
                       << ", filterType = " << filterType;
            return Common::Error::Filter_GetStrategyInstError;
        }

        // 调用策略
        int err = filterInst->CallStrategy(requestData, filterParams, responseData);
        if (Common::Error::OK != err)
        {
            LOG(WARNING) << "Filter::CallFilter(), Call Strategy Failed"
                         << ", apiType" << requestData.apiType
                         << ", uuid = " << requestData.uuid
                         << ", user_id = " << requestData.user_id
                         << ", filterExpID = " << requestData.filterExpID
                         << ", filterType = " << filterType
                         << ", err = " << err;
        }
#ifdef DEBUG
        else
        {
            LOG(INFO) << "[DEBUG] Filter::CallFilter() Call Strategy success"
                      << ", apiType" << requestData.apiType
                      << ", uuid = " << requestData.uuid
                      << ", user_id = " << requestData.user_id
                      << ", filterExpID = " << requestData.filterExpID
                      << ", filterType = " << filterType;
        }
#endif
    }

    // 判断是否使用备用Item
    int items_size = responseData.items_list.size();
    if (items_size < pFilterParamData->KeepItemNum)
    {
        for (const auto &item_info : responseData.spare_items_list)
        {
            responseData.items_list.emplace_back(item_info);
        }

        // 数据不足，补充备用Item
        LOG(WARNING) << "Filter::CallFilter() data not enough after filter"
                     << ", apiType" << requestData.apiType
                     << ", uuid = " << requestData.uuid
                     << ", user_id = " << requestData.user_id
                     << ", recallExpID = " << requestData.recallExpID
                     << ", filterExpID = " << requestData.filterExpID
                     << ", displayExpID = " << requestData.displayExpID
                     << ", before = " << items_size
                     << ", after = " << responseData.items_list.size();
    }

    StatisLogData::OutStatisLog(requestData, responseData, "After Filter", {0}, false, true);

    return Common::Error::OK;
}

std::shared_ptr<Filter_Interface> Filter::GetFilterInstance(
    const std::string &filterType)
{
    std::shared_ptr<Filter_Interface> pFilterInst = nullptr;
    auto iter = TypeToStrategyList.find(filterType);
    if (iter != TypeToStrategyList.end())
    {
        pFilterInst = iter->second;
    }

    return pFilterInst;
}
