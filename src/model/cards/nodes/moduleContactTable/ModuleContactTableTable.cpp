#include "oor/model/cards/nodes/moduleContactTable/ModuleContactTableTable.hpp"

namespace oor::model
{

namespace
{

// The pairs follow BCID from field 2, a module and an ID each, without regard to the lines.
constexpr int FIRST_PAIR_FIELD = 2;

constexpr int moduleField(std::size_t index) noexcept
{
    return FIRST_PAIR_FIELD + 2 * static_cast<int>(index);
}

} // namespace

std::optional<ModuleContactTableCard> ModuleContactTableFormat::read(FieldReader& reader)
{
    ModuleContactTableCard card;
    card.id = reader.requiredInteger(1, "id");
    bool gap = false; // a pair with both fields blank, seen before the pair that is being read
    for (std::size_t index = 0; moduleField(index) <= reader.lastUsedField(); ++index)
    {
        const std::optional<ModuleId> pair =
            readModuleId(reader, moduleField(index), moduleField(index) + 1, "module", "contactPairId");
        if (!pair)
        {
            gap = true;
            continue;
        }
        if (gap)
        {
            reader.error("card.bad-value",
                         "blank fields between the pairs of an " + std::string(reader.cardName()) + " are not allowed",
                         "contactPairId");
            break;
        }
        card.pairs.push_back(*pair);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleContactTableFormat::check(const ModuleContactTableCard& card)
{
    if (card.id < 0)
        return CardProblem{"card.bad-id", "{field} must not be negative", "id"};
    if (card.pairs.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one pair of " + CardProblem::fieldNamed("module") + " and {field}",
                           "contactPairId"};
    for (const ModuleId& pair : card.pairs)
    {
        if (const auto problem = checkModuleId(pair, "module", "contactPairId"))
            return problem;
    }
    return std::nullopt;
}

void ModuleContactTableFormat::write(const ModuleContactTableCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    // The module is written even when it is 0: the format allows no blank field between the pairs.
    for (std::size_t index = 0; index < card.pairs.size(); ++index)
        writeModuleId(writer, moduleField(index), moduleField(index) + 1, card.pairs[index], true);
}

std::uint32_t ModuleContactTableTable::append(const ModuleContactTableCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleContactTableTable::store(std::uint32_t row, const ModuleContactTableCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleContactTableTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleContactTableTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleContactTableTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (ModuleId& pair : m_cards.edit(row).pairs)
        visitModuleId(visitor, "contactPairId", ReferenceTarget::ContactPair, pair);
}

} // namespace oor::model
