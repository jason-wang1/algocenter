#pragma once
#include "include/RedisProtoData_Def.h"
#include "MurmurHash3/MurmurHash3.h"

#include "Protobuf/redis/user_feature_data.pb.h"
#include "Protobuf/redis/user_live_feature_data.pb.h"
#include "Protobuf/redis/item_feature_data.pb.h"
#include "Protobuf/redis/item_recall_data.pb.h"

namespace RPD_Common
{
    // Redis库名称
    inline const std::string g_UserFeatureData = "UserFeature"; // 用户特征库
    inline const std::string g_ItemFeatureData = "ItemFeature"; // 物料特征库
    inline const std::string g_InvertIndexData = "InvertIndex"; // 倒排索引库

    // 特征分桶配置
    static constexpr int s_UserFeatureBucketNum = 10000; // 用户特征分1万个桶
    static constexpr int s_ItemFeatureBucketNum = 10000; // 物料特征分1万个桶

    inline std::string GetAutoFeatureKey(
        const std::string &feature_space,
        const std::string &feature_name,
        const std::string &feature_type,
        const std::string &bucket) noexcept
    {
        const static std::string empty = "";
        if (feature_space.empty() || feature_type.empty())
        {
            return empty;
        }

        std::string key;
        key.reserve(feature_space.size() + feature_type.size() + bucket.size() + 10);
        key.append("{");
        {
            key.append(feature_space);
        }
        if (!feature_name.empty())
        {
            key.append(":");
            key.append(feature_name);
        }
        if (!feature_type.empty())
        {
            key.append(":");
            key.append(feature_type);
        }
        if (!bucket.empty())
        {
            key.append(":");
            key.append(bucket);
        }
        key.append("}");
        return key;
    }

    inline const std::string &GetBasicKey(int type) noexcept
    {
        // redis库 key
        const static std::string empty = "";
        static const std::unordered_map<int, std::string> s_basic_key_string_map = {
            // 用户特征
            {RPD_Type::RPD_UserFeatureTimeStamp, "user_features_update_time"},
            {RPD_Type::RPD_UserFeatureBasic, "user_feature"},
            {RPD_Type::RPD_UserFeatureClick, "user_feature"},
            {RPD_Type::RPD_UserFeatureSearch, "user_feature"},
            {RPD_Type::RPD_UserFeatureDownload, "user_feature"},
            {RPD_Type::RPD_UserFeatureIndex, "user_feature"},

            // 用户实时特征
            {RPD_Type::RPD_UserLiveFeatureClick, "user_live_feature"},
            {RPD_Type::RPD_UserLiveFeatureSearch, "user_live_feature"},
            {RPD_Type::RPD_UserLiveFeatureDownload, "user_live_feature"},

            // 物料特征
            {RPD_Type::RPD_ItemFeatureTimeStamp, "item_features_update_time"},
            {RPD_Type::RPD_ItemFeatureBasic, "item_feature"},
            {RPD_Type::RPD_ItemFeatureStatis, "item_feature"},
            {RPD_Type::RPD_ItemFeatureIndex, "item_feature"},

            // 倒排索引
            {RPD_Type::RPD_InvertIndexTimeStamp, "invert_index_update_time"},
            {RPD_Type::RPD_ResTypeHotInvertIndex, "recall_res_type_hot_invert_index"},
            {RPD_Type::RPD_ResTypeQualityInvertIndex, "recall_res_type_quality_invert_index"},
            {RPD_Type::RPD_ResTypeSurgeInvertIndex, "recall_res_type_surge_invert_index"},
            {RPD_Type::RPD_ResTypeCTCVRInvertIndex, "recall_res_type_ctcvr_invert_index"},
            {RPD_Type::RPD_ResTypeDLRInvertIndex, "recall_res_type_dlr_invert_index"},

        };

        auto it = s_basic_key_string_map.find(type);
        if (it != s_basic_key_string_map.end())
        {
            return it->second;
        }
        return empty;
    }

