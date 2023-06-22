#include "Config.h"
#include "glog/logging.h"
#include "Common/Function.h"

Config::Error Config::LoadJson(const std::string &filename, const std::string &algoCenterFilename)
{
    std::string data;
    Common::FastReadFile(filename, data, false);
    if (data.empty())
    {
        LOG(ERROR) << "[Config::LoadJson()] read file: " << filename << " empty.";
        return Config::Error::JsonDataEmpty;
    }

    std::string algoCenterData;
    Common::FastReadFile(algoCenterFilename, algoCenterData, false);
    if (data.empty())
    {
        LOG(ERROR) << "[Config::LoadJson()] read file: " << algoCenterFilename << " empty.";
        return Config::Error::JsonDataEmpty;
    }

    return DecodeJsonData(data, algoCenterData);
}

Config::Error Config::DecodeJsonData(const std::string &jsonData, const std::string &algoCenterJsonData)
{
    rapidjson::Document doc;
    if (doc.Parse(jsonData.data()).HasParseError())
    {
        LOG(ERROR) << "DecodeJsonData() Parse jsonData HasParseError. err = " << doc.GetParseError();
        return Config::Error::ParseError;
    }

    rapidjson::Document algoCenterDoc;
    if (algoCenterDoc.Parse(algoCenterJsonData.data()).HasParseError())
    {
        LOG(ERROR) << "DecodeJsonData() Parse algoCenterJsonData HasParseError. err = " << doc.GetParseError();
        return Config::Error::ParseError;
    }

    // 缓存下标
    int idx = (m_dataIdx + 1) % 2;

    auto cfgErr = DecodeRegisterCenterAddr(algoCenterDoc, idx);
    if (cfgErr != Config::Error::OK)
    {
        return cfgErr;
    }

    cfgErr = DecodeServiceGroupTab(doc, idx);
    if (cfgErr != Config::Error::OK)
    {
        return cfgErr;
    }

    cfgErr = DecodeRedisConfig(algoCenterDoc, idx);
    if (cfgErr != Config::Error::OK)
    {
        return cfgErr;
    }

    cfgErr = DecodeDebugLog(doc, idx);
    if (cfgErr != Config::Error::OK)
    {
        return cfgErr;
    }

    cfgErr = DecodeTFSGRPCAddrList(algoCenterDoc, idx);
    if (cfgErr != Config::Error::OK)
    {
        return cfgErr;
    }

    cfgErr = DecodeTDKafkaConfig(algoCenterDoc, idx);
    if (cfgErr != Config::Error::OK)
    {
        return cfgErr;
    }

    cfgErr = DecodeKafkaPushConfig(doc, idx);
    if (cfgErr != Config::Error::OK)
    {
        return cfgErr;
    }

    cfgErr = DecodeAnnoyConfig(doc, idx);
    if (cfgErr != Config::Error::OK)
    {
        return cfgErr;
    }

    m_dataIdx = idx;

    return Config::Error::OK;
}

Config::Error Config::DecodeRegisterCenterAddr(rapidjson::Document &doc, int dataIdx)
{
    if (doc.HasMember("RegisterCenterAddr") && doc["RegisterCenterAddr"].IsArray())
    {
        auto smAddrList = doc["RegisterCenterAddr"].GetArray();
        m_data[dataIdx].m_RegisterCenterAddr.reserve(smAddrList.Size());
        for (rapidjson::SizeType idx = 0; idx < smAddrList.Size(); ++idx)
        {
            if (smAddrList[idx].IsString())
            {
                m_data[dataIdx].m_RegisterCenterAddr.push_back(smAddrList[idx].GetString());
            }
            else
            {
                LOG(ERROR) << "DecodeRegisterCenterAddr() Parse jsonData RegisterCenterAddr index = " << idx << ", not string.";
                return Config::Error::DecodeRegisterCenterAddrError;
            }
        }
    }
    else
    {
        LOG(ERROR) << "DecodeRegisterCenterAddr() Parse jsonData Not Find RegisterCenterAddr.";
        return Config::Error::DecodeRegisterCenterAddrError;
    }

    return Config::Error::OK;
}

