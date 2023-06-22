#pragma once
#include <string>
#include <vector>
#include <sstream>

#include "Common/Function.h"

#include "Define.h"
#include "APIType.h"

#include "Feature/UserFeature.h"
#include "Feature/ItemFeature.h"

class RequestData
{
public:
    int32_t InitUserFeature() noexcept
    {
        int err = this->userFeature.InitUserFeature(this->user_id, this->context_item_id, this->context_res_type);
        if (Common::Error::OK != err)
        {
            return err;
        }

        // 如果未命中其他用户组，则归为默认组，方便日志分析
        if (this->userGroupList.empty())
        {
            this->userGroupList.insert(Common::DefaultUserGroup);
        }

        return Common::Error::OK;
    }

    const UserFeature &GetUserFeature() const noexcept
    {
        return this->userFeature;
    }

public:
    const std::string LogOutExpIDList(const std::vector<int> &expIdList) const
    {
        std::string result;
        Common::ConvertToString(result, expIdList, ",");
        return "[" + result + "]";
    }

    const std::string LogOutKeynames(const std::vector<std::string> &vecKeynames) const
    {
        std::string result;
        Common::ConvertToString(result, vecKeynames, ",");
        return "[" + result + "]";
    }

    const std::string LogOutUserGroup(const std::unordered_set<std::string> &userGroupList) const
    {
        std::string result;
        Common::ConvertToString(result, userGroupList, ",");
        return "[" + result + "]";
    }

public:
    std::string uuid = "";
    Common::APIType apiType;
    std::vector<int> vecExpID; // exp_id的列表, 各层级自行匹配
    long user_id = 0;
    int ret_count = 0;
    long context_item_id = 0;
    int context_res_type = 0;
    std::vector<std::string> vecKeynames; // 物料关键词列表

    // 用户所属组集合
    std::unordered_set<std::string> userGroupList;

    // 各层级匹配的Exp_id
    int filterExpID = 0;
    int recallExpID = 0;
    int rankExpID = 0;
    int displayExpID = 0;

    // 各层级匹配的用户组
    std::string recallUserGroup = "";
    std::string filterUserGroup = "";
    std::string displayUserGroup = "";

    // 排序截断数量
    int rankTruncCount = 1000;

    // 是否输出统计日志
    bool is_statis_log = false;

public:
    UserFeature userFeature;
    ItemFeature itemFeature;
};
