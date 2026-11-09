#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace oor::query
{

// A block of memory that columns, NumPy views and Arrow consumers share. It is read-only: a column that changes gets a
// new buffer (the old one stays valid for whoever holds it), so a view taken at one time never changes.
//
// The buffer does not own its bytes itself: it holds a shared pointer to whatever does (a `std::vector` made by an
// operator, or the buffer of a model column that the table borrows without copy), and the address of the bytes. Copying
// a ColumnBuffer is one reference count.
class ColumnBuffer
{
public:
    ColumnBuffer() = default;

    // A buffer that owns the values of a vector.
    template <typename T>
    [[nodiscard]] static ColumnBuffer adopt(std::vector<T>&& values)
    {
        auto holder = std::make_shared<const std::vector<T>>(std::move(values));
        const T* first = holder->data();
        const std::size_t bytes = holder->size() * sizeof(T);
        return ColumnBuffer(std::shared_ptr<const void>(holder, holder.get()), first, bytes);
    }

    // A buffer on a vector somebody else shares (the buffer of a model column).
    template <typename T>
    [[nodiscard]] static ColumnBuffer share(std::shared_ptr<const std::vector<T>> values)
    {
        if (!values)
            return ColumnBuffer();
        const T* first = values->data();
        const std::size_t bytes = values->size() * sizeof(T);
        return ColumnBuffer(std::shared_ptr<const void>(values, values.get()), first, bytes);
    }

    // A buffer on bytes that `owner` keeps alive.
    [[nodiscard]] static ColumnBuffer view(std::shared_ptr<const void> owner, const void* data, std::size_t bytes)
    {
        return ColumnBuffer(std::move(owner), data, bytes);
    }

    [[nodiscard]] const void* data() const noexcept { return m_data; }
    [[nodiscard]] std::size_t bytes() const noexcept { return m_bytes; }
    [[nodiscard]] bool empty() const noexcept { return m_bytes == 0; }
    [[nodiscard]] bool isPresent() const noexcept { return m_owner != nullptr; }
    [[nodiscard]] const std::shared_ptr<const void>& owner() const noexcept { return m_owner; }

    template <typename T>
    [[nodiscard]] std::span<const T> as() const noexcept
    {
        return std::span<const T>(static_cast<const T*>(m_data), m_bytes / sizeof(T));
    }

private:
    ColumnBuffer(std::shared_ptr<const void> owner, const void* data, std::size_t bytes)
        : m_owner(std::move(owner)), m_data(data), m_bytes(bytes)
    {
    }

    std::shared_ptr<const void> m_owner;
    const void* m_data = nullptr;
    std::size_t m_bytes = 0;
};

} // namespace oor::query
