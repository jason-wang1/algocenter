#pragma once
#include <mutex>
#include <atomic>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "rapidjson/document.h"
#include "Common/Singleton.h"

#include "AlgoCenter/Define.h"
#include "AlgoCenter/APIType.h"

#include "Recall/RecallType.hpp"

struct RecallParam
{
    inline void Set(const std::string &key, long value)
    {
        longParams[key] = value;
    }

    inline void Set(const std::string &key, const std::string &value)
    {
        stringParams[key] = value;
    }

    inline std::string GetRecallType() const
    {
        auto it = stringParams.find("recallType");
        if (it != stringParams.end())
        {
            return it->second;
        }
        return "";
    }

    inline long GetRecallNum() const
    {
        auto it = longParams.find("recallNum");
        if (it != longParams.end())
        {
            return it->second;
        }
        return 0;
    }

    inline long GetMergeMaxNum() const
    {
        auto it = longParams.find("mergeMaxNum");
        if (it != longParams.end())
        {
            return it->second;
        }
        return 0;
    }

    inline long GetMergeMinNum() const
    {
        auto it = longParams.find("mergeMinNum");
        if (it != longParams.end())
        {
            return it->second;
        }
        return 0;
    }

    inline long GetUseTopKIndex() const
    {
        auto it = longParams.find("useTopKIndex");
        if (it != longParams.end())
        {
            return it->second;
        }
        return 0;
    }

    inline long GetSampleFold() const
    {
        auto it = longParams.find("sampleFold");
        if (it != longParams.end())
        {
            return it->second;
        }
        return 0;
    }

    inline long GetSingleMaxNum() const
    {
        auto it = longParams.find("singleMaxNum");
        if (it != longParams.end())
        {
            return it->second;
        }
        return 0;
    }

    inline long GetWeightPrecision() const
    {
        auto it = longParams.find("weightPrecision");
        if (it != longParams.end())
        {
            return it->second;
        }
        return 0;
    }

    inline long GetIsWeightAllocate() const
    {
        auto it = longParams.find("isWeightAllocate");
        if (it != longParams.end())
        {
            return it->second;
        }
        return 0;
    }

private:
    std::unordered_map<std::string, long> longParams;
    std::unordered_map<std::string, std::string> stringParams;
};

struct RecallParamData
{
    std::string user_group = ""; // 用户分组

    int AllMergeNum = 0;                      // 融合输出数量
    std::vector<RecallParam> vecRecallParams; // 召回/融合参数

    int AllSpareMergeNum = 0;                      // 备用融合输出数量
    std::vector<RecallParam> vecSpareRecallParams; // 备用召回参数

    std::vector<RecallParam> vecExclusiveRecallParams; // 独立召回参数
};

struct RecallConfigData
{
    std::unordered_map<int, std::unordered_map<int, std::vector<RecallParamData>>> m_RecallParamData;
};

class RecallConfig : public Singleton<RecallConfig>
{
public:
    typedef enum class RecallConfigErrorCode
    {
        OK = 0,
        JsonDataEmpty = 1,
        ParseError,
        DecodeRecallParamDataError, // 解析召回/融合配置出错
    } Error;

public:
    bool UpdateJson(const std::string &jsonData);

    const int MatchRecallExpID(const Common::APIType apiType, const std::vector<int> &vecExpIDList) const;
    const RecallParamData *GetRecallParamData(const std::unordered_set<std::string> &userGroupList, Common::APIType apiType, int exp_id) const;

    RecallConfig(token) { m_dataIdx = 0; }
    ~RecallConfig() {}
    RecallConfig(RecallConfig &) = delete;
    RecallConfig &operator=(const RecallConfig &) = delete;

protected:
    RecallConfig::Error DecodeJsonData(const std::string &jsonData);

    RecallConfig::Error DecodeRecallParamData(rapidjson::Document &doc, int dataIdx);

private:
    std::atomic<int> m_dataIdx = 0;
    RecallConfigData m_data[2];

    std::mutex m_update_lock; // 更新锁
};
