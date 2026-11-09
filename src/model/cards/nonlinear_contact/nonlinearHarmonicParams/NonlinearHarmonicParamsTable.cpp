#include "oor/model/cards/nonlinear_contact/nonlinearHarmonicParams/NonlinearHarmonicParamsTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

namespace oor::model
{

std::optional<NonlinearHarmonicParamsCard> NonlinearHarmonicParamsFormat::read(FieldReader& reader)
{
    NonlinearHarmonicParamsCard card;
    card.id = reader.requiredInteger(1, "id");
    card.subharmonicFactor = reader.integer(2, "subharmonicFactor", 1);
    card.harmonicCount = reader.requiredInteger(3, "harmonicCount");
    card.freqListId = reader.requiredInteger(4, "freqListId");
    requireUnusedBlank(reader, {5, 6, 7, 8}, 4);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonlinearHarmonicParamsFormat::check(const NonlinearHarmonicParamsCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.subharmonicFactor < 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1 or more",
                           "subharmonicFactor"};
    if (card.harmonicCount <= 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "harmonicCount"};
    if (card.freqListId <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "freqListId"};
    return std::nullopt;
}

void NonlinearHarmonicParamsFormat::write(const NonlinearHarmonicParamsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.subharmonicFactor, 1);
    writer.integer(3, card.harmonicCount);
    writer.integer(4, card.freqListId);
}

std::uint32_t NonlinearHarmonicParamsTable::append(const NonlinearHarmonicParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_subharmonicFactor.push(card.subharmonicFactor);
    m_harmonicCount.push(card.harmonicCount);
    m_freqListId.push(card.freqListId);
    return row;
}

NonlinearHarmonicParamsCard NonlinearHarmonicParamsTable::card(std::uint32_t row) const
{
    NonlinearHarmonicParamsCard card;
    card.id = m_id.get(row);
    card.subharmonicFactor = m_subharmonicFactor.get(row);
    card.harmonicCount = m_harmonicCount.get(row);
    card.freqListId = m_freqListId.get(row);
    return card;
}

void NonlinearHarmonicParamsTable::store(std::uint32_t row, const NonlinearHarmonicParamsCard& card)
{
    m_id.set(row, card.id);
    m_subharmonicFactor.set(row, card.subharmonicFactor);
    m_harmonicCount.set(row, card.harmonicCount);
    m_freqListId.set(row, card.freqListId);
}

void NonlinearHarmonicParamsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_subharmonicFactor.keepRows(rows);
    m_harmonicCount.keepRows(rows);
    m_freqListId.keepRows(rows);
}

void NonlinearHarmonicParamsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_freqListId.set(row, visitor.visit("freqListId", ReferenceTarget::NonlinearFrequency, m_freqListId.get(row)));
}

} // namespace oor::model
