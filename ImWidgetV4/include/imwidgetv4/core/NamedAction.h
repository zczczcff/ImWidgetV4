#pragma once

#include <algorithm>
#include <uectti/type_name.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ImWidgetV4 {

enum class ENamedActionHandlerType {
    Validator,
    SequentialHandler,
    FinalHandler,
    CompletionListener
};

struct FNamedActionResult {
    bool bSuccess = false;
    bool bValidationPassed = false;
    std::string ErrorMessage;
    std::size_t TotalValidators = 0;
    std::size_t PassedValidators = 0;
    std::size_t TotalHandlers = 0;
    std::size_t ExecutedHandlers = 0;
    std::size_t TotalListeners = 0;
    std::size_t ExecutedListeners = 0;

    std::string ToString() const {
        std::ostringstream stream;
        stream
            << "success=" << (bSuccess ? "true" : "false")
            << ", validationPassed=" << (bValidationPassed ? "true" : "false")
            << ", validators=" << PassedValidators << "/" << TotalValidators
            << ", handlers=" << ExecutedHandlers << "/" << TotalHandlers
            << ", listeners=" << ExecutedListeners << "/" << TotalListeners;

        if (!ErrorMessage.empty()) {
            stream << ", error=\"" << ErrorMessage << "\"";
        }

        return stream.str();
    }
};

template<typename KeyType>
struct TNamedActionHandle {
    std::uint64_t Id = 0;
    KeyType ActionKey {};
    ENamedActionHandlerType HandlerType = ENamedActionHandlerType::SequentialHandler;
    std::string SignatureId;

    bool IsValid() const {
        return Id != 0;
    }

    bool operator==(const TNamedActionHandle& other) const {
        return Id == other.Id;
    }

    bool operator!=(const TNamedActionHandle& other) const {
        return !(*this == other);
    }
};

using FNamedActionHandle = TNamedActionHandle<std::string>;

template<typename KeyType, bool bAllowOverload = false>
class TNamedActionSystem;

template<typename KeyType, bool bAllowOverload, typename... Args>
class TNamedActionInvoker {
public:
    using FSystemType = TNamedActionSystem<KeyType, bAllowOverload>;

    TNamedActionInvoker() = default;

    TNamedActionInvoker(const FSystemType* system, KeyType actionKey)
        : System_(system)
        , ActionKey_(std::move(actionKey)) {
    }

    bool IsValid() const {
        return System_ != nullptr;
    }

    const KeyType& GetActionKey() const {
        return ActionKey_;
    }

    FNamedActionResult Execute(Args... args) const {
        if (System_ == nullptr) {
            FNamedActionResult result;
            result.ErrorMessage = "Named action invoker is not bound to a system.";
            return result;
        }

        return System_->template ExecuteInvoker<Args...>(*this, std::forward<Args>(args)...);
    }

private:
    template<typename, bool>
    friend class TNamedActionSystem;

    const FSystemType* System_ = nullptr;
    KeyType ActionKey_ {};
    mutable std::uint64_t CachedMutationSerial_ = 0;
    mutable const void* CachedVariant_ = nullptr;
};

template<typename... Args>
using FNamedActionInvoker = TNamedActionInvoker<std::string, false, Args...>;

namespace Private {

template<typename Callable>
struct TFunctionTraits : public TFunctionTraits<decltype(&std::remove_reference_t<Callable>::operator())> {
};

template<typename ResultType, typename... Args>
struct TFunctionTraits<ResultType(Args...)> {
    using ReturnType = ResultType;
    using ArgumentTuple = std::tuple<std::decay_t<Args>...>;
    using StdFunctionType = std::function<ResultType(std::decay_t<Args>...)>;
};

template<typename ResultType, typename... Args>
struct TFunctionTraits<ResultType(*)(Args...)> : public TFunctionTraits<ResultType(Args...)> {
};

template<typename ResultType, typename... Args>
struct TFunctionTraits<std::function<ResultType(Args...)>> : public TFunctionTraits<ResultType(Args...)> {
};

template<typename ClassType, typename ResultType, typename... Args>
struct TFunctionTraits<ResultType(ClassType::*)(Args...)> : public TFunctionTraits<ResultType(Args...)> {
};

template<typename ClassType, typename ResultType, typename... Args>
struct TFunctionTraits<ResultType(ClassType::*)(Args...) const> : public TFunctionTraits<ResultType(Args...)> {
};

template<typename Callable>
typename TFunctionTraits<std::decay_t<Callable>>::StdFunctionType MakeStdFunction(Callable&& callable) {
    using FTraits = TFunctionTraits<std::decay_t<Callable>>;
    return typename FTraits::StdFunctionType(std::forward<Callable>(callable));
}

template<typename... Args>
std::string MakeSignatureId() {
    std::ostringstream stream;
    stream << sizeof...(Args);
    (([&stream]() {
        const auto typeName = uectti::type_name<std::decay_t<Args>>();
        stream << "|" << std::string(typeName.begin(), typeName.end());
    }()), ...);
    return stream.str();
}

template<typename KeyType>
struct THandlerBaseEntry {
    TNamedActionHandle<KeyType> Handle;
    std::string Description;
    int Priority = 0;
};

template<typename KeyType, typename CallbackType>
struct TTypedHandlerEntry : public THandlerBaseEntry<KeyType> {
    CallbackType Invoke;
};

template<typename KeyType>
class IActionVariant {
public:
    virtual ~IActionVariant() = default;

