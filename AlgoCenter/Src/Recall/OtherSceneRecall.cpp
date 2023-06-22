#include "OtherSceneRecall.h"
#include "glog/logging.h"

#include "AnnoyRecall/AnnoyIndexCache.h"

int OtherSceneRecall::RecallItem(
    const RequestData &requestData,
    const RecallParam &recallParams,
    std::vector<ItemInfo> &resultRecallData)
{
    double start_time = Common::get_ms_time();

    // 获取召回类型
    std::string recallType = recallParams.GetRecallType();
    long recallTypeId = Common::GetRecallTypeID(recallType);
    if (recallTypeId == Common::RT_ID::RTI_None)
    {
        LOG(ERROR) << "OtherSceneRecall::RecallItem() Get RecallType Error"
                   << ", apiType = " << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID;
        return Common::Error::Recall_GetParamsError;
    }

    // 获取召回数量
    int recallNum = recallParams.GetRecallNum();
    if (recallNum <= 0)
    {
        LOG(ERROR) << "OtherSceneRecall::RecallItem() Get RecallNum is zero"
                   << ", apiType = " << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID
                   << ", recallType = " << recallType;
        return Common::Error::Recall_GetParamsError;
    }

    // 获取样品倍数
    int sampleFold = recallParams.GetSampleFold();
    if (sampleFold < 0)
    {
        LOG(ERROR) << "OtherSceneRecall::RecallItem() Get SampleFold is zero"
                   << ", apiType = " << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID
                   << ", recallType = " << recallType;
        return Common::Error::Recall_GetParamsError;
    }

    // 获取权重精度
    int weightPrecision = recallParams.GetWeightPrecision();
    if (weightPrecision < 0)
    {
        LOG(ERROR) << "OtherSceneRecall::RecallItem() Get WeightPrecision is zero"
                   << ", apiType = " << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID
                   << ", recallType = " << recallType;
        return Common::Error::Recall_GetParamsError;
    }

    // 获取样本数据
    std::vector<RecallCalc::SampleInfo> samplesData;
    int err = GetSamplesData(requestData, recallParams, samplesData);
    if (err != Common::Error::OK)
    {
        LOG(ERROR) << "OtherSceneRecall::RecallItem() Get SamplesData Failed, err = " << err;
        return err;
    }

    // 抽样
    std::vector<std::string> samplingResultData;
    RecallCalc::SingleIndexSampling(
        samplesData, sampleFold, recallNum, weightPrecision, samplingResultData);

    // 召回结果
    int result_idx = 0;
    std::stringstream recallInfo;
    resultRecallData.resize(samplingResultData.size());
    for (auto &item_key : samplingResultData)
    {
        auto &item_info = resultRecallData[result_idx++];
        item_info.item_id = atol(item_key.substr(0, item_key.find_first_of("_")).c_str());
        item_info.res_type = atoi(item_key.substr(item_key.find_first_of("_") + 1).c_str());
        item_info.recall_type = recallType;
        item_info.recall_type_id = recallTypeId;

        if (!recallInfo.str().empty())
        {
            recallInfo << ", ";
        }
        recallInfo << item_key;
    }

    unsigned random_seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(resultRecallData.begin(), resultRecallData.end(),
                 std::default_random_engine(random_seed));

    double end_time = Common::get_ms_time();
    if (requestData.is_statis_log)
    {

        LOG(INFO) << "OtherSceneRecall::RecallItem() Recall Info Log"
                  << ", recallType = " << recallType
                  << ", apiType" << requestData.apiType
                  << ", recallExpID = " << requestData.recallExpID
                  << ", filterExpID = " << requestData.filterExpID
                  << ", displayExpID = " << requestData.displayExpID
                  << ", recallUserGroup = " << requestData.recallUserGroup
                  << ", filterUserGroup = " << requestData.filterUserGroup
                  << ", displayUserGroup = " << requestData.displayUserGroup
                  << ", uuid = " << requestData.uuid
                  << ", user_id = " << requestData.user_id
                  << ", All Time = " << end_time - start_time << "ms"
                  << ", Recall Info = [" << recallInfo.str()
                  << "], Item Num = " << resultRecallData.size();
    }

    return Common::Error::OK;
}

