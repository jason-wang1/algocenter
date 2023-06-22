#pragma once
#include "TDPredict/Interface/ModelInterface.h"
#include "TFFeature_Interface.h"
#include "UserFeature.h"
#include "ItemFeature.h"

class TFFeature : public TFFeature_Interface
{
public:
    RegisterFeature(TFFeature);

public:
    virtual bool AddCommonFeature(
        const long user_id,
        const long context_item_id,
        const int context_res_type,
        TDPredict::FeatureItem &feature_item) const noexcept override;

    virtual bool AddRankFeature(
        const long user_id,
        const long item_id,
        const int res_type,
        TDPredict::FeatureItem &feature_item) const noexcept override;
};