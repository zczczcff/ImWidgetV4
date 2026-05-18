#pragma once
#include <imwidgetv4/core/Types.h>
#include <nlohmann/json.hpp>
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

    void SetBool(const std::string& name, bool value);
    bool GetBool(const std::string& name) const;
    bool GetBool(const std::string& name, bool defaultValue) const;
    bool HasBool(const std::string& name) const;
    void RemoveBool(const std::string& name);

    void SetMargin(const std::string& name, const FMargin& value);
    FMargin GetMargin(const std::string& name) const;
    FMargin GetMargin(const std::string& name, const FMargin& defaultValue) const;
    bool HasMargin(const std::string& name) const;
    void RemoveMargin(const std::string& name);

    // 清空所有样式
    void Clear();

    // 合并另一个样式集（会覆盖同名项）
    void Merge(const FStyleSet& other);

    // 获取所有键
    std::vector<std::string> GetColorKeys() const;
    std::vector<std::string> GetFloatKeys() const;
    std::vector<std::string> GetVector2Keys() const;
    std::vector<std::string> GetBoolKeys() const;
    std::vector<std::string> GetMarginKeys() const;

    nlohmann::ordered_json ToJson() const;
    bool FromJson(const nlohmann::ordered_json& json, std::string* outError = nullptr);

private:
    std::unordered_map<std::string, FColor> Colors_;
    std::unordered_map<std::string, float> Floats_;
    std::unordered_map<std::string, FVector2> Vectors_;
    std::unordered_map<std::string, bool> Bools_;
    std::unordered_map<std::string, FMargin> Margins_;
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

    static FThemePack CreateThemePackFromJson(
        const nlohmann::ordered_json& json,
        std::string* outError = nullptr);
    static nlohmann::ordered_json ThemePackToJson(const FThemePack& themePack);

private:
    FStyleSetFactory() = default;
};

} // namespace ImWidgetV4
