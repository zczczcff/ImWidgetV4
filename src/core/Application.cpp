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
    // 1. 验证控件是否支持焦点
    if (widget && !widget->SupportsKeyboardFocus()) {
        return;
    }

    // 2. 验证控件是否在场景树中（通过 BuildPathToSceneRoot 验证）
    if (widget) {
        auto path = BuildPathToSceneRoot(widget);
        if (path.empty()) {
            return;  // 控件不在场景树中
        }
    }

    // 3. 如果焦点没有变化，直接返回
    if (FocusedWidget_ == widget) {
        return;
    }

    // 4. 通知旧控件失去焦点
    if (FocusedWidget_) {
        FocusedWidget_->NotifyFocusChanged(false);
    }

    // 5. 更新焦点控件
    FocusedWidget_ = widget;

    // 6. 通知新控件获得焦点
    if (FocusedWidget_) {
        FocusedWidget_->NotifyFocusChanged(true);
    }
}

void ImApplication::ClearKeyboardFocus() {
    FocusedWidget_.reset();
}

const std::shared_ptr<ImWidget>& ImApplication::GetKeyboardFocus() const {
    return FocusedWidget_;
}

std::vector<std::shared_ptr<ImWidget>> ImApplication::GetFocusPath() const {
    if (FocusedWidget_) {
        return BuildPathToSceneRoot(FocusedWidget_);
    }
    return {};
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

    // 4.5. 更新悬停状态（基于当前光标位置进行命中测试）
    if (bHasCursorPosition_ && SceneRoot_) {
        // 执行命中测试，找到当前光标下的控件
        std::vector<std::shared_ptr<ImWidget>> hitPath;
        SceneRoot_->BuildHitTestPath(LastCursorPosition_, hitPath);

        // 获取最深的控件（如果有）
        std::shared_ptr<ImWidget> currentHoveredWidget;
        if (!hitPath.empty()) {
            currentHoveredWidget = hitPath.back();
        }

        // 获取上一帧悬停的控件
        std::shared_ptr<ImWidget> lastHoveredWidget = LastHoveredWidget_.lock();

        // 如果悬停控件发生变化
        if (currentHoveredWidget != lastHoveredWidget) {
            // 清除上一帧悬停控件的悬停状态
            if (lastHoveredWidget) {
                // 创建 MouseLeave 事件
                FInputEvent leaveEvent;
                leaveEvent.Type = EInputEventType::MouseMove;
                leaveEvent.MousePosition = LastCursorPosition_;

                // 通知控件鼠标离开（通过 OnInputEvent）
                lastHoveredWidget->OnInputEvent(leaveEvent);
            }

            // 更新当前悬停控件
            LastHoveredWidget_ = currentHoveredWidget;
        }
    }

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
    if (!RootWidget_) {
        return;
    }

    // 更新根控件的几何信息
    // 根控件应该填充整个视口
    RootWidget_->SetGeometry(frameGeometry);

    // 注意：布局更新会在 Paint 方法中自动触发
    // HorizontalBox、VerticalBox 和 Button 等控件会在 Paint 中调用 Relayout()
}

} // namespace ImWidgetV4
