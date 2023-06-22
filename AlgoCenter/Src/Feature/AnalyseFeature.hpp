#pragma once
#include <string>
#include <memory>
#include "Protobuf/redis/user_feature_data.pb.h"
#include "Protobuf/redis/item_feature_data.pb.h"
#include "TDPredict/Interface/ModelInterface.h"
#include "FeatureField.h"

class AnalyseFeature
{
public:
    static bool AnalyseUserFeatureBasic(
        const std::shared_ptr<RSP_UserFeatureData::UserFeatureBasic> ufb,
        TDPredict::FeatureItem &feature_item)
    {
        if (ufb == nullptr)
        {
            return false;
        }

        using namespace FeatureField;
        using namespace FeatureField::UserFeatureBasic;
        AnalyseSingleFeature(feature_item, ufb, user_id, mod_pow_19);
        AnalyseSingleFeature(feature_item, ufb, user_sex, 0);
        AnalyseSingleFeature(feature_item, ufb, user_province_code, mod_pow_8);
        AnalyseSingleFeature(feature_item, ufb, user_city_code, mod_pow_11);
        AnalyseSingleFeature(feature_item, ufb, user_profession, 0);

        static const std::vector<int> user_lb_bucket = {1, 6, 22, 189, 410, 651};
        const int user_lb_value = FeatureField::trans_bucket(user_lb_bucket, ufb->user_lb());
        feature_item[user_lb].emplace_back(user_lb, user_lb_value, 1.0);

        AnalyseSingleFeature(feature_item, ufb, is_vip, 0);

        static const std::vector<int> growth_value_bucket = {0, 10, 100, 1000, 10000, 100000};
        const int growth_value_value = FeatureField::trans_bucket(growth_value_bucket, ufb->growth_value());
        feature_item[growth_value].emplace_back(growth_value, growth_value_value, 1.0);

        AnalyseSingleFeature(feature_item, ufb, vip_status, 0);

        return true;
    }

    static bool AnalyseUserLiveFeatureClick(
        const int context_res_type,
        const std::shared_ptr<RSP_UserLiveFeatureData::UserLiveFeatureClick> ulfc,
        TDPredict::FeatureItem &feature_item)
    {
        if (ulfc == nullptr)
        {
            return false;
        }

        using namespace FeatureField;
        using namespace FeatureField::UserLiveFeatureClick;

        long now_time = Common::get_timestamp();
        std::vector<RSP_UserLiveFeatureData::IDTime> id_time_list;

        const auto &ulfc_click_item = ulfc->click_item();
        if (context_res_type > 0)
        {
            auto iter = ulfc_click_item.find(context_res_type);
            if (iter != ulfc_click_item.end())
            {
                const auto &ulfc_click_item_list = iter->second.id_list();
                for (const auto &id_time : ulfc_click_item_list)
                {
                    if (now_time - id_time.timestamp() < 86400)
                    {
                        id_time_list.push_back(id_time);
                    }
                }
            }
        }
        else
        {
            for (const auto res_type_click_item : ulfc_click_item)
            {
                const auto &ulfc_click_item_list = res_type_click_item.second.id_list();
                for (const auto &id_time : ulfc_click_item_list)
                {
                    if (now_time - id_time.timestamp() < 86400)
                    {
                        id_time_list.push_back(id_time);
                    }
                }
            }
        }

        static const auto &time_cmp =
            [](const RSP_UserLiveFeatureData::IDTime &a, const RSP_UserLiveFeatureData::IDTime &b)
        { return a.timestamp() > b.timestamp(); };
        std::sort(id_time_list.begin(), id_time_list.end(), time_cmp);

        auto &click_item_field = feature_item[click_item];
        for (const auto &id_time : id_time_list)
        {
            click_item_field.emplace_back(click_item, id_time.id() & mod_pow_20, 1.0);
        }

        return true;
    }