    // 特征对应Protobuf
    inline SharedPtrProto GetSPProto(const int type)
    {
        SharedPtrProto spProto = nullptr;
        switch (type)
        {
        // 用户特征
        case RPD_Common::RPD_Type::RPD_UserFeatureBasic:
            spProto = std::make_shared<RSP_UserFeatureData::UserFeatureBasic>();
            break;
        case RPD_Common::RPD_Type::RPD_UserFeatureClick:
            spProto = std::make_shared<RSP_UserFeatureData::UserFeatureClick>();
            break;
        case RPD_Common::RPD_Type::RPD_UserFeatureSearch:
            spProto = std::make_shared<RSP_UserFeatureData::UserFeatureSearch>();
            break;
        case RPD_Common::RPD_Type::RPD_UserFeatureDownload:
            spProto = std::make_shared<RSP_UserFeatureData::UserFeatureDownload>();
            break;
        case RPD_Common::RPD_Type::RPD_UserFeatureIndex:
            spProto = std::make_shared<RSP_UserFeatureData::UserFeatureIndex>();
            break;

        // 用户实时特征
        case RPD_Common::RPD_Type::RPD_UserLiveFeatureClick:
            spProto = std::make_shared<RSP_UserLiveFeatureData::UserLiveFeatureClick>();
            break;
        case RPD_Common::RPD_Type::RPD_UserLiveFeatureSearch:
            spProto = std::make_shared<RSP_UserLiveFeatureData::UserLiveFeatureSearch>();
            break;
        case RPD_Common::RPD_Type::RPD_UserLiveFeatureDownload:
            spProto = std::make_shared<RSP_UserLiveFeatureData::UserLiveFeatureDownload>();
            break;

        // 物料特征
        case RPD_Common::RPD_Type::RPD_ItemFeatureBasic:
            spProto = std::make_shared<RSP_ItemFeatureData::ItemFeatureBasic>();
            break;
        case RPD_Common::RPD_Type::RPD_ItemFeatureStatis:
            spProto = std::make_shared<RSP_ItemFeatureData::ItemFeatureStatis>();
            break;
        case RPD_Common::RPD_Type::RPD_ItemFeatureIndex:
            spProto = std::make_shared<RSP_ItemFeatureData::ItemFeatureIndex>();
            break;

        default:
            break;
        }
        return spProto;
    }

    // 特征后缀名称
    inline const std::string &GetFeatureFieldSuffix(int type) noexcept
    {
        static const std::string empty = "";
        static const std::unordered_map<int, std::string> s_field_suffix_string_map = {
            // 用户特征
            {RPD_Type::RPD_UserFeatureBasic, "basic"},
            {RPD_Type::RPD_UserFeatureClick, "click"},
            {RPD_Type::RPD_UserFeatureSearch, "search"},
            {RPD_Type::RPD_UserFeatureDownload, "download"},
            {RPD_Type::RPD_UserFeatureIndex, "index"},

            // 用户实时特征
            {RPD_Type::RPD_UserLiveFeatureClick, "live_click"},
            {RPD_Type::RPD_UserLiveFeatureSearch, "live_search"},
            {RPD_Type::RPD_UserLiveFeatureDownload, "live_download"},

            // 物料特征
            {RPD_Type::RPD_ItemFeatureBasic, "basic"},
            {RPD_Type::RPD_ItemFeatureStatis, "statis"},
            {RPD_Type::RPD_ItemFeatureIndex, "index"},

        };

        const auto it = s_field_suffix_string_map.find(type);
        if (it != s_field_suffix_string_map.end())
        {
            return it->second;
        }
        return empty;
    }

    inline const std::string GetUserKey(const int type, const long user_id) noexcept
    {
        switch (type)
        {
        // 特殊用户特征处理逻辑
        // case RPD_Type::RPD_UserFeatureOther:
        // {
        // }
        // break;
        default:
        {
            const std::string feature_space = GetBasicKey(type);
            const std::string feature_type = "proto";
            long bucket_idx = (user_id % RPD_Common::s_UserFeatureBucketNum);
            return GetAutoFeatureKey(feature_space, "", feature_type, std::to_string(bucket_idx));
        }
        break;
        }
    }

    inline const std::string GetUserField(const int type, const long user_id, const std::string &param = "") noexcept
    {
        switch (type)
        {
        // 特殊用户特征处理逻辑
        // case RPD_Type::RPD_UserFeatureOther:
        // {
        // }
        // break;
        default:
        {
            std::string field = std::to_string(user_id) + ":" + GetFeatureFieldSuffix(type);
            if (!param.empty())
            {
                field += ":" + param;
            }
            return field;
        }
        break;
        }
    }

    inline const std::string GetItemKey(const int type, const long item_id) noexcept
    {
        switch (type)
        {
        // 特殊物料特征处理逻辑
        // case RPD_Type::RPD_ItemFeatureOther:
        // {
        // }
        // break;
        default:
        {
            const std::string feature_space = GetBasicKey(type);
            const std::string feature_type = "proto";
            long bucket_idx = (item_id % RPD_Common::s_ItemFeatureBucketNum);
            return GetAutoFeatureKey(feature_space, "", feature_type, std::to_string(bucket_idx));
        }
        break;
        }
    }

    inline const std::string GetItemField(const int type, const long item_id, const std::string &param = "") noexcept
    {
        // param目前是res_type（类目）
        switch (type)
        {
        // 特殊物料特征处理逻辑
        // case RPD_Type::RPD_ItemFeatureOther:
        // {
        // }
        // break;
        default:
        {
            std::string field = std::to_string(item_id) + ":" + GetFeatureFieldSuffix(type);
            if (!param.empty())
            {
                field += ":" + param;
            }
            return field;
        }
        break;
        }
    }

} // namespace RPD_Common
