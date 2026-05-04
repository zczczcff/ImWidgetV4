#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <algorithm>
#include <cmath>

namespace ImWidgetV4 {

class ImApplication::FInputQueue {
public:
    void Enqueue(const FInputEvent& inputEvent) {
        PendingInput_.push_back(inputEvent);
    }

    void BeginFrame(std::vector<FInputEvent>&& frameEvents) {
        LastFrameEvents_.clear();
        LastFrameEvents_.reserve(PendingInput_.size() + frameEvents.size());
        LastFrameEvents_.insert(LastFrameEvents_.end(), PendingInput_.begin(), PendingInput_.end());
        LastFrameEvents_.insert(
            LastFrameEvents_.end(),
            std::make_move_iterator(frameEvents.begin()),
            std::make_move_iterator(frameEvents.end())
        );
        PendingInput_.clear();
    }

    const std::vector<FInputEvent>& GetLastFrameEvents() const {
        return LastFrameEvents_;
    }

private:
    std::vector<FInputEvent> PendingInput_;
    std::vector<FInputEvent> LastFrameEvents_;
};

class ImApplication::FInteractionState {
public:
    std::shared_ptr<ImWidget> FocusedWidget_;
    std::shared_ptr<ImWidget> CapturedMouseWidget_;
    EMouseButton CapturedMouseButton_ = EMouseButton::Left;
    std::weak_ptr<ImWidget> HoveredWidget_;
    FVector2 LastCursorPosition_ {0.0f, 0.0f};
    bool bHasCursorPosition_ = false;
};

class ImApplication::FWidgetPathResolver {
public:
    std::vector<std::shared_ptr<ImWidget>> BuildPathToSceneRoot(
        const std::shared_ptr<ImWidget>& sceneRoot,
        const std::shared_ptr<ImWidget>& widget) const {
        std::vector<std::shared_ptr<ImWidget>> path;
        if (!sceneRoot || !widget) {
            return path;
        }

        std::shared_ptr<ImWidget> current = widget;
        while (current) {
            path.push_back(current);
            if (current == sceneRoot) {
                std::reverse(path.begin(), path.end());
                return path;
            }
            current = current->GetParent();
        }

        path.clear();
        return path;
    }