    virtual const std::string& GetSignatureId() const = 0;
    virtual std::size_t GetArgumentCount() const = 0;
    virtual bool RemoveHandler(const TNamedActionHandle<KeyType>& handle) = 0;
    virtual bool IsEmpty() const = 0;
    virtual std::size_t GetHandlerCount(ENamedActionHandlerType handlerType) const = 0;
    virtual std::size_t GetTotalHandlerCount() const = 0;
    virtual std::string GetStatisticsString(const KeyType& actionKey) const = 0;
};

template<typename KeyType, typename... Args>
class TActionVariant : public IActionVariant<KeyType> {
public:
    using FValidatorFunction = std::function<bool(Args...)>;
    using FHandlerFunction = std::function<void(Args...)>;
    using FValidatorEntry = TTypedHandlerEntry<KeyType, FValidatorFunction>;
    using FHandlerEntry = TTypedHandlerEntry<KeyType, FHandlerFunction>;

    const std::string& GetSignatureId() const override {
        return SignatureId_;
    }

    std::size_t GetArgumentCount() const override {
        return sizeof...(Args);
    }

    void AddValidator(const TNamedActionHandle<KeyType>& handle, FValidatorFunction validator, const std::string& description, int priority) {
        if (!validator) {
            return;
        }

        Validators_.push_back({
            { handle, description, priority },
            [validator = std::move(validator)](Args... args) {
                return validator(args...);
            }
        });
        SortByPriority(Validators_);
    }

    void AddSequentialHandler(const TNamedActionHandle<KeyType>& handle, FHandlerFunction handler, const std::string& description, int priority) {
        if (!handler) {
            return;
        }

        SequentialHandlers_.push_back({
            { handle, description, priority },
            [handler = std::move(handler)](Args... args) {
                handler(args...);
            }
        });
        SortByPriority(SequentialHandlers_);
    }

    void SetFinalHandler(const TNamedActionHandle<KeyType>& handle, FHandlerFunction handler, const std::string& description, int priority) {
        if (!handler) {
            FinalHandler_.reset();
            return;
        }

        FinalHandler_ = FHandlerEntry {
            { handle, description, priority },
            [handler = std::move(handler)](Args... args) {
                handler(args...);
            }
        };
    }

    void AddCompletionListener(const TNamedActionHandle<KeyType>& handle, FHandlerFunction listener, const std::string& description, int priority) {
        if (!listener) {
            return;
        }

        CompletionListeners_.push_back({
            { handle, description, priority },
            [listener = std::move(listener)](Args... args) {
                listener(args...);
            }
        });
        SortByPriority(CompletionListeners_);
    }

