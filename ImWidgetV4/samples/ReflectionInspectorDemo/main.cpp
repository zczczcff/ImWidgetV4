#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/core/ReflectableObject.h>
#include <imwidgetv4/core/WindowManager.h>
#include <imwidgetv4/widgets/BoxSlot.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/ExpandableBox.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/HorizontalSplitter.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/Slider.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <imwidgetv4/widgets/VerticalSplitter.h>
#include "../DemoPaths.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace ImWidgetV4;

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

namespace {

std::string TrimCopy(const std::string& text)
{
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }

    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::vector<std::string> SplitCommaSeparated(const std::string& text)
{
    std::vector<std::string> tokens;
    std::stringstream stream(text);
    std::string item;

    while (std::getline(stream, item, ',')) {
        tokens.push_back(TrimCopy(item));
    }

    return tokens;
}

std::string FormatFloat(float value)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
}

std::string FormatInt(int value)
{
    return std::to_string(value);
}

std::string FormatBool(bool value)
{
    return value ? "true" : "false";
}

std::string FormatColor(const FColor& color)
{
    const int r = static_cast<int>(std::round(color.R * 255.0f));
    const int g = static_cast<int>(std::round(color.G * 255.0f));
    const int b = static_cast<int>(std::round(color.B * 255.0f));
    const int a = static_cast<int>(std::round(color.A * 255.0f));
    return std::to_string(r) + ", " + std::to_string(g) + ", " + std::to_string(b) + ", " + std::to_string(a);
}

std::string FormatVec2(const FVector2& value)
{
    return FormatFloat(value.X) + ", " + FormatFloat(value.Y);
}

std::string FormatStringArray(const std::vector<std::string>& values)
{
    if (values.empty()) {
        return "(empty)";
    }

    std::ostringstream stream;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0) {
            stream << "\n";
        }
        stream << "- " << values[index];
    }

    return stream.str();
}

bool TryParseInt(const std::string& text, int& outValue)
{
    try {
        std::size_t consumed = 0;
        const int value = std::stoi(TrimCopy(text), &consumed);
        if (consumed != TrimCopy(text).size()) {
            return false;
        }
        outValue = value;
        return true;
    } catch (...) {
        return false;
    }
}

bool TryParseFloat(const std::string& text, float& outValue)
{
    try {
        std::size_t consumed = 0;
        const float value = std::stof(TrimCopy(text), &consumed);
        if (consumed != TrimCopy(text).size()) {
            return false;
        }
        outValue = value;
        return true;
    } catch (...) {
        return false;
    }
}

bool TryParseColor(const std::string& text, FColor& outColor)
{
    const std::vector<std::string> tokens = SplitCommaSeparated(text);
    if (tokens.size() != 4) {
        return false;
    }

    int rgba[4] = {};
    for (int index = 0; index < 4; ++index) {
        if (!TryParseInt(tokens[index], rgba[index])) {
            return false;
        }

        rgba[index] = std::clamp(rgba[index], 0, 255);
    }

    outColor = FColor::FromBytes(rgba[0], rgba[1], rgba[2], rgba[3]);
    return true;
}

bool TryParseVec2(const std::string& text, FVector2& outValue)
{
    const std::vector<std::string> tokens = SplitCommaSeparated(text);
    if (tokens.size() != 2) {
        return false;
    }

    float values[2] = {};
    for (int index = 0; index < 2; ++index) {
        if (!TryParseFloat(tokens[index], values[index])) {
            return false;
        }
    }

    outValue = FVector2(values[0], values[1]);
    return true;
}

std::shared_ptr<ImTextBlock> MakeLabel(const std::string& text, float fontSize = 15.0f, const FColor& color = FColor::FromBytes(230, 235, 242))
{
    auto label = std::make_shared<ImTextBlock>();
    label->SetText(text);
    label->SetFontSize(fontSize);
    label->SetTextColor(color);
    return label;
}

class ImSelectableSampleCard : public ImPanelWidget {
public:
    using FSelectCallback = std::function<void(const std::shared_ptr<ReflectableObject>&)>;

    explicit ImSelectableSampleCard(FSelectCallback onSelected)
        : OnSelected_(std::move(onSelected))
    {
    }

    void SetContent(const Ptr& child)
    {
        ClearChildren();
        if (child) {
            AddSlot(child);
        }
    }

    void SetSelectionTarget(const std::shared_ptr<ReflectableObject>& target)
    {
        SelectionTarget_ = target;
    }

