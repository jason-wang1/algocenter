#pragma once
#include <ostream>

namespace Common
{
    // 提供对外的api类型, 对应proto的cmd
    enum APIType
    {
        Get_Download_Recommend = 908001, //  买了又买推荐
    };

    inline std::ostream &operator<<(std::ostream &out, Common::APIType type)
    {
        switch (type)
        {
        case APIType::Get_Download_Recommend:
            out << "-买了又买(";
            break;
        default:
            break;
        }

        out << static_cast<std::underlying_type<Common::APIType>::type>(type) << ")";
        return out;
    }
}
