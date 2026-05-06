#include <gtest/gtest.h>
#include <imwidgetv4/platform/ImGuiInputSource.h>

using namespace ImWidgetV4;

TEST(InputSourceTest, MouseMoveDoesNotRepeatWithoutStateChange) {
    FImGuiInputSource inputSource;
    FFrameInfo frameInfo;
    frameInfo.CurrentTime = 1.0;

    FImGuiInputSnapshot snapshot;
    snapshot.MousePosition = FVector2(10.0f, 20.0f);
    snapshot.bHasMousePosition = true;
    inputSource.SetSnapshot(snapshot);

    std::vector<FInputEvent> firstEvents = inputSource.Poll(frameInfo);
    ASSERT_EQ(firstEvents.size(), 1u);
    EXPECT_EQ(firstEvents[0].Type, EInputEventType::MouseMove);

    inputSource.SetSnapshot(snapshot);
    std::vector<FInputEvent> secondEvents = inputSource.Poll(frameInfo);
    EXPECT_TRUE(secondEvents.empty());
}

TEST(InputSourceTest, MouseButtonsWheelAndModifiersAreConverted) {
    FImGuiInputSource inputSource;
    FFrameInfo frameInfo;
    frameInfo.CurrentTime = 2.0;

    FImGuiInputSnapshot snapshot;
    snapshot.MousePosition = FVector2(30.0f, 40.0f);
    snapshot.bHasMousePosition = true;
    snapshot.MouseButtons[0] = true;
    snapshot.MouseWheelDelta = FVector2(1.0f, -2.0f);
    snapshot.Modifiers = FInputModifiers(true, true, false, false);
    inputSource.SetSnapshot(snapshot);

    std::vector<FInputEvent> events = inputSource.Poll(frameInfo);
    ASSERT_EQ(events.size(), 3u);
    EXPECT_EQ(events[0].Type, EInputEventType::MouseMove);
    EXPECT_EQ(events[1].Type, EInputEventType::MouseButtonDown);
    EXPECT_EQ(events[1].MouseButton, EMouseButton::Left);
    EXPECT_TRUE(events[1].Modifiers.bCtrl);
    EXPECT_TRUE(events[1].Modifiers.bShift);
    EXPECT_EQ(events[2].Type, EInputEventType::MouseWheel);
    EXPECT_FLOAT_EQ(events[2].ScrollDelta.X, 1.0f);
    EXPECT_FLOAT_EQ(events[2].ScrollDelta.Y, -2.0f);
}

TEST(InputSourceTest, KeyEdgesAndTextInputAreConverted) {
    FImGuiInputSource inputSource;
    FFrameInfo frameInfo;
    frameInfo.CurrentTime = 3.0;

    FImGuiInputSnapshot snapshot;
    snapshot.Keys[static_cast<std::size_t>(EKey::Enter)] = true;
    snapshot.TextInput = {static_cast<unsigned int>('A')};
    inputSource.SetSnapshot(snapshot);

    std::vector<FInputEvent> events = inputSource.Poll(frameInfo);
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].Type, EInputEventType::KeyDown);
    EXPECT_EQ(events[0].Key, EKey::Enter);
    EXPECT_NE(events[0].NativeKeyCode, 0);
    EXPECT_EQ(events[1].Type, EInputEventType::TextInput);
    EXPECT_EQ(events[1].Codepoint, static_cast<unsigned int>('A'));

    inputSource.SetSnapshot(snapshot);
    std::vector<FInputEvent> repeatEvents = inputSource.Poll(frameInfo);
    ASSERT_EQ(repeatEvents.size(), 1u);
    EXPECT_EQ(repeatEvents[0].Type, EInputEventType::TextInput);

    snapshot.Keys[static_cast<std::size_t>(EKey::Enter)] = false;
    snapshot.TextInput.clear();
    inputSource.SetSnapshot(snapshot);
    std::vector<FInputEvent> keyUpEvents = inputSource.Poll(frameInfo);
    ASSERT_EQ(keyUpEvents.size(), 1u);
    EXPECT_EQ(keyUpEvents[0].Type, EInputEventType::KeyUp);
    EXPECT_EQ(keyUpEvents[0].Key, EKey::Enter);
}
