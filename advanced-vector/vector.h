#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include<stdexcept>
#include<memory>
#include<concepts>
#include<type_traits>
#include<cstring>
#include<tuple>
template<typename T>
class RawMemory;


template <typename T>
class Vector {



    RawMemory<T> data_;
    size_t size_ = 0;
    Vector& CopyExisting(const Vector& rhs) {
        size_t common_size = std::min(size_, rhs.size_);
        std::copy(rhs.begin(), rhs.begin() + common_size, begin());
        if (rhs.size_ > size_) {
            std::uninitialized_copy_n(rhs.begin() + size_, rhs.size_ - size_, begin() + size_);
        }
        else {
            std::destroy_n(begin() + rhs.size_, size_ - rhs.size_);
        }
        size_ = rhs.size_;
        return *this;
    }
public:
    using iterator = T*;
    using const_iterator = const T*;
    Vector() = default;
    explicit Vector(size_t size) :data_(size), size_(size) {
        if (size == 0) {
            return;
        }
        std::uninitialized_value_construct_n(reinterpret_cast<T*>(data_.GetAddress()), size);
    }
    Vector(const Vector& other) :data_(other.size_), size_(other.size_) {
        if (other.size_ == 0) {
            return;
        }
        std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
        size_ = other.size_;
    }
    Vector(Vector&& other) noexcept {
        Swap(other);
    }
    void Resize(size_t new_size) {
        if (new_size == size_) {
            return;
        }
        else if (new_size < size_) {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
            size_ = new_size;
        }
        else {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
            size_ = new_size;
        }
    }
    void PushBack(const T& value) {
        EmplaceBack(value);
    }
    void PushBack(T&& value) {
        EmplaceBack(std::move(value));
    }

    void PopBack() {
        assert(size_ != 0);
        Resize(size_ - 1);
    }
    size_t Size() const noexcept {
        return size_;
    }
    size_t Capacity() const noexcept {
      

        return data_.Capacity();
    }
    Vector& operator=(const Vector& rhs) {
        if (this == &rhs) return *this;

        if (rhs.size_ > data_.Capacity()) {
            Vector tmp(rhs);
            Swap(tmp);
        }
        else {
            return CopyExisting(rhs);

        }

        return *this;
    }
    Vector& operator=(Vector&& rhs) noexcept {
        if (this == &rhs) {
            return *this;
        }
        Swap(rhs);
        return *this;
    }
    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }
    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }
    ~Vector() noexcept {
        if (data_.GetAddress() == nullptr) {
            return;
        }
        std::destroy_n(data_.GetAddress(), size_);
    }
    void Swap(Vector& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
    }
    void Reserve(size_t capacity) {
        if (capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(capacity);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }
    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (size_ < data_.Capacity()) {
            new (data_.GetAddress() + size_) T(std::forward<Args>(args)...);
            ++size_;
            return *(data_.GetAddress() + size_ - 1);
        }
        size_t new_cap = size_ == 0 ? 1 : size_ * 2;
        RawMemory<T> new_data(new_cap);

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        new (new_data.GetAddress() + size_) T(std::forward<Args>(args)...);
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
        ++size_;
        return *(data_.GetAddress() + size_ - 1);
    }
    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        assert(pos >= begin() && pos <= end());
        size_t index = pos - begin();
        T* result = nullptr;
        if (size_ < data_.Capacity()) {
            T* raw = data_.GetAddress();

            if (size_ == 0 || index == size_) {
                new (raw + index) T(std::forward<Args>(args)...);
                result = raw + index;
            }
            else {
                T value(std::forward<Args>(args)...);

                if constexpr (!std::is_copy_constructible_v < T> || std::is_nothrow_move_constructible_v<T>) {
                    new (end()) T(std::move(data_[size_ - 1]));
                    std::move_backward(begin() + index, end()-1, end());
                    data_[index] = std::move(value);
                }
                else {
                    new (raw + size_) T(data_[size_ - 1]);
                    std::copy_backward(raw + index, raw + size_, raw + size_ + 1);
                    data_[index] = value;
                }
            }
            ++size_;
            result = raw + index;
        }
        else {
            size_t new_capacity = data_.Capacity() == 0 ? 1 : data_.Capacity() * 2;
            RawMemory<T> new_data(new_capacity);
            T* new_raw = new_data.GetAddress();
            if constexpr (!std::is_copy_constructible_v < T> || std::is_nothrow_move_constructible_v<T>) {
                std::uninitialized_move_n(begin(), index, new_raw);
                new (new_raw + index) T(std::forward<Args>(args)...);
                std::uninitialized_move_n(begin() + index, size_ - index, new_raw + index + 1);
            }
            else {
                std::uninitialized_copy_n(begin(), index, new_raw);
                new (new_raw + index) T(std::forward<Args>(args)...);
                std::uninitialized_copy_n(begin() + index, size_ - index, new_raw + index + 1);
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
            ++size_;
            result = begin() + index;
        }
        return result;
    }


    iterator Erase(const_iterator pos) {
        assert(pos >= begin() && pos < end());
        size_t index = pos - begin();
        iterator non_const_pos = begin() + index;
        if constexpr (std::is_move_assignable_v<T>) {
            std::move(non_const_pos + 1, end(), non_const_pos);
        }
        else {
            std::copy(non_const_pos + 1, end(), non_const_pos);
        }
        std::destroy_at(end() - 1);
        --size_;
        return begin() + index;
    }
    iterator begin() noexcept {
        return data_.GetAddress();
    }
    iterator end()  noexcept {
        return data_.GetAddress() + size_;
    }
    const_iterator begin() const noexcept
    {
        return data_.GetAddress();
    }
    const_iterator end() const noexcept {
        return data_.GetAddress() + size_;
    }
    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }
    const_iterator cend() const noexcept {
        return data_.GetAddress() + size_;
    }


};



template <typename T>
class RawMemory {
public:
    RawMemory() = default;
    RawMemory(const RawMemory& rh) = delete;
    RawMemory(RawMemory&& other) noexcept {
        Swap(other);
    }
    RawMemory& operator=(RawMemory&& rhs) noexcept {
        if (this!= &rhs) {
            Swap(rhs);
        }
        return *this;
    }
    RawMemory& operator=(const RawMemory& rh) = delete;
    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};
