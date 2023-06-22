#include "FilterConfig.h"
#include "glog/logging.h"
#include "Common/Function.h"

static const std::unordered_map<std::string, Common::APIType> FilterName2CmdType = {
    std::make_pair<std::string, Common::APIType>("Download_Recommend", Common::APIType::Get_Download_Recommend),
};

bool FilterConfig::UpdateJson(const std::string &json_data)
{
    std::lock_guard<std::mutex> lg(m_update_lock);
    if (json_data.empty())
    {
        LOG(ERROR) << "FilterConfig::UpdateJson json_data empty.";
        return false;
    }
    return FilterConfig::Error::OK == DecodeJsonData(json_data);
}

const int FilterConfig::MatchFilterExpID(
    const Common::APIType apiType,
    const std::vector<int> &vecExpIDList) const
{
    int dataIdx = m_dataIdx;
    int type = static_cast<int>(apiType);
    auto itType = m_data[dataIdx].m_FilterParamData.find(type);
    if (itType != m_data[dataIdx].m_FilterParamData.end())
    {
        for (auto exp_id : vecExpIDList)
        {
            if (exp_id != 0)
            {
                auto itExp = itType->second.find(exp_id);
                if (itExp != itType->second.end())
                {
                    return exp_id;
                }
            }
        }
    }
    return 0;
}

const FilterParamData *FilterConfig::GetFilterParamData(
    const std::unordered_set<std::string> &userGroupList, Common::APIType apiType, int exp_id) const
{
    int dataIdx = m_dataIdx;
    int type = static_cast<int>(apiType);

    const FilterParamData *pFilterParamData = nullptr;
    auto itType = m_data[dataIdx].m_FilterParamData.find(type);
    if (itType != m_data[dataIdx].m_FilterParamData.end())
    {
        auto itExp = itType->second.find(exp_id);
        if (itExp == itType->second.end())
        {
            // 如果没有这个exp_id, 则使用默认exp_id -> 0
            itExp = itType->second.find(0);
        }
        if (itExp != itType->second.end())
        {
            for (const auto &itemGroup : itExp->second)
            {
                // 默认组用户配置优先级最低，必须放列表最后
                // 先匹配其他用户组配置，命中其他用户组配置或者遇到最后的默认用户组配置，即完成匹配并退出循环
                if (userGroupList.find(itemGroup.user_group) != userGroupList.end() ||
                    itemGroup.user_group == Common::DefaultUserGroup)
                {
                    pFilterParamData = &itemGroup;
                    break;
                }
            }
        }
    }
    return pFilterParamData;
}

FilterConfig::Error FilterConfig::DecodeJsonData(const std::string &jsonData)
{
    rapidjson::Document doc;
    if (doc.Parse(jsonData.data()).HasParseError())
    {
        LOG(ERROR) << "DecodeJsonData() Parse jsonData HasParseError. err = " << doc.GetParseError();
        return FilterConfig::Error::ParseError;
    }

    // 缓存下标
    int idx = (m_dataIdx + 1) % 2;

    // 解析召回融合配置
    auto cfgErr = DecodeFilterParamData(doc, idx);
    if (cfgErr != FilterConfig::Error::OK)
    {
        return cfgErr;
    }

    m_dataIdx = idx;

    return FilterConfig::Error::OK;
}