    FNamedActionResult Execute(Args... args) const {
        FNamedActionResult result;
        result.TotalValidators = Validators_.size();
        result.TotalHandlers = SequentialHandlers_.size() + (FinalHandler_.has_value() ? 1u : 0u);
        result.TotalListeners = CompletionListeners_.size();

        for (const FValidatorEntry& validatorEntry : Validators_) {
            try {
                if (!validatorEntry.Invoke(args...)) {
                    result.bValidationPassed = false;
                    result.ErrorMessage = validatorEntry.Description.empty()
                        ? "Named action validation failed."
                        : "Named action validation failed: " + validatorEntry.Description;
                    return result;
                }

                ++result.PassedValidators;
            } catch (const std::exception& exception) {
                result.bValidationPassed = false;
                result.ErrorMessage = "Validator error: " + std::string(exception.what());
                return result;
            } catch (...) {
                result.bValidationPassed = false;
                result.ErrorMessage = "Validator error: unknown exception.";
                return result;
            }
        }

        result.bValidationPassed = true;

        for (const FHandlerEntry& handlerEntry : SequentialHandlers_) {
            try {
                handlerEntry.Invoke(args...);
                ++result.ExecutedHandlers;
            } catch (const std::exception& exception) {
                result.ErrorMessage = "Sequential handler error: " + std::string(exception.what());
                return result;
            } catch (...) {
                result.ErrorMessage = "Sequential handler error: unknown exception.";
                return result;
            }
        }

        if (FinalHandler_.has_value()) {
            try {
                FinalHandler_->Invoke(args...);
                ++result.ExecutedHandlers;
            } catch (const std::exception& exception) {
                result.ErrorMessage = "Final handler error: " + std::string(exception.what());
                return result;
            } catch (...) {
                result.ErrorMessage = "Final handler error: unknown exception.";
                return result;
            }
        }

        for (const FHandlerEntry& listenerEntry : CompletionListeners_) {
            try {
                listenerEntry.Invoke(args...);
                ++result.ExecutedListeners;
            } catch (const std::exception& exception) {
                result.ErrorMessage = "Completion listener error: " + std::string(exception.what());
            } catch (...) {
                result.ErrorMessage = "Completion listener error: unknown exception.";
            }
        }

        result.bSuccess = true;
        return result;
    }

    bool RemoveHandler(const TNamedActionHandle<KeyType>& handle) override {
        switch (handle.HandlerType) {
        case ENamedActionHandlerType::Validator:
            return RemoveFromVector(Validators_, handle);
        case ENamedActionHandlerType::SequentialHandler:
            return RemoveFromVector(SequentialHandlers_, handle);
        case ENamedActionHandlerType::FinalHandler:
            if (FinalHandler_.has_value() && FinalHandler_->Handle == handle) {
                FinalHandler_.reset();
                return true;
            }
            return false;
        case ENamedActionHandlerType::CompletionListener:
            return RemoveFromVector(CompletionListeners_, handle);
        default:
            return false;
        }
    }

    bool IsEmpty() const override {
        return Validators_.empty()
            && SequentialHandlers_.empty()
            && CompletionListeners_.empty()
            && !FinalHandler_.has_value();
    }

    std::size_t GetHandlerCount(ENamedActionHandlerType handlerType) const override {
        switch (handlerType) {
        case ENamedActionHandlerType::Validator:
            return Validators_.size();
        case ENamedActionHandlerType::SequentialHandler:
            return SequentialHandlers_.size();
        case ENamedActionHandlerType::FinalHandler:
            return FinalHandler_.has_value() ? 1u : 0u;
        case ENamedActionHandlerType::CompletionListener:
            return CompletionListeners_.size();
        default:
            return 0;
        }
    }

    std::size_t GetTotalHandlerCount() const override {
        return Validators_.size()
            + SequentialHandlers_.size()
            + CompletionListeners_.size()
            + (FinalHandler_.has_value() ? 1u : 0u);
    }

    std::string GetStatisticsString(const KeyType& actionKey) const override {
        std::ostringstream stream;
        stream
            << "Action=" << actionKey
            << ", signature=" << SignatureId_
            << ", validators=" << Validators_.size()
            << ", sequential=" << SequentialHandlers_.size()
            << ", final=" << (FinalHandler_.has_value() ? 1 : 0)
            << ", completion=" << CompletionListeners_.size();
        return stream.str();
    }

private:
    template<typename EntryType>
    static void SortByPriority(std::vector<EntryType>& entries) {
        std::stable_sort(
            entries.begin(),
            entries.end(),
            [](const EntryType& left, const EntryType& right) {
                return left.Priority < right.Priority;
            });
    }

    template<typename EntryType>
    static bool RemoveFromVector(std::vector<EntryType>& entries, const TNamedActionHandle<KeyType>& handle) {
        const auto it = std::find_if(
            entries.begin(),
            entries.end(),
            [&handle](const EntryType& entry) {
                return entry.Handle == handle;
            });

        if (it == entries.end()) {
            return false;
        }

        entries.erase(it);
        return true;
    }

    const std::string SignatureId_ = MakeSignatureId<Args...>();
    std::vector<FValidatorEntry> Validators_;
    std::vector<FHandlerEntry> SequentialHandlers_;
    std::optional<FHandlerEntry> FinalHandler_;
    std::vector<FHandlerEntry> CompletionListeners_;
};

} // namespace Private

