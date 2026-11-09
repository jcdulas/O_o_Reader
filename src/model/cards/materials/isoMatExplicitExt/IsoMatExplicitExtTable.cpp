#include "oor/model/cards/materials/isoMatExplicitExt/IsoMatExplicitExtTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

std::optional<IsoMatExplicitExtCard> IsoMatExplicitExtFormat::read(FieldReader& reader)
{
    IsoMatExplicitExtCard card;
    card.id = reader.requiredInteger(1, "id");
    card.equationOfStateId = reader.requiredInteger(2, "equationOfStateId");
    requireBlankFields(reader, {}, 2);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> IsoMatExplicitExtFormat::check(const IsoMatExplicitExtCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (card.equationOfStateId <= 0)
        return CardProblem{
            "card.bad-id",
            std::string(CardProblem::FIELD) + " must be the ID of an equation of state, a positive integer",
            "equationOfStateId"};
    return std::nullopt;
}

void IsoMatExplicitExtFormat::write(const IsoMatExplicitExtCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.equationOfStateId);
}

std::uint32_t IsoMatExplicitExtTable::append(const IsoMatExplicitExtCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_equationOfStateId.push(card.equationOfStateId);
    return row;
}

IsoMatExplicitExtCard IsoMatExplicitExtTable::card(std::uint32_t row) const
{
    IsoMatExplicitExtCard card;
    card.id = m_id.get(row);
    card.equationOfStateId = m_equationOfStateId.get(row);
    return card;
}

void IsoMatExplicitExtTable::store(std::uint32_t row, const IsoMatExplicitExtCard& card)
{
    m_id.set(row, card.id);
    m_equationOfStateId.set(row, card.equationOfStateId);
}

void IsoMatExplicitExtTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_equationOfStateId.keepRows(rows);
}

void IsoMatExplicitExtTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT1 that the card extends.
    m_id.set(row, visitor.visit("id", ReferenceTarget::Material, m_id.get(row)));
    if (const base::EntityId equationOfState = m_equationOfStateId.get(row); equationOfState > 0)
        m_equationOfStateId.set(row,
                                visitor.visit("equationOfStateId", ReferenceTarget::EquationOfState, equationOfState));
}

} // namespace oor::model
