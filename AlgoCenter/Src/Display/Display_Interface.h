#pragma once
#include <vector>
#include "AlgoCenter/Define.h"
#include "AlgoCenter/Config.h"
#include "AlgoCenter/StatisLogData.h"

#include "Protobuf/redis/item_feature_data.pb.h"
#include "RedisProtoData/include/RedisProtoData.h"

#include "DisplayConfig.h"
#include "DisplayType.h"

class Display_Interface
{
public:
    // CallStrategy 实现展控策略接口
    // [in] requestData 请求数据
    // [in] displayParams 展控参数
    // [in/out] responseData 返回的数据
    virtual int CallStrategy(
        const RequestData &requestData,
        const DisplayParam &displayParams,
        ResponseData &responseData) = 0;
};