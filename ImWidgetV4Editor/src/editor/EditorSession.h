#pragma once

#include "EditorDocument.h"

#include <imwidgetv4/core/Application.h>
#include <functional>
#include <memory>
#include <string>

namespace ImWidgetV4 {
class ImDesignerSurface;
class ImScrollBox;
class ImTabView;
class ImTextBlock;
class ImVerticalBox;
class ImWidget;
}

namespace ImWidgetV4Editor {

class EditorSession {
public:
    explicit EditorSession(std::function<std::shared_ptr<ImWidgetV4::ImWidget>()> createDefaultDocumentRoot);

    void BindDocumentWidgets(
        const std::shared_ptr<ImWidgetV4::ImTabView>& documentTabs,
        int documentTabIndex,
        const std::shared_ptr<ImWidgetV4::ImScrollBox>& documentHost,
        const std::shared_ptr<ImWidgetV4::ImDesignerSurface>& designerSurface,
        const std::shared_ptr<ImWidgetV4::ImTextBlock>& selectionText,
        const std::shared_ptr<ImWidgetV4::ImTextBlock>& outputText);

    const std::shared_ptr<EditorDocument>& GetDocument() const { return m_Document; }
    std::string GetDocumentTabTitle() const;

    bool NewDocument();
    bool OpenDocument(ImWidgetV4::ImApplication& app);
    bool SaveDocument(ImWidgetV4::ImApplication& app);
    bool SaveDocumentAs(ImWidgetV4::ImApplication& app);

    void LogStatus(const std::string& text);

private:
    std::shared_ptr<EditorDocument> CreateDefaultDocument() const;
    void ApplyDocumentToUi();
    void HandleDesignerSelectionChanged(
        ImWidgetV4::ImDesignerSurface& designerSurface,
        const std::shared_ptr<ImWidgetV4::ImWidget>& selectedWidget);
    void UpdateSelectionDetails(const std::shared_ptr<ImWidgetV4::ImWidget>& selectedWidget);
    std::filesystem::path ResolveDialogDirectory() const;

    std::function<std::shared_ptr<ImWidgetV4::ImWidget>()> m_CreateDefaultDocumentRoot;
    std::shared_ptr<EditorDocument> m_Document;
    std::shared_ptr<ImWidgetV4::ImTabView> m_DocumentTabs;
    std::shared_ptr<ImWidgetV4::ImScrollBox> m_DocumentHost;
    std::shared_ptr<ImWidgetV4::ImDesignerSurface> m_DesignerSurface;
    std::shared_ptr<ImWidgetV4::ImTextBlock> m_SelectionText;
    std::shared_ptr<ImWidgetV4::ImTextBlock> m_OutputText;
    int m_DocumentTabIndex = -1;
};

} // namespace ImWidgetV4Editor
