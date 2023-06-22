#pragma once
#include <memory>
#include <string>
#include <future> // std::async, std::future
#include "Common/Singleton.h"
#include "grpcpp/grpcpp.h"
#include "signal/CSignal.h"

#include "Protobuf/proxy/Common.grpc.pb.h"

class Config;
class AlgoCenter;

class Application final
    : public Singleton<Application>
{
public:
    Application(token) {}
    virtual ~Application() {}
    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

public:
    bool Init(
        const std::string &IP,
        const std::string &Port,
        const std::string &Exe,
        const std::string &Nicename);

    // 启动程序
    bool StartProcess(
        const std::string &Port);

    // 等待程序退出
    bool Waiting();

    // 清理资源
    bool Shutdown();

    static void notifyShutdown(int sigNum, siginfo_t *sigInfo, void *context);
    static void notifyConfigUpdate(int sigNum, siginfo_t *sigInfo, void *context);

    bool ConfigUpdate();

public:
    std::shared_ptr<Config> conf = nullptr;
    std::shared_ptr<AlgoCenter> algoCenter = nullptr;

    std::unique_ptr<grpc::Server> grpcServer = nullptr;
    std::shared_ptr<grpc::Service> grpcService = nullptr;
    std::promise<void> exit_requested; // 退出信号
    std::mutex config_updae_lock;      // 配置更新锁
};
