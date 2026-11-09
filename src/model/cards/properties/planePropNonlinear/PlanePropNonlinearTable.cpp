#include "oor/model/cards/properties/planePropNonlinear/PlanePropNonlinearTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

std::optional<PlanePropNonlinearCard> PlanePropNonlinearFormat::read(FieldReader& reader)
{
    PlanePropNonlinearCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.frameId = reader.integer(3, "frameId", 0);
    if (const std::string_view location = reader.text(4); (location == "Gauss"))
        card.gaussOutput = true;
    else if (!location.empty() && !(location == "Node"))
        reader.error(
            "card.bad-value",
            std::string("stressLocation") + " must be GAUS, GRID or blank, found \"" + std::string(location) + "\"",
            "stressLocation");
    if (reader.lastUsedField() > 4)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has four fields", "stressLocation");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PlanePropNonlinearFormat::check(const PlanePropNonlinearCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.matId <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "matId"};
    return std::nullopt;
}

void PlanePropNonlinearFormat::write(const PlanePropNonlinearCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.integerUnlessDefault(3, card.frameId, 0);
    writer.textUnlessDefault(4, card.gaussOutput ? "Gauss" : "Node", "Node");
}

std::uint32_t PlanePropNonlinearTable::append(const PlanePropNonlinearCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_frameId.push(card.frameId);
    m_gaussOutput.push(card.gaussOutput ? 1 : 0);
    return row;
}

PlanePropNonlinearCard PlanePropNonlinearTable::card(std::uint32_t row) const
{
    return PlanePropNonlinearCard{m_id.get(row), m_matId.get(row), m_frameId.get(row), m_gaussOutput.get(row) != 0};
}

void PlanePropNonlinearTable::store(std::uint32_t row, const PlanePropNonlinearCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_frameId.set(row, card.frameId);
    m_gaussOutput.set(row, card.gaussOutput ? 1 : 0);
}

void PlanePropNonlinearTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_frameId.keepRows(rows);
    m_gaussOutput.keepRows(rows);
}

void PlanePropNonlinearTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
    // A negative CID is an axes combination, not a system; zero is the basic system.
    if (const base::EntityId frameId = m_frameId.get(row); frameId > 0)
        m_frameId.set(row, visitor.visit("frameId", ReferenceTarget::Coordinate, frameId));
}

} // namespace oor::model
