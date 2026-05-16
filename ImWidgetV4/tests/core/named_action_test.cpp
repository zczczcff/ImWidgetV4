#include <gtest/gtest.h>
#include <imwidgetv4/core/NamedAction.h>
#include <string>
#include <vector>

using namespace ImWidgetV4;

TEST(NamedActionTest, SequentialHandlersAndFinalHandlerExecuteInPriorityOrder) {
    FNamedActionSystem system;
    std::vector<std::string> order;

    system.AddSequentialHandler("OpenProject", [&]() { order.push_back("second"); }, "second", 10);
    system.AddSequentialHandler("OpenProject", [&]() { order.push_back("first"); }, "first", 0);
    system.SetFinalHandler("OpenProject", [&]() { order.push_back("final"); }, "final");

    const FNamedActionResult result = system.Execute("OpenProject");

    EXPECT_TRUE(result.bSuccess);
    EXPECT_TRUE(result.bValidationPassed);
    EXPECT_EQ(result.TotalHandlers, 3u);
    EXPECT_EQ(result.ExecutedHandlers, 3u);
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], "first");
    EXPECT_EQ(order[1], "second");
    EXPECT_EQ(order[2], "final");
}

TEST(NamedActionTest, MissingActionReturnsFailure) {
    FNamedActionSystem system;

    const FNamedActionResult result = system.Execute("MissingAction", 42);

    EXPECT_FALSE(result.bSuccess);
    EXPECT_FALSE(result.bValidationPassed);
    EXPECT_FALSE(result.ErrorMessage.empty());
}

TEST(NamedActionTest, ValidatorBlocksExecution) {
    FNamedActionSystem system;
    int callCount = 0;

    system.AddValidator("SaveDocument", [](int revision) { return revision > 0; }, "revision check");
    system.AddSequentialHandler("SaveDocument", [&](int) { ++callCount; });

    const FNamedActionResult result = system.Execute("SaveDocument", 0);

    EXPECT_FALSE(result.bSuccess);
    EXPECT_FALSE(result.bValidationPassed);
    EXPECT_EQ(result.TotalValidators, 1u);
    EXPECT_EQ(result.PassedValidators, 0u);
    EXPECT_EQ(callCount, 0);
}

TEST(NamedActionTest, CompletionListenerRunsAfterHandlers) {
    FNamedActionSystem system;
    std::vector<std::string> order;

    system.AddSequentialHandler("Build", [&]() { order.push_back("handler"); });
    system.AddCompletionListener("Build", [&]() { order.push_back("completion"); });

    const FNamedActionResult result = system.Execute("Build");

    EXPECT_TRUE(result.bSuccess);
    EXPECT_EQ(result.TotalListeners, 1u);
    EXPECT_EQ(result.ExecutedListeners, 1u);
    ASSERT_EQ(order.size(), 2u);
    EXPECT_EQ(order[0], "handler");
    EXPECT_EQ(order[1], "completion");
}

TEST(NamedActionTest, StringArgumentsRemainStableAcrossMultipleStages) {
    FNamedActionSystem system;
    std::vector<std::string> observed;

    system.AddValidator("Rename", [&](std::string value) {
        observed.push_back("validator:" + value);
        return true;
    });
    system.AddSequentialHandler("Rename", [&](std::string value) {
        observed.push_back("handler:" + value);
    });
    system.AddCompletionListener("Rename", [&](std::string value) {
        observed.push_back("completion:" + value);
    });

    const FNamedActionResult result = system.Execute("Rename", std::string("Button1"));

    EXPECT_TRUE(result.bSuccess);
    ASSERT_EQ(observed.size(), 3u);
    EXPECT_EQ(observed[0], "validator:Button1");
    EXPECT_EQ(observed[1], "handler:Button1");
    EXPECT_EQ(observed[2], "completion:Button1");
}

TEST(NamedActionTest, GlobalCompletionListenerRunsOnFailureAndSuccess) {
    FNamedActionSystem system;
    std::vector<std::string> events;

    system.AddGlobalCompletionListener(
        [&](const std::string& key, const FNamedActionResult& result) {
            events.push_back(key + ":" + (result.bSuccess ? "success" : "failure"));
        });

    system.AddSequentialHandler("Run", []() {});

    system.Execute("Missing");
    system.Execute("Run");

    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0], "Missing:failure");
    EXPECT_EQ(events[1], "Run:success");
}

