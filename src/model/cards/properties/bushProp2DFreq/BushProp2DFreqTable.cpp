#include "oor/model/cards/properties/bushProp2DFreq/BushProp2DFreqTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

namespace
{

using TermTables = std::array<base::EntityId, BushProp2DFreqCard::TERM_COUNT>;

constexpr int FIELDS_PER_LINE = 8;
constexpr int TERMS_OFFSET = 2; // the first table of a group is two fields after the first field of its line

// The names of the terms of each group in the order of the format: the diagnostics use the upper case ones and the
// references the lower case ones.
constexpr const char* K_NAMES[BushProp2DFreqCard::TERM_COUNT] = {"stiffnessTableId11", "stiffnessTableId22",
                                                                 "stiffnessTableId12", "stiffnessTableId21"};
constexpr const char* B_NAMES[BushProp2DFreqCard::TERM_COUNT] = {"dampingTableId11", "dampingTableId22",
                                                                 "dampingTableId12", "dampingTableId21"};
constexpr const char* M_NAMES[BushProp2DFreqCard::TERM_COUNT] = {"massTableId11", "massTableId22", "massTableId12",
                                                                 "massTableId21"};
constexpr const char* K_REFERENCES[BushProp2DFreqCard::TERM_COUNT] = {"stiffnessTableId11", "stiffnessTableId22",
                                                                      "stiffnessTableId12", "stiffnessTableId21"};
constexpr const char* B_REFERENCES[BushProp2DFreqCard::TERM_COUNT] = {"dampingTableId11", "dampingTableId22",
                                                                      "dampingTableId12", "dampingTableId21"};
constexpr const char* M_REFERENCES[BushProp2DFreqCard::TERM_COUNT] = {"massTableId11", "massTableId22", "massTableId12",
                                                                      "massTableId21"};

// The fields from `first` to `last` must be blank: the cells of the grid that a group leaves empty.
void requireBlank(FieldReader& reader, int first, int last, const char* name)
{
    for (int field = first; field <= last; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value", "this field of a " + std::string(reader.cardName()) + " must be blank",
                         name);
            return;
        }
    }
}

// Four table IDs from the first field of a group; a blank one is 0.
void readTerms(FieldReader& reader, int first, const char* const (&names)[BushProp2DFreqCard::TERM_COUNT],
               TermTables& tables)
{
    for (std::size_t index = 0; index < BushProp2DFreqCard::TERM_COUNT; ++index)
        tables[index] = reader.integer(first + static_cast<int>(index), names[index], 0);
}

bool anyGiven(const TermTables& tables)
{
    for (const base::EntityId id : tables)
    {
        if (id != 0)
            return true;
    }
    return false;
}

// Writes a group on the line that starts at field `start`: the flag in the second field, the tables after it.
void writeGroup(FieldWriter& writer, int start, const char* flag, const TermTables& tables)
{
    writer.text(start + 1, flag);
    for (std::size_t index = 0; index < BushProp2DFreqCard::TERM_COUNT; ++index)
        writer.integerUnlessDefault(start + TERMS_OFFSET + static_cast<int>(index), tables[index], 0);
}

std::optional<CardProblem> checkTerms(const TermTables& tables,
                                      const char* const (&names)[BushProp2DFreqCard::TERM_COUNT])
{
    for (std::size_t index = 0; index < BushProp2DFreqCard::TERM_COUNT; ++index)
    {
        if (tables[index] < 0)
            return CardProblem{"card.bad-id", "a table ID must be a positive integer or blank", names[index]};
    }
    return std::nullopt;
}

// Visits the tables of a group as TABLED1 entries.
void visitTerms(ReferenceVisitor& visitor, Column<TermTables>& column, std::uint32_t row,
                const char* const (&references)[BushProp2DFreqCard::TERM_COUNT])
{
    TermTables tables = column.get(row);
    bool changed = false;
    for (std::size_t index = 0; index < BushProp2DFreqCard::TERM_COUNT; ++index)
    {
        if (tables[index] <= 0)
            continue;
        const base::EntityId updated = visitor.visit(references[index], ReferenceTarget::DynamicTable, tables[index]);
        changed = changed || updated != tables[index];
        tables[index] = updated;
    }
    if (changed)
        column.set(row, tables);
}

} // namespace

