#pragma once
#include <vector>
#include <unordered_map>

struct ItemInfo
{
    long item_id = 0;
    int res_type = 0;

    long recall_type_id = 0;
    std::string recall_type = "";

    float score = 0.0f;
    float sc_score = 0.0f;

    ItemInfo() {}

    ItemInfo(long _item_id,
             int _res_type,
             long _recall_type_id,
             const std::string &_recall_type,
             float _score,
             float _sc_score)
        : item_id(_item_id),
          res_type(_res_type),
          recall_type_id(_recall_type_id),
          recall_type(_recall_type),
          score(_score),
          sc_score(_sc_score)
    {
    }
};

class ResponseData
{
public:
    // 最终返回结果
    std::vector<ItemInfo> items_list;

    // 备用召回
    std::vector<ItemInfo> spare_items_list;

    // 独立召回集
    std::unordered_map<std::string, std::vector<ItemInfo>> exclusive_items_list;
};