Config::Error Config::DecodeServiceGroupTab(rapidjson::Document &doc, int dataIdx)
{
    if (doc.HasMember("ServiceGroupTab") && doc["ServiceGroupTab"].IsString())
    {
        m_data[dataIdx].m_ServiceGroupTab = doc["ServiceGroupTab"].GetString();
    }
    else
    {
        LOG(ERROR) << "DecodeJsonData() Parse jsonData Not Find ServiceGroupTab.";
        return Config::Error::DecodeServiceGroupTabError;
    }
    return Config::Error::OK;
}

Config::Error Config::DecodeRedisConfig(rapidjson::Document &doc, int dataIdx)
{
    if (!doc.HasMember("RedisList") || !doc["RedisList"].IsArray())
    {
        LOG(ERROR) << "DecodeRedisConfig() Parse jsonData Not Find RedisList.";
        return Config::Error::DecodeRedisConfigError;
    }

    auto redisList = doc["RedisList"].GetArray();
    for (rapidjson::SizeType idx = 0; idx < redisList.Size(); ++idx)
    {
        if (!redisList[idx].IsObject())
        {
            LOG(ERROR) << "DecodeRedisConfig() Parse Err, redis idx = " << idx;
            continue;
        }

        auto redis = redisList[idx].GetObject();

        RedisConfig redisConf;
        if (redis.HasMember("NameList") && redis["NameList"].IsArray())
        {
            auto nameList = redis["NameList"].GetArray();
            redisConf.NameList.reserve(nameList.Size());
            for (rapidjson::SizeType nl_idx = 0; nl_idx < nameList.Size(); ++nl_idx)
            {
                if (nameList[nl_idx].IsString())
                {
                    redisConf.NameList.push_back(nameList[nl_idx].GetString());
                }
                else
                {
                    LOG(ERROR) << "DecodeRedisConfig() Parse jsonData NameList index = " << nl_idx << ", not string.";
                    return Config::Error::DecodeRedisConfigError;
                }
            }
        }
        else
        {
            LOG(ERROR) << "DecodeRedisConfig() Parse jsonData Not Find NameList.";
            return Config::Error::DecodeRedisConfigError;
        }

        if (redis.HasMember("IP") && redis["IP"].IsString())
        {
            redisConf.IP = redis["IP"].GetString();
        }
        else
        {
            LOG(ERROR) << "DecodeRedisConfig() Parse jsonData Not Find Redis IP.";
            return Config::Error::DecodeRedisConfigError;
        }

        if (redis.HasMember("Port") && redis["Port"].IsInt())
        {
            redisConf.Port = redis["Port"].GetInt();
        }
        else
        {
            LOG(ERROR) << "DecodeRedisConfig() Parse jsonData Not Find Redis Port.";
            return Config::Error::DecodeRedisConfigError;
        }

        if (redis.HasMember("Crypto") && redis["Crypto"].IsBool())
        {
            redisConf.Crypto = redis["Crypto"].GetBool();
        }
        else
        {
            LOG(ERROR) << "DecodeRedisConfig() Parse jsonData Not Find Redis Crypto.";
            return Config::Error::DecodeRedisConfigError;
        }

        if (redis.HasMember("Password") && redis["Password"].IsString())
        {
            redisConf.Password = redis["Password"].GetString();
        }
        else
        {
            LOG(ERROR) << "DecodeRedisConfig() Parse jsonData Not Find Redis Password.";
            return Config::Error::DecodeRedisConfigError;
        }

        if (redis.HasMember("Index") && redis["Index"].IsInt())
        {
            int redisIndex = redis["Index"].GetInt();
            if (redisIndex < 0)
            {
                LOG(ERROR) << "DecodeRedisConfig() Parse jsonData, Redis Index is less then Zero.";
                return Config::Error::DecodeRedisConfigError;
            }
            redisConf.Index = redisIndex;
        }
        else
        {
            LOG(ERROR) << "DecodeRedisConfig() Parse jsonData Not Find Redis Index.";
            return Config::Error::DecodeRedisConfigError;
        }

        if (redis.HasMember("MaxPoolSize") && redis["MaxPoolSize"].IsInt())
        {
            int redisMaxPoolSize = redis["MaxPoolSize"].GetInt();
            if (redisMaxPoolSize < 0)
            {
                LOG(ERROR) << "DecodeRedisConfig() Parse jsonData, Redis MaxPoolSize is less then Zero.";
                return Config::Error::DecodeRedisConfigError;
            }
            redisConf.MaxPoolSize = redisMaxPoolSize;
        }
        else
        {
            LOG(ERROR) << "DecodeRedisConfig() Parse jsonData Not Find Redis MaxPoolSize.";
            return Config::Error::DecodeRedisConfigError;
        }

        m_data[dataIdx].m_RedisList.push_back(redisConf);
    }

    return Config::Error::OK;
}

