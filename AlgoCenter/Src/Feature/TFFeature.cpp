#include "TFFeature.h"
#include "glog/logging.h"
#include "FeatureField.h"

#include "AnalyseFeature.hpp"

bool TFFeature::AddCommonFeature(
    const long user_id,
    const long context_item_id,
    const int context_res_type,
    TDPredict::FeatureItem &feature_item) const noexcept
{
    if (spUserFeature != nullptr)
    {
        // 用户基础特征
        AnalyseFeature::AnalyseUserFeatureBasic(spUserFeature->GetUserFeatureBasic(), feature_item);

        // 用户实时点击特征
        AnalyseFeature::AnalyseUserLiveFeatureClick(context_res_type, spUserFeature->GetUserLiveFeatureClick(), feature_item);

        // 用户实时搜索特征
        AnalyseFeature::AnalyseUserLiveFeatureSearch(context_res_type, spUserFeature->GetUserLiveFeatureSearch(), feature_item);

        // 用户实时下载特征
        AnalyseFeature::AnalyseUserLiveFeatureDownload(context_res_type, spUserFeature->GetUserLiveFeatureDownload(), feature_item);
    }

    if (spItemFeature != nullptr)
    {
        // 上下文物料基础特征
        AnalyseFeature::AnalyseContextFeatureItemBasic(
            spItemFeature->GetItemFeatureBasic(context_item_id, context_res_type), feature_item);

        // 上下文物料统计特征
        AnalyseFeature::AnalyseContextFeatureItemStatis(
            spItemFeature->GetItemFeatureStatis(context_item_id, context_res_type), feature_item);
    }

    return true;
}

bool TFFeature::AddRankFeature(
    const long user_id,
    const long item_id,
    const int res_type,
    TDPredict::FeatureItem &feature_item) const noexcept
{
    if (spItemFeature != nullptr)
    {
        // 上下文物料基础特征
        AnalyseFeature::AnalyseItemFeatureBasic(spItemFeature->GetItemFeatureBasic(item_id, res_type), feature_item);

        // 上下文物料统计特征
        AnalyseFeature::AnalyseItemFeatureStatis(spItemFeature->GetItemFeatureStatis(item_id, res_type), feature_item);
    }

    return true;
}
