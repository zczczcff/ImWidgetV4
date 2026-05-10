#include "ProjectSettingsDialog.h"

#include "../inspector/PropertyEditorWidgets.h"

#include <imwidgetv4/core/WindowManager.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <algorithm>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;
using namespace ImWidgetV4Editor::PropertyEditorWidgets;

namespace {

FButtonStyle MakeDialogButtonStyle(bool bPrimary)
{
    return bPrimary ? FButtonStyle::CreatePrimary() : FButtonStyle();
}

FVector2 MaxSize(const FVector2& left, const FVector2& right)
{
    return FVector2(
        std::max(left.X, right.X),
        std::max(left.Y, right.Y));
}

std::vector<std::string> BuildProbeLines(const FEnvironmentProbeReport& report)
{
    std::vector<std::string> lines;
    lines.push_back("Target: " + GetTargetPlatformDisplayName(report.TargetPlatform));
    lines.push_back(std::string("Ready: ") + (report.bReady ? "Yes" : "No"));
    lines.push_back("");
    for (const FEnvironmentProbeItem& item : report.Items) {
        lines.push_back(item.Label + " [" + ToDisplayString(item.Status) + "]");
        lines.push_back("  " + item.Details);
    }
    return lines;
}

} // namespace

bool ProjectSettingsDialog::Open(ImApplication& app, const FProjectSettingsDialogOptions& options)
{
    Close(app);
    m_Options = options;

    auto title = std::make_shared<ImTextBlock>();
    title->SetText(m_Options.HeadingText);
    title->SetWrapText(false);
    title->SetFontSize(16.0f);
    title->SetTextColor(FColor::FromBytes(238, 242, 247));

    auto projectNameField = MakeInspectorReadOnlyField(m_Options.ProjectName);
    auto namespaceField = MakeInspectorReadOnlyField(m_Options.NamespaceName);
    auto startupField = MakeInspectorReadOnlyField(m_Options.StartupDocument);

    auto profileComboBox = std::make_shared<ImComboBox>();
    ApplyInspectorComboBoxStyle(*profileComboBox);
    profileComboBox->SetItems(m_Options.BuildProfileNames);
    for (int index = 0; index < static_cast<int>(m_Options.BuildProfileNames.size()); ++index) {
        if (m_Options.BuildProfileNames[static_cast<std::size_t>(index)] == m_Options.ActiveBuildProfileName) {
            profileComboBox->SetSelectedIndex(index);
            break;
        }
    }

    auto probeText = std::make_shared<ImTextList>();
    FTextListStyle probeStyle = probeText->GetStyle();
    probeStyle.BackgroundColor = FColor::FromBytes(18, 23, 29);
    probeStyle.BorderColor = FColor::FromBytes(16, 19, 23);
    probeStyle.FocusedOutlineColor = FColor::FromBytes(103, 177, 255);
    probeStyle.TextColor = FColor::FromBytes(196, 205, 217);
    probeStyle.SelectionBackgroundColor = FColor::FromBytes(72, 104, 146, 148);
    probeStyle.Padding = FMargin(10.0f);
    probeStyle.MinDesiredSize = FVector2(0.0f, 200.0f);
    probeStyle.CornerRadius = 5.0f;
    probeStyle.BorderThickness = 1.0f;
    probeStyle.FontSize = 13.0f;
    probeStyle.LineSpacing = 1.1f;
    probeText->SetStyle(probeStyle);
    probeText->SetItems(BuildProbeLines(m_Options.ProbeReport));

    auto errorText = std::make_shared<ImTextBlock>();
    errorText->SetText("");
    errorText->SetWrapText(false);
    errorText->SetFontSize(12.0f);
    errorText->SetTextColor(FColor::FromBytes(239, 103, 103));

    auto confirmButton = std::make_shared<ImButton>();
    confirmButton->SetStyle(MakeDialogButtonStyle(true));
    confirmButton->SetText("Apply");

    auto cancelButton = std::make_shared<ImButton>();
    cancelButton->SetStyle(MakeDialogButtonStyle(false));
    cancelButton->SetText("Close");

    auto fields = std::make_shared<ImVerticalBox>();
    fields->SetSpacing(8.0f);
    fields->AddChild(MakeInspectorVerticalPropertyRow("Project", projectNameField), FMargin(0.0f));
    fields->AddChild(MakeInspectorVerticalPropertyRow("Namespace", namespaceField), FMargin(0.0f));
    fields->AddChild(MakeInspectorVerticalPropertyRow("Startup UI", startupField), FMargin(0.0f));
    fields->AddChild(MakeInspectorVerticalPropertyRow("Active Build Profile", profileComboBox), FMargin(0.0f));
    fields->AddChild(MakeInspectorVerticalPropertyRow("Environment Probe", probeText), FMargin(0.0f));
    fields->AddChild(errorText, FMargin(2.0f, 0.0f, 2.0f, 0.0f));

    auto buttonRow = std::make_shared<ImHorizontalBox>();
    buttonRow->SetSpacing(8.0f);
    buttonRow->AddChildFill(MakeInspectorFlexibleSpacer(), 1.0f, FMargin(0.0f));
    buttonRow->AddChild(confirmButton);
    buttonRow->AddChild(cancelButton);

    auto root = std::make_shared<ImVerticalBox>();
    root->SetSpacing(10.0f);
    root->AddChild(title, FMargin(12.0f, 12.0f, 12.0f, 0.0f));
    root->AddChild(fields, FMargin(12.0f, 0.0f, 12.0f, 0.0f));
    root->AddChild(buttonRow, FMargin(12.0f, 0.0f, 12.0f, 12.0f));
    const FVector2 popupContentMinSize = root->GetMinSize();

    m_Root = root;
    m_ProfileComboBox = profileComboBox;
    m_ProbeText = probeText;
    m_ErrorText = errorText;
    m_ConfirmButton = confirmButton;
    m_CancelButton = cancelButton;

    const std::weak_ptr<ProjectSettingsDialog> weakThis = weak_from_this();
    auto requestConfirm = [weakThis, &app]() {
        if (auto self = weakThis.lock()) {
            self->SetErrorMessage("");
            if (!self->m_ProfileComboBox || !self->m_ProfileComboBox->HasSelection()) {
                self->SetErrorMessage("Select a build profile.");
                return;
            }

            const std::string profileName = self->m_ProfileComboBox->GetSelectedText();
            const std::function<bool(const std::string&)> onConfirm = self->m_Options.OnConfirm;
            if (onConfirm && !onConfirm(profileName)) {
                return;
            }

            self->Close(app);
        }
    };

    auto requestCancel = [weakThis, &app]() {
        if (auto self = weakThis.lock()) {
            const std::function<void()> onCancel = self->m_Options.OnCancel;
            self->Close(app);
            if (onCancel) {
                onCancel();
            }
        }
    };

    confirmButton->OnClicked.AddLambda([requestConfirm](ImButton&) {
        requestConfirm();
    });
    cancelButton->OnClicked.AddLambda([requestCancel](ImButton&) {
        requestCancel();
    });

    FPopupOptions popupOptions;
    popupOptions.Title = m_Options.PopupTitle;
    popupOptions.Position = m_Options.Position;
    popupOptions.Size = MaxSize(m_Options.Size, popupContentMinSize);
    popupOptions.RootWidget = root;
    popupOptions.bCloseOnClickOutside = true;
    popupOptions.Style.CornerRadius = 6.0f;
    popupOptions.Style.BorderThickness = 1.0f;
    popupOptions.Style.bDrawShadow = false;

    m_Window = app.GetWindowManager().CreatePopup(popupOptions);
    app.SetKeyboardFocus(profileComboBox);
    return static_cast<bool>(m_Window);
}

void ProjectSettingsDialog::Close(ImApplication& app)
{
    if (m_Window) {
        app.GetWindowManager().CloseWindow(m_Window);
    }
    Reset();
}

bool ProjectSettingsDialog::IsOpen() const
{
    return m_Window && m_Window->IsOpen();
}

void ProjectSettingsDialog::Reset()
{
    m_Window.reset();
    m_Root.reset();
    m_ProfileComboBox.reset();
    m_ProbeText.reset();
    m_ErrorText.reset();
    m_ConfirmButton.reset();
    m_CancelButton.reset();
    m_Options = FProjectSettingsDialogOptions();
}

void ProjectSettingsDialog::SetErrorMessage(const std::string& message)
{
    if (m_ErrorText) {
        m_ErrorText->SetText(message);
    }
}

} // namespace ImWidgetV4Editor
