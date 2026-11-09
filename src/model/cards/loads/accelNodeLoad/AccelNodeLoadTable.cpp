#include "oor/model/cards/loads/accelNodeLoad/AccelNodeLoadTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

// The grid points start on the second line; a run goes up and BY is allowed.
constexpr int FIRST_GRID_FIELD = 9;
constexpr ContactRunRules RULES{true, false};

} // namespace

std::optional<AccelNodeLoadCard> AccelNodeLoadFormat::read(FieldReader& reader)
{
    AccelNodeLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.frameId = reader.integer(2, "frameId", 0);
    card.magnitude = reader.requiredReal(3, "magnitude");
    card.directionX = reader.real(4, "directionX", 0.0);
    card.directionY = reader.real(5, "directionY", 0.0);
    card.directionZ = reader.real(6, "directionZ", 0.0);
    requireBlankFields(reader, {7, 8}, reader.lastUsedField());
    card.grids = readContactRuns(reader, FIRST_GRID_FIELD, reader.lastUsedField(), "nodeId", RULES);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AccelNodeLoadFormat::check(const AccelNodeLoadCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.frameId, "frameId"))
        return problem;
    if (const auto problem = checkReal(card.magnitude, "magnitude", RealRule::Finite))
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
    if (card.grids.empty())
        return CardProblem{"card.missing-field", "the card needs at least one grid point", "nodeId"};
    return checkContactRuns(card.grids, "nodeId", RULES);
}

void AccelNodeLoadFormat::write(const AccelNodeLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.frameId, 0);
    writer.real(3, card.magnitude);
    writer.realUnlessDefault(4, card.directionX, 0.0);
    writer.realUnlessDefault(5, card.directionY, 0.0);
    writer.realUnlessDefault(6, card.directionZ, 0.0);
    writeContactRuns(writer, FIRST_GRID_FIELD, card.grids);
}

std::uint32_t AccelNodeLoadTable::append(const AccelNodeLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_frameId.push(card.frameId);
    m_magnitude.push(card.magnitude);
    m_directionX.push(card.directionX);
    m_directionY.push(card.directionY);
    m_directionZ.push(card.directionZ);
    m_grids.push(toContactTriples(card.grids));
    return row;
}

AccelNodeLoadCard AccelNodeLoadTable::card(std::uint32_t row) const
{
    AccelNodeLoadCard card;
    card.id = m_id.get(row);
    card.frameId = m_frameId.get(row);
    card.magnitude = m_magnitude.get(row);
    card.directionX = m_directionX.get(row);
    card.directionY = m_directionY.get(row);
    card.directionZ = m_directionZ.get(row);
    card.grids = toContactRuns(m_grids.get(row));
    return card;
}

void AccelNodeLoadTable::store(std::uint32_t row, const AccelNodeLoadCard& card)
{
    m_id.set(row, card.id);
    m_frameId.set(row, card.frameId);
    m_magnitude.set(row, card.magnitude);
    m_directionX.set(row, card.directionX);
    m_directionY.set(row, card.directionY);
    m_directionZ.set(row, card.directionZ);
    m_grids.set(row, toContactTriples(card.grids));
}

void AccelNodeLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_frameId.keepRows(rows);
    m_magnitude.keepRows(rows);
    m_directionX.keepRows(rows);
    m_directionY.keepRows(rows);
    m_directionZ.keepRows(rows);
    m_grids.keepRows(rows);
}

void AccelNodeLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The basic coordinate system (0) is not visited.
    if (const base::EntityId frameId = m_frameId.get(row); frameId > 0)
        m_frameId.set(row, visitor.visit("frameId", ReferenceTarget::Coordinate, frameId));
    visitContactTriples(m_grids, row, visitor, "nodeId", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
