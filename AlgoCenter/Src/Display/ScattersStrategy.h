#pragma once
#include "Common/Singleton.h"
#include "Display_Interface.h"

// 打散策略
class ScattersStrategy
    : public Singleton<ScattersStrategy>,
      public Display_Interface
{
    struct ControlInfo
    {
        std::unordered_set<std::string> element_set;
        std::string control_method = "";
        int control_count = 0;
        int now_count = 0;
    };

public:
    // CallStrategy 实现展控策略接口
    virtual int CallStrategy(
        const RequestData &requestData,
        const DisplayParam &displayParams,
        ResponseData &responseData) override;

public:
    ScattersStrategy(token) {}
    ~ScattersStrategy() {}
    ScattersStrategy(ScattersStrategy &) = delete;
    ScattersStrategy &operator=(const ScattersStrategy &) = delete;
};
