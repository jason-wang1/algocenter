#pragma once
#include <iostream>

#include "RequestData.h"
#include "ResponseData.h"

class StatisLogData
{
public:
    static void OutStatisLog(
        const RequestData &requestData,
        const ResponseData &responseData,
        const std::string &describe,
        const std::vector<int> &topNList,
        bool spareSwitch = false,
        bool exclusiveSwitch = false)
    {
        if (!requestData.is_statis_log)
        {
            return;
        }

        auto lambda_statis_log =
            [&requestData, &describe](
                const int topN,
                const std::string &recallType,
                const std::vector<ItemInfo> &itemsList) -> void
        {
            std::stringstream statis_info;
            for (const auto &item_info : itemsList)
            {
                if (!statis_info.str().empty())
                {
                    statis_info << ",";
                }
                statis_info << item_info.item_id << "_" << item_info.res_type;
            }

            std::string topNInfo = topN > 0 ? " top" + std::to_string(topN) : "";

            LOG(INFO) << "OutStatisLog() Statis Log: " << describe << topNInfo
                      << ", recallType = " << recallType
                      << ", apiType" << requestData.apiType
                      << ", uuid = " << requestData.uuid
                      << ", user_id = " << requestData.user_id
                      << ", recallExpID = " << requestData.recallExpID
                      << ", filterExpID = " << requestData.filterExpID
                      << ", displayExpID = " << requestData.displayExpID
                      << ", recallUserGroup = " << requestData.recallUserGroup
                      << ", filterUserGroup = " << requestData.filterUserGroup
                      << ", displayUserGroup = " << requestData.displayUserGroup
                      << ", Item List = [" << statis_info.str()
                      << "], Item Num = " << itemsList.size();
        };

        for (const int top_n : topNList)
        {
            std::unordered_map<std::string, std::vector<ItemInfo>> statisMergeItems;

            int item_size = responseData.items_list.size();
            int use_item_size = top_n > 0 ? std::min(top_n, item_size) : item_size;
            for (int idx = 0; idx < use_item_size; idx++)
            {
                const auto &item_info = responseData.items_list[idx];
                statisMergeItems[item_info.recall_type].emplace_back(item_info);
            }

            for (auto iter = statisMergeItems.begin(); iter != statisMergeItems.end(); iter++)
            {
                lambda_statis_log(top_n, iter->first, iter->second);
            }
        }

        if (spareSwitch)
        {
            std::unordered_map<std::string, std::vector<ItemInfo>> spareStatisMergeItems;
            for (const auto &item_info : responseData.spare_items_list)
            {
                spareStatisMergeItems[item_info.recall_type].emplace_back(item_info);
            }

            for (auto iter = spareStatisMergeItems.begin(); iter != spareStatisMergeItems.end(); iter++)
            {
                lambda_statis_log(0, iter->first, iter->second);
            }
        }

        if (exclusiveSwitch)
        {
            for (auto iter = responseData.exclusive_items_list.begin();
                 iter != responseData.exclusive_items_list.end(); iter++)
            {
                lambda_statis_log(0, iter->first, iter->second);
            }
        }
    }
};