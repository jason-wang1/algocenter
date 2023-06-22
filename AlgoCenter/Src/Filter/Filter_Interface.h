#pragma once
#include <vector>

#include "AlgoCenter/Define.h"
#include "AlgoCenter/Config.h"
#include "AlgoCenter/StatisLogData.h"

#include "Protobuf/redis/user_feature_data.pb.h"
#include "Protobuf/redis/item_feature_data.pb.h"
#include "RedisProtoData/include/RedisProtoData.h"

#include "FilterConfig.h"
#include "FilterType.h"

class Filter_Interface
{
public:
    // CallStrategy 调用过滤策略接口
    // [in] requestData 请求数据
    // [in] filterParams 过滤参数
    // [in/out] responseData 返回数据
    virtual int CallStrategy(
        const RequestData &requestData,
        const FilterParam &filterParams,
        ResponseData &responseData) = 0;
};
