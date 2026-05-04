#include <Windows.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/core/ReflectableObject.h>
#include <imwidgetv4/core/WindowManager.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/ExpandableBox.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/Slider.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
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
    using FSelectCallback = std::function<void(const std::shared_ptr<ImWidget>&)>;

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

    void SetSelectionTarget(const std::shared_ptr<ImWidget>& target)
    {
        SelectionTarget_ = target;
    }

    std::shared_ptr<ImWidget> GetSelectionTarget() const
    {
        if (auto target = SelectionTarget_.lock()) {
            return target;
        }

        const auto& children = GetChildren();
        return children.empty() ? nullptr : children.front();
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
    std::weak_ptr<ImWidget> SelectionTarget_;
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

    void SelectWidget(const std::shared_ptr<ImWidget>& widget)
    {
        SelectedWidget_ = widget;
        UpdateSelectionVisuals();

        if (StatusText_) {
            if (widget) {
                StatusText_->SetText("Selected: " + widget->GetClassName());
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

            card->SetSelected(card->GetSelectionTarget() == SelectedWidget_);
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

        if (!SelectedWidget_) {
            content->AddChild(MakeLabel("Select a widget in the main window to inspect its runtime properties.", 14.0f, FColor::FromBytes(189, 198, 209)));
            return content;
        }

        auto title = MakeLabel("Selected Object", 19.0f, FColor::FromBytes(255, 214, 102));
        content->AddChild(title);
        content->AddChild(MakeLabel(SelectedWidget_->GetClassName(), 16.0f, FColor::White));

        std::unordered_set<const ReflectableObject*> visited;
        content->AddChild(BuildObjectSection(
            *SelectedWidget_,
            SelectedWidget_->GetClassName(),
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
    std::shared_ptr<ImWidget> SelectedWidget_;
    std::vector<std::shared_ptr<ImSelectableSampleCard>> SampleCards_;
};

std::shared_ptr<ImSelectableSampleCard> MakeInspectableSample(
    const std::shared_ptr<ImWidget>& widget,
    const std::function<void(const std::shared_ptr<ImWidget>&)>& onSelected)
{
    auto card = std::make_shared<ImSelectableSampleCard>(onSelected);
    card->SetContent(widget);
    card->SetSelectionTarget(widget);
    return card;
}

} // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    auto backend = std::make_shared<ImWin32DX11Backend>(
        L"Reflection Inspector Demo - ImWidgetV4",
        1440,
        860);

    if (!backend->Initialize()) {
        MessageBoxW(nullptr, L"Backend initialization failed", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    auto app = std::make_shared<ImApplication>();
    app->SetIniSettingsPath(Examples::GetDefaultDemoImGuiIniPath(L"ReflectionInspectorDemo.ini"));
    backend->SetApplication(app.get());

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

    FPropertyInspectorController controller(status);

    auto primaryButton = std::make_shared<ImButton>();
    primaryButton->SetText("Primary Action");
    primaryButton->SetStyle(FButtonStyle::CreatePrimary());
    auto primaryButtonCard = MakeInspectableSample(primaryButton, [&](const std::shared_ptr<ImWidget>& widget) {
        controller.SelectWidget(widget);
    });
    controller.RegisterSampleCard(primaryButtonCard);
    sampleColumn->AddChild(primaryButtonCard);

    auto dangerButton = std::make_shared<ImButton>();
    dangerButton->SetText("Delete Asset");
    dangerButton->SetStyle(FButtonStyle::CreateDanger());
    auto dangerButtonCard = MakeInspectableSample(dangerButton, [&](const std::shared_ptr<ImWidget>& widget) {
        controller.SelectWidget(widget);
    });
    controller.RegisterSampleCard(dangerButtonCard);
    sampleColumn->AddChild(dangerButtonCard);

    auto headline = std::make_shared<ImTextBlock>();
    headline->SetText("Headline TextBlock");
    headline->SetFontSize(26.0f);
    headline->SetTextColor(FColor::FromBytes(255, 214, 102));
    auto headlineCard = MakeInspectableSample(headline, [&](const std::shared_ptr<ImWidget>& widget) {
        controller.SelectWidget(widget);
    });
    controller.RegisterSampleCard(headlineCard);
    sampleColumn->AddChild(headlineCard);

    auto paragraph = std::make_shared<ImTextBlock>();
    paragraph->SetText("This standalone text block is intentionally wrap-enabled so you can edit its text, alignment, font size, and wrap behavior from the inspector.");
    paragraph->SetWrapText(true);
    paragraph->SetTextColor(FColor::FromBytes(220, 227, 235));
    auto paragraphCard = MakeInspectableSample(paragraph, [&](const std::shared_ptr<ImWidget>& widget) {
        controller.SelectWidget(widget);
    });
    controller.RegisterSampleCard(paragraphCard);
    sampleColumn->AddChild(paragraphCard);

    auto compositeInfo = std::make_shared<ImTextBlock>();
    compositeInfo->SetText("The controls below are here to keep the main window interactive while you inspect button/text properties.");
    compositeInfo->SetWrapText(true);
    compositeInfo->SetTextColor(FColor::FromBytes(174, 186, 200));
    sampleColumn->AddChild(compositeInfo);

    auto interactionsRow = std::make_shared<ImHorizontalBox>();
    interactionsRow->SetSpacing(12.0f);

    auto sampleCheckBox = std::make_shared<ImCheckBox>();
    sampleCheckBox->SetLabel("Live toggle");
    interactionsRow->AddChild(sampleCheckBox);

    auto sampleSlider = std::make_shared<ImSlider>();
    sampleSlider->SetRange(0.0f, 100.0f);
    sampleSlider->SetValue(42.0f);
    interactionsRow->AddChildFill(sampleSlider, 1.0f);

    sampleColumn->AddChild(interactionsRow);
    mainRoot->AddChild(sampleColumn, FMargin(24.0f, 8.0f, 24.0f, 24.0f));

    app->SetRootWidget(mainRoot);

    FWindowOptions inspectorOptions;
    inspectorOptions.Title = "Property Inspector";
    inspectorOptions.Position = FVector2(930.0f, 76.0f);
    inspectorOptions.Size = FVector2(430.0f, 700.0f);
    inspectorOptions.RootWidget = std::make_shared<ImVerticalBox>();
    auto inspectorWindow = app->GetWindowManager().CreateWindow(inspectorOptions);
    controller.SetInspectorWindow(inspectorWindow);

    primaryButton->OnClicked.AddLambda([&](ImButton&) {
        status->SetText("Primary button clicked. Inspector edits remain live.");
    });

    dangerButton->OnClicked.AddLambda([&](ImButton&) {
        status->SetText("Danger button clicked. Try editing its text or style in the inspector.");
    });

    sampleCheckBox->OnCheckStateChanged.AddLambda([&](ImCheckBox&, bool checked) {
        sampleSlider->SetDisabled(checked);
    });

    controller.SelectWidget(primaryButton);

    backend->Run();
    backend->Shutdown();
    return 0;
}
