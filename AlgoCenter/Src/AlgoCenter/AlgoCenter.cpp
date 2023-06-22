#include "AlgoCenter.h"

#include <future>
#include <thread>
#include <unordered_set>

#include <algorithm>
#include <random>
#include <chrono>

#include "glog/logging.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "Common/Function.h"
#include "GrpcDispatcher/GrpcDispatcher.h"
#include "RegisterCenter/RegisterCenter.h"
#include "Protobuf/proxy/Common.pb.h"
#include "MurmurHash3/MurmurHash3.h"

#include "Define.h"
#include "Config.h"
#include "RequestData.h"
#include "ResponseData.h"
#include "Application.h"

#include "Recall/Recall.h"
#include "Recall/RecallConfig.h"

#include "Filter/Filter.h"
#include "Filter/FilterConfig.h"

#include "Display/Display.h"
#include "Display/DisplayConfig.h"

#include "ItemRank.h"

#include "RedisProtoData/include/RedisProtoData.h"

#include "Protobuf/proxy/AlgoCenter.pb.h"

#include "StatisLogData.h"

bool AlgoCenter::Init()
{
    // 绑定grpc
    GrpcDispatcher::GetInstance()->Bind(this, GrpcProtos::CMD_GET_DOWNLOAD_RECOMMEND, OnDownloadRecommen);

    return true;
}

bool AlgoCenter::OnDownloadRecommen(
    std::any obj,
    const int cmd,
    const long deadline_ms,
    const std::string &request,
    int &result,
    std::string &response)
{
    RequestData requestData;
    ResponseData responseData;

    COMM_AlgoCenter::AlgoCenterRequest requestProto;

    result = GrpcProtos::ResultType::OK;
    if (!requestProto.ParseFromString(request))
    {
        result = GrpcProtos::ResultType::ERR_Decode_Request;
        response = "Decode Request Failed.";
        return false;
    }

    requestData.uuid = requestProto.request_id();
    requestData.user_id = requestProto.user_id();
    requestData.ret_count = requestProto.ret_count();

    for (int idx = 0; idx < requestProto.exp_list_size(); ++idx)
    {
        requestData.vecExpID.push_back(requestProto.exp_list(idx));
    }

    requestData.context_item_id = requestProto.ll_id();
    requestData.context_res_type = requestProto.res_type();
    for (int idx = 0; idx < requestProto.keyname_list_size(); ++idx)
    {
        requestData.vecKeynames.push_back(requestProto.keyname_list(idx));
    }

    // 确定请求类型
    switch (cmd)
    {
    case GrpcProtos::CMD_GET_DOWNLOAD_RECOMMEND:
        requestData.apiType = Common::APIType::Get_Download_Recommend;
        break;
    default:
        result = GrpcProtos::ResultType::ERR_Service_CMD;
        response = "CMD Type Error.";
        return false;
    }

    int random_mod = rand() % 100;
    if (random_mod >= 0 && random_mod < 5)
    {
        requestData.is_statis_log = true;
    }

#ifdef DEBUG
    requestData.is_statis_log = true;
#endif

    if (GetRecommendItem(requestData, responseData) == false)
    {
        result = GrpcProtos::ResultType::ERR_Service_Cal;
        response = "Get Recommend Item Error.";
        return false;
    }

    int items_list_size = responseData.items_list.size();
    int use_ret_count = std::min(requestData.ret_count, items_list_size);

    COMM_AlgoCenter::AlgoCenterResponse protoReply;
    protoReply.set_user_id(requestData.user_id);
    for (int idx = 0; idx < use_ret_count; idx++)
    {
        auto item_info = responseData.items_list[idx];
        COMM_AlgoCenter::ItemData *item_data_ptr = protoReply.add_item_list();
        item_data_ptr->set_ll_id(item_info.item_id);
        item_data_ptr->set_res_type(item_info.res_type);
        item_data_ptr->set_source(item_info.recall_type_id);
    }

    response.clear();
    if (!protoReply.SerializeToString(&response))
    {
        result = GrpcProtos::ResultType::ERR_Encode_Response;
        response = "Encode Response Failed.";
        LOG(ERROR) << "OnRecommenItemGet: Reply proto serialize to string Failed!";
        return false;
    }

    return true;
}

