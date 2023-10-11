#pragma once
#include "array_ptr.h" 
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <stdexcept>

class ReserveProxyObj
{
public:
    ReserveProxyObj() = delete;
    ReserveProxyObj(size_t new_capacity):capacity_to_reserve_(new_capacity){}
    size_t capacity_to_reserve_;

};
ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}
template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) :SimpleVector(size, 0) {}

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) {
        ArrayPtr<Type> new_items(size);
        std::fill(&new_items[0], &new_items[size], value);
        size_ = size;
        capacity_ = size;
        items_.swap(new_items);
    }
    SimpleVector(SimpleVector&& other)
    {
        items_ = std::move(other.items_);
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
    }
    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) {
        size_ = init.size();
        capacity_ = size_;
        ArrayPtr<Type> new_items(size_);
        for (size_t i = 0; i < size_; i++)
        {
            auto it = init.begin() + i;

            new_items[i] = *it;
        }
        items_.swap(new_items);
    }

    // Создает вектор с зарезервируемым количеством элементов
    SimpleVector(ReserveProxyObj obj)
    {
        this->Reserve(obj.capacity_to_reserve_);
    }

    ~SimpleVector()
    {
        //items_.Release();
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        // Напишите тело самостоятельно
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        // Напишите тело самостоятельно
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_)
        {
            throw std::out_of_range("out of range");
        }
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_)
        {
            throw std::out_of_range("out of range");
        }
        return items_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    void Reserve(size_t new_capacity)
    {
        if (new_capacity <= capacity_)
        {
            return;
        }
        ArrayPtr<Type> new_array(new_capacity);
        std::copy(&items_[0], &items_[size_], &new_array[0]);
        items_.swap(new_array);
        (void)new_array.Release();
        capacity_ = new_capacity;
    }
    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        // Напишите тело самостоятельно
        if (new_size < capacity_)
        {
            this->Fill(&items_[size_], &items_[capacity_], Type());
            
        }
        else
        {
            auto new_capacity = std::max(new_size, 2 * capacity_);
            ArrayPtr<Type> new_array(new_capacity);
            std::move(&items_[0], &items_[size_], &new_array[0]);
            this->Fill(&new_array[size_], &new_array[new_size], Type());
            items_.swap(new_array);
            (void)new_array.Release();
            capacity_ = new_capacity;
        }
        size_ = new_size;
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return Iterator{ &items_[0] };
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return Iterator{ &items_[size_] };
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return ConstIterator{ &items_[0] };
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return ConstIterator{ &items_[size_] };
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return begin();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return end();
    }

    SimpleVector(const SimpleVector& other) {
        CopyAndSwap(other);
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs)
        {
            CopyAndSwap(rhs);
        }
        return *this;
    }
    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (IsEmpty())
        {
            ArrayPtr<Type> new_items(1);
            items_.swap(new_items);
        }
        items_[size_] = std::move(item);
        size_++;
        if (size_ > capacity_) {
            Resize(size_);
        }
    }
    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(Type&& item) {
        if (IsEmpty())
        {
            ArrayPtr<Type> new_items(1);
            items_.swap(new_items);
        }
        items_[size_] = std::move(item);
        size_++;
        if (size_ > capacity_) {
            Resize(size_);
        }
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(pos >= begin() && pos <= end());
        auto dist = this->end() - pos;
        auto length_dist = pos - this->begin();
        if (IsEmpty())
        {
            ArrayPtr<Type> new_items(1);
            items_.swap(new_items);
            items_[length_dist] = std::move(value);
        }
        else
        {
            ArrayPtr<Type> copy_arr(dist);
            std::move(&items_[length_dist], &items_[size_], &copy_arr[0]);
            items_[length_dist] = std::move(value);
            std::move(&copy_arr[0], &copy_arr[dist], &items_[length_dist + 1]);
        }

        size_++;
        if (size_ > capacity_) {
            Resize(size_);
        }
        return Iterator{&items_[length_dist]};
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        if (!IsEmpty())
        {
            size_ --;
        }
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos <= end());
        if (IsEmpty())
        {
            return {};
        }
        auto dist = this->end() - pos;
        auto length_dist = pos - this->begin();
        ArrayPtr<Type> copy_arr(dist);
        std::move(&items_[length_dist + 1], &items_[size_], &copy_arr[0]);
        std::move(&copy_arr[0], &copy_arr[dist], &items_[length_dist]);
        size_--;
        Resize(size_);
        return Iterator{ &items_[length_dist] };
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        this->items_.swap(other.items_);
        std::swap(this->size_, other.size_);
        std::swap(this->capacity_, other.capacity_);
    }
private:
    void CopyAndSwap(const SimpleVector &other)
    {
        SimpleVector copy_vector(other.GetSize());
        std::copy(other.begin(), other.end(), copy_vector.begin());
        copy_vector.capacity_ = other.capacity_;
        this->swap(copy_vector);
    }
    template <typename It>
    void Fill(It begin, It end, Type& value)
    {
        for (auto it = begin; it != end; it++)
        {
            *it = value;
        }
    }
    template <typename It>
    void Fill(It begin, It end, Type&& value)
    {
        (void)value;
        for (auto it = begin; it != end; it++)
        {
            // не понял как сделать что бы класть именно 
            // value поскольку при использовании std::move
            // в value после операции лежит 0 (из за exchange в X)
            *it = std::move(Type{});
        }
    }
    // Вместо сырого указателя лучше использовать умный указатель, такой как ArrayPtr
    ArrayPtr<Type> items_;

    size_t size_ = 0;
    size_t capacity_ = 0;


};
template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    // Заглушка. Напишите тело самостоятельно

    return (std::equal(lhs.begin(), lhs.end(), rhs.begin())) && (lhs.GetSize() == rhs.GetSize());

}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    // Заглушка. Напишите тело самостоятельно
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    // Заглушка. Напишите тело самостоятельно
    return (std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end()));
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    // Заглушка. Напишите тело самостоятельно
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    // Заглушка. Напишите тело самостоятельно
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    // Заглушка. Напишите тело самостоятельно
    return !(lhs < rhs);
}