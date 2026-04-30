#include <gtest/gtest.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imgui.h>

using namespace ImWidgetV4;

/**
 * @brief ImButton 测试套件
 *
 * 测试按钮控件的各种功能：
 * - 基础功能（构造、文本、样式）
 * - 状态管理（悬停、按下、禁用）
 * - 事件处理（点击、悬停、键盘）
 * - 尺寸计算
 */
class ButtonTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化 ImGui 上下文（测试环境需要）
        if (!ImGui::GetCurrentContext()) {
            ImGui::CreateContext();

            // 初始化 ImGui 的 IO 和字体系统
            ImGuiIO& io = ImGui::GetIO();
            io.Fonts->AddFontDefault();
            io.Fonts->Build();

            // 创建一个虚拟的字体纹理（测试环境不需要真实纹理）
            unsigned char* pixels;
            int width, height;
            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

            // 设置显示尺寸（必需，否则 NewFrame 会失败）
            io.DisplaySize = ImVec2(1920, 1080);
            io.DeltaTime = 1.0f / 60.0f;
        }

        // 开始新帧（这会初始化字体指针和其他内部状态）
        ImGui::NewFrame();

        // 创建按钮实例
        m_Button = std::make_shared<ImButton>();
    }

    void TearDown() override {
        m_Button.reset();

        // 结束当前帧（清理 ImGui 状态）
        if (ImGui::GetCurrentContext()) {
            ImGui::EndFrame();
        }
    }

    // 辅助方法：创建鼠标事件
    FInputEvent CreateMouseEvent(EInputEventType type, const FVector2& position,
                                 EMouseButton button = EMouseButton::Left) {
        FInputEvent event;
        event.Type = type;
        event.MousePosition = position;
        event.MouseButton = button;
        event.Timestamp = 0.0;
        return event;
    }

    // 辅助方法：创建键盘事件
    FInputEvent CreateKeyEvent(EInputEventType type, ImGuiKey key) {
        FInputEvent event;
        event.Type = type;
        event.Key = key;
        event.Timestamp = 0.0;
        return event;
    }

    std::shared_ptr<ImButton> m_Button;
};

// ==================== 基础功能测试 ====================

TEST_F(ButtonTest, Construction) {
    // 测试按钮构造
    EXPECT_NE(m_Button, nullptr);
    EXPECT_TRUE(m_Button->IsVisible());
    EXPECT_TRUE(m_Button->IsHitTestVisible());
    EXPECT_TRUE(m_Button->SupportsKeyboardFocus());
    EXPECT_FALSE(m_Button->IsDisabled());
    EXPECT_FALSE(m_Button->IsHovered());
    EXPECT_FALSE(m_Button->IsPressed());
}

TEST_F(ButtonTest, TextProperty) {
    // 测试文本属性
    EXPECT_EQ(m_Button->GetText(), "");

    m_Button->SetText("Click Me");
    EXPECT_EQ(m_Button->GetText(), "Click Me");

    m_Button->SetText("New Text");
    EXPECT_EQ(m_Button->GetText(), "New Text");
}

TEST_F(ButtonTest, DisabledState) {
    // 测试禁用状态
    EXPECT_FALSE(m_Button->IsDisabled());

    m_Button->SetDisabled(true);
    EXPECT_TRUE(m_Button->IsDisabled());

    m_Button->SetDisabled(false);
    EXPECT_FALSE(m_Button->IsDisabled());
}

// ==================== 样式测试 ====================

TEST_F(ButtonTest, DefaultStyle) {
    // 测试默认样式
    const FButtonStyle& style = m_Button->GetStyle();

    // 验证默认样式存在
    EXPECT_NE(style.Normal.BackgroundColor.R, 0.0f);
    EXPECT_NE(style.Hovered.BackgroundColor.R, 0.0f);
    EXPECT_NE(style.Pressed.BackgroundColor.R, 0.0f);
    EXPECT_NE(style.Disabled.BackgroundColor.R, 0.0f);
}

TEST_F(ButtonTest, CustomStyle) {
    // 测试自定义样式
    FButtonStyle customStyle = FButtonStyle::CreatePrimary();
    m_Button->SetStyle(customStyle);

    const FButtonStyle& style = m_Button->GetStyle();
    EXPECT_EQ(style.Normal.BackgroundColor.R, customStyle.Normal.BackgroundColor.R);
    EXPECT_EQ(style.Normal.BackgroundColor.G, customStyle.Normal.BackgroundColor.G);
    EXPECT_EQ(style.Normal.BackgroundColor.B, customStyle.Normal.BackgroundColor.B);
}

TEST_F(ButtonTest, IndividualStateStyles) {
    // 测试单独设置各状态样式
    FButtonStateStyle normalStyle(
        FColor::FromBytes(255, 0, 0, 255),
        FColor::FromBytes(200, 0, 0, 255),
        FColor::FromBytes(255, 255, 255, 255),
        1.0f, 4.0f, false
    );

    m_Button->SetNormalStyle(normalStyle);

    const FButtonStyle& style = m_Button->GetStyle();
    EXPECT_FLOAT_EQ(style.Normal.BackgroundColor.R, 1.0f);
    EXPECT_FLOAT_EQ(style.Normal.BackgroundColor.G, 0.0f);
    EXPECT_FLOAT_EQ(style.Normal.BackgroundColor.B, 0.0f);
}

