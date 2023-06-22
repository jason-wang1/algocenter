#pragma once
#include "Common/Singleton.h"
#include "Recall_Interface.h"

// 多索引类召回
class MultiIndexRecall
    : public Singleton<MultiIndexRecall>,
      public Recall_Interface
{
public:
    // RecallItem 召回函数接口
    virtual int RecallItem(
        const RequestData &requestData,
        const RecallParam &recallParams,
        std::vector<ItemInfo> &resultRecallData) override;

protected:
    static int GetSamplesData(
        const RequestData &requestData,
        const RecallParam &recallParams,
        const std::vector<long> &vecID,
        const std::vector<int> &vecSamplingNum,
        std::vector<std::vector<RecallCalc::SampleInfo>> &vecSamplesData);

public:
    MultiIndexRecall(token) {}
    ~MultiIndexRecall() {}
    MultiIndexRecall(MultiIndexRecall &) = delete;
    MultiIndexRecall &operator=(const MultiIndexRecall &) = delete;
};
