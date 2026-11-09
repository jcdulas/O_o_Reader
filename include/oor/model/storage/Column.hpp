#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <type_traits>
#include <vector>

namespace oor::model
{

namespace detail
{

template <typename T>
struct IsRealArray : std::false_type
{
};
template <typename R, std::size_t N>
struct IsRealArray<std::array<R, N>> : std::is_floating_point<R>
{
};

// True when two values of a column are the same value to the bit: a real -0.0 is not 0.0 (a companion package: the sign of
// a zero a model is built with survives the column and a lossless write), and a NaN is itself.
template <typename T>
[[nodiscard]] bool sameBits(const T& left, const T& right) noexcept
{
    if constexpr (std::is_same_v<T, double>)
        return std::bit_cast<std::uint64_t>(left) == std::bit_cast<std::uint64_t>(right);
    else if constexpr (std::is_same_v<T, float>)
        return std::bit_cast<std::uint32_t>(left) == std::bit_cast<std::uint32_t>(right);
    else if constexpr (IsRealArray<T>::value)
    {
        for (std::size_t index = 0; index < left.size(); ++index)
            if (!sameBits(left[index], right[index]))
                return false;
        return true;
    }
    else
        return left == right;
}

} // namespace detail

// The rows a batch of cards adds, announced on the thread that adds them (a batch add of n cards from arrays): a column
// that has to grow on that thread meanwhile grows at once by the rows announced, instead of doubling at every power of
// two (on Windows each new block over about a megabyte is fresh pages of the system: a batch of 400 000 GRID with
// positions paid 91 ns a card against 62 for 30 000).
class ColumnGrowth
{
public:
    class Batch
    {
    public:
        explicit Batch(std::size_t rows) noexcept : m_previous(s_announced) { s_announced = rows; }
        ~Batch() { s_announced = m_previous; }
        Batch(const Batch&) = delete;
        Batch& operator=(const Batch&) = delete;

    private:
        std::size_t m_previous;
    };

    [[nodiscard]] static std::size_t announced() noexcept { return s_announced; }

private:
    static inline thread_local std::size_t s_announced = 0;
};

// One field of one card type, for all the cards of that type: a contiguous array (structure of arrays), so that
// geometry kernels and NumPy views work on raw memory.
//
// Three properties matter:
//
// * Default columns are not allocated. A column whose every value is the default holds no buffer, only the default
//   and the row count; the buffer appears the first time a value differs, or when a view is asked for. Fields that
//   are almost never set (corner thicknesses, offsets) therefore cost nothing.
// * Copy on write. The buffer is reference counted. A view handed to Python, or a copy of the model, shares it; an
//   edit that finds the buffer shared clones it first. A view is thus a snapshot that never dangles and never
//   changes under the caller.
// * No hidden cost for reads: get() is a branch on "is there a buffer".
//
// Not thread-safe for writing (a Model needs exclusive access to be modified); concurrent reads are fine.
template <typename T>
class Column
{
public:
    using Buffer = std::vector<T>;

    explicit Column(T defaultValue = T{}) : m_default(defaultValue) {}

    [[nodiscard]] std::size_t size() const noexcept { return m_size; }
    [[nodiscard]] const T& defaultValue() const noexcept { return m_default; }
    [[nodiscard]] bool isMaterialised() const noexcept { return m_buffer != nullptr; }

    [[nodiscard]] T get(std::size_t row) const noexcept { return m_buffer ? (*m_buffer)[row] : m_default; }

    void push(const T& value)
    {
        if (!m_buffer)
        {
            if (detail::sameBits(value, m_default))
            {
                ++m_size;
                return;
            }
            materialise();
        }
        Buffer& buffer = writable();
        if (buffer.size() == buffer.capacity() && ColumnGrowth::announced() > 0)
            buffer.reserve(std::max(2 * buffer.capacity(), buffer.size() + ColumnGrowth::announced()));
        buffer.push_back(value);
        ++m_size;
    }

    void set(std::size_t row, const T& value)
    {
        if (!m_buffer)
        {
            if (detail::sameBits(value, m_default))
                return;
            materialise();
        }
        else if (detail::sameBits((*m_buffer)[row], value))
        {
            return; // writing the value that is there changes nothing, and must not clone a shared buffer
        }
        writable()[row] = value;
    }

    // Grows or shrinks to `rowCount`; new rows hold the default.
    void resize(std::size_t rowCount)
    {
        if (m_buffer)
            writable().resize(rowCount, m_default);
        m_size = rowCount;
    }

    // All the values, contiguous. Allocates the buffer if the column is still all default.
    [[nodiscard]] std::span<const T> view()
    {
        materialise();
        return std::span<const T>(*m_buffer);
    }

    // The shared buffer, for a view that must outlive further edits (Python arrays). Allocates it if needed.
    [[nodiscard]] std::shared_ptr<const Buffer> share()
    {
        materialise();
        return m_buffer;
    }

    // Keeps only the rows listed (in that order, which must be increasing): the compaction of a table.
    void keepRows(const std::vector<std::uint32_t>& rows)
    {
        if (m_buffer)
        {
            auto kept = std::make_shared<Buffer>();
            kept->reserve(rows.size());
            for (std::uint32_t row : rows)
                kept->push_back((*m_buffer)[row]);
            m_buffer = std::move(kept);
        }
        m_size = rows.size();
    }

    // Appends all the rows of `other` (used to join the partial tables read in parallel).
    void append(Column& other)
    {
        if (!other.m_buffer)
        {
            if (!m_buffer)
            {
                m_size += other.m_size;
                return;
            }
            writable().resize(m_size + other.m_size, other.m_default);
            m_size += other.m_size;
            return;
        }
        materialise();
        Buffer& target = writable();
        target.insert(target.end(), other.m_buffer->begin(), other.m_buffer->end());
        m_size += other.m_size;
    }

    // True when the column has the same values as `other` (an unallocated column equals an allocated all-default one).
    [[nodiscard]] bool sameValues(const Column& other) const
    {
        if (m_size != other.m_size)
            return false;
        for (std::size_t row = 0; row < m_size; ++row)
        {
            if (!(get(row) == other.get(row)))
                return false;
        }
        return true;
    }

private:
    void materialise()
    {
        if (!m_buffer)
            m_buffer = std::make_shared<Buffer>(m_size, m_default);
    }

    // The buffer, cloned first when somebody else (a view, a model copy) shares it.
    Buffer& writable()
    {
        if (m_buffer.use_count() > 1)
            m_buffer = std::make_shared<Buffer>(*m_buffer);
        return *m_buffer;
    }

    std::shared_ptr<Buffer> m_buffer;
    std::size_t m_size = 0;
    T m_default;
};

} // namespace oor::model
