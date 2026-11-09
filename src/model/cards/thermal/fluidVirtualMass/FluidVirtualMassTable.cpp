#include "oor/model/cards/thermal/fluidVirtualMass/FluidVirtualMassTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <cmath>
#include <limits>

namespace oor::model
{

namespace
{

double notGiven()
{
    return std::numeric_limits<double>::quiet_NaN();
}

} // namespace

FluidVirtualMassTable::FluidVirtualMassTable()
    : m_freeSurfaceZ(notGiven()),
      m_planeSymmetry1("NoSymmetry"),
      m_planeSymmetry2("NoSymmetry"),
      m_characteristicLength(1.0e10),
      m_exactIntegrationRatio(1.0e-15)
{
}

std::optional<FluidVirtualMassCard> FluidVirtualMassFormat::read(FieldReader& reader)
{
    FluidVirtualMassCard card;
    card.id = reader.requiredInteger(1, "id");
    card.frameId = reader.integer(2, "frameId", 0);
    card.freeSurfaceZ = reader.optionalReal(3, "freeSurfaceZ");
    card.density = reader.requiredReal(4, "density");
    card.oneSideElemListId = reader.integer(5, "oneSideElemListId", 0);
    card.bothSidesElemListId = reader.integer(6, "bothSidesElemListId", 0);
    card.planeSymmetry1 = readKeyword(reader, 7, "planeSymmetry1", {"Symmetric", "Antisymmetric", "NoSymmetry"}, "");
    card.planeSymmetry2 = readKeyword(reader, 8, "planeSymmetry2", {"Symmetric", "Antisymmetric", "NoSymmetry"}, "");
    if (!reader.failed() && card.planeSymmetry1.empty())
        reader.error("card.missing-field", std::string("planeSymmetry1") + " (S, A or N) is required",
                     "planeSymmetry1");
    if (!reader.failed() && card.planeSymmetry2.empty())
        reader.error("card.missing-field", std::string("planeSymmetry2") + " (S, A or N) is required",
                     "planeSymmetry2");
    card.characteristicLength = reader.real(9, "characteristicLength", 1.0e10);
    card.exactIntegrationRatio = reader.real(10, "exactIntegrationRatio", 1.0e-15);
    requireUnusedBlank(reader, {}, 10);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FluidVirtualMassFormat::check(const FluidVirtualMassCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the set ID must be a positive integer", "id"};
    if (card.frameId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative", "frameId"};
    if (card.oneSideElemListId < 0 || card.bothSidesElemListId < 0)
        return CardProblem{"card.bad-id", "an element list ID cannot be negative", "oneSideElemListId"};
    if (card.oneSideElemListId + card.bothSidesElemListId <= 0)
        return CardProblem{
            "card.missing-field",
            std::string(CardProblem::FIELD) + " or " + CardProblem::fieldNamed("bothSidesElemListId") + " is required",
            "oneSideElemListId"};
    if (const auto problem =
            checkKeyword(card.planeSymmetry1, "planeSymmetry1", {"Symmetric", "Antisymmetric", "NoSymmetry"}))
        return problem;
    if (const auto problem =
            checkKeyword(card.planeSymmetry2, "planeSymmetry2", {"Symmetric", "Antisymmetric", "NoSymmetry"}))
        return problem;
    if (!(card.characteristicLength > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "characteristicLength"};
    return std::nullopt;
}

void FluidVirtualMassFormat::write(const FluidVirtualMassCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.frameId, 0);
    if (card.freeSurfaceZ)
        writer.real(3, *card.freeSurfaceZ);
    writer.real(4, card.density);
    writer.integerUnlessDefault(5, card.oneSideElemListId, 0);
    writer.integerUnlessDefault(6, card.bothSidesElemListId, 0);
    writer.text(7, card.planeSymmetry1);
    writer.text(8, card.planeSymmetry2);
    writer.realUnlessDefault(9, card.characteristicLength, 1.0e10);
    writer.realUnlessDefault(10, card.exactIntegrationRatio, 1.0e-15);
}

std::uint32_t FluidVirtualMassTable::append(const FluidVirtualMassCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_frameId.push(card.frameId);
    m_freeSurfaceZ.push(card.freeSurfaceZ.value_or(notGiven()));
    m_density.push(card.density);
    m_oneSideElemListId.push(card.oneSideElemListId);
    m_bothSidesElemListId.push(card.bothSidesElemListId);
    m_planeSymmetry1.push(card.planeSymmetry1);
    m_planeSymmetry2.push(card.planeSymmetry2);
    m_characteristicLength.push(card.characteristicLength);
    m_exactIntegrationRatio.push(card.exactIntegrationRatio);
    return row;
}

FluidVirtualMassCard FluidVirtualMassTable::card(std::uint32_t row) const
{
    FluidVirtualMassCard card;
    card.id = m_id.get(row);
    card.frameId = m_frameId.get(row);
    if (const double freeSurfaceZ = m_freeSurfaceZ.get(row); !std::isnan(freeSurfaceZ))
        card.freeSurfaceZ = freeSurfaceZ;
    card.density = m_density.get(row);
    card.oneSideElemListId = m_oneSideElemListId.get(row);
    card.bothSidesElemListId = m_bothSidesElemListId.get(row);
    card.planeSymmetry1 = m_planeSymmetry1.get(row);
    card.planeSymmetry2 = m_planeSymmetry2.get(row);
    card.characteristicLength = m_characteristicLength.get(row);
    card.exactIntegrationRatio = m_exactIntegrationRatio.get(row);
    return card;
}

void FluidVirtualMassTable::store(std::uint32_t row, const FluidVirtualMassCard& card)
{
    m_id.set(row, card.id);
    m_frameId.set(row, card.frameId);
    m_freeSurfaceZ.set(row, card.freeSurfaceZ.value_or(notGiven()));
    m_density.set(row, card.density);
    m_oneSideElemListId.set(row, card.oneSideElemListId);
    m_bothSidesElemListId.set(row, card.bothSidesElemListId);
    m_planeSymmetry1.set(row, card.planeSymmetry1);
    m_planeSymmetry2.set(row, card.planeSymmetry2);
    m_characteristicLength.set(row, card.characteristicLength);
    m_exactIntegrationRatio.set(row, card.exactIntegrationRatio);
}

void FluidVirtualMassTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_frameId.keepRows(rows);
    m_freeSurfaceZ.keepRows(rows);
    m_density.keepRows(rows);
    m_oneSideElemListId.keepRows(rows);
    m_bothSidesElemListId.keepRows(rows);
    m_planeSymmetry1.keepRows(rows);
    m_planeSymmetry2.keepRows(rows);
    m_characteristicLength.keepRows(rows);
    m_exactIntegrationRatio.keepRows(rows);
}

void FluidVirtualMassTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId frameId = m_frameId.get(row); frameId > 0)
        m_frameId.set(row, visitor.visit("frameId", ReferenceTarget::Coordinate, frameId));
    if (const base::EntityId listId = m_oneSideElemListId.get(row); listId > 0)
        m_oneSideElemListId.set(row, visitor.visit("oneSideElemListId", ReferenceTarget::ElementList, listId));
    if (const base::EntityId listId = m_bothSidesElemListId.get(row); listId > 0)
        m_bothSidesElemListId.set(row, visitor.visit("bothSidesElemListId", ReferenceTarget::ElementList, listId));
}

} // namespace oor::model