    std::shared_ptr<ReflectableObject> GetSelectionTarget() const
    {
        if (auto target = SelectionTarget_.lock()) {
            return target;
        }

        const auto& children = GetChildren();
        return children.empty() ? nullptr : std::static_pointer_cast<ReflectableObject>(children.front());
    }

    void SetSelected(bool bSelected)
    {
        if (bSelected_ == bSelected) {
            return;
        }

        bSelected_ = bSelected;
        Invalidate(EInvalidateReason::Paint);
    }

    std::unique_ptr<ImSlot> CreateSlot() override
    {
        auto slot = std::make_unique<ImPaddingSlot>();
        slot->PaddingLeft = 14.0f;
        slot->PaddingRight = 14.0f;
        slot->PaddingTop = 12.0f;
        slot->PaddingBottom = 12.0f;
        return slot;
    }

    void Paint(const FPaintContext& paintContext) override
    {
        const FColor background = bSelected_
            ? FColor::FromBytes(34, 52, 78)
            : FColor::FromBytes(23, 28, 35);
        const FColor border = bSelected_
            ? FColor::FromBytes(103, 177, 255)
            : FColor::FromBytes(58, 69, 84);

        paintContext.DrawContext_.DrawRectFilled(
            m_Geometry.GetMin(),
            m_Geometry.GetMax(),
            background,
            10.0f);
        paintContext.DrawContext_.DrawRect(
            m_Geometry.GetMin(),
            m_Geometry.GetMax(),
            border,
            10.0f,
            bSelected_ ? 2.0f : 1.0f);

        if (auto slot = dynamic_cast<ImPaddingSlot*>(GetSlotAt(0))) {
            const auto& children = GetChildren();
            if (!children.empty()) {
                slot->SetSlotPosition(m_Geometry.Position);
                slot->SetSlotSize(m_Geometry.Size);
                slot->ApplyLayout(children.front().get());
            }
        }

        RenderChildren(paintContext);
    }

    FVector2 GetMinSize() const override
    {
        const auto& children = GetChildren();
        const auto* slot = dynamic_cast<const ImPaddingSlot*>(GetSlotAt(0));
        FVector2 childSize = children.empty() ? FVector2(120.0f, 40.0f) : children.front()->GetMinSize();

        if (slot) {
            childSize.X += slot->PaddingLeft + slot->PaddingRight;
            childSize.Y += slot->PaddingTop + slot->PaddingBottom;
        }

        childSize.X = std::max(childSize.X, 200.0f);
        return childSize;
    }

    FReply OnPreviewInputEvent(const FInputEvent& event) override
    {
        if (event.Type == EInputEventType::MouseButtonDown &&
            event.MouseButton == EMouseButton::Left &&
            m_Geometry.Contains(event.MousePosition)) {
            if (OnSelected_) {
                OnSelected_(GetSelectionTarget());
            }
        }

        return FReply::Unhandled();
    }

private:
    FSelectCallback OnSelected_;
    std::weak_ptr<ReflectableObject> SelectionTarget_;
    bool bSelected_ = false;
};

class FPropertyInspectorController {
public:
    explicit FPropertyInspectorController(const std::shared_ptr<ImTextBlock>& statusText)
        : StatusText_(statusText)
    {
    }

    void SetInspectorWindow(const std::shared_ptr<ImWindow>& inspectorWindow)
    {
        InspectorWindow_ = inspectorWindow;
        RebuildInspector();
    }

    void RegisterSampleCard(const std::shared_ptr<ImSelectableSampleCard>& sampleCard)
    {
        SampleCards_.push_back(sampleCard);
    }

    void SelectObject(const std::shared_ptr<ReflectableObject>& object)
    {
        SelectedObject_ = object;
        UpdateSelectionVisuals();

        if (StatusText_) {
            if (object) {
                StatusText_->SetText("Selected: " + object->GetClassName());
            } else {
                StatusText_->SetText("Selected: none");
            }
        }

        RebuildInspector();
    }

private:
    void UpdateSelectionVisuals()
    {
        for (const auto& card : SampleCards_) {
            if (!card) {
                continue;
            }

            card->SetSelected(card->GetSelectionTarget() == SelectedObject_);
        }
    }

