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

TEST(NamedActionTest, BorrowStagesObserveValueBeforeFinalRvalueConsumption) {
    FNamedActionSystem system;
    std::vector<std::string> observed;
    std::string consumedValue;

    system.AddValidator("Consume", [&](const std::string& value) {
        observed.push_back("validator:" + value);
        return true;
    });
    system.AddSequentialHandler("Consume", [&](const std::string& value) {
        observed.push_back("handler:" + value);
    });
    system.SetFinalHandler("Consume", [&](std::string&& value) {
        consumedValue = std::move(value);
    });

    const FNamedActionResult result = system.Execute("Consume", std::string("PaletteButton"));

    EXPECT_TRUE(result.bSuccess);
    EXPECT_TRUE(result.bValidationPassed);
    EXPECT_EQ(consumedValue, "PaletteButton");
    ASSERT_EQ(observed.size(), 2u);
    EXPECT_EQ(observed[0], "validator:PaletteButton");
    EXPECT_EQ(observed[1], "handler:PaletteButton");
}

TEST(NamedActionTest, FinalRvalueHandlerRejectsLvalueExecution) {
    FNamedActionSystem system;
    std::string stored;
    system.SetFinalHandler("Consume", [&](std::string&& value) {
        stored = std::move(value);
    });

    std::string source = "InspectorButton";
    const FNamedActionResult result = system.Execute("Consume", source);

    EXPECT_FALSE(result.bSuccess);
    EXPECT_TRUE(result.bValidationPassed);
    EXPECT_NE(result.ErrorMessage.find("rvalue"), std::string::npos);
    EXPECT_TRUE(stored.empty());
}

TEST(NamedActionTest, MixedConstNonConstLvalueAndRvalueSemanticsRemainStable) {
    FNamedActionSystem system;
    std::vector<std::string> observed;
    std::string consumedRvalue;

    system.AddValidator("Mutate", [&](const std::string& value) {
        observed.push_back("mutate-validator:" + value);
        return true;
    });
    system.AddSequentialHandler("Mutate", [&](std::string value) {
        observed.push_back("mutate-sequential:" + value);
    });
    system.SetFinalHandler("Mutate", [&](std::string& value) {
        value += "_edited";
        observed.push_back("mutate-final:" + value);
    });
    system.AddCompletionListener("Mutate", [&](const std::string& value) {
        observed.push_back("mutate-completion:" + value);
    });

    std::string mutableSource = "Button";
    const FNamedActionResult mutateResult = system.Execute("Mutate", mutableSource);

    EXPECT_TRUE(mutateResult.bSuccess);
    EXPECT_EQ(mutableSource, "Button_edited");

    system.AddValidator("ConsumeMixed", [&](const std::string& value) {
        observed.push_back("consume-validator:" + value);
        return true;
    });
    system.AddSequentialHandler("ConsumeMixed", [&](std::string value) {
        observed.push_back("consume-sequential:" + value);
    });
    system.SetFinalHandler("ConsumeMixed", [&](std::string&& value) {
        consumedRvalue = std::move(value);
        observed.push_back("consume-final:" + consumedRvalue);
    });
    system.AddCompletionListener("ConsumeMixed", [&](const std::string& value) {
        observed.push_back("consume-completion:" + value);
    });

    const FNamedActionResult consumeResult = system.Execute("ConsumeMixed", std::string("Panel"));

    EXPECT_TRUE(consumeResult.bSuccess);
    EXPECT_EQ(consumedRvalue, "Panel");

    const std::vector<std::string> expected = {
        "mutate-validator:Button",
        "mutate-sequential:Button",
        "mutate-final:Button_edited",
        "mutate-completion:Button_edited",
        "consume-validator:Panel",
        "consume-sequential:Panel",
        "consume-final:Panel",
        "consume-completion:"
    };

    EXPECT_EQ(observed, expected);
}

TEST(NamedActionTest, MixedParameterListSupportsConstRefMutableRefAndRvalueRefTogether) {
    FNamedActionSystem system;
    std::vector<std::string> observed;
    std::string consumedPayload;

    system.AddValidator("Compose", [&](const std::string& title, const std::string& name, const std::string& payload) {
        observed.push_back("validator:" + title + "|" + name + "|" + payload);
        return true;
    });
    system.AddSequentialHandler("Compose", [&](std::string title, const std::string& name, std::string payload) {
        observed.push_back("sequential:" + title + "|" + name + "|" + payload);
    });
    system.SetFinalHandler("Compose", [&](const std::string& title, std::string& name, std::string&& payload) {
        observed.push_back("final-before:" + title + "|" + name + "|" + payload);
        name += "_bound";
        consumedPayload = std::move(payload);
        observed.push_back("final-after:" + title + "|" + name + "|" + consumedPayload);
    });
    system.AddCompletionListener("Compose", [&](const std::string& title, const std::string& name, const std::string& payload) {
        observed.push_back("completion:" + title + "|" + name + "|" + payload);
    });

    const std::string title = "Button";
    std::string name = "Primary";

    const FNamedActionResult result = system.Execute("Compose", title, name, std::string("IconBrush"));

    EXPECT_TRUE(result.bSuccess);
    EXPECT_TRUE(result.bValidationPassed);
    EXPECT_EQ(name, "Primary_bound");
    EXPECT_EQ(consumedPayload, "IconBrush");

    const std::vector<std::string> expected = {
        "validator:Button|Primary|IconBrush",
        "sequential:Button|Primary|IconBrush",
        "final-before:Button|Primary|IconBrush",
        "final-after:Button|Primary_bound|IconBrush",
        "completion:Button|Primary_bound|"
    };
    EXPECT_EQ(observed, expected);
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

TEST(NamedActionTest, AcquireInvokerPreservesLvalueAndRvalueSemantics) {
    FNamedActionSystem system;
    std::string consumed;
    system.SetFinalHandler("Consume", [&](std::string&& value) {
        consumed = std::move(value);
    });

    const auto invoker = system.AcquireInvoker<std::string>("Consume");

    std::string lvalue = "FromLvalue";
    const FNamedActionResult lvalueResult = invoker.Execute(lvalue);
    EXPECT_FALSE(lvalueResult.bSuccess);
    EXPECT_TRUE(consumed.empty());

    const FNamedActionResult rvalueResult = invoker.Execute(std::string("FromRvalue"));
    EXPECT_TRUE(rvalueResult.bSuccess);
    EXPECT_EQ(consumed, "FromRvalue");
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
