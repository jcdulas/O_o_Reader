#pragma once

#include "oor/results/core/ResultFile.hpp"
#include "oor/results/core/ResultTable.hpp"
#include "oor/results/core/ResultTables.hpp"

#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace oor::testing
{

// A result file with nothing in it: the attachment and the frame code only need its tables, which are made by hand.
class MemoryResultFile final : public results::ResultFile
{
public:
    [[nodiscard]] std::string_view format() const noexcept override { return "memory"; }
    [[nodiscard]] const results::ResultFileMetadata& metadata() const noexcept override { return m_metadata; }
    [[nodiscard]] const std::vector<base::Diagnostic>& diagnostics() const noexcept override { return m_diagnostics; }
    [[nodiscard]] results::ResultTables tables(const results::ResultFilter&) const override { return {}; }

private:
    results::ResultFileMetadata m_metadata;
    std::vector<base::Diagnostic> m_diagnostics;
};

// What a table made by hand holds: the rows (IDs of grid points or elements, with the grid point and layer of each
// place of an element table) and the values of every step, row by row, component by component.
struct TableSpec
{
    results::ResultKind kind = results::ResultKind::Displacements;
    std::string elementName;
    std::string layout; // "centre" or "corner" for the element types that have both
    std::vector<std::int32_t> ids;
    std::vector<std::int32_t> nodeIds;
    std::vector<std::int32_t> layers;
    std::vector<std::string> components;
    std::vector<double> values; // steps * rows * components
    std::size_t steps = 1;
    results::ValueType type = results::ValueType::Float64;
    std::int32_t subcase = 1;
    std::vector<double> stepValues;
};

// The table of a spec, decoded already (it holds its values). Complex values are given as pairs (real, imaginary) in
// `values`.
inline std::shared_ptr<results::ResultTable> makeTable(const TableSpec& spec)
{
    auto data = std::make_shared<results::ResultData>();
    data->type = spec.type;
    data->stepCount = spec.steps;
    data->rowCount = spec.ids.size();
    data->componentCount = spec.components.size();
    data->components = spec.components;
    data->ids = spec.ids;
    data->nodeIds = spec.nodeIds;
    data->layers = spec.layers;
    data->steps = spec.stepValues.empty() ? std::vector<double>(spec.steps, std::nan("")) : spec.stepValues;
    if (results::resultKindInfo(spec.kind).nodal)
        data->pointTypes.assign(spec.ids.size(), 1);
    data->allocate();
    const bool single = spec.type == results::ValueType::Float32 || spec.type == results::ValueType::Complex64;
    const std::size_t count = data->valueCount() * (results::isComplexType(spec.type) ? 2 : 1);
    if (single)
    {
        float* target = reinterpret_cast<float*>(data->storage.get());
        for (std::size_t index = 0; index < count; ++index)
            target[index] = static_cast<float>(spec.values[index]);
    }
    else
    {
        std::memcpy(data->storage.get(), spec.values.data(), count * sizeof(double));
    }
    results::ResultTableMeta meta;
    meta.kind = spec.kind;
    meta.elementName = spec.elementName;
    meta.layout = spec.layout;
    meta.subcase = spec.subcase;
    meta.valueType = spec.type;
    meta.valueKind = results::isComplexType(spec.type) ? results::ValueKind::Complex : results::ValueKind::Real;
    meta.stepKind = results::StepKind::None;
    meta.storedFrame = results::resultKindInfo(spec.kind).nodal ? results::StoredFrame::NodeOutputSystem
                                                                : results::StoredFrame::Unknown;
    meta.status = results::ResultStatus::Validated;
    return std::make_shared<results::ResultTable>(std::move(meta), std::shared_ptr<const results::ResultData>(data));
}

// The tables of a result file, in their containers.
inline std::shared_ptr<results::ResultTables> makeTables(
    const std::vector<std::shared_ptr<results::ResultTable>>& tables)
{
    auto result = std::make_shared<results::ResultTables>();
    for (const std::shared_ptr<results::ResultTable>& table : tables)
    {
        const results::ResultTableMeta& meta = table->meta();
        if (results::resultKindInfo(meta.kind).family)
            result->mutableElementContainer(meta.kind, meta.elementName).add(table);
        else
            result->mutableContainer(meta.kind).add(table);
    }
    return result;
}

} // namespace oor::testing
