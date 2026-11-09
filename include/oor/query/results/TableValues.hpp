#pragma once

#include "oor/results/core/ResultData.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace oor::query
{

// Read access to the block of values of a decoded result table, whatever its precision (single or double) and whether
// it is complex: `real(step, row, component)` and `imag(...)` as doubles. The block is (steps, rows, components) in C
// order.
class TableValues
{
public:
    explicit TableValues(const results::ResultData& data) noexcept
        : m_data(data),
          m_complex(results::isComplexType(data.type)),
          m_single(data.type == results::ValueType::Float32 || data.type == results::ValueType::Complex64)
    {
    }

    [[nodiscard]] const results::ResultData& data() const noexcept { return m_data; }
    [[nodiscard]] bool isComplex() const noexcept { return m_complex; }
    [[nodiscard]] bool isSingle() const noexcept { return m_single; }
    [[nodiscard]] std::size_t steps() const noexcept { return m_data.stepCount; }
    [[nodiscard]] std::size_t rows() const noexcept { return m_data.rowCount; }
    [[nodiscard]] std::size_t components() const noexcept { return m_data.componentCount; }

    [[nodiscard]] double real(std::size_t step, std::size_t row, std::size_t component) const noexcept
    {
        return at(index(step, row, component) * (m_complex ? 2 : 1));
    }
    [[nodiscard]] double imag(std::size_t step, std::size_t row, std::size_t component) const noexcept
    {
        return m_complex ? at(index(step, row, component) * 2 + 1) : 0.0;
    }
    [[nodiscard]] double magnitude(std::size_t step, std::size_t row, std::size_t component) const noexcept
    {
        return m_complex ? std::hypot(real(step, row, component), imag(step, row, component))
                         : std::fabs(real(step, row, component));
    }
    // The position of a component by name, or -1.
    [[nodiscard]] int component(const std::string& name) const noexcept
    {
        const auto found = std::find(m_data.components.begin(), m_data.components.end(), name);
        return found == m_data.components.end() ? -1 : static_cast<int>(found - m_data.components.begin());
    }

private:
    [[nodiscard]] std::size_t index(std::size_t step, std::size_t row, std::size_t component) const noexcept
    {
        return (step * m_data.rowCount + row) * m_data.componentCount + component;
    }
    [[nodiscard]] double at(std::size_t position) const noexcept
    {
        return m_single ? static_cast<double>(m_data.values<float>()[position]) : m_data.values<double>()[position];
    }

    const results::ResultData& m_data;
    bool m_complex;
    bool m_single;
};

} // namespace oor::query
