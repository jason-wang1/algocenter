#pragma once
#include <string>
#include <cmath>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include <stdlib.h>

#include "glog/logging.h"
#include "Common/Function.h"

class RecallCalc
{
public:
    // 带权重ID: <ID, weight>
    struct IDWeight
    {
        long id = 0;
        float weight = 0.0f;
    };

    // 样品信息
    struct SampleInfo
    {
        long id = 0;
        int res_type = 0;
        float weight = 0.0f;
    };

public:
    /**
     * @brief 单索引抽样
     *
     * @param vecSample         抽样物料
     * @param sampleFold        样品倍数
     * @param samplingNum       抽样数量
     * @param weightPrecision   权重精度, 如果大于零认为是使用按权重抽样
     * @param resultData        输出抽样结果: 物料列表
     * @return 无
     */
    static void SingleIndexSampling(
        const std::vector<SampleInfo> &vecSample,
        const int sampleFold,
        const int samplingNum,
        const long weightPrecision,
        std::vector<std::string> &resultData)
    {
        std::unordered_set<std::string> result_samples_set;
        std::unordered_map<std::string, int> sampling_data;
        std::unordered_map<std::string, int> sampling_calc_weight;
        if (weightPrecision > 0)
        {
            WeightSampling(
                vecSample, sampleFold, samplingNum, weightPrecision,
                sampling_data, sampling_calc_weight);
        }
        else
        {
            RandomSampling(vecSample, sampleFold, samplingNum, sampling_data);
        }

#ifdef DEBUG
        std::stringstream sampling_info;
#endif
        for (auto &item_pos : sampling_data)
        {
            result_samples_set.insert(item_pos.first);
#ifdef DEBUG
            if (!sampling_info.str().empty())
            {
                sampling_info << ",";
            }
            sampling_info << item_pos.first << ":" << item_pos.second
                          << ":" << sampling_calc_weight[item_pos.first];
#endif
        }
#ifdef DEBUG
        LOG(INFO) << "[DEBUG] RecallCalc::SingleIndexSampling() Sampling Info"
                  << ", sample size = " << vecSample.size()
                  << ", sampleFold = " << sampleFold
                  << ", samplingNum = " << samplingNum
                  << ", sampling_info = [" << sampling_info.str() << "]";
#endif

        resultData.assign(result_samples_set.begin(), result_samples_set.end());
    }

