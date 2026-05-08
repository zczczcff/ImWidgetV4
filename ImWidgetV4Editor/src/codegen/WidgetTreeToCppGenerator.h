#pragma once

#include "../editor/EditorDocument.h"

#include <memory>
#include <string>

namespace ImWidgetV4 {
class ImWidget;
}

namespace ImWidgetV4Editor {

struct FCodeGenOptions {
    std::string ClassName;
    std::string Namespace;
    std::string BaseClass = "ImWidgetV4::ImUserWidget";
};

struct FGeneratedFilePair {
    std::string HeaderFileName;
    std::string SourceFileName;
    std::string HeaderText;
    std::string SourceText;
};

struct FCodeGenResult {
    bool bSuccess = false;
    std::string ErrorMessage;
    FGeneratedFilePair Files;
};

class WidgetTreeToCppGenerator {
public:
    static FCodeGenResult Generate(
        const std::shared_ptr<ImWidgetV4::ImWidget>& rootWidget,
        const FCodeGenOptions& options);

    static FCodeGenResult Generate(
        const EditorDocument& document,
        const FCodeGenOptions& options);
};

} // namespace ImWidgetV4Editor
