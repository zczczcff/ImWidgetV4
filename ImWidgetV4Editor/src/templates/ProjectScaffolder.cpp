#include "ProjectScaffolder.h"

#include "../codegen/WidgetTreeToCppGenerator.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <vector>

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

std::string BuildPathLiteral(const std::filesystem::path& value)
{
    return BuildStringLiteral(value.generic_string());
}

std::string BuildCMakeQuotedPath(const std::filesystem::path& value)
{
    std::string result;
    result.reserve(value.generic_string().size() + 8);
    for (char c : value.generic_string()) {
        switch (c) {
        case '\\':
            result += '/';
            break;
        case '\"':
            result += "\\\"";
            break;
        default:
            result.push_back(c);
            break;
        }
    }
    return result;
}

std::string BuildCMakeVersionArgument(const std::string& value)
{
    std::string result;
    result.reserve(value.size() + 1);
    for (char c : value) {
        const unsigned char ch = static_cast<unsigned char>(c);
        if (std::isalnum(ch) != 0 || c == '.' || c == '_' || c == '-') {
            result.push_back(c);
        }
    }
    return result.empty() ? "0.1.0" : result;
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

bool ReadTextFile(
    const std::filesystem::path& filePath,
    std::string& outText,
    std::string& outError)
{
    try {
        std::ifstream stream(filePath, std::ios::binary);
        if (!stream.is_open()) {
            outError = "Failed to open " + filePath.string() + " for reading.";
            return false;
        }

        std::stringstream buffer;
        buffer << stream.rdbuf();
        outText = buffer.str();
        return true;
    } catch (const std::exception& exception) {
        outError = exception.what();
        return false;
    } catch (...) {
        outError = "Unknown file read error.";
        return false;
    }
}

void AppendChangedFile(
    const std::filesystem::path& filePath,
    const std::string& expectedText,
    FProjectScaffoldResult& result)
{
    auto normalizeLineEndings = [](const std::string& text) {
        std::string normalized;
        normalized.reserve(text.size());
        for (std::size_t index = 0; index < text.size(); ++index) {
            if (text[index] == '\r') {
                if (index + 1 < text.size() && text[index + 1] == '\n') {
                    continue;
                }
                normalized.push_back('\n');
            } else {
                normalized.push_back(text[index]);
            }
        }
        return normalized;
    };

    std::string existingText;
    std::string errorMessage;
    if (!ReadTextFile(filePath, existingText, errorMessage) ||
        normalizeLineEndings(existingText) != normalizeLineEndings(expectedText)) {
        result.GeneratedFiles.push_back(filePath);
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
        << "# User-owned entry point. The editor creates this file once and does not\n"
        << "# overwrite it during project code regeneration.\n"
        << "include(cmake/ImWidgetV4GeneratedProject.cmake)\n\n"
        << "if(EXISTS \"${CMAKE_CURRENT_SOURCE_DIR}/cmake/UserProject.cmake\")\n"
        << "    include(cmake/UserProject.cmake)\n"
        << "endif()\n";
    return stream.str();
}

std::string BuildUserProjectCMakeText()
{
    std::ostringstream stream;
    stream
        << "# User-owned CMake extension file.\n"
        << "# Add your own sources, include directories, compile definitions, libraries,\n"
        << "# resources, or target properties here. The editor never overwrites this file.\n"
        << "#\n"
        << "# Example:\n"
        << "# target_sources(${PROJECT_NAME} PRIVATE src/MyFeature.cpp)\n"
        << "# target_compile_definitions(${PROJECT_NAME} PRIVATE MY_FEATURE=1)\n";
    return stream.str();
}

std::string GetLibraryIntegrationModeToken(EEditorLibraryIntegrationMode mode)
{
    return mode == EEditorLibraryIntegrationMode::SDK ? "SDK" : "Source";
}

std::string BuildGeneratedProjectCMakeText(
    const FProjectScaffoldRequest& request)
{
    const FEditorApplicationSettings& settings = request.ApplicationSettings;
    const std::string integrationMode = GetLibraryIntegrationModeToken(settings.LibraryIntegrationMode);
    const std::string sdkPackagePath = BuildCMakeQuotedPath(settings.SdkPackagePath);
    const std::string minimumSdkVersion = BuildCMakeVersionArgument(settings.MinimumSdkVersion);

    std::ostringstream stream;
    stream
        << "# Auto-generated by ImWidgetV4Editor. Do not edit manually.\n"
        << "# Use cmake/UserProject.cmake for user CMake customizations.\n\n"
        << "set(CMAKE_CXX_STANDARD 17)\n"
        << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
        << "set(CMAKE_CXX_EXTENSIONS OFF)\n\n"
        << "set(IMWIDGETV4_LIBRARY_MODE \"" << integrationMode << "\" CACHE STRING \"ImWidgetV4 integration mode: SDK or Source\")\n"
        << "set_property(CACHE IMWIDGETV4_LIBRARY_MODE PROPERTY STRINGS SDK Source)\n"
        << "set(IMWIDGETV4_SDK_DIR \"" << sdkPackagePath << "\" CACHE PATH \"Path to the ImWidgetV4 SDK CMake package directory\")\n"
        << "set(IMWIDGETV4_ROOT \"\" CACHE PATH \"Path to the ImWidgetV4 library root\")\n"
        << "if(NOT IMWIDGETV4_LIBRARY_MODE STREQUAL \"SDK\" AND NOT IMWIDGETV4_LIBRARY_MODE STREQUAL \"Source\")\n"
        << "    message(FATAL_ERROR \"IMWIDGETV4_LIBRARY_MODE must be SDK or Source.\")\n"
        << "endif()\n\n"
        << "if(IMWIDGETV4_LIBRARY_MODE STREQUAL \"SDK\")\n"
        << "    if(IMWIDGETV4_SDK_DIR STREQUAL \"\")\n"
        << "        find_package(ImWidgetV4 " << minimumSdkVersion << " CONFIG REQUIRED)\n"
        << "    else()\n"
        << "        find_package(ImWidgetV4 " << minimumSdkVersion << " CONFIG REQUIRED PATHS \"${IMWIDGETV4_SDK_DIR}\" NO_DEFAULT_PATH)\n"
        << "    endif()\n"
        << "    get_property(IMWIDGETV4_SDK_IMPORTED_CONFIGURATIONS TARGET ImWidgetV4::core PROPERTY IMPORTED_CONFIGURATIONS)\n"
        << "    set(IMWIDGETV4_SDK_CONFIGURATIONS \"\")\n"
        << "    foreach(IMWIDGETV4_SDK_IMPORTED_CONFIGURATION IN LISTS IMWIDGETV4_SDK_IMPORTED_CONFIGURATIONS)\n"
        << "        string(TOLOWER \"${IMWIDGETV4_SDK_IMPORTED_CONFIGURATION}\" IMWIDGETV4_SDK_CONFIGURATION_LOWER)\n"
        << "        if(IMWIDGETV4_SDK_CONFIGURATION_LOWER STREQUAL \"debug\")\n"
        << "            list(APPEND IMWIDGETV4_SDK_CONFIGURATIONS Debug)\n"
        << "        elseif(IMWIDGETV4_SDK_CONFIGURATION_LOWER STREQUAL \"release\")\n"
        << "            list(APPEND IMWIDGETV4_SDK_CONFIGURATIONS Release)\n"
        << "        elseif(IMWIDGETV4_SDK_CONFIGURATION_LOWER STREQUAL \"relwithdebinfo\")\n"
        << "            list(APPEND IMWIDGETV4_SDK_CONFIGURATIONS RelWithDebInfo)\n"
        << "        elseif(IMWIDGETV4_SDK_CONFIGURATION_LOWER STREQUAL \"minsizerel\")\n"
        << "            list(APPEND IMWIDGETV4_SDK_CONFIGURATIONS MinSizeRel)\n"
        << "        else()\n"
        << "            list(APPEND IMWIDGETV4_SDK_CONFIGURATIONS \"${IMWIDGETV4_SDK_IMPORTED_CONFIGURATION}\")\n"
        << "        endif()\n"
        << "    endforeach()\n"
        << "    if(IMWIDGETV4_SDK_CONFIGURATIONS)\n"
        << "        list(REMOVE_DUPLICATES IMWIDGETV4_SDK_CONFIGURATIONS)\n"
        << "        if(CMAKE_CONFIGURATION_TYPES)\n"
        << "            set(CMAKE_CONFIGURATION_TYPES \"${IMWIDGETV4_SDK_CONFIGURATIONS}\" CACHE STRING \"Available ImWidgetV4 SDK configurations\" FORCE)\n"
        << "        elseif(CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE IN_LIST IMWIDGETV4_SDK_CONFIGURATIONS)\n"
        << "            message(FATAL_ERROR \"The selected ImWidgetV4 SDK does not provide configuration '${CMAKE_BUILD_TYPE}'. Available configurations: ${IMWIDGETV4_SDK_CONFIGURATIONS}\")\n"
        << "        endif()\n"
        << "    endif()\n"
        << "else()\n"
        << "    if(IMWIDGETV4_ROOT STREQUAL \"\")\n"
        << "        get_filename_component(IMWIDGETV4_ROOT \"${CMAKE_CURRENT_SOURCE_DIR}/../ImWidgetV4\" ABSOLUTE)\n"
        << "    endif()\n\n"
        << "    if(NOT EXISTS \"${IMWIDGETV4_ROOT}/CMakeLists.txt\")\n"
        << "        message(FATAL_ERROR \"IMWIDGETV4_ROOT must point to the ImWidgetV4 library root.\")\n"
        << "    endif()\n\n"
        << "set(IMWIDGETV4_BUILD_TESTS OFF CACHE BOOL \"\" FORCE)\n"
        << "set(IMWIDGETV4_BUILD_SAMPLES OFF CACHE BOOL \"\" FORCE)\n"
        << "set(IMWIDGETV4_BUILD_EDITOR OFF CACHE BOOL \"\" FORCE)\n"
        << "if(WIN32)\n"
        << "    set(IMWIDGETV4_BUILD_WIN32_DX11_BACKEND ON CACHE BOOL \"\" FORCE)\n"
        << "    set(IMWIDGETV4_BUILD_ANDROID_GLES3_BACKEND OFF CACHE BOOL \"\" FORCE)\n"
        << "elseif(ANDROID)\n"
        << "    set(IMWIDGETV4_BUILD_WIN32_DX11_BACKEND OFF CACHE BOOL \"\" FORCE)\n"
        << "    set(IMWIDGETV4_BUILD_ANDROID_GLES3_BACKEND ON CACHE BOOL \"\" FORCE)\n"
        << "else()\n"
        << "    set(IMWIDGETV4_BUILD_WIN32_DX11_BACKEND OFF CACHE BOOL \"\" FORCE)\n"
        << "    set(IMWIDGETV4_BUILD_ANDROID_GLES3_BACKEND OFF CACHE BOOL \"\" FORCE)\n"
        << "endif()\n\n"
        << "    add_subdirectory(\"${IMWIDGETV4_ROOT}\" \"${CMAKE_BINARY_DIR}/ImWidgetV4\")\n"
        << "endif()\n\n"
        << "set(IMWIDGETV4_APP_SOURCES\n"
        << "    src/main.cpp\n"
        << "    generated/AppProjectConfig.cpp\n"
        << "    generated/" << request.StartupWidgetClassName << ".cpp\n";
    if (request.TitleBarRootWidget) {
        stream
            << "    generated/" << request.TitleBarWidgetClassName << ".cpp\n";
    }
    stream
        << ")\n\n"
        << "if(ANDROID)\n"
        << "    set(IMWIDGETV4_ANDROID_NDK_ROOT \"\")\n"
        << "    if(DEFINED CMAKE_ANDROID_NDK)\n"
        << "        set(IMWIDGETV4_ANDROID_NDK_ROOT ${CMAKE_ANDROID_NDK})\n"
        << "    elseif(DEFINED ANDROID_NDK)\n"
        << "        set(IMWIDGETV4_ANDROID_NDK_ROOT ${ANDROID_NDK})\n"
        << "    endif()\n\n"
        << "    set(IMWIDGETV4_ANDROID_NATIVE_APP_GLUE_SOURCE \"\")\n"
        << "    if(IMWIDGETV4_ANDROID_NDK_ROOT)\n"
        << "        set(IMWIDGETV4_ANDROID_NATIVE_APP_GLUE_SOURCE\n"
        << "            ${IMWIDGETV4_ANDROID_NDK_ROOT}/sources/android/native_app_glue/android_native_app_glue.c\n"
        << "        )\n"
        << "    endif()\n\n"
        << "    add_library(${PROJECT_NAME} SHARED\n"
        << "        ${IMWIDGETV4_APP_SOURCES}\n"
        << "        ${IMWIDGETV4_ANDROID_NATIVE_APP_GLUE_SOURCE}\n"
        << "    )\n"
        << "else()\n"
        << "    add_executable(${PROJECT_NAME}\n"
        << "        ${IMWIDGETV4_APP_SOURCES}\n"
        << "    )\n"
        << "endif()\n\n"
        << "target_include_directories(${PROJECT_NAME} PRIVATE\n"
        << "    ${CMAKE_CURRENT_SOURCE_DIR}/generated\n"
        << ")\n\n"
        << "if(IMWIDGETV4_LIBRARY_MODE STREQUAL \"SDK\" AND COMMAND imwidgetv4_attach_sdk_sources)\n"
        << "    imwidgetv4_attach_sdk_sources(${PROJECT_NAME})\n"
        << "endif()\n\n"
        << "target_link_libraries(${PROJECT_NAME} PRIVATE\n"
        << "    $<IF:$<STREQUAL:${IMWIDGETV4_LIBRARY_MODE},SDK>,ImWidgetV4::core,imwidgetv4_core>\n"
        << "    $<IF:$<STREQUAL:${IMWIDGETV4_LIBRARY_MODE},SDK>,ImWidgetV4::reflection_json,imwidgetv4_reflection_json>\n"
        << ")\n\n"
        << "if(WIN32)\n"
        << "    target_link_libraries(${PROJECT_NAME} PRIVATE\n"
        << "        $<IF:$<STREQUAL:${IMWIDGETV4_LIBRARY_MODE},SDK>,ImWidgetV4::platform_win32_dx11,imwidgetv4_platform_win32_dx11>\n"
        << "        $<IF:$<STREQUAL:${IMWIDGETV4_LIBRARY_MODE},SDK>,ImWidgetV4::app_host_win32_main,imwidgetv4_app_host_win32_main>\n"
        << "    )\n"
        << "    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)\n"
        << "elseif(ANDROID)\n"
        << "    target_link_libraries(${PROJECT_NAME} PRIVATE\n"
        << "        $<IF:$<STREQUAL:${IMWIDGETV4_LIBRARY_MODE},SDK>,ImWidgetV4::platform_android_gles3,imwidgetv4_platform_android_gles3>\n"
        << "        $<IF:$<STREQUAL:${IMWIDGETV4_LIBRARY_MODE},SDK>,ImWidgetV4::app_host_android_main,imwidgetv4_app_host_android_main>\n"
        << "        android\n"
        << "        log\n"
        << "    )\n"
        << "    if(IMWIDGETV4_ANDROID_NDK_ROOT)\n"
        << "        target_include_directories(${PROJECT_NAME} PRIVATE\n"
        << "            ${IMWIDGETV4_ANDROID_NDK_ROOT}/sources/android/native_app_glue\n"
        << "        )\n"
        << "    endif()\n"
        << "else()\n"
        << "    message(FATAL_ERROR \"Generated project supports only WIN32 or ANDROID hosts.\")\n"
        << "endif()\n\n"
        << "if(MSVC)\n"
        << "    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)\n"
        << "endif()\n";
    return stream.str();
}

std::string BuildMainCppText()
{
    std::ostringstream stream;
    stream
        << "// Stable user entry translation unit.\n"
        << "// Platform entry points are provided by the selected ImWidgetV4 app host target.\n\n"
        << "#include \"AppProjectConfig.h\"\n\n"
        << "#include <imwidgetv4/app/ApplicationHost.h>\n"
        << "#include <imwidgetv4/core/Application.h>\n\n"
        << "#include <memory>\n\n"
        << "namespace {\n\n"
        << "class FGeneratedAppHostDelegate final : public ImWidgetV4::IApplicationHostDelegate\n"
        << "{\n"
        << "public:\n"
        << "    ImWidgetV4::FApplicationHostConfig GetHostConfig() const override\n"
        << "    {\n"
        << "        ImWidgetV4::FApplicationHostConfig config = GeneratedApp::BuildHostConfig();\n"
        << "        return config;\n"
        << "    }\n\n"
        << "    void ConfigureApplication(ImWidgetV4::ImApplication& application) override\n"
        << "    {\n"
        << "        GeneratedApp::ConfigureApplication(application);\n"
        << "    }\n\n"
        << "    // Optional host overrides. Uncomment the functions you want to customize.\n"
        << "    // void ConfigureBackend(ImWidgetV4::ImApplicationBackend& backend) override\n"
        << "    // {\n"
        << "    //     (void)backend;\n"
        << "    // }\n\n"
        << "    // bool InitializeApplication(ImWidgetV4::ImApplication& application, ImWidgetV4::ImApplicationBackend& backend) override\n"
        << "    // {\n"
        << "    //     (void)application;\n"
        << "    //     (void)backend;\n"
        << "    //     return true;\n"
        << "    // }\n\n"
        << "    // void Tick(ImWidgetV4::ImApplication& application, const ImWidgetV4::FFrameInfo& frameInfo) override\n"
        << "    // {\n"
        << "    //     (void)application;\n"
        << "    //     (void)frameInfo;\n"
        << "    // }\n\n"
        << "    // bool OnCloseRequested(ImWidgetV4::ImApplication& application) override\n"
        << "    // {\n"
        << "    //     (void)application;\n"
        << "    //     return true;\n"
        << "    // }\n\n"
        << "    // void OnShutdown(ImWidgetV4::ImApplication& application) override\n"
        << "    // {\n"
        << "    //     (void)application;\n"
        << "    // }\n"
        << "};\n\n"
        << "} // namespace\n\n"
        << "namespace ImWidgetV4 {\n\n"
        << "std::shared_ptr<IApplicationHostDelegate> CreateApplicationHostDelegate()\n"
        << "{\n"
        << "    return std::make_shared<FGeneratedAppHostDelegate>();\n"
        << "}\n\n"
        << "} // namespace ImWidgetV4\n";
    return stream.str();
}

std::string BuildCommentedText(const std::string& text)
{
    std::ostringstream stream;
    std::istringstream input(text);
    std::string line;
    bool bWroteAnyLine = false;
    while (std::getline(input, line)) {
        stream << "// " << line << "\n";
        bWroteAnyLine = true;
    }
    if (!bWroteAnyLine && !text.empty()) {
        stream << "// " << text << "\n";
    }
    return stream.str();
}

std::string BuildAppProjectConfigHeaderText(const FProjectScaffoldRequest& request)
{
    std::ostringstream stream;
    stream
        << "#pragma once\n\n"
        << "#include <imwidgetv4/app/ApplicationHost.h>\n\n"
        << "#include <memory>\n\n"
        << "namespace " << request.NamespaceName << " {\n"
        << "class " << request.StartupWidgetClassName << ";\n";
    if (request.ApplicationSettings.bUseTitleBar && request.TitleBarRootWidget) {
        stream
            << "class " << request.TitleBarWidgetClassName << ";\n";
    }
    stream
        << "} // namespace " << request.NamespaceName << "\n\n"
        << "namespace ImWidgetV4 {\n"
        << "class ImApplication;\n"
        << "}\n\n"
        << "namespace GeneratedApp {\n\n"
        << "ImWidgetV4::FApplicationHostConfig BuildHostConfig();\n"
        << "void ConfigureApplication(ImWidgetV4::ImApplication& application);\n"
        << "std::shared_ptr<" << request.NamespaceName << "::" << request.StartupWidgetClassName << "> GetStartupView();\n";
    if (request.ApplicationSettings.bUseTitleBar && request.TitleBarRootWidget) {
        stream
            << "std::shared_ptr<" << request.NamespaceName << "::" << request.TitleBarWidgetClassName << "> GetTitleBarView();\n";
    }
    stream
        << "\n"
        << "} // namespace GeneratedApp\n";
    return stream.str();
}

std::string BuildAppProjectConfigSourceText(const FProjectScaffoldRequest& request)
{
    const FEditorApplicationSettings& settings = request.ApplicationSettings;
    const std::string title = settings.Title.empty() ? request.ProjectName : settings.Title;
    std::ostringstream stream;
    stream
        << "#include \"AppProjectConfig.h\"\n\n"
        << "#include <imwidgetv4/core/Application.h>\n"
        << "#include <imwidgetv4/core/Types.h>\n"
        << "#include <imwidgetv4/widgets/Button.h>\n"
        << "#include <imwidgetv4/widgets/TextBlock.h>\n"
        << "#include <imwidgetv4/widgets/TitleBar.h>\n"
        << "#include <imwidgetv4/widgets/VerticalBox.h>\n"
        << "#include <filesystem>\n"
        << "#include <memory>\n"
        << "#include <string>\n\n"
        << "#include \"" << request.StartupWidgetClassName << ".h\"\n";
    if (settings.bUseTitleBar && request.TitleBarRootWidget) {
        stream
            << "#include \"" << request.TitleBarWidgetClassName << ".h\"\n";
    }
    stream
        << "\n"
        << "namespace {\n\n"
        << "std::weak_ptr<" << request.NamespaceName << "::" << request.StartupWidgetClassName << "> GStartupView;\n";
    if (settings.bUseTitleBar && request.TitleBarRootWidget) {
        stream
            << "std::weak_ptr<" << request.NamespaceName << "::" << request.TitleBarWidgetClassName << "> GTitleBarView;\n";
    }
    stream
        << "\n"
        << "} // namespace\n\n"
        << "namespace GeneratedApp {\n\n"
        << "ImWidgetV4::FApplicationHostConfig BuildHostConfig()\n"
        << "{\n"
        << "        ImWidgetV4::FApplicationHostConfig config;\n"
        << "        config.Title = " << BuildStringLiteral(title) << ";\n"
        << "        config.InitialWidth = " << std::max(1, settings.InitialWidth) << ";\n"
        << "        config.InitialHeight = " << std::max(1, settings.InitialHeight) << ";\n"
        << "        config.bUseCustomHostChrome = " << (settings.bUseCustomHostChrome ? "true" : "false") << ";\n";
    if (!settings.IconPath.empty()) {
        stream
            << "        // TODO: load application icon from " << settings.IconPath.generic_string() << " when a runtime image loader is available.\n";
    }
    if (settings.bEnableIniSettings && !settings.IniSettingsPath.empty()) {
        stream
            << "        config.IniSettingsPath = std::filesystem::path(" << BuildPathLiteral(settings.IniSettingsPath) << ");\n";
    }
    stream
        << "        return config;\n"
        << "}\n\n"
        << "void ConfigureApplication(ImWidgetV4::ImApplication& application)\n"
        << "{\n";
    if (!settings.DefaultTheme.empty()) {
        stream
            << "        application.SetActiveTheme(" << BuildStringLiteral(settings.DefaultTheme) << ");\n";
    }
    if (!settings.DefaultCulture.empty()) {
        stream
            << "        application.SetCulture(" << BuildStringLiteral(settings.DefaultCulture) << ");\n";
    }
    for (const std::filesystem::path& stringTablePath : settings.StringTablePaths) {
        if (!stringTablePath.empty()) {
            stream
                << "        application.LoadStringTable(std::filesystem::path(" << BuildPathLiteral(stringTablePath) << "));\n";
        }
    }
    stream
        << "        application.SetApplicationTitle(" << BuildStringLiteral(title) << ");\n";

    if (settings.bUseTitleBar && request.TitleBarRootWidget) {
        stream
            << "        auto rootLayout = std::make_shared<ImWidgetV4::ImVerticalBox>();\n"
            << "        rootLayout->SetSpacing(0.0f);\n"
            << "        auto titleBarView = std::make_shared<" << request.NamespaceName << "::" << request.TitleBarWidgetClassName << ">();\n"
            << "        GTitleBarView = titleBarView;\n"
            << "        rootLayout->AddChild(titleBarView, ImWidgetV4::FMargin(0.0f));\n"
            << "        auto startupView = std::make_shared<" << request.NamespaceName << "::" << request.StartupWidgetClassName << ">();\n"
            << "        GStartupView = startupView;\n"
            << "        rootLayout->AddChildFill(startupView, 1.0f, ImWidgetV4::FMargin(0.0f));\n"
            << "        application.SetRootWidget(rootLayout);\n";
    } else {
        stream
            << "        auto startupView = std::make_shared<" << request.NamespaceName << "::" << request.StartupWidgetClassName << ">();\n"
            << "        GStartupView = startupView;\n"
            << "        application.SetRootWidget(startupView);\n";
    }

    stream
        << "}\n\n"
        << "std::shared_ptr<" << request.NamespaceName << "::" << request.StartupWidgetClassName << "> GetStartupView()\n"
        << "{\n"
        << "        return GStartupView.lock();\n"
        << "}\n";
    if (settings.bUseTitleBar && request.TitleBarRootWidget) {
        stream
            << "\n"
            << "std::shared_ptr<" << request.NamespaceName << "::" << request.TitleBarWidgetClassName << "> GetTitleBarView()\n"
            << "{\n"
            << "        return GTitleBarView.lock();\n"
            << "}\n";
    }

    stream
        << "\n"
        << "} // namespace GeneratedApp\n";
    return stream.str();
}

FProjectScaffoldResult GenerateBlankAppCode(const FProjectScaffoldRequest& request)
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
    const FCodeGenResult generatedCode = request.StartupRootWidgetJson.is_object()
        ? WidgetTreeToCppGenerator::Generate(request.StartupRootWidgetJson, codeGenOptions)
        : WidgetTreeToCppGenerator::Generate(request.StartupRootWidget, codeGenOptions);
    if (!generatedCode.bSuccess) {
        result.ErrorMessage = generatedCode.ErrorMessage.empty()
            ? std::string("Startup widget code generation failed.")
            : generatedCode.ErrorMessage;
        return result;
    }

    FCodeGenResult generatedTitleBarCode;
    if (request.TitleBarRootWidget) {
        FCodeGenOptions titleBarCodeGenOptions;
        titleBarCodeGenOptions.ClassName = request.TitleBarWidgetClassName;
        titleBarCodeGenOptions.Namespace = request.NamespaceName;
        generatedTitleBarCode = request.TitleBarRootWidgetJson.is_object()
            ? WidgetTreeToCppGenerator::Generate(request.TitleBarRootWidgetJson, titleBarCodeGenOptions)
            : WidgetTreeToCppGenerator::Generate(request.TitleBarRootWidget, titleBarCodeGenOptions);
        if (!generatedTitleBarCode.bSuccess) {
            result.ErrorMessage = generatedTitleBarCode.ErrorMessage.empty()
                ? std::string("Title bar widget code generation failed.")
                : generatedTitleBarCode.ErrorMessage;
            return result;
        }
    }

    std::string errorMessage;

    const std::filesystem::path appProjectConfigHeaderPath =
        request.ProjectRoot / "generated" / "AppProjectConfig.h";
    if (!WriteTextFile(appProjectConfigHeaderPath, BuildAppProjectConfigHeaderText(request), errorMessage)) {
        result.ErrorMessage = errorMessage;
        return result;
    }
    result.GeneratedFiles.push_back(appProjectConfigHeaderPath);

    const std::filesystem::path appProjectConfigSourcePath =
        request.ProjectRoot / "generated" / "AppProjectConfig.cpp";
    if (!WriteTextFile(appProjectConfigSourcePath, BuildAppProjectConfigSourceText(request), errorMessage)) {
        result.ErrorMessage = errorMessage;
        return result;
    }
    result.GeneratedFiles.push_back(appProjectConfigSourcePath);

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

    if (request.TitleBarRootWidget) {
        const std::filesystem::path titleBarHeaderPath =
            request.ProjectRoot / "generated" / generatedTitleBarCode.Files.HeaderFileName;
        if (!WriteTextFile(titleBarHeaderPath, generatedTitleBarCode.Files.HeaderText, errorMessage)) {
            result.ErrorMessage = errorMessage;
            return result;
        }
        result.GeneratedFiles.push_back(titleBarHeaderPath);

        const std::filesystem::path titleBarSourcePath =
            request.ProjectRoot / "generated" / generatedTitleBarCode.Files.SourceFileName;
        if (!WriteTextFile(titleBarSourcePath, generatedTitleBarCode.Files.SourceText, errorMessage)) {
            result.ErrorMessage = errorMessage;
            return result;
        }
        result.GeneratedFiles.push_back(titleBarSourcePath);
    }

    result.bSuccess = true;
    return result;
}

FProjectScaffoldResult PreviewBlankAppCode(const FProjectScaffoldRequest& request)
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
    const FCodeGenResult generatedCode = request.StartupRootWidgetJson.is_object()
        ? WidgetTreeToCppGenerator::Generate(request.StartupRootWidgetJson, codeGenOptions)
        : WidgetTreeToCppGenerator::Generate(request.StartupRootWidget, codeGenOptions);
    if (!generatedCode.bSuccess) {
        result.ErrorMessage = generatedCode.ErrorMessage.empty()
            ? std::string("Startup widget code generation failed.")
            : generatedCode.ErrorMessage;
        return result;
    }

    FCodeGenResult generatedTitleBarCode;
    if (request.TitleBarRootWidget) {
        FCodeGenOptions titleBarCodeGenOptions;
        titleBarCodeGenOptions.ClassName = request.TitleBarWidgetClassName;
        titleBarCodeGenOptions.Namespace = request.NamespaceName;
        generatedTitleBarCode = request.TitleBarRootWidgetJson.is_object()
            ? WidgetTreeToCppGenerator::Generate(request.TitleBarRootWidgetJson, titleBarCodeGenOptions)
            : WidgetTreeToCppGenerator::Generate(request.TitleBarRootWidget, titleBarCodeGenOptions);
        if (!generatedTitleBarCode.bSuccess) {
            result.ErrorMessage = generatedTitleBarCode.ErrorMessage.empty()
                ? std::string("Title bar widget code generation failed.")
                : generatedTitleBarCode.ErrorMessage;
            return result;
        }
    }

    AppendChangedFile(
        request.ProjectRoot / "generated" / "AppProjectConfig.h",
        BuildAppProjectConfigHeaderText(request),
        result);
    AppendChangedFile(
        request.ProjectRoot / "generated" / "AppProjectConfig.cpp",
        BuildAppProjectConfigSourceText(request),
        result);
    AppendChangedFile(
        request.ProjectRoot / "generated" / generatedCode.Files.HeaderFileName,
        generatedCode.Files.HeaderText,
        result);
    AppendChangedFile(
        request.ProjectRoot / "generated" / generatedCode.Files.SourceFileName,
        generatedCode.Files.SourceText,
        result);

    if (request.TitleBarRootWidget) {
        AppendChangedFile(
            request.ProjectRoot / "generated" / generatedTitleBarCode.Files.HeaderFileName,
            generatedTitleBarCode.Files.HeaderText,
            result);
        AppendChangedFile(
            request.ProjectRoot / "generated" / generatedTitleBarCode.Files.SourceFileName,
            generatedTitleBarCode.Files.SourceText,
            result);
    }

    AppendChangedFile(
        request.ProjectRoot / "cmake" / "ImWidgetV4GeneratedProject.cmake",
        BuildGeneratedProjectCMakeText(request),
        result);

    result.bSuccess = true;
    return result;
}

