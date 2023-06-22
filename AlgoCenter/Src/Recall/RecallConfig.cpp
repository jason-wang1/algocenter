#include "RecallConfig.h"
#include "glog/logging.h"
#include "Common/Function.h"

static const std::unordered_map<std::string, Common::APIType> RecallName2CmdType = {
    std::make_pair<std::string, Common::APIType>("Download_Recommend", Common::APIType::Get_Download_Recommend),
};

bool RecallConfig::UpdateJson(const std::string &jsonData)
{
    std::lock_guard<std::mutex> lg(m_update_lock);
    if (jsonData.empty())
    {
        LOG(ERROR) << "RecallConfig::UpdateJson() json_data empty.";
        return false;
    }
    return DecodeJsonData(jsonData) == RecallConfig::Error::OK;
}

const int RecallConfig::MatchRecallExpID(
    const Common::APIType apiType,
    const std::vector<int> &vecExpIDList) const
{
    int dataIdx = m_dataIdx;
    int type = static_cast<int>(apiType);
    auto itType = m_data[dataIdx].m_RecallParamData.find(type);
    if (itType != m_data[dataIdx].m_RecallParamData.end())
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

const RecallParamData *RecallConfig::GetRecallParamData(
    const std::unordered_set<std::string> &userGroupList, Common::APIType apiType, int exp_id) const
{
    int dataIdx = m_dataIdx;
    int type = static_cast<int>(apiType);

    const RecallParamData *pRecallParamData = nullptr;
    auto itType = m_data[dataIdx].m_RecallParamData.find(type);
    if (itType != m_data[dataIdx].m_RecallParamData.end())
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
                    pRecallParamData = &itemGroup;
                    break;
                }
            }
        }
    }
    return pRecallParamData;
}

RecallConfig::Error RecallConfig::DecodeJsonData(const std::string &jsonData)
{
    rapidjson::Document doc;
    if (doc.Parse(jsonData.data()).HasParseError())
    {
        LOG(ERROR) << "DecodeJsonData() Parse jsonData HasParseError. err = " << doc.GetParseError();
        return RecallConfig::Error::ParseError;
    }

    // 缓存下标
    int idx = (m_dataIdx + 1) % 2;

    // 解析召回融合配置
    auto cfgErr = DecodeRecallParamData(doc, idx);
    if (cfgErr != RecallConfig::Error::OK)
    {
        return cfgErr;
    }

    m_dataIdx = idx;

    return RecallConfig::Error::OK;
}

