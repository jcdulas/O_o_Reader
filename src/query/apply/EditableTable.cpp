#include "oor/query/apply/EditableTable.hpp"

#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/providers/StoredColumnCatalogue.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace oor::query
{

namespace
{

constexpr std::uint32_t NO_NEXT = 0xFFFFFFFFU;

std::uint16_t nodeType()
{
    return model::typeIndexOf<model::NodeTable>();
}

std::string cardName(std::uint16_t type)
{
    return std::string(model::CardRegistry::instance().entry(type).info.name);
}

// The axis a position field stands for (0, 1, 2), or 3 for xyzGlobal.
int axisOf(std::string_view field) noexcept
{
    if (field == "x")
        return 0;
    if (field == "y")
        return 1;
    if (field == "z")
        return 2;
    return 3;
}

// A number column's value in row `row` as an integer; false for a null or a value that is not a whole number.
bool readInteger(const QueryColumn& column, std::size_t row, std::int64_t& value) noexcept
{
    if (column.isNull(row))
        return false;
    switch (column.type())
    {
    case ColumnType::UInt8:
        value = column.as<std::uint8_t>()[row];
        return true;
    case ColumnType::Int32:
        value = column.as<std::int32_t>()[row];
        return true;
    case ColumnType::UInt32:
        value = column.as<std::uint32_t>()[row];
        return true;
    case ColumnType::Int64:
        value = column.as<std::int64_t>()[row];
        return true;
    case ColumnType::Float64:
    {
        const double real = column.as<double>()[row];
        if (!std::isfinite(real) || real != std::floor(real) || std::fabs(real) > 9.0e18)
            return false;
        value = static_cast<std::int64_t>(real);
        return true;
    }
    default:
        return false;
    }
}

template <typename F>
void naming(const std::string& column, F&& action)
{
    try
    {
        action();
    }
    catch (const QueryTypeError& error)
    {
        throw QueryTypeError("column '" + column + "': " + error.what());
    }
}

} // namespace

bool isPositionField(std::string_view field) noexcept
{
    return field == "x" || field == "y" || field == "z" || field == "xyzGlobal";
}

EditableTable::EditableTable(std::shared_ptr<const QueryTable> base, std::shared_ptr<const EditPolicy> policy)
    : m_tracker(std::move(base)), m_policy(std::move(policy))
{
    m_editability.resize(columnCount());
    m_currentColumns.resize(columnCount());
}

std::size_t EditableTable::columnIndex(std::string_view name) const
{
    if (const auto found = base()->find(name))
        return *found;
    (void)base()->column(name); // throws QueryError with the names that exist
    return 0;
}

std::map<std::uint16_t, std::uint32_t> EditableTable::typesPresent() const
{
    if (!m_types)
    {
        std::map<std::uint16_t, std::uint32_t> types;
        if (hasCards())
        {
            const RowOrigin& origin = base()->origin();
            std::uint16_t last = 0xFFFFU;
            for (std::size_t row = 0; row < origin.types->size(); ++row)
            {
                const std::uint16_t type = (*origin.types)[row];
                if (type == last)
                    continue;
                last = type;
                types.emplace(type, (*origin.keys)[row]);
            }
        }
        m_types = std::move(types);
    }
    return *m_types;
}

std::vector<std::string> EditableTable::keyFields() const
{
    std::vector<std::string> fields;
    for (const auto& [type, key] : typesPresent())
    {
        (void)key;
        if (model::CardRegistry::instance().entry(type).info.idSpace == base::IdSpace::None)
            continue;
        const std::vector<std::string> names = StoredColumnCatalogue::instance().names(type);
        if (!names.empty() && std::find(fields.begin(), fields.end(), names.front()) == fields.end())
            fields.push_back(names.front());
    }
    return fields;
}

ColumnEditability EditableTable::computeEditability(std::size_t column) const
{
    const ColumnInfo& info = base()->info(column);
    ColumnEditability result;
    const auto refuse = [&](std::string reason)
    {
        result.editable = false;
        result.reason = std::move(reason);
        return result;
    };
    if (!ColumnEdits::supports(base()->column(column)))
        return refuse("its values (" + std::string(columnTypeName(base()->column(column).type())) +
                      ") cannot be set cell by cell");
    switch (info.origin)
    {
    case ColumnOrigin::Joined:
        return refuse("it is joined from the cards of another table: edit that table");
    case ColumnOrigin::Derived:
        return refuse(hasCards() ? "it is not a field of the cards: an expression, a card type name or an aggregate"
                                 : "the table is not made of cards (it comes from arrays or a group): there is nothing "
                                   "to write the value to");
    case ColumnOrigin::Computed:
    case ColumnOrigin::Materialised:
    {
        if (!isPositionField(info.field))
            return refuse("it is a quantity computed from the model, not a field of a card");
        if (!hasCards())
            return refuse("the table is not made of cards");
        const std::map<std::uint16_t, std::uint32_t> types = typesPresent();
        if (types.size() != 1 || types.begin()->first != nodeType())
            return refuse("it is a position in the basic system, editable only when every row is a GRID card");
        result.editable = true;
        result.position = true;
        return result;
    }
    case ColumnOrigin::Stored:
        break;
    }
    if (!hasCards())
        return refuse("the table is not made of cards: there is nothing to write the value to");
    const std::vector<std::string> keys = keyFields();
    if (std::find(keys.begin(), keys.end(), info.field) != keys.end())
        return refuse("it is the key ID of the cards: change an ID with the renumbering, not with an edit");
    std::string firstReason;
    std::size_t good = 0;
    for (const auto& [type, key] : typesPresent())
    {
        const std::string problem = m_policy ? m_policy->fieldProblem(type, info.field, key) : std::string();
        if (problem.empty())
        {
            ++good;
            continue;
        }
        result.rejectedTypes.push_back(type);
        if (firstReason.empty())
            firstReason = problem;
    }
    if (good == 0)
        return refuse(firstReason.empty() ? "no field setter" : firstReason);
    result.editable = true;
    if (!result.rejectedTypes.empty())
        result.reason = firstReason; // the types that cannot take it
    return result;
}

const ColumnEditability& EditableTable::editability(std::size_t column) const
{
    if (!m_editability[column])
        m_editability[column] = computeEditability(column);
    return *m_editability[column];
}

void EditableTable::requireEditable(std::size_t column) const
{
    const ColumnEditability& state = editability(column);
    if (!state.editable)
        throw ReadOnlyColumnError("column '" + base()->name(column) + "' is read-only: " + state.reason);
}

void EditableTable::checkWritable(std::size_t column, std::span<const std::uint32_t> rows) const
{
    requireEditable(column);
    const ColumnEditability& state = editability(column);
    if (state.rejectedTypes.empty())
        return;
    for (const std::uint32_t row : rows)
    {
        if (row >= rowCount())
            throw QueryError("row " + std::to_string(row) + " is outside the table (" + std::to_string(rowCount()) +
                             " rows)");
        const std::uint16_t type = cardTypeAt(row);
        if (std::find(state.rejectedTypes.begin(), state.rejectedTypes.end(), type) != state.rejectedTypes.end())
            throw ReadOnlyColumnError("column '" + base()->name(column) + "' is read-only for row " +
                                      std::to_string(row) + ", a " + cardName(type) + ": " + state.reason);
    }
}

void EditableTable::invalidate(std::size_t column)
{
    m_currentColumns[column].reset();
    m_current.reset();
}

std::optional<std::size_t> EditableTable::positionColumn(std::string_view field) const
{
    for (std::size_t column = 0; column < columnCount(); ++column)
    {
        if (base()->info(column).field == field && editability(column).position)
            return column;
    }
    return std::nullopt;
}

void EditableTable::mirrorPositions(std::size_t column, std::span<const std::uint32_t> rows)
{
    if (!editability(column).position)
        return;
    const int axis = axisOf(base()->info(column).field);
    if (axis < 3)
    {
        const auto global = positionColumn("xyzGlobal");
        if (!global)
            return;
        ColumnEdits& target = m_tracker.edits(*global);
        for (const std::uint32_t row : rows)
        {
            const CellValue now = m_tracker.value(column, row);
            CellValue cell = m_tracker.value(*global, row);
            if (now.isNull() || cell.kind() != CellValue::Kind::RealList || cell.listSize() != 3)
                continue;
            std::vector<double> values = cell.asReals();
            values[static_cast<std::size_t>(axis)] = now.asReal();
            target.set(row, CellValue::ofReals(std::move(values)));
        }
        invalidate(*global);
        return;
    }
    for (int other = 0; other < 3; ++other)
    {
        static constexpr const char* NAMES[] = {"x", "y", "z"};
        const auto scalar = positionColumn(NAMES[other]);
        if (!scalar)
            continue;
        ColumnEdits& target = m_tracker.edits(*scalar);
        for (const std::uint32_t row : rows)
        {
            const CellValue now = m_tracker.value(column, row);
            if (now.kind() != CellValue::Kind::RealList || now.listSize() != 3)
                continue;
            target.set(row, CellValue::ofReal(now.listReal(static_cast<std::size_t>(other))));
        }
        invalidate(*scalar);
    }
}

void EditableTable::setCell(std::size_t column, std::size_t row, const CellValue& value)
{
    const auto rows = static_cast<std::uint32_t>(row);
    if (row >= rowCount())
        throw QueryError("row " + std::to_string(row) + " is outside the table (" + std::to_string(rowCount()) +
                         " rows)");
    checkWritable(column, std::span<const std::uint32_t>(&rows, 1));
    naming(base()->name(column), [&] { m_tracker.edits(column).set(row, value); });
    invalidate(column);
    mirrorPositions(column, std::span<const std::uint32_t>(&rows, 1));
}

void EditableTable::setBroadcast(std::size_t column, std::span<const std::uint32_t> rows, const CellValue& value)
{
    checkWritable(column, rows);
    ColumnEdits& edits = m_tracker.edits(column);
    naming(base()->name(column),
           [&]
           {
               edits.check(value);
               for (const std::uint32_t row : rows)
               {
                   if (row >= rowCount())
                       throw QueryError("row " + std::to_string(row) + " is outside the table (" +
                                        std::to_string(rowCount()) + " rows)");
               }
               for (const std::uint32_t row : rows)
                   edits.set(row, value);
           });
    invalidate(column);
    mirrorPositions(column, rows);
}

void EditableTable::setReals(std::size_t column, std::span<const std::uint32_t> rows, std::span<const double> values)
{
    checkWritable(column, rows);
    naming(base()->name(column), [&] { m_tracker.edits(column).setReals(rows, values); });
    invalidate(column);
    mirrorPositions(column, rows);
}

void EditableTable::setIntegers(std::size_t column, std::span<const std::uint32_t> rows,
                                std::span<const std::int64_t> values)
{
    checkWritable(column, rows);
    naming(base()->name(column), [&] { m_tracker.edits(column).setIntegers(rows, values); });
    invalidate(column);
    mirrorPositions(column, rows);
}

void EditableTable::setValues(std::size_t column, std::span<const std::uint32_t> rows,
                              std::span<const CellValue> values)
{
    if (values.size() != rows.size())
        throw QueryError(std::to_string(values.size()) + " values for " + std::to_string(rows.size()) + " rows");
    checkWritable(column, rows);
    ColumnEdits& edits = m_tracker.edits(column);
    naming(base()->name(column),
           [&]
           {
               for (std::size_t index = 0; index < rows.size(); ++index)
               {
                   if (rows[index] >= rowCount())
                       throw QueryError("row " + std::to_string(rows[index]) + " is outside the table (" +
                                        std::to_string(rowCount()) + " rows)");
                   edits.check(values[index]);
               }
               for (std::size_t index = 0; index < rows.size(); ++index)
                   edits.set(rows[index], values[index]);
           });
    invalidate(column);
    mirrorPositions(column, rows);
}

void EditableTable::reset()
{
    m_tracker.clear();
    m_currentColumns.assign(columnCount(), std::nullopt);
    m_current.reset();
}

void EditableTable::reset(std::size_t column)
{
    if (editability(column).position)
    {
        for (std::size_t other = 0; other < columnCount(); ++other)
        {
            if (editability(other).position)
            {
                m_tracker.clear(other);
                invalidate(other);
            }
        }
        return;
    }
    m_tracker.clear(column);
    invalidate(column);
}

void EditableTable::reset(std::size_t column, std::span<const std::uint32_t> rows)
{
    const bool position = editability(column).position;
    for (std::size_t other = 0; other < columnCount(); ++other)
    {
        if (other != column && !(position && editability(other).position))
            continue;
        if (m_tracker.find(other) == nullptr)
            continue;
        ColumnEdits& edits = m_tracker.edits(other);
        for (const std::uint32_t row : rows)
            edits.unset(row);
        invalidate(other);
    }
}

QueryColumn EditableTable::currentColumn(std::size_t column) const
{
    const ColumnEdits* edits = m_tracker.find(column);
    if (edits == nullptr || edits->changedCount() == 0)
        return base()->column(column);
    if (!m_currentColumns[column])
        m_currentColumns[column] = edits->materialize(base()->column(column));
    return *m_currentColumns[column];
}

std::shared_ptr<const QueryTable> EditableTable::current() const
{
    if (!hasChanges())
        return base();
    if (!m_current)
    {
        auto table = std::make_shared<QueryTable>();
        for (std::size_t column = 0; column < columnCount(); ++column)
            table->addColumn(base()->name(column), currentColumn(column), base()->info(column).origin,
                             base()->info(column).field);
        table->setOrigin(base()->origin());
        table->setReport(base()->report());
        table->setContractVersion(base()->contractVersion());
        m_current = std::move(table);
    }
    return m_current;
}

void EditableTable::commit()
{
    std::shared_ptr<const QueryTable> next = current();
    m_tracker.rebase(std::move(next));
    m_currentColumns.assign(columnCount(), std::nullopt);
    m_current.reset();
    m_editability.assign(columnCount(), std::nullopt);
    m_types.reset();
}

UpdateResult EditableTable::update(const QueryTable& data, const UpdateOptions& options)
{
    UpdateResult result;
    const std::size_t rows = rowCount();

    // The keys: which column of the table and of the data holds them.
    std::string keyName;
    bool byCardKey = false;
    if (!options.on.empty())
    {
        keyName = options.on;
        byCardKey = keyName == "cardKey" || keyName == "cardKeys";
    }
    else
    {
        for (const std::string& field : keyFields())
        {
            if (base()->has(field) && data.has(field))
            {
                keyName = field;
                break;
            }
        }
        if (keyName.empty() && (data.has("cardKey") || data.has("cardKeys")))
        {
            keyName = data.has("cardKey") ? "cardKey" : "cardKeys";
            byCardKey = true;
        }
        if (keyName.empty())
        {
            std::string wanted;
            for (const std::string& field : keyFields())
                wanted += (wanted.empty() ? "" : ", ") + field;
            throw QueryError("update: the data has no column to match the rows by: " +
                             (wanted.empty() ? std::string("name one with on=") : wanted + " or cardKey") +
                             " (columns of the data: " +
                             [&]
                             {
                                 std::string names;
                                 for (const std::string& name : data.columnNames())
                                     names += (names.empty() ? "" : ", ") + name;
                                 return names;
                             }() +
                             ")");
        }
    }
    const QueryColumn* dataKeys = nullptr;
    if (byCardKey)
    {
        if (!hasCards())
            throw QueryError("update: the table is not made of cards, it has no card keys");
        if (data.has(keyName))
            dataKeys = &data.column(keyName);
        else if (data.has("cardKey"))
            dataKeys = &data.column("cardKey");
        else if (data.has("cardKeys"))
            dataKeys = &data.column("cardKeys");
        else
            throw QueryError("update: the data has no column cardKey");
    }
    else
    {
        if (!base()->has(keyName))
            throw QueryError("update: the table has no column '" + keyName + "' to match the rows by");
        if (!data.has(keyName))
            throw QueryError("update: the data has no column '" + keyName + "' to match the rows by");
        dataKeys = &data.column(keyName);
    }

    // The rows of the table, by key.
    std::unordered_map<std::int64_t, std::uint32_t> head;
    std::vector<std::uint32_t> next(rows, NO_NEXT);
    {
        const QueryColumn* tableKeys = byCardKey ? nullptr : &base()->column(keyName);
        head.reserve(rows);
        for (std::size_t row = rows; row-- > 0;)
        {
            std::int64_t key = 0;
            if (byCardKey)
                key = (*base()->origin().keys)[row];
            else if (!readInteger(*tableKeys, row, key))
                continue;
            const auto [slot, inserted] = head.emplace(key, static_cast<std::uint32_t>(row));
            if (!inserted)
            {
                next[row] = slot->second;
                slot->second = static_cast<std::uint32_t>(row);
            }
        }
    }

    // The keys of the data, matched.
    const auto dataRows = static_cast<std::size_t>(data.rowCount());
    std::vector<std::int64_t> keys(dataRows);
    std::unordered_map<std::int64_t, std::size_t> seen;
    seen.reserve(dataRows);
    for (std::size_t row = 0; row < dataRows; ++row)
    {
        if (!readInteger(*dataKeys, row, keys[row]))
            throw QueryError("update: row " + std::to_string(row) + " of the data has no usable key in column '" +
                             keyName + "' (a whole number is expected)");
        if (!seen.emplace(keys[row], row).second)
            throw QueryError("update: the key " + std::to_string(keys[row]) + " appears twice in the data (rows " +
                             std::to_string(seen[keys[row]]) + " and " + std::to_string(row) + ")");
    }
    std::vector<std::size_t> matched; // rows of the data that have rows in the table
    for (std::size_t row = 0; row < dataRows; ++row)
    {
        if (head.find(keys[row]) == head.end())
            result.missingKeys.push_back(keys[row]);
        else
            matched.push_back(row);
    }
    if (!result.missingKeys.empty() && !options.ignoreMissing)
    {
        std::string listed;
        for (std::size_t index = 0; index < result.missingKeys.size() && index < 10; ++index)
            listed += (index == 0 ? "" : ", ") + std::to_string(result.missingKeys[index]);
        if (result.missingKeys.size() > 10)
            listed += ", ...";
        throw QueryError("update: " + std::to_string(result.missingKeys.size()) +
                         " key(s) of the data match no row of " + "the table (" + keyName + " " + listed +
                         "); pass onMissing=\"ignore\" to leave them out");
    }
    result.matchedKeys = matched.size();

    // The columns taken.
    struct Planned
    {
        std::size_t column = 0;
        std::size_t dataColumn = 0;
    };
    std::vector<Planned> columns;
    const auto named = [&](const std::string& name)
    { return std::find(options.columns.begin(), options.columns.end(), name) != options.columns.end(); };
    for (const std::string& name : options.columns)
    {
        if (!data.has(name))
            throw QueryError("update: the data has no column '" + name + "'");
    }
    for (std::size_t index = 0; index < data.columnCount(); ++index)
    {
        const std::string& name = data.name(index);
        if (name == keyName || (byCardKey && (name == "cardKey" || name == "cardKeys")))
            continue;
        if (!options.columns.empty() && !named(name))
            continue;
        const auto found = base()->find(name);
        if (!found)
        {
            if (named(name))
                throw QueryError("update: the table has no column '" + name + "'");
            result.columnsIgnored[name] = "it is not a column of the table";
            continue;
        }
        const ColumnEditability& state = editability(*found);
        if (!state.editable)
        {
            if (named(name))
                throw ReadOnlyColumnError("column '" + name + "' is read-only: " + state.reason);
            result.columnsIgnored[name] = state.reason;
            continue;
        }
        columns.push_back(Planned{*found, index});
    }

    // The cells that differ, checked before anything is written.
    struct Write
    {
        std::size_t column = 0;
        std::uint32_t row = 0;
        CellValue value;
    };
    std::vector<Write> writes;
    std::size_t matchedRows = 0;
    for (const std::size_t dataRow : matched)
    {
        for (std::uint32_t row = head.find(keys[dataRow])->second; row != NO_NEXT; row = next[row])
        {
            ++matchedRows;
            for (const Planned& planned : columns)
            {
                CellValue value = cellAt(data.column(planned.dataColumn), dataRow);
                if (m_tracker.value(planned.column, row).sameAs(value))
                    continue;
                naming(base()->name(planned.column),
                       [&]
                       {
                           m_tracker.edits(planned.column).check(value);
                           checkWritable(planned.column, std::span<const std::uint32_t>(&row, 1));
                       });
                writes.push_back(Write{planned.column, row, std::move(value)});
            }
        }
    }
    result.matchedRows = matchedRows;
    for (Write& write : writes)
        setCell(write.column, write.row, write.value);
    result.cellsChanged = writes.size();
    for (const Planned& planned : columns)
    {
        const auto touched = std::any_of(writes.begin(), writes.end(),
                                         [&](const Write& write) { return write.column == planned.column; });
        if (touched)
            result.columnsUpdated.push_back(base()->name(planned.column));
    }
    return result;
}

} // namespace oor::query
