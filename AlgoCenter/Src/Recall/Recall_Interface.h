#pragma once
#include <algorithm>
#include <random>
#include <chrono>
#include "AlgoCenter/Define.h"
#include "AlgoCenter/Config.h"
#include "AlgoCenter/StatisLogData.h"

#include "Protobuf/redis/item_recall_data.pb.h"
#include "Protobuf/redis/item_feature_data.pb.h"
#include "Protobuf/redis/user_feature_data.pb.h"
#include "RedisProtoData/include/RedisProtoData.h"

#include "RecallConfig.h"
#include "RecallType.hpp"
#include "RecallCalc.hpp"

class Recall_Interface
{
public:
    // RecallItem 召回函数接口
    // [in] requestData: 请求数据
    // [in] recallParams: 召回参数
    // [out] resultData: 返回数据
    virtual int RecallItem(
        const RequestData &requestData,
        const RecallParam &recallParams,
        std::vector<ItemInfo> &resultRecallData) = 0;
};