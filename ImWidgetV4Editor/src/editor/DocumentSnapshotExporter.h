#pragma once

#include <imwidgetv4/core/Types.h>

#include <filesystem>
#include <optional>
#include <string>

namespace ImWidgetV4Editor {

struct FDocumentSnapshotExportRequest {
    std::filesystem::path InputPath;
    std::filesystem::path OutputPath;
    std::optional<int> Width;
    std::optional<int> Height;
};

struct FDocumentSnapshotExportResult {
    bool bSuccess = false;
    std::string ErrorMessage;
    std::filesystem::path OutputPath;
    ImWidgetV4::FVector2 ExportSize {0.0f, 0.0f};
};

class DocumentSnapshotExporter {
public:
    static FDocumentSnapshotExportResult ExportToPng(const FDocumentSnapshotExportRequest& request);
};

} // namespace ImWidgetV4Editor