    std::shared_ptr<ImWidget> BuildPropertyEditor(ReflectableObject::ROPProperty prop)
    {
        const auto type = prop.GetType();

        if (type == PropertyType::Bool) {
            auto editor = std::make_shared<ImCheckBox>();
            editor->SetLabel("");
            editor->SetChecked(prop.GetValue<bool>());
            editor->OnCheckStateChanged.AddLambda([prop](ImCheckBox&, bool checked) mutable {
                prop.SetValue<bool>(checked);
            });
            return editor;
        }

        if (type == PropertyType::String) {
            auto editor = std::make_shared<ImEditableText>();
            editor->SetText(prop.GetValue<std::string>());
            editor->OnTextCommitted.AddLambda([prop, weakEditor = std::weak_ptr<ImEditableText>(editor)](ImEditableText&, const std::string& text) mutable {
                prop.SetValue<std::string>(text);
                if (auto locked = weakEditor.lock()) {
                    locked->SetText(prop.GetValue<std::string>());
                }
            });
            return editor;
        }

        if (type == PropertyType::Int) {
            auto editor = std::make_shared<ImEditableText>();
            editor->SetText(FormatInt(prop.GetValue<int>()));
            editor->OnTextCommitted.AddLambda([prop, weakEditor = std::weak_ptr<ImEditableText>(editor)](ImEditableText&, const std::string& text) mutable {
                int value = 0;
                if (TryParseInt(text, value)) {
                    prop.SetValue<int>(value);
                }
                if (auto locked = weakEditor.lock()) {
                    locked->SetText(FormatInt(prop.GetValue<int>()));
                }
            });
            return editor;
        }

        if (type == PropertyType::Float) {
            auto editor = std::make_shared<ImEditableText>();
            editor->SetText(FormatFloat(prop.GetValue<float>()));
            editor->OnTextCommitted.AddLambda([prop, weakEditor = std::weak_ptr<ImEditableText>(editor)](ImEditableText&, const std::string& text) mutable {
                float value = 0.0f;
                if (TryParseFloat(text, value)) {
                    prop.SetValue<float>(value);
                }
                if (auto locked = weakEditor.lock()) {
                    locked->SetText(FormatFloat(prop.GetValue<float>()));
                }
            });
            return editor;
        }

        if (type == PropertyType::Enum) {
            auto editor = std::make_shared<ImComboBox>();
            auto optionalProperty = prop.GetObject()->ToOptionalProperty(prop);
            const std::vector<std::string>& options = optionalProperty.GetOptionList();
            editor->SetItems(options);

            const std::string currentOption = optionalProperty.GetOptionString();
            for (int index = 0; index < static_cast<int>(options.size()); ++index) {
                if (options[index] == currentOption) {
                    editor->SetSelectedIndex(index);
                    break;
                }
            }

            editor->OnSelectionChanged.AddLambda([prop](ImComboBox&, int index) mutable {
                auto optional = prop.GetObject()->ToOptionalProperty(prop);
                optional.SetOptionByIndex(index);
            });
            return editor;
        }

        if (type == PropertyType::Color) {
            auto editor = std::make_shared<ImEditableText>();
            editor->SetText(FormatColor(prop.GetValue<FColor>()));
            editor->OnTextCommitted.AddLambda([prop, weakEditor = std::weak_ptr<ImEditableText>(editor)](ImEditableText&, const std::string& text) mutable {
                FColor value;
                if (TryParseColor(text, value)) {
                    prop.SetValue<FColor>(value);
                }
                if (auto locked = weakEditor.lock()) {
                    locked->SetText(FormatColor(prop.GetValue<FColor>()));
                }
            });
            return editor;
        }

        if (type == PropertyType::Vec2) {
            auto editor = std::make_shared<ImEditableText>();
            editor->SetText(FormatVec2(prop.GetValue<FVector2>()));
            editor->OnTextCommitted.AddLambda([prop, weakEditor = std::weak_ptr<ImEditableText>(editor)](ImEditableText&, const std::string& text) mutable {
                FVector2 value;
                if (TryParseVec2(text, value)) {
                    prop.SetValue<FVector2>(value);
                }
                if (auto locked = weakEditor.lock()) {
                    locked->SetText(FormatVec2(prop.GetValue<FVector2>()));
                }
            });
            return editor;
        }

        if (type == PropertyType::StringArray) {
            auto readOnly = MakeLabel(FormatStringArray(prop.GetValue<std::vector<std::string>>()), 13.0f, FColor::FromBytes(189, 198, 209));
            readOnly->SetWrapText(true);
            return readOnly;
        }

        return MakeLabel("(unsupported)", 13.0f, FColor::FromBytes(214, 149, 149));
    }

    std::shared_ptr<ImWidget> BuildPropertyRow(ReflectableObject::ROPProperty prop)
    {
        auto row = std::make_shared<ImHorizontalBox>();
        row->SetSpacing(10.0f);

        const std::string labelText = prop.GetName() + "  [" + prop.GetClassName() + "]";
        auto label = MakeLabel(labelText, 14.0f, FColor::FromBytes(219, 226, 234));
        row->AddChild(label, FMargin(0.0f, 0.0f, 0.0f, 0.0f));

        auto editor = BuildPropertyEditor(prop);
        row->AddChildFill(editor, 1.0f, FMargin(8.0f, 0.0f, 0.0f, 0.0f));

        return row;
    }

