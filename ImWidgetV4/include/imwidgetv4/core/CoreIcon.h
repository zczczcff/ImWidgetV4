#pragma once

#include <imwidgetv4/core/Types.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace ImWidgetV4 {

enum class ECoreIcon {
    Save,
    Folder,
    File,
    Copy,
    Paste,
    Cut,
    Trash,
    Undo,
    Redo,
    Search,
    Settings,
    Add,
    Remove,
    ArrowUp,
    ArrowDown,
    Download,
    Upload,
    Lock,
    Unlock,
    View,
    Check,
    Close,
    Favorite,
    Heart,
    Home,
    Refresh,
    Print,
    Info,
    Warning,
    Play,
    Pause,
    Stop,
    FastForward,
    Rewind,
    User,
    Mail,
    Cart,
    ZoomIn,
    ZoomOut,
    AddToCart,
    Bookmark,
    ExpandableBox,
    Button,
    ColorPalette,
    CheckBox,
    ComboBox,
    EditableText,
    HorizontalBox,
    Slider,
    Image,
    ListView,
    PopupMenu,
    ScrollBox,
    HorizontalSplitter,
    Switch,
    TabView,
    TextBlock,
    OutlineView,
    UserWidget,
    VerticalBox,
    VerticalSplitter,
    CanvasPanel,
    DesignerSurface,
    BoxSlot,
    Style,
    TextList,
    TextOutlineView,
    Configure,
    Generate,
    Build,
    BuildAll,
    Clean,
    Rebuild,
    Debug,
    Install,
    Test,
    CMakeFile,
    TargetSelection,
    BuildOutput,
    OpenBuildDirectory,
    CMakeCache,
    AddSourceFile,
    AddLibrary,
    AddExecutable,
    DependencyGraph,
    FindPackage,
    Properties,
    ClearCache,
    OpenTerminal,
    CompileCurrentFile,
    ProjectTree,
    Package,
    MemoryCheck,
    ParallelBuild
};

const char* GetCoreIconName(ECoreIcon icon);
std::vector<std::string> GetCoreIconNames();
bool TryParseCoreIconName(const std::string& name, ECoreIcon& outIcon);
bool BuildCoreIconRgba(
    ECoreIcon icon,
    int size,
    const FColor& tint,
    const FColor& background,
    std::vector<std::uint8_t>& outPixels);
bool ExportCoreIconIco(
    ECoreIcon icon,
    const std::filesystem::path& outputPath,
    const FColor& tint = FColor::White,
    const FColor& background = FColor::FromBytes(0, 0, 0, 0));
bool ExportCoreIconIco(
    ECoreIcon icon,
    const std::filesystem::path& outputPath,
    const std::vector<int>& sizes,
    const FColor& tint = FColor::White,
    const FColor& background = FColor::FromBytes(0, 0, 0, 0));

} // namespace ImWidgetV4