Config::Error Config::DecodeDebugLog(rapidjson::Document &doc, int dataIdx)
{
    if (doc.HasMember("DebugLog") && doc["DebugLog"].IsObject())
    {
        auto debugLog = doc["DebugLog"].GetObject();
        if (debugLog.HasMember("Switch") && debugLog["Switch"].IsBool())
        {
            m_data[dataIdx].m_DebugLogSwitch = debugLog["Switch"].GetBool();
        }
        else
        {
            LOG(ERROR) << "DecodeDebugLog() Parse jsonData Not Find DebugLog Switch.";
            return Config::Error::DecodeDebugLogError;
        }

        if (debugLog.HasMember("Count") && debugLog["Count"].IsInt())
        {
            int debugLogCount = debugLog["Count"].GetInt();
            if (debugLogCount < 0)
            {
                LOG(ERROR) << "DecodeDebugLog() Parse jsonData, DebugLog Count is less then Zero.";
                return Config::Error::DecodeDebugLogError;
            }
            m_data[dataIdx].m_DebugLogCount = debugLogCount;
        }
        else
        {
            LOG(ERROR) << "DecodeDebugLog() Parse jsonData Not Find DebugLog Count.";
            return Config::Error::DecodeDebugLogError;
        }
    }
    else
    {
        LOG(ERROR) << "DecodeDebugLog() Parse jsonData Not Find DebugLog.";
        return Config::Error::DecodeDebugLogError;
    }
    return Config::Error::OK;
}

Config::Error Config::DecodeTFSGRPCAddrList(rapidjson::Document &doc, int dataIdx)
{
    if (!doc.HasMember("DefaultTFSGRPCAddrList") || !doc["DefaultTFSGRPCAddrList"].IsArray())
    {
        LOG(ERROR) << "DecodeTFSGRPCAddrList() Parse jsonData DefaultTFSGRPCAddrList Not Array.";
        return Config::Error::DecodeTFSGRPCAddrListError;
    }
    else
    {
        auto array = doc["DefaultTFSGRPCAddrList"].GetArray();
        auto array_size = array.Size();

        auto &vecAddrList = m_data[dataIdx].m_DefaultTFSGRPCAddrList;
        vecAddrList.clear();
        vecAddrList.reserve(array_size);
        for (rapidjson::SizeType idx = 0; idx < array_size; idx++)
        {
            if (!array[idx].IsString())
            {
                LOG(ERROR) << "DecodeTFSGRPCAddrList() Parse jsonData"
                           << ", DefaultTFSGRPCAddrList Not String"
                           << ", idx = " << idx;
                return Config::Error::DecodeTFSGRPCAddrListError;
            }
            vecAddrList.emplace_back(std::string(array[idx].GetString(),
                                                 array[idx].GetStringLength()));
        }
    }

    return Config::Error::OK;
}

Config::Error Config::DecodeTDKafkaConfig(rapidjson::Document &doc, int dataIdx)
{
    if (!doc.HasMember("TDKafka") || !doc["TDKafka"].IsObject())
    {
        LOG(ERROR) << "DecodeTDKafkaConfig() Parse jsonData Not Find TDKafka.";
        return Config::Error::DecodeTDKafkaConfigError;
    }
    auto tdKafka = doc["TDKafka"].GetObject();

    // BrokersList
    if (!tdKafka.HasMember("BrokersList") || !tdKafka["BrokersList"].IsArray())
    {
        LOG(ERROR) << "DecodeTDKafkaConfig() Parse jsonData Not Find BrokersList.";
        return Config::Error::DecodeTDKafkaConfigError;
    }
    auto brokersList = tdKafka["BrokersList"].GetArray();

    m_data[dataIdx].m_KafkaConf.m_VecBrokers.clear();
    m_data[dataIdx].m_KafkaConf.m_VecBrokers.reserve(brokersList.Size());
    for (unsigned int idx = 0; idx < brokersList.Size(); ++idx)
    {
        if (brokersList[idx].IsString())
        {
            m_data[dataIdx].m_KafkaConf.m_VecBrokers.emplace_back(brokersList[idx].GetString());
        }
        else
        {
            LOG(ERROR) << "DecodeTDKafkaConfig() Parse jsonData BrokersList index = " << idx << ", not string.";
            return Config::Error::DecodeTDKafkaConfigError;
        }
    }

    return Config::Error::OK;
}

