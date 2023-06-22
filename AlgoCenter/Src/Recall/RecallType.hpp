#pragma once
#include <ostream>
#include <string>
#include <unordered_map>

namespace Common
{
    // 召回类型ID
    typedef enum RecallTypeID : long
    {
        RTI_None = 0,

        RTI_ResType_Hot = 1,     // 类目热门召回
        RTI_ResType_Quality = 2, // 类目优质召回
        RTI_ResType_Surge = 3,   // 类目飙升召回
        RTI_ResType_CTCVR = 4,   // 类目高点击率召回
        RTI_ResType_DLR = 5,     // 类目高下载率召回

        RTI_ResType_User_CF = 10, // 类目用户协同过滤召回
        RTI_Item_CF = 11,         // 物料协同过滤召回

        RTI_Item_Annoy  = 20, // 物料Annoy召回

        RTI_ClickOccur = 30,    // 物料点击共现
        RTI_DownloadOccur = 31, // 物料下载共现

    } RT_ID;

    inline int GetRecallTypeID(const std::string &recallType) noexcept
    {
        static const std::unordered_map<std::string, int> s_RecallTypeIDConf = {
            {"ResType_Hot", RT_ID::RTI_ResType_Hot},
            {"ResType_Quality", RT_ID::RTI_ResType_Quality},
            {"ResType_Surge", RT_ID::RTI_ResType_Surge},
            {"ResType_CTCVR", RT_ID::RTI_ResType_CTCVR},
            {"ResType_DLR", RT_ID::RTI_ResType_DLR},

            {"ResType_User_CF", RT_ID::RTI_ResType_User_CF},
            {"Item_CF", RT_ID::RTI_Item_CF},

            {"Item_Annoy", RT_ID::RTI_Item_Annoy},

            {"ClickOccur", RT_ID::RTI_ClickOccur},
            {"DownloadOccur", RT_ID::RTI_DownloadOccur},
        };

        auto iter = s_RecallTypeIDConf.find(recallType);
        if (iter != s_RecallTypeIDConf.end())
        {
            return iter->second;
        }
        return 0;
    }
}