    std::shared_ptr<ImExpandableBox> BuildObjectSection(
        ReflectableObject& object,
        const std::string& sectionTitle,
        bool bExpanded,
        std::unordered_set<const ReflectableObject*>& visited)
    {
        auto section = std::make_shared<ImExpandableBox>();
        section->SetExpanded(bExpanded);
        section->SetHeader(MakeLabel(sectionTitle, 15.0f, FColor::FromBytes(248, 250, 252)));

        auto body = std::make_shared<ImVerticalBox>();
        body->SetSpacing(8.0f);

        if (visited.find(&object) != visited.end()) {
            body->AddChild(MakeLabel("(cycle detected)", 13.0f, FColor::FromBytes(214, 149, 149)));
            section->SetBody(body);
            return section;
        }

        visited.insert(&object);
        const auto properties = object.GetAllPropertiesOrdered();
        for (auto prop : properties) {
            if (prop.GetType() == PropertyType::Struct) {
                auto nestedObject = prop.GetPointer<ReflectableObject>();
                if (nestedObject) {
                    body->AddChild(
                        BuildObjectSection(
                            *nestedObject,
                            prop.GetName() + "  <" + nestedObject->GetClassName() + ">",
                            false,
                            visited));
                } else {
                    body->AddChild(MakeLabel(prop.GetName() + ": (null)", 13.0f, FColor::FromBytes(189, 198, 209)));
                }
            } else {
                body->AddChild(BuildPropertyRow(prop));
            }
        }
        visited.erase(&object);

        section->SetBody(body);
        return section;
    }

    std::shared_ptr<ImWidget> BuildSelectionContent()
    {
        auto content = std::make_shared<ImVerticalBox>();
        content->SetSpacing(10.0f);

        if (!SelectedObject_) {
            content->AddChild(MakeLabel("Select a widget, container, or reflection sample in the main window to inspect its runtime properties.", 14.0f, FColor::FromBytes(189, 198, 209)));
            return content;
        }

        auto title = MakeLabel("Selected Object", 19.0f, FColor::FromBytes(255, 214, 102));
        content->AddChild(title);
        content->AddChild(MakeLabel(SelectedObject_->GetClassName(), 16.0f, FColor::White));

        std::unordered_set<const ReflectableObject*> visited;
        content->AddChild(BuildObjectSection(
            *SelectedObject_,
            SelectedObject_->GetClassName(),
            true,
            visited));

        return content;
    }

    void RebuildInspector()
    {
        if (!InspectorWindow_) {
            return;
        }

        auto root = std::make_shared<ImVerticalBox>();
        root->SetSpacing(12.0f);

        auto intro = MakeLabel(
            "Edit properties live. Scalar values update immediately on the selected widget, and nested reflectable structs are expanded recursively.",
            14.0f,
            FColor::FromBytes(214, 222, 234));
        intro->SetWrapText(true);
        root->AddChild(intro, FMargin(10.0f, 0.0f, 10.0f, 0.0f));

        auto scrollBox = std::make_shared<ImScrollBox>();
        scrollBox->SetContent(BuildSelectionContent());
        root->AddChildFill(scrollBox, 1.0f, FMargin(0.0f));

        InspectorWindow_->SetRootWidget(root);
    }

    std::shared_ptr<ImTextBlock> StatusText_;
    std::shared_ptr<ImWindow> InspectorWindow_;
    std::shared_ptr<ReflectableObject> SelectedObject_;
    std::vector<std::shared_ptr<ImSelectableSampleCard>> SampleCards_;
};

std::shared_ptr<ImSelectableSampleCard> MakeInspectableSample(
    const std::shared_ptr<ImWidget>& widget,
    const std::shared_ptr<ReflectableObject>& target,
    const std::function<void(const std::shared_ptr<ReflectableObject>&)>& onSelected)
{
    auto card = std::make_shared<ImSelectableSampleCard>(onSelected);
    card->SetContent(widget);
    card->SetSelectionTarget(target ? target : std::static_pointer_cast<ReflectableObject>(widget));
    return card;
}

std::shared_ptr<ImSelectableSampleCard> MakeInspectableSample(
    const std::shared_ptr<ImWidget>& widget,
    const std::function<void(const std::shared_ptr<ReflectableObject>&)>& onSelected)
{
    return MakeInspectableSample(
        widget,
        std::static_pointer_cast<ReflectableObject>(widget),
        onSelected);
}

