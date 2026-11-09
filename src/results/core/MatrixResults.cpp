#include "oor/results/core/MatrixResults.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/results/core/NamedResultBuilder.hpp"

#include <cstring>

namespace oor::results
{

namespace
{

template <typename T>
std::span<const std::byte> bytesOf(const std::vector<T>& values)
{
    return std::as_bytes(std::span<const T>(values));
}

const ResultArray& need(const NamedResultData& data, std::string_view name)
{
    const ResultArray* array = data.find(name);
    if (array == nullptr)
        throw base::UsageError("the matrix has no array '" + std::string(name) + "'");
    return *array;
}

} // namespace

std::shared_ptr<NamedResult> matrixResultOf(const model::CooMatrix& matrix, std::int32_t subcase)
{
    const std::int64_t shape[2] = {matrix.rowCount, matrix.columnCount};
    std::vector<NamedArrayInput> arrays;
    arrays.push_back({"shape", ArrayType::Int64, {2}, std::as_bytes(std::span<const std::int64_t>(shape))});
    const std::size_t count = matrix.termCount();
    arrays.push_back({"row", ArrayType::Int32, {count}, bytesOf(matrix.row)});
    arrays.push_back({"column", ArrayType::Int32, {count}, bytesOf(matrix.column)});
    std::vector<double> interleaved;
    if (matrix.complex)
    {
        interleaved.resize(2 * count);
        for (std::size_t index = 0; index < count; ++index)
        {
            interleaved[2 * index] = matrix.real[index];
            interleaved[2 * index + 1] = matrix.imag[index];
        }
        arrays.push_back({"value", ArrayType::Complex128, {count}, bytesOf(interleaved)});
    }
    else
    {
        arrays.push_back({"value", ArrayType::Float64, {count}, bytesOf(matrix.real)});
    }
    if (!matrix.rowId.empty() || !matrix.columnId.empty())
    {
        arrays.push_back({"rowIds", ArrayType::Int32, {matrix.rowId.size()}, bytesOf(matrix.rowId)});
        arrays.push_back(
            {"rowComponents", ArrayType::Int32, {matrix.rowComponent.size()}, bytesOf(matrix.rowComponent)});
        arrays.push_back({"columnIds", ArrayType::Int32, {matrix.columnId.size()}, bytesOf(matrix.columnId)});
        arrays.push_back(
            {"columnComponents", ArrayType::Int32, {matrix.columnComponent.size()}, bytesOf(matrix.columnComponent)});
        if (!matrix.rowHarmonic.empty() || !matrix.columnHarmonic.empty())
        {
            // DMIAX: the harmonic number is part of the label of a degree of freedom
            arrays.push_back(
                {"rowHarmonics", ArrayType::Int32, {matrix.rowHarmonic.size()}, bytesOf(matrix.rowHarmonic)});
            arrays.push_back(
                {"columnHarmonics", ArrayType::Int32, {matrix.columnHarmonic.size()}, bytesOf(matrix.columnHarmonic)});
        }
    }
    NamedResultOrigin origin;
    origin.subcase = subcase;
    origin.name = matrix.name;
    origin.attributes.emplace_back("origin", "model");
    origin.attributes.emplace_back("card", matrix.card);
    origin.attributes.emplace_back("form", std::string(model::matrixFormName(matrix.form, matrix.card)));
    origin.attributes.emplace_back("formCode", std::to_string(matrix.form));
    origin.attributes.emplace_back("typeCode", std::to_string(matrix.inputType));
    origin.attributes.emplace_back("outputType", std::to_string(matrix.outputType));
    origin.attributes.emplace_back("polar", matrix.polar ? "1" : "0");
    std::shared_ptr<NamedResult> result = makeNamedResult("matrix", origin, arrays, {});
    return result;
}

model::LabelledTerms matrixTermsOf(const NamedResult& matrix, const NodalMatrixStorage& storage)
{
    if (matrix.meta().kind != "matrix")
        throw base::UsageError("a DMIG matrix is stored from a matrix result, not from a " + matrix.meta().kind);
    const std::shared_ptr<const NamedResultData> data = matrix.data();
    const ResultArray& shape = need(*data, "shape");
    const auto extents = shape.values<std::int64_t>();
    const ResultArray& row = need(*data, "row");
    const ResultArray& column = need(*data, "column");
    const ResultArray& value = need(*data, "value");
    const ResultArray* rowIds = data->find("rowIds");
    const ResultArray* rowComponents = data->find("rowComponents");
    const ResultArray* columnIds = data->find("columnIds");
    const ResultArray* columnComponents = data->find("columnComponents");
    if (rowIds == nullptr || rowComponents == nullptr || columnIds == nullptr || columnComponents == nullptr)
        throw base::UsageError(
            "the matrix " + matrix.meta().name +
            " has no labels for its rows and columns, and a DMIG matrix is indexed by point and component: make it "
            "with rowIds, rowComponents, columnIds and columnComponents");
    model::LabelledTerms terms;
    terms.complex = value.type == ArrayType::Complex64 || value.type == ArrayType::Complex128;
    // the form: asked, else what the matrix says, else by its shape
    std::int32_t form = 0;
    if (storage.form)
    {
        form = *storage.form;
    }
    else if (const std::string* said = matrix.meta().attribute("form"))
    {
        form = *said == "symmetric" ? 6 : (*said == "square" ? 1 : (*said == "rectangular" ? 9 : 0));
    }
    if (form == 0)
        form = extents[0] == extents[1] ? 1 : 9;
    if ((form == 1 || form == 6) && extents[0] != extents[1])
        throw base::UsageError("the matrix " + matrix.meta().name + " is " + std::to_string(extents[0]) + " by " +
                               std::to_string(extents[1]) + ": it cannot be stored as a square or symmetric DMIG");
    terms.form = form;
    terms.inputType = storage.inputType.value_or(terms.complex ? 4 : 2);
    terms.outputType = storage.outputType;
    const std::size_t count = row.valueCount();
    const auto rowIndex = row.values<std::int32_t>();
    const auto columnIndex = column.values<std::int32_t>();
    const auto rowIdValues = rowIds->values<std::int32_t>();
    const auto rowComponentValues = rowComponents->values<std::int32_t>();
    const auto columnIdValues = columnIds->values<std::int32_t>();
    const auto columnComponentValues = columnComponents->values<std::int32_t>();
    const bool numbered = form == 9;
    if (numbered)
        terms.columnCount = static_cast<std::int32_t>(extents[1]);
    for (std::size_t index = 0; index < count; ++index)
    {
        terms.rowId.push_back(rowIdValues[static_cast<std::size_t>(rowIndex[index])]);
        terms.rowComponent.push_back(rowComponentValues[static_cast<std::size_t>(rowIndex[index])]);
        if (numbered)
        {
            terms.columnId.push_back(columnIndex[index] + 1);
            terms.columnComponent.push_back(0);
        }
        else
        {
            terms.columnId.push_back(columnIdValues[static_cast<std::size_t>(columnIndex[index])]);
            terms.columnComponent.push_back(columnComponentValues[static_cast<std::size_t>(columnIndex[index])]);
        }
    }
    if (terms.complex)
    {
        for (std::size_t index = 0; index < count; ++index)
        {
            if (value.type == ArrayType::Complex128)
            {
                const auto numbers = value.values<double>();
                terms.real.push_back(numbers[2 * index]);
                terms.imag.push_back(numbers[2 * index + 1]);
            }
            else
            {
                const auto numbers = value.values<float>();
                terms.real.push_back(static_cast<double>(numbers[2 * index]));
                terms.imag.push_back(static_cast<double>(numbers[2 * index + 1]));
            }
        }
    }
    else if (value.type == ArrayType::Float64)
    {
        const auto numbers = value.values<double>();
        terms.real.assign(numbers.begin(), numbers.end());
    }
    else
    {
        const auto numbers = value.values<float>();
        for (const float number : numbers)
            terms.real.push_back(static_cast<double>(number));
    }
    return terms;
}

} // namespace oor::results
