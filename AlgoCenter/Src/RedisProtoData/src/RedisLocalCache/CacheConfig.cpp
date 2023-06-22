#include "CacheConfig.h"
#include "glog/logging.h"
#include "Common/Function.h"

int CacheConfig::Init(const std::string &filename)
{
    std::string data;
    Common::FastReadFile(filename, data, false);
    if (data.empty())
    {
        LOG(ERROR) << "Config::Init() read file: " << filename << " empty.";
        return CacheConfig::Error::JsonDataEmpty;
    }
    return DecodeJsonData(data);
}

int CacheConfig::DecodeJsonData(const std::string &jsonData)
{
    rapidjson::Document doc;
    if (doc.Parse(jsonData.data()).HasParseError())
    {
        LOG(ERROR) << "DecodeJsonData() Parse jsonData HasParseError. err = " << doc.GetParseError();
        return CacheConfig::Error::ParseError;
    }

    // 缓存下标
    int idx = (m_dataIdx + 1) % 2;

    // 解析缓存参数
    auto cfgErr = DecodeCacheParam(doc, idx);
    if (cfgErr != CacheConfig::Error::OK)
    {
        return cfgErr;
    }

    // 解析刷新时间参数
    cfgErr = DecodeUpdateTime(doc, idx);
    if (cfgErr != CacheConfig::Error::OK)
    {
        return cfgErr;
    }

    m_dataIdx = idx;

    return CacheConfig::Error::OK;
}

int CacheConfig::DecodeCacheParam(rapidjson::Document &doc, int dataIdx)
{
    auto &data = m_data[dataIdx];
    if (doc.HasMember("CacheParamArray") && doc["CacheParamArray"].IsArray())
    {
        // 解析参数
        auto cacheParamArray = doc["CacheParamArray"].GetArray();
        auto array_size = cacheParamArray.Size();
        data.vecCacheConfig.resize(array_size);
        for (rapidjson::SizeType idx = 0; idx != array_size; idx++)
        {
            auto &param = data.vecCacheConfig[idx];
            auto &conf = cacheParamArray[idx];

            // 解析参数
            for (auto it = conf.MemberBegin(); it != conf.MemberEnd(); it++)
            {
                std::string key = it->name.GetString();
                if (key == "Name")
                {
                    std::string name = std::string(conf["Name"].GetString(), conf["Name"].GetStringLength());
                    param.SetCacheName(name);
                }
                else if (key == "RefreshTime")
                {
                    param.SetRefreshTime(conf["RefreshTime"].GetInt64());
                }
                else if (key == "RefreshIncrTime")
                {
                    param.SetRefreshIncrTime(conf["RefreshIncrTime"].GetInt64());
                }
                else
                {
                    if (it->value.IsString())
                    {
                        std::string value(it->value.GetString(), it->value.GetStringLength());
                        param.SetStrParam(key, value);
                    }
                    else if (it->value.IsInt64())
                    {
                        param.SetLongParam(key, it->value.GetInt64());
                    }
                    else
                    {
                        LOG(ERROR) << "DecodeCacheParam() Parse CacheParam Type Not Support"
                                   << ", name = " << key
                                   << ", idx = " << idx;
                        return CacheConfig::Error::DecodeCacheParamError;
                    }
                }
            }
        }
    }
    else
    {
        LOG(ERROR) << "DecodeCacheParam() Parse jsonData Not Find CacheParamArray.";
        return CacheConfig::Error::DecodeCacheParamError;
    }
    return CacheConfig::Error::OK;
}

int CacheConfig::DecodeUpdateTime(rapidjson::Document &doc, int dataIdx)
{
    auto &data = m_data[dataIdx];
    if (doc.HasMember("UpdateTimeArray") && doc["UpdateTimeArray"].IsArray())
    {
        auto updateTimeArray = doc["UpdateTimeArray"].GetArray();
        auto array_size = updateTimeArray.Size();
        data.vecUpdateTime.resize(array_size);
        for (rapidjson::SizeType idx = 0; idx != array_size; idx++)
        {
            if (updateTimeArray[idx].IsInt64())
            {
                data.vecUpdateTime[idx] = updateTimeArray[idx].GetInt64();
            }
            else
            {
                LOG(ERROR) << "DecodeUpdateTimeError() Parse UpdateTimeArray Type Not Int64"
                           << ", idx = " << idx;
                return CacheConfig::Error::DecodeCacheParamError;
            }
        }
    }
    else
    {
        LOG(ERROR) << "DecodeUpdateTime() Parse jsonData Not Find UpdateTimeArray.";
        return CacheConfig::Error::DecodeUpdateTimeError;
    }

    if (doc.HasMember("UpdateTimeMinSleep") && doc["UpdateTimeMinSleep"].IsInt64())
    {
        data.UpdateTimeMinSleep = doc["UpdateTimeMinSleep"].GetInt64();
    }
    else
    {
        LOG(ERROR) << "DecodeUpdateTime() Parse jsonData Not Find UpdateTimeMinSleep.";
        return CacheConfig::Error::DecodeUpdateTimeError;
    }

    if (doc.HasMember("UpdateTimeMaxLoop") && doc["UpdateTimeMaxLoop"].IsInt64())
    {
        data.UpdateTimeMaxLoop = doc["UpdateTimeMaxLoop"].GetInt64();
    }
    else
    {
        LOG(ERROR) << "DecodeUpdateTime() Parse jsonData Not Find UpdateTimeMaxLoop.";
        return CacheConfig::Error::DecodeUpdateTimeError;
    }

    return CacheConfig::Error::OK;
}