// 召回融合配置
RecallConfig::Error RecallConfig::DecodeRecallParamData(rapidjson::Document &doc, int dataIdx)
{
    if (!doc.HasMember("RecallAndMerge") || !doc["RecallAndMerge"].IsObject())
    {
        LOG(ERROR) << "DecodeRecallParamData() RecallAndMerge Not Find Or Not Object.";
        return RecallConfig::Error::DecodeRecallParamDataError;
    }
    auto RecallAndMergeObject = doc["RecallAndMerge"].GetObject();
    for (auto &api : RecallName2CmdType)
    {
        const char *apiName = api.first.c_str();
        int apiType = static_cast<int>(api.second);

        if (!RecallAndMergeObject.HasMember(apiName) || !RecallAndMergeObject[apiName].IsArray())
        {
            LOG(ERROR) << "DecodeRecallParamData() RecallAndMerge Not Find " << apiName;
            return RecallConfig::Error::DecodeRecallParamDataError;
        }
        auto apiParamList = RecallAndMergeObject[apiName].GetArray();

        rapidjson::SizeType ParamListSize = apiParamList.Size();
        for (rapidjson::SizeType ParamListIdx = 0; ParamListIdx < ParamListSize; ++ParamListIdx)
        {
            if (!apiParamList[ParamListIdx].IsObject())
            {
                LOG(ERROR) << "DecodeRecallParamData() Parse " << apiName
                           << ", Param List index = " << ParamListIdx
                           << ", not object.";
                return RecallConfig::Error::DecodeRecallParamDataError;
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
                LOG(ERROR) << "DecodeRecallParamData() Parse " << apiName
                           << ", Param List index = " << ParamListIdx
                           << ", not find exp_id.";
                return RecallConfig::Error::DecodeRecallParamDataError;
            }

            if (!ParamListItem.HasMember("UserGroupList") || !ParamListItem["UserGroupList"].IsArray())
            {
                LOG(ERROR) << "DecodeRecallParamData() Parse " << apiName
                           << ", Param List index = " << ParamListIdx
                           << ", not find UserGroupList.";
                return RecallConfig::Error::DecodeRecallParamDataError;
            }
            auto userGroupList = ParamListItem["UserGroupList"].GetArray();

            rapidjson::SizeType UserGroupListSize = userGroupList.Size();

            bool def_group_user_conf = false;
            auto &configItemList = m_data[dataIdx].m_RecallParamData[apiType][exp_id];
            configItemList.resize(UserGroupListSize);
            for (rapidjson::SizeType UserGroupListIdx = 0; UserGroupListIdx < UserGroupListSize; ++UserGroupListIdx)
            {
                if (!userGroupList[UserGroupListIdx].IsObject())
                {
                    LOG(ERROR) << "DecodeRecallParamData() Parse " << apiName
                               << ", exp_id = " << exp_id
                               << ", UserGroup List index = " << UserGroupListIdx
                               << ", not object.";
                    return RecallConfig::Error::DecodeRecallParamDataError;
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

                // AllMergeNum
                if (UserGroupListItem.HasMember("AllMergeNum") && UserGroupListItem["AllMergeNum"].IsInt())
                {
                    configItem.AllMergeNum = UserGroupListItem["AllMergeNum"].GetInt();
                }
                else
                {
                    LOG(ERROR) << "DecodeRecallParamData() Parse " << apiName
                               << ", UserGroup List index = " << UserGroupListIdx
                               << ", user_group = " << configItem.user_group
                               << ", exp_id = " << exp_id
                               << ", not find AllMergeNum.";
                    return RecallConfig::Error::DecodeRecallParamDataError;
                }

                // RecallAndMergeList 召回融合列表, 直接将所有参数作为Params解析
                if (!UserGroupListItem.HasMember("RecallAndMergeList") || !UserGroupListItem["RecallAndMergeList"].IsArray())
                {
                    LOG(ERROR) << "DecodeRecallParamData() Parse " << apiName
                               << ", UserGroup List index = " << UserGroupListIdx
                               << ", user_group = " << configItem.user_group
                               << ", exp_id = " << exp_id
                               << ", not find RecallAndMergeList.";
                    return RecallConfig::Error::DecodeRecallParamDataError;
                }
                auto RecallAndMergeArray = UserGroupListItem["RecallAndMergeList"].GetArray();
                rapidjson::SizeType RecallAndMergeArraySize = RecallAndMergeArray.Size();
                configItem.vecRecallParams.resize(RecallAndMergeArraySize);
                for (rapidjson::SizeType RecallAndMergeArrayIdx = 0; RecallAndMergeArrayIdx < RecallAndMergeArraySize; ++RecallAndMergeArrayIdx)
                {
                    if (!RecallAndMergeArray[RecallAndMergeArrayIdx].IsObject())
                    {
                        LOG(ERROR) << "DecodeRecallParamData() Parse " << apiName
                                   << ", RecallAndMerge Array index = " << RecallAndMergeArrayIdx
                                   << ", user_group = " << configItem.user_group
                                   << ", exp_id = " << exp_id
                                   << ", not object.";
                        return RecallConfig::Error::DecodeRecallParamDataError;
                    }

                    // 解析参数
                    auto &params = configItem.vecRecallParams[RecallAndMergeArrayIdx];
                    auto RecallAndMergeArrayItem = RecallAndMergeArray[RecallAndMergeArrayIdx].GetObject();
                    for (auto it = RecallAndMergeArrayItem.MemberBegin(); it != RecallAndMergeArrayItem.MemberEnd(); it++)
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

                // AllSpareMergeNum
                if (UserGroupListItem.HasMember("AllSpareMergeNum") && UserGroupListItem["AllSpareMergeNum"].IsInt())
                {
                    configItem.AllSpareMergeNum = UserGroupListItem["AllSpareMergeNum"].GetInt();
                }
                else
                {
                    LOG(ERROR) << "DecodeRecallParamData() Parse " << apiName
                               << ", UserGroup List index = " << UserGroupListIdx
                               << ", user_group = " << configItem.user_group
                               << ", exp_id = " << exp_id
                               << ", not find AllSpareMergeNum.";
                    return RecallConfig::Error::DecodeRecallParamDataError;
                }

                // SpareRecallList 备用召回列表, 直接将所有参数作为Params解析
                if (!UserGroupListItem.HasMember("SpareRecallList") || !UserGroupListItem["SpareRecallList"].IsArray())
                {
                    LOG(ERROR) << "DecodeRecallParamData() Parse " << apiName
                               << ", UserGroup List index = " << UserGroupListIdx
                               << ", user_group = " << configItem.user_group
                               << ", exp_id = " << exp_id
                               << ", not find SpareRecallList.";
                    return RecallConfig::Error::DecodeRecallParamDataError;
                }
                auto SpareRecallArray = UserGroupListItem["SpareRecallList"].GetArray();
                rapidjson::SizeType SpareRecallArraySize = SpareRecallArray.Size();
                configItem.vecSpareRecallParams.resize(SpareRecallArraySize);
                for (rapidjson::SizeType SpareRecallArrayIdx = 0; SpareRecallArrayIdx < SpareRecallArraySize; ++SpareRecallArrayIdx)
                {
                    if (!SpareRecallArray[SpareRecallArrayIdx].IsObject())
                    {
                        LOG(ERROR) << "DecodeRecallParamData() Parse " << apiName
                                   << ", SpareRecall Array index = " << SpareRecallArrayIdx
                                   << ", user_group = " << configItem.user_group
                                   << ", exp_id = " << exp_id
                                   << ", not object.";
                        return RecallConfig::Error::DecodeRecallParamDataError;
                    }

                    // 解析参数
                    auto &params = configItem.vecSpareRecallParams[SpareRecallArrayIdx];
                    auto SpareRecallArrayItem = SpareRecallArray[SpareRecallArrayIdx].GetObject();
                    for (auto it = SpareRecallArrayItem.MemberBegin(); it != SpareRecallArrayItem.MemberEnd(); it++)
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

                // ExclusiveRecallList 独立召回列表, 直接将所有参数作为Params解析
                if (!UserGroupListItem.HasMember("ExclusiveRecallList") || !UserGroupListItem["ExclusiveRecallList"].IsArray())
                {
                    LOG(ERROR) << "DecodeRecallParamData() Parse " << apiName
                               << ", UserGroup List index = " << UserGroupListIdx
                               << ", user_group = " << configItem.user_group
                               << ", exp_id = " << exp_id
                               << ", not find ExclusiveRecallList.";
                    return RecallConfig::Error::DecodeRecallParamDataError;
                }
                auto ExclusiveRecallArray = UserGroupListItem["ExclusiveRecallList"].GetArray();
                rapidjson::SizeType ExclusiveRecallArraySize = ExclusiveRecallArray.Size();
                configItem.vecExclusiveRecallParams.resize(ExclusiveRecallArraySize);
                for (rapidjson::SizeType ExclusiveRecallArrayIdx = 0; ExclusiveRecallArrayIdx < ExclusiveRecallArraySize; ++ExclusiveRecallArrayIdx)
                {
                    if (!ExclusiveRecallArray[ExclusiveRecallArrayIdx].IsObject())
                    {
                        LOG(ERROR) << "DecodeRecallParamData() Parse " << apiName
                                   << ", ExclusiveRecall Array index = " << ExclusiveRecallArrayIdx
                                   << ", user_group = " << configItem.user_group
                                   << ", exp_id = " << exp_id
                                   << ", not object.";
                        return RecallConfig::Error::DecodeRecallParamDataError;
                    }

                    // 解析参数
                    auto &params = configItem.vecExclusiveRecallParams[ExclusiveRecallArrayIdx];
                    auto ExclusiveRecallArrayItem = ExclusiveRecallArray[ExclusiveRecallArrayIdx].GetObject();
                    for (auto it = ExclusiveRecallArrayItem.MemberBegin(); it != ExclusiveRecallArrayItem.MemberEnd(); it++)
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
                LOG(ERROR) << "DecodeRecallParamData() Default user group is missing or not the last one"
                           << ", apiName = " << apiName
                           << ", exp_id = " << exp_id;
            }
        }
    }
    return RecallConfig::Error::OK;
}