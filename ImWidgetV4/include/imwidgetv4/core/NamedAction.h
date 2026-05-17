#pragma once

#include <algorithm>
#include <uectti/type_name.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
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

template<typename T>
using TNamedActionCanonicalArg = std::remove_cv_t<std::remove_reference_t<T>>;

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

    template<typename... CallArgs>
    FNamedActionResult Execute(CallArgs&&... args) const {
        static_assert(sizeof...(CallArgs) == sizeof...(Args),
            "Invoker argument count must match the acquired action signature.");
        static_assert((std::is_same_v<TNamedActionCanonicalArg<CallArgs>, Args> && ...),
            "Invoker argument types must match the acquired canonical action signature.");

        if (System_ == nullptr) {
            FNamedActionResult result;
            result.ErrorMessage = "Named action invoker is not bound to a system.";
            return result;
        }

        return System_->template ExecuteInvoker<Args...>(*this, std::forward<CallArgs>(args)...);
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

template<typename T>
using TCanonicalArg = TNamedActionCanonicalArg<T>;

template<typename Callable>
struct TFunctionTraits : public TFunctionTraits<decltype(&std::remove_reference_t<Callable>::operator())> {
};

template<typename ResultType, typename... Args>
struct TFunctionTraits<ResultType(Args...)> {
    using ReturnType = ResultType;
    using ArgumentTuple = std::tuple<Args...>;
    using StdFunctionType = std::function<ResultType(Args...)>;
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
std::string MakeCanonicalSignatureId() {
    std::ostringstream stream;
    stream << sizeof...(Args);
    (([&stream]() {
        const auto typeName = uectti::type_name<TCanonicalArg<Args>>();
        stream << "|" << std::string(typeName.begin(), typeName.end());
    }()), ...);
    return stream.str();
}

template<typename... Args>
std::string MakeHandlerSignatureId() {
    std::ostringstream stream;
    stream << sizeof...(Args);
    (([&stream]() {
        const auto typeName = uectti::type_name_with_cvr<Args>();
        stream << "|" << std::string(typeName.begin(), typeName.end());
    }()), ...);
    return stream.str();
}

template<typename HandlerArg>
constexpr bool IsSupportedBorrowStageArgV =
    !std::is_rvalue_reference_v<HandlerArg>
    && (!std::is_lvalue_reference_v<HandlerArg> || std::is_const_v<std::remove_reference_t<HandlerArg>>);

template<typename HandlerArg, typename SourceArg>
HandlerArg ConvertBorrowStageArg(const SourceArg& sourceArg) {
    static_assert(IsSupportedBorrowStageArgV<HandlerArg>,
        "Borrow stages only support value parameters or const lvalue references.");
    static_assert(std::is_same_v<TCanonicalArg<HandlerArg>, TCanonicalArg<SourceArg>>,
        "Borrow stage parameter types must match the canonical action payload types.");

    if constexpr (std::is_lvalue_reference_v<HandlerArg>) {
        return sourceArg;
    } else {
        static_assert(std::is_constructible_v<HandlerArg, const SourceArg&>,
            "Borrow stage value parameters must be copy-constructible from const references.");
        return sourceArg;
    }
}

template<typename T>
class TFinalArgDispatch {
public:
    template<typename SourceArg>
    explicit TFinalArgDispatch(SourceArg&& sourceArg)
        : Ptr_(const_cast<T*>(std::addressof(sourceArg)))
        , bMutable_(!std::is_const_v<std::remove_reference_t<SourceArg>>)
        , bCanMove_(!std::is_lvalue_reference_v<SourceArg&&> && !std::is_const_v<std::remove_reference_t<SourceArg>>) {
    }

    const T& AsConstRef() const {
        return *Ptr_;
    }

    T& AsMutableRef() const {
        if (!bMutable_) {
            throw std::runtime_error("Final handler requires a mutable argument, but the action was executed with a const source.");
        }

        return *Ptr_;
    }

    T&& AsRvalueRef() const {
        if (!bMutable_) {
            throw std::runtime_error("Final handler requires an rvalue argument, but the action was executed with a const source.");
        }

        if (!bCanMove_) {
            throw std::runtime_error("Final handler requires an rvalue argument, but the action was executed with an lvalue source.");
        }

        return std::move(*Ptr_);
    }

    T MakeValue() const {
        if (bMutable_ && bCanMove_) {
            return std::move(*Ptr_);
        }

        return *Ptr_;
    }

private:
    T* Ptr_ = nullptr;
    bool bMutable_ = false;
    bool bCanMove_ = false;
};

template<typename HandlerArg, typename T>
HandlerArg ConvertFinalStageArg(const TFinalArgDispatch<T>& sourceArg) {
    static_assert(std::is_same_v<TCanonicalArg<HandlerArg>, T>,
        "Final handler parameter types must match the canonical action payload types.");

    if constexpr (std::is_rvalue_reference_v<HandlerArg>) {
        if constexpr (std::is_const_v<std::remove_reference_t<HandlerArg>>) {
            return std::move(sourceArg.AsConstRef());
        } else {
            return sourceArg.AsRvalueRef();
        }
    } else if constexpr (std::is_lvalue_reference_v<HandlerArg>) {
        if constexpr (std::is_const_v<std::remove_reference_t<HandlerArg>>) {
            return sourceArg.AsConstRef();
        } else {
            return sourceArg.AsMutableRef();
        }
    } else {
        return sourceArg.MakeValue();
    }
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
    using FBorrowValidatorFunction = std::function<bool(const Args&...)>;
    using FBorrowHandlerFunction = std::function<void(const Args&...)>;
    using FValidatorEntry = TTypedHandlerEntry<KeyType, FBorrowValidatorFunction>;
    using FHandlerEntry = TTypedHandlerEntry<KeyType, FBorrowHandlerFunction>;

    class IFinalHandlerInvoker {
    public:
        virtual ~IFinalHandlerInvoker() = default;
        virtual void Invoke(TFinalArgDispatch<Args>... args) const = 0;
        virtual const std::string& GetHandlerSignatureId() const = 0;
    };

    template<typename... HandlerArgs>
    class TFinalHandlerInvoker final : public IFinalHandlerInvoker {
    public:
        using FFunctionType = std::function<void(HandlerArgs...)>;

        explicit TFinalHandlerInvoker(FFunctionType function)
            : Function_(std::move(function)) {
        }

        void Invoke(TFinalArgDispatch<Args>... args) const override {
            Function_(ConvertFinalStageArg<HandlerArgs>(args)...);
        }

        const std::string& GetHandlerSignatureId() const override {
            return HandlerSignatureId_;
        }

    private:
        FFunctionType Function_;
        std::string HandlerSignatureId_ = MakeHandlerSignatureId<HandlerArgs...>();
    };

    struct FFinalHandlerEntry : public THandlerBaseEntry<KeyType> {
        std::unique_ptr<IFinalHandlerInvoker> Invoker;
    };

    const std::string& GetSignatureId() const override {
        return SignatureId_;
    }

    std::size_t GetArgumentCount() const override {
        return sizeof...(Args);
    }

    template<typename... HandlerArgs>
    void AddValidator(const TNamedActionHandle<KeyType>& handle, std::function<bool(HandlerArgs...)> validator, const std::string& description, int priority) {
        if (!validator) {
            return;
        }

        static_assert(sizeof...(HandlerArgs) == sizeof...(Args),
            "Validator parameter count must match the action payload count.");
        static_assert((IsSupportedBorrowStageArgV<HandlerArgs> && ...),
            "Validators only support value parameters or const lvalue references.");

        Validators_.push_back({
            { handle, description, priority },
            [validator = std::move(validator)](const Args&... args) {
                return validator(ConvertBorrowStageArg<HandlerArgs>(args)...);
            }
        });
        SortByPriority(Validators_);
    }

    template<typename... HandlerArgs>
    void AddSequentialHandler(const TNamedActionHandle<KeyType>& handle, std::function<void(HandlerArgs...)> handler, const std::string& description, int priority) {
        if (!handler) {
            return;
        }

        static_assert(sizeof...(HandlerArgs) == sizeof...(Args),
            "Sequential handler parameter count must match the action payload count.");
        static_assert((IsSupportedBorrowStageArgV<HandlerArgs> && ...),
            "Sequential handlers only support value parameters or const lvalue references.");

        SequentialHandlers_.push_back({
            { handle, description, priority },
            [handler = std::move(handler)](const Args&... args) {
                handler(ConvertBorrowStageArg<HandlerArgs>(args)...);
            }
        });
        SortByPriority(SequentialHandlers_);
    }

    template<typename... HandlerArgs>
    void SetFinalHandler(const TNamedActionHandle<KeyType>& handle, std::function<void(HandlerArgs...)> handler, const std::string& description, int priority) {
        if (!handler) {
            FinalHandler_.reset();
            return;
        }

        static_assert(sizeof...(HandlerArgs) == sizeof...(Args),
            "Final handler parameter count must match the action payload count.");

        FinalHandler_ = FFinalHandlerEntry {
            { handle, description, priority },
            std::make_unique<TFinalHandlerInvoker<HandlerArgs...>>(std::move(handler))
        };
    }

    template<typename... HandlerArgs>
    void AddCompletionListener(const TNamedActionHandle<KeyType>& handle, std::function<void(HandlerArgs...)> listener, const std::string& description, int priority) {
        if (!listener) {
            return;
        }

        static_assert(sizeof...(HandlerArgs) == sizeof...(Args),
            "Completion listener parameter count must match the action payload count.");
        static_assert((IsSupportedBorrowStageArgV<HandlerArgs> && ...),
            "Completion listeners only support value parameters or const lvalue references.");

        CompletionListeners_.push_back({
            { handle, description, priority },
            [listener = std::move(listener)](const Args&... args) {
                listener(ConvertBorrowStageArg<HandlerArgs>(args)...);
            }
        });
        SortByPriority(CompletionListeners_);
    }

    template<typename... CallArgs>
    FNamedActionResult ExecuteWithArgs(CallArgs&&... args) const {
        static_assert(sizeof...(CallArgs) == sizeof...(Args),
            "Execution argument count must match the registered action payload count.");
        static_assert((std::is_same_v<TCanonicalArg<CallArgs>, Args> && ...),
            "Execution argument types must match the registered action payload types.");

        FNamedActionResult result;
        result.TotalValidators = Validators_.size();
        result.TotalHandlers = SequentialHandlers_.size() + (FinalHandler_.has_value() ? 1u : 0u);
        result.TotalListeners = CompletionListeners_.size();

        for (const FValidatorEntry& validatorEntry : Validators_) {
            try {
                if (!validatorEntry.Invoke(static_cast<const Args&>(args)...)) {
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
                handlerEntry.Invoke(static_cast<const Args&>(args)...);
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
                FinalHandler_->Invoker->Invoke(TFinalArgDispatch<Args>(std::forward<CallArgs>(args))...);
                ++result.ExecutedHandlers;
            } catch (const std::exception& exception) {
                result.ErrorMessage = "Final handler error (" + FinalHandler_->Invoker->GetHandlerSignatureId() + "): " + std::string(exception.what());
                return result;
            } catch (...) {
                result.ErrorMessage = "Final handler error (" + FinalHandler_->Invoker->GetHandlerSignatureId() + "): unknown exception.";
                return result;
            }
        }

        for (const FHandlerEntry& listenerEntry : CompletionListeners_) {
            try {
                listenerEntry.Invoke(static_cast<const Args&>(args)...);
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

    const std::string SignatureId_ = MakeCanonicalSignatureId<Args...>();
    std::vector<FValidatorEntry> Validators_;
    std::vector<FHandlerEntry> SequentialHandlers_;
    std::optional<FFinalHandlerEntry> FinalHandler_;
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
        using FVariantType = Private::TActionVariant<KeyType, Private::TCanonicalArg<Args>...>;

        FNamedActionResult result;
        const FVariantType* variant = ResolveVariant<Private::TCanonicalArg<Args>...>(actionKey);
        if (variant == nullptr) {
            result.ErrorMessage = "No named action registered for the requested key/signature.";
            NotifyGlobalCompletionListeners(actionKey, result);
            return result;
        }

        result = variant->template ExecuteWithArgs<Args...>(std::forward<Args>(args)...);
        NotifyGlobalCompletionListeners(actionKey, result);
        return result;
    }

    template<typename... Args>
    TNamedActionInvoker<KeyType, bAllowOverload, Private::TCanonicalArg<Args>...> AcquireInvoker(const KeyType& actionKey) const {
        return TNamedActionInvoker<KeyType, bAllowOverload, Private::TCanonicalArg<Args>...>(this, actionKey);
    }

    bool HasAction(const KeyType& actionKey) const {
        const auto it = Actions_.find(actionKey);
        return it != Actions_.end() && !it->second.empty();
    }

    template<typename... Args>
    bool HasActionWithArgs(const KeyType& actionKey) const {
        return ResolveVariant<Private::TCanonicalArg<Args>...>(actionKey) != nullptr;
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

    template<typename... ActionArgs, typename... CallArgs>
    FNamedActionResult ExecuteInvoker(
        const TNamedActionInvoker<KeyType, bAllowOverload, ActionArgs...>& invoker,
        CallArgs&&... args) const {
        using FVariantType = Private::TActionVariant<KeyType, ActionArgs...>;

        const FVariantType* variant = nullptr;
        if (invoker.CachedVariant_ != nullptr && invoker.CachedMutationSerial_ == MutationSerial_) {
            variant = static_cast<const FVariantType*>(invoker.CachedVariant_);
        } else {
            variant = ResolveVariant<ActionArgs...>(invoker.ActionKey_);
            invoker.CachedVariant_ = variant;
            invoker.CachedMutationSerial_ = MutationSerial_;
        }

        FNamedActionResult result;
        if (variant == nullptr) {
            result.ErrorMessage = "No named action registered for the requested key/signature.";
            NotifyGlobalCompletionListeners(invoker.ActionKey_, result);
            return result;
        }

        result = variant->template ExecuteWithArgs<CallArgs...>(std::forward<CallArgs>(args)...);
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
        const std::string signatureId = Private::MakeCanonicalSignatureId<Args...>();

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

        const std::string signatureId = Private::MakeCanonicalSignatureId<Args...>();
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

        auto* variant = GetOrCreateVariant<Private::TCanonicalArg<Args>...>(actionKey);
        if (variant == nullptr) {
            return {};
        }

        FHandle handle = MakeHandle(actionKey, ENamedActionHandlerType::Validator, Private::MakeCanonicalSignatureId<Args...>());
        variant->template AddValidator<Args...>(handle, std::move(validator), description, priority);
        ++MutationSerial_;
        return handle;
    }

    template<typename... Args>
    FHandle AddSequentialHandlerImpl(const KeyType& actionKey, std::function<void(Args...)> handler, const std::string& description, int priority) {
        if (!handler) {
            return {};
        }

        auto* variant = GetOrCreateVariant<Private::TCanonicalArg<Args>...>(actionKey);
        if (variant == nullptr) {
            return {};
        }

        FHandle handle = MakeHandle(actionKey, ENamedActionHandlerType::SequentialHandler, Private::MakeCanonicalSignatureId<Args...>());
        variant->template AddSequentialHandler<Args...>(handle, std::move(handler), description, priority);
        ++MutationSerial_;
        return handle;
    }

    template<typename... Args>
    FHandle SetFinalHandlerImpl(const KeyType& actionKey, std::function<void(Args...)> handler, const std::string& description, int priority) {
        if (!handler) {
            return {};
        }

        auto* variant = GetOrCreateVariant<Private::TCanonicalArg<Args>...>(actionKey);
        if (variant == nullptr) {
            return {};
        }

        FHandle handle = MakeHandle(actionKey, ENamedActionHandlerType::FinalHandler, Private::MakeCanonicalSignatureId<Args...>());
        variant->template SetFinalHandler<Args...>(handle, std::move(handler), description, priority);
        ++MutationSerial_;
        return handle;
    }

    template<typename... Args>
    FHandle AddCompletionListenerImpl(const KeyType& actionKey, std::function<void(Args...)> listener, const std::string& description, int priority) {
        if (!listener) {
            return {};
        }

        auto* variant = GetOrCreateVariant<Private::TCanonicalArg<Args>...>(actionKey);
        if (variant == nullptr) {
            return {};
        }

        FHandle handle = MakeHandle(actionKey, ENamedActionHandlerType::CompletionListener, Private::MakeCanonicalSignatureId<Args...>());
        variant->template AddCompletionListener<Args...>(handle, std::move(listener), description, priority);
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