    static bool AnalyseUserLiveFeatureSearch(
        const int context_res_type,
        const std::shared_ptr<RSP_UserLiveFeatureData::UserLiveFeatureSearch> ulfs,
        TDPredict::FeatureItem &feature_item)
    {
        if (ulfs == nullptr)
        {
            return false;
        }

        using namespace FeatureField;
        using namespace FeatureField::UserLiveFeatureSearch;

        long now_time = Common::get_timestamp();
        std::vector<RSP_UserLiveFeatureData::IDTime> id_time_list;

        const auto &ulfs_search_words = ulfs->search_words();
        if (context_res_type > 0)
        {
            auto iter = ulfs_search_words.find(context_res_type);
            if (iter != ulfs_search_words.end())
            {
                const auto &ulfs_search_words_list = iter->second.id_list();
                for (const auto &id_time : ulfs_search_words_list)
                {
                    if (now_time - id_time.timestamp() < 86400)
                    {
                        id_time_list.push_back(id_time);
                    }
                }
            }
        }
        else
        {
            for (const auto res_type_search_words : ulfs_search_words)
            {
                const auto &ulfs_search_words_list = res_type_search_words.second.id_list();
                for (const auto &id_time : ulfs_search_words_list)
                {
                    if (now_time - id_time.timestamp() < 86400)
                    {
                        id_time_list.push_back(id_time);
                    }
                }
            }
        }

        static const auto &time_cmp =
            [](const RSP_UserLiveFeatureData::IDTime &a, const RSP_UserLiveFeatureData::IDTime &b)
        { return a.timestamp() > b.timestamp(); };
        std::sort(id_time_list.begin(), id_time_list.end(), time_cmp);

        auto &search_words_field = feature_item[search_words];
        for (const auto &id_time : id_time_list)
        {
            search_words_field.emplace_back(search_words, id_time.id() & mod_pow_20, 1.0);
        }

        return true;
    }

    static bool AnalyseUserLiveFeatureDownload(
        const int context_res_type,
        const std::shared_ptr<RSP_UserLiveFeatureData::UserLiveFeatureDownload> ulfd,
        TDPredict::FeatureItem &feature_item)
    {
        if (ulfd == nullptr)
        {
            return false;
        }

        using namespace FeatureField;
        using namespace FeatureField::UserLiveFeatureDownload;

        long now_time = Common::get_timestamp();
        std::vector<RSP_UserLiveFeatureData::IDTime> id_time_list;

        const auto &ulfd_download_item = ulfd->download_item();
        if (context_res_type > 0)
        {
            auto iter = ulfd_download_item.find(context_res_type);
            if (iter != ulfd_download_item.end())
            {
                const auto &ulfd_download_item_list = iter->second.id_list();
                for (const auto &id_time : ulfd_download_item_list)
                {
                    if (now_time - id_time.timestamp() < 86400)
                    {
                        id_time_list.push_back(id_time);
                    }
                }
            }
        }
        else
        {
            for (const auto res_type_download_item : ulfd_download_item)
            {
                const auto &ulfd_download_item_list = res_type_download_item.second.id_list();
                for (const auto &id_time : ulfd_download_item_list)
                {
                    if (now_time - id_time.timestamp() < 86400)
                    {
                        id_time_list.push_back(id_time);
                    }
                }
            }
        }

        static const auto &time_cmp =
            [](const RSP_UserLiveFeatureData::IDTime &a, const RSP_UserLiveFeatureData::IDTime &b)
        { return a.timestamp() > b.timestamp(); };
        std::sort(id_time_list.begin(), id_time_list.end(), time_cmp);

        auto &download_item_field = feature_item[download_item];
        for (const auto &id_time : id_time_list)
        {
            download_item_field.emplace_back(download_item, id_time.id() & mod_pow_20, 1.0);
        }

        return true;
    }

