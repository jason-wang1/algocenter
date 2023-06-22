#pragma once
#include <ostream>
namespace Common
{
    // 展控策略 - 错误类型
    static const std::string DisplayType_ErrorType = "";

    // 展控策略 - 打散策略
    static const std::string DisplayType_Scatter = "Scatter";
    static const std::string ControlMethod_NotMoreThan = "NotMoreThan"; // 控制方式 - 不超过
    static const std::string ControlMethod_NotLessThan = "NotLessThan"; // 控制方式 - 不少于

}
