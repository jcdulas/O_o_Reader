#include "oor/model/cards/tables/tableMultiVariable1/TableMultiVariable1Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/tables/TableClosureSupport.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <utility>

namespace oor::model
{

namespace
{

constexpr int MAX_VARIABLES = static_cast<int>(TableMultiVariable1Card::MAX_VARIABLES);

// Where the fields of the header are. The four variables share them: KINDi, EXTRPi and SMi are consecutive fields, the
// NWi are fields 7 to 10, and ITIDSi and ITIDBi alternate from field 17.
constexpr int VARIABLE_COUNT_FIELD = 2;
constexpr int FIRST_KIND_FIELD = 3;
constexpr std::array<int, MAX_VARIABLES> VALUE_COUNT_FIELDS{7, 8, 9, 10};
constexpr int FIRST_EXTRAPOLATION_FIELD = 11;
constexpr int FIRST_BELOW_RANGE_FIELD = 17;
constexpr int FIRST_SMOOTHING_FIELD = 25;
// The header has four lines (32 fields): the first list of X values is on the fifth.
constexpr int FIRST_DATA_FIELD = 33;

// The names the diagnostics and the problems report: string literals, one per variable.
constexpr std::array<const char*, MAX_VARIABLES> KIND_NAMES{"kind1", "kind2", "kind3", "kind4"};
constexpr std::array<const char*, MAX_VARIABLES> VALUE_COUNT_NAMES{"valueCount1", "valueCount2", "valueCount3",
                                                                   "valueCount4"};
constexpr std::array<const char*, MAX_VARIABLES> EXTRAPOLATION_NAMES{"extrapolation1", "extrapolation2",
                                                                     "extrapolation3", "extrapolation4"};
constexpr std::array<const char*, MAX_VARIABLES> BELOW_RANGE_NAMES{"belowRangeTableId1", "belowRangeTableId2",
                                                                   "belowRangeTableId3", "belowRangeTableId4"};
constexpr std::array<const char*, MAX_VARIABLES> ABOVE_RANGE_NAMES{"aboveRangeTableId1", "aboveRangeTableId2",
                                                                   "aboveRangeTableId3", "aboveRangeTableId4"};
constexpr std::array<const char*, MAX_VARIABLES> SMOOTHING_NAMES{"smoothing1", "smoothing2", "smoothing3",
                                                                 "smoothing4"};
constexpr std::array<const char*, MAX_VARIABLES> VARIABLE_VALUES_NAMES{"variableValues1", "variableValues2",
                                                                       "variableValues3", "variableValues4"};

// The header fields of the variable in `slot` (0 to 3). The ones of a variable beyond NV must be blank, or hold the
// default of an EXTRP and an SM (the same thing), and the NWi of the variables that are there are kept in
// `valueCounts` for the lists that follow.
void readVariableHeader(FieldReader& reader, int slot, int variableCount, TableMultiVariable1Card& card,
                        std::array<int, MAX_VARIABLES>& valueCounts)
{
    const auto index = static_cast<std::size_t>(slot);
    const int kind = reader.integer(FIRST_KIND_FIELD + slot, KIND_NAMES[index], 0);
    const int count = reader.integer(VALUE_COUNT_FIELDS[index], VALUE_COUNT_NAMES[index], 0);
    const int extrapolation =
        reader.integer(FIRST_EXTRAPOLATION_FIELD + slot, EXTRAPOLATION_NAMES[index], TABLE_DEFAULT_EXTRAPOLATION);
    const base::EntityId belowRangeTableId =
        reader.integer(FIRST_BELOW_RANGE_FIELD + 2 * slot, BELOW_RANGE_NAMES[index], 0);
    const base::EntityId aboveRangeTableId =
        reader.integer(FIRST_BELOW_RANGE_FIELD + 2 * slot + 1, ABOVE_RANGE_NAMES[index], 0);
    const int smoothing = reader.integer(FIRST_SMOOTHING_FIELD + slot, SMOOTHING_NAMES[index], 0);
    if (slot < variableCount)
    {
        if (count <= 0)
            reader.error("card.bad-value",
                         std::string(VALUE_COUNT_NAMES[index]) + " must be greater than 0 for each of the " +
                             std::string("variableCount") + " variables",
                         VALUE_COUNT_NAMES[index]);
        TableMultiVariable1Variable variable;
        variable.kind = kind;
        variable.extrapolation = extrapolation;
        variable.belowRangeTableId = belowRangeTableId;
        variable.aboveRangeTableId = aboveRangeTableId;
        variable.smoothing = smoothing;
        card.variables.push_back(std::move(variable));
        valueCounts[index] = count;
        return;
    }
    if (kind != 0 || count != 0 || extrapolation != TABLE_DEFAULT_EXTRAPOLATION || belowRangeTableId != 0 ||
        aboveRangeTableId != 0 || smoothing != 0)
        reader.error("card.bad-value",
                     "variable " + std::to_string(slot + 1) + " is given but " + std::string("variableCount") + " is " +
                         std::to_string(variableCount),
                     KIND_NAMES[index]);
}

} // namespace

std::optional<TableMultiVariable1Card> TableMultiVariable1Format::read(FieldReader& reader)
{
    TableMultiVariable1Card card;
    card.id = reader.requiredInteger(1, "id");
    const int variableCount = reader.requiredInteger(VARIABLE_COUNT_FIELD, "variableCount");
    if (reader.failed())
        return card;
    if (variableCount < 1 || variableCount > MAX_VARIABLES)
    {
        reader.error("card.bad-value", std::string("variableCount") + " must be 1, 2, 3 or 4", "variableCount");
        return card;
    }
    std::array<int, MAX_VARIABLES> valueCounts{};
    for (int slot = 0; slot < MAX_VARIABLES; ++slot)
        readVariableHeader(reader, slot, variableCount, card, valueCounts);
    for (const int field : {15, 16})
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value", "fields 15 and 16 of a " + std::string(reader.cardName()) + " are not used",
                         "extrapolation4");
    }
    for (const int field : {29, 30, 31, 32})
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value", "fields 29 to 32 of a " + std::string(reader.cardName()) + " are not used",
                         "smoothing4");
    }
    if (reader.failed())
        return card;

    // The lists: each variable's X values start on a line of their own, then the Y values on the next one.
    const int last = reader.lastUsedField();
    int field = FIRST_DATA_FIELD;
    for (int slot = 0; slot < variableCount; ++slot)
    {
        const auto index = static_cast<std::size_t>(slot);
        const int count = valueCounts[index];
        if (count > last - field + 1)
        {
            reader.error("card.missing-field",
                         std::string(VALUE_COUNT_NAMES[index]) + " says there are more values of " +
                             std::string(VARIABLE_VALUES_NAMES[index]) + " than the card has",
                         VARIABLE_VALUES_NAMES[index]);
            return card;
        }
        std::vector<double>& values = card.variables[index].x;
        for (int offset = 0; offset < count; ++offset)
            values.push_back(reader.requiredReal(field + offset, VARIABLE_VALUES_NAMES[index]));
        const int next = tableLineStartAtOrAfter(field + count);
        for (int gap = field + count; gap < next; ++gap)
        {
            if (!reader.isBlank(gap))
                reader.error("card.bad-value", "the values of the next list start on a new line of the card",
                             slot + 1 < variableCount ? VARIABLE_VALUES_NAMES[index + 1] : "ordinate");
        }
        field = next;
    }
    if (reader.failed())
        return card;
    // The values of Y are one per point of the grid and fill the rest of the card. The product stops growing once it
    // is more than the fields there are, so it cannot overflow.
    std::int64_t gridPoints = 1;
    for (int slot = 0; slot < variableCount && gridPoints <= last; ++slot)
        gridPoints *= valueCounts[static_cast<std::size_t>(slot)];
    const int available = last - field + 1;
    if (gridPoints != available)
    {
        reader.error("card.bad-value",
                     "the card has " + std::to_string(available > 0 ? available : 0) + " values of " +
                         std::string("ordinate") + " and the grid of the X lists has " + std::to_string(gridPoints) +
                         " points",
                     "ordinate");
        return card;
    }
    for (int offset = 0; offset < available && !reader.failed(); ++offset)
        card.ordinate.push_back(reader.requiredReal(field + offset, "ordinate"));
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TableMultiVariable1Format::check(const TableMultiVariable1Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.variables.empty() || card.variables.size() > TableMultiVariable1Card::MAX_VARIABLES)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1, 2, 3 or 4",
                           "variableCount"};
    std::size_t gridPoints = 1;
    for (std::size_t slot = 0; slot < card.variables.size(); ++slot)
    {
        const TableMultiVariable1Variable& variable = card.variables[slot];
        if (const auto problem = checkIndependentVariableKind(variable.kind, KIND_NAMES[slot]))
            return problem;
        if (variable.x.empty())
            return CardProblem{"card.missing-field",
                               CardProblem::fieldNamed(VARIABLE_VALUES_NAMES[slot]) + " needs at least one value",
                               VALUE_COUNT_NAMES[slot]};
        if (const auto problem = checkExtrapolationFlag(variable.extrapolation, EXTRAPOLATION_NAMES[slot]))
            return problem;
        if (const auto problem = checkSmoothingFlag(variable.smoothing, SMOOTHING_NAMES[slot]))
            return problem;
        if (const auto problem = checkFiniteTableValues(variable.x, VARIABLE_VALUES_NAMES[slot]))
            return problem;
        if (gridPoints <= card.ordinate.size())
            gridPoints *= variable.x.size();
    }
    if (gridPoints != card.ordinate.size())
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " needs one value for each point of the grid of the X lists", "ordinate"};
    return checkFiniteTableValues(card.ordinate, "ordinate");
}

