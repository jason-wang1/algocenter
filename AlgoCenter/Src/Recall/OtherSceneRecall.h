#pragma once
#include "Common/Singleton.h"
#include "Recall_Interface.h"

// 其他场景召回
class OtherSceneRecall
    : public Singleton<OtherSceneRecall>,
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
    OtherSceneRecall(token) {}
    ~OtherSceneRecall() {}
    OtherSceneRecall(OtherSceneRecall &) = delete;
    OtherSceneRecall &operator=(const OtherSceneRecall &) = delete;
};
