#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <ostream>
#include "Common/Error.h"
#include "ProtoFactory/ProtoDefine.h"

namespace RPD_Common
{
    // 特征类型
    typedef enum RedisProtoDataType : int
    {
        RPD_None = 0,

        // 用户特征
        RPD_UserFeatureTimeStamp, // 用户特征更新时间戳
        RPD_UserFeatureBasic,     // 用户基础画像
        RPD_UserFeatureClick,     // 用户点击特征
        RPD_UserFeatureSearch,    // 用户搜索特征
        RPD_UserFeatureDownload,  // 用户下载数据
        RPD_UserFeatureIndex,     // 用户索引数据

        // 用户实时特征
        RPD_UserLiveFeatureClick,     // 用户实时点击特征
        RPD_UserLiveFeatureSearch,    // 用户实时搜索特征
        RPD_UserLiveFeatureDownload,  // 用户实时下载数据

        // 物料特征
        RPD_ItemFeatureTimeStamp, // 物料特征更新时间戳
        RPD_ItemFeatureBasic,     // 物料基础特征
        RPD_ItemFeatureStatis,    // 物料统计特征
        RPD_ItemFeatureIndex,     // 物料索引数据

        // 召回倒排索引
        RPD_InvertIndexTimeStamp,      // 倒排索引特征更新时间戳
        RPD_ResTypeHotInvertIndex,     // 类目热门倒排索引
        RPD_ResTypeQualityInvertIndex, // 类目优质倒排索引
        RPD_ResTypeSurgeInvertIndex,   // 类目飙升倒排索引
        RPD_ResTypeCTCVRInvertIndex,   // 类目高点击率倒排索引
        RPD_ResTypeDLRInvertIndex,     // 类目高下载率倒排索引

    } RPD_Type;

    inline std::ostream &operator<<(std::ostream &out, RPD_Type type)
    {
        switch (type)
        {
        case RPD_Type::RPD_UserFeatureTimeStamp:
            out << "用户特征更新时间戳";
            break;
        case RPD_Type::RPD_UserFeatureBasic:
            out << "用户基础画像";
            break;
        case RPD_Type::RPD_UserFeatureClick:
            out << "用户点击特征";
            break;
        case RPD_Type::RPD_UserFeatureSearch:
            out << "用户搜索特征";
            break;
        case RPD_Type::RPD_UserFeatureDownload:
            out << "用户下载数据";
            break;
        case RPD_Type::RPD_UserFeatureIndex:
            out << "用户索引数据";
            break;
        case RPD_Type::RPD_UserLiveFeatureClick:
            out << "用户实时点击特征";
            break;
        case RPD_Type::RPD_UserLiveFeatureSearch:
            out << "用户实时搜索特征";
            break;
        case RPD_Type::RPD_UserLiveFeatureDownload:
            out << "用户实时下载数据";
            break;
        case RPD_Type::RPD_ItemFeatureTimeStamp:
            out << "物料特征更新时间戳";
            break;
        case RPD_Type::RPD_ItemFeatureBasic:
            out << "物料基础画像";
            break;
        case RPD_Type::RPD_ItemFeatureStatis:
            out << "物料统计特征";
            break;
        case RPD_Type::RPD_ItemFeatureIndex:
            out << "物料索引数据";
            break;
        case RPD_Type::RPD_InvertIndexTimeStamp:
            out << "倒排索引特征更新时间戳";
            break;
        case RPD_Type::RPD_ResTypeHotInvertIndex:
            out << "类目热门倒排索引";
            break;
        case RPD_Type::RPD_ResTypeQualityInvertIndex:
            out << "类目优质倒排索引";
            break;
        case RPD_Type::RPD_ResTypeSurgeInvertIndex:
            out << "类目飙升倒排索引";
            break;
        case RPD_Type::RPD_ResTypeCTCVRInvertIndex:
            out << "类目高点击率倒排索引";
            break;
        case RPD_Type::RPD_ResTypeDLRInvertIndex:
            out << "类目高下载率倒排索引";
            break;
        default:
            break;
        }
        out << "(" << static_cast<std::underlying_type<RPD_Common::RPD_Type>::type>(type) << ")";
        return out;
    }
}

using RPDType2SPProto = std::unordered_map<RPD_Common::RPD_Type, SharedPtrProto>;