    /**
     * @brief 多索引抽样
     *
     * @param vecSampleInfo    抽样列表
     * @param vecSamplingNum   分配数量列表
     * @param sampleFold       样品倍数
     * @param weightPrecision  权重精度, 如果大于零认为是使用按权重抽样
     * @param resultData       输出抽样结果: 物料列表
     * @return 无
     */
    static void MultiIndexSampling(
        std::vector<std::vector<SampleInfo>> &vecSampleList,
        std::vector<int> &vecSamplingNum,
        const int sampleFold,
        const long weightPrecision,
        std::vector<std::string> &resultData)
    {
        int sampling_num_size = vecSamplingNum.size();
        int vec_sample_list_size = vecSampleList.size();
        if (vec_sample_list_size != sampling_num_size)
        {
            LOG(ERROR) << "RecallCalc::MultiIndexSampling() SampleList or SamplingNum size error!!!";
            return;
        }

        std::unordered_set<std::string> result_samples_set;
        for (int idx = 0; idx < sampling_num_size; idx++)
        {
            auto &sampling_num = vecSamplingNum[idx];
            const auto &vec_sample = vecSampleList[idx];

            std::unordered_map<std::string, int> sampling_data;
            std::unordered_map<std::string, int> sampling_calc_weight;
            if (weightPrecision > 0)
            {
                WeightSampling(
                    vec_sample, sampleFold, sampling_num, weightPrecision,
                    sampling_data, sampling_calc_weight);
            }
            else
            {
                RandomSampling(vec_sample, sampleFold, sampling_num, sampling_data);
            }

#ifdef DEBUG
            std::stringstream sampling_info;
#endif
            for (auto &item_pos : sampling_data)
            {
                result_samples_set.insert(item_pos.first);
#ifdef DEBUG
                if (!sampling_info.str().empty())
                {
                    sampling_info << ",";
                }
                sampling_info << item_pos.first << ":" << item_pos.second
                              << ":" << sampling_calc_weight[item_pos.first];
#endif
            }
#ifdef DEBUG
            LOG(INFO) << "[DEBUG] RecallCalc::MultiIndexSampling() Sampling Info"
                      << ", sample size = " << vec_sample.size()
                      << ", sampleFold = " << sampleFold
                      << ", sampling_num = " << sampling_num
                      << ", sampling_info = [" << sampling_info.str() << "]";
#endif
        }

        resultData.assign(result_samples_set.begin(), result_samples_set.end());
    }

public:
    /**
     * @brief 随机抽样
     *
     * @param vecSample    抽样列表
     * @param sampleFold   样品倍数
     * @param samplingNum  抽样数量
     * @param resultData   输出抽样结果<item_id, item在物料列表的下标位置>
     * @return 无
     */
    static void RandomSampling(
        const std::vector<SampleInfo> &vecSample,
        const int sampleFold,
        const int samplingNum,
        std::unordered_map<std::string, int> &resultData)
    {
        int sample_size = vecSample.size();
        if (sample_size == 0 || sampleFold < 0 || samplingNum <= 0)
        {
            return;
        }

        // 仅当输入物料数量大于或者等于样本倍数时才进行抽样
        if (sample_size >= sampleFold)
        {
            int use_sampling_num = 0;
            int use_sample_size = 0;

            // 样品倍数为零时，认为直接从所有样本中抽取
            if (sampleFold <= 0)
            {
                use_sample_size = sample_size;
                use_sampling_num = std::min(samplingNum, use_sample_size);
            }
            else
            {
                use_sample_size = std::min(samplingNum * sampleFold, sample_size);
                use_sampling_num = std::min(samplingNum, sample_size / sampleFold);
            }

            int begin_pos = use_sample_size - use_sampling_num;
            for (int idx = begin_pos < 0 ? 0 : begin_pos; idx < use_sample_size; idx++)
            {
                int pos = rand() % (idx + 1);
                const auto &sample_info = vecSample[pos];
                std::string item_key = std::to_string(sample_info.id) + "_" + std::to_string(sample_info.res_type);
                if (resultData.find(item_key) != resultData.end())
                {
                    pos = idx;
                    const auto &tmp_sample_info = vecSample[pos];
                    item_key = std::to_string(tmp_sample_info.id) + "_" + std::to_string(tmp_sample_info.res_type);
                }

                resultData.insert(std::make_pair(item_key, pos));
            }
        }
    }

    /**
     * @brief 按权重抽样
     *
     * @param vecSample         样品列表
     * @param sampleFold        样品倍数
     * @param samplingNum       抽样数量
     * @param weightPrecision   权重精度
     * @param resultData        输出抽样结果<item_id, item在物料列表的下标位置>
     * @param resultCalcWeight  输出抽样结果<item_id, item的计算权重>
     * @return 无
     */
    static void WeightSampling(
        const std::vector<SampleInfo> &vecSample,
        const int sampleFold,
        const int samplingNum,
        const long weightPrecision,
        std::unordered_map<std::string, int> &resultData,
        std::unordered_map<std::string, int> &resultCalcWeight)
    {
        int sample_size = vecSample.size();
        if (sample_size == 0 || sampleFold < 0 || samplingNum <= 0)
        {
            return;
        }

        // 仅当输入物料数量大于或者等于样本倍数时才进行抽样
        if (sample_size >= sampleFold)
        {
            int use_sampling_num = 0;
            int use_sample_size = 0;

            // 样品倍数为零时，认为直接从所有样本中抽取
            if (sampleFold <= 0)
            {
                use_sample_size = sample_size;
                use_sampling_num = std::min(samplingNum, use_sample_size);
            }
            else
            {
                use_sample_size = std::min(samplingNum * sampleFold, sample_size);
                use_sampling_num = std::min(samplingNum, sample_size / sampleFold);
            }

            int total_calc_weight = 0;
            std::unordered_map<std::string, int> use_item_calc_weight;
            std::unordered_map<std::string, int> use_item_index;
            for (int idx = 0; idx < use_sample_size; idx++)
            {
                auto &sample_info = vecSample[idx];
                std::string item_key = std::to_string(sample_info.id) + "_" + std::to_string(sample_info.res_type);
                int calc_weight = sample_info.weight * weightPrecision;
                use_item_calc_weight[item_key] = calc_weight;

                use_item_index[item_key] = idx;
                total_calc_weight += calc_weight;
            }

            for (int idx = 0; idx < use_sampling_num; idx++)
            {
                if (total_calc_weight == 0)
                {
                    break;
                }

                int random_num = rand() % total_calc_weight;
                for (auto iter = use_item_calc_weight.begin(); iter != use_item_calc_weight.end(); iter++)
                {
                    if (random_num < iter->second)
                    {
                        resultData[iter->first] = use_item_index[iter->first];
                        resultCalcWeight[iter->first] = iter->second;

                        total_calc_weight -= iter->second;
                        use_item_calc_weight.erase(iter);
                        break;
                    }
                    else
                    {
                        random_num -= iter->second;
                    }
                }
            }
        }
    }

