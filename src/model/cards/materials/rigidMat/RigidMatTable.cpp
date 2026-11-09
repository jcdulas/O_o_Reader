#include "oor/model/cards/materials/rigidMat/RigidMatTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

// The fields of the card: eight to a line, the inertia on the second, the velocities on the third, the local centre on
// the fourth.
constexpr int FIELD_MASS = 5;
constexpr int FIELD_CENTER = 6;
constexpr int FIELD_INERTIA = 9;
constexpr int FIELD_FRAME = 15;
constexpr int FIELD_VELOCITY = 17;
constexpr int FIELD_ROTATION = 20;
constexpr int FIELD_LOCAL_CENTER = 25;

template <std::size_t N>
void readOptionalReals(FieldReader& reader, int firstField, const char* name,
                       std::array<std::optional<double>, N>& values)
{
    for (std::size_t index = 0; index < N; ++index)
        values[index] = reader.optionalReal(firstField + static_cast<int>(index), name);
}

template <std::size_t N>
void writeOptionalReals(FieldWriter& writer, int firstField, const std::array<std::optional<double>, N>& values)
{
    for (std::size_t index = 0; index < N; ++index)
    {
        if (values[index])
            writer.real(firstField + static_cast<int>(index), *values[index]);
    }
}

template <std::size_t N>
bool allFinite(const std::array<std::optional<double>, N>& values)
{
    for (const std::optional<double>& value : values)
    {
        if (value && !std::isfinite(*value))
            return false;
    }
    return true;
}

template <std::size_t N>
bool anyGiven(const std::array<std::optional<double>, N>& values)
{
    for (const std::optional<double>& value : values)
    {
        if (value)
            return true;
    }
    return false;
}

} // namespace

std::optional<RigidMatCard> RigidMatFormat::read(FieldReader& reader)
{
    RigidMatCard card;
    card.id = reader.requiredInteger(1, "id");
    card.density = reader.real(2, "density", 1.0);
    card.youngsModulus = reader.real(3, "youngsModulus", 1.0);
    card.poissonRatio = reader.real(4, "poissonRatio", 0.2);
    card.mass = reader.optionalReal(FIELD_MASS, "mass");
    readOptionalReals(reader, FIELD_CENTER, "centerX", card.center);
    readOptionalReals(reader, FIELD_INERTIA, "inertiaXx", card.inertia);
    card.frameId = reader.integer(FIELD_FRAME, "frameId", 0);
    readOptionalReals(reader, FIELD_VELOCITY, "velocityX", card.velocity);
    readOptionalReals(reader, FIELD_ROTATION, "angularVelocityX", card.rotation);
    readOptionalReals(reader, FIELD_LOCAL_CENTER, "localCenterX", card.localCenter);
    requireBlankFields(reader, {16, 23, 24, 28, 29, 30, 31, 32}, FIELD_LOCAL_CENTER + 2);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RigidMatFormat::check(const RigidMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (!(card.density > 0.0) || !std::isfinite(card.density))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero", "density"};
    if (!(card.youngsModulus > 0.0) || !std::isfinite(card.youngsModulus))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "youngsModulus"};
    if (!(card.poissonRatio >= 0.0 && card.poissonRatio < 0.5))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be from 0.0 up to, and not including, 0.5",
                           "poissonRatio"};
    // format reference: a mass of zero is a blank one (the mass is computed from the mesh).
    if (card.mass && (!(*card.mass >= 0.0) || !std::isfinite(*card.mass)))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "mass"};
    if (!allFinite(card.center) || !allFinite(card.inertia) || !allFinite(card.velocity) || !allFinite(card.rotation) ||
        !allFinite(card.localCenter))
        return CardProblem{"card.bad-value", "a coordinate, an inertia or a velocity must be a finite number",
                           "centerX"};
    if (card.frameId < 0)
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) + " must be the ID of a coordinate system, or blank",
                           "frameId"};
    // the centre of gravity is in the basic system or in the system CID, not both.
    if (anyGiven(card.center) && anyGiven(card.localCenter))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + ", " + CardProblem::fieldNamed("centerY") + " and " +
                               CardProblem::fieldNamed("centerZ") + " are blank when " +
                               CardProblem::fieldNamed("localCenterX") + ", " +
                               CardProblem::fieldNamed("localCenterY") + " and " +
                               CardProblem::fieldNamed("localCenterZ") + " are given",
                           "centerX"};
    return std::nullopt;
}

void RigidMatFormat::write(const RigidMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.density, 1.0);
    writer.realUnlessDefault(3, card.youngsModulus, 1.0);
    writer.realUnlessDefault(4, card.poissonRatio, 0.2);
    if (card.mass)
        writer.real(FIELD_MASS, *card.mass);
    writeOptionalReals(writer, FIELD_CENTER, card.center);
    writeOptionalReals(writer, FIELD_INERTIA, card.inertia);
    writer.integerUnlessDefault(FIELD_FRAME, card.frameId, 0);
    writeOptionalReals(writer, FIELD_VELOCITY, card.velocity);
    writeOptionalReals(writer, FIELD_ROTATION, card.rotation);
    writeOptionalReals(writer, FIELD_LOCAL_CENTER, card.localCenter);
}

std::uint32_t RigidMatTable::append(const RigidMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void RigidMatTable::store(std::uint32_t row, const RigidMatCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void RigidMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void RigidMatTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void RigidMatTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    RigidMatCard& card = m_cards.edit(row);
    if (card.frameId != 0)
        card.frameId = visitor.visit("frameId", ReferenceTarget::Coordinate, card.frameId);
}

} // namespace oor::model
