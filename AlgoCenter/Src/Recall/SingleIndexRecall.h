#pragma once
#include "Common/Singleton.h"
#include "Recall_Interface.h"

// 单索引类召回
class SingleIndexRecall
    : public Singleton<SingleIndexRecall>,
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
        std::vector<RecallCalc::SampleInfo> &samplesData);

public:
    SingleIndexRecall(token) {}
    ~SingleIndexRecall() {}
    SingleIndexRecall(SingleIndexRecall &) = delete;
    SingleIndexRecall &operator=(const SingleIndexRecall &) = delete;
};