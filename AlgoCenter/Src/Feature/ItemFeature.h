#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "glog/logging.h"

#include "Protobuf/redis/item_feature_data.pb.h"
#include "RedisProtoData/include/RedisProtoData.h"

class ItemFeature final
{
public:
    // 初始化特征
    int32_t InitItemFeature(const std::vector<std::string> &vecItemKeys) noexcept
    {
        return RedisProtoData::GetItemFeature(vecItemKeys, item_type_proto);
    }

    const RPDType2SPProto &GetFeatureProto(const long &item_id, const int res_type) const noexcept
    {
        static const RPDType2SPProto empty;
        std::string item_key = std::to_string(item_id) + "_" + std::to_string(res_type);
        if (const auto it = item_type_proto.find(item_key);
            it != item_type_proto.end())
        {
            return it->second;
        }
        return empty;
    }

    const auto GetItemFeatureBasic(const long item_id, const int res_type) const noexcept
    {
        SharedPtrProto proto = nullptr;
        RPD_Common::RedisProtoDataType type = RPD_Common::RPD_Type::RPD_ItemFeatureBasic;
        const auto &type_proto = GetFeatureProto(item_id, res_type);
        const auto iter = type_proto.find(type);
        if (iter != type_proto.end())
        {
            proto = iter->second;
        }
        return std::dynamic_pointer_cast<RSP_ItemFeatureData::ItemFeatureBasic>(proto);
    }

    const auto GetItemFeatureStatis(const long item_id, const int res_type) const noexcept
    {
        SharedPtrProto proto = nullptr;
        RPD_Common::RedisProtoDataType type = RPD_Common::RPD_Type::RPD_ItemFeatureStatis;
        const auto &type_proto = GetFeatureProto(item_id, res_type);
        const auto iter = type_proto.find(type);
        if (iter != type_proto.end())
        {
            proto = iter->second;
        }
        return std::dynamic_pointer_cast<RSP_ItemFeatureData::ItemFeatureStatis>(proto);
    }

    const auto GetItemFeatureIndex(const long item_id, const int res_type) const noexcept
    {
        SharedPtrProto proto = nullptr;
        RPD_Common::RedisProtoDataType type = RPD_Common::RPD_Type::RPD_ItemFeatureIndex;
        const auto &type_proto = GetFeatureProto(item_id, res_type);
        const auto iter = type_proto.find(type);
        if (iter != type_proto.end())
        {
            proto = iter->second;
        }
        return std::dynamic_pointer_cast<RSP_ItemFeatureData::ItemFeatureIndex>(proto);
    }

private:
    std::unordered_map<std::string, RPDType2SPProto> item_type_proto;
};
