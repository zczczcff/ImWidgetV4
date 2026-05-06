#pragma once
#include <imwidgetv4/core/Types.h>
#include <string>
#include <unordered_map>
#include <memory>

namespace ImWidgetV4 {

// 样式集（简化版）
class FStyleSet {
public:
    FStyleSet() = default;
    ~FStyleSet() = default;

    // 禁止拷贝，允许移动
    FStyleSet(const FStyleSet&) = delete;
    FStyleSet& operator=(const FStyleSet&) = delete;
    FStyleSet(FStyleSet&&) = default;
    FStyleSet& operator=(FStyleSet&&) = default;

    // 颜色管理
    void SetColor(const std::string& name, const FColor& color);
    FColor GetColor(const std::string& name) const;
    FColor GetColor(const std::string& name, const FColor& defaultColor) const;
    bool HasColor(const std::string& name) const;
    void RemoveColor(const std::string& name);

    // 浮点值管理
    void SetFloat(const std::string& name, float value);
    float GetFloat(const std::string& name) const;
    float GetFloat(const std::string& name, float defaultValue) const;
    bool HasFloat(const std::string& name) const;
    void RemoveFloat(const std::string& name);

    // 向量值管理
    void SetVector2(const std::string& name, const FVector2& value);
    FVector2 GetVector2(const std::string& name) const;
    FVector2 GetVector2(const std::string& name, const FVector2& defaultValue) const;
    bool HasVector2(const std::string& name) const;
    void RemoveVector2(const std::string& name);

    // 清空所有样式
    void Clear();

    // 合并另一个样式集（会覆盖同名项）
    void Merge(const FStyleSet& other);

    // 获取所有键
    std::vector<std::string> GetColorKeys() const;
    std::vector<std::string> GetFloatKeys() const;
    std::vector<std::string> GetVector2Keys() const;

private:
    std::unordered_map<std::string, FColor> Colors_;
    std::unordered_map<std::string, float> Floats_;
    std::unordered_map<std::string, FVector2> Vectors_;
};

// 主题包
struct FThemePack {
    std::string Name;
    FStyleSet StyleSet;

    FThemePack() = default;
    explicit FThemePack(const std::string& name) : Name(name) {}

    // 禁止拷贝，允许移动
    FThemePack(const FThemePack&) = delete;
    FThemePack& operator=(const FThemePack&) = delete;
    FThemePack(FThemePack&&) = default;
    FThemePack& operator=(FThemePack&&) = default;
};

// 样式集工厂（用于创建预定义主题）
class FStyleSetFactory {
public:
    // 创建默认样式集
    static std::shared_ptr<FStyleSet> CreateDefault();

    // 创建深色主题
    static std::shared_ptr<FStyleSet> CreateDarkTheme();

    // 创建浅色主题
    static std::shared_ptr<FStyleSet> CreateLightTheme();

private:
    FStyleSetFactory() = default;
};

} // namespace ImWidgetV4