void TableMultiVariable1Format::write(const TableMultiVariable1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(VARIABLE_COUNT_FIELD, static_cast<std::int64_t>(card.variables.size()));
    int field = FIRST_DATA_FIELD;
    for (std::size_t index = 0; index < card.variables.size() && index < TableMultiVariable1Card::MAX_VARIABLES;
         ++index)
    {
        const TableMultiVariable1Variable& variable = card.variables[index];
        const int slot = static_cast<int>(index);
        writer.integer(FIRST_KIND_FIELD + slot, variable.kind);
        writer.integer(VALUE_COUNT_FIELDS[index], static_cast<std::int64_t>(variable.x.size()));
        writer.integerUnlessDefault(FIRST_EXTRAPOLATION_FIELD + slot, variable.extrapolation,
                                    TABLE_DEFAULT_EXTRAPOLATION);
        writer.integerUnlessDefault(FIRST_BELOW_RANGE_FIELD + 2 * slot, variable.belowRangeTableId, 0);
        writer.integerUnlessDefault(FIRST_BELOW_RANGE_FIELD + 2 * slot + 1, variable.aboveRangeTableId, 0);
        writer.integerUnlessDefault(FIRST_SMOOTHING_FIELD + slot, variable.smoothing, 0);
        for (const double value : variable.x)
            writer.real(field++, value);
        field = tableLineStartAtOrAfter(field);
    }
    for (const double value : card.ordinate)
        writer.real(field++, value);
}

std::uint32_t TableMultiVariable1Table::append(const TableMultiVariable1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void TableMultiVariable1Table::store(std::uint32_t row, const TableMultiVariable1Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void TableMultiVariable1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void TableMultiVariable1Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
