#include "oor/model/cards/aero/aeroLiftingPanelProp1/AeroLiftingPanelProp1Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <array>
#include <string>

namespace oor::model
{

namespace
{

constexpr int FIRST_BODY_FIELD = 2;
constexpr int LAST_FIELD = FIRST_BODY_FIELD + static_cast<int>(AeroLiftingPanelProp1Card::BODY_COUNT) - 1;

// A CardProblem keeps a pointer to the name of its field, so the names are literals and not built in the loop.
constexpr std::array<const char*, AeroLiftingPanelProp1Card::BODY_COUNT> BODY_NAMES{"body1", "body2", "body3",
                                                                                    "body4", "body5", "body6"};

} // namespace

std::optional<AeroLiftingPanelProp1Card> AeroLiftingPanelProp1Format::read(FieldReader& reader)
{
    AeroLiftingPanelProp1Card card;
    card.id = reader.requiredInteger(1, "id");
    for (std::size_t index = 0; index < AeroLiftingPanelProp1Card::BODY_COUNT; ++index)
        card.bodies[index] = reader.integer(FIRST_BODY_FIELD + static_cast<int>(index), BODY_NAMES[index], 0);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 7 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroLiftingPanelProp1Format::check(const AeroLiftingPanelProp1Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    for (std::size_t index = 0; index < AeroLiftingPanelProp1Card::BODY_COUNT; ++index)
    {
        if (const auto problem = checkOptionalId(card.bodies[index], BODY_NAMES[index]))
            return problem;
        // The format reference does not allow a blank field between two bodies.
        if (index > 0 && card.bodies[index] != 0 && card.bodies[index - 1] == 0)
            return CardProblem{"card.bad-value", "{field} follows a blank field: the bodies must be consecutive",
                               BODY_NAMES[index]};
    }
    return std::nullopt;
}

void AeroLiftingPanelProp1Format::write(const AeroLiftingPanelProp1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t index = 0; index < AeroLiftingPanelProp1Card::BODY_COUNT; ++index)
    {
        if (card.bodies[index] != 0)
            writer.integer(FIRST_BODY_FIELD + static_cast<int>(index), card.bodies[index]);
    }
}

std::uint32_t AeroLiftingPanelProp1Table::append(const AeroLiftingPanelProp1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_bodies.push(card.bodies);
    return row;
}

AeroLiftingPanelProp1Card AeroLiftingPanelProp1Table::card(std::uint32_t row) const
{
    AeroLiftingPanelProp1Card card;
    card.id = m_id.get(row);
    card.bodies = m_bodies.get(row);
    return card;
}

void AeroLiftingPanelProp1Table::store(std::uint32_t row, const AeroLiftingPanelProp1Card& card)
{
    m_id.set(row, card.id);
    m_bodies.set(row, card.bodies);
}

void AeroLiftingPanelProp1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_bodies.keepRows(rows);
}

void AeroLiftingPanelProp1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::array<base::EntityId, AeroLiftingPanelProp1Card::BODY_COUNT> bodies = m_bodies.get(row);
    for (base::EntityId& body : bodies)
    {
        if (body > 0)
            body = visitor.visit("bodies", ReferenceTarget::AeroElement, body);
    }
    m_bodies.set(row, bodies);
}

} // namespace oor::model
