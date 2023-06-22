#pragma once
#include <string>
#include <atomic>
#include <vector>
#include <any>

class RequestData;
class ResponseData;

class AlgoCenter
{
public:
    bool Init();

public:
    static bool OnDownloadRecommen(
        std::any obj,
        const int cmd,
        const long deadline_ms,
        const std::string &request,
        int &result,
        std::string &response);

    static bool GetRecommendItem(
        RequestData &requestData,
        ResponseData &responseData);

protected:
    // 输出日志
    static void DebugLogFeature(
        const RequestData &requestData,
        const ResponseData &responseData);
};