FProjectScaffoldResult ScaffoldBlankApp(const FProjectScaffoldRequest& request)
{
    FProjectScaffoldResult result;
    if (request.ProjectRoot.empty()) {
        result.ErrorMessage = "Project root is empty.";
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

    const std::filesystem::path userProjectCMakePath = request.ProjectRoot / "cmake" / "UserProject.cmake";
    if (!WriteTextFile(userProjectCMakePath, BuildUserProjectCMakeText(), errorMessage)) {
        result.ErrorMessage = errorMessage;
        return result;
    }
    result.GeneratedFiles.push_back(userProjectCMakePath);

    const std::filesystem::path mainCppPath = request.ProjectRoot / "src" / "main.cpp";
    if (!WriteTextFile(mainCppPath, BuildMainCppText(), errorMessage)) {
        result.ErrorMessage = errorMessage;
        return result;
    }
    result.GeneratedFiles.push_back(mainCppPath);

    FProjectScaffoldResult codeResult = ProjectScaffolder::GenerateCode(request);
    if (!codeResult.bSuccess) {
        return codeResult;
    }
    result.GeneratedFiles.insert(
        result.GeneratedFiles.end(),
        codeResult.GeneratedFiles.begin(),
        codeResult.GeneratedFiles.end());
    result.bSuccess = true;
    return result;
}

} // namespace