    std::vector<std::shared_ptr<ImWidget>> BuildHitTestPath(
        const std::shared_ptr<ImWidget>& sceneRoot,
        const FVector2& position) const {
        std::vector<std::shared_ptr<ImWidget>> path;
        if (!sceneRoot) {
            return path;
        }

        sceneRoot->BuildHitTestPath(position, path);
        return path;
    }
};

class ImApplication::FEventRouter {
public:
    FReply Route(const FInputEvent& event, const std::vector<std::shared_ptr<ImWidget>>& eventPath) const {
        for (const auto& widget : eventPath) {
            FReply reply = widget->OnPreviewInputEvent(event);
            if (reply.IsHandled()) {
                return reply;
            }
        }

        for (auto it = eventPath.rbegin(); it != eventPath.rend(); ++it) {
            FReply reply = (*it)->OnInputEvent(event);
            if (reply.IsHandled()) {
                return reply;
            }
        }

        return FReply::Unhandled();
    }
};

ImApplication::ImApplication()
    : InputQueue_(std::make_unique<FInputQueue>())
    , InteractionState_(std::make_unique<FInteractionState>())
    , EventRouter_(std::make_unique<FEventRouter>())
    , PathResolver_(std::make_unique<FWidgetPathResolver>()) {
    auto defaultStyleSet = FStyleSetFactory::CreateDefault();
    if (defaultStyleSet) {
        StyleSet_ = std::move(*defaultStyleSet);
    }

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

    SetActiveTheme("Default");
}

ImApplication::~ImApplication() = default;

void ImApplication::SetRootWidget(const std::shared_ptr<ImWidget>& rootWidget) {
    if (RootWidget_ == rootWidget) {
        return;
    }

    ResetInteractionState();
    RootWidget_ = rootWidget;
    SceneRoot_ = rootWidget;
}

const std::shared_ptr<ImWidget>& ImApplication::GetRootWidget() const {
    return RootWidget_;
}

void ImApplication::SetStyleSet(const FStyleSet& styleSet) {
    StyleSet_.Clear();
    StyleSet_.Merge(styleSet);
}

const FStyleSet& ImApplication::GetStyleSet() const {
    return StyleSet_;
}

FStyleSet& ImApplication::GetStyleSet() {
    return StyleSet_;
}

void ImApplication::RegisterThemePack(FThemePack&& themePack) {
    ThemePacks_.push_back(std::move(themePack));
}

bool ImApplication::SetActiveTheme(const std::string& name) {
    for (const auto& pack : ThemePacks_) {
        if (pack.Name == name) {
            ActiveThemeName_ = name;
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

void ImApplication::EnqueueInput(const FInputEvent& inputEvent) {
    InputQueue_->Enqueue(inputEvent);
}

const std::vector<FInputEvent>& ImApplication::GetLastFrameEvents() const {
    return InputQueue_->GetLastFrameEvents();
}

void ImApplication::SetKeyboardFocus(const std::shared_ptr<ImWidget>& widget) {
    if (!widget) {
        ClearKeyboardFocus();
        return;
    }

    if (!widget->SupportsKeyboardFocus()) {
        return;
    }

    if (BuildPathToSceneRoot(widget).empty()) {
        return;
    }

    if (InteractionState_->FocusedWidget_ == widget) {
        return;
    }

    if (InteractionState_->FocusedWidget_) {
        InteractionState_->FocusedWidget_->NotifyFocusChanged(false);
    }

    InteractionState_->FocusedWidget_ = widget;
    InteractionState_->FocusedWidget_->NotifyFocusChanged(true);
}

void ImApplication::ClearKeyboardFocus() {
    if (InteractionState_->FocusedWidget_) {
        InteractionState_->FocusedWidget_->NotifyFocusChanged(false);
        InteractionState_->FocusedWidget_.reset();
    }
}

const std::shared_ptr<ImWidget>& ImApplication::GetKeyboardFocus() const {
    return InteractionState_->FocusedWidget_;
}

std::vector<std::shared_ptr<ImWidget>> ImApplication::GetFocusPath() const {
    return BuildPathToSceneRoot(InteractionState_->FocusedWidget_);
}

void ImApplication::SetMouseCapture(const std::shared_ptr<ImWidget>& widget, EMouseButton button) {
    if (!widget) {
        ReleaseMouseCapture();
        return;
    }

    if (BuildPathToSceneRoot(widget).empty()) {
        return;
    }

    InteractionState_->CapturedMouseWidget_ = widget;
    InteractionState_->CapturedMouseButton_ = button;
}

void ImApplication::ReleaseMouseCapture() {
    InteractionState_->CapturedMouseWidget_.reset();
}

const std::shared_ptr<ImWidget>& ImApplication::GetMouseCapture() const {
    return InteractionState_->CapturedMouseWidget_;
}

EMouseButton ImApplication::GetCapturedMouseButton() const {
    return InteractionState_->CapturedMouseButton_;
}

void ImApplication::AdvanceFrame(const FFrameContext& frameContext) {
    ++FrameNumber_;

    InputQueue_->BeginFrame(CollectFrameInputs(frameContext));
    RouteInputEvents();
    const auto& lastFrameEvents = InputQueue_->GetLastFrameEvents();
    const bool bHadMouseEvent = std::any_of(
        lastFrameEvents.begin(),
        lastFrameEvents.end(),
        [](const FInputEvent& event) {
            return event.IsMouseEvent() &&
                   event.Type != EInputEventType::MouseEnter &&
                   event.Type != EInputEventType::MouseLeave;
        }
    );

    const FGeometry frameGeometry(
        frameContext.FrameInfo.ViewportPosition,
        frameContext.FrameInfo.ViewportSize
    );

    if (NeedsPrepassAndArrange(frameGeometry)) {
        PerformLayoutPass(frameGeometry);
    }

    if (InteractionState_->bHasCursorPosition_ && !bHadMouseEvent) {
        UpdateHoveredWidget(
            InteractionState_->LastCursorPosition_,
            frameContext.FrameInfo.CurrentTime
        );
    }

    if (frameContext.DrawContext_ != nullptr && SceneRoot_) {
        FPaintContext paintContext(
            *frameContext.DrawContext_,
            frameGeometry,
            &StyleSet_,
            InteractionState_->LastCursorPosition_,
            InteractionState_->bHasCursorPosition_,
            frameContext.FrameInfo.DeltaTime
        );
        SceneRoot_->Paint(paintContext);
    }

    LastFrameGeometry_ = frameGeometry;
    bHasLastFrameGeometry_ = true;
}

std::vector<FInputEvent> ImApplication::CollectFrameInputs(const FFrameContext& frameContext) {
    std::vector<FInputEvent> frameInputs;

    if (frameContext.InputSource != nullptr) {
        std::vector<FInputEvent> polledEvents = frameContext.InputSource->Poll(frameContext.FrameInfo);
        frameInputs.insert(
            frameInputs.end(),
            std::make_move_iterator(polledEvents.begin()),
            std::make_move_iterator(polledEvents.end())
        );
    }

    if (frameContext.InputEvents != nullptr) {
        frameInputs.insert(
            frameInputs.end(),
            frameContext.InputEvents->begin(),
            frameContext.InputEvents->end()
        );
    }

    return frameInputs;
}

void ImApplication::RouteInputEvents() {
    if (!SceneRoot_) {
        return;
    }

    for (const FInputEvent& inputEvent : InputQueue_->GetLastFrameEvents()) {
        if (inputEvent.IsMouseEvent() &&
            inputEvent.Type != EInputEventType::MouseEnter &&
            inputEvent.Type != EInputEventType::MouseLeave) {
            InteractionState_->LastCursorPosition_ = inputEvent.MousePosition;
            InteractionState_->bHasCursorPosition_ = true;

            if (inputEvent.Type == EInputEventType::MouseMove) {
                UpdateHoveredWidget(inputEvent.MousePosition, inputEvent.Timestamp);
            }
        }

        std::vector<std::shared_ptr<ImWidget>> eventPath;
        if (inputEvent.IsKeyboardEvent()) {
            eventPath = BuildPathToSceneRoot(InteractionState_->FocusedWidget_);
        } else if (inputEvent.IsMouseEvent()) {
            if (InteractionState_->CapturedMouseWidget_ &&
                inputEvent.Type != EInputEventType::MouseEnter &&
                inputEvent.Type != EInputEventType::MouseLeave) {
                eventPath = BuildPathToSceneRoot(InteractionState_->CapturedMouseWidget_);
            } else {
                eventPath = PathResolver_->BuildHitTestPath(SceneRoot_, inputEvent.MousePosition);
            }
        }

        if (!eventPath.empty()) {
            ProcessReply(RouteEvent(inputEvent, eventPath));
        }
    }
}

void ImApplication::UpdateHoveredWidget(const FVector2& cursorPosition, double timestamp) {
    if (!SceneRoot_) {
        return;
    }

    std::vector<std::shared_ptr<ImWidget>> hitPath =
        PathResolver_->BuildHitTestPath(SceneRoot_, cursorPosition);
    std::shared_ptr<ImWidget> currentHoveredWidget =
        hitPath.empty() ? nullptr : hitPath.back();
    std::shared_ptr<ImWidget> lastHoveredWidget = InteractionState_->HoveredWidget_.lock();

    if (currentHoveredWidget == lastHoveredWidget) {
        return;
    }

    if (lastHoveredWidget) {
        FInputEvent leaveEvent;
        leaveEvent.Type = EInputEventType::MouseLeave;
        leaveEvent.MousePosition = cursorPosition;
        leaveEvent.Timestamp = timestamp;

        std::vector<std::shared_ptr<ImWidget>> leavePath = BuildPathToSceneRoot(lastHoveredWidget);
        if (!leavePath.empty()) {
            ProcessReply(RouteEvent(leaveEvent, leavePath));
        }
    }

    InteractionState_->HoveredWidget_ = currentHoveredWidget;

    if (currentHoveredWidget) {
        FInputEvent enterEvent;
        enterEvent.Type = EInputEventType::MouseEnter;
        enterEvent.MousePosition = cursorPosition;
        enterEvent.Timestamp = timestamp;

        ProcessReply(RouteEvent(enterEvent, hitPath));
    }
}

void ImApplication::ProcessReply(const FReply& reply) {
    if (reply.bReleaseMouseCapture) {
        ReleaseMouseCapture();
    }
    if (reply.MouseCaptureTarget) {
        SetMouseCapture(reply.MouseCaptureTarget, reply.MouseCaptureButton);
    }

    if (reply.bClearKeyboardFocus) {
        ClearKeyboardFocus();
    }
    if (reply.FocusTarget) {
        SetKeyboardFocus(reply.FocusTarget);
    }
}

void ImApplication::ResetInteractionState() {
    ClearKeyboardFocus();
    ReleaseMouseCapture();
    InteractionState_->HoveredWidget_.reset();
    InteractionState_->bHasCursorPosition_ = false;
    InteractionState_->LastCursorPosition_ = FVector2(0.0f, 0.0f);
}

std::vector<std::shared_ptr<ImWidget>> ImApplication::BuildPathToSceneRoot(
    const std::shared_ptr<ImWidget>& widget) const {
    return PathResolver_->BuildPathToSceneRoot(SceneRoot_, widget);
}

FReply ImApplication::RouteEvent(
    const FInputEvent& event,
    const std::vector<std::shared_ptr<ImWidget>>& eventPath) {
    return EventRouter_->Route(event, eventPath);
}

bool ImApplication::NeedsPrepassAndArrange(const FGeometry& frameGeometry) const {
    if (!bHasLastFrameGeometry_) {
        return true;
    }

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

    RootWidget_->SetGeometry(frameGeometry);
}

} // namespace ImWidgetV4
