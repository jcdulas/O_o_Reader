#include "oor/model/cards/explicit/eulerHydrostaticPreset/EulerHydrostaticPresetTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<EulerHydrostaticPresetCard> EulerHydrostaticPresetFormat::read(FieldReader& reader)
{
    EulerHydrostaticPresetCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.nodeId = reader.integer(3, "nodeId", 0);
    card.frameId = reader.integer(4, "frameId", 0);
    card.surfacePointX = reader.optionalReal(5, "surfacePointX");
    card.surfacePointY = reader.optionalReal(6, "surfacePointY");
    card.surfacePointZ = reader.optionalReal(7, "surfacePointZ");
    card.surfacePressure = reader.requiredReal(8, "surfacePressure");
    if (reader.lastUsedField() > 8)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 8 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> EulerHydrostaticPresetFormat::check(const EulerHydrostaticPresetCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.matId, "matId"))
        return problem;
    if (const auto problem = checkOptionalId(card.nodeId, "nodeId"))
        return problem;
    if (const auto problem = checkOptionalId(card.frameId, "frameId"))
        return problem;
    if (const auto problem = checkOptionalReal(card.surfacePointX, "surfacePointX", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.surfacePointY, "surfacePointY", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.surfacePointZ, "surfacePointZ", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.surfacePressure, "surfacePressure", RealRule::Finite))
        return problem;

    // the free surface is placed by a grid point or by coordinates, not by both.
    const bool hasCoordinates = card.surfacePointX || card.surfacePointY || card.surfacePointZ;
    if (card.nodeId > 0)
    {
        if (card.frameId != 0)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must be blank when " +
                                   CardProblem::fieldNamed("nodeId") + " gives the free surface",
                               "frameId"};
        if (card.surfacePointX)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must be blank when " +
                                   CardProblem::fieldNamed("nodeId") + " gives the free surface",
                               "surfacePointX"};
        if (card.surfacePointY)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must be blank when " +
                                   CardProblem::fieldNamed("nodeId") + " gives the free surface",
                               "surfacePointY"};
        if (card.surfacePointZ)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must be blank when " +
                                   CardProblem::fieldNamed("nodeId") + " gives the free surface",
                               "surfacePointZ"};
        return std::nullopt;
    }
    if (!hasCoordinates)
        return CardProblem{"card.missing-field",
                           "the free surface needs " + std::string(CardProblem::FIELD) + " or the coordinates " +
                               CardProblem::fieldNamed("surfacePointX") + ", " +
                               CardProblem::fieldNamed("surfacePointY") + " and " +
                               CardProblem::fieldNamed("surfacePointZ"),
                           "nodeId"};
    return std::nullopt;
}

void EulerHydrostaticPresetFormat::write(const EulerHydrostaticPresetCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.integerUnlessDefault(3, card.nodeId, 0);
    writer.integerUnlessDefault(4, card.frameId, 0);
    if (card.surfacePointX)
        writer.real(5, *card.surfacePointX);
    if (card.surfacePointY)
        writer.real(6, *card.surfacePointY);
    if (card.surfacePointZ)
        writer.real(7, *card.surfacePointZ);
    writer.real(8, card.surfacePressure);
}

std::uint32_t EulerHydrostaticPresetTable::append(const EulerHydrostaticPresetCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void EulerHydrostaticPresetTable::store(std::uint32_t row, const EulerHydrostaticPresetCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void EulerHydrostaticPresetTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void EulerHydrostaticPresetTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void EulerHydrostaticPresetTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    EulerHydrostaticPresetCard& card = m_cards.edit(row);
    visitId(visitor, "matId", ReferenceTarget::Material, card.matId);
    visitId(visitor, "nodeId", ReferenceTarget::Node, card.nodeId);
    visitId(visitor, "frameId", ReferenceTarget::Coordinate, card.frameId);
}

} // namespace oor::model