bool AlgoCenter::GetRecommendItem(
    RequestData &requestData,
    ResponseData &responseData)
{
    double start_time = Common::get_ms_time();

    LOG(INFO) << "GetRecommendItem() Request Start "
              << ", apiType = " << requestData.apiType
              << ", uuid = " << requestData.uuid
              << ", exp_id = " << requestData.LogOutExpIDList(requestData.vecExpID)
              << ", user_id = " << requestData.user_id
              << ", context_item_id = " << requestData.context_item_id
              << ", context_res_type = " << requestData.context_res_type
              << ", keyname = " << requestData.LogOutKeynames(requestData.vecKeynames)
              << ", ret_count = " << requestData.ret_count;

    // 1. 上下文初始化
    // 1.1 初始化用户特征
    int32_t initContextUserErr = requestData.InitUserFeature();
    if (Common::Error::OK != initContextUserErr)
    {
        LOG(ERROR) << "GetRecommendItem() Context InitUserFeature Failed"
                   << ", apiType" << requestData.apiType
                   << ", uuid = " << requestData.uuid
                   << ", user_id = " << requestData.user_id
                   << ", err = " << initContextUserErr;
        return false;
    }

    // 1.2 初始化物料特征
    switch (requestData.apiType)
    {
    case Common::APIType::Get_Download_Recommend:
    {
        std::string item_key = std::to_string(requestData.context_item_id) + "_" + std::to_string(requestData.context_res_type);
        int initContextItemErr = requestData.itemFeature.InitItemFeature({item_key});
        if (Common::Error::OK != initContextItemErr)
        {
            LOG(ERROR) << "GetRecommendItem() Context InitItemFeature Failed"
                       << ", apiType" << requestData.apiType
                       << ", uuid = " << requestData.uuid
                       << ", user_id = " << requestData.user_id
                       << ", context_item_id = " << requestData.context_item_id
                       << ", context_res_type = " << requestData.context_res_type
                       << ", err = " << initContextItemErr;
            return false;
        }
        break;
    }
    default:
    {
        break;
    }
    }

    double context_init_end_time = Common::get_ms_time();

    // 2. 获取统一匹配实验id和用户组实验配置
    requestData.recallExpID = RecallConfig::GetInstance()->MatchRecallExpID(requestData.apiType, requestData.vecExpID);
    requestData.filterExpID = FilterConfig::GetInstance()->MatchFilterExpID(requestData.apiType, requestData.vecExpID);
    requestData.displayExpID = DisplayConfig::GetInstance()->MatchDisplayExpID(requestData.apiType, requestData.vecExpID);

    const RecallParamData *pRecallParamData =
        Recall::GetInstance()->GetRecallParamData(requestData, responseData);
    if (pRecallParamData == nullptr)
    {
        LOG(ERROR) << "GetRecommendItem() Get Recall Config is nullptr"
                   << ", userGroupList" << requestData.LogOutUserGroup(requestData.userGroupList)
                   << ", apiType" << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID;
        return false;
    }
    requestData.recallUserGroup = pRecallParamData->user_group;

    const FilterParamData *pFilterParamData =
        Filter::GetInstance()->GetFilterParamData(requestData, responseData);
    if (pFilterParamData == nullptr)
    {
        LOG(ERROR) << "GetRecommendItem() Get Filter Config is nullptr"
                   << ", userGroupList" << requestData.LogOutUserGroup(requestData.userGroupList)
                   << ", apiType" << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID;
        return false;
    }
    requestData.filterUserGroup = pFilterParamData->user_group;

    const DisplayParamData *pDisplayParamData =
        Display::GetInstance()->GetDisplayParamData(requestData, responseData);
    if (pDisplayParamData == nullptr)
    {
        LOG(ERROR) << "GetRecommendItem() Get Display Config is nullptr"
                   << ", userGroupList" << requestData.LogOutUserGroup(requestData.userGroupList)
                   << ", apiType" << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID;
        return false;
    }
    requestData.displayUserGroup = pDisplayParamData->user_group;

    // 3. 召回
    int recallErr = Recall::GetInstance()->CallRecall(pRecallParamData, requestData, responseData);
    if (recallErr != Common::Error::OK)
    {
        LOG(ERROR) << "GetRecommendItem Call Recall Failed"
                   << ", apiType" << requestData.apiType
                   << ", Recall exp_id = " << requestData.recallExpID
                   << ", err = " << recallErr;
        return false;
    }

    double call_recall_end_time = Common::get_ms_time();

    // 4. 初始化Item特征，并过滤
    std::unordered_set<std::string> total_keys_set;
    for (const auto &item_info : responseData.items_list)
    {
        total_keys_set.insert(std::to_string(item_info.item_id) + "_" + std::to_string(item_info.res_type));
    }

    for (const auto &item_info : responseData.spare_items_list)
    {
        total_keys_set.insert(std::to_string(item_info.item_id) + "_" + std::to_string(item_info.res_type));
    }

    for (auto iter = responseData.exclusive_items_list.begin();
         iter != responseData.exclusive_items_list.end(); iter++)
    {
        for (const auto &item_info : iter->second)
        {
            total_keys_set.insert(std::to_string(item_info.item_id) + "_" + std::to_string(item_info.res_type));
        }
    }

    std::vector<std::string> vec_item_keys;
    vec_item_keys.reserve(total_keys_set.size());
    vec_item_keys.assign(total_keys_set.begin(), total_keys_set.end());
    int initItemErr = requestData.itemFeature.InitItemFeature(vec_item_keys);
    if (Common::Error::OK != initItemErr)
    {
        LOG(ERROR) << "GetRecommendItem() InitItemFeature Failed"
                   << ", apiType" << requestData.apiType
                   << ", uuid = " << requestData.uuid
                   << ", user_id = " << requestData.user_id
                   << ", err = " << initItemErr;
        return false;
    }

    double feature_init_end_time = Common::get_ms_time();

    int filterErr = Filter::GetInstance()->CallFilter(pFilterParamData, requestData, responseData);
    if (filterErr != Common::Error::OK)
    {
        LOG(ERROR) << "GetRecommendItem Call Filter Failed"
                   << ", apiType" << requestData.apiType
                   << ", filter exp_id = " << requestData.filterExpID
                   << ", err = " << filterErr;
        return false;
    }

    unsigned random_seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(responseData.items_list.begin(), responseData.items_list.end(),
                 std::default_random_engine(random_seed));

    double filter_end_time = Common::get_ms_time();

    // 5. 调用排序
    int rank_calc_num = responseData.items_list.size();
    ItemRank::RankCalc(requestData, responseData);

    StatisLogData::OutStatisLog(requestData, responseData, "After Rank", {50, 20, 8, 3}, false, true);

    double rank_call_end_time = Common::get_ms_time();

    // 6. 展控模块
    int scError = Display::GetInstance()->CallDisplay(pDisplayParamData, requestData, responseData);
    if (scError != Common::Error::OK)
    {
        LOG(ERROR) << "GetRecommendItem Call Show Control Failed"
                   << ", apiType" << requestData.apiType
                   << ", Show Control exp_id = " << requestData.displayExpID
                   << ", err = " << scError;
        return false;
    }

    StatisLogData::OutStatisLog(requestData, responseData, "After Display", {20, 8, 3}, false, false);

    double end_time = Common::get_ms_time();

    LOG(INFO) << "GetRecommendItem() Time Info Log: uuid = " << requestData.uuid
              << ", exp_id = " << requestData.LogOutExpIDList(requestData.vecExpID)
              << ", user_id = " << requestData.user_id
              << ", apiType" << requestData.apiType
              << ", context_item_id = " << requestData.context_item_id
              << ", context_res_type = " << requestData.context_res_type
              << ", keynames = " << requestData.LogOutKeynames(requestData.vecKeynames)
              << ", ret_count = " << requestData.ret_count
              << ", recallExpID = " << requestData.recallExpID
              << ", filterExpID = " << requestData.filterExpID
              << ", displayExpID = " << requestData.displayExpID
              << ", recallUserGroup = " << requestData.recallUserGroup
              << ", filterUserGroup = " << requestData.filterUserGroup
              << ", displayUserGroup = " << requestData.displayUserGroup
              << ", Context Init Time = " << context_init_end_time - start_time << "ms"
              << ", Call Recall Time = " << call_recall_end_time - context_init_end_time << "ms"
              << ", Feature Init Time = " << feature_init_end_time - call_recall_end_time << "ms"
              << ", Call Filter Time = " << filter_end_time - feature_init_end_time << "ms"
              << ", Rank Calc Num = " << rank_calc_num
              << ", Rank Calc Time = " << rank_call_end_time - filter_end_time << "ms"
              << ", Call Display Time = " << end_time - rank_call_end_time << "ms"
              << ", All Request Time = " << end_time - start_time << "ms.";

    // 输出特征
    DebugLogFeature(requestData, responseData);

    return true;
}

