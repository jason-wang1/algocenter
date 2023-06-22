#include "DisplayConfig.h"
#include "glog/logging.h"
#include "Common/Function.h"

static const std::unordered_map<std::string, Common::APIType> DisplayName2CmdType = {
    std::make_pair<std::string, Common::APIType>("Download_Recommend", Common::APIType::Get_Download_Recommend),
};

bool DisplayConfig::UpdateJson(const std::string &jsonData)
{
    std::lock_guard<std::mutex> lg(m_update_lock);
    if (jsonData.empty())
    {
        LOG(ERROR) << "DisplayConfig::UpdateJson() json_data empty.";
        return false;
    }
    return DecodeJsonData(jsonData) == DisplayConfig::Error::OK;
}

const int DisplayConfig::MatchDisplayExpID(
    Common::APIType apiType,
    const std::vector<int> &vecExpIDList) const
{
    int dataIdx = m_dataIdx;
    int type = static_cast<int>(apiType);
    auto itType = m_data[dataIdx].m_DisplayParamData.find(type);
    if (itType != m_data[dataIdx].m_DisplayParamData.end())
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

const DisplayParamData *DisplayConfig::GetDisplayParamData(
    const std::unordered_set<std::string> &userGroupList, Common::APIType apiType, int exp_id) const
{
    int dataIdx = m_dataIdx;
    int type = static_cast<int>(apiType);

    const DisplayParamData *pDisplayParamData = nullptr;
    auto itType = m_data[dataIdx].m_DisplayParamData.find(type);
    if (itType != m_data[dataIdx].m_DisplayParamData.end())
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
                    pDisplayParamData = &itemGroup;
                    break;
                }
            }
        }
    }
    return pDisplayParamData;
}

DisplayConfig::Error DisplayConfig::DecodeJsonData(const std::string &jsonData)
{
    rapidjson::Document doc;
    if (doc.Parse(jsonData.data()).HasParseError())
    {
        LOG(ERROR) << "DecodeJsonData() Parse jsonData HasParseError. err = " << doc.GetParseError();
        return DisplayConfig::Error::ParseError;
    }

    // 缓存下标
    int idx = (m_dataIdx + 1) % 2;

    // 解析展控层配置
    auto cfgErr = DecodeDisplayParamData(doc, idx);
    if (cfgErr != DisplayConfig::Error::OK)
    {
        return cfgErr;
    }

    m_dataIdx = idx;

    return DisplayConfig::Error::OK;
}