FilterConfig::Error FilterConfig::DecodeFilterParamData(rapidjson::Document &doc, int dataIdx)
{
    if (!doc.HasMember("Filter") || !doc["Filter"].IsObject())
    {
        LOG(ERROR) << "DecodeFilterParamData() Filter Not Find Or Not Object.";
        return FilterConfig::Error::DecodeFilterParamDataError;
    }
    auto FilterObject = doc["Filter"].GetObject();
    for (auto &api : FilterName2CmdType)
    {
        const char *apiName = api.first.c_str();
        int apiType = static_cast<int>(api.second);

        if (!FilterObject.HasMember(apiName) || !FilterObject[apiName].IsArray())
        {
            LOG(ERROR) << "DecodeFilterParamData() Filter Not Find " << apiName;
            return FilterConfig::Error::DecodeFilterParamDataError;
        }
        auto apiParamList = FilterObject[apiName].GetArray();

        rapidjson::SizeType ParamListSize = apiParamList.Size();
        for (rapidjson::SizeType ParamListIdx = 0; ParamListIdx < ParamListSize; ++ParamListIdx)
        {
            if (!apiParamList[ParamListIdx].IsObject())
            {
                LOG(ERROR) << "DecodeFilterParamData() Parse " << apiName
                           << ", Param List index = " << ParamListIdx
                           << ", not object.";
                return FilterConfig::Error::DecodeFilterParamDataError;
            }
            auto ParamListItem = apiParamList[ParamListIdx].GetObject();

            // 获取实验ID
            int exp_id = 0;
            if (ParamListItem.HasMember("exp_id") && ParamListItem["exp_id"].IsInt())
            {
                exp_id = ParamListItem["exp_id"].GetInt();
            }
            else
            {
                LOG(ERROR) << "DecodeFilterParamData() Parse " << apiName
                           << ", Param List index = " << ParamListIdx
                           << ", not find exp_id.";
                return FilterConfig::Error::DecodeFilterParamDataError;
            }

            if (!ParamListItem.HasMember("UserGroupList") || !ParamListItem["UserGroupList"].IsArray())
            {
                LOG(ERROR) << "DecodeFilterParamData() Parse " << apiName
                           << ", Param List index = " << ParamListIdx
                           << ", not find UserGroupList.";
                return FilterConfig::Error::DecodeFilterParamDataError;
            }
            auto userGroupList = ParamListItem["UserGroupList"].GetArray();

            rapidjson::SizeType UserGroupListSize = userGroupList.Size();

            bool def_group_user_conf = false;
            auto &configItemList = m_data[dataIdx].m_FilterParamData[apiType][exp_id];
            configItemList.resize(UserGroupListSize);
            for (rapidjson::SizeType UserGroupListIdx = 0; UserGroupListIdx < UserGroupListSize; ++UserGroupListIdx)
            {
                if (!userGroupList[UserGroupListIdx].IsObject())
                {
                    LOG(ERROR) << "DecodeFilterParamData() Parse " << apiName
                               << ", exp_id = " << exp_id
                               << ", UserGroup List index = " << UserGroupListIdx
                               << ", not object.";
                    return FilterConfig::Error::DecodeFilterParamDataError;
                }
                auto UserGroupListItem = userGroupList[UserGroupListIdx].GetObject();

                auto &configItem = configItemList[UserGroupListIdx];
                if (UserGroupListItem.HasMember("UserGroup") && UserGroupListItem["UserGroup"].IsString())
                {
                    configItem.user_group = UserGroupListItem["UserGroup"].GetString();
                    if (UserGroupListIdx == UserGroupListSize - 1)
                    {
                        if (configItem.user_group == Common::DefaultUserGroup)
                        {
                            def_group_user_conf = true;
                        }
                    }
                }

                // KeepItemNum
                if (UserGroupListItem.HasMember("KeepItemNum") && UserGroupListItem["KeepItemNum"].IsInt())
                {
                    configItem.KeepItemNum = UserGroupListItem["KeepItemNum"].GetInt();
                }
                else
                {
                    LOG(ERROR) << "DecodeFilterParamData() Parse " << apiName
                               << ", UserGroup List index = " << UserGroupListIdx
                               << ", user_group = " << configItem.user_group
                               << ", exp_id = " << exp_id
                               << ", not find KeepItemNum.";
                    return FilterConfig::Error::DecodeFilterParamDataError;
                }

                // FilterStrategyList 过滤策略, 直接将所有参数作为Params解析
                if (!UserGroupListItem.HasMember("FilterStrategyList") || !UserGroupListItem["FilterStrategyList"].IsArray())
                {
                    LOG(ERROR) << "DecodeFilterParamData() Parse " << apiName
                               << ", UserGroup List index = " << UserGroupListIdx
                               << ", user_group = " << configItem.user_group
                               << ", exp_id = " << exp_id
                               << ", not find FilterStrategyList.";
                    return FilterConfig::Error::DecodeFilterParamDataError;
                }
                auto FilterStrategyArray = UserGroupListItem["FilterStrategyList"].GetArray();
                rapidjson::SizeType FilterStrategyArraySize = FilterStrategyArray.Size();
                configItem.vecFilterParams.resize(FilterStrategyArraySize);
                for (rapidjson::SizeType FilterStrategyArrayIdx = 0; FilterStrategyArrayIdx < FilterStrategyArraySize; ++FilterStrategyArrayIdx)
                {
                    if (!FilterStrategyArray[FilterStrategyArrayIdx].IsObject())
                    {
                        LOG(ERROR) << "DecodeFilterParamData() Parse " << apiName
                                   << ", FilterStrategy Array index = " << FilterStrategyArrayIdx
                                   << ", user_group = " << configItem.user_group
                                   << ", exp_id = " << exp_id
                                   << ", not object.";
                        return FilterConfig::Error::DecodeFilterParamDataError;
                    }

                    // 解析参数
                    auto &params = configItem.vecFilterParams[FilterStrategyArrayIdx];
                    auto FilterStrategyArrayItem = FilterStrategyArray[FilterStrategyArrayIdx].GetObject();
                    for (auto it = FilterStrategyArrayItem.MemberBegin(); it != FilterStrategyArrayItem.MemberEnd(); it++)
                    {
                        std::string key = it->name.GetString();
                        if (it->value.IsString())
                        {
                            params.Set(key, it->value.GetString());
                        }
                        else if (it->value.IsInt64())
                        {
                            params.Set(key, it->value.GetInt64());
                        }
                    }
                }
            }

            if (!def_group_user_conf)
            {
                LOG(ERROR) << "DecodeFilterParamData() Default user group is missing or not the last one"
                           << ", apiName = " << apiName
                           << ", exp_id = " << exp_id;
            }
        }
    }
    return FilterConfig::Error::OK;
}
