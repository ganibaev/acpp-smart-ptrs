#pragma once

#include <cstddef>  // std::nullptr_t
#include <utility>
#include <memory>
#include <type_traits>

// Primary template
template <typename T, typename Deleter = std::default_delete<T>>
class UniquePtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : raw_ptr_(ptr){};
    UniquePtr(T* ptr, Deleter deleter) : raw_ptr_(ptr), del_(std::forward<Deleter>(deleter)){};

    UniquePtr(UniquePtr&& other) noexcept
        : raw_ptr_(other.Release()), del_(std::forward<Deleter>(other.GetDeleter())){};

    // Upcasting
    template <typename Up, typename Ep>
    UniquePtr(UniquePtr<Up, Ep>&& other) noexcept
        : raw_ptr_(other.Release()), del_(std::forward<Ep>(other.GetDeleter())) {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        Reset(other.Release());
        del_ = std::forward<Deleter>(other.GetDeleter());
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        if (raw_ptr_) {
            GetDeleter()(std::move(raw_ptr_));
            raw_ptr_ = nullptr;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* res_ptr = Get();
        raw_ptr_ = nullptr;
        return res_ptr;
    }
    void Reset(T* ptr = nullptr) {
        std::swap(raw_ptr_, ptr);
        if (ptr) {
            GetDeleter()(std::move(ptr));
        }
    }
    void Swap(UniquePtr& other) {
        std::swap(this->raw_ptr_, other.raw_ptr_);
        std::swap(this->del_, other.del_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return raw_ptr_;
    }
    Deleter& GetDeleter() {
        return del_;
    }
    const Deleter& GetDeleter() const {
        return del_;
    }
    explicit operator bool() const {
        return Get() == nullptr ? false : true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    std::add_lvalue_reference_t<T> operator*() const {
        if (Get()) {
            return *Get();
        }
    }
    T* operator->() const {
        if (Get()) {
            return Get();
        }
    }

    // Ban copying

    UniquePtr& operator=(const UniquePtr&) = delete;
    UniquePtr(const UniquePtr&) = delete;

private:
    T* raw_ptr_;
    [[no_unique_address]] Deleter del_;
};

// Specialization for arrays
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : raw_ptr_(ptr){};
    UniquePtr(T* ptr, Deleter deleter) : raw_ptr_(ptr), del_(std::forward<Deleter>(deleter)){};

    UniquePtr(UniquePtr&& other)
        : raw_ptr_(other.Release()), del_(std::forward<Deleter>(other.GetDeleter())){};

    // Upcasting
    template <typename Up, typename Ep>
    UniquePtr(UniquePtr<Up, Ep>&& other) noexcept
        : raw_ptr_(other.Release()), del_(std::forward<Ep>(other.GetDeleter())) {
    }

    template <typename Up, typename Ep>
    UniquePtr& operator=(UniquePtr<Up, Ep>&& other) noexcept {
        Reset(other.Release());
        del_ = std::forward<Ep>(other.GetDeleter());
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr&& other) {
        Reset(other.Release());
        del_ = std::forward<Deleter>(other.GetDeleter());
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        if (raw_ptr_) {
            GetDeleter()(std::move(raw_ptr_));
            raw_ptr_ = nullptr;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* res_ptr = Get();
        raw_ptr_ = nullptr;
        return res_ptr;
    }
    void Reset(T* ptr = nullptr) {
        std::swap(raw_ptr_, ptr);
        if (ptr) {
            GetDeleter()(std::move(ptr));
        }
    }
    void Swap(UniquePtr& other) {
        std::swap(this->raw_ptr_, other->raw_ptr_);
        std::swap(this->del_, other->del_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return raw_ptr_;
    }
    Deleter& GetDeleter() {
        return del_;
    }
    const Deleter& GetDeleter() const {
        return del_;
    }
    explicit operator bool() const {
        return Get() == nullptr ? false : true;
    }

    std::add_lvalue_reference_t<T> operator[](size_t i) const {
        if (Get()) {
            return Get()[i];
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    std::add_lvalue_reference_t<T> operator*() const {
        if (Get()) {
            return *Get();
        }
    }
    T* operator->() const {
        if (Get()) {
            return Get();
        }
    }

    // Ban copying

    UniquePtr& operator=(const UniquePtr&) = delete;
    UniquePtr(const UniquePtr&) = delete;

private:
    T* raw_ptr_;
    [[no_unique_address]] Deleter del_;
};
