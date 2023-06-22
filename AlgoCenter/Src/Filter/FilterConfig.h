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

struct FilterParam
{
    inline void Set(const std::string &key, long value)
    {
        longParams[key] = value;
    }

    inline void Set(const std::string &key, const std::string &value)
    {
        stringParams[key] = value;
    }

    inline std::string GetFilterType() const
    {
        auto it = stringParams.find("FilterType");
        if (it != stringParams.end())
        {
            return it->second;
        }
        return "";
    }

    inline long GetFilterLogSwitch() const
    {
        auto it = longParams.find("FilterLogSwitch");
        if (it != longParams.end())
        {
            return it->second;
        }
        return 0;
    }

    inline long GetValidityTime() const
    {
        auto it = longParams.find("ValidityTime");
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

struct FilterParamData
{
    std::string user_group = ""; // 用户分组

    int KeepItemNum = 0;                      // 保持Item数量
    std::vector<FilterParam> vecFilterParams; // 过滤参数
};

struct FilterConfigData
{
    std::unordered_map<int, std::unordered_map<int, std::vector<FilterParamData>>> m_FilterParamData;
};

class FilterConfig : public Singleton<FilterConfig>
{
public:
    typedef enum class FilterConfigErrorCode
    {
        OK = 0,
        JsonDataEmpty = 1,
        ParseError,
        DecodeFilterParamDataError, // 解析召回/融合配置出错
    } Error;

public:
    bool UpdateJson(const std::string &jsonData);

    const int MatchFilterExpID(const Common::APIType apiType, const std::vector<int> &vecExpIDList) const;
    const FilterParamData *GetFilterParamData(const std::unordered_set<std::string> &userGroupList, Common::APIType apiType, int exp_id) const;

    FilterConfig(token) { m_dataIdx = 0; }
    ~FilterConfig() {}
    FilterConfig(FilterConfig &) = delete;
    FilterConfig &operator=(const FilterConfig &) = delete;

protected:
    FilterConfig::Error DecodeJsonData(const std::string &jsonData);

    FilterConfig::Error DecodeFilterParamData(rapidjson::Document &doc, int dataIdx);

private:
    std::atomic<int> m_dataIdx = 0;
    FilterConfigData m_data[2];

    std::mutex m_update_lock; // 更新锁
};