    /**
     * @brief 分配抽样数量
     *
     * @param vecIDWeight           带权重ID列表: <ID, weight>
     * @param useTopKID             使用ID列表TopK
     * @param totalAllocateNum      总分配数量
     * @param idMaxAllocateNum      单个ID最大分配数量
     * @param isWeightAllocate      是否按权重分配
     * @param actualAllocateNum     实际总分配数量
     * @param vecResultID           输出ID列表
     * @param vecSamplingNum        输出ID对应得分配数量列表
     * @return bool 成功返回true, 失败返回false
     */
    static void AllocateSamplingNum(
        const std::vector<IDWeight> &vecIDWeight,
        const int useTopKID,
        const int totalAllocateNum,
        const int idMaxAllocateNum,
        const bool isWeightAllocate,
        int &actualAllocateNum,
        std::vector<long> &vecResultID,
        std::vector<int> &vecSamplingNum)
    {
        if (useTopKID <= 0)
        {
            return;
        }

        int vec_id_size = vecIDWeight.size();
        int use_id_num = std::min(useTopKID, vec_id_size);
        if (use_id_num <= 0)
        {
            return;
        }

        int useIDMaxAllocateNum =
            (idMaxAllocateNum <= 0) ? totalAllocateNum : idMaxAllocateNum;

        // 计算总权重值
        float total_weight = 0.0;
        vecResultID.resize(use_id_num);
        for (int idx = 0; idx < use_id_num; idx++)
        {
            total_weight += vecIDWeight[idx].weight;
            vecResultID[idx] = vecIDWeight[idx].id;
        }

        vecSamplingNum.resize(use_id_num);
        if (isWeightAllocate && fabs(total_weight) > 1e-6)
        {
            int calc_total_allocate_num = totalAllocateNum;
            float unit_sampling_num = (1.0f / total_weight) * totalAllocateNum;
            for (int idx = 0; idx < use_id_num; idx++)
            {
                const auto &id_weight = vecIDWeight[idx];
                auto &sampling_num = vecSamplingNum[idx];
                sampling_num = (id_weight.weight * unit_sampling_num) + 0.5f;

                // 限制单个ID最大分配值
                sampling_num = std::min(useIDMaxAllocateNum, sampling_num);

                sampling_num = std::min(calc_total_allocate_num, sampling_num);
                calc_total_allocate_num -= sampling_num;

                actualAllocateNum += sampling_num;
            }
        }
        else
        {
            int quotient = totalAllocateNum / use_id_num;
            int modulo = totalAllocateNum % use_id_num;
            for (int idx = 0; idx < use_id_num; idx++)
            {
                auto &sampling_num = vecSamplingNum[idx];
                if (quotient > useIDMaxAllocateNum)
                {
                    sampling_num = useIDMaxAllocateNum;
                }
                else
                {
                    if (idx < modulo)
                    {
                        if ((quotient + 1) > useIDMaxAllocateNum)
                        {
                            sampling_num = useIDMaxAllocateNum;
                        }
                        else
                        {
                            sampling_num = quotient + 1;
                        }
                    }
                    else
                    {
                        sampling_num = quotient;
                    }
                }

                actualAllocateNum += sampling_num;
            }
        }
    }
};