int OtherSceneRecall::GetSamplesData(
    const RequestData &requestData,
    const RecallParam &recallParams,
    std::vector<RecallCalc::SampleInfo> &samplesData)
{
    std::string recallType = recallParams.GetRecallType();
    long recallTypeId = Common::GetRecallTypeID(recallType);
    int recallNum = recallParams.GetRecallNum();
    int sampleFold = recallParams.GetSampleFold();

    int samplesNum = recallNum * sampleFold;
    samplesData.reserve(samplesNum);

    std::string protoData;
    protoData.clear();

    switch (recallTypeId)
    {
    case Common::RT_ID::RTI_ResType_User_CF:
    {
        auto ufi = requestData.GetUserFeature().GetUserFeatureIndex();
        if (ufi != nullptr)
        {
            const auto &ufi_cf_item = ufi->cf_item();
            auto iter = ufi_cf_item.find(requestData.context_res_type);
            if (iter != ufi_cf_item.end())
            {
                const auto &id_weight_list = iter->second.id_list();
                int id_weight_size = id_weight_list.size();
                if (samplesNum == 0)
                {
                    samplesNum = id_weight_size;
                }
                int use_samples_num = std::min(id_weight_size, samplesNum);
                samplesData.resize(use_samples_num);
                for (int idx = 0; idx < use_samples_num; idx++)
                {
                    auto &item_info = samplesData[idx];
                    item_info.id = id_weight_list[idx].id();
                    item_info.res_type = requestData.context_res_type;
                    item_info.weight = id_weight_list[idx].weight();
                }
            }
        }
        break;
    }
    case Common::RT_ID::RTI_Item_CF:
    {
        auto &item_feature = requestData.itemFeature;
        auto item_index = item_feature.GetItemFeatureIndex(requestData.context_item_id, requestData.context_res_type);
        if (item_index != nullptr)
        {
            const auto &id_weight_list = item_index->cf_item();
            int id_weight_size = id_weight_list.size();
            if (samplesNum == 0)
            {
                samplesNum = id_weight_size;
            }
            int use_samples_num = std::min(id_weight_size, samplesNum);
            samplesData.resize(use_samples_num);
            for (int idx = 0; idx < use_samples_num; idx++)
            {
                auto &item_info = samplesData[idx];
                item_info.id = id_weight_list[idx].id();
                item_info.res_type = requestData.context_res_type;
                item_info.weight = id_weight_list[idx].weight();
            }
        }
        break;
    }
    case Common::RT_ID::RTI_Item_Annoy:
    {
        if (samplesNum == 0)
        {
            samplesNum = recallNum;
        }

        std::vector<std::string> vec_near_items;
        std::vector<float> vec_distances;
        int err = AnnoyIndexCache::GetInstance()->AnnoyRetrieval(
            "res_type_" + std::to_string(requestData.context_res_type),
            std::to_string(requestData.context_item_id) + "_" + std::to_string(requestData.context_res_type),
            requestData.vecKeynames, samplesNum, vec_near_items, vec_distances);
        size_t near_items_size = vec_near_items.size();
        if (Common::Error::OK == err && vec_distances.size() == near_items_size)
        {
            samplesData.resize(near_items_size);
            for (size_t idx = 0; idx < near_items_size; idx++)
            {
                std::string &item_key = vec_near_items[idx];

                auto &item_info = samplesData[idx];
                item_info.id = atol(item_key.substr(0, item_key.find_first_of("_")).c_str());
                item_info.res_type = atoi(item_key.substr(item_key.find_first_of("_") + 1).c_str());
                item_info.weight = vec_distances[idx];
            }
        }
        break;
    }
    case Common::RT_ID::RTI_ClickOccur:
    {
        auto &item_feature = requestData.itemFeature;
        auto item_index = item_feature.GetItemFeatureIndex(requestData.context_item_id, requestData.context_res_type);
        if (item_index != nullptr)
        {
            const auto &id_weight_list = item_index->click_occur();
            int id_weight_size = id_weight_list.size();
            if (samplesNum == 0)
            {
                samplesNum = id_weight_size;
            }
            int use_samples_num = std::min(id_weight_size, samplesNum);
            samplesData.resize(use_samples_num);
            for (int idx = 0; idx < use_samples_num; idx++)
            {
                auto &item_info = samplesData[idx];
                item_info.id = id_weight_list[idx].id();
                item_info.res_type = requestData.context_res_type;
                item_info.weight = id_weight_list[idx].weight();
            }
        }
        break;
    }
    case Common::RT_ID::RTI_DownloadOccur:
    {
        auto &item_feature = requestData.itemFeature;
        auto item_index = item_feature.GetItemFeatureIndex(requestData.context_item_id, requestData.context_res_type);
        if (item_index != nullptr)
        {
            const auto &id_weight_list = item_index->download_occur();
            int id_weight_size = id_weight_list.size();
            if (samplesNum == 0)
            {
                samplesNum = id_weight_size;
            }
            int use_samples_num = std::min(id_weight_size, samplesNum);
            samplesData.resize(use_samples_num);
            for (int idx = 0; idx < use_samples_num; idx++)
            {
                auto &item_info = samplesData[idx];
                item_info.id = id_weight_list[idx].id();
                item_info.res_type = requestData.context_res_type;
                item_info.weight = id_weight_list[idx].weight();
            }
        }
        break;
    }
    default:
    {
        LOG(ERROR) << "OtherSceneRecall::GetSamplesData() RecallType Not Support"
                   << ", apiType = " << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID
                   << ", recallTypeId = " << recallTypeId
                   << ", recallType = " << recallType;
        return Common::Recall_RecallTypeNotSupport;
    }
    }

    return Common::Error::OK;
}
