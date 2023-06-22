#pragma once
#include <vector>
#include <unordered_map>
#include "Common/Singleton.h"
#include "AlgoCenter/Define.h"

#include "DisplayConfig.h"
#include "DisplayType.h"

class RequestData;
class ResponseData;
class Display_Interface;

class Display : public Singleton<Display>
{
public:
    const DisplayParamData *GetDisplayParamData(
        const RequestData &requestData,
        ResponseData &responseData);

    int CallDisplay(
        const DisplayParamData *pDisplayParamData,
        const RequestData &requestData,
        ResponseData &responseData);

protected:
    static std::shared_ptr<Display_Interface> GetDisplayStrategyInst(
        const std::string &scType);

public:
    Display(token) {}
    ~Display() {}
    Display(Display &) = delete;
    Display &operator=(const Display &) = delete;
};