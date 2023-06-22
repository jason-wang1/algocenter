#include "DownloadFilter.h"
#include "glog/logging.h"

#include "Common/Function.h"

int DownloadFilter::CallStrategy(
    const RequestData &requestData,
    const FilterParam &filterParams,
    ResponseData &responseData)
{
    if (responseData.items_list.size() <= 0 &&
        responseData.spare_items_list.size() <= 0)
    {
        LOG(WARNING) << "DownloadFilter::CallStrategy() item is empty"
                     << ", apiType" << requestData.apiType
                     << ", filterExpID = " << requestData.filterExpID
                     << ", uuid = " << requestData.uuid
                     << ", user_id = " << requestData.user_id;
        return Common::Error::OK;
    }

    // 有效时间
    int validity_time = filterParams.GetValidityTime();
    if (validity_time <= 0)
    {
        LOG(ERROR) << "DownloadFilter::CallStrategy() ValidityTime is zero."
                   << ", apiType" << requestData.apiType
                   << ", exp_id = " << requestData.displayExpID;
        return Common::Error::Filter_GetParamsError;
    }

    auto ufd = requestData.GetUserFeature().GetUserFeatureDownload();
    if (ufd == nullptr)
    {
#ifdef DEBUG
        LOG(INFO) << "[DEBUG] DownloadFilter::CallStrategy() UserFeatureDownload is nullptr"
                  << ", apiType" << requestData.apiType
                  << ", filterExpID = " << requestData.filterExpID
                  << ", uuid = " << requestData.uuid
                  << ", user_id = " << requestData.user_id;
#endif
        return Common::Error::OK;
    }

    const auto &ufd_download_item = ufd->download_item();
    auto iter = ufd_download_item.find(requestData.context_res_type);
    if (iter == ufd_download_item.end())
    {
#ifdef DEBUG
        LOG(INFO) << "[DEBUG] DownloadFilter::CallStrategy() ResType Download is empty"
                  << ", apiType" << requestData.apiType
                  << ", filterExpID = " << requestData.filterExpID
                  << ", uuid = " << requestData.uuid
                  << ", user_id = " << requestData.user_id;
#endif
        return Common::Error::OK;
    }

    auto &download_list = iter->second.id_list();
    if (download_list.size() == 0)
    {
#ifdef DEBUG
        LOG(INFO) << "[DEBUG] DownloadFilter::CallStrategy() Download List is empty"
                  << ", apiType" << requestData.apiType
                  << ", filterExpID = " << requestData.filterExpID
                  << ", uuid = " << requestData.uuid
                  << ", user_id = " << requestData.user_id;
#endif
        return Common::Error::OK;
    }

#ifdef DEBUG
    std::stringstream downloadDataInfo;
#endif
    long now_time = Common::get_timestamp();
    std::unordered_set<std::string> download_items_set;
    for (const auto &id_time : download_list)
    {
        if (now_time - id_time.timestamp() < validity_time)
        {
            std::string item_key = std::to_string(id_time.id()) + "_" + std::to_string(requestData.context_res_type);
            download_items_set.insert(item_key);

#ifdef DEBUG
            if (!downloadDataInfo.str().empty())
            {
                downloadDataInfo << ", ";
            }
            downloadDataInfo << item_key;
#endif
        }
    }
#ifdef DEBUG
    LOG(INFO) << "[DEBUG] DownloadFilter::CallStrategy() Download List"
              << ", apiType" << requestData.apiType
              << ", filterExpID = " << requestData.filterExpID
              << ", uuid = " << requestData.uuid
              << ", user_id = " << requestData.user_id
              << ", List = [" << downloadDataInfo.str() << "]";
#endif

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
            std::string item_key = std::to_string(item_info.item_id) + "_" + std::to_string(item_info.res_type);
            if (download_items_set.find(item_key) != download_items_set.end())
            {
                if (!filterInfo.str().empty())
                {
                    filterInfo << ", ";
                }
                filterInfo << item_key;

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
        LOG(INFO) << "DownloadFilter::CallStrategy() Download Filter List"
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