    static bool AnalyseItemFeatureBasic(
        const std::shared_ptr<RSP_ItemFeatureData::ItemFeatureBasic> ifb,
        TDPredict::FeatureItem &feature_item)
    {
        if (ifb == nullptr)
        {
            return false;
        }

        using namespace FeatureField;
        using namespace FeatureField::ItemFeatureBasic;

        AnalyseSingleFeature(feature_item, ifb, ll_id, mod_pow_20);
        AnalyseSingleFeature(feature_item, ifb, res_style, 0);
        AnalyseSingleFeature(feature_item, ifb, style_id, 0);
        AnalyseSingleFeature(feature_item, ifb, res_name, 0);
        AnalyseSingleFeature(feature_item, ifb, class_id_1, 0);
        AnalyseSingleFeature(feature_item, ifb, class_id_2, 0);
        AnalyseSingleFeature(feature_item, ifb, class_id, 0);

        static const std::vector<int> ifb_res_price = {1, 10, 28, 32, 80, 100, 280};
        const int res_price_value = FeatureField::trans_bucket(ifb_res_price, ifb->res_price());
        feature_item[res_price].emplace_back(res_price, res_price_value, 1.0);

        AnalyseSingleFeature(feature_item, ifb, pay_type, 0);

        static const std::vector<int> ifb_up_time = {1, 7, 14, 28, 90, 365, 1095};
        const int up_time_value = FeatureField::trans_bucket(ifb_up_time, ifb->up_time());
        feature_item[up_time].emplace_back(up_time, up_time_value, 1.0);

        auto &keyname_list_field = feature_item[keyname_list];
        const auto &ifb_keyname_list = ifb->keyname_list();
        for (const auto keyname : ifb_keyname_list)
        {
            keyname_list_field.emplace_back(keyname_list, keyname, 1.0);
        }

        AnalyseSingleFeature(feature_item, ifb, res_type, 0);

        AnalyseSingleFeature(feature_item, ifb, user_id, mod_pow_17);
        AnalyseSingleFeature(feature_item, ifb, studio_id, mod_pow_9);

        AnalyseSingleFeature(feature_item, ifb, heart_level, 0);
        AnalyseSingleFeature(feature_item, ifb, type, 0);
        AnalyseSingleFeature(feature_item, ifb, is_single, 0);
        AnalyseSingleFeature(feature_item, ifb, res_map, 0);
        AnalyseSingleFeature(feature_item, ifb, has_3dl, 0);
        AnalyseSingleFeature(feature_item, ifb, renderer_version, 0);
        AnalyseSingleFeature(feature_item, ifb, res_good, 0);
        AnalyseSingleFeature(feature_item, ifb, is_pano, 0);
        AnalyseSingleFeature(feature_item, ifb, is_brand, 0);
        AnalyseSingleFeature(feature_item, ifb, have_lights, 0);
        AnalyseSingleFeature(feature_item, ifb, use_place, 0);

        return true;
    }

