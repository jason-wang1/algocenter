#pragma once
#include <mutex>
#include <atomic>
#include <memory>
#include <vector>
#include <unordered_map>
#include "rapidjson/document.h"
#include "Common/Singleton.h"

#include "KafkaClient/TDKafkaProducer.h"

#include "Define.h"
#include "APIType.h"

struct RedisConfig
{
    std::vector<std::string> NameList;
    std::string IP;
    int Port = 0;
    bool Crypto = false;
    std::string Password;
    int Index = 0;
    int MaxPoolSize = 0;
};

struct ConfigData
{
    // 注册中心地址
    std::vector<std::string> m_RegisterCenterAddr;

    // 服务分组标签
    std::string m_ServiceGroupTab;

    // Redis配置
    std::vector<RedisConfig> m_RedisList;

    // Log输出配置
    bool m_DebugLogSwitch = false;
    int m_DebugLogCount = 0;

    // TDPredict 模型地址配置
    std::vector<std::string> m_DefaultTFSGRPCAddrList; // TFS GRPC 默认调用地址

    // Kafka推送配置
    TDKafkaConfig m_KafkaConf;
    bool m_KafkaPushSwitch = false;    // 推送开关
    int m_KafkaPushThreadNum = 0;      // 推送线程数量
    std::string m_KafkaPushTopic = ""; // 推送Topic

    // Annoy配置
    std::string m_AnnoyBasicPath;
    int m_AnnoyEmbDimension;
    int m_AnnoySearchNodeNum = 0;
};

class Config : public Singleton<Config>
{
public:
    typedef enum class ConfigErrorCode
    {
        OK = 0,
        JsonDataEmpty = 1,
        ParseError,
        DecodeRegisterCenterAddrError, // 解析注册中心地址配置出错
        DecodeServiceGroupTabError,    // 解析服务分组标签配置出错
        DecodeRedisConfigError,        // 解析Redis配置出错
        DecodeDebugLogError,           // 解析信息日志配置出错
        DecodeTFSGRPCAddrListError,    // 解析TFS地址列表出错
        DecodeTDKafkaConfigError,      // 解析TDKafka配置出错
        DecodeKafkaPushConfigError,    // 解析Kafka推送配置出错
        DecodeAnnoyConfigError,        // 解析Annoy配置出错
    } Error;

public:
    Config::Error LoadJson(const std::string &filename, const std::string &algoCenterFilename);

    const std::vector<std::string> &GetRegisterCenterAddr() const noexcept
    {
        return m_data[m_dataIdx].m_RegisterCenterAddr;
    }

    inline const std::string &GetServiceGroupTab() const noexcept
    {
        return std::cref(m_data[m_dataIdx].m_ServiceGroupTab);
    }

    const std::vector<RedisConfig> &GetRedisConfig() const noexcept
    {
        return m_data[m_dataIdx].m_RedisList;
    }

    bool IsDebugLogSwitch() const noexcept
    {
        return m_data[m_dataIdx].m_DebugLogSwitch;
    }

    int GetDebugLogCount() const noexcept
    {
        return m_data[m_dataIdx].m_DebugLogCount;
    }

    const std::vector<std::string> &GetDefaultTFSGRPCAddrList() const noexcept
    {
        return m_data[m_dataIdx].m_DefaultTFSGRPCAddrList;
    }

    const TDKafkaConfig &GetTDKafkaConf() const noexcept
    {
        return m_data[m_dataIdx].m_KafkaConf;
    }

    inline bool GetKafkaPushSwitch() const noexcept
    {
        return m_data[m_dataIdx].m_KafkaPushSwitch;
    }

    const int GetKafkaPushThreadNum() const noexcept
    {
        return m_data[m_dataIdx].m_KafkaPushThreadNum;
    }

    std::string GetKafkaPushTopic() const noexcept
    {
        return m_data[m_dataIdx].m_KafkaPushTopic;
    }

    std::string GetAnnoyBasicPath() const noexcept
    {
        return m_data[m_dataIdx].m_AnnoyBasicPath;
    }

    const int GetAnnoyEmbDimension() const noexcept
    {
        return m_data[m_dataIdx].m_AnnoyEmbDimension;
    }

    const int GetAnnoySearchNodeNum() const noexcept
    {
        return m_data[m_dataIdx].m_AnnoySearchNodeNum;
    }

    Config(token) { m_dataIdx = 0; }
    ~Config() {}
    Config(Config &) = delete;
    Config &operator=(const Config &) = delete;

protected:
    Config::Error DecodeJsonData(const std::string &jsonData, const std::string &algoCenterJsonData);

    Config::Error DecodeRegisterCenterAddr(rapidjson::Document &doc, int dataIdx);
    Config::Error DecodeServiceGroupTab(rapidjson::Document &doc, int dataIdx);
    Config::Error DecodeRedisConfig(rapidjson::Document &doc, int dataIdx);
    Config::Error DecodeDebugLog(rapidjson::Document &doc, int dataIdx);
    Config::Error DecodeTFSGRPCAddrList(rapidjson::Document &doc, int dataIdx);
    Config::Error DecodeTDKafkaConfig(rapidjson::Document &doc, int dataIdx);
    Config::Error DecodeKafkaPushConfig(rapidjson::Document &doc, int dataIdx);
    Config::Error DecodeAnnoyConfig(rapidjson::Document &doc, int dataIdx);

private:
    std::atomic<int> m_dataIdx = 0;
    ConfigData m_data[2];
};
