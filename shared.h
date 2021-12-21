#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t
#include <utility>

struct ControlBlockBase {
    virtual ~ControlBlockBase() = default;
    virtual void OnZeroShared() = 0;
    virtual void OnZeroWeak() = 0;
    virtual void IncrSharedCount() = 0;
    virtual void DecrSharedCount() = 0;
    virtual size_t GetSharedCount() const = 0;
};

template <typename T>
struct ControlBlockPtr : ControlBlockBase {
    ControlBlockPtr(T* ptr) : p_obj_(ptr) {
    }
    void IncrSharedCount() override {
        shared_cnt_++;
    }
    void DecrSharedCount() override {
        --shared_cnt_;
        if (shared_cnt_ == 0 && weak_cnt_ == 0) {
            OnZeroShared();
            OnZeroWeak();
        } else if (shared_cnt_ == 0) {
            OnZeroShared();
        }
    }
    size_t GetSharedCount() const override {
        return shared_cnt_;
    }
    size_t weak_cnt_ = 0;
    size_t shared_cnt_ = 1;
    T* p_obj_;

    void OnZeroShared() override {
        delete p_obj_;
    }

    void OnZeroWeak() override {
        delete this;
    }

    ~ControlBlockPtr() override = default;
};

template <typename T, typename... Args>
struct ControlBlockMakeShared : ControlBlockBase {
    ControlBlockMakeShared() {
    }
    void IncrSharedCount() override {
        shared_cnt_++;
    }
    void DecrSharedCount() override {
        --shared_cnt_;
        if (shared_cnt_ == 0 && weak_cnt_ == 0) {
            OnZeroShared();
            OnZeroWeak();
        } else if (shared_cnt_ == 0) {
            OnZeroShared();
        }
    }
    size_t GetSharedCount() const override {
        return shared_cnt_;
    }
    size_t weak_cnt_ = 0;
    size_t shared_cnt_ = 1;
    std::aligned_storage_t<sizeof(T), alignof(T)> holder_;

    void OnZeroShared() override {
        reinterpret_cast<T*>(&holder_)->~T();
    }

    void OnZeroWeak() override {
        delete this;
    }
};

template <typename T>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() : raw_ptr_(nullptr) {
    }
    SharedPtr(std::nullptr_t) : raw_ptr_(nullptr) {
    }

    template <typename Y>
    explicit SharedPtr(Y* ptr) {
        p_ctrl_block_ = new ControlBlockPtr<Y>(ptr);
        raw_ptr_ = ptr;
    }

    explicit SharedPtr(T* ptr) {
        p_ctrl_block_ = new ControlBlockPtr<T>(ptr);
        raw_ptr_ = ptr;
    }

    SharedPtr(const SharedPtr& other) noexcept {
        if (!other) {
            raw_ptr_ = nullptr;
        } else {
            raw_ptr_ = other.raw_ptr_;
            p_ctrl_block_ = other.p_ctrl_block_;
            p_ctrl_block_->IncrSharedCount();
        }
    }

    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other) noexcept {
        if (!other) {
            raw_ptr_ = nullptr;
        } else {
            raw_ptr_ = other.raw_ptr_;
            p_ctrl_block_ = other.p_ctrl_block_;
            p_ctrl_block_->IncrSharedCount();
        }
    }

    SharedPtr(SharedPtr&& other) noexcept {
        p_ctrl_block_ = other.p_ctrl_block_;
        raw_ptr_ = other.raw_ptr_;
        other.p_ctrl_block_ = nullptr;
        other.raw_ptr_ = nullptr;
    }

    template <typename Y>
    SharedPtr(SharedPtr<Y>&& other) noexcept {
        p_ctrl_block_ = other.p_ctrl_block_;
        raw_ptr_ = other.raw_ptr_;
        other.p_ctrl_block_ = nullptr;
        other.raw_ptr_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) {
        if (!other) {
            raw_ptr_ = nullptr;
        }
        p_ctrl_block_ = other.p_ctrl_block_;
        p_ctrl_block_->IncrSharedCount();
        raw_ptr_ = ptr;
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        SharedPtr<T>(other).Swap(*this);
        return *this;
    }

    template <typename Y>
    SharedPtr& operator=(const SharedPtr<Y>& other) {
        SharedPtr<T>(other).Swap(*this);
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) {
        SharedPtr<T>(std::move(other)).Swap(*this);
        return *this;
    }

    template <typename Y>
    SharedPtr& operator=(SharedPtr<Y>&& other) {
        SharedPtr<T>(std::move(other)).Swap(*this);
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        if (p_ctrl_block_) {
            p_ctrl_block_->DecrSharedCount();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() noexcept {
        SharedPtr().Swap(*this);
    }

    template <typename Y>
    void Reset(Y* ptr) {
        SharedPtr<T>(ptr).Swap(*this);
    }

    void Swap(SharedPtr& other) noexcept {
        std::swap(p_ctrl_block_, other.p_ctrl_block_);
        std::swap(raw_ptr_, other.raw_ptr_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return raw_ptr_;
    }
    std::add_lvalue_reference_t<T> operator*() const {
        return *Get();
    }
    T* operator->() const {
        return Get();
    }
    size_t UseCount() const {
        if (!p_ctrl_block_) {
            return 0;
        } else {
            return p_ctrl_block_->GetSharedCount();
        }
    }
    explicit operator bool() const {
        return Get() == nullptr ? false : true;
    }

    template <typename Y, typename... Args>
    friend SharedPtr<Y> MakeShared(Args&&... args);

    template <typename Y>
    friend class SharedPtr;

private:
    ControlBlockBase* p_ctrl_block_ = nullptr;
    T* raw_ptr_;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.Get() == right.Get();
}

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    auto result = SharedPtr<T>();
    result.p_ctrl_block_ = new ControlBlockMakeShared<T>;
    result.raw_ptr_ =
        new (&(dynamic_cast<ControlBlockMakeShared<T>*>(result.p_ctrl_block_)->holder_))
            T(std::forward<Args>(args)...);
    return result;
}