#pragma once
#include <string>
#include <vector>
#include <unordered_map>

class RequestData;
class ResponseData;

class ItemRank
{
public:
    static int RankCalc(
        const RequestData &requestData,
        ResponseData &responseData);
};
