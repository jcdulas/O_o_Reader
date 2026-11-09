#include "oor/model/cards/nodes/moduleMirror2/ModuleMirror2Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<ModuleMirror2Card> ModuleMirror2Format::read(FieldReader& reader)
{
    ModuleMirror2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.frameId = reader.integer(2, "frameId", 0);
    card.referenceNode = reader.integer(3, "referenceNode", 0);
    card.axes = readRequiredKeyword<ModuleMirror2Axes>(reader, 4, "axes", MODULE_MIRROR2_AXES_WORDS);
    if (reader.lastUsedField() > 4)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 4 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleMirror2Format::check(const ModuleMirror2Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.frameId, "frameId"))
        return problem;
    return checkOptionalId(card.referenceNode, "referenceNode");
}

void ModuleMirror2Format::write(const ModuleMirror2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.frameId, 0);
    writer.integerUnlessDefault(3, card.referenceNode, 0);
    writeKeyword(writer, 4, MODULE_MIRROR2_AXES_WORDS, card.axes);
}

std::uint32_t ModuleMirror2Table::append(const ModuleMirror2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleMirror2Table::store(std::uint32_t row, const ModuleMirror2Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleMirror2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleMirror2Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleMirror2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleMirror2Card& card = m_cards.edit(row);
    visitId(visitor, "frameId", ReferenceTarget::Coordinate, card.frameId);
    visitId(visitor, "referenceNode", ReferenceTarget::Node, card.referenceNode);
}

} // namespace oor::model
