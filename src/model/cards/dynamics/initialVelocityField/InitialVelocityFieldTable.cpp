#include "oor/model/cards/dynamics/initialVelocityField/InitialVelocityFieldTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_GRID_FIELD = 17;
constexpr ContactRunRules RULES{true, false}; // THRU and BY are allowed; a range goes upwards

} // namespace

std::optional<InitialVelocityFieldCard> InitialVelocityFieldFormat::read(FieldReader& reader)
{
    InitialVelocityFieldCard card;
    card.id = reader.requiredInteger(1, "id");
    card.node = reader.requiredInteger(2, "node");
    card.scaleFactor = reader.real(4, "scaleFactor", 1.0);
    card.xVelocity = reader.real(9, "xVelocity", 0.0);
    card.yVelocity = reader.real(10, "yVelocity", 0.0);
    card.zVelocity = reader.real(11, "zVelocity", 0.0);
    card.xRotation = reader.real(12, "xRotation", 0.0);
    card.yRotation = reader.real(13, "yRotation", 0.0);
    card.zRotation = reader.real(14, "zRotation", 0.0);
    requireBlankFields(reader, {3, 5, 6, 7, 8, 15, 16}, reader.lastUsedField());
    card.grids = readContactRuns(reader, FIRST_GRID_FIELD, reader.lastUsedField(), "node1", RULES);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> InitialVelocityFieldFormat::check(const InitialVelocityFieldCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.node, "node"))
        return problem;
    if (const auto problem = checkReal(card.scaleFactor, "scaleFactor", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.xVelocity, "xVelocity", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.yVelocity, "yVelocity", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.zVelocity, "zVelocity", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.xRotation, "xRotation", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.yRotation, "yRotation", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.zRotation, "zRotation", RealRule::Finite))
        return problem;
    if (card.grids.empty())
        return CardProblem{"card.missing-field", "the card needs the grid points to initialize", "node1"};
    return checkContactRuns(card.grids, "node1", RULES);
}

void InitialVelocityFieldFormat::write(const InitialVelocityFieldCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.node);
    writer.realUnlessDefault(4, card.scaleFactor, 1.0);
    writer.realUnlessDefault(9, card.xVelocity, 0.0);
    writer.realUnlessDefault(10, card.yVelocity, 0.0);
    writer.realUnlessDefault(11, card.zVelocity, 0.0);
    writer.realUnlessDefault(12, card.xRotation, 0.0);
    writer.realUnlessDefault(13, card.yRotation, 0.0);
    writer.realUnlessDefault(14, card.zRotation, 0.0);
    writeContactRuns(writer, FIRST_GRID_FIELD, card.grids);
}

std::uint32_t InitialVelocityFieldTable::append(const InitialVelocityFieldCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void InitialVelocityFieldTable::store(std::uint32_t row, const InitialVelocityFieldCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void InitialVelocityFieldTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void InitialVelocityFieldTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void InitialVelocityFieldTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    InitialVelocityFieldCard& card = m_cards.edit(row);
    visitId(visitor, "node", ReferenceTarget::Node, card.node);
    visitContactRuns(card.grids, visitor, "node", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
