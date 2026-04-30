#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <cmath>

namespace ImWidgetV4 {

// ========== 构造函数 ==========

ImApplication::ImApplication() {
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

void ImApplication::SetRootWidget(const std::shared_ptr<ImWidget>& rootWidget) {
    RootWidget_ = rootWidget;
    SceneRoot_ = rootWidget;  // 简化版：直接使用根控件
}

const std::shared_ptr<ImWidget>& ImApplication::GetRootWidget() const {
    return RootWidget_;
}

// ========== 样式系统 ==========

void ImApplication::SetStyleSet(const FStyleSet& styleSet) {
    // FStyleSet 禁止拷贝，需要使用 Merge 方法
    StyleSet_.Clear();
    StyleSet_.Merge(styleSet);
}

const FStyleSet& ImApplication::GetStyleSet() const {
    return StyleSet_;
}

FStyleSet& ImApplication::GetStyleSet() {
    return StyleSet_;
}

// ========== 主题管理 ==========

void ImApplication::RegisterThemePack(FThemePack&& themePack) {
    ThemePacks_.push_back(std::move(themePack));
}

bool ImApplication::SetActiveTheme(const std::string& name) {
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

const std::string& ImApplication::GetActiveThemeName() const {
    return ActiveThemeName_;
}

const std::vector<FThemePack>& ImApplication::GetThemePacks() const {
    return ThemePacks_;
}

// ========== 输入处理 ==========

void ImApplication::EnqueueInput(const FInputEvent& inputEvent) {
    PendingInput_.push_back(inputEvent);
}

const std::vector<FInputEvent>& ImApplication::GetLastFrameEvents() const {
    return LastFrameEvents_;
}

void ImApplication::RouteInputEvents() {
    if (!SceneRoot_) {
        return;
    }

    // 遍历所有待处理的输入事件
    for (const auto& inputEvent : LastFrameEvents_) {
        std::vector<std::shared_ptr<ImWidget>> eventPath;

        // 根据事件类型确定事件路径
        if (inputEvent.IsMouseEvent()) {
            // 鼠标事件：使用命中测试或鼠标捕获
            if (CapturedMouseWidget_) {
                // 如果有鼠标捕获，事件路由到捕获的控件
                eventPath = BuildPathToSceneRoot(CapturedMouseWidget_);
            } else {
                // 否则执行命中测试
                SceneRoot_->BuildHitTestPath(inputEvent.MousePosition, eventPath);
            }

            // 处理鼠标捕获逻辑
            if (inputEvent.Type == EInputEventType::MouseButtonDown) {
                // 鼠标按下时捕获最深的控件
                if (!eventPath.empty()) {
                    SetMouseCapture(eventPath.back(), inputEvent.MouseButton);
                }
            } else if (inputEvent.Type == EInputEventType::MouseButtonUp) {
                // 鼠标释放时释放捕获
                if (CapturedMouseWidget_ && inputEvent.MouseButton == CapturedMouseButton_) {
                    ReleaseMouseCapture();
                }
            }
        } else if (inputEvent.IsKeyboardEvent()) {
            // 键盘事件：使用焦点路径
            if (FocusedWidget_) {
                eventPath = BuildPathToSceneRoot(FocusedWidget_);
            }
        }

        // 路由事件到控件
        if (!eventPath.empty()) {
            RouteEvent(inputEvent, eventPath);
        }
    }
}

std::vector<std::shared_ptr<ImWidget>> ImApplication::BuildPathToSceneRoot(
    const std::shared_ptr<ImWidget>& widget) const {
    std::vector<std::shared_ptr<ImWidget>> path;

    if (!widget || !SceneRoot_) {
        return path;
    }

    // 简化版：暂时只返回单个控件
    // 完整实现需要遍历控件树构建完整路径
    path.push_back(widget);

    return path;
}

bool ImApplication::RouteEvent(const FInputEvent& event,
                               const std::vector<std::shared_ptr<ImWidget>>& eventPath) {
    bool handled = false;

    // 阶段 1: 预览阶段（从根到叶）
    for (const auto& widget : eventPath) {
        if (widget->OnPreviewInputEvent(event).IsHandled()) {
            handled = true;
            break;
        }
    }

    // 阶段 2: 正常阶段（从叶到根）
    if (!handled) {
        for (auto it = eventPath.rbegin(); it != eventPath.rend(); ++it) {
            if ((*it)->OnInputEvent(event).IsHandled()) {
                handled = true;
                break;
            }
        }
    }

    return handled;
}

// ========== 焦点管理 ==========

void ImApplication::SetKeyboardFocus(const std::shared_ptr<ImWidget>& widget) {
    FocusedWidget_ = widget;
}

void ImApplication::ClearKeyboardFocus() {
    FocusedWidget_.reset();
}

const std::shared_ptr<ImWidget>& ImApplication::GetKeyboardFocus() const {
    return FocusedWidget_;
}

void ImApplication::SetMouseCapture(const std::shared_ptr<ImWidget>& widget, EMouseButton button) {
    CapturedMouseWidget_ = widget;
    CapturedMouseButton_ = button;
}

void ImApplication::ReleaseMouseCapture() {
    CapturedMouseWidget_.reset();
}

const std::shared_ptr<ImWidget>& ImApplication::GetMouseCapture() const {
    return CapturedMouseWidget_;
}

// ========== 帧更新（核心方法） ==========

void ImApplication::AdvanceFrame(const FFrameContext& frameContext) {
    ++FrameNumber_;

    // 1. 从 ImGui 轮询输入事件
    if (frameContext.ImGuiIo) {
        auto events = InputAdapter_.Poll(*frameContext.ImGuiIo, frameContext.FrameInfo.CurrentTime);
        for (const auto& event : events) {
            PendingInput_.push_back(event);
        }
    }

    // 2. 处理输入事件
    LastFrameEvents_ = PendingInput_;
    PendingInput_.clear();

    // 3. 更新鼠标位置
    if (frameContext.ImGuiIo) {
        LastCursorPosition_ = FVector2(
            frameContext.ImGuiIo->MousePos.x,
            frameContext.ImGuiIo->MousePos.y
        );
        bHasCursorPosition_ = true;
    }

    // 4. 路由输入事件到控件
    RouteInputEvents();

    // 5. 布局计算
    const FGeometry frameGeometry(
        frameContext.FrameInfo.ViewportPosition,
        frameContext.FrameInfo.ViewportSize
    );

    if (NeedsPrepassAndArrange(frameGeometry)) {
        PerformLayoutPass(frameGeometry);
    }

    // 6. 绘制
    if (frameContext.DrawList && SceneRoot_) {
        // 创建 DrawContext
        DrawContext drawContext(frameContext.DrawList);

        // 创建 PaintContext
        FPaintContext paintContext(
            drawContext,
            frameGeometry,
            &StyleSet_,
            LastCursorPosition_,
            bHasCursorPosition_,
            frameContext.FrameInfo.DeltaTime
        );

        // 调用根控件的 Paint 方法
        SceneRoot_->Paint(paintContext);
    }

    // 7. 保存几何信息
    LastFrameGeometry_ = frameGeometry;
    bHasLastFrameGeometry_ = true;
}

bool ImApplication::NeedsPrepassAndArrange(const FGeometry& frameGeometry) const {
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

void ImApplication::PerformLayoutPass(const FGeometry& frameGeometry) {
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
