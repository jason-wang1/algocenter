#include "Recall.h"
#include "glog/logging.h"

#include "RecallInstance.hpp"

const RecallParamData *Recall::GetRecallParamData(
    const RequestData &requestData,
    ResponseData &responseData)
{
    return RecallConfig::GetInstance()->GetRecallParamData(
        requestData.userGroupList, requestData.apiType, requestData.recallExpID);
}

int Recall::CallRecall(
    const RecallParamData *pRecallParamData,
    const RequestData &requestData,
    ResponseData &responseData)
{
    double start_time = Common::get_ms_time();

    // 1. 召回
    auto lambda_async_recall =
        [&requestData](
            const std::vector<RecallParam> &vecRecallParam,
            std::unordered_map<std::string, std::vector<ItemInfo>> &recallTypeItemList) -> int
    {
        for (const auto &recallParams : vecRecallParam)
        {
            std::string recallType = recallParams.GetRecallType();
            long recallTypeId = Common::GetRecallTypeID(recallType);
            if (recallTypeId == Common::RT_ID::RTI_None)
            {
                LOG(ERROR) << "Recall::CallRecall() Get Recall Type Error"
                           << ", apiType" << requestData.apiType
                           << ", recallExpID = " << requestData.recallExpID;
                return Common::Error::Recall_GetParamsError;
            }

            auto &recallItemList = recallTypeItemList[recallType];
            recallItemList.clear();

            auto recallInstance = Common::GetRecallInstance(recallTypeId);
            int err = recallInstance->RecallItem(requestData, recallParams, recallItemList);
            if (Common::Error::OK != err)
            {
                LOG(WARNING) << "Recall::CallRecall() Recall Calc Failed"
                             << ", recallType = " << recallType
                             << ", apiType" << requestData.apiType
                             << ", recallExpID = " << requestData.recallExpID
                             << ", uuid = " << requestData.uuid
                             << ", user_id = " << requestData.user_id
                             << ", err = " << err;
            }
        }

        return Common::Error::OK;
    };

    // 1.1 遍历召回策略列表, 调用召回策略
    std::unordered_map<std::string, std::vector<ItemInfo>> recallTypeItemList;
    int recall_err = lambda_async_recall(
        pRecallParamData->vecRecallParams, recallTypeItemList);
    if (recall_err != Common::Error::OK)
    {
        return recall_err;
    }

    // 1.2 遍历备用召回策略列表, 调用召回策略
    std::unordered_map<std::string, std::vector<ItemInfo>> spareRecallTypeItemList;
    int spare_recall_err = lambda_async_recall(
        pRecallParamData->vecSpareRecallParams, spareRecallTypeItemList);
    if (spare_recall_err != Common::Error::OK)
    {
        return spare_recall_err;
    }

    // 1.3 遍历独立召回策略列表, 调用召回策略
    int exclusive_recall_err = lambda_async_recall(
        pRecallParamData->vecExclusiveRecallParams, responseData.exclusive_items_list);
    if (exclusive_recall_err != Common::Error::OK)
    {
        return exclusive_recall_err;
    }

    double recall_time = Common::get_ms_time();

    // 2. 融合
    std::unordered_set<long> recallMergeSet;

    // 2.1 召回融合
    int merge_err = ItemMerge(
        pRecallParamData->AllMergeNum, pRecallParamData->vecRecallParams,
        recallTypeItemList, recallMergeSet, responseData.items_list);
    if (Common::Error::OK != merge_err)
    {
        LOG(ERROR) << "Recall::CallRecall() ItemMerge Failed, merge_err = " << merge_err;
        return Common::Error::Recall_RecallCalcFailed;
    }

    // 2.2 备用召回融合
    int spare_merge_err = ItemMerge(
        pRecallParamData->AllSpareMergeNum, pRecallParamData->vecSpareRecallParams,
        spareRecallTypeItemList, recallMergeSet, responseData.spare_items_list);
    if (Common::Error::OK != spare_merge_err)
    {
        LOG(ERROR) << "Recall::CallRecall() ItemMerge Failed, spare_merge_err = " << spare_merge_err;
        return Common::Error::Recall_RecallCalcFailed;
    }

    // 2.3 独立召回去重
    std::unordered_set<long> exclusiveRecallItemSet;
    for (auto iter = responseData.exclusive_items_list.begin();
         iter != responseData.exclusive_items_list.end(); iter++)
    {
        auto originalItemList = std::move(iter->second);

        iter->second.clear();
        iter->second.reserve(originalItemList.size());

        for (auto &item_info : originalItemList)
        {
            if (exclusiveRecallItemSet.find(item_info.item_id) == exclusiveRecallItemSet.end())
            {
                iter->second.emplace_back(std::move(item_info));
            }
            else
            {
                exclusiveRecallItemSet.insert(item_info.item_id);
            }
        }
    }

    StatisLogData::OutStatisLog(requestData, responseData, "After Merge", {0}, true, true);
    double end_time = Common::get_ms_time();

    // 3. 输出调试日志
    if (requestData.is_statis_log)
    {
        LOG(INFO) << "Recall::CallRecall() Recall Time Info"
                  << ", apiType = " << requestData.apiType
                  << ", uuid = " << requestData.uuid
                  << ", user_id = " << requestData.user_id
                  << ", filterExpID = " << requestData.filterExpID
                  << ", recallExpID = " << requestData.recallExpID
                  << ", displayExpID = " << requestData.displayExpID
                  << ", Recall Time = " << recall_time - start_time << "ms"
                  << ", Merge Time = " << end_time - recall_time << "ms"
                  << ", All Recall Time = " << end_time - start_time << "ms.";
    }

    return Common::Error::OK;
}

