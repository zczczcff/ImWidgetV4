#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace ImWidgetV4 {

struct FDelegateHandle {
    std::uint64_t Id = 0;

    bool IsValid() const {
        return Id != 0;
    }

    bool operator==(const FDelegateHandle& other) const {
        return Id == other.Id;
    }

    bool operator!=(const FDelegateHandle& other) const {
        return !(*this == other);
    }
};

template<typename... Args>
class TMulticastDelegate {
public:
    using FCallback = std::function<void(Args...)>;

    FDelegateHandle Add(FCallback callback) {
        if (!callback) {
            return {};
        }

        FDelegateHandle handle { NextHandleId_++ };
        Entries_.push_back({handle, std::move(callback), false});
        return handle;
    }

    template<typename Callable>
    FDelegateHandle AddLambda(Callable&& callback) {
        return Add(FCallback(std::forward<Callable>(callback)));
    }

    bool Remove(const FDelegateHandle& handle) {
        for (FEntry& entry : Entries_) {
            if (entry.Handle == handle && !entry.bRemoved) {
                entry.bRemoved = true;
                if (BroadcastDepth_ == 0) {
                    Compact();
                }
                return true;
            }
        }

        return false;
    }

    void Clear() {
        if (BroadcastDepth_ == 0) {
            Entries_.clear();
            return;
        }

        for (FEntry& entry : Entries_) {
            entry.bRemoved = true;
        }
    }

    bool IsBound() const {
        for (const FEntry& entry : Entries_) {
            if (!entry.bRemoved) {
                return true;
            }
        }

        return false;
    }

    void Broadcast(Args... args) {
        std::vector<FCallback> callbacks;
        callbacks.reserve(Entries_.size());

        for (const FEntry& entry : Entries_) {
            if (!entry.bRemoved && entry.Callback) {
                callbacks.push_back(entry.Callback);
            }
        }

        ++BroadcastDepth_;
        for (const FCallback& callback : callbacks) {
            callback(args...);
        }
        --BroadcastDepth_;

        if (BroadcastDepth_ == 0) {
            Compact();
        }
    }

private:
    struct FEntry {
        FDelegateHandle Handle;
        FCallback Callback;
        bool bRemoved = false;
    };

    void Compact() {
        Entries_.erase(
            std::remove_if(
                Entries_.begin(),
                Entries_.end(),
                [](const FEntry& entry) {
                    return entry.bRemoved;
                }),
            Entries_.end()
        );
    }

    std::vector<FEntry> Entries_;
    std::uint64_t NextHandleId_ = 1;
    int BroadcastDepth_ = 0;
};

} // namespace ImWidgetV4
