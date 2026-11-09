#pragma once

#include "oor/results/core/ResultData.hpp"
#include "oor/results/core/ResultTable.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace oor::results
{

// The type of the numbers of one array of a named result.
enum class ArrayType : std::uint8_t
{
    Int32,
    Int64,
    Float32,
    Float64,
    Complex64,
    Complex128
};

[[nodiscard]] std::string_view arrayTypeName(ArrayType type) noexcept;
[[nodiscard]] std::size_t arrayTypeSize(ArrayType type) noexcept;

// One named array of a named result: a shape (empty for a number alone) and the values in C order, 8-byte aligned.
struct ResultArray
{
    std::string name;
    std::string description;
    ArrayType type = ArrayType::Float32;
    std::vector<std::size_t> shape;
    std::shared_ptr<double[]> storage;

    [[nodiscard]] std::size_t valueCount() const noexcept;
    [[nodiscard]] std::size_t byteCount() const noexcept { return valueCount() * arrayTypeSize(type); }
    [[nodiscard]] const std::byte* bytes() const noexcept { return reinterpret_cast<const std::byte*>(storage.get()); }
    [[nodiscard]] std::byte* mutableBytes() noexcept { return reinterpret_cast<std::byte*>(storage.get()); }
    template <typename T>
    [[nodiscard]] std::span<const T> values() const noexcept
    {
        return std::span<const T>(reinterpret_cast<const T*>(storage.get()), byteCount() / sizeof(T));
    }
    template <typename T>
    [[nodiscard]] std::span<T> mutableValues() noexcept
    {
        return std::span<T>(reinterpret_cast<T*>(storage.get()), byteCount() / sizeof(T));
    }
    // Allocates the storage for the shape and type (not initialised).
    void allocate();
};

// What a named result holds once decoded: a few arrays and text labels, each by name. A named result is a result that
// is not a block of (steps, rows, components): the grid point weight (a 6 x 6 matrix, a centre of gravity, ...), a
// matrix (its non-zero terms), an optimisation table (a column of numbers per name, one row per design cycle, design
// variable or response).
struct NamedResultData
{
    std::vector<ResultArray> arrays;
    std::vector<ResultText> texts; // text labels, one per row of the arrays that have rows
    // The array of that name; null when there is none.
    [[nodiscard]] const ResultArray* find(std::string_view name) const noexcept;
    [[nodiscard]] const ResultText* findText(std::string_view name) const noexcept;
};

// What a named result is, without decoding it.
struct NamedResultMeta
{
    std::string kind; // "nodePointWeight", "matrix", "designVariables", "designHistory", ...
    std::string name; // the name it was given (the name of a matrix, ...)
    std::string tableName;
    std::int32_t subcase = 0;
    std::int32_t analysisCode = 0;
    std::int32_t superelement = -1;
    std::int32_t designCycle = -1;
    ResultStatus status = ResultStatus::NotValidated;
    std::string title;
    std::string subtitle;
    std::string label;
    int wordSize = 4;
    // Free-form facts about it, as text (the form of a matrix, the convergence of a design cycle ...).
    std::vector<std::pair<std::string, std::string>> attributes;
    [[nodiscard]] const std::string* attribute(std::string_view key) const noexcept;
};

// Where the data of a named result come from and how to read them.
class NamedResultSource
{
public:
    virtual ~NamedResultSource() = default;
    [[nodiscard]] virtual std::shared_ptr<const NamedResultData> load(const DecodeContext& context) const = 0;
};

// A source made from a function.
class FunctionNamedSource final : public NamedResultSource
{
public:
    using Function = std::function<std::shared_ptr<const NamedResultData>(const DecodeContext&)>;
    explicit FunctionNamedSource(Function function) : m_function(std::move(function)) {}
    [[nodiscard]] std::shared_ptr<const NamedResultData> load(const DecodeContext& context) const override
    {
        return m_function(context);
    }

private:
    Function m_function;
};

// A result that is a few named arrays. Decoding is lazy and cached, as for ResultTable: the description comes from the
// index, the first use of `data()` reads the arrays on the threads of the context and keeps them.
class NamedResult
{
public:
    NamedResult(NamedResultMeta meta, std::shared_ptr<const NamedResultSource> source);
    NamedResult(NamedResultMeta meta, std::shared_ptr<const NamedResultData> data);

    NamedResult(const NamedResult&) = delete;
    NamedResult& operator=(const NamedResult&) = delete;

    [[nodiscard]] const NamedResultMeta& meta() const noexcept { return m_meta; }
    [[nodiscard]] bool isDecoded() const;
    [[nodiscard]] std::shared_ptr<const NamedResultData> data(const DecodeContext& context = {}) const;

private:
    NamedResultMeta m_meta;
    std::shared_ptr<const NamedResultSource> m_source;
    mutable std::mutex m_mutex;
    mutable std::shared_ptr<const NamedResultData> m_data;
};

} // namespace oor::results
