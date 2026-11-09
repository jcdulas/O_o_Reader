#include "oor/model/cards/aero/aeroMachBoxPanel3/AeroMachBoxPanel3Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 16;

} // namespace

std::optional<AeroMachBoxPanel3Card> AeroMachBoxPanel3Format::read(FieldReader& reader)
{
    AeroMachBoxPanel3Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    card.positionFrameId = reader.integer(3, "positionFrameId", 0);
    card.wingPointListId = reader.requiredInteger(4, "wingPointListId");
    card.controlPointListId1 = reader.integer(5, "controlPointListId1", 0);
    card.controlPointListId2 = reader.integer(6, "controlPointListId2", 0);
    card.point1 = {reader.real(9, "xCoord1", 0.0), reader.real(10, "yCoord1", 0.0), reader.real(11, "zCoord1", 0.0)};
    card.rootChord = reader.requiredReal(12, "rootChord");
    card.point4 = {reader.real(13, "xCoord4", 0.0), reader.real(14, "yCoord4", 0.0), reader.real(15, "zCoord4", 0.0)};
    card.tipChord = reader.real(16, "tipChord", 0.0);
    // Fields 7 and 8 are blank cells of the layout, and the card ends with X43.
    requireBlankFields(reader, {7, 8}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroMachBoxPanel3Format::check(const AeroMachBoxPanel3Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    if (const auto problem = checkOptionalId(card.positionFrameId, "positionFrameId"))
        return problem;
    if (const auto problem = checkRequiredId(card.wingPointListId, "wingPointListId"))
        return problem;
    if (const auto problem = checkOptionalId(card.controlPointListId1, "controlPointListId1"))
        return problem;
    if (const auto problem = checkOptionalId(card.controlPointListId2, "controlPointListId2"))
        return problem;
    for (const double value : card.point1)
    {
        if (!std::isfinite(value))
            return CardProblem{"card.bad-value", "{field}, {field:yCoord1} and {field:zCoord1} must be finite numbers",
                               "xCoord1"};
    }
    for (const double value : card.point4)
    {
        if (!std::isfinite(value))
            return CardProblem{"card.bad-value", "{field}, {field:yCoord4} and {field:zCoord4} must be finite numbers",
                               "xCoord4"};
    }
    // The chords are lengths: the inboard one is not zero, the outboard one may be (a triangle).
    if (const auto problem = checkReal(card.rootChord, "rootChord", RealRule::Positive))
        return problem;
    return checkReal(card.tipChord, "tipChord", RealRule::NonNegative);
}

void AeroMachBoxPanel3Format::write(const AeroMachBoxPanel3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integerUnlessDefault(3, card.positionFrameId, 0);
    writer.integer(4, card.wingPointListId);
    writer.integerUnlessDefault(5, card.controlPointListId1, 0);
    writer.integerUnlessDefault(6, card.controlPointListId2, 0);
    // The continuation line holds the geometry, so its fields are always written, zeros included.
    writer.real(9, card.point1[0]);
    writer.real(10, card.point1[1]);
    writer.real(11, card.point1[2]);
    writer.real(12, card.rootChord);
    writer.real(13, card.point4[0]);
    writer.real(14, card.point4[1]);
    writer.real(15, card.point4[2]);
    writer.real(16, card.tipChord);
}

std::uint32_t AeroMachBoxPanel3Table::append(const AeroMachBoxPanel3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroMachBoxPanel3Table::store(std::uint32_t row, const AeroMachBoxPanel3Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroMachBoxPanel3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroMachBoxPanel3Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AeroMachBoxPanel3Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    AeroMachBoxPanel3Card& card = m_cards.edit(row);
    visitId(visitor, "propId", ReferenceTarget::AeroProperty, card.propId);
    visitId(visitor, "positionFrameId", ReferenceTarget::Coordinate, card.positionFrameId);
    visitId(visitor, "wingPointListId", ReferenceTarget::AeroFactor, card.wingPointListId);
    visitId(visitor, "controlPointListId1", ReferenceTarget::AeroFactor, card.controlPointListId1);
    visitId(visitor, "controlPointListId2", ReferenceTarget::AeroFactor, card.controlPointListId2);
}

} // namespace oor::model
