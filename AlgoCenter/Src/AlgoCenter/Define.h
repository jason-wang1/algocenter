#pragma once
#include <ostream>
#include <string>

#include "Common/Error.h"

namespace Common
{
    // APP版本号
    static const std::string APP_VERSION = "1.0.0";

    // 默认用户组
    const static std::string DefaultUserGroup = "def_group";

    // APP配置路径
    const static std::string ConfigPath_App = "./config/AppConf.json";

    // AlgoCenter配置路径
    const static std::string ConfigPath_AlgoCenter = "./config/AlgoCenterConf.json";

    // 缓存配置路径
    const static std::string ConfigPath_Cache = "./config/CacheConf.json";

    // 召回配置路径
    const static std::string ConfigPath_Recall = "./config/RecallExpConf.json";

    // 过滤配置路径
    const static std::string ConfigPath_Filter = "./config/FilterExpConf.json";

    // 模型配置路径
    const static std::string ConfigPath_Model = "./config/ModelConf.json";

    // 排序配置路径
    const static std::string ConfigPath_Rank = "./config/RankExpConf.json";

    // 展控配置路径
    const static std::string ConfigPath_Display = "./config/DisplayExpConf.json";

} // namespace Common
