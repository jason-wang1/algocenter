#include "SingleIndexRecall.h"
#include "glog/logging.h"

int SingleIndexRecall::RecallItem(
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
        LOG(ERROR) << "SingleIndexRecall::RecallItem() Get RecallType Error"
                   << ", apiType = " << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID;
        return Common::Error::Recall_GetParamsError;
    }

    // 获取召回数量
    int recallNum = recallParams.GetRecallNum();
    if (recallNum <= 0)
    {
        LOG(ERROR) << "SingleIndexRecall::RecallItem() Get RecallNum is zero"
                   << ", apiType = " << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID
                   << ", recallType = " << recallType;
        return Common::Error::Recall_GetParamsError;
    }

    // 获取样品倍数
    int sampleFold = recallParams.GetSampleFold();
    if (sampleFold < 0)
    {
        LOG(ERROR) << "SingleIndexRecall::RecallItem() Get SampleFold is zero"
                   << ", apiType = " << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID
                   << ", recallType = " << recallType;
        return Common::Error::Recall_GetParamsError;
    }

    // 获取权重精度
    int weightPrecision = recallParams.GetWeightPrecision();
    if (weightPrecision < 0)
    {
        LOG(ERROR) << "SingleIndexRecall::RecallItem() Get WeightPrecision is zero"
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
        LOG(ERROR) << "SingleIndexRecall::RecallItem() Get SamplesData Failed, err = " << err;
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

        LOG(INFO) << "SingleIndexRecall::RecallItem() Recall Info Log"
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

int SingleIndexRecall::GetSamplesData(
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

    int err = Common::Error::Failed;
    switch (recallTypeId)
    {
    case Common::RT_ID::RTI_ResType_Hot:
    {
        err = RedisProtoData::GetResTypeHotInvertIndex(requestData.context_res_type, protoData);
        if (Common::Error::OK != err)
        {
            LOG(ERROR) << "SingleIndexRecall::GetSamplesData() Get ResTypeHot ProtoData Failed. err = " << err;
            return err;
        }
        break;
    }
    case Common::RT_ID::RTI_ResType_Quality:
    {
        err = RedisProtoData::GetResTypeQualityInvertIndex(requestData.context_res_type, protoData);
        if (Common::Error::OK != err)
        {
            LOG(ERROR) << "SingleIndexRecall::GetSamplesData() Get ResTypeQuality ProtoData Failed. err = " << err;
            return err;
        }
        break;
    }
    case Common::RT_ID::RTI_ResType_Surge:
    {
        err = RedisProtoData::GetResTypeSurgeInvertIndex(requestData.context_res_type, protoData);
        if (Common::Error::OK != err)
        {
            LOG(ERROR) << "SingleIndexRecall::GetSamplesData() Get ResTypeSurge ProtoData Failed. err = " << err;
            return err;
        }
        break;
    }
    case Common::RT_ID::RTI_ResType_CTCVR:
    {
        err = RedisProtoData::GetResTypeCTCVRInvertIndex(requestData.context_res_type, protoData);
        if (Common::Error::OK != err)
        {
            LOG(ERROR) << "SingleIndexRecall::GetSamplesData() Get ResTypeCTCVR ProtoData Failed. err = " << err;
            return err;
        }
        break;
    }
    case Common::RT_ID::RTI_ResType_DLR:
    {
        err = RedisProtoData::GetResTypeDLRInvertIndex(requestData.context_res_type, protoData);
        if (Common::Error::OK != err)
        {
            LOG(ERROR) << "SingleIndexRecall::GetSamplesData() Get ResTypeDLR ProtoData Failed. err = " << err;
            return err;
        }
        break;
    }
    default:
    {
        LOG(ERROR) << "SingleIndexRecall::GetSamplesData() RecallType Not Support"
                   << ", apiType = " << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID
                   << ", recallTypeId = " << recallTypeId
                   << ", recallType = " << recallType;
        return Common::Recall_RecallTypeNotSupport;
    }
    }

    if (!protoData.empty())
    {
        RSP_ItemRecallData::ItemRecall item_recall;
        if (!item_recall.ParseFromString(protoData))
        {
            LOG(ERROR) << "SingleIndexRecall::GetSamplesData() ProtoData ParseFromString Failed.";
            return Common::Recall_ParseProtoDataFailed;
        }

        auto &recall_item_list = item_recall.item_list();
        int recall_item_size = recall_item_list.size();
        if (samplesNum == 0)
        {
            samplesNum = recall_item_size;
        }
        int use_samples_num = std::min(recall_item_size, samplesNum);
        samplesData.resize(use_samples_num);
        for (int idx = 0; idx < use_samples_num; idx++)
        {
            auto &item_info = samplesData[idx];
            item_info.id = recall_item_list[idx].id();
            item_info.res_type = recall_item_list[idx].res_type();
            item_info.weight = recall_item_list[idx].weight();
        }
    }

    return Common::Error::OK;
}