// ==================== 事件处理测试 ====================

TEST_F(ButtonTest, ClickEvent) {
    // 测试点击事件
    bool clicked = false;
    m_Button->SetOnClicked([&]() { clicked = true; });

    // 设置按钮几何信息
    m_Button->SetGeometry(FGeometry(FVector2(0, 0), FVector2(100, 30)));

    // 模拟鼠标按下
    FInputEvent downEvent = CreateMouseEvent(
        EInputEventType::MouseButtonDown,
        FVector2(50, 15),
        EMouseButton::Left
    );
    FReply downReply = m_Button->OnInputEvent(downEvent);
    EXPECT_TRUE(downReply.IsHandled());
    EXPECT_TRUE(m_Button->IsPressed());
    EXPECT_FALSE(clicked);  // 按下时不应触发点击

    // 模拟鼠标释放
    FInputEvent upEvent = CreateMouseEvent(
        EInputEventType::MouseButtonUp,
        FVector2(50, 15),
        EMouseButton::Left
    );
    FReply upReply = m_Button->OnInputEvent(upEvent);
    EXPECT_TRUE(upReply.IsHandled());
    EXPECT_FALSE(m_Button->IsPressed());
    EXPECT_TRUE(clicked);  // 释放时应触发点击
}

TEST_F(ButtonTest, ClickOutsideDoesNotTrigger) {
    // 测试在按钮外释放不触发点击
    bool clicked = false;
    m_Button->SetOnClicked([&]() { clicked = true; });

    m_Button->SetGeometry(FGeometry(FVector2(0, 0), FVector2(100, 30)));

    // 在按钮内按下
    FInputEvent downEvent = CreateMouseEvent(
        EInputEventType::MouseButtonDown,
        FVector2(50, 15),
        EMouseButton::Left
    );
    m_Button->OnInputEvent(downEvent);
    EXPECT_TRUE(m_Button->IsPressed());

    // 在按钮外释放
    FInputEvent upEvent = CreateMouseEvent(
        EInputEventType::MouseButtonUp,
        FVector2(200, 200),
        EMouseButton::Left
    );
    m_Button->OnInputEvent(upEvent);
    EXPECT_FALSE(m_Button->IsPressed());
    EXPECT_FALSE(clicked);  // 不应触发点击
}

TEST_F(ButtonTest, DisabledButtonDoesNotRespond) {
    // 测试禁用按钮不响应事件
    bool clicked = false;
    m_Button->SetOnClicked([&]() { clicked = true; });
    m_Button->SetDisabled(true);

    m_Button->SetGeometry(FGeometry(FVector2(0, 0), FVector2(100, 30)));

    // 尝试点击
    FInputEvent downEvent = CreateMouseEvent(
        EInputEventType::MouseButtonDown,
        FVector2(50, 15),
        EMouseButton::Left
    );
    FReply reply = m_Button->OnInputEvent(downEvent);
    EXPECT_FALSE(reply.IsHandled());
    EXPECT_FALSE(m_Button->IsPressed());
    EXPECT_FALSE(clicked);
}

TEST_F(ButtonTest, HoverEvents) {
    // 测试悬停事件
    bool hoverBegin = false;
    bool hoverEnd = false;
    m_Button->SetOnHoverBegin([&]() { hoverBegin = true; });
    m_Button->SetOnHoverEnd([&]() { hoverEnd = true; });

    m_Button->SetGeometry(FGeometry(FVector2(0, 0), FVector2(100, 30)));

    // 鼠标进入按钮
    FInputEvent enterEvent = CreateMouseEvent(
        EInputEventType::MouseMove,
        FVector2(50, 15)
    );
    m_Button->OnInputEvent(enterEvent);
    EXPECT_TRUE(m_Button->IsHovered());
    EXPECT_TRUE(hoverBegin);
    EXPECT_FALSE(hoverEnd);

    // 鼠标离开按钮
    hoverBegin = false;
    FInputEvent leaveEvent = CreateMouseEvent(
        EInputEventType::MouseMove,
        FVector2(200, 200)
    );
    m_Button->OnInputEvent(leaveEvent);
    EXPECT_FALSE(m_Button->IsHovered());
    EXPECT_FALSE(hoverBegin);
    EXPECT_TRUE(hoverEnd);
}

