#include "oor/model/cards/loads/accelFieldLoad/AccelFieldLoadTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

// The pairs of the table start on the second line.
constexpr int FIRST_PAIR_FIELD = 9;

} // namespace

std::optional<AccelFieldLoadCard> AccelFieldLoadFormat::read(FieldReader& reader)
{
    AccelFieldLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.frameId = reader.integer(2, "frameId", 0);
    card.directionX = reader.real(3, "directionX", 0.0);
    card.directionY = reader.real(4, "directionY", 0.0);
    card.directionZ = reader.real(5, "directionZ", 0.0);
    card.direction = readRequiredKeyword<AccelFieldLoadDirection>(reader, 6, "direction", ACCEL_FIELD_DIRECTION_WORDS);
    requireBlankFields(reader, {7, 8}, reader.lastUsedField());
    for (int field = FIRST_PAIR_FIELD; field <= reader.lastUsedField(); field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue; // the padding of a line
        const double location = reader.requiredReal(field, "location");
        const double value = reader.requiredReal(field + 1, "acceleration");
        card.points.emplace_back(location, value);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AccelFieldLoadFormat::check(const AccelFieldLoadCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.frameId, "frameId"))
        return problem;
    if (const auto problem = checkReal(card.directionX, "directionX", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.directionY, "directionY", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.directionZ, "directionZ", RealRule::Finite))
        return problem;
    if (card.directionX == 0.0 && card.directionY == 0.0 && card.directionZ == 0.0)
        return CardProblem{"card.bad-value", "at least one component of the acceleration vector must not be zero",
                           "directionX"};
    if (card.points.size() < 2)
        return CardProblem{"card.missing-field",
                           "the card needs at least two pairs of " + std::string(CardProblem::FIELD) + " and " +
                               CardProblem::fieldNamed("acceleration"),
                           "location"};
    for (const auto& [location, value] : card.points)
    {
        if (const auto problem = checkReal(location, "location", RealRule::Finite))
            return problem;
        if (const auto problem = checkReal(value, "acceleration", RealRule::Finite))
            return problem;
    }
    return std::nullopt;
}

void AccelFieldLoadFormat::write(const AccelFieldLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.frameId, 0);
    writer.realUnlessDefault(3, card.directionX, 0.0);
    writer.realUnlessDefault(4, card.directionY, 0.0);
    writer.realUnlessDefault(5, card.directionZ, 0.0);
    writeKeyword(writer, 6, ACCEL_FIELD_DIRECTION_WORDS, card.direction);
    for (std::size_t index = 0; index < card.points.size(); ++index)
    {
        const int field = FIRST_PAIR_FIELD + 2 * static_cast<int>(index);
        writer.real(field, card.points[index].first);
        writer.real(field + 1, card.points[index].second);
    }
}

namespace
{

std::vector<std::array<double, 2>> toArrays(const std::vector<std::pair<double, double>>& points)
{
    std::vector<std::array<double, 2>> arrays;
    arrays.reserve(points.size());
    for (const auto& [location, value] : points)
        arrays.push_back({location, value});
    return arrays;
}

} // namespace

std::uint32_t AccelFieldLoadTable::append(const AccelFieldLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_frameId.push(card.frameId);
    m_directionX.push(card.directionX);
    m_directionY.push(card.directionY);
    m_directionZ.push(card.directionZ);
    m_direction.push(static_cast<std::uint8_t>(card.direction));
    m_points.push(toArrays(card.points));
    return row;
}

AccelFieldLoadCard AccelFieldLoadTable::card(std::uint32_t row) const
{
    AccelFieldLoadCard card;
    card.id = m_id.get(row);
    card.frameId = m_frameId.get(row);
    card.directionX = m_directionX.get(row);
    card.directionY = m_directionY.get(row);
    card.directionZ = m_directionZ.get(row);
    card.direction = static_cast<AccelFieldLoadDirection>(m_direction.get(row));
    for (const std::array<double, 2>& point : m_points.get(row))
        card.points.emplace_back(point[0], point[1]);
    return card;
}

void AccelFieldLoadTable::store(std::uint32_t row, const AccelFieldLoadCard& card)
{
    m_id.set(row, card.id);
    m_frameId.set(row, card.frameId);
    m_directionX.set(row, card.directionX);
    m_directionY.set(row, card.directionY);
    m_directionZ.set(row, card.directionZ);
    m_direction.set(row, static_cast<std::uint8_t>(card.direction));
    m_points.set(row, toArrays(card.points));
}

void AccelFieldLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_frameId.keepRows(rows);
    m_directionX.keepRows(rows);
    m_directionY.keepRows(rows);
    m_directionZ.keepRows(rows);
    m_direction.keepRows(rows);
    m_points.keepRows(rows);
}

void AccelFieldLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The basic coordinate system (0) is not visited.
    if (const base::EntityId frameId = m_frameId.get(row); frameId > 0)
        m_frameId.set(row, visitor.visit("frameId", ReferenceTarget::Coordinate, frameId));
}

} // namespace oor::model
