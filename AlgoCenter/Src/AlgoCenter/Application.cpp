#include "Application.h"
#include <thread>
#include <unordered_map>
#include "glog/logging.h"
#include "timer/timer.h"
#include "TDRedis/TDRedisConnPool.h"

#include "Common/Function.h"
#include "GrpcDispatcher/GrpcReceiver.h"
#include "GrpcDispatcher/GrpcDispatcher.h"
#include "RegisterCenter/RegisterCenter.h"
#include "KafkaClient/PushKafkaPool.h"

#include "RedisProtoData/include/RedisProtoData.h"
#include "Recall/RecallConfig.h"
#include "Recall/AnnoyRecall/AnnoyIndexCache.h"
#include "Filter/FilterConfig.h"
#include "Display/DisplayConfig.h"

#include "Define.h"
#include "Config.h"
#include "AlgoCenter.h"

#include "TDPredict/Config/ModelConfig.h"
#include "TDPredict/Config/RankExpConfig.h"
#include "Feature/FeatureField.h"

bool Application::Init(
    const std::string &IP,
    const std::string &Port,
    const std::string &Exe,
    const std::string &Nicename)
{
    LOG(INFO) << "Application->Init("
              << "IP = " << IP
              << ", Port = " << Port
              << ", Exe = " << Exe
              << ", Nicename = " << Nicename;
    // 注册信号
    CSignal::installSignal(SIGINT, notifyShutdown);
    CSignal::installSignal(SIGQUIT, notifyShutdown);
    CSignal::installSignal(SIGTERM, notifyShutdown);
    CSignal::installSignal(SignalNumber::ReloadConfig, notifyConfigUpdate); // ReloadConfig = 35
    CSignal::installSignal(SignalNumber::StopProcess, notifyShutdown);      // StopProcess = 36

    // 读取配置
    conf = Config::GetInstance();
    if (Config::Error::OK != conf->LoadJson(Common::ConfigPath_App, Common::ConfigPath_AlgoCenter))
    {
        LOG(ERROR) << "LoadJson error"
                   << ", App Config Path = " << Common::ConfigPath_App
                   << ", AlgoCenter Config Path = " << Common::ConfigPath_AlgoCenter;
        return false;
    }

    // 初始化Redis连接池
    const auto &vecRedisConfig = conf->GetRedisConfig();
    for (const auto &redis : vecRedisConfig)
    {
        for (const auto &redisName : redis.NameList)
        {
            if (!RedisProtoData::AddConnectPool(
                    redisName, redis.IP, redis.Port,
                    redis.Crypto, redis.Password,
                    redis.Index, redis.MaxPoolSize))
            {
                LOG(ERROR) << "Init Redis Conn Pool Error"
                           << ", redisName = " << redisName
                           << ", redis.IP = " << redis.IP
                           << ", redis.Port = " << redis.Port
                           << ", redis.Crypto = " << redis.Crypto
                           << ", redis.Password = " << redis.Password
                           << ", redis.Index = " << redis.Index
                           << ", redis.MaxPoolSize = " << redis.MaxPoolSize;
                return false;
            }
            else
            {
                LOG(INFO) << "Init Redis Conn Pool Succ, redisName = " << redisName;
            }
        }
    }

    // 初始化Redis数据缓存模块
    if (Common::Error::OK != RedisProtoData::InitCache(Common::ConfigPath_Cache))
    {
        LOG(ERROR) << "Init RedisProtoData Cache Error, Cache Config Path = " << Common::ConfigPath_Cache;
        return false;
    }
    else
    {
        LOG(INFO) << "Init RedisProtoData Cache Succ.";
    }

    // 初始化 PushKafkaPool
    if (!PushKafkaPool::GetInstance()->Init(conf->GetKafkaPushThreadNum(), conf->GetTDKafkaConf()))
    {
        LOG(ERROR) << "Init PushKafkaPool Error!";
        return false;
    }
    else
    {
        LOG(INFO) << "Init PushKafkaPool Succ.";
    }

    // 初始化Annoy索引缓存模块
    if (!AnnoyIndexCache::GetInstance()->Init(
            conf->GetAnnoyBasicPath(),
            conf->GetAnnoyEmbDimension(),
            conf->GetAnnoySearchNodeNum()))
    {
        LOG(ERROR) << "Init AnnoyIndexCache Error!";
        return false;
    }
    else
    {
        LOG(INFO) << "Init AnnoyIndexCache Succ.";
    }

    // 注册服务接口
    algoCenter = std::make_shared<AlgoCenter>();
    if (!algoCenter->Init())
    {
        LOG(ERROR) << "Init AlgoCenter Error.";
        return false;
    }
    else
    {
        LOG(INFO) << "Init AlgoCenter Succ.";
    }

    // 本地服务信息
    GrpcProtos::ServiceInfo serviceInfo;
    serviceInfo.set_service_type(GrpcProtos::ServiceType::SERVICE_ALGO_CENTER);
    serviceInfo.set_semver(Common::APP_VERSION);
    serviceInfo.set_addr(IP + ":" + Port);
    serviceInfo.set_host_name(Common::get_hostname());
    serviceInfo.set_status(GrpcProtos::ServiceStatus::Online);
    serviceInfo.set_group_tab(conf->GetServiceGroupTab());
    serviceInfo.set_service_name(Exe);
    serviceInfo.set_nickname(Nicename);

    if (!RegisterCenter::GetInstance()->Init(conf->GetRegisterCenterAddr(), serviceInfo))
    {
        LOG(ERROR) << "Init Register Client Failed.";
        return false;
    }

    // 配置更新
    if (!ConfigUpdate())
    {
        LOG(ERROR) << "ConfigUpdate Failed.";
        return false;
    }

    return true;
}

