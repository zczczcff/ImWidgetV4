#include <imwidgetv4/style/StyleSet.h>

namespace ImWidgetV4 {

void FStyleSet::SetColor(const std::string& name, const FColor& color) {
    Colors_[name] = color;
}

FColor FStyleSet::GetColor(const std::string& name) const {
    auto it = Colors_.find(name);
    if (it != Colors_.end()) {
        return it->second;
    }
    return FColor::White;
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
    return 0.0f;
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
    return FVector2::Zero;
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
    for (const auto& pair : other.Colors_) {
        Colors_[pair.first] = pair.second;
    }

    for (const auto& pair : other.Floats_) {
        Floats_[pair.first] = pair.second;
    }

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

namespace {

void PopulateSharedThemeGeometry(FStyleSet& styleSet)
{
    styleSet.SetFloat("BorderThickness", 1.0f);
    styleSet.SetFloat("CornerRadius", 4.0f);
    styleSet.SetFloat("Padding", 8.0f);
    styleSet.SetVector2("DefaultSize", FVector2(100.0f, 30.0f));
    styleSet.SetVector2("DefaultPadding", FVector2(8.0f, 8.0f));

    styleSet.SetFloat("Float.Button.BorderThickness", 1.0f);
    styleSet.SetFloat("Float.Button.Pressed.BorderThickness", 2.0f);
    styleSet.SetFloat("Float.Button.Focused.BorderThickness", 2.0f);

    styleSet.SetFloat("Float.Input.CornerRadius", 7.0f);
    styleSet.SetFloat("Float.Input.BorderThickness", 1.0f);
    styleSet.SetFloat("Float.Input.FontSize", 16.0f);

    styleSet.SetFloat("Float.CheckBox.IndicatorSize", 18.0f);
    styleSet.SetFloat("Float.CheckBox.IndicatorCornerRadius", 4.0f);
    styleSet.SetFloat("Float.CheckBox.LabelSpacing", 10.0f);
    styleSet.SetFloat("Float.CheckBox.BorderThickness", 1.0f);
    styleSet.SetFloat("Float.CheckBox.FontSize", 16.0f);

    styleSet.SetFloat("Float.ComboBox.FontSize", 16.0f);
    styleSet.SetFloat("Float.ComboBox.BorderThickness", 1.0f);
    styleSet.SetFloat("Float.ComboBox.CornerRadius", 7.0f);
    styleSet.SetFloat("Float.ComboBox.ArrowSize", 10.0f);
    styleSet.SetFloat("Float.ComboBox.PopupItemHeight", 30.0f);
    styleSet.SetFloat("Float.ComboBox.PopupMaxVisibleItems", 6.0f);

    styleSet.SetFloat("Float.PopupMenu.FontSize", 14.0f);
    styleSet.SetFloat("Float.PopupMenu.RowHeight", 28.0f);
    styleSet.SetFloat("Float.PopupMenu.IconSize", 18.0f);
    styleSet.SetFloat("Float.PopupMenu.HorizontalPadding", 12.0f);
    styleSet.SetFloat("Float.PopupMenu.IconTextSpacing", 8.0f);
    styleSet.SetFloat("Float.PopupMenu.SubmenuIndicatorSpacing", 12.0f);
    styleSet.SetFloat("Float.PopupMenu.OuterPaddingX", 4.0f);
    styleSet.SetFloat("Float.PopupMenu.OuterPaddingY", 6.0f);
    styleSet.SetFloat("Float.PopupMenu.CornerRadius", 8.0f);
    styleSet.SetFloat("Float.PopupMenu.BorderThickness", 1.0f);

    styleSet.SetFloat("Float.ScrollBox.BorderThickness", 1.0f);
    styleSet.SetFloat("Float.ScrollBox.CornerRadius", 6.0f);
    styleSet.SetFloat("Float.ScrollBox.ScrollbarThickness", 10.0f);
    styleSet.SetFloat("Float.ScrollBox.ScrollbarPadding", 2.0f);
    styleSet.SetFloat("Float.ScrollBox.ThumbMinLength", 28.0f);
    styleSet.SetFloat("Float.ScrollBox.WheelScrollStep", 32.0f);

    styleSet.SetFloat("Float.Switch.BorderThickness", 1.0f);
    styleSet.SetFloat("Float.Switch.ThumbInset", 3.0f);
    styleSet.SetVector2("Vector2.Switch.DesiredSize", FVector2(52.0f, 28.0f));

    styleSet.SetFloat("Float.TabView.TabSpacing", 4.0f);
    styleSet.SetFloat("Float.TabView.TabMinWidth", 96.0f);
    styleSet.SetFloat("Float.TabView.TabHeight", 32.0f);
    styleSet.SetFloat("Float.TabView.IconSize", 16.0f);
    styleSet.SetFloat("Float.TabView.DirtyMarkerRadius", 4.0f);
    styleSet.SetFloat("Float.TabView.CloseButtonSize", 12.0f);
    styleSet.SetFloat("Float.TabView.OverflowButtonWidth", 24.0f);
    styleSet.SetFloat("Float.TabView.FontSize", 14.0f);
    styleSet.SetFloat("Float.TabView.BorderThickness", 1.0f);
    styleSet.SetFloat("Float.TabView.CornerRadius", 6.0f);

    styleSet.SetFloat("Float.TitleBar.Height", 34.0f);
    styleSet.SetFloat("Float.TitleBar.DragRegionMinWidth", 34.0f);
    styleSet.SetFloat("Float.TitleBar.SystemButtonSize", 34.0f);
    styleSet.SetFloat("Float.TitleBar.SystemButtonSpacing", 0.0f);
}

void PopulateDefaultTheme(FStyleSet& styleSet)
{
    styleSet.SetColor("Background", FColor::FromBytes(30, 30, 30));
    styleSet.SetColor("Text", FColor::White);
    styleSet.SetColor("Border", FColor::FromBytes(60, 60, 60));
    styleSet.SetColor("Highlight", FColor::FromBytes(100, 150, 255));

    styleSet.SetColor("Color.Button.Normal.Background", FColor::FromBytes(245, 250, 255));
    styleSet.SetColor("Color.Button.Normal.Border", FColor::FromBytes(200, 220, 240));
    styleSet.SetColor("Color.Button.Normal.Text", FColor::FromBytes(50, 50, 50));
    styleSet.SetColor("Color.Button.Hovered.Background", FColor::FromBytes(210, 230, 250));
    styleSet.SetColor("Color.Button.Hovered.Border", FColor::FromBytes(120, 170, 220));
    styleSet.SetColor("Color.Button.Hovered.Text", FColor::FromBytes(30, 30, 30));
    styleSet.SetColor("Color.Button.Pressed.Background", FColor::FromBytes(190, 220, 245));
    styleSet.SetColor("Color.Button.Pressed.Border", FColor::FromBytes(100, 150, 210));
    styleSet.SetColor("Color.Button.Pressed.Text", FColor::FromBytes(20, 20, 20));
    styleSet.SetColor("Color.Button.Focused.Background", FColor::FromBytes(235, 245, 255));
    styleSet.SetColor("Color.Button.Focused.Border", FColor::FromBytes(0, 120, 215));
    styleSet.SetColor("Color.Button.Focused.Text", FColor::FromBytes(30, 30, 30));
    styleSet.SetColor("Color.Button.Disabled.Background", FColor::FromBytes(240, 240, 240));
    styleSet.SetColor("Color.Button.Disabled.Border", FColor::FromBytes(200, 200, 200));
    styleSet.SetColor("Color.Button.Disabled.Text", FColor::FromBytes(150, 150, 150));
    styleSet.SetFloat("Float.Button.CornerRadius", 0.0f);

    styleSet.SetColor("Color.Input.Background", FColor::FromBytes(31, 37, 46));
    styleSet.SetColor("Color.Input.HoveredBackground", FColor::FromBytes(39, 46, 56));
    styleSet.SetColor("Color.Input.FocusedBackground", FColor::FromBytes(24, 31, 40));
    styleSet.SetColor("Color.Input.DisabledBackground", FColor::FromBytes(56, 60, 66));
    styleSet.SetColor("Color.Input.Border", FColor::FromBytes(16, 19, 23));
    styleSet.SetColor("Color.Input.FocusedOutline", FColor::FromBytes(103, 177, 255));
    styleSet.SetColor("Color.Input.Text", FColor::FromBytes(238, 241, 245));
    styleSet.SetColor("Color.Input.DisabledText", FColor::FromBytes(140, 147, 156));
    styleSet.SetColor("Color.Input.HintText", FColor::FromBytes(135, 145, 157));
    styleSet.SetColor("Color.Input.Caret", FColor::FromBytes(245, 247, 250));
    styleSet.SetColor("Color.Input.SelectionBackground", FColor::FromBytes(93, 149, 212, 176));
    styleSet.SetColor("Color.Input.SelectedText", FColor::FromBytes(248, 250, 252));

    styleSet.SetColor("Color.CheckBox.Background", FColor::FromBytes(31, 37, 46));
    styleSet.SetColor("Color.CheckBox.HoveredBackground", FColor::FromBytes(42, 51, 62));
    styleSet.SetColor("Color.CheckBox.PressedBackground", FColor::FromBytes(23, 29, 37));
    styleSet.SetColor("Color.CheckBox.CheckedBackground", FColor::FromBytes(78, 126, 196));
    styleSet.SetColor("Color.CheckBox.DisabledBackground", FColor::FromBytes(56, 60, 66));
    styleSet.SetColor("Color.CheckBox.Border", FColor::FromBytes(16, 19, 23));
    styleSet.SetColor("Color.CheckBox.CheckMark", FColor::FromBytes(248, 250, 252));
    styleSet.SetColor("Color.CheckBox.Text", FColor::FromBytes(238, 241, 245));
    styleSet.SetColor("Color.CheckBox.DisabledText", FColor::FromBytes(140, 147, 156));
    styleSet.SetColor("Color.CheckBox.FocusedOutline", FColor::FromBytes(103, 177, 255));

    styleSet.SetColor("Color.ComboBox.Background", FColor::FromBytes(31, 37, 46));
    styleSet.SetColor("Color.ComboBox.HoveredBackground", FColor::FromBytes(39, 46, 56));
    styleSet.SetColor("Color.ComboBox.PressedBackground", FColor::FromBytes(24, 31, 40));
    styleSet.SetColor("Color.ComboBox.DisabledBackground", FColor::FromBytes(56, 60, 66));
    styleSet.SetColor("Color.ComboBox.Border", FColor::FromBytes(16, 19, 23));
    styleSet.SetColor("Color.ComboBox.FocusedOutline", FColor::FromBytes(103, 177, 255));
    styleSet.SetColor("Color.ComboBox.Text", FColor::FromBytes(238, 241, 245));
    styleSet.SetColor("Color.ComboBox.PlaceholderText", FColor::FromBytes(135, 145, 157));
    styleSet.SetColor("Color.ComboBox.DisabledText", FColor::FromBytes(140, 147, 156));
    styleSet.SetColor("Color.ComboBox.Arrow", FColor::FromBytes(220, 227, 235));
    styleSet.SetColor("Color.ComboBox.PopupRowHovered", FColor::FromBytes(46, 58, 76));
    styleSet.SetColor("Color.ComboBox.PopupRowSelected", FColor::FromBytes(78, 126, 196));
    styleSet.SetColor("Color.ComboBox.PopupRowSelectedHovered", FColor::FromBytes(96, 149, 221));
    styleSet.SetColor("Color.ComboBox.PopupOutline", FColor::FromBytes(16, 19, 23));

    styleSet.SetColor("Color.PopupMenu.Background", FColor::FromBytes(26, 31, 38));
    styleSet.SetColor("Color.PopupMenu.Border", FColor::FromBytes(63, 73, 89));
    styleSet.SetColor("Color.PopupMenu.RowHovered", FColor::FromBytes(48, 60, 77));
    styleSet.SetColor("Color.PopupMenu.RowPressed", FColor::FromBytes(69, 101, 154));
    styleSet.SetColor("Color.PopupMenu.Text", FColor::FromBytes(238, 242, 247));
    styleSet.SetColor("Color.PopupMenu.DisabledText", FColor::FromBytes(128, 134, 143));
    styleSet.SetColor("Color.PopupMenu.Separator", FColor::FromBytes(57, 66, 80));
    styleSet.SetColor("Color.PopupMenu.SubmenuArrow", FColor::FromBytes(238, 242, 247));

    styleSet.SetColor("Color.ScrollBox.Background", FColor::FromBytes(24, 28, 34));
    styleSet.SetColor("Color.ScrollBox.Border", FColor::FromBytes(16, 19, 23));
    styleSet.SetColor("Color.ScrollBox.ScrollbarTrack", FColor::FromBytes(38, 45, 56));
    styleSet.SetColor("Color.ScrollBox.ScrollbarThumb", FColor::FromBytes(88, 102, 119));
    styleSet.SetColor("Color.ScrollBox.ScrollbarThumbHovered", FColor::FromBytes(122, 143, 168));

    styleSet.SetColor("Color.Switch.OffTrack", FColor::FromBytes(57, 64, 75));
    styleSet.SetColor("Color.Switch.OffTrackHovered", FColor::FromBytes(73, 82, 95));
    styleSet.SetColor("Color.Switch.OffTrackPressed", FColor::FromBytes(43, 49, 58));
    styleSet.SetColor("Color.Switch.OnTrack", FColor::FromBytes(78, 126, 196));
    styleSet.SetColor("Color.Switch.OnTrackHovered", FColor::FromBytes(96, 149, 221));
    styleSet.SetColor("Color.Switch.OnTrackPressed", FColor::FromBytes(63, 108, 177));
    styleSet.SetColor("Color.Switch.DisabledTrack", FColor::FromBytes(74, 79, 87));
    styleSet.SetColor("Color.Switch.Thumb", FColor::FromBytes(248, 250, 252));
    styleSet.SetColor("Color.Switch.ThumbHovered", FColor::White);
    styleSet.SetColor("Color.Switch.ThumbPressed", FColor::FromBytes(255, 214, 102));
    styleSet.SetColor("Color.Switch.DisabledThumb", FColor::FromBytes(170, 176, 184));
    styleSet.SetColor("Color.Switch.Border", FColor::FromBytes(19, 23, 29));
    styleSet.SetColor("Color.Switch.FocusedOutline", FColor::FromBytes(103, 177, 255));

    styleSet.SetColor("Color.TabView.Background", FColor::FromBytes(20, 24, 30));
    styleSet.SetColor("Color.TabView.Border", FColor::FromBytes(16, 19, 23));
    styleSet.SetColor("Color.TabView.FocusedOutline", FColor::FromBytes(103, 177, 255));
    styleSet.SetColor("Color.TabView.TabStripBackground", FColor::FromBytes(26, 31, 39));
    styleSet.SetColor("Color.TabView.Tab", FColor::FromBytes(44, 51, 61));
    styleSet.SetColor("Color.TabView.TabHovered", FColor::FromBytes(56, 66, 80));
    styleSet.SetColor("Color.TabView.TabPressed", FColor::FromBytes(35, 43, 52));
    styleSet.SetColor("Color.TabView.ActiveTab", FColor::FromBytes(64, 88, 123));
    styleSet.SetColor("Color.TabView.DisabledTab", FColor::FromBytes(39, 44, 51));
    styleSet.SetColor("Color.TabView.Text", FColor::FromBytes(214, 222, 234));
    styleSet.SetColor("Color.TabView.ActiveText", FColor::White);
    styleSet.SetColor("Color.TabView.DisabledText", FColor::FromBytes(126, 132, 141));
    styleSet.SetColor("Color.TabView.TabBorder", FColor::FromBytes(18, 22, 28));
    styleSet.SetColor("Color.TabView.DirtyMarker", FColor::FromBytes(255, 196, 84));
    styleSet.SetColor("Color.TabView.CloseButton", FColor::FromBytes(182, 190, 202));
    styleSet.SetColor("Color.TabView.CloseButtonHovered", FColor::White);
    styleSet.SetColor("Color.TabView.CloseButtonPressed", FColor::FromBytes(255, 214, 102));
    styleSet.SetColor("Color.TabView.OverflowButton", FColor::FromBytes(88, 102, 119));
    styleSet.SetColor("Color.TabView.OverflowButtonHovered", FColor::FromBytes(122, 143, 168));
    styleSet.SetColor("Color.TabView.OverflowButtonPressed", FColor::FromBytes(156, 182, 212));
    styleSet.SetColor("Color.TabView.OverflowButtonDisabled", FColor::FromBytes(72, 78, 86));

    styleSet.SetColor("Color.TitleBar.Background", FColor::FromBytes(28, 33, 41));
    styleSet.SetColor("Color.TitleBar.Border", FColor::FromBytes(16, 19, 24));
    styleSet.SetColor("Color.TitleBar.SystemButton.Hovered", FColor::FromBytes(255, 255, 255, 24));
    styleSet.SetColor("Color.TitleBar.SystemButton.Pressed", FColor::FromBytes(255, 255, 255, 40));
    styleSet.SetColor("Color.TitleBar.CloseButton.Hovered", FColor::FromBytes(212, 58, 76, 224));
    styleSet.SetColor("Color.TitleBar.CloseButton.Pressed", FColor::FromBytes(188, 46, 66, 240));
}

void PopulateDarkTheme(FStyleSet& styleSet)
{
    styleSet.SetColor("Background", FColor::FromBytes(20, 20, 20));
    styleSet.SetColor("Text", FColor::FromBytes(220, 220, 220));
    styleSet.SetColor("Border", FColor::FromBytes(50, 50, 50));
    styleSet.SetColor("Highlight", FColor::FromBytes(70, 120, 200));

    styleSet.SetColor("Color.Button.Normal.Background", FColor::FromBytes(50, 50, 50));
    styleSet.SetColor("Color.Button.Normal.Border", FColor::FromBytes(72, 72, 72));
    styleSet.SetColor("Color.Button.Normal.Text", FColor::FromBytes(220, 220, 220));
    styleSet.SetColor("Color.Button.Hovered.Background", FColor::FromBytes(70, 70, 70));
    styleSet.SetColor("Color.Button.Hovered.Border", FColor::FromBytes(102, 102, 102));
    styleSet.SetColor("Color.Button.Hovered.Text", FColor::FromBytes(236, 236, 236));
    styleSet.SetColor("Color.Button.Pressed.Background", FColor::FromBytes(40, 40, 40));
    styleSet.SetColor("Color.Button.Pressed.Border", FColor::FromBytes(86, 120, 182));
    styleSet.SetColor("Color.Button.Pressed.Text", FColor::FromBytes(250, 250, 250));
    styleSet.SetColor("Color.Button.Focused.Background", FColor::FromBytes(58, 58, 58));
    styleSet.SetColor("Color.Button.Focused.Border", FColor::FromBytes(103, 177, 255));
    styleSet.SetColor("Color.Button.Focused.Text", FColor::FromBytes(244, 244, 244));
    styleSet.SetColor("Color.Button.Disabled.Background", FColor::FromBytes(44, 44, 44));
    styleSet.SetColor("Color.Button.Disabled.Border", FColor::FromBytes(62, 62, 62));
    styleSet.SetColor("Color.Button.Disabled.Text", FColor::FromBytes(120, 120, 120));
    styleSet.SetFloat("Float.Button.CornerRadius", 4.0f);

    styleSet.SetColor("Color.Input.Background", FColor::FromBytes(30, 30, 30));
    styleSet.SetColor("Color.Input.HoveredBackground", FColor::FromBytes(38, 38, 38));
    styleSet.SetColor("Color.Input.FocusedBackground", FColor::FromBytes(24, 24, 24));
    styleSet.SetColor("Color.Input.DisabledBackground", FColor::FromBytes(45, 45, 45));
    styleSet.SetColor("Color.Input.Border", FColor::FromBytes(60, 60, 60));
    styleSet.SetColor("Color.Input.FocusedOutline", FColor::FromBytes(103, 177, 255));
    styleSet.SetColor("Color.Input.Text", FColor::FromBytes(220, 220, 220));
    styleSet.SetColor("Color.Input.DisabledText", FColor::FromBytes(130, 130, 130));
    styleSet.SetColor("Color.Input.HintText", FColor::FromBytes(144, 144, 144));
    styleSet.SetColor("Color.Input.Caret", FColor::FromBytes(240, 240, 240));
    styleSet.SetColor("Color.Input.SelectionBackground", FColor::FromBytes(78, 120, 184, 180));
    styleSet.SetColor("Color.Input.SelectedText", FColor::FromBytes(250, 250, 250));

    styleSet.SetColor("Color.CheckBox.Background", FColor::FromBytes(36, 36, 36));
    styleSet.SetColor("Color.CheckBox.HoveredBackground", FColor::FromBytes(52, 52, 52));
    styleSet.SetColor("Color.CheckBox.PressedBackground", FColor::FromBytes(28, 28, 28));
    styleSet.SetColor("Color.CheckBox.CheckedBackground", FColor::FromBytes(86, 120, 182));
    styleSet.SetColor("Color.CheckBox.DisabledBackground", FColor::FromBytes(44, 44, 44));
    styleSet.SetColor("Color.CheckBox.Border", FColor::FromBytes(60, 60, 60));
    styleSet.SetColor("Color.CheckBox.CheckMark", FColor::FromBytes(250, 250, 250));
    styleSet.SetColor("Color.CheckBox.Text", FColor::FromBytes(220, 220, 220));
    styleSet.SetColor("Color.CheckBox.DisabledText", FColor::FromBytes(120, 120, 120));
    styleSet.SetColor("Color.CheckBox.FocusedOutline", FColor::FromBytes(103, 177, 255));

    styleSet.SetColor("Color.ComboBox.Background", FColor::FromBytes(30, 30, 30));
    styleSet.SetColor("Color.ComboBox.HoveredBackground", FColor::FromBytes(38, 38, 38));
    styleSet.SetColor("Color.ComboBox.PressedBackground", FColor::FromBytes(24, 24, 24));
    styleSet.SetColor("Color.ComboBox.DisabledBackground", FColor::FromBytes(45, 45, 45));
    styleSet.SetColor("Color.ComboBox.Border", FColor::FromBytes(60, 60, 60));
    styleSet.SetColor("Color.ComboBox.FocusedOutline", FColor::FromBytes(103, 177, 255));
    styleSet.SetColor("Color.ComboBox.Text", FColor::FromBytes(220, 220, 220));
    styleSet.SetColor("Color.ComboBox.PlaceholderText", FColor::FromBytes(144, 144, 144));
    styleSet.SetColor("Color.ComboBox.DisabledText", FColor::FromBytes(130, 130, 130));
    styleSet.SetColor("Color.ComboBox.Arrow", FColor::FromBytes(236, 236, 236));
    styleSet.SetColor("Color.ComboBox.PopupRowHovered", FColor::FromBytes(60, 60, 60));
    styleSet.SetColor("Color.ComboBox.PopupRowSelected", FColor::FromBytes(86, 120, 182));
    styleSet.SetColor("Color.ComboBox.PopupRowSelectedHovered", FColor::FromBytes(103, 141, 205));
    styleSet.SetColor("Color.ComboBox.PopupOutline", FColor::FromBytes(72, 72, 72));

    styleSet.SetColor("Color.PopupMenu.Background", FColor::FromBytes(34, 34, 34));
    styleSet.SetColor("Color.PopupMenu.Border", FColor::FromBytes(66, 66, 66));
    styleSet.SetColor("Color.PopupMenu.RowHovered", FColor::FromBytes(58, 58, 58));
    styleSet.SetColor("Color.PopupMenu.RowPressed", FColor::FromBytes(86, 120, 182));
    styleSet.SetColor("Color.PopupMenu.Text", FColor::FromBytes(236, 236, 236));
    styleSet.SetColor("Color.PopupMenu.DisabledText", FColor::FromBytes(130, 130, 130));
    styleSet.SetColor("Color.PopupMenu.Separator", FColor::FromBytes(72, 72, 72));
    styleSet.SetColor("Color.PopupMenu.SubmenuArrow", FColor::FromBytes(236, 236, 236));

    styleSet.SetColor("Color.ScrollBox.Background", FColor::FromBytes(28, 28, 28));
    styleSet.SetColor("Color.ScrollBox.Border", FColor::FromBytes(60, 60, 60));
    styleSet.SetColor("Color.ScrollBox.ScrollbarTrack", FColor::FromBytes(42, 42, 42));
    styleSet.SetColor("Color.ScrollBox.ScrollbarThumb", FColor::FromBytes(92, 92, 92));
    styleSet.SetColor("Color.ScrollBox.ScrollbarThumbHovered", FColor::FromBytes(122, 122, 122));

    styleSet.SetColor("Color.Switch.OffTrack", FColor::FromBytes(66, 66, 66));
    styleSet.SetColor("Color.Switch.OffTrackHovered", FColor::FromBytes(82, 82, 82));
    styleSet.SetColor("Color.Switch.OffTrackPressed", FColor::FromBytes(54, 54, 54));
    styleSet.SetColor("Color.Switch.OnTrack", FColor::FromBytes(86, 120, 182));
    styleSet.SetColor("Color.Switch.OnTrackHovered", FColor::FromBytes(103, 141, 205));
    styleSet.SetColor("Color.Switch.OnTrackPressed", FColor::FromBytes(74, 108, 166));
    styleSet.SetColor("Color.Switch.DisabledTrack", FColor::FromBytes(48, 48, 48));
    styleSet.SetColor("Color.Switch.Thumb", FColor::FromBytes(244, 244, 244));
    styleSet.SetColor("Color.Switch.ThumbHovered", FColor::White);
    styleSet.SetColor("Color.Switch.ThumbPressed", FColor::FromBytes(255, 214, 102));
    styleSet.SetColor("Color.Switch.DisabledThumb", FColor::FromBytes(144, 144, 144));
    styleSet.SetColor("Color.Switch.Border", FColor::FromBytes(60, 60, 60));
    styleSet.SetColor("Color.Switch.FocusedOutline", FColor::FromBytes(103, 177, 255));

    styleSet.SetColor("Color.TabView.Background", FColor::FromBytes(24, 24, 24));
    styleSet.SetColor("Color.TabView.Border", FColor::FromBytes(52, 52, 52));
    styleSet.SetColor("Color.TabView.FocusedOutline", FColor::FromBytes(103, 177, 255));
    styleSet.SetColor("Color.TabView.TabStripBackground", FColor::FromBytes(32, 32, 32));
    styleSet.SetColor("Color.TabView.Tab", FColor::FromBytes(48, 48, 48));
    styleSet.SetColor("Color.TabView.TabHovered", FColor::FromBytes(62, 62, 62));
    styleSet.SetColor("Color.TabView.TabPressed", FColor::FromBytes(40, 40, 40));
    styleSet.SetColor("Color.TabView.ActiveTab", FColor::FromBytes(70, 88, 122));
    styleSet.SetColor("Color.TabView.DisabledTab", FColor::FromBytes(40, 40, 40));
    styleSet.SetColor("Color.TabView.Text", FColor::FromBytes(220, 220, 220));
    styleSet.SetColor("Color.TabView.ActiveText", FColor::FromBytes(248, 248, 248));
    styleSet.SetColor("Color.TabView.DisabledText", FColor::FromBytes(120, 120, 120));
    styleSet.SetColor("Color.TabView.TabBorder", FColor::FromBytes(72, 72, 72));
    styleSet.SetColor("Color.TabView.DirtyMarker", FColor::FromBytes(255, 196, 84));
    styleSet.SetColor("Color.TabView.CloseButton", FColor::FromBytes(206, 206, 206));
    styleSet.SetColor("Color.TabView.CloseButtonHovered", FColor::White);
    styleSet.SetColor("Color.TabView.CloseButtonPressed", FColor::FromBytes(255, 214, 102));
    styleSet.SetColor("Color.TabView.OverflowButton", FColor::FromBytes(156, 156, 156));
    styleSet.SetColor("Color.TabView.OverflowButtonHovered", FColor::FromBytes(220, 220, 220));
    styleSet.SetColor("Color.TabView.OverflowButtonPressed", FColor::FromBytes(255, 214, 102));
    styleSet.SetColor("Color.TabView.OverflowButtonDisabled", FColor::FromBytes(96, 96, 96));

    styleSet.SetColor("Color.TitleBar.Background", FColor::FromBytes(24, 24, 24));
    styleSet.SetColor("Color.TitleBar.Border", FColor::FromBytes(52, 52, 52));
    styleSet.SetColor("Color.TitleBar.SystemButton.Hovered", FColor::FromBytes(255, 255, 255, 28));
    styleSet.SetColor("Color.TitleBar.SystemButton.Pressed", FColor::FromBytes(255, 255, 255, 46));
    styleSet.SetColor("Color.TitleBar.CloseButton.Hovered", FColor::FromBytes(212, 58, 76, 224));
    styleSet.SetColor("Color.TitleBar.CloseButton.Pressed", FColor::FromBytes(188, 46, 66, 240));
}

void PopulateLightTheme(FStyleSet& styleSet)
{
    styleSet.SetColor("Background", FColor::FromBytes(240, 240, 240));
    styleSet.SetColor("Text", FColor::FromBytes(30, 30, 30));
    styleSet.SetColor("Border", FColor::FromBytes(200, 200, 200));
    styleSet.SetColor("Highlight", FColor::FromBytes(0, 120, 215));

    styleSet.SetColor("Color.Button.Normal.Background", FColor::FromBytes(220, 220, 220));
    styleSet.SetColor("Color.Button.Normal.Border", FColor::FromBytes(190, 190, 190));
    styleSet.SetColor("Color.Button.Normal.Text", FColor::FromBytes(30, 30, 30));
    styleSet.SetColor("Color.Button.Hovered.Background", FColor::FromBytes(200, 200, 200));
    styleSet.SetColor("Color.Button.Hovered.Border", FColor::FromBytes(168, 168, 168));
    styleSet.SetColor("Color.Button.Hovered.Text", FColor::FromBytes(20, 20, 20));
    styleSet.SetColor("Color.Button.Pressed.Background", FColor::FromBytes(180, 180, 180));
    styleSet.SetColor("Color.Button.Pressed.Border", FColor::FromBytes(0, 120, 215));
    styleSet.SetColor("Color.Button.Pressed.Text", FColor::FromBytes(20, 20, 20));
    styleSet.SetColor("Color.Button.Focused.Background", FColor::FromBytes(212, 226, 248));
    styleSet.SetColor("Color.Button.Focused.Border", FColor::FromBytes(0, 120, 215));
    styleSet.SetColor("Color.Button.Focused.Text", FColor::FromBytes(20, 20, 20));
    styleSet.SetColor("Color.Button.Disabled.Background", FColor::FromBytes(236, 236, 236));
    styleSet.SetColor("Color.Button.Disabled.Border", FColor::FromBytes(214, 214, 214));
    styleSet.SetColor("Color.Button.Disabled.Text", FColor::FromBytes(150, 150, 150));
    styleSet.SetFloat("Float.Button.CornerRadius", 4.0f);

    styleSet.SetColor("Color.Input.Background", FColor::White);
    styleSet.SetColor("Color.Input.HoveredBackground", FColor::FromBytes(248, 248, 248));
    styleSet.SetColor("Color.Input.FocusedBackground", FColor::White);
    styleSet.SetColor("Color.Input.DisabledBackground", FColor::FromBytes(240, 240, 240));
    styleSet.SetColor("Color.Input.Border", FColor::FromBytes(200, 200, 200));
    styleSet.SetColor("Color.Input.FocusedOutline", FColor::FromBytes(0, 120, 215));
    styleSet.SetColor("Color.Input.Text", FColor::FromBytes(30, 30, 30));
    styleSet.SetColor("Color.Input.DisabledText", FColor::FromBytes(140, 140, 140));
    styleSet.SetColor("Color.Input.HintText", FColor::FromBytes(132, 132, 132));
    styleSet.SetColor("Color.Input.Caret", FColor::FromBytes(20, 20, 20));
    styleSet.SetColor("Color.Input.SelectionBackground", FColor::FromBytes(162, 205, 255, 200));
    styleSet.SetColor("Color.Input.SelectedText", FColor::FromBytes(20, 20, 20));

    styleSet.SetColor("Color.CheckBox.Background", FColor::White);
    styleSet.SetColor("Color.CheckBox.HoveredBackground", FColor::FromBytes(248, 248, 248));
    styleSet.SetColor("Color.CheckBox.PressedBackground", FColor::FromBytes(236, 236, 236));
    styleSet.SetColor("Color.CheckBox.CheckedBackground", FColor::FromBytes(0, 120, 215));
    styleSet.SetColor("Color.CheckBox.DisabledBackground", FColor::FromBytes(240, 240, 240));
    styleSet.SetColor("Color.CheckBox.Border", FColor::FromBytes(200, 200, 200));
    styleSet.SetColor("Color.CheckBox.CheckMark", FColor::FromBytes(250, 250, 250));
    styleSet.SetColor("Color.CheckBox.Text", FColor::FromBytes(30, 30, 30));
    styleSet.SetColor("Color.CheckBox.DisabledText", FColor::FromBytes(140, 140, 140));
    styleSet.SetColor("Color.CheckBox.FocusedOutline", FColor::FromBytes(0, 120, 215));

    styleSet.SetColor("Color.ComboBox.Background", FColor::White);
    styleSet.SetColor("Color.ComboBox.HoveredBackground", FColor::FromBytes(248, 248, 248));
    styleSet.SetColor("Color.ComboBox.PressedBackground", FColor::FromBytes(236, 236, 236));
    styleSet.SetColor("Color.ComboBox.DisabledBackground", FColor::FromBytes(240, 240, 240));
    styleSet.SetColor("Color.ComboBox.Border", FColor::FromBytes(200, 200, 200));
    styleSet.SetColor("Color.ComboBox.FocusedOutline", FColor::FromBytes(0, 120, 215));
    styleSet.SetColor("Color.ComboBox.Text", FColor::FromBytes(30, 30, 30));
    styleSet.SetColor("Color.ComboBox.PlaceholderText", FColor::FromBytes(132, 132, 132));
    styleSet.SetColor("Color.ComboBox.DisabledText", FColor::FromBytes(140, 140, 140));
    styleSet.SetColor("Color.ComboBox.Arrow", FColor::FromBytes(80, 80, 80));
    styleSet.SetColor("Color.ComboBox.PopupRowHovered", FColor::FromBytes(232, 240, 250));
    styleSet.SetColor("Color.ComboBox.PopupRowSelected", FColor::FromBytes(0, 120, 215));
    styleSet.SetColor("Color.ComboBox.PopupRowSelectedHovered", FColor::FromBytes(32, 138, 226));
    styleSet.SetColor("Color.ComboBox.PopupOutline", FColor::FromBytes(200, 200, 200));

    styleSet.SetColor("Color.PopupMenu.Background", FColor::White);
    styleSet.SetColor("Color.PopupMenu.Border", FColor::FromBytes(208, 212, 218));
    styleSet.SetColor("Color.PopupMenu.RowHovered", FColor::FromBytes(234, 242, 252));
    styleSet.SetColor("Color.PopupMenu.RowPressed", FColor::FromBytes(214, 229, 247));
    styleSet.SetColor("Color.PopupMenu.Text", FColor::FromBytes(26, 26, 26));
    styleSet.SetColor("Color.PopupMenu.DisabledText", FColor::FromBytes(150, 150, 150));
    styleSet.SetColor("Color.PopupMenu.Separator", FColor::FromBytes(224, 228, 234));
    styleSet.SetColor("Color.PopupMenu.SubmenuArrow", FColor::FromBytes(56, 56, 56));

    styleSet.SetColor("Color.ScrollBox.Background", FColor::White);
    styleSet.SetColor("Color.ScrollBox.Border", FColor::FromBytes(200, 200, 200));
    styleSet.SetColor("Color.ScrollBox.ScrollbarTrack", FColor::FromBytes(232, 236, 241));
    styleSet.SetColor("Color.ScrollBox.ScrollbarThumb", FColor::FromBytes(170, 178, 188));
    styleSet.SetColor("Color.ScrollBox.ScrollbarThumbHovered", FColor::FromBytes(132, 146, 164));

    styleSet.SetColor("Color.Switch.OffTrack", FColor::FromBytes(196, 202, 210));
    styleSet.SetColor("Color.Switch.OffTrackHovered", FColor::FromBytes(182, 190, 200));
    styleSet.SetColor("Color.Switch.OffTrackPressed", FColor::FromBytes(170, 178, 188));
    styleSet.SetColor("Color.Switch.OnTrack", FColor::FromBytes(0, 120, 215));
    styleSet.SetColor("Color.Switch.OnTrackHovered", FColor::FromBytes(32, 138, 226));
    styleSet.SetColor("Color.Switch.OnTrackPressed", FColor::FromBytes(0, 102, 187));
    styleSet.SetColor("Color.Switch.DisabledTrack", FColor::FromBytes(224, 228, 232));
    styleSet.SetColor("Color.Switch.Thumb", FColor::White);
    styleSet.SetColor("Color.Switch.ThumbHovered", FColor::White);
    styleSet.SetColor("Color.Switch.ThumbPressed", FColor::FromBytes(255, 214, 102));
    styleSet.SetColor("Color.Switch.DisabledThumb", FColor::FromBytes(180, 186, 192));
    styleSet.SetColor("Color.Switch.Border", FColor::FromBytes(200, 200, 200));
    styleSet.SetColor("Color.Switch.FocusedOutline", FColor::FromBytes(0, 120, 215));

    styleSet.SetColor("Color.TabView.Background", FColor::FromBytes(248, 249, 251));
    styleSet.SetColor("Color.TabView.Border", FColor::FromBytes(210, 214, 220));
    styleSet.SetColor("Color.TabView.FocusedOutline", FColor::FromBytes(0, 120, 215));
    styleSet.SetColor("Color.TabView.TabStripBackground", FColor::FromBytes(238, 240, 244));
    styleSet.SetColor("Color.TabView.Tab", FColor::FromBytes(230, 233, 238));
    styleSet.SetColor("Color.TabView.TabHovered", FColor::FromBytes(220, 226, 234));
    styleSet.SetColor("Color.TabView.TabPressed", FColor::FromBytes(212, 219, 228));
    styleSet.SetColor("Color.TabView.ActiveTab", FColor::White);
    styleSet.SetColor("Color.TabView.DisabledTab", FColor::FromBytes(236, 238, 242));
    styleSet.SetColor("Color.TabView.Text", FColor::FromBytes(36, 36, 36));
    styleSet.SetColor("Color.TabView.ActiveText", FColor::FromBytes(20, 20, 20));
    styleSet.SetColor("Color.TabView.DisabledText", FColor::FromBytes(140, 140, 140));
    styleSet.SetColor("Color.TabView.TabBorder", FColor::FromBytes(200, 204, 210));
    styleSet.SetColor("Color.TabView.DirtyMarker", FColor::FromBytes(212, 132, 24));
    styleSet.SetColor("Color.TabView.CloseButton", FColor::FromBytes(90, 90, 90));
    styleSet.SetColor("Color.TabView.CloseButtonHovered", FColor::FromBytes(20, 20, 20));
    styleSet.SetColor("Color.TabView.CloseButtonPressed", FColor::FromBytes(212, 132, 24));
    styleSet.SetColor("Color.TabView.OverflowButton", FColor::FromBytes(110, 118, 126));
    styleSet.SetColor("Color.TabView.OverflowButtonHovered", FColor::FromBytes(72, 82, 92));
    styleSet.SetColor("Color.TabView.OverflowButtonPressed", FColor::FromBytes(40, 52, 66));
    styleSet.SetColor("Color.TabView.OverflowButtonDisabled", FColor::FromBytes(170, 176, 182));

    styleSet.SetColor("Color.TitleBar.Background", FColor::FromBytes(246, 247, 249));
    styleSet.SetColor("Color.TitleBar.Border", FColor::FromBytes(210, 214, 220));
    styleSet.SetColor("Color.TitleBar.SystemButton.Hovered", FColor::FromBytes(0, 0, 0, 18));
    styleSet.SetColor("Color.TitleBar.SystemButton.Pressed", FColor::FromBytes(0, 0, 0, 30));
    styleSet.SetColor("Color.TitleBar.CloseButton.Hovered", FColor::FromBytes(212, 58, 76, 224));
    styleSet.SetColor("Color.TitleBar.CloseButton.Pressed", FColor::FromBytes(188, 46, 66, 240));
}

} // namespace

std::shared_ptr<FStyleSet> FStyleSetFactory::CreateDefault() {
    auto styleSet = std::make_shared<FStyleSet>();
    PopulateSharedThemeGeometry(*styleSet);
    PopulateDefaultTheme(*styleSet);
    return styleSet;
}

std::shared_ptr<FStyleSet> FStyleSetFactory::CreateDarkTheme() {
    auto styleSet = std::make_shared<FStyleSet>();
    PopulateSharedThemeGeometry(*styleSet);
    PopulateDarkTheme(*styleSet);
    return styleSet;
}

std::shared_ptr<FStyleSet> FStyleSetFactory::CreateLightTheme() {
    auto styleSet = std::make_shared<FStyleSet>();
    PopulateSharedThemeGeometry(*styleSet);
    PopulateLightTheme(*styleSet);
    return styleSet;
}

} // namespace ImWidgetV4
