#include "ItemRank.h"
#include <future>
#include <cmath>
#include <algorithm>
#include "glog/logging.h"
#include "Common/Function.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "Config.h"
#include "Define.h"
#include "RequestData.h"
#include "ResponseData.h"

#include "TDPredict/RankCalc/RankCalc.h"
#include "Feature/UserFeature.h"
#include "Feature/ItemFeature.h"
#include "Feature/TFFeature.h"

#include "KafkaClient/PushKafkaPool.h"

static std::string GenerateAlgoData(
    const RequestData &requestData,
    const std::vector<TDPredict::RankItem> &vec_rank_item)
{
    std::string data_type = "";
    switch (requestData.apiType)
    {
    case Common::APIType::Get_Download_Recommend:
    {
        data_type = "rec_download_user_feature";
        break;
    }
    default:
    {
        break;
    }
    }

    long now_time = Common::get_timestamp();

    // 构造 Json
    rapidjson::StringBuffer logBuf;
    rapidjson::Writer<rapidjson::StringBuffer> logWriter(logBuf);
    logWriter.StartObject();
    {
        logWriter.Key("type");
        logWriter.String(data_type.c_str(), data_type.size());

        logWriter.Key("data");
        logWriter.StartObject();
        {
            logWriter.Key("add_time");
            logWriter.Int64(now_time);
            logWriter.Key("request_id");
            logWriter.String(requestData.uuid.c_str(), requestData.uuid.size());
            logWriter.Key("user_id");
            logWriter.Int64(requestData.user_id);
            logWriter.Key("context_item_id");
            logWriter.Int64(requestData.context_item_id);
            logWriter.Key("context_res_type");
            logWriter.Int(requestData.context_res_type);

            // 公共特征
            logWriter.Key("common_feature");
            logWriter.StartArray();
            {
                if (!vec_rank_item.empty())
                {
                    auto &rank_item = vec_rank_item[0];
                    const auto &common_feature = rank_item.spFeatureData->common_feature;
                    for (const auto &field_item_list : common_feature)
                    {
                        for (const auto &field_item : field_item_list)
                        {
                            logWriter.StartObject();
                            {
                                std::string field_value = std::to_string(field_item.field_value.field_value());
                                logWriter.Key(field_value.c_str(), field_value.size());
                                logWriter.Double(field_item.weight);
                            }
                            logWriter.EndObject();
                        }
                    }
                }
            }
            logWriter.EndArray();
        }
        logWriter.EndObject();
    }
    logWriter.EndObject();

    // 打印Log
    std::string featureJsonLog;
    featureJsonLog.clear();
    featureJsonLog.append(logBuf.GetString(), logBuf.GetSize());

    return featureJsonLog;
}

int ItemRank::RankCalc(
    const RequestData &requestData,
    ResponseData &responseData)
{
    double start_time = Common::get_ms_time();

    // 百分之一的概率打印5条排序日志
    int rank_log_count = 0;
    if ((rand() % 10) == 1)
    {
        rank_log_count = 5;
    }

#ifdef DEBUG
    rank_log_count = 5;
#endif

    // 设置 user_id 与 item_id
    const int items_list_size = responseData.items_list.size();
    std::vector<TDPredict::RankItem> vec_rank_item(items_list_size);
    for (int idx = 0; idx != items_list_size; idx++)
    {
        auto &rank_item = vec_rank_item[idx];
        rank_item.setParam("user_id", requestData.user_id);
        rank_item.setParam("context_item_id", requestData.context_item_id);
        rank_item.setParam("context_res_type", (long)requestData.context_res_type);
        rank_item.setParam("item_id", responseData.items_list[idx].item_id);
        rank_item.setParam("res_type", (long)responseData.items_list[idx].res_type);
        rank_item.setParam("recall_type_id", responseData.items_list[idx].recall_type_id);
        rank_item.setParam("recall_type", responseData.items_list[idx].recall_type);
    }

    auto spInitStruct = std::make_shared<InitStruct>();
    spInitStruct->spUserFeature = std::make_shared<UserFeature>(requestData.userFeature);
    spInitStruct->spItemFeature = std::make_shared<ItemFeature>(requestData.itemFeature);

    std::unordered_map<TDPredict::RankType, int> rand_exp_id;

    // 执行多级排序计算
    TDPredict::RankCalc calc;
    calc.SetUUID(requestData.uuid);
    calc.SetRankLogCount(rank_log_count);
    calc.SetRankLogFeature(true);
    calc.SetFeatureFactory(TDPredict::GetFeatureFactory());
    calc.SetInitData(spInitStruct);
    calc.Calc(requestData.vecExpID, vec_rank_item, rand_exp_id);

    // 输出 Kafka & 日志文件
    if (Config::GetInstance()->GetKafkaPushSwitch())
    {
        TDKafkaData kafka_data;
        kafka_data.msg = GenerateAlgoData(requestData, vec_rank_item);
        kafka_data.topic_id = Config::GetInstance()->GetKafkaPushTopic();
        kafka_data.partition = RdKafka::Topic::PARTITION_UA;

        LOG(DATA) << kafka_data.msg;

        int free_threads_num = 0;
        int queue_tasks_num = 0;
        PushKafkaPool::GetInstance()->Push(std::move(kafka_data), free_threads_num, queue_tasks_num);

        if ((rand() % 1000) < 5)
        {
            LOG(INFO) << "ItemRank::RankCalc() PushKafkaPool Status"
                      << ", free_threads_num = " << free_threads_num
                      << ", queue_tasks_num = " << queue_tasks_num;
        }
    }

    // 截断返回数量
    const int trunc_count = std::min(requestData.rankTruncCount, int(vec_rank_item.size()));
    vec_rank_item.resize(trunc_count);

    // 写回 items_list
    responseData.items_list.clear();
    responseData.items_list.reserve(trunc_count);
    for (int idx = 0; idx < trunc_count; idx++)
    {
        const auto &rank_item = vec_rank_item[idx];

        long item_id = 0;
        long res_type = 0;
        long recall_type_id = 0;
        std::string recall_type = "";
        if (rank_item.getParam("item_id", item_id) &&
            rank_item.getParam("res_type", res_type) &&
            rank_item.getParam("recall_type_id", recall_type_id) &&
            rank_item.getParam("recall_type", recall_type))
        {
            auto score = rank_item.getRankScore(TDPredict::RankType::Rank);
            responseData.items_list.emplace_back(
                item_id, res_type, recall_type_id, recall_type, score, score);
        }
    }

    double end_time = Common::get_ms_time();
    LOG(INFO) << "ItemRank::Calc() TimeInfoLog"
              << ", uuid = " << requestData.uuid
              << ", user_id = " << requestData.user_id
              << ", items_list = " << responseData.items_list.size()
              << ", all_time = " << end_time - start_time << "ms.";

    return Common::Error::OK;
}
