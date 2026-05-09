#include "ProjectScaffolder.h"

#include "../codegen/WidgetTreeToCppGenerator.h"

#include <cctype>
#include <fstream>
#include <sstream>

namespace ImWidgetV4Editor {

namespace {

std::string BuildStringLiteral(const std::string& value)
{
    std::string result = "\"";
    result.reserve(value.size() + 8);

    for (char c : value) {
        switch (c) {
        case '\\':
            result += "\\\\";
            break;
        case '\"':
            result += "\\\"";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            result.push_back(c);
            break;
        }
    }

    result.push_back('\"');
    return result;
}

bool IsIdentifierStartChar(char c)
{
    const unsigned char value = static_cast<unsigned char>(c);
    return std::isalpha(value) != 0 || c == '_';
}

bool IsIdentifierContinueChar(char c)
{
    const unsigned char value = static_cast<unsigned char>(c);
    return std::isalnum(value) != 0 || c == '_';
}

std::string NormalizeIdentifier(const std::string& rawText, const std::string& fallback)
{
    auto sanitize = [](const std::string& text) {
        std::string result;
        result.reserve(text.size());
        for (char c : text) {
            if (IsIdentifierContinueChar(c)) {
                result.push_back(c);
            } else if (result.empty() || result.back() != '_') {
                result.push_back('_');
            }
        }

        while (!result.empty() && result.back() == '_') {
            result.pop_back();
        }

        return result;
    };

    std::string normalized = sanitize(rawText);
    if (normalized.empty()) {
        normalized = sanitize(fallback);
    }
    if (normalized.empty()) {
        normalized = "ImWidgetApp";
    }
    if (!IsIdentifierStartChar(normalized.front())) {
        normalized.insert(normalized.begin(), '_');
    }
    return normalized;
}

bool WriteTextFile(
    const std::filesystem::path& filePath,
    const std::string& text,
    std::string& outError)
{
    try {
        const std::filesystem::path parentPath = filePath.parent_path();
        if (!parentPath.empty()) {
            std::filesystem::create_directories(parentPath);
        }

        std::ofstream stream(filePath, std::ios::binary | std::ios::trunc);
        if (!stream.is_open()) {
            outError = "Failed to open " + filePath.string() + " for writing.";
            return false;
        }

        stream << text;
        stream.flush();
        if (!stream.good()) {
            outError = "Failed to write " + filePath.string() + ".";
            return false;
        }

        return true;
    } catch (const std::exception& exception) {
        outError = exception.what();
        return false;
    } catch (...) {
        outError = "Unknown file write error.";
        return false;
    }
}

std::string BuildRootCMakeListsText(
    const FProjectScaffoldRequest& request,
    const std::string& cmakeProjectName)
{
    std::ostringstream stream;
    stream
        << "cmake_minimum_required(VERSION 3.15)\n"
        << "project(" << cmakeProjectName << " LANGUAGES CXX)\n\n"
        << "set(CMAKE_CXX_STANDARD 17)\n"
        << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
        << "set(CMAKE_CXX_EXTENSIONS OFF)\n\n"
        << "set(IMWIDGETV4_ROOT \"\" CACHE PATH \"Path to the ImWidgetV4 library root\")\n"
        << "if(IMWIDGETV4_ROOT STREQUAL \"\")\n"
        << "    get_filename_component(IMWIDGETV4_ROOT \"${CMAKE_CURRENT_SOURCE_DIR}/../ImWidgetV4\" ABSOLUTE)\n"
        << "endif()\n\n"
        << "if(NOT EXISTS \"${IMWIDGETV4_ROOT}/CMakeLists.txt\")\n"
        << "    message(FATAL_ERROR \"IMWIDGETV4_ROOT must point to the ImWidgetV4 library root.\")\n"
        << "endif()\n\n"
        << "set(IMWIDGETV4_BUILD_TESTS OFF CACHE BOOL \"\" FORCE)\n"
        << "set(IMWIDGETV4_BUILD_SAMPLES OFF CACHE BOOL \"\" FORCE)\n"
        << "set(IMWIDGETV4_BUILD_EDITOR OFF CACHE BOOL \"\" FORCE)\n"
        << "set(IMWIDGETV4_BUILD_ANDROID_GLES3_BACKEND OFF CACHE BOOL \"\" FORCE)\n"
        << "set(IMWIDGETV4_BUILD_WIN32_DX11_BACKEND ON CACHE BOOL \"\" FORCE)\n\n"
        << "add_subdirectory(\"${IMWIDGETV4_ROOT}\" \"${CMAKE_BINARY_DIR}/ImWidgetV4\")\n\n"
        << "add_executable(${PROJECT_NAME} WIN32\n"
        << "    src/main.cpp\n"
        << "    generated/" << request.StartupWidgetClassName << ".cpp\n"
        << ")\n\n"
        << "target_include_directories(${PROJECT_NAME} PRIVATE\n"
        << "    ${CMAKE_CURRENT_SOURCE_DIR}/generated\n"
        << ")\n\n"
        << "target_link_libraries(${PROJECT_NAME} PRIVATE\n"
        << "    imwidgetv4_core\n"
        << "    imwidgetv4_platform_win32_dx11\n"
        << ")\n\n"
        << "if(MSVC)\n"
        << "    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)\n"
        << "endif()\n";
    return stream.str();
}

std::string BuildMainCppText(const FProjectScaffoldRequest& request)
{
    std::ostringstream stream;
    stream
        << "#include <imwidgetv4/core/Application.h>\n"
        << "#include <imwidgetv4/platform/Win32DX11Backend.h>\n"
        << "#include <memory>\n"
        << "#include <string>\n"
        << "#include <Windows.h>\n\n"
        << "#include \"" << request.StartupWidgetClassName << ".h\"\n\n"
        << "namespace {\n\n"
        << "std::wstring Utf8ToWide(const std::string& text)\n"
        << "{\n"
        << "    if (text.empty()) {\n"
        << "        return std::wstring();\n"
        << "    }\n\n"
        << "    const int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);\n"
        << "    if (length <= 0) {\n"
        << "        return std::wstring(text.begin(), text.end());\n"
        << "    }\n\n"
        << "    std::wstring result(static_cast<std::size_t>(length), L'\\0');\n"
        << "    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), length);\n"
        << "    if (!result.empty() && result.back() == L'\\0') {\n"
        << "        result.pop_back();\n"
        << "    }\n"
        << "    return result;\n"
        << "}\n\n"
        << "} // namespace\n\n"
        << "int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)\n"
        << "{\n"
        << "    const std::string windowTitle = " << BuildStringLiteral(request.ProjectName) << ";\n"
        << "    auto backend = std::make_shared<ImWidgetV4::ImWin32DX11Backend>(Utf8ToWide(windowTitle), 1280, 720);\n"
        << "    if (!backend->Initialize()) {\n"
        << "        return -1;\n"
        << "    }\n\n"
        << "    auto application = std::make_shared<ImWidgetV4::ImApplication>();\n"
        << "    backend->SetApplication(application.get());\n"
        << "    application->SetApplicationTitle(windowTitle);\n"
        << "    application->SetRootWidget(std::make_shared<" << request.NamespaceName << "::" << request.StartupWidgetClassName << ">());\n\n"
        << "    backend->Run();\n"
        << "    backend->Shutdown();\n"
        << "    return 0;\n"
        << "}\n";
    return stream.str();
}

FProjectScaffoldResult ScaffoldBlankApp(const FProjectScaffoldRequest& request)
{
    FProjectScaffoldResult result;
    if (request.ProjectRoot.empty()) {
        result.ErrorMessage = "Project root is empty.";
        return result;
    }
    if (!request.StartupRootWidget) {
        result.ErrorMessage = "Startup widget tree is empty.";
        return result;
    }

    FCodeGenOptions codeGenOptions;
    codeGenOptions.ClassName = request.StartupWidgetClassName;
    codeGenOptions.Namespace = request.NamespaceName;
    const FCodeGenResult generatedCode =
        WidgetTreeToCppGenerator::Generate(request.StartupRootWidget, codeGenOptions);
    if (!generatedCode.bSuccess) {
        result.ErrorMessage = generatedCode.ErrorMessage.empty()
            ? std::string("Startup widget code generation failed.")
            : generatedCode.ErrorMessage;
        return result;
    }

    const std::string cmakeProjectName = NormalizeIdentifier(request.ProjectName, "ImWidgetApp");
    std::string errorMessage;

    const std::filesystem::path rootCMakeListsPath = request.ProjectRoot / "CMakeLists.txt";
    if (!WriteTextFile(rootCMakeListsPath, BuildRootCMakeListsText(request, cmakeProjectName), errorMessage)) {
        result.ErrorMessage = errorMessage;
        return result;
    }
    result.GeneratedFiles.push_back(rootCMakeListsPath);

    const std::filesystem::path mainCppPath = request.ProjectRoot / "src" / "main.cpp";
    if (!WriteTextFile(mainCppPath, BuildMainCppText(request), errorMessage)) {
        result.ErrorMessage = errorMessage;
        return result;
    }
    result.GeneratedFiles.push_back(mainCppPath);

    const std::filesystem::path generatedHeaderPath =
        request.ProjectRoot / "generated" / generatedCode.Files.HeaderFileName;
    if (!WriteTextFile(generatedHeaderPath, generatedCode.Files.HeaderText, errorMessage)) {
        result.ErrorMessage = errorMessage;
        return result;
    }
    result.GeneratedFiles.push_back(generatedHeaderPath);

    const std::filesystem::path generatedSourcePath =
        request.ProjectRoot / "generated" / generatedCode.Files.SourceFileName;
    if (!WriteTextFile(generatedSourcePath, generatedCode.Files.SourceText, errorMessage)) {
        result.ErrorMessage = errorMessage;
        return result;
    }
    result.GeneratedFiles.push_back(generatedSourcePath);

    result.bSuccess = true;
    return result;
}

} // namespace

FProjectScaffoldResult ProjectScaffolder::Scaffold(const FProjectScaffoldRequest& request)
{
    if (request.TemplateName.empty() || request.TemplateName == "Blank App") {
        return ScaffoldBlankApp(request);
    }

    FProjectScaffoldResult result;
    result.ErrorMessage = "Unsupported project template: " + request.TemplateName;
    return result;
}

} // namespace ImWidgetV4Editor