    static bool AnalyseItemFeatureStatis(
        const std::shared_ptr<RSP_ItemFeatureData::ItemFeatureStatis> ifs,
        TDPredict::FeatureItem &feature_item)
    {
        if (ifs == nullptr)
        {
            return false;
        }

        using namespace FeatureField;
        using namespace FeatureField::ItemFeatureStatis;

        static const std::vector<int> ifs_click_14_days = {0, 1, 2, 3, 4, 5, 8, 20};
        const int click_14_days_value = FeatureField::trans_bucket(ifs_click_14_days, ifs->click_14_days());
        feature_item[click_14_days].emplace_back(click_14_days, click_14_days_value, 1.0);

        static const std::vector<int> ifs_click_30_days = {0, 1, 2, 3, 5, 10, 20, 50};
        const int click_30_days_value = FeatureField::trans_bucket(ifs_click_30_days, ifs->click_30_days());
        feature_item[click_30_days].emplace_back(click_30_days, click_30_days_value, 1.0);

        static const std::vector<float> ifs_ctr_14_days = {0.0, 0.001, 0.005, 0.01, 0.02, 0.03, 0.05};
        const int ctr_14_days_value = FeatureField::trans_bucket(ifs_ctr_14_days, ifs->ctr_14_days());
        feature_item[ctr_14_days].emplace_back(ctr_14_days, ctr_14_days_value, 1.0);

        static const std::vector<float> ifs_ctr_30_days = {0.0, 0.001, 0.005, 0.01, 0.02, 0.03, 0.05};
        const int ctr_30_days_value = FeatureField::trans_bucket(ifs_ctr_30_days, ifs->ctr_30_days());
        feature_item[ctr_30_days].emplace_back(ctr_30_days, ctr_30_days_value, 1.0);

        static const std::vector<int> ifs_download_14_days = {0, 1, 2, 3, 4, 5, 10};
        const int download_14_days_value = FeatureField::trans_bucket(ifs_download_14_days, ifs->download_14_days());
        feature_item[download_14_days].emplace_back(download_14_days, download_14_days_value, 1.0);

        static const std::vector<int> ifs_download_30_days = {0, 1, 2, 3, 5, 10, 20};
        const int download_30_days_value = FeatureField::trans_bucket(ifs_download_30_days, ifs->download_30_days());
        feature_item[download_30_days].emplace_back(download_30_days, download_30_days_value, 1.0);

        static const std::vector<int> ifs_ctcvr_14_days = {0, 1, 2, 3, 4, 5, 10};
        const int ctcvr_14_days_value = FeatureField::trans_bucket(ifs_ctcvr_14_days, ifs->ctcvr_14_days());
        feature_item[ctcvr_14_days].emplace_back(ctcvr_14_days, ctcvr_14_days_value, 1.0);

        static const std::vector<int> ifs_ctcvr_30_days = {0, 1, 2, 3, 5, 10, 20};
        const int ctcvr_30_days_value = FeatureField::trans_bucket(ifs_ctcvr_30_days, ifs->ctcvr_30_days());
        feature_item[ctcvr_30_days].emplace_back(ctcvr_30_days, ctcvr_30_days_value, 1.0);

        return true;
    }

    static bool AnalyseContextFeatureItemBasic(
        const std::shared_ptr<RSP_ItemFeatureData::ItemFeatureBasic> ifb,
        TDPredict::FeatureItem &feature_item)
    {
        if (ifb == nullptr)
        {
            return false;
        }

        using namespace FeatureField;
        using namespace FeatureField::ContextFeatureItemBasic;

        AnalyseSingleFeature(feature_item, ifb, ll_id, mod_pow_20);
        AnalyseSingleFeature(feature_item, ifb, res_style, 0);
        AnalyseSingleFeature(feature_item, ifb, style_id, 0);
        AnalyseSingleFeature(feature_item, ifb, res_name, 0);
        AnalyseSingleFeature(feature_item, ifb, class_id_1, 0);
        AnalyseSingleFeature(feature_item, ifb, class_id_2, 0);
        AnalyseSingleFeature(feature_item, ifb, class_id, 0);

        static const std::vector<int> ifb_res_price = {1, 10, 28, 32, 80, 100, 280};
        const int res_price_value = FeatureField::trans_bucket(ifb_res_price, ifb->res_price());
        feature_item[res_price].emplace_back(res_price, res_price_value, 1.0);

        AnalyseSingleFeature(feature_item, ifb, pay_type, 0);

        static const std::vector<int> ifb_up_time = {1, 7, 14, 28, 90, 365, 1095};
        const int up_time_value = FeatureField::trans_bucket(ifb_up_time, ifb->up_time());
        feature_item[up_time].emplace_back(up_time, up_time_value, 1.0);

        auto &keyname_list_field = feature_item[keyname_list];
        const auto &ifb_keyname_list = ifb->keyname_list();
        for (const auto keyname : ifb_keyname_list)
        {
            keyname_list_field.emplace_back(keyname_list, keyname, 1.0);
        }

        AnalyseSingleFeature(feature_item, ifb, res_type, 0);

        AnalyseSingleFeature(feature_item, ifb, user_id, mod_pow_17);
        AnalyseSingleFeature(feature_item, ifb, studio_id, mod_pow_9);

        AnalyseSingleFeature(feature_item, ifb, heart_level, 0);
        AnalyseSingleFeature(feature_item, ifb, type, 0);
        AnalyseSingleFeature(feature_item, ifb, is_single, 0);
        AnalyseSingleFeature(feature_item, ifb, res_map, 0);
        AnalyseSingleFeature(feature_item, ifb, has_3dl, 0);
        AnalyseSingleFeature(feature_item, ifb, renderer_version, 0);
        AnalyseSingleFeature(feature_item, ifb, res_good, 0);
        AnalyseSingleFeature(feature_item, ifb, is_pano, 0);
        AnalyseSingleFeature(feature_item, ifb, is_brand, 0);
        AnalyseSingleFeature(feature_item, ifb, have_lights, 0);
        AnalyseSingleFeature(feature_item, ifb, use_place, 0);

        return true;
    }

