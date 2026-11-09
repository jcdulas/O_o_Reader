#include "oor/query/results/ResultTableSource.hpp"

#include "oor/analysis/frames/StoredFrames.hpp"
#include "oor/model/core/CardRegistry.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/operators/ColumnOps.hpp"
#include "oor/query/results/NamedArrays.hpp"
#include "oor/results/core/NamedResultKinds.hpp"
#include "oor/results/core/ResultKind.hpp"

#include <algorithm>
#include <complex>
#include <map>
#include <set>

namespace oor::query
{

namespace
{

std::string stepNoun(results::StepKind kind)
{
    switch (kind)
    {
    case results::StepKind::Time:
        return "time";
    case results::StepKind::Frequency:
        return "freq";
    case results::StepKind::Mode:
        return "mode";
    case results::StepKind::LoadFactor:
        return "loadStep";
    default:
        return {};
    }
}

std::string joinNames(const std::vector<std::string>& names)
{
    std::string text;
    for (const std::string& name : names)
        text += (text.empty() ? "" : ", ") + name;
    return text;
}

// The column with a null where `valid` is 0 (a row that is not linked to a card of the model).
QueryColumn nullable(QueryColumn column, const std::vector<std::uint8_t>& valid)
{
    std::int64_t nullCount = 0;
    ColumnBuffer bits = validityFromFlags(valid, nullCount);
    return nullCount == 0 ? column : column.withValidity(std::move(bits), nullCount);
}

std::vector<std::uint8_t> positive(const std::vector<std::int32_t>& ids)
{
    std::vector<std::uint8_t> valid(ids.size());
    for (std::size_t row = 0; row < ids.size(); ++row)
        valid[row] = ids[row] > 0 ? 1 : 0;
    return valid;
}

} // namespace

void addResultModelColumns(QueryTable& result, const results::ResultTable& source, const results::ResultData& data)
{
    const std::shared_ptr<results::ResultAttachment> attachment = source.attachment();
    if (!attachment)
        return;
    const std::shared_ptr<model::Model> model = attachment->model();
    if (!model)
        return;
    const model::CardRegistry& registry = model::CardRegistry::instance();
    const std::size_t rows = data.rowCount;
    const auto repeat = [&](const auto& perRow)
    {
        using Item = typename std::decay_t<decltype(perRow)>::value_type;
        std::vector<Item> column;
        column.reserve(rows * data.stepCount);
        for (std::size_t step = 0; step < data.stepCount; ++step)
            column.insert(column.end(), perRow.begin(), perRow.end());
        return column;
    };
    const results::RowKind kind = results::resultKindInfo(source.meta().kind).rows;
    const auto nodeColumn = [&](const results::LabelArray<std::int32_t>& ids, const char* name)
    {
        std::vector<std::int32_t> modelIds(ids.size());
        for (std::size_t row = 0; row < ids.size(); ++row)
            modelIds[row] = ids[row] > 0 ? attachment->modelNodeId(*model, ids[row]) : 0;
        result.addColumn(name, nullable(QueryColumn::primitive(repeat(modelIds)), repeat(positive(modelIds))));
    };
    const auto elementColumns = [&](const results::LabelArray<std::int32_t>& ids)
    {
        std::vector<std::int32_t> modelIds(ids.size(), 0);
        std::vector<std::int32_t> properties(ids.size(), 0);
        std::vector<std::string> cards(ids.size());
        for (std::size_t row = 0; row < ids.size(); ++row)
        {
            if (ids[row] <= 0)
                continue;
            const std::optional<model::CardLocation> location = attachment->elementLocation(*model, ids[row]);
            if (!location)
                continue;
            modelIds[row] = model->findTable(location->table)->idAt(location->row);
            cards[row] = std::string(registry.entry(location->table).info.name);
            if (const auto pid = analysis::StoredFrames::propertyOf(*model, *location))
                properties[row] = *pid;
        }
        // a row the model does not link to a card has nulls
        result.addColumn("modelElementId",
                         nullable(QueryColumn::primitive(repeat(modelIds)), repeat(positive(modelIds))));
        result.addColumn("propId", nullable(QueryColumn::primitive(repeat(properties)), repeat(positive(properties))));
        const std::vector<std::string> repeated = repeat(cards);
        result.addColumn(
            "card", nullable(QueryColumn::utf8(std::span<const std::string>(repeated)), repeat(positive(modelIds))));
    };
    switch (kind)
    {
    case results::RowKind::GridPoint:
        nodeColumn(data.ids, "modelNodeId");
        break;
    case results::RowKind::Element:
        elementColumns(data.ids);
        break;
    case results::RowKind::GridPointElement:
        nodeColumn(data.ids, "modelNodeId");
        if (!data.elementIds.empty())
            elementColumns(data.elementIds);
        break;
    case results::RowKind::Mode:
        break;
    }
}

std::shared_ptr<QueryTable> resultTableAsQuery(const std::shared_ptr<const results::ResultData>& decoded,
                                               const results::ResultTable& table, bool block)
{
    const results::ResultData& data = *decoded;
    const results::ResultTableMeta& meta = table.meta();
    const std::size_t rows = data.stepCount * data.rowCount;
    auto result = std::make_shared<QueryTable>();
    result->addColumn("subcase", QueryColumn::primitive(std::vector<std::int32_t>(rows, meta.subcase)));
    const std::string noun = stepNoun(meta.stepKind);
    if (!noun.empty())
    {
        const auto spread = [&](const std::vector<double>& perStep)
        {
            std::vector<double> column;
            column.reserve(rows);
            for (std::size_t step = 0; step < data.stepCount; ++step)
                column.insert(column.end(), data.rowCount, perStep[step]);
            return column;
        };
        if (meta.stepKind == results::StepKind::Mode)
        {
            std::vector<std::int32_t> modes;
            modes.reserve(rows);
            for (std::size_t step = 0; step < data.stepCount; ++step)
                modes.insert(modes.end(), data.rowCount, static_cast<std::int32_t>(data.steps[step]));
            result->addColumn(noun, QueryColumn::primitive(std::move(modes)));
        }
        else
        {
            result->addColumn(noun, QueryColumn::primitive(spread(data.steps)));
        }
        if (!data.eigenvalues.empty())
            result->addColumn("eigenvalue", QueryColumn::primitive(spread(data.eigenvalues)));
        if (!data.eigenvaluesImag.empty())
            result->addColumn("eigenvalueImag", QueryColumn::primitive(spread(data.eigenvaluesImag)));
        if (!data.cycles.empty())
            result->addColumn("cycles", QueryColumn::primitive(spread(data.cycles)));
    }
    const auto perStep = [&](const auto& labels)
    {
        using Item = typename std::decay_t<decltype(labels)>::value_type;
        std::vector<Item> column;
        column.reserve(rows);
        for (std::size_t step = 0; step < data.stepCount; ++step)
            column.insert(column.end(), labels.begin(), labels.end());
        return column;
    };
    if (data.isElementData())
    {
        result->addColumn("elementId", QueryColumn::primitive(perStep(data.ids)));
        if (!data.nodeIds.empty())
            result->addColumn("nodeId", QueryColumn::primitive(perStep(data.nodeIds)));
        result->addColumn("layer", QueryColumn::primitive(perStep(data.layers)));
        if (!data.positions.empty())
            result->addColumn("position", QueryColumn::primitive(perStep(data.positions)));
    }
    else if (results::resultKindInfo(meta.kind).rows == results::RowKind::Mode)
    {
        result->addColumn("mode", QueryColumn::primitive(perStep(data.ids)));
    }
    else if (results::resultKindInfo(meta.kind).rows == results::RowKind::GridPointElement)
    {
        result->addColumn("nodeId", QueryColumn::primitive(perStep(data.ids)));
        if (!data.elementIds.empty())
            result->addColumn("elementId", QueryColumn::primitive(perStep(data.elementIds)));
        for (const results::ResultText& text : data.texts)
        {
            const std::vector<std::string> labels = text.values();
            std::vector<std::string> column;
            column.reserve(rows);
            for (std::size_t step = 0; step < data.stepCount; ++step)
                column.insert(column.end(), labels.begin(), labels.end());
            result->addColumn(text.name, QueryColumn::utf8(std::span<const std::string>(column)));
        }
    }
    else
    {
        result->addColumn("nodeId", QueryColumn::primitive(perStep(data.ids)));
        result->addColumn("pointType", QueryColumn::primitive(perStep(data.pointTypes)));
    }

    const bool isComplex = results::isComplexType(data.type);
    const bool single = data.type == results::ValueType::Float32 || data.type == results::ValueType::Complex64;
    const std::size_t perValue = isComplex ? 2 : 1;
    const void* values = data.bytes();
    if (block)
    {
        const std::size_t width = data.componentCount * perValue;
        const std::size_t count = rows * width;
        const ColumnBuffer buffer = ColumnBuffer::view(std::shared_ptr<const void>(decoded, decoded.get()), values,
                                                       count * (single ? sizeof(float) : sizeof(double)));
        QueryColumn child = QueryColumn::fromBuffer(single ? ColumnType::Float32 : ColumnType::Float64,
                                                    static_cast<std::int64_t>(count), buffer);
        result->addColumn("values", QueryColumn::fixedList(std::move(child), static_cast<std::int32_t>(width),
                                                           static_cast<std::int64_t>(rows)));
        addResultModelColumns(*result, table, data);
        return result;
    }
    const auto gather = [&](std::size_t component, std::size_t part, auto sample)
    {
        using Real = decltype(sample);
        const Real* all = static_cast<const Real*>(values);
        std::vector<Real> column(rows);
        for (std::size_t row = 0; row < rows; ++row)
            column[row] = all[(row * data.componentCount + component) * perValue + part];
        return QueryColumn::primitive(std::move(column));
    };
    for (std::size_t component = 0; component < data.componentCount; ++component)
    {
        for (std::size_t part = 0; part < perValue; ++part)
        {
            const std::string name =
                isComplex ? data.components[component] + (part == 0 ? "Real" : "Imag") : data.components[component];
            result->addColumn(name, single ? gather(component, part, float{}) : gather(component, part, double{}));
        }
    }
    addResultModelColumns(*result, table, data);
    return result;
}

namespace
{

// A matrix as a table of its terms: `row` and `column` (from 0), `value` (a complex matrix: `valueReal` and
// `valueImag`) and, when the matrix has the labels of its degrees of freedom, the label of the row and of the column
// of each term (`rowId`, `rowComponent`, `columnId`, `columnComponent`). The shape is not a column.
std::shared_ptr<QueryTable> matrixAsQuery(const results::NamedResultData& data)
{
    auto table = std::make_shared<QueryTable>();
    const results::ResultArray& rows = *data.find("row");
    const results::ResultArray& columns = *data.find("column");
    const results::ResultArray& value = *data.find("value");
    table->addColumn("row", QueryColumn::primitive(arrayInts(rows)));
    table->addColumn("column", QueryColumn::primitive(arrayInts(columns)));
    if (value.type == results::ArrayType::Complex64 || value.type == results::ArrayType::Complex128)
    {
        const std::size_t count = value.valueCount();
        std::vector<double> real(count);
        std::vector<double> imaginary(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            const std::complex<double> number = value.type == results::ArrayType::Complex64
                                                    ? std::complex<double>(value.values<std::complex<float>>()[index])
                                                    : value.values<std::complex<double>>()[index];
            real[index] = number.real();
            imaginary[index] = number.imag();
        }
        table->addColumn("valueReal", QueryColumn::primitive(std::move(real)));
        table->addColumn("valueImag", QueryColumn::primitive(std::move(imaginary)));
    }
    else
    {
        table->addColumn("value", QueryColumn::primitive(arrayDoubles(value)));
    }
    const auto label = [&](const char* source, const results::ResultArray& index, const char* name)
    {
        const results::ResultArray* labels = data.find(source);
        if (labels == nullptr)
            return;
        const std::vector<std::int32_t> values = arrayInts(*labels);
        const std::vector<std::int32_t> of = arrayInts(index);
        std::vector<std::int32_t> column(of.size());
        for (std::size_t term = 0; term < of.size(); ++term)
            column[term] = values[static_cast<std::size_t>(of[term])];
        table->addColumn(name, QueryColumn::primitive(std::move(column)));
    };
    label("rowIds", rows, "rowId");
    label("rowComponents", rows, "rowComponent");
    label("columnIds", columns, "columnId");
    label("columnComponents", columns, "columnComponent");
    label("rowHarmonics", rows, "rowHarmonic");
    label("columnHarmonics", columns, "columnHarmonic");
    return table;
}

} // namespace

std::shared_ptr<QueryTable> namedResultAsQuery(const results::NamedResultData& data, bool lists)
{
    if (data.find("shape") != nullptr && data.find("row") != nullptr && data.find("column") != nullptr &&
        data.find("value") != nullptr)
        return matrixAsQuery(data);
    auto table = std::make_shared<QueryTable>();
    std::size_t rows = 0;
    for (const results::ResultArray& array : data.arrays)
        if (array.shape.size() == 1)
            rows = std::max(rows, array.shape[0]);
    if (lists)
        for (const results::ResultText& text : data.texts)
            rows = std::max(rows, text.size());
    for (const results::ResultArray& array : data.arrays)
    {
        if (lists && array.shape.size() > 1 && array.shape[0] == rows &&
            (array.type == results::ArrayType::Float32 || array.type == results::ArrayType::Float64))
        {
            std::size_t width = 1;
            for (std::size_t axis = 1; axis < array.shape.size(); ++axis)
                width *= array.shape[axis];
            const bool single = array.type == results::ArrayType::Float32;
            QueryColumn child = single ? QueryColumn::primitive(std::vector<float>(array.values<float>().begin(),
                                                                                   array.values<float>().end()))
                                       : QueryColumn::primitive(std::vector<double>(array.values<double>().begin(),
                                                                                    array.values<double>().end()));
            table->addColumn(array.name, QueryColumn::fixedList(std::move(child), static_cast<std::int32_t>(width),
                                                                static_cast<std::int64_t>(rows)));
            continue;
        }
        if (array.shape.size() != 1 || array.shape[0] != rows)
            continue;
        switch (array.type)
        {
        case results::ArrayType::Int32:
            table->addColumn(array.name, QueryColumn::primitive(arrayInts(array)));
            break;
        case results::ArrayType::Int64:
        {
            std::vector<std::int64_t> values(array.values<std::int64_t>().begin(), array.values<std::int64_t>().end());
            table->addColumn(array.name, QueryColumn::primitive(std::move(values)));
            break;
        }
        case results::ArrayType::Float32:
        case results::ArrayType::Float64:
            table->addColumn(array.name, QueryColumn::primitive(arrayDoubles(array)));
            break;
        case results::ArrayType::Complex64:
        case results::ArrayType::Complex128:
        {
            std::vector<double> real(rows);
            std::vector<double> imaginary(rows);
            for (std::size_t index = 0; index < rows; ++index)
            {
                const std::complex<double> value =
                    array.type == results::ArrayType::Complex64
                        ? std::complex<double>(array.values<std::complex<float>>()[index])
                        : array.values<std::complex<double>>()[index];
                real[index] = value.real();
                imaginary[index] = value.imag();
            }
            table->addColumn(array.name + "Real", QueryColumn::primitive(std::move(real)));
            table->addColumn(array.name + "Imag", QueryColumn::primitive(std::move(imaginary)));
            break;
        }
        }
    }
    for (const results::ResultText& text : data.texts)
        if (text.size() == rows)
        {
            const std::vector<std::string> values = text.values();
            table->addColumn(text.name, QueryColumn::utf8(std::span<const std::string>(values)));
        }
    return table;
}

std::shared_ptr<const QueryTable> concatenateResultTables(const std::vector<std::shared_ptr<const QueryTable>>& parts,
                                                          const std::string& what)
{
    if (parts.size() == 1)
        return parts.front();
    std::vector<std::string> names;
    for (const auto& part : parts)
        for (const std::string& name : part->columnNames())
            if (std::find(names.begin(), names.end(), name) == names.end())
                names.push_back(name);
    std::vector<std::int64_t> lengths;
    for (const auto& part : parts)
        lengths.push_back(part->rowCount());
    auto result = std::make_shared<QueryTable>();
    for (const std::string& name : names)
    {
        std::vector<const QueryColumn*> columns;
        const QueryColumn* first = nullptr;
        for (const auto& part : parts)
        {
            const QueryColumn* column = part->has(name) ? &part->column(name) : nullptr;
            columns.push_back(column);
            if (column != nullptr && first == nullptr)
                first = column;
        }
        for (const QueryColumn* column : columns)
            if (column != nullptr && !sameLayout(*first, *column))
                throw QueryError("the tables of " + what + " do not give the column '" + name +
                                 "' the same type (a 32-bit and a 64-bit file, or a fixed and a variable width): query "
                                 "one result set or one subcase at a time");
        result->addColumn(name, concatenate(columns, lengths));
    }
    return result;
}

std::vector<std::string> resultSourceNames(const std::vector<ResultSet>& sets)
{
    std::set<std::string> names;
    const auto variants = [&](results::ResultVariant variant, const std::string& prefix)
    {
        for (const results::ResultKindInfo& info : results::allResultKinds())
        {
            for (const ResultSet& set : sets)
            {
                if (info.family)
                {
                    for (const std::string& element : set.tables->elementNames(info.kind, variant))
                        names.insert(prefix + std::string(info.name) + "." + element);
                }
                else if (set.tables->container(info.kind, variant).tableCount() > 0)
                {
                    names.insert(prefix + std::string(info.name));
                }
            }
        }
    };
    variants(results::ResultVariant::Plain, "");
    for (const results::ResultVariant variant : results::allResultVariants())
        variants(variant, std::string(results::resultVariantName(variant)) + ".");
    for (const ResultSet& set : sets)
        for (const auto& named : set.tables->namedResults())
            names.insert(named->meta().kind);
    return {names.begin(), names.end()};
}

std::shared_ptr<const QueryTable> resultSourceTable(const std::vector<ResultSet>& sets, std::string_view name,
                                                    const results::DecodeContext& context)
{
    std::vector<std::string> parts;
    {
        std::size_t start = 0;
        while (start <= name.size())
        {
            const std::size_t dot = name.find('.', start);
            const std::size_t stop = dot == std::string_view::npos ? name.size() : dot;
            parts.emplace_back(name.substr(start, stop - start));
            if (dot == std::string_view::npos)
                break;
            start = dot + 1;
        }
    }
    const auto unknown = [&]() -> QueryError
    {
        return QueryError("there is no result source called '" + std::string(name) +
                          "' (the sources of these results are " + joinNames(resultSourceNames(sets)) + ")");
    };
    std::vector<std::shared_ptr<const QueryTable>> tables;
    const bool several = sets.size() > 1;
    const auto addAnalysis = [&](QueryTable& table, const ResultSet& set)
    {
        if (!several)
            return;
        const std::vector<std::string> labels(static_cast<std::size_t>(table.rowCount()), set.analysis);
        table.addColumn("analysis", QueryColumn::utf8(std::span<const std::string>(labels)));
    };
    if (const results::NamedKindInfo* namedKind = parts.size() == 1 ? results::findNamedKind(parts[0]) : nullptr;
        namedKind != nullptr)
    {
        const bool lists = namedKind->listColumns;
        for (const ResultSet& set : sets)
            for (const auto& named : set.tables->named(parts[0]))
            {
                const std::shared_ptr<const results::NamedResultData> data = named->data(context);
                std::shared_ptr<QueryTable> table = namedResultAsQuery(*data, lists);
                if (table->columnCount() == 0)
                    continue;
                // the subcase the result belongs to, then its columns; a result whose rows say their own subcase (the
                // EPSILON of each subcase, the monitor point loads) keeps that column alone
                auto labelled = std::make_shared<QueryTable>();
                const auto rows = static_cast<std::size_t>(table->rowCount());
                if (!table->has("subcase"))
                    labelled->addColumn("subcase",
                                        QueryColumn::primitive(std::vector<std::int32_t>(rows, named->meta().subcase)));
                for (std::size_t index = 0; index < table->columnCount(); ++index)
                    labelled->addColumn(table->name(index), table->column(index));
                addAnalysis(*labelled, set);
                tables.push_back(labelled);
            }
        if (tables.empty())
            throw unknown();
        return concatenateResultTables(tables, "the named results " + parts[0]);
    }
    std::size_t at = 0;
    results::ResultVariant variant = results::ResultVariant::Plain;
    if (parts.size() > 1)
    {
        if (const auto found = results::resultVariantFromName(parts[0]);
            found && *found != results::ResultVariant::Plain)
        {
            variant = *found;
            at = 1;
        }
    }
    const auto kind = results::resultKindFromName(parts[at]);
    if (!kind)
        throw unknown();
    const results::ResultKindInfo& info = results::resultKindInfo(*kind);
    const bool wantsElement = parts.size() > at + 1;
    if (info.family != wantsElement)
        throw unknown();
    for (const ResultSet& set : sets)
    {
        const results::ResultContainer& container = info.family
                                                        ? set.tables->elementContainer(*kind, parts[at + 1], variant)
                                                        : set.tables->container(*kind, variant);
        for (const std::shared_ptr<results::ResultTable>& table : container.tables())
        {
            const std::shared_ptr<const results::ResultData> data = table->data(context);
            std::shared_ptr<QueryTable> converted = resultTableAsQuery(data, *table, false);
            addAnalysis(*converted, set);
            tables.push_back(std::move(converted));
        }
    }
    if (tables.empty())
        throw unknown();
    return concatenateResultTables(tables, "the container " + std::string(name));
}

} // namespace oor::query