FProjectScaffoldResult ProjectScaffolder::GenerateCMake(const FProjectScaffoldRequest& request)
{
    FProjectScaffoldResult result;
    if (request.ProjectRoot.empty()) {
        result.ErrorMessage = "Project root is empty.";
        return result;
    }

    std::string errorMessage;
    const std::filesystem::path generatedCMakePath =
        request.ProjectRoot / "cmake" / "ImWidgetV4GeneratedProject.cmake";
    if (!WriteTextFile(generatedCMakePath, BuildGeneratedProjectCMakeText(request), errorMessage)) {
        result.ErrorMessage = errorMessage;
        return result;
    }

    result.GeneratedFiles.push_back(generatedCMakePath);
    result.bSuccess = true;
    return result;
}

FProjectScaffoldResult ProjectScaffolder::GenerateCode(const FProjectScaffoldRequest& request)
{
    if (request.TemplateName.empty() || request.TemplateName == "Blank App") {
        FProjectScaffoldResult codeResult = GenerateBlankAppCode(request);
        if (!codeResult.bSuccess) {
            return codeResult;
        }

        FProjectScaffoldResult cmakeResult = GenerateCMake(request);
        if (!cmakeResult.bSuccess) {
            return cmakeResult;
        }

        codeResult.GeneratedFiles.insert(
            codeResult.GeneratedFiles.end(),
            cmakeResult.GeneratedFiles.begin(),
            cmakeResult.GeneratedFiles.end());
        return codeResult;
    }

    FProjectScaffoldResult result;
    result.ErrorMessage = "Unsupported project template: " + request.TemplateName;
    return result;
}

