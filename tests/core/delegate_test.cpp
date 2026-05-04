#include <gtest/gtest.h>
#include <imwidgetv4/core/Delegate.h>
#include <vector>

using namespace ImWidgetV4;

TEST(DelegateTest, AddLambdaAndBroadcastCallsAllListeners) {
    TMulticastDelegate<int> delegate;
    std::vector<int> values;

    delegate.AddLambda([&](int value) { values.push_back(value); });
    delegate.AddLambda([&](int value) { values.push_back(value * 2); });

    delegate.Broadcast(3);

    ASSERT_EQ(values.size(), 2u);
    EXPECT_EQ(values[0], 3);
    EXPECT_EQ(values[1], 6);
}

TEST(DelegateTest, ListenersRunInRegistrationOrder) {
    TMulticastDelegate<> delegate;
    std::vector<int> order;

    delegate.AddLambda([&]() { order.push_back(1); });
    delegate.AddLambda([&]() { order.push_back(2); });
    delegate.AddLambda([&]() { order.push_back(3); });

    delegate.Broadcast();

    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

TEST(DelegateTest, RemoveOnlyRemovesMatchingListener) {
    TMulticastDelegate<> delegate;
    int callCountA = 0;
    int callCountB = 0;

    const FDelegateHandle handleA = delegate.AddLambda([&]() { ++callCountA; });
    delegate.AddLambda([&]() { ++callCountB; });

    EXPECT_TRUE(delegate.Remove(handleA));
    delegate.Broadcast();

    EXPECT_EQ(callCountA, 0);
    EXPECT_EQ(callCountB, 1);
}

TEST(DelegateTest, ClearRemovesAllListenersAndUpdatesBoundState) {
    TMulticastDelegate<> delegate;
    delegate.AddLambda([]() {});
    delegate.AddLambda([]() {});

    EXPECT_TRUE(delegate.IsBound());
    delegate.Clear();
    EXPECT_FALSE(delegate.IsBound());
    EXPECT_NO_THROW(delegate.Broadcast());
}

TEST(DelegateTest, BroadcastOnEmptyDelegateIsSafe) {
    TMulticastDelegate<int, int> delegate;
    EXPECT_FALSE(delegate.IsBound());
    EXPECT_NO_THROW(delegate.Broadcast(1, 2));
}

TEST(DelegateTest, RemovingDuringBroadcastDoesNotCorruptCurrentBroadcast) {
    TMulticastDelegate<> delegate;
    std::vector<int> order;
    FDelegateHandle firstHandle;
    FDelegateHandle secondHandle;

    firstHandle = delegate.AddLambda([&]() {
        order.push_back(1);
        delegate.Remove(firstHandle);
        delegate.Remove(secondHandle);
    });
    secondHandle = delegate.AddLambda([&]() {
        order.push_back(2);
    });

    delegate.Broadcast();

    ASSERT_EQ(order.size(), 2u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);

    order.clear();
    delegate.Broadcast();
    EXPECT_TRUE(order.empty());
}

TEST(DelegateTest, CanAddDifferentListenersAndRemoveByHandle) {
    TMulticastDelegate<int> delegate;
    int sum = 0;

    const FDelegateHandle first = delegate.AddLambda([&](int value) { sum += value; });
    const FDelegateHandle second = delegate.AddLambda([&](int value) { sum += value * 10; });

    EXPECT_TRUE(first.IsValid());
    EXPECT_TRUE(second.IsValid());
    EXPECT_NE(first, second);

    delegate.Broadcast(2);
    EXPECT_EQ(sum, 22);

    EXPECT_TRUE(delegate.Remove(second));
    delegate.Broadcast(3);
    EXPECT_EQ(sum, 25);
}