Config::Error Config::DecodeKafkaPushConfig(rapidjson::Document &doc, int dataIdx)
{
    if (!doc.HasMember("KafkaPush") || !doc["KafkaPush"].IsObject())
    {
        LOG(ERROR) << "DecodeKafkaPushConfig() Parse jsonData Not Find KafkaPush.";
        return Config::Error::DecodeKafkaPushConfigError;
    }
    auto tdKafka = doc["KafkaPush"].GetObject();

    // Switch
    if (!tdKafka.HasMember("Switch") || !tdKafka["Switch"].IsBool())
    {
        LOG(ERROR) << "DecodeKafkaPushConfig() Parse jsonData Not Find Switch.";
        return Config::Error::DecodeKafkaPushConfigError;
    }
    m_data[dataIdx].m_KafkaPushSwitch = tdKafka["Switch"].GetBool();

    // ThreadNum
    if (!tdKafka.HasMember("ThreadNum") || !tdKafka["ThreadNum"].IsInt())
    {
        LOG(ERROR) << "DecodeKafkaPushConfig() Parse jsonData Not Find ThreadNum.";
        return Config::Error::DecodeKafkaPushConfigError;
    }
    m_data[dataIdx].m_KafkaPushThreadNum = tdKafka["ThreadNum"].GetInt();

    // Topic
    if (!tdKafka.HasMember("Topic") || !tdKafka["Topic"].IsString())
    {
        LOG(ERROR) << "DecodeKafkaPushConfig() Parse jsonData Not Find Topic.";
        return Config::Error::DecodeKafkaPushConfigError;
    }
    m_data[dataIdx].m_KafkaPushTopic = std::string(tdKafka["Topic"].GetString());

    return Config::Error::OK;
}

Config::Error Config::DecodeAnnoyConfig(rapidjson::Document &doc, int dataIdx)
{
    if (!doc.HasMember("Annoy") || !doc["Annoy"].IsObject())
    {
        LOG(ERROR) << "DecodeAnnoyConfig() Parse jsonData Not Find Annoy.";
        return Config::Error::DecodeAnnoyConfigError;
    }
    auto annoy = doc["Annoy"].GetObject();

    // BasicPath
    if (!annoy.HasMember("BasicPath") || !annoy["BasicPath"].IsString())
    {
        LOG(ERROR) << "DecodeAnnoyConfig() Parse jsonData Not Find BasicPath.";
        return Config::Error::DecodeAnnoyConfigError;
    }
    m_data[dataIdx].m_AnnoyBasicPath = std::string(annoy["BasicPath"].GetString());

    // EmbDimension
    if (!annoy.HasMember("EmbDimension") || !annoy["EmbDimension"].IsInt())
    {
        LOG(ERROR) << "DecodeAnnoyConfig() Parse jsonData Not Find EmbDimension.";
        return Config::Error::DecodeAnnoyConfigError;
    }
    m_data[dataIdx].m_AnnoyEmbDimension = annoy["EmbDimension"].GetInt();

    // SearchNodeNum
    if (!annoy.HasMember("SearchNodeNum") || !annoy["SearchNodeNum"].IsInt())
    {
        LOG(ERROR) << "DecodeAnnoyConfig() Parse jsonData Not Find SearchNodeNum.";
        return Config::Error::DecodeAnnoyConfigError;
    }
    m_data[dataIdx].m_AnnoySearchNodeNum = annoy["SearchNodeNum"].GetInt();

    return Config::Error::OK;
}
