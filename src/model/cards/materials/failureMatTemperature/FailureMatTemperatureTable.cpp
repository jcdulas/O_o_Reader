#include "oor/model/cards/materials/failureMatTemperature/FailureMatTemperatureTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int LINE = 8; // fields to a line
constexpr int FIELD_TSB = 3;
constexpr int FIRST_BLOCK_FIELD = 9;
constexpr int BLOCK_FIELDS = 3 * LINE; // the three lines of a KIND
constexpr int FIRST_TABLE_OFFSET = 2;  // the tables follow KIND and CRITERIA

bool blockHasValues(const FieldReader& reader, int first)
{
    for (int field = first; field < first + BLOCK_FIELDS; ++field)
    {
        if (!reader.isBlank(field))
            return true;
    }
    return false;
}

} // namespace

std::optional<FailureMatTemperatureCard> FailureMatTemperatureFormat::read(FieldReader& reader)
{
    FailureMatTemperatureCard card;
    card.id = reader.requiredInteger(1, "id");
    card.bondShearAllowableTableId = reader.integer(FIELD_TSB, "bondShearAllowableTableId", 0);
    for (int field = 2; field < FIRST_BLOCK_FIELD; ++field)
    {
        if (field != FIELD_TSB && !reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of a " + std::string(reader.cardName()) +
                             " is not used and must be blank",
                         "");
            return card;
        }
    }
    const int last = reader.lastUsedField();
    for (int first = FIRST_BLOCK_FIELD; first <= last && !reader.failed(); first += BLOCK_FIELDS)
    {
        if (!blockHasValues(reader, first))
            continue;
        FailureMatTemperatureBlock block;
        block.kind = reader.requiredInteger(first, "kind");
        block.criterion = reader.requiredInteger(first + 1, "criterion");
        for (std::size_t index = 0; index < block.tables.size(); ++index)
            block.tables[index] =
                reader.integer(first + FIRST_TABLE_OFFSET + static_cast<int>(index), "failurePropertyTableId", 0);
        card.blocks.push_back(block);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FailureMatTemperatureFormat::check(const FailureMatTemperatureCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (const auto problem = checkTableReference(card.bondShearAllowableTableId, "bondShearAllowableTableId"))
        return problem;
    for (const FailureMatTemperatureBlock& block : card.blocks)
    {
        if (block.kind <= 0)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must be a positive integer (12 is the temperature)",
                               "kind"};
        if (block.criterion <= 0)
            return CardProblem{
                "card.bad-value",
                std::string(CardProblem::FIELD) + " must be the criterion of the failure material, a positive integer",
                "criterion"};
        for (const base::EntityId table : block.tables)
        {
            if (const auto problem = checkTableReference(table, "failurePropertyTableId"))
                return problem;
        }
    }
    return std::nullopt;
}

void FailureMatTemperatureFormat::write(const FailureMatTemperatureCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(FIELD_TSB, card.bondShearAllowableTableId, 0);
    int first = FIRST_BLOCK_FIELD;
    for (const FailureMatTemperatureBlock& block : card.blocks)
    {
        writer.integer(first, block.kind);
        writer.integer(first + 1, block.criterion);
        for (std::size_t index = 0; index < block.tables.size(); ++index)
            writer.integerUnlessDefault(first + FIRST_TABLE_OFFSET + static_cast<int>(index), block.tables[index], 0);
        first += BLOCK_FIELDS;
    }
}

std::uint32_t FailureMatTemperatureTable::append(const FailureMatTemperatureCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void FailureMatTemperatureTable::store(std::uint32_t row, const FailureMatTemperatureCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void FailureMatTemperatureTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void FailureMatTemperatureTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void FailureMatTemperatureTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MATF that the card extends; the tables are TABLEMi cards.
    FailureMatTemperatureCard& card = m_cards.edit(row);
    card.id = visitor.visit("id", ReferenceTarget::Material, card.id);
    m_id.set(row, card.id);
    const auto visitTable = [&](base::EntityId& table, const char* field)
    {
        if (table != 0)
            table = visitor.visit(field, ReferenceTarget::MaterialTable, table);
    };
    visitTable(card.bondShearAllowableTableId, "bondShearAllowableTableId");
    for (FailureMatTemperatureBlock& block : card.blocks)
    {
        for (base::EntityId& table : block.tables)
            visitTable(table, "failurePropertyTableId");
    }
}

} // namespace oor::model