    static bool AnalyseContextFeatureItemStatis(
        const std::shared_ptr<RSP_ItemFeatureData::ItemFeatureStatis> ifs,
        TDPredict::FeatureItem &feature_item)
    {
        if (ifs == nullptr)
        {
            return false;
        }

        using namespace FeatureField;
        using namespace FeatureField::ContextFeatureItemStatis;

        static const std::vector<int> ifs_click_14_days = {0, 1, 2, 3, 4, 5, 8, 20};
        const int click_14_days_value = FeatureField::trans_bucket(ifs_click_14_days, ifs->click_14_days());
        feature_item[click_14_days].emplace_back(click_14_days, click_14_days_value, 1.0);

        static const std::vector<int> ifs_click_30_days = {0, 1, 2, 3, 5, 10, 20, 50};
        const int click_30_days_value = FeatureField::trans_bucket(ifs_click_30_days, ifs->click_30_days());
        feature_item[click_30_days].emplace_back(click_30_days, click_30_days_value, 1.0);

        static const std::vector<float> ifs_ctr_14_days = {0.0, 0.001, 0.005, 0.01, 0.02, 0.03, 0.05};
        const int ctr_14_days_value = FeatureField::trans_bucket(ifs_ctr_14_days, ifs->ctr_14_days());
        feature_item[ctr_14_days].emplace_back(ctr_14_days, ctr_14_days_value, 1.0);

        static const std::vector<float> ifs_ctr_30_days = {0.0, 0.001, 0.005, 0.01, 0.02, 0.03, 0.05};
        const int ctr_30_days_value = FeatureField::trans_bucket(ifs_ctr_30_days, ifs->ctr_30_days());
        feature_item[ctr_30_days].emplace_back(ctr_30_days, ctr_30_days_value, 1.0);

        static const std::vector<int> ifs_download_14_days = {0, 1, 2, 3, 4, 5, 10};
        const int download_14_days_value = FeatureField::trans_bucket(ifs_download_14_days, ifs->download_14_days());
        feature_item[download_14_days].emplace_back(download_14_days, download_14_days_value, 1.0);

        static const std::vector<int> ifs_download_30_days = {0, 1, 2, 3, 5, 10, 20};
        const int download_30_days_value = FeatureField::trans_bucket(ifs_download_30_days, ifs->download_30_days());
        feature_item[download_30_days].emplace_back(download_30_days, download_30_days_value, 1.0);

        static const std::vector<int> ifs_ctcvr_14_days = {0, 1, 2, 3, 4, 5, 10};
        const int ctcvr_14_days_value = FeatureField::trans_bucket(ifs_ctcvr_14_days, ifs->ctcvr_14_days());
        feature_item[ctcvr_14_days].emplace_back(ctcvr_14_days, ctcvr_14_days_value, 1.0);

        static const std::vector<int> ifs_ctcvr_30_days = {0, 1, 2, 3, 5, 10, 20};
        const int ctcvr_30_days_value = FeatureField::trans_bucket(ifs_ctcvr_30_days, ifs->ctcvr_30_days());
        feature_item[ctcvr_30_days].emplace_back(ctcvr_30_days, ctcvr_30_days_value, 1.0);

        return true;
    }
};
