#include "MultiIndexRecall.h"
#include "glog/logging.h"

int MultiIndexRecall::RecallItem(
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
        LOG(ERROR) << "MultiIndexRecall::RecallItem() Get RecallType Error"
                   << ", apiType = " << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID;
        return Common::Error::Recall_GetParamsError;
    }

    // 获取召回数量
    int recallNum = recallParams.GetRecallNum();
    if (recallNum <= 0)
    {
        LOG(ERROR) << "MultiIndexRecall::RecallItem() Get RecallNum is zero"
                   << ", apiType = " << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID
                   << ", recallType = " << recallType;
        return Common::Error::Recall_GetParamsError;
    }

    // 使用TopK索引
    int useTopKIndex = recallParams.GetUseTopKIndex();
    if (useTopKIndex <= 0)
    {
        LOG(ERROR) << "MultiIndexRecall::RecallItem() Get UseTopKIndex is zero"
                   << ", apiType = " << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID
                   << ", recallType = " << recallType;
        return Common::Error::Recall_GetParamsError;
    }

    // 获取样品倍数
    int sampleFold = recallParams.GetSampleFold();
    if (sampleFold < 0)
    {
        LOG(ERROR) << "MultiIndexRecall::RecallItem() Get SampleFold is zero"
                   << ", apiType = " << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID
                   << ", recallType = " << recallType;
        return Common::Error::Recall_GetParamsError;
    }

    // 单个最大数量
    int singleMaxNum = recallParams.GetSingleMaxNum();
    if (singleMaxNum < 0)
    {
        LOG(ERROR) << "MultiIndexRecall::RecallItem() Get SingleMaxNum is zero"
                   << ", apiType = " << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID
                   << ", recallType = " << recallType;
        return Common::Error::Recall_GetParamsError;
    }

    // 获取权重精度
    int weightPrecision = recallParams.GetWeightPrecision();
    if (weightPrecision < 0)
    {
        LOG(ERROR) << "MultiIndexRecall::RecallItem() Get WeightPrecision is zero"
                   << ", apiType = " << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID
                   << ", recallType = " << recallType;
        return Common::Error::Recall_GetParamsError;
    }

    // 获取是否按权重分配
    int isWeightAllocate = recallParams.GetIsWeightAllocate();
    if (isWeightAllocate < 0)
    {
        LOG(ERROR) << "MultiIndexRecall::RecallItem() Get IsWeightAllocate is zero"
                   << ", apiType = " << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID
                   << ", recallType = " << recallType;
        return Common::Error::Recall_GetParamsError;
    }

    bool useIsWeightAllocate = isWeightAllocate > 0 ? true : false;

    // 根据召回类型, 调用对应用户画像解码
    std::vector<RecallCalc::IDWeight> vec_id_weight;
    switch (recallTypeId)
    {
    default:
    {
        LOG(ERROR) << "MultiIndexRecall::RecallItem()  RecallType Not Support"
                   << ", apiType = " << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID
                   << ", uuid = " << requestData.uuid
                   << ", user_id = " << requestData.user_id
                   << ", recallType = " << recallType;
        return Common::Recall_RecallTypeNotSupport;
    }
    }

    // 分配抽样数量
    int actual_allocate_num = 0;
    std::vector<long> vecID;
    std::vector<int> vecSamplingNum;
    RecallCalc::AllocateSamplingNum(
        vec_id_weight, useTopKIndex, recallNum, singleMaxNum, useIsWeightAllocate,
        actual_allocate_num, vecID, vecSamplingNum);

    // 获取样本数据
    std::vector<std::vector<RecallCalc::SampleInfo>> vecSamplesData;
    int err = GetSamplesData(requestData, recallParams, vecID, vecSamplingNum, vecSamplesData);
    if (err != Common::Error::OK)
    {
        LOG(ERROR) << "MultiIndexRecall::RecallItem() Get SamplesData Failed, err = " << err;
        return err;
    }

    // 抽样
    std::vector<std::string> samplingResultData;
    RecallCalc::MultiIndexSampling(
        vecSamplesData, vecSamplingNum, sampleFold, weightPrecision, samplingResultData);

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

        LOG(INFO) << "MultiIndexRecall::RecallItem() Recall Info Log"
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

int MultiIndexRecall::GetSamplesData(
    const RequestData &requestData,
    const RecallParam &recallParams,
    const std::vector<long> &vecID,
    const std::vector<int> &vecSamplingNum,
    std::vector<std::vector<RecallCalc::SampleInfo>> &vecSamplesData)
{
    std::string recallType = recallParams.GetRecallType();
    long recallTypeId = Common::GetRecallTypeID(recallType);
    int sampleFold = recallParams.GetSampleFold();

    int vec_id_size = vecID.size();
    int vec_sampling_num_size = vecSamplingNum.size();
    if (vec_id_size <= 0)
    {
        return Common::Error::OK;
    }

    if (vec_sampling_num_size != vec_id_size)
    {
        LOG(ERROR) << "MultiIndexRecall::GetSamplesData() ID or SamplingNum size error!!!";
        return Common::Error::Recall_RecallCalFailed;
    }

    vecSamplesData.resize(vec_id_size);

    std::vector<int> vec_use_sampling_num;
    vec_use_sampling_num.reserve(vec_id_size);

    for (int idx = 0; idx < vec_id_size; idx++)
    {
        int use_sampling_num = vecSamplingNum[idx] * sampleFold;
        vec_use_sampling_num.push_back(use_sampling_num);

        auto &samples_data = vecSamplesData[idx];
        samples_data.reserve(use_sampling_num);
    }

    std::vector<std::string> vecProtoData;
    vecProtoData.resize(vec_id_size);
    for (std::string &protoData : vecProtoData)
    {
        protoData.clear();
    }

    // int err = Common::Error::Failed;
    switch (recallTypeId)
    {
    default:
    {
        LOG(ERROR) << "MultiIndexRecall::GetSamplesData() RecallType Not Support"
                   << ", apiType = " << requestData.apiType
                   << ", recallExpID = " << requestData.recallExpID
                   << ", recallTypeId = " << recallTypeId
                   << ", recallType = " << recallType;
        return Common::Recall_RecallTypeNotSupport;
    }
    }

    for (int idx = 0; idx < vec_id_size; idx++)
    {
        int use_sampling_num = vec_use_sampling_num[idx];
        auto &samples_data = vecSamplesData[idx];

        std::string &protoData = vecProtoData[idx];
        if (!protoData.empty())
        {
            RSP_ItemRecallData::ItemRecall item_recall;
            if (!item_recall.ParseFromString(protoData))
            {
                LOG(ERROR) << "MultiIndexRecall::GetSamplesData() ProtoData ParseFromString Failed.";
                return Common::Recall_ParseProtoDataFailed;
            }

            auto &recall_item_list = item_recall.item_list();
            int recall_item_size = recall_item_list.size();

            int use_samples_num = std::min(recall_item_size, use_sampling_num);
            samples_data.resize(use_samples_num);
            for (int idx = 0; idx < use_samples_num; idx++)
            {
                auto &item_info = samples_data[idx];
                item_info.id = recall_item_list[idx].id();
                item_info.res_type = recall_item_list[idx].res_type();
                item_info.weight = recall_item_list[idx].weight();
            }
        }
    }

    return Common::Error::OK;
}
