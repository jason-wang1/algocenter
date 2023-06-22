#pragma once
#include <vector>

namespace FeatureField
{
    inline constexpr int mod_pow_8 = 255;
    inline constexpr int mod_pow_9 = 511;
    inline constexpr int mod_pow_11 = 2047;
    inline constexpr int mod_pow_17 = 131071;
    inline constexpr int mod_pow_19 = 524287;
    inline constexpr int mod_pow_20 = 1048575;

    namespace UserFeatureBasic
    {
        enum
        {
            user_id = 1,
            user_sex = 2,
            user_province_code = 3,
            user_city_code = 4,
            user_profession = 5,
            user_lb = 9,
            is_vip = 10,
            growth_value = 11,
            vip_status = 12,
        };
    }

    namespace UserLiveFeatureClick
    {
        enum
        {
            click_item = 6,
        };
    }

    namespace UserLiveFeatureSearch
    {
        enum
        {
            search_words = 7,
        };
    }

    namespace UserLiveFeatureDownload
    {
        enum
        {
            download_item = 8,
        };
    }

    namespace ItemFeatureBasic
    {
        enum
        {
            ll_id = 13,
            res_style = 14,
            style_id = 15,
            res_name = 16,
            class_id_1 = 17,
            class_id_2 = 18,
            class_id = 19,
            res_price = 20,
            pay_type = 21,
            up_time = 22,
            keyname_list = 23,
            res_type = 24,
            user_id = 25,
            studio_id = 26,
            heart_level = 27,
            type = 28,
            is_single = 29,
            res_map = 30,
            has_3dl = 31,
            renderer_version = 32,
            res_good = 33,
            is_pano = 34,
            is_brand = 35,
            have_lights = 36,
            use_place = 37,
        };
    }

    namespace ItemFeatureStatis
    {
        enum
        {
            click_14_days = 38,
            click_30_days = 39,
            ctr_14_days = 40,
            ctr_30_days = 41,
            download_14_days = 42,
            download_30_days = 43,
            ctcvr_14_days = 44,
            ctcvr_30_days = 45,
        };
    }

    namespace ContextFeatureItemBasic
    {
        enum
        {
            ll_id = 46,
            res_style = 47,
            style_id = 48,
            res_name = 49,
            class_id_1 = 50,
            class_id_2 = 51,
            class_id = 52,
            res_price = 53,
            pay_type = 54,
            up_time = 55,
            keyname_list = 56,
            res_type = 57,
            user_id = 58,
            studio_id = 59,
            heart_level = 60,
            type = 61,
            is_single = 62,
            res_map = 63,
            has_3dl = 64,
            renderer_version = 65,
            res_good = 66,
            is_pano = 67,
            is_brand = 68,
            have_lights = 69,
            use_place = 70,
        };
    }

    namespace ContextFeatureItemStatis
    {
        enum
        {
            click_14_days = 71,
            click_30_days = 72,
            ctr_14_days = 73,
            ctr_30_days = 74,
            download_14_days = 75,
            download_30_days = 76,
            ctcvr_14_days = 77,
            ctcvr_30_days = 78,
        };
    }

    inline int trans_bucket(const std::vector<int> &bucket, const int weight)
    {
        const int size = bucket.size();
        for (int idx = 0; idx < size; idx++)
        {
            if (weight < bucket[idx])
            {
                return idx;
            }
        }
        return size;
    }

    inline int trans_bucket(const std::vector<float> &bucket, const float weight)
    {
        const int size = bucket.size();
        for (int idx = 0; idx < size; idx++)
        {
            if (weight < bucket[idx])
            {
                return idx;
            }
        }
        return size;
    }

#define AnalyseSingleFeature(feature_item, proto, name, mod)               \
    {                                                                      \
        const int use_val = mod > 0 ? proto->name() & mod : proto->name(); \
        feature_item[name].emplace_back(name, use_val, 1.0);               \
    }

#define AnalyseRepeatedFeature(feature_item, proto, name, count)                   \
    {                                                                              \
        const auto &proto_list = proto->name();                                    \
        auto &field_list = feature_item[name];                                     \
        const int proto_size = proto_list.size();                                  \
        const int use_size = count > 0 ? std::min(count, proto_size) : proto_size; \
        field_list.reserve(use_size);                                              \
        for (int idx = 0; idx < use_size; ++idx)                                   \
        {                                                                          \
            const auto &item = proto_list.at(idx);                                 \
            field_list.emplace_back(name, item.id(), item.weight());               \
        }                                                                          \
    }

}