TEST_F(ButtonTest, KeyboardActivation) {
    // 测试键盘激活（Enter 和 Space 键）
    bool clicked = false;
    m_Button->SetOnClicked([&]() { clicked = true; });
    m_Button->SetHasKeyboardFocus(true);

    // 测试 Enter 键
    FInputEvent enterEvent = CreateKeyEvent(EInputEventType::KeyDown, ImGuiKey_Enter);
    FReply enterReply = m_Button->OnInputEvent(enterEvent);
    EXPECT_TRUE(enterReply.IsHandled());
    EXPECT_TRUE(clicked);

    // 测试 Space 键
    clicked = false;
    FInputEvent spaceEvent = CreateKeyEvent(EInputEventType::KeyDown, ImGuiKey_Space);
    FReply spaceReply = m_Button->OnInputEvent(spaceEvent);
    EXPECT_TRUE(spaceReply.IsHandled());
    EXPECT_TRUE(clicked);
}

TEST_F(ButtonTest, KeyboardWithoutFocus) {
    // 测试没有焦点时键盘不触发
    bool clicked = false;
    m_Button->SetOnClicked([&]() { clicked = true; });
    m_Button->SetHasKeyboardFocus(false);

    FInputEvent enterEvent = CreateKeyEvent(EInputEventType::KeyDown, ImGuiKey_Enter);
    FReply reply = m_Button->OnInputEvent(enterEvent);
    EXPECT_FALSE(reply.IsHandled());
    EXPECT_FALSE(clicked);
}

TEST_F(ButtonTest, PressedAndReleasedCallbacks) {
    // 测试按下和释放回调
    bool pressed = false;
    bool released = false;
    m_Button->SetOnPressed([&]() { pressed = true; });
    m_Button->SetOnReleased([&]() { released = true; });

    m_Button->SetGeometry(FGeometry(FVector2(0, 0), FVector2(100, 30)));

    // 按下
    FInputEvent downEvent = CreateMouseEvent(
        EInputEventType::MouseButtonDown,
        FVector2(50, 15),
        EMouseButton::Left
    );
    m_Button->OnInputEvent(downEvent);
    EXPECT_TRUE(pressed);
    EXPECT_FALSE(released);

    // 释放
    pressed = false;
    FInputEvent upEvent = CreateMouseEvent(
        EInputEventType::MouseButtonUp,
        FVector2(50, 15),
        EMouseButton::Left
    );
    m_Button->OnInputEvent(upEvent);
    EXPECT_FALSE(pressed);
    EXPECT_TRUE(released);
}

// ==================== 尺寸计算测试 ====================

TEST_F(ButtonTest, MinSizeWithoutText) {
    // 测试无文本时的最小尺寸
    FVector2 minSize = m_Button->GetMinSize();
    EXPECT_GE(minSize.X, 0.0f);
    EXPECT_GE(minSize.Y, 0.0f);
}

TEST_F(ButtonTest, MinSizeWithText) {
    // 测试有文本时的最小尺寸
    m_Button->SetText("Click Me");
    FVector2 minSize = m_Button->GetMinSize();

    // 文本尺寸应该大于默认最小尺寸
    EXPECT_GT(minSize.X, 0.0f);
    EXPECT_GT(minSize.Y, 0.0f);

    // 更长的文本应该有更大的尺寸
    m_Button->SetText("This is a much longer button text");
    FVector2 longerMinSize = m_Button->GetMinSize();
    EXPECT_GT(longerMinSize.X, minSize.X);
}

// ==================== 状态优先级测试 ====================

TEST_F(ButtonTest, StatePriority) {
    // 测试状态优先级：Disabled > Pressed > Hovered > Normal
    m_Button->SetGeometry(FGeometry(FVector2(0, 0), FVector2(100, 30)));

    // 设置不同颜色的样式以便区分
    FButtonStyle style;
    style.Normal.BackgroundColor = FColor::FromBytes(255, 255, 255, 255);    // 白色
    style.Hovered.BackgroundColor = FColor::FromBytes(200, 200, 200, 255);   // 浅灰
    style.Pressed.BackgroundColor = FColor::FromBytes(150, 150, 150, 255);   // 中灰
    style.Disabled.BackgroundColor = FColor::FromBytes(100, 100, 100, 255);  // 深灰
    m_Button->SetStyle(style);

    // 正常状态
    // 注意：GetCurrentStateStyle 是 protected 方法，我们通过状态标志间接测试

    // 悬停状态
    FInputEvent hoverEvent = CreateMouseEvent(EInputEventType::MouseMove, FVector2(50, 15));
    m_Button->OnInputEvent(hoverEvent);
    EXPECT_TRUE(m_Button->IsHovered());

    // 按下状态（应该覆盖悬停）
    FInputEvent downEvent = CreateMouseEvent(EInputEventType::MouseButtonDown, FVector2(50, 15));
    m_Button->OnInputEvent(downEvent);
    EXPECT_TRUE(m_Button->IsPressed());
    EXPECT_TRUE(m_Button->IsHovered());

    // 禁用状态（应该覆盖所有其他状态）
    m_Button->SetDisabled(true);
    EXPECT_TRUE(m_Button->IsDisabled());
    EXPECT_TRUE(m_Button->IsPressed());  // 状态标志仍然存在
    EXPECT_TRUE(m_Button->IsHovered());  // 状态标志仍然存在
}

// ==================== 主函数 ====================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
