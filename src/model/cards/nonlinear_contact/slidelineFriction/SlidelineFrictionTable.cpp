#include "oor/model/cards/nonlinear_contact/slidelineFriction/SlidelineFrictionTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <cmath>

namespace oor::model
{

std::optional<SlidelineFrictionCard> SlidelineFrictionFormat::read(FieldReader& reader)
{
    SlidelineFrictionCard card;
    card.id = reader.requiredInteger(1, "id");
    // Field 2 is blank in the Card layout, and so is everything after MU1.
    requireUnusedBlank(reader, {2}, 4);
    card.stickStiffness = reader.real(3, "stickStiffness", 0.0);
    card.staticFriction = reader.requiredReal(4, "staticFriction");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SlidelineFrictionFormat::check(const SlidelineFrictionCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (!(card.stickStiffness >= 0.0) || !std::isfinite(card.stickStiffness))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be positive, or blank to let the program choose it",
                           "stickStiffness"};
    if (!(card.staticFriction > 0.0) || !std::isfinite(card.staticFriction))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "staticFriction"};
    return std::nullopt;
}

void SlidelineFrictionFormat::write(const SlidelineFrictionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(3, card.stickStiffness, 0.0);
    writer.real(4, card.staticFriction);
}

std::uint32_t SlidelineFrictionTable::append(const SlidelineFrictionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_stickStiffness.push(card.stickStiffness);
    m_staticFriction.push(card.staticFriction);
    return row;
}

SlidelineFrictionCard SlidelineFrictionTable::card(std::uint32_t row) const
{
    SlidelineFrictionCard card;
    card.id = m_id.get(row);
    card.stickStiffness = m_stickStiffness.get(row);
    card.staticFriction = m_staticFriction.get(row);
    return card;
}

void SlidelineFrictionTable::store(std::uint32_t row, const SlidelineFrictionCard& card)
{
    m_id.set(row, card.id);
    m_stickStiffness.set(row, card.stickStiffness);
    m_staticFriction.set(row, card.staticFriction);
}

void SlidelineFrictionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_stickStiffness.keepRows(rows);
    m_staticFriction.keepRows(rows);
}

} // namespace oor::model
