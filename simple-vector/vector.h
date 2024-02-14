
#pragma once

#include<algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <new>
#include <utility>


template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }



    RawMemory(const RawMemory&) = delete;

    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept {
        buffer_ = other.buffer_;
        capacity_ = other.capacity_;
        other.buffer_ = nullptr;
        other.capacity_ = 0;
    }

    RawMemory& operator=(RawMemory&& rhs) noexcept
    {
        if (this != &rhs) {
            RawMemory rhs_tmp(std::move(rhs));
            this->Swap(rhs_tmp);
        }
        return *this;
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

template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept
    {
        return const_cast<iterator>(this->cbegin());
    }

    iterator end() noexcept
    {
        return const_cast<iterator>(this->cend());
    }

    const_iterator begin() const noexcept
    {
        return this->cbegin();
    }

    const_iterator end() const noexcept
    {
        return this->cend();
    }

    const_iterator cbegin() const noexcept
    {
        return data_.GetAddress();
    }
    const_iterator cend() const noexcept
    {
        return data_.GetAddress() + size_;
    }

    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)  //
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size_);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)  //
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept
    {
        this->Swap(other);
    }

    Vector& operator=(const Vector& rhs)
    {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            }
            else {
                if (rhs.size_ < this->size_) {
                    std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + rhs.size_, data_.GetAddress());
                    std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
                }
                else {
                    std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + size_, data_.GetAddress());
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_);
                }
            }
            size_ = rhs.size_;
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            Swap(rhs);
        }
        return *this;
    }

    void Swap(Vector& other) noexcept
    {
        this->data_.Swap(other.data_);
        std::swap(this->size_, other.size_);
    }

    ~Vector()
    {
        std::destroy_n(data_.GetAddress(), size_);
    }


    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }


    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data = RawMemory<T>(new_capacity);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    void Resize(size_t new_size)
    {
        if (new_size <= this->size_) {
            std::destroy_n(data_.GetAddress() + new_size, this->size_ - new_size);

        }
        else {
            this->Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - this->size_);
        }
        size_ = new_size;
    }

    template <typename Type>
    void PushBack(Type&& value)
    {
        if (size_ == this->Capacity()) {
            RawMemory<T> new_data(this->size_ == 0 ? 1 : this->size_ * 2);
            new(new_data.GetAddress() + this->size_) T(std::forward<Type>(value));
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), this->size_);
            data_.Swap(new_data);
        }
        else {
            new(data_.GetAddress() + this->size_) T(std::forward<Type>(value));
        }

        size_++;
    }


    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (size_ == this->Capacity()) {
            RawMemory<T> new_data(this->size_ == 0 ? 1 : this->size_ * 2);
            new(new_data.GetAddress() + this->size_) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), this->size_);
            data_.Swap(new_data);
        }
        else {
            new(data_.GetAddress() + this->size_) T(std::forward<Args>(args)...);
        }

        size_++;
        return *(data_.GetAddress() + this->size_ - 1);
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args)
    {
        std::size_t offset = std::distance(this->begin(), const_cast<iterator>(pos));;
        if (this->size_ == this->Capacity()) {
            RawMemory<T> new_data(this->size_ == 0 ? 1 : this->size_ * 2);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), this->size_);
            data_.Swap(new_data);
        }

        auto& last_value = *(this->end() - 1);

        new(this->end()) T(std::forward<T>(last_value));
        std::move_backward(this->begin() + offset, this->end(), this->end() + 1);
        *(this->begin() + offset) = T(std::forward<Args>(args)...);


        size_++;
        return this->begin() + offset;
    }

    void PopBack() /* noexcept */
    {
        if (this->size_ > 0) {
            size_--;
            std::destroy_at(this->end());

        }
    }

    iterator Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/
    {
        auto offset = pos - this->begin();
        std::move_backward(this->begin() + offset, this->end(), this->end() - 1);
        this->PopBack();
        return this->begin() + offset;
    }

    iterator Insert(const_iterator pos, const T& value)
    {
        return this->Emplace(pos, value);

    }

    iterator Insert(const_iterator pos, T&& value)
    {
        return this->Emplace(pos, std::move(value));

    }
private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Вызывает деструкторы n объектов массива по адресу buf
    static void DestroyN(T* buf, size_t n) noexcept {
        for (size_t i = 0; i != n; ++i) {
            Destroy(buf + i);
        }
    }

    // Создаёт копию объекта elem в сырой памяти по адресу buf
    static void CopyConstruct(T* buf, const T& elem) {
        new (buf) T(elem);
    }
    // Вызывает деструктор объекта по адресу buf
    static void Destroy(T* buf) noexcept {
        buf->~T();
    }


private:
    RawMemory<T> data_;
    size_t size_ = 0;

};