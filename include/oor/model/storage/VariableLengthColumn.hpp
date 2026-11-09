#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace oor::model
{

// A field that holds a list of values per card (node lists, plies, THRU lists) for all the cards of a type, in CSR
// form: one array of values and, per row, where its values begin and how many there are. Exposed to Python as
// (offsets, values) after canonicalise().
//
// Same copy-on-write rule as Column: the three buffers are shared by views and copies and cloned on the first edit.
// Replacing the values of a row with a list of another length appends the new values at the end and leaves a hole;
// canonicalise() (called before a CSR view and by compaction) squeezes the holes out.
template <typename T>
class VariableLengthColumn
{
public:
    [[nodiscard]] std::size_t size() const noexcept { return m_begin ? m_begin->size() : 0; }

    void push(std::span<const T> values)
    {
        ensure();
        beginEdit();
        m_begin->push_back(m_values->size());
        m_length->push_back(static_cast<std::uint32_t>(values.size()));
        m_values->insert(m_values->end(), values.begin(), values.end());
    }

    [[nodiscard]] std::span<const T> get(std::size_t row) const noexcept
    {
        if (!m_begin)
            return {};
        return std::span<const T>(m_values->data() + (*m_begin)[row], (*m_length)[row]);
    }

    void set(std::size_t row, std::span<const T> values)
    {
        ensure();
        beginEdit();
        if (values.size() == (*m_length)[row])
        {
            std::copy(values.begin(), values.end(), m_values->begin() + static_cast<std::ptrdiff_t>((*m_begin)[row]));
            return;
        }
        (*m_begin)[row] = m_values->size();
        (*m_length)[row] = static_cast<std::uint32_t>(values.size());
        m_values->insert(m_values->end(), values.begin(), values.end());
    }

    // Squeezes out the holes and puts the rows in order: afterwards offsets() and values() are a canonical CSR.
    void canonicalise()
    {
        if (!m_begin)
            return;
        std::size_t hole = m_values->size();
        std::size_t used = 0;
        for (std::uint32_t length : *m_length)
            used += length;
        if (used == hole && isInOrder())
            return;
        auto values = std::make_shared<std::vector<T>>();
        values->reserve(used);
        auto begin = std::make_shared<std::vector<std::uint64_t>>();
        begin->reserve(m_begin->size());
        for (std::size_t row = 0; row < m_begin->size(); ++row)
        {
            begin->push_back(values->size());
            const auto first = m_values->begin() + static_cast<std::ptrdiff_t>((*m_begin)[row]);
            values->insert(values->end(), first, first + (*m_length)[row]);
        }
        m_begin = std::move(begin);
        m_values = std::move(values);
    }

    // CSR offsets (size + 1 entries) of a canonical column; call canonicalise() first.
    [[nodiscard]] std::vector<std::uint64_t> offsets() const
    {
        std::vector<std::uint64_t> result;
        if (!m_begin)
        {
            result.push_back(0);
            return result;
        }
        result.reserve(m_begin->size() + 1);
        result.insert(result.end(), m_begin->begin(), m_begin->end());
        result.push_back(m_values->size());
        return result;
    }
    [[nodiscard]] std::span<const T> values() const noexcept
    {
        return m_values ? std::span<const T>(*m_values) : std::span<const T>{};
    }
    [[nodiscard]] std::shared_ptr<const std::vector<T>> shareValues() const { return m_values; }

    void keepRows(const std::vector<std::uint32_t>& rows)
    {
        if (!m_begin)
            return;
        auto values = std::make_shared<std::vector<T>>();
        auto begin = std::make_shared<std::vector<std::uint64_t>>();
        auto length = std::make_shared<std::vector<std::uint32_t>>();
        for (std::uint32_t row : rows)
        {
            const auto first = m_values->begin() + static_cast<std::ptrdiff_t>((*m_begin)[row]);
            begin->push_back(values->size());
            length->push_back((*m_length)[row]);
            values->insert(values->end(), first, first + (*m_length)[row]);
        }
        m_begin = std::move(begin);
        m_length = std::move(length);
        m_values = std::move(values);
    }

    void append(const VariableLengthColumn& other)
    {
        for (std::size_t row = 0; row < other.size(); ++row)
            push(other.get(row));
    }

private:
    void ensure()
    {
        if (!m_begin)
        {
            m_begin = std::make_shared<std::vector<std::uint64_t>>();
            m_length = std::make_shared<std::vector<std::uint32_t>>();
            m_values = std::make_shared<std::vector<T>>();
        }
    }

    void beginEdit()
    {
        if (m_begin.use_count() > 1)
            m_begin = std::make_shared<std::vector<std::uint64_t>>(*m_begin);
        if (m_length.use_count() > 1)
            m_length = std::make_shared<std::vector<std::uint32_t>>(*m_length);
        if (m_values.use_count() > 1)
            m_values = std::make_shared<std::vector<T>>(*m_values);
    }

    [[nodiscard]] bool isInOrder() const
    {
        std::uint64_t expected = 0;
        for (std::size_t row = 0; row < m_begin->size(); ++row)
        {
            if ((*m_begin)[row] != expected)
                return false;
            expected += (*m_length)[row];
        }
        return true;
    }

    std::shared_ptr<std::vector<std::uint64_t>> m_begin;
    std::shared_ptr<std::vector<std::uint32_t>> m_length;
    std::shared_ptr<std::vector<T>> m_values;
};

} // namespace oor::model