bool Application::StartProcess(
    const std::string &Port)
{
    std::string _addr = "0.0.0.0:" + Port;
    LOG(INFO) << "Application::StartProcess() addr:" << _addr;

    // 定义服务接收器
    grpcService = std::make_shared<GrpcReceiver>();

    // 注册, 启动服务
    grpc::ServerBuilder builder;
    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 10000);
    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 10000);
    builder.AddChannelArgument(GRPC_ARG_HTTP2_BDP_PROBE, 1);
    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
    builder.AddChannelArgument(GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS, 5000);
    builder.AddChannelArgument(GRPC_ARG_HTTP2_MIN_SENT_PING_INTERVAL_WITHOUT_DATA_MS, 10000);
    builder.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_GZIP);
    builder.AddListeningPort(_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(grpcService.get()); // 注册服务接收器
    grpcServer = builder.BuildAndStart();
    if (grpcServer == nullptr)
    {
        LOG(ERROR) << "StartProcess Error, BuildAndStart grpcServer Failed.";
        return false;
    }

    // 服务上线
    if (!RegisterCenter::GetInstance()->Online())
    {
        LOG(ERROR) << "StartProcess Error, Register Client Online error";
        return false;
    }

    // 服务启动完毕
    return true;
}

bool Application::Waiting()
{
    // 起线程 等待grpc线程退出
    std::thread serving_thread(
        [&]()
        {
            LOG(INFO) << "grpcServer Waiting ...";
            grpcServer->Wait();
        });

    // 等待退出消息
    auto quit = exit_requested.get_future();
    quit.wait();

    // 得到系统退出信号, 开始执行退出程序
    LOG(INFO) << "Get Shutdown Signal, exit StartProcess";

    // 向服务管理报送退出
    RegisterCenter::GetInstance()->Offline();

    // sleep 1s, 等待上级服务状态同步
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 停止grpc任务, 3秒等待grpc退出
    grpcServer->Shutdown(
        std::chrono::system_clock::now() +
        std::chrono::seconds(3));
    if (serving_thread.joinable())
    {
        serving_thread.join();
    }
    return true;
}

