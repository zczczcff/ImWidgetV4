#include <imwidgetv4/core/Application.h>
#include <cmath>

namespace ImWidgetV4 {

// ========== 构造函数 ==========

FApplication::FApplication() {
    // 初始化默认样式集
    auto defaultStyleSet = FStyleSetFactory::CreateDefault();
    if (defaultStyleSet) {
        StyleSet_ = std::move(*defaultStyleSet);
    }

    // 注册默认主题
    {
        FThemePack defaultTheme("Default");
        auto styleSet = FStyleSetFactory::CreateDefault();
        if (styleSet) {
            defaultTheme.StyleSet = std::move(*styleSet);
        }
        RegisterThemePack(std::move(defaultTheme));
    }

    {
        FThemePack darkTheme("Dark");
        auto styleSet = FStyleSetFactory::CreateDarkTheme();
        if (styleSet) {
            darkTheme.StyleSet = std::move(*styleSet);
        }
        RegisterThemePack(std::move(darkTheme));
    }

    {
        FThemePack lightTheme("Light");
        auto styleSet = FStyleSetFactory::CreateLightTheme();
        if (styleSet) {
            lightTheme.StyleSet = std::move(*styleSet);
        }
        RegisterThemePack(std::move(lightTheme));
    }

    // 设置默认主题
    SetActiveTheme("Default");
}

// ========== 控件树管理 ==========

void FApplication::SetRootWidget(const std::shared_ptr<SWidget>& rootWidget) {
    RootWidget_ = rootWidget;
    SceneRoot_ = rootWidget;  // 简化版：直接使用根控件
}

const std::shared_ptr<SWidget>& FApplication::GetRootWidget() const {
    return RootWidget_;
}

// ========== 样式系统 ==========

void FApplication::SetStyleSet(const FStyleSet& styleSet) {
    // FStyleSet 禁止拷贝，需要使用 Merge 方法
    StyleSet_.Clear();
    StyleSet_.Merge(styleSet);
}

const FStyleSet& FApplication::GetStyleSet() const {
    return StyleSet_;
}

FStyleSet& FApplication::GetStyleSet() {
    return StyleSet_;
}

// ========== 主题管理 ==========

void FApplication::RegisterThemePack(FThemePack&& themePack) {
    ThemePacks_.push_back(std::move(themePack));
}

bool FApplication::SetActiveTheme(const std::string& name) {
    for (const auto& pack : ThemePacks_) {
        if (pack.Name == name) {
            ActiveThemeName_ = name;
            // FStyleSet 禁止拷贝，需要使用 Merge 方法
            StyleSet_.Clear();
            StyleSet_.Merge(pack.StyleSet);
            return true;
        }
    }
    return false;
}

const std::string& FApplication::GetActiveThemeName() const {
    return ActiveThemeName_;
}

const std::vector<FThemePack>& FApplication::GetThemePacks() const {
    return ThemePacks_;
}

// ========== 输入处理 ==========

void FApplication::EnqueueInput(const FInputEvent& inputEvent) {
    PendingInput_.push_back(inputEvent);
}

const std::vector<FInputEvent>& FApplication::GetLastFrameEvents() const {
    return LastFrameEvents_;
}

void FApplication::RouteInputEvents() {
    // 简化版：暂时不路由事件到控件
    // 后续实现时会根据控件树和焦点状态路由事件
    //
    // 完整实现将包括：
    // 1. 检查鼠标捕获控件
    // 2. 检查焦点控件
    // 3. 执行命中测试
    // 4. 将事件分发到相应控件
}

// ========== 焦点管理 ==========

void FApplication::SetKeyboardFocus(const std::shared_ptr<SWidget>& widget) {
    FocusedWidget_ = widget;
}

void FApplication::ClearKeyboardFocus() {
    FocusedWidget_.reset();
}

const std::shared_ptr<SWidget>& FApplication::GetKeyboardFocus() const {
    return FocusedWidget_;
}

void FApplication::SetMouseCapture(const std::shared_ptr<SWidget>& widget) {
    CapturedMouseWidget_ = widget;
}

void FApplication::ReleaseMouseCapture() {
    CapturedMouseWidget_.reset();
}

const std::shared_ptr<SWidget>& FApplication::GetMouseCapture() const {
    return CapturedMouseWidget_;
}

// ========== 帧更新（核心方法） ==========

void FApplication::AdvanceFrame(const FFrameContext& frameContext) {
    ++FrameNumber_;

    // 1. 处理输入事件
    LastFrameEvents_ = PendingInput_;
    PendingInput_.clear();

    // 2. 更新鼠标位置
    if (frameContext.ImGuiIo) {
        LastCursorPosition_ = FVector2(
            frameContext.ImGuiIo->MousePos.x,
            frameContext.ImGuiIo->MousePos.y
        );
        bHasCursorPosition_ = true;
    }

    // 3. 路由输入事件到控件
    RouteInputEvents();

    // 4. 布局计算
    const FGeometry frameGeometry(
        frameContext.FrameInfo.ViewportPosition,
        frameContext.FrameInfo.ViewportSize
    );

    if (NeedsPrepassAndArrange(frameGeometry)) {
        PerformLayoutPass(frameGeometry);
    }

    // 5. 绘制（简化版：暂时不绘制）
    // 后续实现时会调用 SceneRoot_->Paint(paintContext)
    //
    // 完整实现将包括：
    // 1. 创建绘制上下文
    // 2. 调用根控件的 Paint 方法
    // 3. 收集绘制命令
    // 4. 提交到 ImGui

    // 6. 保存几何信息
    LastFrameGeometry_ = frameGeometry;
    bHasLastFrameGeometry_ = true;
}

bool FApplication::NeedsPrepassAndArrange(const FGeometry& frameGeometry) const {
    // 简化版：如果没有上一帧的几何信息，或者几何信息发生变化，则需要重新布局
    if (!bHasLastFrameGeometry_) {
        return true;
    }

    // 检查视口大小是否变化
    const float epsilon = 0.01f;
    if (std::abs(frameGeometry.Size.X - LastFrameGeometry_.Size.X) > epsilon ||
        std::abs(frameGeometry.Size.Y - LastFrameGeometry_.Size.Y) > epsilon) {
        return true;
    }

    return false;
}

void FApplication::PerformLayoutPass(const FGeometry& frameGeometry) {
    // 简化版：暂时不执行布局
    // 后续实现时会调用控件的布局方法
    //
    // 完整实现将包括：
    // 1. 计算期望尺寸（Prepass）
    // 2. 排列子控件（Arrange）
    // 3. 更新控件几何信息
    // 4. 处理滚动和裁剪
}

} // namespace ImWidgetV4