// 展控层配置
DisplayConfig::Error DisplayConfig::DecodeDisplayParamData(rapidjson::Document &doc, int dataIdx)
{
    if (!doc.HasMember("Display") || !doc["Display"].IsObject())
    {
        LOG(ERROR) << "DecodeDisplayParamData() Display Not Find Or Not Object.";
        return DisplayConfig::Error::DecodeDisplayParamDataError;
    }

    auto DisplayObject = doc["Display"].GetObject();
    for (auto &api : DisplayName2CmdType)
    {
        const char *apiName = api.first.c_str();
        int apiType = static_cast<int>(api.second);
        if (!DisplayObject.HasMember(apiName) || !DisplayObject[apiName].IsArray())
        {
            LOG(ERROR) << "DecodeDisplayParamData() Show Control Not Find " << apiName;
            return DisplayConfig::Error::DecodeDisplayParamDataError;
        }

        auto apiParamList = DisplayObject[apiName].GetArray();
        rapidjson::SizeType ParamListSize = apiParamList.Size();
        for (rapidjson::SizeType ParamListIdx = 0; ParamListIdx < ParamListSize; ++ParamListIdx)
        {
            if (!apiParamList[ParamListIdx].IsObject())
            {
                LOG(ERROR) << "DecodeDisplayParamData() Parse " << apiName
                           << ", Param List index = " << ParamListIdx << ", not object.";
                return DisplayConfig::Error::DecodeDisplayParamDataError;
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
                LOG(ERROR) << "DecodeDisplayParamData() Parse " << apiName
                           << ", Param List index = " << ParamListIdx
                           << ", not find exp_id.";
                return DisplayConfig::Error::DecodeDisplayParamDataError;
            }

            if (!ParamListItem.HasMember("UserGroupList") || !ParamListItem["UserGroupList"].IsArray())
            {
                LOG(ERROR) << "DecodeDisplayParamData() Parse " << apiName
                           << ", Param List index = " << ParamListIdx
                           << ", not find UserGroupList.";
                return DisplayConfig::Error::DecodeDisplayParamDataError;
            }
            auto userGroupList = ParamListItem["UserGroupList"].GetArray();

            rapidjson::SizeType UserGroupListSize = userGroupList.Size();

            bool def_group_user_conf = false;
            auto &configItemList = m_data[dataIdx].m_DisplayParamData[apiType][exp_id];
            configItemList.resize(UserGroupListSize);
            for (rapidjson::SizeType UserGroupListIdx = 0; UserGroupListIdx < UserGroupListSize; ++UserGroupListIdx)
            {
                if (!userGroupList[UserGroupListIdx].IsObject())
                {
                    LOG(ERROR) << "DecodeDisplayParamData() Parse " << apiName
                               << ", exp_id = " << exp_id
                               << ", UserGroup List index = " << UserGroupListIdx
                               << ", not object.";
                    return DisplayConfig::Error::DecodeDisplayParamDataError;
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

                // StrategyList 策略列表
                if (UserGroupListItem.HasMember("StrategyList") && UserGroupListItem["StrategyList"].IsArray())
                {
                    // 直接将所有参数作为Params解析
                    auto StrategyArray = UserGroupListItem["StrategyList"].GetArray();
                    rapidjson::SizeType StrategyArraySize = StrategyArray.Size();
                    configItem.vecDisplayParams.resize(StrategyArraySize);
                    for (rapidjson::SizeType StrategyArrayIdx = 0; StrategyArrayIdx < StrategyArraySize; ++StrategyArrayIdx)
                    {
                        if (!StrategyArray[StrategyArrayIdx].IsObject())
                        {
                            LOG(ERROR) << "DecodeDisplayParamData() Parse " << apiName
                                       << ", exp_id = " << exp_id
                                       << ", user_group = " << configItem.user_group
                                       << ", Strategy Array index = " << StrategyArrayIdx
                                       << ", not object.";
                            return DisplayConfig::Error::DecodeDisplayParamDataError;
                        }

                        auto &params = configItem.vecDisplayParams[StrategyArrayIdx];

                        auto StrategyArrayItem = StrategyArray[StrategyArrayIdx].GetObject();
                        for (auto it = StrategyArrayItem.MemberBegin(); it != StrategyArrayItem.MemberEnd(); it++)
                        {
                            std::string key = it->name.GetString();
                            if (it->value.IsString())
                            {
                                params.Set(key, it->value.GetString());
                            }
                            else if (it->value.IsDouble())
                            {
                                params.Set(key, it->value.GetDouble());
                            }
                            else if (it->value.IsInt64())
                            {
                                params.Set(key, it->value.GetInt64());
                            }
                            else if (it->value.IsArray())
                            {
                                auto &secondaryParamData = params.GetSecondaryParamData(key);

                                auto SecondaryParamArray = it->value.GetArray();
                                rapidjson::SizeType SecondaryParamArraySize = it->value.Size();
                                secondaryParamData.vecSecondaryParam.resize(SecondaryParamArraySize);
                                for (rapidjson::SizeType SecondaryParamArrayIdx = 0; SecondaryParamArrayIdx < SecondaryParamArraySize; ++SecondaryParamArrayIdx)
                                {
                                    if (!SecondaryParamArray[SecondaryParamArrayIdx].IsObject())
                                    {
                                        LOG(ERROR) << "DecodeDisplayParamData() Parse " << apiName
                                                   << ", exp_id = " << exp_id
                                                   << ", user_group = " << configItem.user_group
                                                   << ", Strategy Param Array index = " << SecondaryParamArrayIdx
                                                   << ", not object.";
                                        return DisplayConfig::Error::DecodeDisplayParamDataError;
                                    }

                                    auto &secondaryParam = secondaryParamData.vecSecondaryParam[SecondaryParamArrayIdx];

                                    auto SecondaryParamArrayItem = SecondaryParamArray[SecondaryParamArrayIdx].GetObject();
                                    for (auto it = SecondaryParamArrayItem.MemberBegin(); it != SecondaryParamArrayItem.MemberEnd(); it++)
                                    {
                                        std::string key = it->name.GetString();
                                        if (it->value.IsString())
                                        {
                                            secondaryParam.Set(key, it->value.GetString());
                                        }
                                        else if (it->value.IsDouble())
                                        {
                                            secondaryParam.Set(key, it->value.GetDouble());
                                        }
                                        else if (it->value.IsInt64())
                                        {
                                            secondaryParam.Set(key, it->value.GetInt64());
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    LOG(ERROR) << "DecodeDisplayParamData() Parse " << apiName
                               << ", UserGroup List index = " << UserGroupListIdx
                               << ", user_group = " << configItem.user_group
                               << ", exp_id = " << exp_id
                               << ", not find StrategyList.";
                    return DisplayConfig::Error::DecodeDisplayParamDataError;
                }
            }

            if (!def_group_user_conf)
            {
                LOG(ERROR) << "DecodeDisplayParamData() Default user group is missing or not the last one"
                           << ", apiName = " << apiName
                           << ", exp_id = " << exp_id;
            }
        }
    }
    return DisplayConfig::Error::OK;
}
