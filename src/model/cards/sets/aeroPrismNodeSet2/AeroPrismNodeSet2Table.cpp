#include "oor/model/cards/sets/aeroPrismNodeSet2/AeroPrismNodeSet2Table.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

std::optional<AeroPrismNodeSet2Card> AeroPrismNodeSet2Format::read(FieldReader& reader)
{
    AeroPrismNodeSet2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.aeroPanelId = reader.requiredInteger(2, "aeroPanelId");
    card.spanStart = reader.requiredReal(3, "spanStart");
    card.spanEnd = reader.requiredReal(4, "spanEnd");
    card.chordStart = reader.requiredReal(5, "chordStart");
    card.chordEnd = reader.requiredReal(6, "chordEnd");
    card.zMaximum = reader.real(7, "zMaximum", 0.0);
    card.zMinimum = reader.real(8, "zMinimum", 0.0);
    if (reader.lastUsedField() > 8)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has eight fields", "zMinimum");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroPrismNodeSet2Format::check(const AeroPrismNodeSet2Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the set ID must be a positive integer", "id"};
    if (card.aeroPanelId <= 0)
        return CardProblem{"card.bad-id", "the macro element ID must be a positive integer", "aeroPanelId"};
    if (!std::isfinite(card.spanStart))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number", "spanStart"};
    if (!std::isfinite(card.spanEnd))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number", "spanEnd"};
    if (!std::isfinite(card.chordStart))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                           "chordStart"};
    if (!std::isfinite(card.chordEnd))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number", "chordEnd"};
    if (!std::isfinite(card.zMaximum))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number", "zMaximum"};
    if (!std::isfinite(card.zMinimum))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number", "zMinimum"};
    return std::nullopt;
}

void AeroPrismNodeSet2Format::write(const AeroPrismNodeSet2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.aeroPanelId);
    writer.real(3, card.spanStart);
    writer.real(4, card.spanEnd);
    writer.real(5, card.chordStart);
    writer.real(6, card.chordEnd);
    // 0.0 means "no limit", which is what a blank field means too.
    writer.realUnlessDefault(7, card.zMaximum, 0.0);
    writer.realUnlessDefault(8, card.zMinimum, 0.0);
}

std::uint32_t AeroPrismNodeSet2Table::append(const AeroPrismNodeSet2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_aeroPanelId.push(card.aeroPanelId);
    m_spanStart.push(card.spanStart);
    m_spanEnd.push(card.spanEnd);
    m_chordStart.push(card.chordStart);
    m_chordEnd.push(card.chordEnd);
    m_zMaximum.push(card.zMaximum);
    m_zMinimum.push(card.zMinimum);
    return row;
}

AeroPrismNodeSet2Card AeroPrismNodeSet2Table::card(std::uint32_t row) const
{
    return AeroPrismNodeSet2Card{m_id.get(row),       m_aeroPanelId.get(row), m_spanStart.get(row),
                                 m_spanEnd.get(row),  m_chordStart.get(row),  m_chordEnd.get(row),
                                 m_zMaximum.get(row), m_zMinimum.get(row)};
}

void AeroPrismNodeSet2Table::store(std::uint32_t row, const AeroPrismNodeSet2Card& card)
{
    m_id.set(row, card.id);
    m_aeroPanelId.set(row, card.aeroPanelId);
    m_spanStart.set(row, card.spanStart);
    m_spanEnd.set(row, card.spanEnd);
    m_chordStart.set(row, card.chordStart);
    m_chordEnd.set(row, card.chordEnd);
    m_zMaximum.set(row, card.zMaximum);
    m_zMinimum.set(row, card.zMinimum);
}

void AeroPrismNodeSet2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_aeroPanelId.keepRows(rows);
    m_spanStart.keepRows(rows);
    m_spanEnd.keepRows(rows);
    m_chordStart.keepRows(rows);
    m_chordEnd.keepRows(rows);
    m_zMaximum.keepRows(rows);
    m_zMinimum.keepRows(rows);
}

void AeroPrismNodeSet2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The macro element is a CAERO1 to CAERO5, in the space of the aerodynamic elements.
    m_aeroPanelId.set(row, visitor.visit("aeroPanelId", ReferenceTarget::AeroElement, m_aeroPanelId.get(row)));
}

} // namespace oor::model
