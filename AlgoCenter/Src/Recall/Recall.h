#pragma once
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "Common/Singleton.h"

#include "RecallConfig.h"

class RequestData;

struct ItemInfo;
class ResponseData;

class Recall_Interface;

class Recall : public Singleton<Recall>
{
public:
    const RecallParamData *GetRecallParamData(
        const RequestData &requestData,
        ResponseData &responseData);

    int CallRecall(
        const RecallParamData *pRecallParamData,
        const RequestData &requestData,
        ResponseData &responseData);

protected:
    int ItemMerge(
        const int allMergeNum,
        const std::vector<RecallParam> &vecRecallParams,
        const std::unordered_map<std::string, std::vector<ItemInfo>> &recallTypeItemList,
        std::unordered_set<long> &recallMergeSet,
        std::vector<ItemInfo> &resultItemList);

public:
    Recall(token) {}
    ~Recall() {}
    Recall(Recall &) = delete;
    Recall &operator=(const Recall &) = delete;
};