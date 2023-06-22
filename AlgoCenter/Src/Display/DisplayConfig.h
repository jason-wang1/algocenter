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

#include "Display/DisplayType.h"

struct SecondaryParam
{
    inline void Set(const std::string &key, long value)
    {
        longParams[key] = value;
    }

    inline void Set(const std::string &key, const std::string &value)
    {
        stringParams[key] = value;
    }

    inline long GetControlCount() const
    {
        auto it = longParams.find("ControlCount");
        if (it != longParams.end())
        {
            return it->second;
        }
        return 0;
    }

    inline std::string GetElementList() const
    {
        auto it = stringParams.find("ElementList");
        if (it != stringParams.end())
        {
            return it->second;
        }
        return "";
    }

    inline std::string GetControlMethod() const
    {
        auto it = stringParams.find("ControlMethod");
        if (it != stringParams.end())
        {
            return it->second;
        }
        return "";
    }

private:
    std::unordered_map<std::string, long> longParams;
    std::unordered_map<std::string, std::string> stringParams;
};

struct SecondaryParamData
{
    std::vector<SecondaryParam> vecSecondaryParam;
};

struct DisplayParam
{
    inline void Set(const std::string &key, long value)
    {
        longParams[key] = value;
    }

    inline void Set(const std::string &key, double value)
    {
        doubleParams[key] = value;
    }

    inline void Set(const std::string &key, const std::string &value)
    {
        stringParams[key] = value;
    }

    inline std::string GetDisplayType() const
    {
        auto it = stringParams.find("DisplayType");
        if (it != stringParams.end())
        {
            return it->second;
        }
        return "";
    }

    inline long GetPageSize() const
    {
        auto it = longParams.find("PageSize");
        if (it != longParams.end())
        {
            return it->second;
        }
        return 0;
    }

    inline long GetTopNPage() const
    {
        auto it = longParams.find("TopNPage");
        if (it != longParams.end())
        {
            return it->second;
        }
        return 0;
    }

    inline long GetTopNDisplay() const
    {
        auto it = longParams.find("TopNDisplay");
        if (it != longParams.end())
        {
            return it->second;
        }
        return 0;
    }

    inline const SecondaryParamData *GetScattersParam() const
    {
        const SecondaryParamData *secondaryParamDataPtr = nullptr;
        auto it = secondaryParamData.find("Scatters");
        if (it != secondaryParamData.end())
        {
            secondaryParamDataPtr = &it->second;
        }

        return secondaryParamDataPtr;
    }

    inline SecondaryParamData &GetSecondaryParamData(const std::string &key)
    {
        return secondaryParamData[key];
    }

private:
    std::unordered_map<std::string, long> longParams;
    std::unordered_map<std::string, double> doubleParams;
    std::unordered_map<std::string, std::string> stringParams;

private:
    std::unordered_map<std::string, SecondaryParamData> secondaryParamData;
};

struct DisplayParamData
{
    std::string user_group = ""; // 用户分组

    std::vector<DisplayParam> vecDisplayParams; // 展控策略参数
};

struct DisplayConfigData
{
    std::unordered_map<int, std::unordered_map<int, std::vector<DisplayParamData>>> m_DisplayParamData;
};

class DisplayConfig : public Singleton<DisplayConfig>
{
public:
    typedef enum class DisplayErrorCode
    {
        OK = 0,
        JsonDataEmpty = 1,
        ParseError,
        DecodeDisplayParamDataError, // 解析展控配置出错
    } Error;

public:
    bool UpdateJson(const std::string &jsonData);

    const int MatchDisplayExpID(const Common::APIType apiType, const std::vector<int> &vecExpIDList) const;
    const DisplayParamData *GetDisplayParamData(const std::unordered_set<std::string> &userGroupList, Common::APIType apiType, int exp_id) const;

    DisplayConfig(token) { m_dataIdx = 0; }
    ~DisplayConfig() {}
    DisplayConfig(DisplayConfig &) = delete;
    DisplayConfig &operator=(const DisplayConfig &) = delete;

protected:
    DisplayConfig::Error DecodeJsonData(const std::string &jsonData);

    DisplayConfig::Error DecodeDisplayParamData(rapidjson::Document &doc, int dataIdx);

private:
    std::atomic<int> m_dataIdx = 0;
    DisplayConfigData m_data[2];

    std::mutex m_update_lock; // 更新锁
};
