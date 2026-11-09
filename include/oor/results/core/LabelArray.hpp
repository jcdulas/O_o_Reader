#pragma once

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace oor::results
{

// The labels of the rows of a table, one item a row (the IDs, the grid points, the elements, the layers, the positions,
// the codes of a text): a vector of its own, or the items of a block that another owner keeps alive and that nothing
// writes (the buffer of a companion package that a table made in memory keeps without a copy,
// `MemoryResults.add_table(copy=False)`; P11.4). It reads as a `std::vector` and keeps its names, so that what fills
// the labels and what reads them uses it as it did a vector. A change of a kept array first copies its items into a
// vector of its own (the block is never written); so does a read through a non-const reference, which may write: a kept
// array is read through a const one (`std::as_const`; a span made of a non-const array calls its non-const `data`). A
// copy of a kept array keeps the same block (the keeper is shared); a copy of an array of its own copies the vector.
template <typename T>
class LabelArray
{
public:
    // the names of std::vector
    // NOLINTBEGIN(readability-identifier-naming)
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    // NOLINTEND(readability-identifier-naming)

    LabelArray() noexcept = default;
    // From a vector, which it takes as its own (not explicit: a vector is given where a LabelArray is expected).
    LabelArray(std::vector<T> items) noexcept : m_owned(std::move(items)) { sync(); }
    LabelArray(std::initializer_list<T> items) : m_owned(items) { sync(); }
    explicit LabelArray(size_type count, const T& value = T()) : m_owned(count, value) { sync(); }
    template <std::input_iterator Iterator>
    LabelArray(Iterator first, Iterator last) : m_owned(first, last)
    {
        sync();
    }

    // The `count` items at `items`, kept by `keeper` for as long as an array has them; nothing writes them.
    [[nodiscard]] static LabelArray kept(const T* items, size_type count, std::shared_ptr<const void> keeper) noexcept
    {
        LabelArray array;
        array.m_items = items;
        array.m_size = count;
        array.m_keeper = std::move(keeper);
        return array;
    }

    LabelArray(const LabelArray& other) : m_owned(other.m_owned), m_keeper(other.m_keeper)
    {
        if (m_keeper)
        {
            m_items = other.m_items;
            m_size = other.m_size;
        }
        else
        {
            sync();
        }
    }
    LabelArray(LabelArray&& other) noexcept
        : m_owned(std::move(other.m_owned)),
          m_items(other.m_items),
          m_size(other.m_size),
          m_keeper(std::move(other.m_keeper))
    {
        if (!m_keeper)
            sync();
        other.m_owned.clear();
        other.sync();
    }
    LabelArray& operator=(const LabelArray& other)
    {
        if (this != &other)
            *this = LabelArray(other);
        return *this;
    }
    LabelArray& operator=(LabelArray&& other) noexcept
    {
        if (this != &other)
        {
            m_owned = std::move(other.m_owned);
            m_keeper = std::move(other.m_keeper);
            m_items = other.m_items;
            m_size = other.m_size;
            if (!m_keeper)
                sync();
            other.m_owned.clear();
            other.sync();
        }
        return *this;
    }
    LabelArray& operator=(std::vector<T> items) noexcept
    {
        m_keeper.reset();
        m_owned = std::move(items);
        sync();
        return *this;
    }
    LabelArray& operator=(std::initializer_list<T> items) { return *this = std::vector<T>(items); }
    ~LabelArray() = default;

    // Whether the items are those of a block kept from another owner (not copied).
    [[nodiscard]] bool isKept() const noexcept { return static_cast<bool>(m_keeper); }
    // The owner of a kept block (empty for an array of its own).
    [[nodiscard]] const std::shared_ptr<const void>& keeper() const noexcept { return m_keeper; }

    // ---- reading
    [[nodiscard]] size_type size() const noexcept { return m_size; }
    [[nodiscard]] bool empty() const noexcept { return m_size == 0; }
    [[nodiscard]] const T* data() const noexcept { return m_items; }
    [[nodiscard]] const T& operator[](size_type index) const noexcept { return m_items[index]; }
    [[nodiscard]] const T& at(size_type index) const
    {
        if (index >= m_size)
            throw std::out_of_range("LabelArray::at: the index is past the last item");
        return m_items[index];
    }
    [[nodiscard]] const T& front() const noexcept { return m_items[0]; }
    [[nodiscard]] const T& back() const noexcept { return m_items[m_size - 1]; }
    [[nodiscard]] const_iterator begin() const noexcept { return m_items; }
    [[nodiscard]] const_iterator end() const noexcept { return m_items + m_size; }
    [[nodiscard]] const_iterator cbegin() const noexcept { return begin(); }
    [[nodiscard]] const_iterator cend() const noexcept { return end(); }
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    [[nodiscard]] const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    [[nodiscard]] std::span<const T> span() const noexcept { return {m_items, m_size}; }
    operator std::span<const T>() const noexcept { return span(); }
    // The items as a vector (a copy).
    [[nodiscard]] std::vector<T> toVector() const { return std::vector<T>(begin(), end()); }
    [[nodiscard]] size_type capacity() const noexcept { return m_keeper ? m_size : m_owned.capacity(); }

    friend bool operator==(const LabelArray& left, const LabelArray& right) noexcept
    {
        return std::equal(left.begin(), left.end(), right.begin(), right.end());
    }
    friend bool operator==(const LabelArray& left, const std::vector<T>& right) noexcept
    {
        return std::equal(left.begin(), left.end(), right.begin(), right.end());
    }

    // ---- writing: the items of a kept block are first copied into a vector of their own
    [[nodiscard]] T* data() { return writable(); }
    [[nodiscard]] T& operator[](size_type index) { return writable()[index]; }
    [[nodiscard]] T& at(size_type index)
    {
        if (index >= m_size)
            throw std::out_of_range("LabelArray::at: the index is past the last item");
        return writable()[index];
    }
    [[nodiscard]] T& front() { return writable()[0]; }
    [[nodiscard]] T& back() { return writable()[m_size - 1]; }
    [[nodiscard]] iterator begin() { return writable(); }
    [[nodiscard]] iterator end() { return writable() + m_size; }
    [[nodiscard]] reverse_iterator rbegin() { return reverse_iterator(end()); }
    [[nodiscard]] reverse_iterator rend() { return reverse_iterator(begin()); }

    // the names of std::vector
    // NOLINTBEGIN(readability-identifier-naming)
    void push_back(const T& value)
    {
        owned().push_back(value);
        sync();
    }
    template <typename... Arguments>
    T& emplace_back(Arguments&&... arguments)
    {
        T& item = owned().emplace_back(std::forward<Arguments>(arguments)...);
        sync();
        return item;
    }
    void pop_back()
    {
        owned().pop_back();
        sync();
    }
    void shrink_to_fit()
    {
        owned().shrink_to_fit();
        sync();
    }
    // NOLINTEND(readability-identifier-naming)
    void reserve(size_type count)
    {
        owned().reserve(count);
        sync();
    }
    void resize(size_type count)
    {
        owned().resize(count);
        sync();
    }
    void resize(size_type count, const T& value)
    {
        owned().resize(count, value);
        sync();
    }
    void assign(size_type count, const T& value)
    {
        m_keeper.reset();
        m_owned.assign(count, value);
        sync();
    }
    template <std::input_iterator Iterator>
    void assign(Iterator first, Iterator last)
    {
        std::vector<T> items(first, last); // the items may be this array's own
        *this = std::move(items);
    }
    void assign(std::initializer_list<T> items) { *this = std::vector<T>(items); }
    void clear() noexcept
    {
        m_keeper.reset();
        m_owned.clear();
        sync();
    }
    iterator insert(const_iterator position, const T& value)
    {
        const auto offset = position - m_items;
        std::vector<T>& items = owned();
        const auto at = items.insert(items.begin() + offset, value);
        sync();
        return m_owned.data() + (at - items.begin());
    }
    template <std::input_iterator Iterator>
    iterator insert(const_iterator position, Iterator first, Iterator last)
    {
        const auto offset = position - m_items;
        std::vector<T> added(first, last); // the items may be this array's own
        std::vector<T>& items = owned();
        const auto at = items.insert(items.begin() + offset, added.begin(), added.end());
        sync();
        return m_owned.data() + (at - items.begin());
    }
    iterator erase(const_iterator first, const_iterator last)
    {
        const auto offset = first - m_items;
        const auto count = last - first;
        std::vector<T>& items = owned();
        const auto at = items.erase(items.begin() + offset, items.begin() + offset + count);
        sync();
        return m_owned.data() + (at - items.begin());
    }
    iterator erase(const_iterator position) { return erase(position, position + 1); }
    void swap(LabelArray& other) noexcept
    {
        LabelArray moved(std::move(other));
        other = std::move(*this);
        *this = std::move(moved);
    }
    // The items as a vector, this array left empty (no copy of an array of its own).
    [[nodiscard]] std::vector<T> release()
    {
        std::vector<T> items = m_keeper ? toVector() : std::move(m_owned);
        clear();
        return items;
    }

private:
    // The vector of its own, the items of a kept block first copied into it.
    std::vector<T>& owned()
    {
        if (m_keeper)
        {
            m_owned.assign(m_items, m_items + m_size);
            m_keeper.reset();
        }
        return m_owned;
    }
    T* writable()
    {
        if (m_keeper)
        {
            owned();
            sync();
        }
        return m_owned.data();
    }
    void sync() noexcept
    {
        m_items = m_owned.data();
        m_size = m_owned.size();
    }

    std::vector<T> m_owned;
    const T* m_items = nullptr; // the items: those of m_owned, or of the kept block
    size_type m_size = 0;
    std::shared_ptr<const void> m_keeper; // the owner of the kept block; empty for an array of its own
};

} // namespace oor::results