FProjectScaffoldResult ProjectScaffolder::GenerateCodePreview(const FProjectScaffoldRequest& request)
{
    if (request.TemplateName.empty() || request.TemplateName == "Blank App") {
        return PreviewBlankAppCode(request);
    }

    FProjectScaffoldResult result;
    result.ErrorMessage = "Unsupported project template: " + request.TemplateName;
    return result;
}

FProjectScaffoldResult ProjectScaffolder::ReinitializeMainCpp(const FProjectScaffoldRequest& request)
{
    FProjectScaffoldResult result;
    if (request.ProjectRoot.empty()) {
        result.ErrorMessage = "Project root is empty.";
        return result;
    }

    const std::filesystem::path mainCppPath = request.ProjectRoot / "src" / "main.cpp";
    std::string existingText;
    {
        std::ifstream stream(mainCppPath, std::ios::binary);
        if (stream.is_open()) {
            std::stringstream buffer;
            buffer << stream.rdbuf();
            existingText = buffer.str();
        }
    }

    std::ostringstream replacement;
    if (!existingText.empty()) {
        replacement
            << "// Previous main.cpp content was preserved by Reinitialize Main.cpp.\n"
            << "// -----------------------------------------------------------------------------\n"
            << BuildCommentedText(existingText)
            << "// -----------------------------------------------------------------------------\n\n";
    }
    replacement << BuildMainCppText();

    std::string errorMessage;
    if (!WriteTextFile(mainCppPath, replacement.str(), errorMessage)) {
        result.ErrorMessage = errorMessage;
        return result;
    }

    result.GeneratedFiles.push_back(mainCppPath);
    result.bSuccess = true;
    return result;
}

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
