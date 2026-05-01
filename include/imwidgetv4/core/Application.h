#pragma once
#include <imwidgetv4/core/Types.h>
#include <imwidgetv4/core/Widget.h>
#include <imwidgetv4/input/Input.h>
#include <imwidgetv4/style/StyleSet.h>
#include <memory>
#include <vector>
#include <string>
#include <cstdint>

namespace ImWidgetV4 {

/**
 * @brief Application 类 - UI 框架的核心
 *
 * ImApplication 负责管理控件树、输入处理、焦点管理和渲染循环。
 * 这是整个 UI 系统的中心协调者。
 */
class ImApplication {
public:
    /**
     * @brief 构造函数
     *
     * 初始化应用程序，创建默认样式集和主题包。
     */
    ImApplication();

    /**
     * @brief 析构函数
     */
    virtual ~ImApplication() = default;

    // ========== 控件树管理 ==========

    /**
     * @brief 设置根控件
     * @param rootWidget 根控件的共享指针
     */
    void SetRootWidget(const std::shared_ptr<ImWidget>& rootWidget);

    /**
     * @brief 获取根控件
     * @return 根控件的共享指针
     */
    const std::shared_ptr<ImWidget>& GetRootWidget() const;

    // ========== 样式系统 ==========

    /**
     * @brief 设置样式集
     * @param styleSet 样式集对象
     */
    void SetStyleSet(const FStyleSet& styleSet);

    /**
     * @brief 获取样式集（只读）
     * @return 样式集的常量引用
     */
    const FStyleSet& GetStyleSet() const;

    /**
     * @brief 获取样式集（可修改）
     * @return 样式集的引用
     */
    FStyleSet& GetStyleSet();

    // ========== 主题管理 ==========

    /**
     * @brief 注册主题包
     * @param themePack 主题包对象（右值引用）
     */
    void RegisterThemePack(FThemePack&& themePack);

    /**
     * @brief 设置活动主题
     * @param name 主题名称
     * @return 如果主题存在并成功设置返回 true，否则返回 false
     */
    bool SetActiveTheme(const std::string& name);

    /**
     * @brief 获取活动主题名称
     * @return 当前活动主题的名称
     */
    const std::string& GetActiveThemeName() const;

    /**
     * @brief 获取所有主题包
     * @return 主题包列表的常量引用
     */
    const std::vector<FThemePack>& GetThemePacks() const;

    // ========== 帧更新（核心方法） ==========

    /**
     * @brief 推进一帧
     *
     * 这是应用程序的核心方法，每帧调用一次。
     * 负责处理输入、更新布局、执行绘制。
     *
     * @param frameContext 帧上下文，包含 ImGui IO、时间信息等
     */
    void AdvanceFrame(const FFrameContext& frameContext);

    // ========== 输入处理 ==========

    /**
     * @brief 将输入事件加入队列
     * @param inputEvent 输入事件对象
     */
    void EnqueueInput(const FInputEvent& inputEvent);

    /**
     * @brief 获取上一帧的输入事件
     * @return 输入事件列表的常量引用
     */
    const std::vector<FInputEvent>& GetLastFrameEvents() const;

    // ========== 焦点管理 ==========

    /**
     * @brief 设置键盘焦点到指定控件
     * @param widget 要获得焦点的控件
     */
    void SetKeyboardFocus(const std::shared_ptr<ImWidget>& widget);

    /**
     * @brief 清除键盘焦点
     */
    void ClearKeyboardFocus();

    /**
     * @brief 获取当前拥有键盘焦点的控件
     * @return 拥有焦点的控件的共享指针
     */
    const std::shared_ptr<ImWidget>& GetKeyboardFocus() const;

    /**
     * @brief 获取焦点路径（从根到焦点控件）
     * @return 焦点路径
     */
    std::vector<std::shared_ptr<ImWidget>> GetFocusPath() const;

    // ========== 鼠标捕获 ==========

    /**
     * @brief 设置鼠标捕获到指定控件
     * @param widget 要捕获鼠标的控件
     * @param button 捕获的鼠标按钮
     */
    void SetMouseCapture(const std::shared_ptr<ImWidget>& widget, EMouseButton button);

    /**
     * @brief 释放鼠标捕获
     */
    void ReleaseMouseCapture();

    /**
     * @brief 获取当前捕获鼠标的控件
     * @return 捕获鼠标的控件的共享指针
     */
    const std::shared_ptr<ImWidget>& GetMouseCapture() const;

    /**
     * @brief 获取捕获的鼠标按钮
     * @return 捕获的鼠标按钮
     */
    EMouseButton GetCapturedMouseButton() const { return CapturedMouseButton_; }

    // ========== 帧信息 ==========

    /**
     * @brief 获取当前帧号
     * @return 帧号（从 0 开始递增）
     */
    std::uint64_t GetFrameNumber() const { return FrameNumber_; }

private:
    // ========== 控件树 ==========
    std::shared_ptr<ImWidget> RootWidget_;
    std::shared_ptr<ImWidget> SceneRoot_;

    // ========== 样式系统 ==========
    FStyleSet StyleSet_;
    std::vector<FThemePack> ThemePacks_;
    std::string ActiveThemeName_;

    // ========== 输入系统 ==========
    std::vector<FInputEvent> PendingInput_;
    std::vector<FInputEvent> LastFrameEvents_;
    FImGuiInputAdapter InputAdapter_;

    // ========== 焦点管理 ==========
    std::shared_ptr<ImWidget> FocusedWidget_;
    std::shared_ptr<ImWidget> CapturedMouseWidget_;
    EMouseButton CapturedMouseButton_ = EMouseButton::Left;

    // ========== 鼠标状态 ==========
    FVector2 LastCursorPosition_{0.0f, 0.0f};
    bool bHasCursorPosition_ = false;
    std::weak_ptr<ImWidget> LastHoveredWidget_;  // 上一帧悬停的控件

    // ========== 布局缓存 ==========
    FGeometry LastFrameGeometry_;
    bool bHasLastFrameGeometry_ = false;

    // ========== 帧计数 ==========
    std::uint64_t FrameNumber_ = 0;

    // ========== 内部方法 ==========

    /**
     * @brief 路由输入事件到控件
     *
     * 根据控件树和焦点状态，将输入事件分发到相应的控件。
     */
    void RouteInputEvents();

    /**
     * @brief 构建从根到指定控件的路径
     * @param widget 目标控件
     * @return 从根到目标控件的路径
     */
    std::vector<std::shared_ptr<ImWidget>> BuildPathToSceneRoot(const std::shared_ptr<ImWidget>& widget) const;

    /**
     * @brief 路由单个输入事件
     * @param event 输入事件
     * @param eventPath 事件路径（从根到叶）
     * @return 事件是否被处理
     */
    bool RouteEvent(const FInputEvent& event, const std::vector<std::shared_ptr<ImWidget>>& eventPath);

    /**
     * @brief 执行布局计算
     * @param frameGeometry 帧几何信息
     */
    void PerformLayoutPass(const FGeometry& frameGeometry);

    /**
     * @brief 检查是否需要重新布局
     * @param frameGeometry 当前帧的几何信息
     * @return 如果需要重新布局返回 true
     */
    bool NeedsPrepassAndArrange(const FGeometry& frameGeometry) const;
};

} // namespace ImWidgetV4
