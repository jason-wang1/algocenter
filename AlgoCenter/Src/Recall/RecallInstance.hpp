#pragma once
#include <ostream>
#include <string>
#include <unordered_map>

#include "SingleIndexRecall.h"
#include "MultiIndexRecall.h"
#include "OtherSceneRecall.h"

namespace Common
{
    static std::shared_ptr<Recall_Interface> GetRecallInstance(const long recallTypeId)
    {
        switch (recallTypeId)
        {
        case Common::RT_ID::RTI_ResType_Hot:
        case Common::RT_ID::RTI_ResType_Quality:
        case Common::RT_ID::RTI_ResType_Surge:
        case Common::RT_ID::RTI_ResType_CTCVR:
        case Common::RT_ID::RTI_ResType_DLR:
        {
            return SingleIndexRecall::GetInstance();
            break;
        }
        case Common::RT_ID::RTI_ResType_User_CF:
        case Common::RT_ID::RTI_Item_CF:
        case Common::RT_ID::RTI_Item_Annoy:
        case Common::RT_ID::RTI_ClickOccur:
        case Common::RT_ID::RTI_DownloadOccur:
        {
            return OtherSceneRecall::GetInstance();
            break;
        }
        default:
        {
            break;
        }
        }

        return nullptr;
    }
}