int Recall::ItemMerge(
    const int allMergeNum,
    const std::vector<RecallParam> &vecRecallParams,
    const std::unordered_map<std::string, std::vector<ItemInfo>> &recallTypeItemList,
    std::unordered_set<long> &recallMergeSet,
    std::vector<ItemInfo> &resultItemList)
{
    // 融合所有渠道的最大房间数量
    std::size_t useAllMergeNum = static_cast<std::size_t>(allMergeNum);

    // 召回类型已融合数量
    std::unordered_map<std::string, std::size_t> recallTypeMergeNum;

    for (const auto &mergeParams : vecRecallParams)
    {
        auto recallType = mergeParams.GetRecallType();
        auto iter = recallTypeItemList.find(recallType);
        if (iter == recallTypeItemList.end())
        {
            continue;
        }

        std::size_t mergeMinNum = mergeParams.GetMergeMinNum();
        for (auto &item_info : iter->second)
        {
            // 达到总融合最大数量, 直接返回(一般不会出现)
            if (recallMergeSet.size() >= useAllMergeNum)
            {
                LOG(WARNING) << "Recall::ItemMerge() More than allMergeNum!!!";
                return Common::Error::OK;
            }

            // 达到渠道最小融合数量, 则不再继续添加
            if (recallTypeMergeNum[recallType] >= mergeMinNum)
            {
                break;
            }

            if (recallMergeSet.find(item_info.item_id) == recallMergeSet.end())
            {
                recallMergeSet.insert(item_info.item_id);

                resultItemList.emplace_back(item_info);
                ++recallTypeMergeNum[item_info.recall_type];
            }
        }
    }

    for (const auto &mergeParams : vecRecallParams)
    {
        auto recallType = mergeParams.GetRecallType();
        auto iter = recallTypeItemList.find(recallType);
        if (iter == recallTypeItemList.end())
        {
            continue;
        }

        std::size_t mergeMaxNum = mergeParams.GetMergeMaxNum();
        for (auto &item_info : iter->second)
        {
            // 达到总融合最大数量, 直接返回
            if (recallMergeSet.size() >= useAllMergeNum)
            {
                return Common::Error::OK;
            }

            // 达到渠道最大融合数量, 则不再继续添加
            if (recallTypeMergeNum[recallType] >= mergeMaxNum)
            {
                break;
            }

            if (recallMergeSet.find(item_info.item_id) == recallMergeSet.end())
            {
                recallMergeSet.insert(item_info.item_id);

                resultItemList.emplace_back(item_info);
                ++recallTypeMergeNum[item_info.recall_type];
            }
        }
    }

    return Common::Error::OK;
}
