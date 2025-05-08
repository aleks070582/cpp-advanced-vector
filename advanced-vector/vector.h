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
        data_ = std::move(other.data_);
        size_ = other.size_;
        other.size_ = 0;
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
        if (size_ < data_.Capacity()) {
            new (data_.GetAddress() + size_) T(value);
            ++size_;
            return;
        }
        size_t new_cap = size_ == 0 ? 1 : size_ * 2;
        RawMemory<T> new_data(new_cap);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        new (new_data.GetAddress() + size_) T(value);

        // 3) теперь, когда ничего не упало, уничтожаем старые и меняем буферы
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
        ++size_;
    }
    void PushBack(T&& value) {
        if (size_ < data_.Capacity()) {
            new (data_.GetAddress() + size_) T(std::move(value));
            ++size_;
            return;
        }
        size_t new_cap = size_ == 0 ? 1 : size_ * 2;
        RawMemory<T> new_data(new_cap);

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        new (new_data.GetAddress() + size_) T(std::move(value));
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
        ++size_;
    }

    void PopBack() {
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
            size_t i = 0;
            for (; i < std::min(size_, rhs.size_); ++i) {
                data_[i] = rhs.data_[i];
            }
            if (i < rhs.size_) {
                std::uninitialized_copy_n(rhs.data_ + i, rhs.size_ - i, data_ + i);
            }
            else {
                std::destroy_n(data_ + i, size_ - i);
            }
            size_ = rhs.size_;
        }

        return *this;
    }
    Vector& operator=(Vector&& rhs) noexcept {
        if (this == &rhs) {
            return *this;
        }
        data_ = std::move(rhs.data_);
        size_ = rhs.size_;
        rhs.size_ = 0;
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
        buffer_ = std::move(other.buffer_);
        capacity_ = other.capacity_;
        other.capacity_ = 0;
    }
    RawMemory& operator=(RawMemory&& rhs) noexcept {
        if (this->buffer_ != rhs.buffer_) {
            capacity_ = rhs.capacity_;
            std::swap(buffer_, rhs.buffer_);
            rhs.buffer_ = nullptr;
            rhs.capacity_ = 0;
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
}
