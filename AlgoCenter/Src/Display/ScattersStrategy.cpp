#include "ScattersStrategy.h"
#include <deque>
#include "glog/logging.h"

#include "Common/Function.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

int ScattersStrategy::CallStrategy(
    const RequestData &requestData,
    const DisplayParam &displayParams,
    ResponseData &responseData)
{
    if (responseData.items_list.size() == 0)
    {
        LOG(WARNING) << "ScattersStrategy::CallStrategy() items is empty"
                     << ", apiType" << requestData.apiType
                     << ", displayExpID = " << requestData.displayExpID
                     << ", uuid = " << requestData.uuid
                     << ", user_id = " << requestData.user_id;
        return Common::Error::OK;
    }

    // 每页数量
    int pageSize = displayParams.GetPageSize();
    if (pageSize <= 0)
    {
        LOG(ERROR) << "ScattersStrategy::CallStrategy() PageSize is zero."
                   << ", apiType" << requestData.apiType
                   << ", displayExpID = " << requestData.displayExpID
                   << ", uuid = " << requestData.uuid
                   << ", user_id = " << requestData.user_id;
        return Common::Error::Display_GetParamsError;
    }

    // 打散TopN页
    int topNPage = displayParams.GetTopNPage();
    if (topNPage <= 0)
    {
        LOG(ERROR) << "ScattersStrategy::CallStrategy() TopNPage is zero."
                   << ", apiType" << requestData.apiType
                   << ", displayExpID = " << requestData.displayExpID
                   << ", uuid = " << requestData.uuid
                   << ", user_id = " << requestData.user_id;
        return Common::Error::Display_GetParamsError;
    }

    // 打散参数
    auto pScattersParam = displayParams.GetScattersParam();
    if (pScattersParam == nullptr)
    {
        LOG(ERROR) << "ScattersStrategy::CallStrategy() ScattersParam is nullptr"
                   << ", apiType" << requestData.apiType
                   << ", exp_id = " << requestData.displayExpID;
        return Common::Error::Display_GetParamsError;
    }

    int secondary_param_size = pScattersParam->vecSecondaryParam.size();
    std::vector<ControlInfo> vec_control_info;
    vec_control_info.resize(secondary_param_size);

    // 控制信息列表
    for (int idx = 0; idx < secondary_param_size; idx++)
    {
        auto &secondary_param = pScattersParam->vecSecondaryParam[idx];
        auto &control_info = vec_control_info[idx];

        // 解析元素列表
        std::vector<std::string> vec_element;
        std::string elementList = secondary_param.GetElementList();
        Common::SplitString(elementList, ',', vec_element);
        if (vec_element.size() == 0)
        {
            LOG(ERROR) << "ScattersStrategy::CallStrategy() ElementList is empty"
                       << ", apiType" << requestData.apiType
                       << ", exp_id = " << requestData.displayExpID;
            return Common::Error::Display_GetParamsError;
        }

        // 控制方式
        control_info.control_method = secondary_param.GetControlMethod();
        if (control_info.control_method != Common::ControlMethod_NotMoreThan &&
            control_info.control_method != Common::ControlMethod_NotLessThan)
        {
            LOG(ERROR) << "ScattersStrategy::CallStrategy() ControlMethod Not Support"
                       << ", apiType" << requestData.apiType
                       << ", exp_id = " << requestData.displayExpID
                       << ", uuid = " << requestData.uuid
                       << ", user_id = " << requestData.user_id
                       << ", control_method = " << control_info.control_method;
            return Common::Error::Display_GetParamsError;
        }

        control_info.element_set.insert(vec_element.begin(), vec_element.end());

        // 控制数量
        control_info.control_count = secondary_param.GetControlCount();
        if (control_info.control_count <= 0 || control_info.control_count > pageSize)
        {
            LOG(ERROR) << "ScattersStrategy::CallStrategy() ControlCount is zero or greater than PageSize"
                       << ", apiType" << requestData.apiType
                       << ", exp_id = " << requestData.displayExpID
                       << ", uuid = " << requestData.uuid
                       << ", user_id = " << requestData.user_id
                       << ", control_count = " << control_info.control_count;
            return Common::Error::Display_GetParamsError;
        }

#ifdef DEBUG
        LOG(INFO) << "ScattersStrategy::CallStrategy() Scatters Param"
                  << ", apiType" << requestData.apiType
                  << ", exp_id = " << requestData.displayExpID
                  << ", uuid = " << requestData.uuid
                  << ", user_id = " << requestData.user_id
                  << ", pageSize = " << pageSize
                  << ", control_count = " << control_info.control_count
                  << ", elementList = [" << elementList << "]";
#endif
    }

    if (requestData.is_statis_log)
    {
        rapidjson::StringBuffer logBuf;
        rapidjson::Writer<rapidjson::StringBuffer> logWriter(logBuf);
        logWriter.StartObject();
        logWriter.Key("item_list");
        logWriter.StartArray();
        for (size_t idx = 0; idx < 20 && idx < responseData.items_list.size(); idx++)
        {
            auto &item_info = responseData.items_list[idx];

            logWriter.StartObject();
            logWriter.Key("item_id");
            logWriter.Int64(item_info.item_id);
            logWriter.Key("res_type");
            logWriter.Int(item_info.res_type);
            logWriter.Key("recall_type");
            logWriter.String(item_info.recall_type.c_str(), item_info.recall_type.size());
            logWriter.EndObject();
        }
        logWriter.EndArray();
        logWriter.EndObject();

        std::string displayJsonLog = "";
        displayJsonLog.append(logBuf.GetString(), logBuf.GetSize());
        LOG(INFO) << "ScattersStrategy::CallStrategy() Scatters Before"
                  << ", apiType" << requestData.apiType
                  << ", exp_id = " << requestData.displayExpID
                  << ", uuid = " << requestData.uuid
                  << ", user_id = " << requestData.user_id
                  << ", Json Log: " << displayJsonLog;
    }

    // 记录上一页处理后剩余的Items
    auto vecPrevSurplusItems = std::move(responseData.items_list);

    int totalItemCount = vecPrevSurplusItems.size();
    responseData.items_list.clear();
    responseData.items_list.reserve(vecPrevSurplusItems.size());

    // Items总页数
    int quotient = totalItemCount / pageSize;
    int modulo = totalItemCount % pageSize;
    int totalPage = (modulo == 0) ? quotient : quotient + 1;

    // 上一页已打散数量
    int prevScattersCount = 0;

    for (int page = 1; page <= topNPage && page <= totalPage; page++)
    {
        // 记录当前页未处理的Items
        std::deque<ItemInfo> deqCurrentUnqualifiedItems;

        // 剩余Items未处理Items数量
        int prevSurplusItemCount = vecPrevSurplusItems.size();

        // 当前页抽取完成后，已打散页总数量,
        // 非最后一页：页数 * 每页数量
        // 最后一页 = 地图总数量
        int currentScattersCount = (page == totalPage) ? totalItemCount : page * pageSize;

        // 实际已抽取到的Items总数量
        int actualExtractedCount = responseData.items_list.size();

        auto page_control_info = vec_control_info;
        for (int idx = 0; idx < prevSurplusItemCount; idx++)
        {
            auto &item_info = vecPrevSurplusItems[idx];

            // 是否符合条件
            bool isQualified = true;
            int sum_control_count = 0;
            int sum_now_count = 0;
            for (auto &control_info : page_control_info)
            {
                auto &element = item_info.recall_type;
                if (control_info.element_set.find(element) != control_info.element_set.end())
                {
                    control_info.now_count += 1;
                }

                if (control_info.control_method == Common::ControlMethod_NotMoreThan)
                {
                    if (control_info.now_count > control_info.control_count)
                    {
                        isQualified = false;
                    }
                }
                else if (control_info.control_method == Common::ControlMethod_NotLessThan)
                {
                    sum_control_count += control_info.control_count;
                    sum_now_count += std::min(control_info.now_count, control_info.control_count);
                    if ((sum_control_count - sum_now_count) > (currentScattersCount - (actualExtractedCount + 1)))
                    {
                        isQualified = false;
                    }
                }
            }

            if (!isQualified)
            {
                for (auto &control_info : page_control_info)
                {
                    auto &element = item_info.recall_type;
                    if (control_info.element_set.find(element) != control_info.element_set.end())
                    {
                        control_info.now_count -= 1;
                    }
                }

                deqCurrentUnqualifiedItems.push_back(std::move(item_info));
            }
            else
            {
                // 符合条件的Items则直接放入返回值：DisplayItemList
                responseData.items_list.emplace_back(std::move(item_info));

                // 更新实际已抽取到的Items总数量
                actualExtractedCount = responseData.items_list.size();

                // 当抽到足够的地图后退出循环
                if (actualExtractedCount >= currentScattersCount)
                {
                    break;
                }
            }
        }

        // 需要补充的Items数量，如果当前页抽取完成，无需补充Items
        int needSupplyCount = 0;

        // 当前页未抽取到足够的符合条件的Items, 需要补充Items
        if (actualExtractedCount < currentScattersCount)
        {
            needSupplyCount = currentScattersCount - actualExtractedCount;
        }

        // 如果当前页未抽取到足够符合条件的Items，则优先使用排名靠前的Items补充, 即使不符合条件
        while (needSupplyCount > 0)
        {
            auto beginIter = deqCurrentUnqualifiedItems.begin();
            responseData.items_list.emplace_back(std::move(*beginIter));
            deqCurrentUnqualifiedItems.erase(beginIter);
            --needSupplyCount;
        }

        // 当前页抽取数量
        int currentExtractedCount = currentScattersCount - prevScattersCount;

        // 未抽取&未进入不符合条件的Items起始index
        int moveStartIdx = currentExtractedCount + deqCurrentUnqualifiedItems.size();
        for (int idx = moveStartIdx; idx < prevSurplusItemCount; idx++)
        {
            deqCurrentUnqualifiedItems.push_back(std::move(vecPrevSurplusItems[idx]));
        }

        vecPrevSurplusItems.clear();
        for (auto iter = deqCurrentUnqualifiedItems.begin(); iter != deqCurrentUnqualifiedItems.end(); iter++)
        {
            vecPrevSurplusItems.emplace_back(std::move(*iter));
        }

        prevScattersCount = responseData.items_list.size();
    }

    // 不需要打散的页面Items放入返回值：items_list
    int lastEndIdx = vecPrevSurplusItems.size();
    for (int idx = 0; idx < lastEndIdx; idx++)
    {
        responseData.items_list.emplace_back(std::move(vecPrevSurplusItems[idx]));
    }

    if (requestData.is_statis_log)
    {
        rapidjson::StringBuffer logBuf;
        rapidjson::Writer<rapidjson::StringBuffer> logWriter(logBuf);
        logWriter.StartObject();
        logWriter.Key("item_list");
        logWriter.StartArray();
        for (size_t idx = 0; idx < 20 && idx < responseData.items_list.size(); idx++)
        {
            auto &item_info = responseData.items_list[idx];

            logWriter.StartObject();
            logWriter.Key("item_id");
            logWriter.Int64(item_info.item_id);
            logWriter.Key("res_type");
            logWriter.Int(item_info.res_type);
            logWriter.Key("recall_type");
            logWriter.String(item_info.recall_type.c_str(), item_info.recall_type.size());
            logWriter.EndObject();
        }
        logWriter.EndArray();
        logWriter.EndObject();

        std::string displayJsonLog = "";
        displayJsonLog.append(logBuf.GetString(), logBuf.GetSize());
        LOG(INFO) << "ScattersStrategy::CallStrategy() Scatters After"
                  << ", apiType" << requestData.apiType
                  << ", exp_id = " << requestData.displayExpID
                  << ", uuid = " << requestData.uuid
                  << ", user_id = " << requestData.user_id
                  << ", Json Log: " << displayJsonLog;
    }

    return Common::Error::OK;
}