std::shared_ptr<ImSelectableSampleCard> MakeInspectableSummaryCard(
    const std::string& title,
    const std::string& subtitle,
    const std::shared_ptr<ReflectableObject>& target,
    const std::function<void(const std::shared_ptr<ReflectableObject>&)>& onSelected)
{
    auto content = std::make_shared<ImVerticalBox>();
    content->SetSpacing(6.0f);
    content->AddChild(MakeLabel(title, 16.0f, FColor::White));

    auto description = MakeLabel(subtitle, 13.0f, FColor::FromBytes(189, 198, 209));
    description->SetWrapText(true);
    content->AddChild(description);

    return MakeInspectableSample(content, target, onSelected);
}

} // namespace

class FReflectionInspectorDemoHostDelegate : public IApplicationHostDelegate {
public:
    FApplicationHostConfig GetHostConfig() const override
    {
        FApplicationHostConfig config;
        config.Title = "Reflection Inspector Demo - ImWidgetV4";
        config.InitialWidth = 1440;
        config.InitialHeight = 860;
#if defined(_WIN32)
        config.IniSettingsPath = Samples::GetDefaultSampleImGuiIniPath(L"ReflectionInspectorDemo.ini");
#endif
        return config;
    }

    void ConfigureApplication(ImApplication& application) override
    {
        ImApplication* app = &application;
    auto mainRoot = std::make_shared<ImVerticalBox>();
    mainRoot->SetSpacing(12.0f);

    auto title = MakeLabel("Reflection Property Inspector Demo", 30.0f, FColor::White);
    mainRoot->AddChild(title, FMargin(24.0f, 18.0f, 24.0f, 0.0f));

    auto intro = MakeLabel(
        "Click any sample card below. The floating inspector window will recursively display its reflectable properties and let you edit scalar values in place.",
        15.0f,
        FColor::FromBytes(214, 222, 234));
    intro->SetWrapText(true);
    mainRoot->AddChild(intro, FMargin(24.0f, 0.0f, 24.0f, 0.0f));

    auto status = MakeLabel("Selected: none", 15.0f, FColor::FromBytes(160, 214, 190));
    mainRoot->AddChild(status, FMargin(24.0f, 0.0f, 24.0f, 0.0f));

    auto sampleColumn = std::make_shared<ImVerticalBox>();
    sampleColumn->SetSpacing(12.0f);

    auto controller = std::make_shared<FPropertyInspectorController>(status);
    const auto selectObject = [controller](const std::shared_ptr<ReflectableObject>& object) {
        controller->SelectObject(object);
    };

    sampleColumn->AddChild(MakeLabel("Core Widgets", 18.0f, FColor::FromBytes(255, 214, 102)));

    auto primaryButton = std::make_shared<ImButton>();
    primaryButton->SetText("Primary Action");
    primaryButton->SetStyle(FButtonStyle::CreatePrimary());
    auto primaryButtonCard = MakeInspectableSample(primaryButton, selectObject);
    controller->RegisterSampleCard(primaryButtonCard);
    sampleColumn->AddChild(primaryButtonCard);

    auto dangerButton = std::make_shared<ImButton>();
    dangerButton->SetText("Delete Asset");
    dangerButton->SetStyle(FButtonStyle::CreateDanger());
    auto dangerButtonCard = MakeInspectableSample(dangerButton, selectObject);
    controller->RegisterSampleCard(dangerButtonCard);
    sampleColumn->AddChild(dangerButtonCard);

    auto headline = std::make_shared<ImTextBlock>();
    headline->SetText("Headline TextBlock");
    headline->SetFontSize(26.0f);
    headline->SetTextColor(FColor::FromBytes(255, 214, 102));
    auto headlineCard = MakeInspectableSample(headline, selectObject);
    controller->RegisterSampleCard(headlineCard);
    sampleColumn->AddChild(headlineCard);

    auto paragraph = std::make_shared<ImTextBlock>();
    paragraph->SetText("This standalone text block is intentionally wrap-enabled so you can edit its text, alignment, font size, and wrap behavior from the inspector.");
    paragraph->SetWrapText(true);
    paragraph->SetTextColor(FColor::FromBytes(220, 227, 235));
    auto paragraphCard = MakeInspectableSample(paragraph, selectObject);
    controller->RegisterSampleCard(paragraphCard);
    sampleColumn->AddChild(paragraphCard);

    sampleColumn->AddChild(MakeLabel("Input Widgets", 18.0f, FColor::FromBytes(255, 214, 102)));

    auto sampleCheckBox = std::make_shared<ImCheckBox>();
    sampleCheckBox->SetLabel("Disable the other input samples");
    auto sampleCheckBoxCard = MakeInspectableSample(sampleCheckBox, selectObject);
    controller->RegisterSampleCard(sampleCheckBoxCard);
    sampleColumn->AddChild(sampleCheckBoxCard);

    auto sampleSlider = std::make_shared<ImSlider>();
    sampleSlider->SetRange(0.0f, 100.0f);
    sampleSlider->SetValue(42.0f);
    sampleSlider->SetStep(5.0f);
    auto sampleSliderCard = MakeInspectableSample(sampleSlider, selectObject);
    controller->RegisterSampleCard(sampleSliderCard);
    sampleColumn->AddChild(sampleSliderCard);

    auto sampleComboBox = std::make_shared<ImComboBox>();
    sampleComboBox->SetPlaceholderText("Choose a preset");
    sampleComboBox->SetItems({"Prototype UI", "Polish Theme", "Write Tests", "Ship Demo"});
    sampleComboBox->SetSelectedIndex(1);
    auto sampleComboBoxCard = MakeInspectableSample(sampleComboBox, selectObject);
    controller->RegisterSampleCard(sampleComboBoxCard);
    sampleColumn->AddChild(sampleComboBoxCard);

    auto sampleEditableText = std::make_shared<ImEditableText>();
    sampleEditableText->SetHintText("Commit text with Enter");
    sampleEditableText->SetText("Reflection-enabled input");
    auto sampleEditableTextCard = MakeInspectableSample(sampleEditableText, selectObject);
    controller->RegisterSampleCard(sampleEditableTextCard);
    sampleColumn->AddChild(sampleEditableTextCard);

    sampleColumn->AddChild(MakeLabel("Layout And Containers", 18.0f, FColor::FromBytes(255, 214, 102)));

    auto sampleHorizontalBox = std::make_shared<ImHorizontalBox>();
    sampleHorizontalBox->SetSpacing(10.0f);
    sampleHorizontalBox->AddChild(MakeLabel("Left", 14.0f, FColor::White));
    sampleHorizontalBox->AddChild(std::make_shared<ImButton>(), FMargin(0.0f));
    auto middleButton = std::dynamic_pointer_cast<ImButton>(sampleHorizontalBox->GetChildren()[1]);
    middleButton->SetText("Center");
    sampleHorizontalBox->AddChild(MakeLabel("Right", 14.0f, FColor::FromBytes(189, 198, 209)));
    auto sampleHorizontalBoxCard = MakeInspectableSample(sampleHorizontalBox, selectObject);
    controller->RegisterSampleCard(sampleHorizontalBoxCard);
    sampleColumn->AddChild(sampleHorizontalBoxCard);

    auto sampleVerticalBox = std::make_shared<ImVerticalBox>();
    sampleVerticalBox->SetSpacing(8.0f);
    sampleVerticalBox->AddChild(MakeLabel("Stacked Row A", 14.0f, FColor::White));
    sampleVerticalBox->AddChild(MakeLabel("Stacked Row B", 14.0f, FColor::FromBytes(189, 198, 209)));
    sampleVerticalBox->AddChild(MakeLabel("Stacked Row C", 14.0f, FColor::FromBytes(160, 214, 190)));
    auto sampleVerticalBoxCard = MakeInspectableSample(sampleVerticalBox, selectObject);
    controller->RegisterSampleCard(sampleVerticalBoxCard);
    sampleColumn->AddChild(sampleVerticalBoxCard);

    auto sampleScrollContent = std::make_shared<ImVerticalBox>();
    sampleScrollContent->SetSpacing(6.0f);
    for (int index = 0; index < 10; ++index) {
        sampleScrollContent->AddChild(
            MakeLabel("Scrollable entry " + std::to_string(index + 1), 13.0f, FColor::FromBytes(214, 222, 234)));
    }
    auto sampleScrollBox = std::make_shared<ImScrollBox>();
    sampleScrollBox->SetContent(sampleScrollContent);
    auto sampleScrollBoxCard = MakeInspectableSample(sampleScrollBox, selectObject);
    controller->RegisterSampleCard(sampleScrollBoxCard);
    sampleColumn->AddChild(sampleScrollBoxCard);

    auto sampleExpandableBox = std::make_shared<ImExpandableBox>();
    sampleExpandableBox->SetExpanded(true);
    sampleExpandableBox->SetHeader(MakeLabel("ExpandableBox Header", 15.0f, FColor::White));
    auto expandableBody = std::make_shared<ImVerticalBox>();
    expandableBody->SetSpacing(8.0f);
    expandableBody->AddChild(MakeLabel("Body row one", 13.0f, FColor::FromBytes(214, 222, 234)));
    auto expandableCheckBox = std::make_shared<ImCheckBox>();
    expandableCheckBox->SetLabel("Nested body toggle");
    expandableBody->AddChild(expandableCheckBox);
    sampleExpandableBox->SetBody(expandableBody);
    auto sampleExpandableBoxCard = MakeInspectableSample(sampleExpandableBox, selectObject);
    controller->RegisterSampleCard(sampleExpandableBoxCard);
    sampleColumn->AddChild(sampleExpandableBoxCard);

    auto sampleCanvasPanel = std::make_shared<ImCanvasPanel>();
    sampleCanvasPanel->SetDesiredSize(FVector2(320.0f, 170.0f));
    sampleCanvasPanel->AddChildAt(MakeLabel("Canvas Top Left", 13.0f, FColor::White), FVector2(0.04f, 0.06f));
    auto canvasButton = std::make_shared<ImButton>();
    canvasButton->SetText("Overlay");
    sampleCanvasPanel->AddChildAt(canvasButton, FVector2(0.42f, 0.28f), FVector2(0.3f, 0.28f));
    auto canvasCheckBox = std::make_shared<ImCheckBox>();
    canvasCheckBox->SetLabel("Floating");
    sampleCanvasPanel->AddChildAt(canvasCheckBox, FVector2(0.18f, 0.64f));
    auto sampleCanvasPanelCard = MakeInspectableSample(sampleCanvasPanel, selectObject);
    controller->RegisterSampleCard(sampleCanvasPanelCard);
    sampleColumn->AddChild(sampleCanvasPanelCard);

    auto sampleHorizontalSplitter = std::make_shared<ImHorizontalSplitter>();
    sampleHorizontalSplitter->AddPart(MakeLabel("Left Pane", 13.0f, FColor::White), 1.0f, 50.0f);
    sampleHorizontalSplitter->AddPart(MakeLabel("Middle Pane", 13.0f, FColor::FromBytes(214, 222, 234)), 1.5f, 60.0f);
    sampleHorizontalSplitter->AddPart(MakeLabel("Right Pane", 13.0f, FColor::FromBytes(160, 214, 190)), 1.0f, 50.0f);
    auto sampleHorizontalSplitterCard = MakeInspectableSample(sampleHorizontalSplitter, selectObject);
    controller->RegisterSampleCard(sampleHorizontalSplitterCard);
    sampleColumn->AddChild(sampleHorizontalSplitterCard);

    auto sampleVerticalSplitter = std::make_shared<ImVerticalSplitter>();
    sampleVerticalSplitter->AddPart(MakeLabel("Top Pane", 13.0f, FColor::White), 1.0f, 32.0f);
    sampleVerticalSplitter->AddPart(MakeLabel("Middle Pane", 13.0f, FColor::FromBytes(214, 222, 234)), 1.2f, 36.0f);
    sampleVerticalSplitter->AddPart(MakeLabel("Bottom Pane", 13.0f, FColor::FromBytes(160, 214, 190)), 1.0f, 32.0f);
    auto sampleVerticalSplitterCard = MakeInspectableSample(sampleVerticalSplitter, selectObject);
    controller->RegisterSampleCard(sampleVerticalSplitterCard);
    sampleColumn->AddChild(sampleVerticalSplitterCard);

    sampleColumn->AddChild(MakeLabel("Content Widgets", 18.0f, FColor::FromBytes(255, 214, 102)));

    auto sampleImage = std::make_shared<ImImage>();
    sampleImage->SetDesiredSize(FVector2(240.0f, 140.0f));
    auto sampleImageCard = MakeInspectableSample(sampleImage, selectObject);
    controller->RegisterSampleCard(sampleImageCard);
    sampleColumn->AddChild(sampleImageCard);

    auto sampleTextList = std::make_shared<ImTextList>();
    sampleTextList->SetItems({
        "A long multi-line item that should wrap nicely inside the text list sample.",
        "A shorter row.",
        "Another row that you can select and inspect through the property panel.",
        "Final row for scrolling behavior."
    });
    sampleTextList->SetScrollOffset(18.0f);
    auto sampleTextListCard = MakeInspectableSample(sampleTextList, selectObject);
    controller->RegisterSampleCard(sampleTextListCard);
    sampleColumn->AddChild(sampleTextListCard);

    sampleColumn->AddChild(MakeLabel("Reflection-Only Slot Samples", 18.0f, FColor::FromBytes(255, 214, 102)));

    auto boxSlot = std::make_shared<ImBoxSlot>();
    boxSlot->SetFillCoefficient(1.75f);
    auto boxSlotCard = MakeInspectableSummaryCard(
        "ImBoxSlot",
        "Standalone slot sample for editing fill coefficient and inherited padding/geometry fields.",
        boxSlot,
        selectObject);
    controller->RegisterSampleCard(boxSlotCard);
    sampleColumn->AddChild(boxSlotCard);

    auto canvasSlot = std::make_shared<ImCanvasPanelSlot>();
    canvasSlot->SetRelativePosition(FVector2(0.15f, 0.2f));
    canvasSlot->SetRelativeSize(FVector2(0.45f, 0.3f));
    canvasSlot->SetAutoSize(false);
    auto canvasSlotCard = MakeInspectableSummaryCard(
        "ImCanvasPanelSlot",
        "Edits relative position, relative size, and autosize without needing a live canvas child.",
        canvasSlot,
        selectObject);
    controller->RegisterSampleCard(canvasSlotCard);
    sampleColumn->AddChild(canvasSlotCard);

    auto horizontalSplitterSlot = std::make_shared<ImHorizontalSplitterSlot>();
    horizontalSplitterSlot->SetRatio(1.4f);
    horizontalSplitterSlot->SetMinSize(72.0f);
    auto horizontalSplitterSlotCard = MakeInspectableSummaryCard(
        "ImHorizontalSplitterSlot",
        "Reflection sample for splitter ratios and minimum widths.",
        horizontalSplitterSlot,
        selectObject);
    controller->RegisterSampleCard(horizontalSplitterSlotCard);
    sampleColumn->AddChild(horizontalSplitterSlotCard);

    auto verticalSplitterSlot = std::make_shared<ImVerticalSplitterSlot>();
    verticalSplitterSlot->SetRatio(1.25f);
    verticalSplitterSlot->SetMinSize(64.0f);
    auto verticalSplitterSlotCard = MakeInspectableSummaryCard(
        "ImVerticalSplitterSlot",
        "Reflection sample for splitter ratios and minimum heights.",
        verticalSplitterSlot,
        selectObject);
    controller->RegisterSampleCard(verticalSplitterSlotCard);
    sampleColumn->AddChild(verticalSplitterSlotCard);

    auto mainSamplesScroll = std::make_shared<ImScrollBox>();
    mainSamplesScroll->SetContent(sampleColumn);
    mainRoot->AddChildFill(mainSamplesScroll, 1.0f, FMargin(24.0f, 8.0f, 24.0f, 24.0f));

    app->SetRootWidget(mainRoot);

    FWindowOptions inspectorOptions;
    inspectorOptions.Title = "Property Inspector";
    inspectorOptions.Position = FVector2(930.0f, 76.0f);
    inspectorOptions.Size = FVector2(430.0f, 700.0f);
    inspectorOptions.RootWidget = std::make_shared<ImVerticalBox>();
    auto inspectorWindow = app->GetWindowManager().CreateWindow(inspectorOptions);
    controller->SetInspectorWindow(inspectorWindow);

    primaryButton->OnClicked.AddLambda([status](ImButton&) {
        status->SetText("Primary button clicked. Inspector edits remain live.");
    });

    dangerButton->OnClicked.AddLambda([status](ImButton&) {
        status->SetText("Danger button clicked. Try editing its text or style in the inspector.");
    });

    sampleCheckBox->OnCheckStateChanged.AddLambda([sampleComboBox, sampleEditableText, sampleSlider, status](ImCheckBox&, bool checked) {
        sampleComboBox->SetDisabled(checked);
        sampleEditableText->SetDisabled(checked);
        sampleSlider->SetDisabled(checked);
        status->SetText(checked ? "Input samples disabled by checkbox." : "Input samples re-enabled.");
    });

    sampleComboBox->OnSelectionChanged.AddLambda([status](ImComboBox& sender, int) {
        status->SetText("Combo selection: " + sender.GetSelectedText());
    });

    sampleEditableText->OnTextCommitted.AddLambda([status](ImEditableText&, const std::string& text) {
        status->SetText("Editable text committed: " + text);
    });

    sampleSlider->OnValueChanged.AddLambda([status](ImSlider&, float value) {
        status->SetText("Slider value changed to " + FormatFloat(value));
    });

        controller->SelectObject(primaryButton);
    }
};


namespace ImWidgetV4 {

std::shared_ptr<IApplicationHostDelegate> CreateApplicationHostDelegate()
{
    return std::make_shared<FReflectionInspectorDemoHostDelegate>();
}

} // namespace ImWidgetV4








