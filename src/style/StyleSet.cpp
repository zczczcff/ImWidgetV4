#include <imwidgetv4/style/StyleSet.h>
#include <algorithm>

namespace ImWidgetV4 {

// ============================================================================
// FStyleSet 实现
// ============================================================================

void FStyleSet::SetColor(const std::string& name, const FColor& color) {
    Colors_[name] = color;
}

FColor FStyleSet::GetColor(const std::string& name) const {
    auto it = Colors_.find(name);
    if (it != Colors_.end()) {
        return it->second;
    }
    return FColor::White; // 默认返回白色
}

FColor FStyleSet::GetColor(const std::string& name, const FColor& defaultColor) const {
    auto it = Colors_.find(name);
    if (it != Colors_.end()) {
        return it->second;
    }
    return defaultColor;
}

bool FStyleSet::HasColor(const std::string& name) const {
    return Colors_.find(name) != Colors_.end();
}

void FStyleSet::RemoveColor(const std::string& name) {
    Colors_.erase(name);
}

void FStyleSet::SetFloat(const std::string& name, float value) {
    Floats_[name] = value;
}

float FStyleSet::GetFloat(const std::string& name) const {
    auto it = Floats_.find(name);
    if (it != Floats_.end()) {
        return it->second;
    }
    return 0.0f; // 默认返回 0
}

float FStyleSet::GetFloat(const std::string& name, float defaultValue) const {
    auto it = Floats_.find(name);
    if (it != Floats_.end()) {
        return it->second;
    }
    return defaultValue;
}

bool FStyleSet::HasFloat(const std::string& name) const {
    return Floats_.find(name) != Floats_.end();
}

void FStyleSet::RemoveFloat(const std::string& name) {
    Floats_.erase(name);
}

void FStyleSet::SetVector2(const std::string& name, const FVector2& value) {
    Vectors_[name] = value;
}

FVector2 FStyleSet::GetVector2(const std::string& name) const {
    auto it = Vectors_.find(name);
    if (it != Vectors_.end()) {
        return it->second;
    }
    return FVector2::Zero; // 默认返回零向量
}

FVector2 FStyleSet::GetVector2(const std::string& name, const FVector2& defaultValue) const {
    auto it = Vectors_.find(name);
    if (it != Vectors_.end()) {
        return it->second;
    }
    return defaultValue;
}

bool FStyleSet::HasVector2(const std::string& name) const {
    return Vectors_.find(name) != Vectors_.end();
}

void FStyleSet::RemoveVector2(const std::string& name) {
    Vectors_.erase(name);
}

void FStyleSet::Clear() {
    Colors_.clear();
    Floats_.clear();
    Vectors_.clear();
}

void FStyleSet::Merge(const FStyleSet& other) {
    // 合并颜色
    for (const auto& pair : other.Colors_) {
        Colors_[pair.first] = pair.second;
    }

    // 合并浮点值
    for (const auto& pair : other.Floats_) {
        Floats_[pair.first] = pair.second;
    }

    // 合并向量
    for (const auto& pair : other.Vectors_) {
        Vectors_[pair.first] = pair.second;
    }
}

std::vector<std::string> FStyleSet::GetColorKeys() const {
    std::vector<std::string> keys;
    keys.reserve(Colors_.size());
    for (const auto& pair : Colors_) {
        keys.push_back(pair.first);
    }
    return keys;
}

std::vector<std::string> FStyleSet::GetFloatKeys() const {
    std::vector<std::string> keys;
    keys.reserve(Floats_.size());
    for (const auto& pair : Floats_) {
        keys.push_back(pair.first);
    }
    return keys;
}

std::vector<std::string> FStyleSet::GetVector2Keys() const {
    std::vector<std::string> keys;
    keys.reserve(Vectors_.size());
    for (const auto& pair : Vectors_) {
        keys.push_back(pair.first);
    }
    return keys;
}

// ============================================================================
// FStyleSetFactory 实现
// ============================================================================

std::shared_ptr<FStyleSet> FStyleSetFactory::CreateDefault() {
    auto styleSet = std::make_shared<FStyleSet>();

    // 基础颜色
    styleSet->SetColor("Background", FColor::FromBytes(30, 30, 30));
    styleSet->SetColor("Text", FColor::White);
    styleSet->SetColor("Border", FColor::FromBytes(60, 60, 60));
    styleSet->SetColor("Highlight", FColor::FromBytes(100, 150, 255));

    // 按钮颜色
    styleSet->SetColor("Button.Normal", FColor::FromBytes(60, 60, 60));
    styleSet->SetColor("Button.Hovered", FColor::FromBytes(80, 80, 80));
    styleSet->SetColor("Button.Pressed", FColor::FromBytes(50, 50, 50));
    styleSet->SetColor("Button.Text", FColor::White);

    // 基础尺寸
    styleSet->SetFloat("BorderThickness", 1.0f);
    styleSet->SetFloat("CornerRadius", 4.0f);
    styleSet->SetFloat("Padding", 8.0f);

    // 基础向量
    styleSet->SetVector2("DefaultSize", FVector2(100.0f, 30.0f));
    styleSet->SetVector2("DefaultPadding", FVector2(8.0f, 8.0f));

    return styleSet;
}

std::shared_ptr<FStyleSet> FStyleSetFactory::CreateDarkTheme() {
    auto styleSet = std::make_shared<FStyleSet>();

    // 深色主题颜色
    styleSet->SetColor("Background", FColor::FromBytes(20, 20, 20));
    styleSet->SetColor("Text", FColor::FromBytes(220, 220, 220));
    styleSet->SetColor("Border", FColor::FromBytes(50, 50, 50));
    styleSet->SetColor("Highlight", FColor::FromBytes(70, 120, 200));

    // 按钮颜色
    styleSet->SetColor("Button.Normal", FColor::FromBytes(50, 50, 50));
    styleSet->SetColor("Button.Hovered", FColor::FromBytes(70, 70, 70));
    styleSet->SetColor("Button.Pressed", FColor::FromBytes(40, 40, 40));
    styleSet->SetColor("Button.Text", FColor::FromBytes(220, 220, 220));

    // 输入框颜色
    styleSet->SetColor("Input.Background", FColor::FromBytes(30, 30, 30));
    styleSet->SetColor("Input.Border", FColor::FromBytes(60, 60, 60));
    styleSet->SetColor("Input.Text", FColor::FromBytes(220, 220, 220));

    // 基础尺寸
    styleSet->SetFloat("BorderThickness", 1.0f);
    styleSet->SetFloat("CornerRadius", 4.0f);
    styleSet->SetFloat("Padding", 8.0f);

    // 基础向量
    styleSet->SetVector2("DefaultSize", FVector2(100.0f, 30.0f));
    styleSet->SetVector2("DefaultPadding", FVector2(8.0f, 8.0f));

    return styleSet;
}

std::shared_ptr<FStyleSet> FStyleSetFactory::CreateLightTheme() {
    auto styleSet = std::make_shared<FStyleSet>();

    // 浅色主题颜色
    styleSet->SetColor("Background", FColor::FromBytes(240, 240, 240));
    styleSet->SetColor("Text", FColor::FromBytes(30, 30, 30));
    styleSet->SetColor("Border", FColor::FromBytes(200, 200, 200));
    styleSet->SetColor("Highlight", FColor::FromBytes(0, 120, 215));

    // 按钮颜色
    styleSet->SetColor("Button.Normal", FColor::FromBytes(220, 220, 220));
    styleSet->SetColor("Button.Hovered", FColor::FromBytes(200, 200, 200));
    styleSet->SetColor("Button.Pressed", FColor::FromBytes(180, 180, 180));
    styleSet->SetColor("Button.Text", FColor::FromBytes(30, 30, 30));

    // 输入框颜色
    styleSet->SetColor("Input.Background", FColor::White);
    styleSet->SetColor("Input.Border", FColor::FromBytes(200, 200, 200));
    styleSet->SetColor("Input.Text", FColor::FromBytes(30, 30, 30));

    // 基础尺寸
    styleSet->SetFloat("BorderThickness", 1.0f);
    styleSet->SetFloat("CornerRadius", 4.0f);
    styleSet->SetFloat("Padding", 8.0f);

    // 基础向量
    styleSet->SetVector2("DefaultSize", FVector2(100.0f, 30.0f));
    styleSet->SetVector2("DefaultPadding", FVector2(8.0f, 8.0f));

    return styleSet;
}

} // namespace ImWidgetV4