// 输出日志
void AlgoCenter::DebugLogFeature(
    const RequestData &requestData,
    const ResponseData &responseData)
{
    // 根据Log判断是否打印日志
    auto conf = Config::GetInstance();
    if (!conf->IsDebugLogSwitch())
    {
        return;
    }

    // 构造 Json
    rapidjson::StringBuffer logBuf;
    rapidjson::Writer<rapidjson::StringBuffer> logWriter(logBuf);
    logWriter.StartObject();
    logWriter.Key("uuid");
    logWriter.String(requestData.uuid.c_str(), requestData.uuid.size());
    logWriter.Key("exp_list");
    logWriter.StartArray();
    for (auto item : requestData.vecExpID)
    {
        logWriter.Int(item);
    }
    logWriter.EndArray();
    logWriter.Key("userGroupList");
    logWriter.StartArray();
    for (auto item : requestData.userGroupList)
    {
        logWriter.String(item.c_str(), item.size());
    }
    logWriter.EndArray();
    logWriter.Key("keynames");
    logWriter.StartArray();
    for (auto item : requestData.vecKeynames)
    {
        logWriter.String(item.c_str(), item.size());
    }
    logWriter.EndArray();
    logWriter.Key("user_id");
    logWriter.Int64(requestData.user_id);
    logWriter.Key("apiType");
    logWriter.Int(static_cast<int>(requestData.apiType));
    logWriter.Key("recallExpID");
    logWriter.Int(requestData.recallExpID);
    logWriter.Key("filterExpID");
    logWriter.Int(requestData.filterExpID);
    logWriter.Key("displayExpID");
    logWriter.Int(requestData.displayExpID);
    logWriter.Key("recallUserGroup");
    logWriter.String(requestData.recallUserGroup.c_str(), requestData.recallUserGroup.size());
    logWriter.Key("filterUserGroup");
    logWriter.String(requestData.filterUserGroup.c_str(), requestData.filterUserGroup.size());
    logWriter.Key("displayUserGroup");
    logWriter.String(requestData.displayUserGroup.c_str(), requestData.displayUserGroup.size());

    // 特征
    logWriter.Key("item_list");
    logWriter.StartArray();
    std::size_t logCount = std::min(responseData.items_list.size(), std::size_t(conf->GetDebugLogCount()));
    for (std::size_t logIdx = 0; logIdx < logCount; logIdx++)
    {
        const auto &item_info = responseData.items_list[logIdx];

        // 添加一条记录
        logWriter.StartObject();

        // 基本信息
        logWriter.Key("item_id");
        logWriter.Int64(item_info.item_id);
        logWriter.Key("recall_type");
        logWriter.String(item_info.recall_type.c_str(), item_info.recall_type.size());
        logWriter.Key("score");
        logWriter.Double(item_info.score);
        logWriter.Key("sc_score");
        logWriter.Double(item_info.sc_score);

        logWriter.EndObject();
    }
    logWriter.EndArray();

    logWriter.EndObject();

    // 打印Log
    std::string featureJsonLog;
    featureJsonLog.clear();
    featureJsonLog.append(logBuf.GetString(), logBuf.GetSize());
    LOG(INFO) << "DebugLogFeature() Item Info Log: " << featureJsonLog;
}
