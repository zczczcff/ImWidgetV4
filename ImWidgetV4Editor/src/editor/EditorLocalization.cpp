#include "EditorLocalization.h"

#include <utility>

namespace ImWidgetV4Editor {

namespace {

constexpr const char* GEditorNamespacePrefix = "Editor.";

} // namespace

ImWidgetV4::FText EditorText(const std::string& key, const std::string& defaultText)
{
    return ImWidgetV4::FText::FromKey(std::string(GEditorNamespacePrefix) + key, defaultText);
}

void RegisterEditorDefaultStringTables()
{
    ImWidgetV4::FStringTable english;
    english.Culture = "en-US";
    english.Entries = {
        {"Editor.App.Title", "ImWidgetV4 Editor"},
        {"Editor.App.InitialHint", "Drag widgets from the left palette into the designer surface."},
        {"Editor.App.Action", "Action"},
        {"Editor.TitleBar.File", "File"},
        {"Editor.TitleBar.Edit", "Edit"},
        {"Editor.TitleBar.Project", "Project"},
        {"Editor.TitleBar.Build", "Build"},
        {"Editor.TitleBar.View", "View"},
        {"Editor.TitleBar.Search", "Search"},
        {"Editor.TitleBar.Undo", "Undo"},
        {"Editor.TitleBar.Redo", "Redo"},
        {"Editor.Dock.Controls", "Controls"},
        {"Editor.Dock.Project", "Project"},
        {"Editor.Dock.Build", "Build"},
        {"Editor.Dock.WidgetTree", "Widget Tree"},
        {"Editor.Workspace.Designer", "Designer"},
        {"Editor.Workspace.Preview", "Preview"},
        {"Editor.Workspace.Schema", "Schema"},
        {"Editor.Build.NoProjectLoaded", "No project loaded."},
        {"Editor.Build.OverviewHint", "Build/Toolchain overview will appear here."},
        {"Editor.Build.PanelTitle", "Build"},
        {"Editor.Build.PanelBody", "Toolchain readiness, active profile, and recent build output."},
        {"Editor.Build.ActiveProfile", "Active Profile"},
        {"Editor.Build.Generator", "Generator"},
        {"Editor.Build.AndroidSdkRoot", "Android SDK Root"},
        {"Editor.Build.AndroidNdkRoot", "Android NDK Root"},
        {"Editor.Build.OverrideAndroidSdkRoot", "Override Android SDK root"},
        {"Editor.Build.OverrideAndroidNdkRoot", "Override Android NDK root"},
        {"Editor.Build.Browse", "Browse"},
        {"Editor.Build.Clear", "Clear"},
        {"Editor.Build.Apply", "Apply"},
        {"Editor.Build.ReProbe", "Re-Probe"},
        {"Editor.Build.Configure", "Configure"},
        {"Editor.Build.Build", "Build"},
        {"Editor.Build.Clean", "Clean"},
        {"Editor.Build.Rebuild", "Rebuild"},
        {"Editor.Build.Settings", "Settings"},
        {"Editor.Build.Reveal", "Reveal"},
        {"Editor.Output.Booting", "Booting editor session..."},
        {"Editor.Output.NoOpenDocuments", "No open documents."},
        {"Editor.Project.OpenDocuments", "Open Documents"},
        {"Editor.Project.NoOpenDocuments", "No open documents"},
        {"Editor.Project.RecentFiles", "Recent Files"},
        {"Editor.Project.NoRecentFiles", "No recent files"},
        {"Editor.Project.Workspace", "Workspace"},
        {"Editor.Project.WorkspaceRootNotConfigured", "Workspace root not configured"},
        {"Editor.Project.BuildProfiles", "Build Profiles"},
        {"Editor.Project.RefreshingEnvironmentProbe", "Refreshing environment probe..."},
        {"Editor.Project.ProbeDataUnavailable", "Probe data unavailable"},
        {"Editor.Project.NoSupportedFiles", "No supported files"}
    };

    ImWidgetV4::FLocalizationManager::Get().RegisterStringTable(std::move(english));

    ImWidgetV4::FStringTable chinese;
    chinese.Culture = "zh-CN";
    chinese.Entries = {
        {"Editor.App.Title", "ImWidgetV4 编辑器"},
        {"Editor.App.InitialHint", "从左侧控件面板拖拽控件到设计画布。"},
        {"Editor.App.Action", "操作"},
        {"Editor.TitleBar.File", "文件"},
        {"Editor.TitleBar.Edit", "编辑"},
        {"Editor.TitleBar.Project", "项目"},
        {"Editor.TitleBar.Build", "构建"},
        {"Editor.TitleBar.View", "视图"},
        {"Editor.TitleBar.Search", "搜索"},
        {"Editor.TitleBar.Undo", "撤销"},
        {"Editor.TitleBar.Redo", "重做"},
        {"Editor.Dock.Controls", "控件"},
        {"Editor.Dock.Project", "项目"},
        {"Editor.Dock.Build", "构建"},
        {"Editor.Dock.WidgetTree", "控件树"},
        {"Editor.Workspace.Designer", "设计器"},
        {"Editor.Workspace.Preview", "预览"},
        {"Editor.Workspace.Schema", "Schema"},
        {"Editor.Build.NoProjectLoaded", "未加载项目。"},
        {"Editor.Build.OverviewHint", "构建/工具链概览将显示在这里。"},
        {"Editor.Build.PanelTitle", "构建"},
        {"Editor.Build.PanelBody", "工具链状态、当前配置和最近构建输出。"},
        {"Editor.Build.ActiveProfile", "当前配置"},
        {"Editor.Build.Generator", "生成器"},
        {"Editor.Build.AndroidSdkRoot", "Android SDK 根目录"},
        {"Editor.Build.AndroidNdkRoot", "Android NDK 根目录"},
        {"Editor.Build.OverrideAndroidSdkRoot", "覆盖 Android SDK 根目录"},
        {"Editor.Build.OverrideAndroidNdkRoot", "覆盖 Android NDK 根目录"},
        {"Editor.Build.Browse", "浏览"},
        {"Editor.Build.Clear", "清除"},
        {"Editor.Build.Apply", "应用"},
        {"Editor.Build.ReProbe", "重新探测"},
        {"Editor.Build.Configure", "配置"},
        {"Editor.Build.Build", "构建"},
        {"Editor.Build.Clean", "清理"},
        {"Editor.Build.Rebuild", "重新构建"},
        {"Editor.Build.Settings", "设置"},
        {"Editor.Build.Reveal", "显示"},
        {"Editor.Output.Booting", "正在启动编辑器会话..."},
        {"Editor.Output.NoOpenDocuments", "没有打开的文档。"},
        {"Editor.Project.OpenDocuments", "打开的文档"},
        {"Editor.Project.NoOpenDocuments", "没有打开的文档"},
        {"Editor.Project.RecentFiles", "最近文件"},
        {"Editor.Project.NoRecentFiles", "没有最近文件"},
        {"Editor.Project.Workspace", "工作区"},
        {"Editor.Project.WorkspaceRootNotConfigured", "尚未配置工作区根目录"},
        {"Editor.Project.BuildProfiles", "构建配置"},
        {"Editor.Project.RefreshingEnvironmentProbe", "正在刷新环境探测..."},
        {"Editor.Project.ProbeDataUnavailable", "探测数据不可用"},
        {"Editor.Project.NoSupportedFiles", "没有支持的文件"}
    };

    ImWidgetV4::FLocalizationManager::Get().RegisterStringTable(std::move(chinese));
}

} // namespace ImWidgetV4Editor
