#include "Display.h"
#include <unordered_map>
#include "glog/logging.h"

#include "ScattersStrategy.h"

static std::unordered_map<std::string, std::shared_ptr<Display_Interface>> TypeToStrategyList = {
    {Common::DisplayType_Scatter, ScattersStrategy::GetInstance()},
};

const DisplayParamData *Display::GetDisplayParamData(
    const RequestData &requestData,
    ResponseData &responseData)
{
    return DisplayConfig::GetInstance()->GetDisplayParamData(
        requestData.userGroupList, requestData.apiType, requestData.displayExpID);
}

int Display::CallDisplay(
    const DisplayParamData *pDisplayParamData,
    const RequestData &requestData,
    ResponseData &responseData)
{

    // 遍历展控参数列表, 调用展控策略
    for (const auto &scParams : pDisplayParamData->vecDisplayParams)
    {
        std::string scType = scParams.GetDisplayType();
        if (scType == Common::DisplayType_ErrorType)
        {
            LOG(ERROR) << "Display::CallDisplay() Get Display Type error"
                       << ", apiType" << requestData.apiType
                       << ", displayExpID = " << requestData.displayExpID;
            return Common::Error::Display_Error;
        }

        auto strategyInst = GetDisplayStrategyInst(scType);
        if (strategyInst == nullptr || strategyInst.get() == nullptr)
        {
            LOG(ERROR) << "Display::CallDisplay() Get DisplayStrategy Inst Error"
                       << ", apiType" << requestData.apiType
                       << ", displayExpID = " << requestData.displayExpID
                       << ", displayType = " << scType;
            return Common::Error::Display_GetStrategyInstError;
        }

        // 调用策略
        int err = strategyInst->CallStrategy(requestData, scParams, responseData);
        if (Common::Error::OK != err)
        {
            LOG(WARNING) << "Display::CallDisplay(), Call Strategy Failed"
                         << ", apiType" << requestData.apiType
                         << ", uuid = " << requestData.uuid
                         << ", user_id = " << requestData.user_id
                         << ", displayExpID = " << requestData.displayExpID
                         << ", displayType = " << scType
                         << ", err = " << err;
        }
#ifdef DEBUG
        else
        {
            LOG(INFO) << "[DEBUG] Display::CallDisplay() Call Strategy success"
                      << ", apiType" << requestData.apiType
                      << ", uuid = " << requestData.uuid
                      << ", user_id = " << requestData.user_id
                      << ", filterExpID = " << requestData.filterExpID
                      << ", displayType = " << scType;
        }
#endif
    }

    return Common::Error::OK;
}

std::shared_ptr<Display_Interface> Display::GetDisplayStrategyInst(
    const std::string &scType)
{
    std::shared_ptr<Display_Interface> pStrategyInst = nullptr;
    auto iter = TypeToStrategyList.find(scType);
    if (iter != TypeToStrategyList.end())
    {
        pStrategyInst = iter->second;
    }

    return pStrategyInst;
}
