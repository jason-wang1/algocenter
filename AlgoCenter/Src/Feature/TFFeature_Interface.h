#pragma once
#include <string>
#include <memory>
#include "TDPredict/Interface/FeatureInterface.h"

#include "UserFeature.h"
#include "ItemFeature.h"

struct InitStruct
{
    std::shared_ptr<UserFeature> spUserFeature;
    std::shared_ptr<ItemFeature> spItemFeature;
};
using InitStructPtr = std::shared_ptr<InitStruct>;

// TFFeature_Interface 封装业务实现接口
// P.s> 业务实现方不应修改此文件
class TFFeature_Interface
    : public TDPredict::FeatureInterafce
{
public:
    // 初始化数据接口
    virtual bool SetInitData(const std::any init_data) noexcept override
    {
        if (init_data.type() != typeid(InitStructPtr))
        {
            LOG(ERROR) << "SetInitData() init_data.type() != typeid(InitStructPtr)";
            return false;
        }

        InitStructPtr spInitData = std::any_cast<InitStructPtr>(init_data);
        spUserFeature = spInitData->spUserFeature;
        spItemFeature = spInitData->spItemFeature;
        if (spUserFeature == nullptr || spItemFeature == nullptr)
        {
            return false;
        }
        return true;
    }

    virtual bool AnalyseCommonFeature(TDPredict::RankItem &rank_item) const noexcept override
    {
        // 获取用户ID
        long user_id = 0;
        {
            bool succ = rank_item.getParam("user_id", user_id);
            if (!succ || user_id == 0)
            {
                return false;
            }
        }

        // 获取上下文物料ID
        long context_item_id = 0;
        {
            bool succ = rank_item.getParam("context_item_id", context_item_id);
            if (!succ || context_item_id == 0)
            {
                return false;
            }
        }

        // 获取上下文物料类目
        long context_res_type = 0;
        {
            bool succ = rank_item.getParam("context_res_type", context_res_type);
            if (!succ || context_res_type == 0)
            {
                return false;
            }
        }

        // 调用拼接common特征接口
        auto &common_feature = rank_item.spFeatureData->common_feature;
        return this->AddCommonFeature(user_id, context_item_id, context_res_type, common_feature);
    }

    virtual bool AnalyseRankFeature(TDPredict::RankItem &rank_item) const noexcept override
    {
        // 获取用户ID
        long user_id = 0;
        {
            bool succ = rank_item.getParam("user_id", user_id);
            if (!succ || user_id == 0)
            {
                return false;
            }
        }

        // 获取物料ID
        long item_id = 0;
        {
            bool succ = rank_item.getParam("item_id", item_id);
            if (!succ || item_id == 0)
            {
                return false;
            }
        }

        // 获取物料类目
        long res_type = 0;
        {
            bool succ = rank_item.getParam("res_type", res_type);
            if (!succ || res_type == 0)
            {
                return false;
            }
        }

        // 调用拼接rank特征接口
        auto &rank_feature = rank_item.spFeatureData->rank_feature;
        return this->AddRankFeature(user_id, item_id, res_type, rank_feature);
    }

protected:
    // 业务方实现拼接公共特征接口
    virtual bool AddCommonFeature(
        const long user_id,
        const long context_item_id,
        const int context_res_type,
        TDPredict::FeatureItem &feature_item) const noexcept = 0;

    // 业务方实现拼接排序特征接口
    virtual bool AddRankFeature(
        const long user_id,
        const long item_id,
        const int res_type,
        TDPredict::FeatureItem &feature_item) const noexcept = 0;

protected:
    // 用户/物料特征
    std::shared_ptr<UserFeature> spUserFeature = nullptr;
    std::shared_ptr<ItemFeature> spItemFeature = nullptr;
};