template<typename KeyType, bool bAllowOverload>
class TNamedActionSystem {
public:
    using FHandle = TNamedActionHandle<KeyType>;
    using FGlobalCompletionListener = std::function<void(const KeyType&, const FNamedActionResult&)>;

    template<typename Callable>
    FHandle AddValidator(const KeyType& actionKey, Callable&& validator, const std::string& description = "", int priority = 0) {
        using FTraits = Private::TFunctionTraits<std::decay_t<Callable>>;
        static_assert(
            std::is_convertible_v<typename FTraits::ReturnType, bool>,
            "Named action validators must return bool or a bool-convertible type.");

        return AddValidatorImpl(
            actionKey,
            Private::MakeStdFunction(std::forward<Callable>(validator)),
            description,
            priority);
    }

    template<typename Callable>
    FHandle AddSequentialHandler(const KeyType& actionKey, Callable&& handler, const std::string& description = "", int priority = 0) {
        using FTraits = Private::TFunctionTraits<std::decay_t<Callable>>;
        static_assert(
            std::is_void_v<typename FTraits::ReturnType>,
            "Named action sequential handlers must return void.");

        return AddSequentialHandlerImpl(
            actionKey,
            Private::MakeStdFunction(std::forward<Callable>(handler)),
            description,
            priority);
    }

    template<typename Callable>
    FHandle SetFinalHandler(const KeyType& actionKey, Callable&& handler, const std::string& description = "", int priority = 0) {
        using FTraits = Private::TFunctionTraits<std::decay_t<Callable>>;
        static_assert(
            std::is_void_v<typename FTraits::ReturnType>,
            "Named action final handlers must return void.");

        return SetFinalHandlerImpl(
            actionKey,
            Private::MakeStdFunction(std::forward<Callable>(handler)),
            description,
            priority);
    }

    template<typename Callable>
    FHandle AddCompletionListener(const KeyType& actionKey, Callable&& listener, const std::string& description = "", int priority = 0) {
        using FTraits = Private::TFunctionTraits<std::decay_t<Callable>>;
        static_assert(
            std::is_void_v<typename FTraits::ReturnType>,
            "Named action completion listeners must return void.");

        return AddCompletionListenerImpl(
            actionKey,
            Private::MakeStdFunction(std::forward<Callable>(listener)),
            description,
            priority);
    }

    std::uint64_t AddGlobalCompletionListener(FGlobalCompletionListener listener, int priority = 0, const std::string& description = "") {
        if (!listener) {
            return 0;
        }

        const std::uint64_t handleId = NextGlobalHandleId_++;
        GlobalCompletionListeners_.push_back({ handleId, std::move(listener), description, priority });
        std::stable_sort(
            GlobalCompletionListeners_.begin(),
            GlobalCompletionListeners_.end(),
            [](const FGlobalListenerEntry& left, const FGlobalListenerEntry& right) {
                return left.Priority < right.Priority;
            });
        ++MutationSerial_;
        return handleId;
    }

    bool RemoveGlobalCompletionListener(std::uint64_t handleId) {
        const auto it = std::find_if(
            GlobalCompletionListeners_.begin(),
            GlobalCompletionListeners_.end(),
            [handleId](const FGlobalListenerEntry& entry) {
                return entry.HandleId == handleId;
            });

        if (it == GlobalCompletionListeners_.end()) {
            return false;
        }

        GlobalCompletionListeners_.erase(it);
        ++MutationSerial_;
        return true;
    }

    template<typename... Args>
    FNamedActionResult Execute(const KeyType& actionKey, Args&&... args) const {
        using FVariantType = Private::TActionVariant<KeyType, std::decay_t<Args>...>;

        FNamedActionResult result;
        const FVariantType* variant = ResolveVariant<std::decay_t<Args>...>(actionKey);
        if (variant == nullptr) {
            result.ErrorMessage = "No named action registered for the requested key/signature.";
            NotifyGlobalCompletionListeners(actionKey, result);
            return result;
        }

        result = variant->Execute(std::forward<Args>(args)...);
        NotifyGlobalCompletionListeners(actionKey, result);
        return result;
    }

    template<typename... Args>
    TNamedActionInvoker<KeyType, bAllowOverload, std::decay_t<Args>...> AcquireInvoker(const KeyType& actionKey) const {
        return TNamedActionInvoker<KeyType, bAllowOverload, std::decay_t<Args>...>(this, actionKey);
    }