// 更新配置
bool Application::ConfigUpdate()
{
    auto &app = Application::GetInstance();
    std::lock_guard<std::mutex> lg(app->config_updae_lock);

    // 更新召回配置
    {
        std::string jsonData;
        Common::FastReadFile(Common::ConfigPath_Recall, jsonData, false);
        if (jsonData.empty())
        {
            LOG(ERROR) << "ConfigUpdate() read file: " << Common::ConfigPath_Recall << " empty.";
            return false;
        }

        if (!RecallConfig::GetInstance()->UpdateJson(jsonData))
        {
            LOG(ERROR) << "RecallConfig UpdateJson Error, Config Path = " << Common::ConfigPath_Recall
                       << ", jsonData : " << jsonData;
            return false;
        }
    }

    // 过滤
    {
        std::string jsonData;
        Common::FastReadFile(Common::ConfigPath_Filter, jsonData, false);
        if (jsonData.empty())
        {
            LOG(ERROR) << "ConfigUpdate() read file: " << Common::ConfigPath_Filter << " empty.";
            return false;
        }

        if (!FilterConfig::GetInstance()->UpdateJson(jsonData))
        {
            LOG(ERROR) << "FilterConfig UpdateJson Error, Config Path = " << Common::ConfigPath_Filter
                       << ", jsonData : " << jsonData;
            return false;
        }
    }

    // 模型
    {
        std::string jsonData;
        Common::FastReadFile(Common::ConfigPath_Model, jsonData, false);
        if (jsonData.empty())
        {
            LOG(ERROR) << "ConfigUpdate() read file: " << Common::ConfigPath_Model << " empty.";
            return false;
        }

        auto &model_config = TDPredict::Config::ModelConfig::GetInstance();
        model_config->SetDefaultGRPCAddrList(app->conf->GetDefaultTFSGRPCAddrList());
        if (!model_config->UpdateJson(jsonData))
        {
            LOG(ERROR) << "ModelConfig UpdateJson Error, Config Path = " << Common::ConfigPath_Model
                       << ", jsonData : " << jsonData;
            return false;
        }
    }

    // 排序
    {
        std::string jsonData;
        Common::FastReadFile(Common::ConfigPath_Rank, jsonData, false);
        if (jsonData.empty())
        {
            LOG(ERROR) << "ConfigUpdate() read file: " << Common::ConfigPath_Rank << " empty.";
            return false;
        }

        if (!TDPredict::Config::RankExpConfig::GetInstance()->UpdateJson(jsonData))
        {
            LOG(ERROR) << "RankExpConfig UpdateJson Error, Config Path = " << Common::ConfigPath_Rank
                       << ", jsonData : " << jsonData;
            return false;
        }
    }

    // 展控
    {
        std::string jsonData;
        Common::FastReadFile(Common::ConfigPath_Display, jsonData, false);
        if (jsonData.empty())
        {
            LOG(ERROR) << "ConfigUpdate() read file: " << Common::ConfigPath_Display << " empty.";
            return false;
        }

        if (!DisplayConfig::GetInstance()->UpdateJson(jsonData))
        {
            LOG(ERROR) << "DisplayConfig UpdateJson Error, Config Path = " << Common::ConfigPath_Display
                       << ", jsonData : " << jsonData;
            return false;
        }
    }

    return true;
}

bool Application::Shutdown()
{
    grpcService = nullptr;                     // 清理服务接收器
    RegisterCenter::GetInstance()->ShutDown(); // 停止客户端管理
    RedisProtoData::ShutDownCache();           // 停止数据模块更新服务
    RedisProtoData::ShutDownConnectPool();     // 停止连接池任务
    return true;
}

void Application::notifyShutdown(int sigNum, siginfo_t *sigInfo, void *context)
{
    static std::once_flag oc; // 用于call_once的局部静态变量

    LOG(INFO) << "Application::notifyShutdown() sigNum = " << sigNum;
    std::call_once(oc, [&]()
                   { Application::GetInstance()->exit_requested.set_value(); });
}

void Application::notifyConfigUpdate(int sigNum, siginfo_t *sigInfo, void *context)
{
    LOG(INFO) << "Application::notifyConfigUpdate() sigNum = " << sigNum;
    std::thread(
        [&]()
        {
            Application::GetInstance()->ConfigUpdate();
        })
        .detach();
}
