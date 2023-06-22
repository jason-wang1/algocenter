#include "UnfeaturedFilter.h"
#include "glog/logging.h"

#include "Common/Function.h"

int UnfeaturedFilter::CallStrategy(
    const RequestData &requestData,
    const FilterParam &filterParams,
    ResponseData &responseData)
{
    if (responseData.items_list.size() <= 0 &&
        responseData.spare_items_list.size() <= 0)
    {
        LOG(WARNING) << "UnfeaturedFilter::CallStrategy() item is empty"
                     << ", apiType" << requestData.apiType
                     << ", filterExpID = " << requestData.filterExpID
                     << ", uuid = " << requestData.uuid
                     << ", user_id = " << requestData.user_id;
        return Common::Error::OK;
    }

    auto &item_feature = requestData.itemFeature;

    auto lambda_filter =
        [&](std::vector<ItemInfo> &vecItemList,
            std::stringstream &filterInfo,
            int &filterNum) -> void
    {
        auto originalItemList = std::move(vecItemList);

        vecItemList.clear();
        vecItemList.reserve(originalItemList.size());
        for (auto &item_info : originalItemList)
        {
            auto item_basic = item_feature.GetItemFeatureBasic(item_info.item_id, item_info.res_type);
            auto item_statis = item_feature.GetItemFeatureStatis(item_info.item_id, item_info.res_type);
            if (item_basic == nullptr && item_statis == nullptr)
            {
                if (!filterInfo.str().empty())
                {
                    filterInfo << ", ";
                }
                filterInfo << item_info.item_id;

                filterNum++;
                continue;
            }

            vecItemList.emplace_back(std::move(item_info));
        }
    };

    // 1. 过滤召回数据
    int filter_num = 0;
    std::stringstream filter_info;
    lambda_filter(responseData.items_list, filter_info, filter_num);

    // 2. 过滤备用召回数据
    lambda_filter(responseData.spare_items_list, filter_info, filter_num);

    // 3. 过滤独立召回数据
    std::stringstream exclusive_size_info;
    for (auto iter = responseData.exclusive_items_list.begin();
         iter != responseData.exclusive_items_list.end(); iter++)
    {
        lambda_filter(iter->second, filter_info, filter_num);

        if (!exclusive_size_info.str().empty())
        {
            exclusive_size_info << ", ";
        }
        exclusive_size_info << iter->first << ":" << iter->second.size();
    }

    if (!filter_info.str().empty() && filterParams.GetFilterLogSwitch() > 0)
    {
        LOG(INFO) << "UnfeaturedFilter::CallStrategy() Unfeatured Filter List"
                  << ", apiType" << requestData.apiType
                  << ", filterExpID = " << requestData.filterExpID
                  << ", uuid = " << requestData.uuid
                  << ", user_id = " << requestData.user_id
                  << ", filter_num = " << filter_num
                  << ", item size = " << responseData.items_list.size()
                  << ", spare item size = " << responseData.spare_items_list.size()
                  << ", exclusive item size = [" << exclusive_size_info.str()
                  << "], Filter Info = {" << filter_info.str() << "}";
    }

    return Common::Error::OK;
}
