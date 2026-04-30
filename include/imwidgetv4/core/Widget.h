#pragma once
#include <imwidgetv4/core/Types.h>
#include <memory>
#include <string>

namespace ImWidgetV4 {

/**
 * @brief 简化的 Widget 基类（占位符）
 *
 * 这是一个简化版本的控件基类，用于支持 Application 类的实现。
 * 完整的 Widget 系统将在后续阶段实现。
 */
class ImWidget {
public:
    ImWidget() = default;
    virtual ~ImWidget() = default;

    /**
     * @brief 设置控件名称
     * @param name 控件名称
     */
    void SetName(const std::string& name) { Name_ = name; }

    /**
     * @brief 获取控件名称
     * @return 控件名称
     */
    const std::string& GetName() const { return Name_; }

private:
    std::string Name_;
};

} // namespace ImWidgetV4