    bool HasAction(const KeyType& actionKey) const {
        const auto it = Actions_.find(actionKey);
        return it != Actions_.end() && !it->second.empty();
    }

    template<typename... Args>
    bool HasActionWithArgs(const KeyType& actionKey) const {
        return ResolveVariant<std::decay_t<Args>...>(actionKey) != nullptr;
    }

    std::size_t GetActionVariantCount(const KeyType& actionKey) const {
        const auto it = Actions_.find(actionKey);
        return it == Actions_.end() ? 0u : it->second.size();
    }

    std::size_t GetHandlerCount(const KeyType& actionKey, ENamedActionHandlerType handlerType) const {
        const auto it = Actions_.find(actionKey);
        if (it == Actions_.end()) {
            return 0u;
        }

        std::size_t count = 0;
        for (const std::unique_ptr<Private::IActionVariant<KeyType>>& variant : it->second) {
            count += variant->GetHandlerCount(handlerType);
        }

        return count;
    }

    bool RemoveHandler(const FHandle& handle) {
        if (!handle.IsValid()) {
            return false;
        }

        auto it = Actions_.find(handle.ActionKey);
        if (it == Actions_.end()) {
            return false;
        }

        for (auto variantIt = it->second.begin(); variantIt != it->second.end(); ++variantIt) {
            if ((*variantIt)->GetSignatureId() != handle.SignatureId) {
                continue;
            }

            if (!(*variantIt)->RemoveHandler(handle)) {
                return false;
            }

            if ((*variantIt)->IsEmpty()) {
                it->second.erase(variantIt);
            }

            if (it->second.empty()) {
                Actions_.erase(it);
            }

            ++MutationSerial_;
            return true;
        }

        return false;
    }

    void Clear() {
        Actions_.clear();
        GlobalCompletionListeners_.clear();
        ++MutationSerial_;
    }

    std::size_t GetGlobalCompletionListenerCount() const {
        return GlobalCompletionListeners_.size();
    }

    std::string GetStatisticsString() const {
        std::ostringstream stream;
        stream
            << "NamedActionSystem actions=" << Actions_.size()
            << ", globalCompletionListeners=" << GlobalCompletionListeners_.size();

        for (const auto& pair : Actions_) {
            for (const std::unique_ptr<Private::IActionVariant<KeyType>>& variant : pair.second) {
                stream << "\n  " << variant->GetStatisticsString(pair.first);
            }
        }

        return stream.str();
    }

private:
    struct FGlobalListenerEntry {
        std::uint64_t HandleId = 0;
        FGlobalCompletionListener Listener;
        std::string Description;
        int Priority = 0;
    };

    template<typename, bool, typename...>
    friend class TNamedActionInvoker;

    template<typename... Args>
    FNamedActionResult ExecuteInvoker(
        const TNamedActionInvoker<KeyType, bAllowOverload, Args...>& invoker,
        Args... args) const {
        using FVariantType = Private::TActionVariant<KeyType, std::decay_t<Args>...>;

        const FVariantType* variant = nullptr;
        if (invoker.CachedVariant_ != nullptr && invoker.CachedMutationSerial_ == MutationSerial_) {
            variant = static_cast<const FVariantType*>(invoker.CachedVariant_);
        } else {
            variant = ResolveVariant<std::decay_t<Args>...>(invoker.ActionKey_);
            invoker.CachedVariant_ = variant;
            invoker.CachedMutationSerial_ = MutationSerial_;
        }

        FNamedActionResult result;
        if (variant == nullptr) {
            result.ErrorMessage = "No named action registered for the requested key/signature.";
            NotifyGlobalCompletionListeners(invoker.ActionKey_, result);
            return result;
        }

        result = variant->Execute(std::forward<Args>(args)...);
        NotifyGlobalCompletionListeners(invoker.ActionKey_, result);
        return result;
    }

    void NotifyGlobalCompletionListeners(const KeyType& actionKey, const FNamedActionResult& result) const {
        for (const FGlobalListenerEntry& entry : GlobalCompletionListeners_) {
            try {
                entry.Listener(actionKey, result);
            } catch (...) {
            }
        }
    }