std::optional<BushProp2DFreqCard> BushProp2DFreqFormat::read(FieldReader& reader)
{
    BushProp2DFreqCard card;
    card.id = reader.requiredInteger(1, "id");

    unsigned seen = 0; // one bit per group already read
    for (int start = 1; start <= reader.lastUsedField(); start += FIELDS_PER_LINE)
    {
        if (start > 1 && !reader.isBlank(start))
            reader.error("card.bad-value",
                         "the first field of a continuation line of a " + std::string(reader.cardName()) + " is blank",
                         "id");
        const std::string_view flag = reader.text(start + 1);
        if (flag.empty())
        {
            // A line with no group: only the line of the PID may stand without one, and nothing else is on it.
            requireBlank(reader, start + TERMS_OFFSET, start + FIELDS_PER_LINE - 1, "id");
            continue;
        }
        const auto is = [&](const char* name) { return (flag == name); };
        unsigned bit = 0;
        if (is("Stiffness"))
        {
            bit = 1;
            readTerms(reader, start + TERMS_OFFSET, K_NAMES, card.stiffnessTableIds);
        }
        else if (is("Damping"))
        {
            bit = 2;
            readTerms(reader, start + TERMS_OFFSET, B_NAMES, card.dampingTableIds);
        }
        else if (is("Mass"))
        {
            bit = 4;
            readTerms(reader, start + TERMS_OFFSET, M_NAMES, card.massTableIds);
        }
        else
        {
            reader.error("card.bad-value",
                         "a line of a " + std::string(reader.cardName()) + " starts with K, B or M, found \"" +
                             std::string(flag) + "\"",
                         "id");
            continue;
        }
        // The last two cells of the line are empty in the grid.
        requireBlank(reader, start + TERMS_OFFSET + static_cast<int>(BushProp2DFreqCard::TERM_COUNT),
                     start + FIELDS_PER_LINE - 1, "id");
        if ((seen & bit) != 0)
            reader.error("card.bad-value",
                         "a group of a " + std::string(reader.cardName()) + " may be given only once: \"" +
                             std::string(flag) + "\"",
                         "id");
        seen |= bit;
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BushProp2DFreqFormat::check(const BushProp2DFreqCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (const auto problem = checkTerms(card.stiffnessTableIds, K_NAMES))
        return problem;
    if (const auto problem = checkTerms(card.dampingTableIds, B_NAMES))
        return problem;
    return checkTerms(card.massTableIds, M_NAMES);
}

void BushProp2DFreqFormat::write(const BushProp2DFreqCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int start = 1; // the first field of the line of the next group
    if (anyGiven(card.stiffnessTableIds))
    {
        writeGroup(writer, start, "Stiffness", card.stiffnessTableIds);
        start += FIELDS_PER_LINE;
    }
    if (anyGiven(card.dampingTableIds))
    {
        writeGroup(writer, start, "Damping", card.dampingTableIds);
        start += FIELDS_PER_LINE;
    }
    if (anyGiven(card.massTableIds))
        writeGroup(writer, start, "Mass", card.massTableIds);
}

std::uint32_t BushProp2DFreqTable::append(const BushProp2DFreqCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_stiffnessTableIds.push(card.stiffnessTableIds);
    m_dampingTableIds.push(card.dampingTableIds);
    m_massTableIds.push(card.massTableIds);
    return row;
}

BushProp2DFreqCard BushProp2DFreqTable::card(std::uint32_t row) const
{
    BushProp2DFreqCard card;
    card.id = m_id.get(row);
    card.stiffnessTableIds = m_stiffnessTableIds.get(row);
    card.dampingTableIds = m_dampingTableIds.get(row);
    card.massTableIds = m_massTableIds.get(row);
    return card;
}

void BushProp2DFreqTable::store(std::uint32_t row, const BushProp2DFreqCard& card)
{
    m_id.set(row, card.id);
    m_stiffnessTableIds.set(row, card.stiffnessTableIds);
    m_dampingTableIds.set(row, card.dampingTableIds);
    m_massTableIds.set(row, card.massTableIds);
}

void BushProp2DFreqTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_stiffnessTableIds.keepRows(rows);
    m_dampingTableIds.keepRows(rows);
    m_massTableIds.keepRows(rows);
}

void BushProp2DFreqTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the PBUSH2D that the card extends.
    m_id.set(row, visitor.visit("id", ReferenceTarget::Property, m_id.get(row)));
    visitTerms(visitor, m_stiffnessTableIds, row, K_REFERENCES);
    visitTerms(visitor, m_dampingTableIds, row, B_REFERENCES);
    visitTerms(visitor, m_massTableIds, row, M_REFERENCES);
}

} // namespace oor::model
