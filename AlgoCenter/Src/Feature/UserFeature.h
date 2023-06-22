#pragma once
#include <string>
#include <unordered_map>
#include "glog/logging.h"
#include "Common/Function.h"

#include "Protobuf/redis/user_feature_data.pb.h"
#include "Protobuf/redis/user_live_feature_data.pb.h"
#include "RedisProtoData/include/RedisProtoData.h"

#include "AlgoCenter/Define.h"

class UserFeature final
{
    static inline const std::vector<RPD_Common::RPD_Type> gs_vec_type = {
        RPD_Common::RPD_Type::RPD_UserFeatureBasic,    // 用户基础特征
        RPD_Common::RPD_Type::RPD_UserFeatureClick,    // 用户点击特征
        RPD_Common::RPD_Type::RPD_UserFeatureSearch,   // 用户搜索特征
        RPD_Common::RPD_Type::RPD_UserFeatureDownload, // 用户下载数据
        RPD_Common::RPD_Type::RPD_UserFeatureIndex,    // 用户索引数据

        RPD_Common::RPD_Type::RPD_UserLiveFeatureClick,    // 用户实时点击特征
        RPD_Common::RPD_Type::RPD_UserLiveFeatureSearch,   // 用户实时搜索特征
        RPD_Common::RPD_Type::RPD_UserLiveFeatureDownload, // 用户实时下载数据
    };

public:
    // 初始化用户特征
    int32_t InitUserFeature(const long user_id, const long ll_id, const int res_type) noexcept
    {
        // 请求用户数据
        this->user_id = user_id;
        this->context_item_id = ll_id;
        this->context_res_type = res_type;
        return RedisProtoData::GetUserFeatureProto(this->user_id, gs_vec_type, type2proto);
    }

public:
    long GetUserId() const noexcept
    {
        return this->user_id;
    }

    long GetContextItemId() const noexcept
    {
        return this->context_item_id;
    }

    int GetContextResType() const noexcept
    {
        return this->context_res_type;
    }

    const auto GetUserFeatureBasic() const noexcept
    {
        SharedPtrProto proto = nullptr;
        RPD_Common::RedisProtoDataType type = RPD_Common::RPD_Type::RPD_UserFeatureBasic;
        const auto iter = type2proto.find(type);
        if (iter != type2proto.end())
        {
            proto = iter->second;
        }
        return std::dynamic_pointer_cast<RSP_UserFeatureData::UserFeatureBasic>(proto);
    }

    const auto GetUserFeatureClick() const noexcept
    {
        SharedPtrProto proto = nullptr;
        RPD_Common::RedisProtoDataType type = RPD_Common::RPD_Type::RPD_UserFeatureClick;
        const auto iter = type2proto.find(type);
        if (iter != type2proto.end())
        {
            proto = iter->second;
        }
        return std::dynamic_pointer_cast<RSP_UserFeatureData::UserFeatureClick>(proto);
    }

    const auto GetUserFeatureSearch() const noexcept
    {
        SharedPtrProto proto = nullptr;
        RPD_Common::RedisProtoDataType type = RPD_Common::RPD_Type::RPD_UserFeatureSearch;
        const auto iter = type2proto.find(type);
        if (iter != type2proto.end())
        {
            proto = iter->second;
        }
        return std::dynamic_pointer_cast<RSP_UserFeatureData::UserFeatureSearch>(proto);
    }

    const auto GetUserFeatureDownload() const noexcept
    {
        SharedPtrProto proto = nullptr;
        RPD_Common::RedisProtoDataType type = RPD_Common::RPD_Type::RPD_UserFeatureDownload;
        const auto iter = type2proto.find(type);
        if (iter != type2proto.end())
        {
            proto = iter->second;
        }
        return std::dynamic_pointer_cast<RSP_UserFeatureData::UserFeatureDownload>(proto);
    }

    const auto GetUserFeatureIndex() const noexcept
    {
        SharedPtrProto proto = nullptr;
        RPD_Common::RedisProtoDataType type = RPD_Common::RPD_Type::RPD_UserFeatureIndex;
        const auto iter = type2proto.find(type);
        if (iter != type2proto.end())
        {
            proto = iter->second;
        }
        return std::dynamic_pointer_cast<RSP_UserFeatureData::UserFeatureIndex>(proto);
    }

    const auto GetUserLiveFeatureClick() const noexcept
    {
        SharedPtrProto proto = nullptr;
        RPD_Common::RedisProtoDataType type = RPD_Common::RPD_Type::RPD_UserLiveFeatureClick;
        const auto iter = type2proto.find(type);
        if (iter != type2proto.end())
        {
            proto = iter->second;
        }
        return std::dynamic_pointer_cast<RSP_UserLiveFeatureData::UserLiveFeatureClick>(proto);
    }

    const auto GetUserLiveFeatureSearch() const noexcept
    {
        SharedPtrProto proto = nullptr;
        RPD_Common::RedisProtoDataType type = RPD_Common::RPD_Type::RPD_UserLiveFeatureSearch;
        const auto iter = type2proto.find(type);
        if (iter != type2proto.end())
        {
            proto = iter->second;
        }
        return std::dynamic_pointer_cast<RSP_UserLiveFeatureData::UserLiveFeatureSearch>(proto);
    }

    const auto GetUserLiveFeatureDownload() const noexcept
    {
        SharedPtrProto proto = nullptr;
        RPD_Common::RedisProtoDataType type = RPD_Common::RPD_Type::RPD_UserLiveFeatureDownload;
        const auto iter = type2proto.find(type);
        if (iter != type2proto.end())
        {
            proto = iter->second;
        }
        return std::dynamic_pointer_cast<RSP_UserLiveFeatureData::UserLiveFeatureDownload>(proto);
    }

private:
    long user_id = 0;
    long context_item_id = 0;
    int context_res_type = 0;
    RPDType2SPProto type2proto;
};