TEST(NamedActionTest, AcquireInvokerExecutesTypedAction) {
    FNamedActionSystem system;
    std::vector<int> values;

    system.AddSequentialHandler("SelectIndex", [&](int index) { values.push_back(index); });

    const auto invoker = system.AcquireInvoker<int>("SelectIndex");
    const FNamedActionResult result = invoker.Execute(7);

    EXPECT_TRUE(invoker.IsValid());
    EXPECT_TRUE(result.bSuccess);
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ(values[0], 7);
}

TEST(NamedActionTest, OverloadModeSupportsMultipleSignaturesPerActionKey) {
    FNamedActionSystemOverload system;
    std::vector<std::string> values;

    system.AddSequentialHandler("Open", [&]() { values.push_back("noargs"); });
    system.AddSequentialHandler("Open", [&](int index) { values.push_back(std::to_string(index)); });

    EXPECT_TRUE(system.HasAction("Open"));
    EXPECT_TRUE(system.HasActionWithArgs<>("Open"));
    EXPECT_TRUE(system.HasActionWithArgs<int>("Open"));
    EXPECT_EQ(system.GetActionVariantCount("Open"), 2u);

    EXPECT_TRUE(system.Execute("Open").bSuccess);
    EXPECT_TRUE(system.Execute("Open", 3).bSuccess);

    ASSERT_EQ(values.size(), 2u);
    EXPECT_EQ(values[0], "noargs");
    EXPECT_EQ(values[1], "3");
}

TEST(NamedActionTest, RemoveHandlerRemovesOnlyMatchingRegistration) {
    FNamedActionSystem system;
    std::vector<int> values;

    const FNamedActionHandle first = system.AddSequentialHandler("Append", [&](int value) { values.push_back(value); });
    system.AddSequentialHandler("Append", [&](int value) { values.push_back(value * 10); });

    EXPECT_TRUE(system.RemoveHandler(first));
    EXPECT_FALSE(system.RemoveHandler(first));
    EXPECT_TRUE(system.Execute("Append", 2).bSuccess);

    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ(values[0], 20);
}

TEST(NamedActionTest, NonOverloadModeRejectsDifferentSignatureForSameActionKey) {
    FNamedActionSystem system;

    const FNamedActionHandle first = system.AddSequentialHandler("Inspect", []() {});
    const FNamedActionHandle second = system.AddSequentialHandler("Inspect", [](int) {});

    EXPECT_TRUE(first.IsValid());
    EXPECT_FALSE(second.IsValid());
    EXPECT_EQ(system.GetActionVariantCount("Inspect"), 1u);
    EXPECT_TRUE(system.HasActionWithArgs<>("Inspect"));
    EXPECT_FALSE(system.HasActionWithArgs<int>("Inspect"));
}

TEST(NamedActionTest, ClearResetsAllState) {
    FNamedActionSystem system;
    system.AddSequentialHandler("A", []() {});
    system.AddGlobalCompletionListener([](const std::string&, const FNamedActionResult&) {});

    EXPECT_TRUE(system.HasAction("A"));
    EXPECT_EQ(system.GetGlobalCompletionListenerCount(), 1u);

    system.Clear();

    EXPECT_FALSE(system.HasAction("A"));
    EXPECT_EQ(system.GetActionVariantCount("A"), 0u);
    EXPECT_EQ(system.GetGlobalCompletionListenerCount(), 0u);
}

TEST(NamedActionTest, StatisticsAndCountsReflectRegisteredHandlers) {
    FNamedActionSystem system;

    system.AddValidator("Validate", []() { return true; });
    system.AddSequentialHandler("Validate", []() {});
    system.SetFinalHandler("Validate", []() {});
    system.AddCompletionListener("Validate", []() {});

    EXPECT_EQ(system.GetHandlerCount("Validate", ENamedActionHandlerType::Validator), 1u);
    EXPECT_EQ(system.GetHandlerCount("Validate", ENamedActionHandlerType::SequentialHandler), 1u);
    EXPECT_EQ(system.GetHandlerCount("Validate", ENamedActionHandlerType::FinalHandler), 1u);
    EXPECT_EQ(system.GetHandlerCount("Validate", ENamedActionHandlerType::CompletionListener), 1u);

    const std::string stats = system.GetStatisticsString();
    EXPECT_NE(stats.find("Validate"), std::string::npos);
    EXPECT_NE(stats.find("validators=1"), std::string::npos);
}