    template<typename... Args>
    Private::TActionVariant<KeyType, Args...>* GetOrCreateVariant(const KeyType& actionKey) {
        std::vector<std::unique_ptr<Private::IActionVariant<KeyType>>>& variants = Actions_[actionKey];
        const std::string signatureId = Private::MakeSignatureId<Args...>();

        for (std::unique_ptr<Private::IActionVariant<KeyType>>& variant : variants) {
            if (variant->GetSignatureId() == signatureId) {
                return static_cast<Private::TActionVariant<KeyType, Args...>*>(variant.get());
            }

            if constexpr (!bAllowOverload) {
                return nullptr;
            }
        }

        auto createdVariant = std::make_unique<Private::TActionVariant<KeyType, Args...>>();
        Private::TActionVariant<KeyType, Args...>* createdVariantPtr = createdVariant.get();
        variants.push_back(std::move(createdVariant));
        return createdVariantPtr;
    }

    template<typename... Args>
    const Private::TActionVariant<KeyType, Args...>* ResolveVariant(const KeyType& actionKey) const {
        const auto it = Actions_.find(actionKey);
        if (it == Actions_.end()) {
            return nullptr;
        }

        const std::string signatureId = Private::MakeSignatureId<Args...>();
        for (const std::unique_ptr<Private::IActionVariant<KeyType>>& variant : it->second) {
            if (variant->GetSignatureId() == signatureId) {
                return static_cast<const Private::TActionVariant<KeyType, Args...>*>(variant.get());
            }
        }

        return nullptr;
    }

    template<typename... Args>
    FHandle AddValidatorImpl(const KeyType& actionKey, std::function<bool(Args...)> validator, const std::string& description, int priority) {
        if (!validator) {
            return {};
        }

        auto* variant = GetOrCreateVariant<Args...>(actionKey);
        if (variant == nullptr) {
            return {};
        }

        FHandle handle = MakeHandle(actionKey, ENamedActionHandlerType::Validator, Private::MakeSignatureId<Args...>());
        variant->AddValidator(handle, std::move(validator), description, priority);
        ++MutationSerial_;
        return handle;
    }

    template<typename... Args>
    FHandle AddSequentialHandlerImpl(const KeyType& actionKey, std::function<void(Args...)> handler, const std::string& description, int priority) {
        if (!handler) {
            return {};
        }

        auto* variant = GetOrCreateVariant<Args...>(actionKey);
        if (variant == nullptr) {
            return {};
        }

        FHandle handle = MakeHandle(actionKey, ENamedActionHandlerType::SequentialHandler, Private::MakeSignatureId<Args...>());
        variant->AddSequentialHandler(handle, std::move(handler), description, priority);
        ++MutationSerial_;
        return handle;
    }

    template<typename... Args>
    FHandle SetFinalHandlerImpl(const KeyType& actionKey, std::function<void(Args...)> handler, const std::string& description, int priority) {
        if (!handler) {
            return {};
        }

        auto* variant = GetOrCreateVariant<Args...>(actionKey);
        if (variant == nullptr) {
            return {};
        }

        FHandle handle = MakeHandle(actionKey, ENamedActionHandlerType::FinalHandler, Private::MakeSignatureId<Args...>());
        variant->SetFinalHandler(handle, std::move(handler), description, priority);
        ++MutationSerial_;
        return handle;
    }

    template<typename... Args>
    FHandle AddCompletionListenerImpl(const KeyType& actionKey, std::function<void(Args...)> listener, const std::string& description, int priority) {
        if (!listener) {
            return {};
        }

        auto* variant = GetOrCreateVariant<Args...>(actionKey);
        if (variant == nullptr) {
            return {};
        }

        FHandle handle = MakeHandle(actionKey, ENamedActionHandlerType::CompletionListener, Private::MakeSignatureId<Args...>());
        variant->AddCompletionListener(handle, std::move(listener), description, priority);
        ++MutationSerial_;
        return handle;
    }

    FHandle MakeHandle(const KeyType& actionKey, ENamedActionHandlerType handlerType, const std::string& signatureId) {
        return FHandle { NextHandleId_++, actionKey, handlerType, signatureId };
    }

    std::unordered_map<KeyType, std::vector<std::unique_ptr<Private::IActionVariant<KeyType>>>> Actions_;
    std::vector<FGlobalListenerEntry> GlobalCompletionListeners_;
    std::uint64_t NextHandleId_ = 1;
    std::uint64_t NextGlobalHandleId_ = 1;
    mutable std::uint64_t MutationSerial_ = 1;
};

using FNamedActionSystem = TNamedActionSystem<std::string, false>;
using FNamedActionSystemOverload = TNamedActionSystem<std::string, true>;

} // namespace ImWidgetV4
