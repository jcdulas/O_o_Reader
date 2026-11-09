#include "oor/model/cards/nodes/node/NodeTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/model/cards/nodes/nodeDefaults/NodeDefaultsTable.hpp"
#include "oor/model/core/Model.hpp"

namespace oor::model
{

namespace
{

// PS is a list of the digits 1 to 6 with no repeat ("123456", "3", "246").
bool isValidPermanentConstraint(std::int32_t digits)
{
    if (digits == 0)
        return true;
    bool seen[7] = {};
    for (std::int32_t rest = digits; rest > 0; rest /= 10)
    {
        const std::int32_t digit = rest % 10;
        if (digit < 1 || digit > 6 || seen[digit])
            return false;
        seen[digit] = true;
    }
    return digits > 0;
}

} // namespace

std::optional<NodeCard> NodeFormat::read(FieldReader& reader)
{
    NodeCard card;
    card.id = reader.requiredInteger(1, "id");
    card.positionFrameId = reader.integer(2, "positionFrameId", 0);
    card.xyz.x = reader.real(3, "position1", 0.0);
    card.xyz.y = reader.real(4, "position2", 0.0);
    card.xyz.z = reader.real(5, "position3", 0.0);
    card.displacementFrameId = reader.integer(6, "displacementFrameId", 0);
    card.permanentConstraints = base::sortedDigits(reader.integer(7, "permanentConstraints", 0));
    card.superelementId = reader.integer(8, "superelementId", 0);
    // Which of the optional fields were blank (a GRDSET gives them a default); zero is not blank.
    if (reader.isBlank(2))
        card.blankFields |= NodeCard::BLANK_POSITION_FRAME;
    if (reader.isBlank(6))
        card.blankFields |= NodeCard::BLANK_DISPLACEMENT_FRAME;
    if (reader.isBlank(7))
        card.blankFields |= NodeCard::BLANK_PERMANENT_CONSTRAINTS;
    if (reader.isBlank(8))
        card.blankFields |= NodeCard::BLANK_SUPERELEMENT;
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NodeFormat::check(const NodeCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the ID must be a positive integer", "id"};
    if (card.positionFrameId < 0)
        return CardProblem{"card.bad-value", "{field} must be 0 or a coordinate system ID", "positionFrameId"};
    if (card.displacementFrameId < -1)
        return CardProblem{"card.bad-value", "{field} must be -1, 0 or a coordinate system ID", "displacementFrameId"};
    if (!isValidPermanentConstraint(card.permanentConstraints))
        return CardProblem{"card.bad-value", "{field} must be digits 1 to 6 with no repeat", "permanentConstraints"};
    return std::nullopt;
}

void NodeFormat::write(const NodeCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    // With a GRDSET in the model, a blank field would take its default when the deck is read again, so a field that is
    // zero is written as 0 (the format reference: zero overrides the default).
    const bool explicitFields = writer.governedFieldsExplicit();
    if (explicitFields)
        writer.integer(2, card.positionFrameId);
    else
        writer.integerUnlessDefault(2, card.positionFrameId, 0);
    writer.real(3, card.xyz.x);
    writer.real(4, card.xyz.y);
    writer.real(5, card.xyz.z);
    if (explicitFields)
        writer.integer(6, card.displacementFrameId);
    else
        writer.integerUnlessDefault(6, card.displacementFrameId, 0);
    if (explicitFields || card.permanentConstraints != 0)
        writer.integer(7, card.permanentConstraints);
    if (explicitFields)
        writer.integer(8, card.superelementId);
    else
        writer.integerUnlessDefault(8, card.superelementId, 0);
}

std::uint32_t NodeTable::append(const NodeCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_positionFrameId.push(card.positionFrameId);
    m_xyz.push({card.xyz.x, card.xyz.y, card.xyz.z});
    m_displacementFrameId.push(card.displacementFrameId);
    m_permanentConstraints.push(card.permanentConstraints);
    m_superelementId.push(card.superelementId);
    m_blankFields.push(card.blankFields);
    return row;
}

NodeCard NodeTable::card(std::uint32_t row) const
{
    return NodeCard{m_id.get(row),
                    m_positionFrameId.get(row),
                    xyz(row),
                    m_displacementFrameId.get(row),
                    m_permanentConstraints.get(row),
                    m_superelementId.get(row),
                    m_blankFields.get(row)};
}

void NodeTable::store(std::uint32_t row, const NodeCard& card)
{
    if (m_id.get(row) != card.id)
        m_changes.noteIdChange();
    m_changes.noteRow(row);
    m_id.set(row, card.id);
    m_positionFrameId.set(row, card.positionFrameId);
    m_xyz.set(row, {card.xyz.x, card.xyz.y, card.xyz.z});
    m_displacementFrameId.set(row, card.displacementFrameId);
    m_permanentConstraints.set(row, card.permanentConstraints);
    m_superelementId.set(row, card.superelementId);
    m_blankFields.set(row, card.blankFields);
}

base::Vec3 NodeTable::xyz(std::uint32_t row) const
{
    const std::array<double, 3> value = m_xyz.get(row);
    return base::Vec3{value[0], value[1], value[2]};
}

void NodeTable::setXyzComponent(std::uint32_t row, std::size_t component, double value)
{
    m_changes.noteRow(row);
    std::array<double, 3> current = m_xyz.get(row);
    current[component] = value;
    m_xyz.set(row, current);
}

void NodeTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_changes.noteCompaction(rows);
    m_id.keepRows(rows);
    m_positionFrameId.keepRows(rows);
    m_xyz.keepRows(rows);
    m_displacementFrameId.keepRows(rows);
    m_permanentConstraints.keepRows(rows);
    m_superelementId.keepRows(rows);
    m_blankFields.keepRows(rows);
}

void NodeTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId positionFrame = m_positionFrameId.get(row); positionFrame > 0)
    {
        const base::EntityId updated = visitor.visit("positionFrameId", ReferenceTarget::Coordinate, positionFrame);
        if (updated != positionFrame)
            m_changes.noteRow(row);
        m_positionFrameId.set(row, updated);
    }
    if (const base::EntityId displacementFrame = m_displacementFrameId.get(row); displacementFrame > 0)
        m_displacementFrameId.set(row,
                                  visitor.visit("displacementFrameId", ReferenceTarget::Coordinate, displacementFrame));
    // The superelement the grid point is interior to (a deck may define a list superelement by the SEID of its grids).
    if (const base::EntityId superelement = m_superelementId.get(row); superelement > 0)
        m_superelementId.set(row, visitor.visit("superelementId", ReferenceTarget::Superelement, superelement));
}

bool NodeTable::fieldsGovernedByDefaults(const Model& model) const
{
    const NodeDefaultsTable* defaults = model.findTable<NodeDefaultsTable>();
    return defaults != nullptr && defaults->rows().aliveCount() > 0;
}

void NodeTable::finishRead(Model& model, const ReadContext&)
{
    if (!m_blankFields.isMaterialised())
        return;
    // The defaults of the (only) GRDSET, for the fields that were blank on the GRID. Without a GRDSET the blank fields
    // stay noted: one added later (and Model::applyDefaults) gives them their defaults.
    const NodeDefaultsTable* defaults = model.findTable<NodeDefaultsTable>();
    if (defaults == nullptr || defaults->rows().aliveCount() == 0)
        return;
    for (std::uint32_t first = 0; first < defaults->rows().rowCount(); ++first)
    {
        if (!defaults->rows().isAlive(first))
            continue;
        const NodeDefaultsCard set = defaults->card(first);
        for (std::uint32_t row = 0; row < m_rows.rowCount(); ++row)
        {
            const std::uint8_t blank = m_blankFields.get(row);
            if (blank == 0)
                continue;
            if ((blank & NodeCard::BLANK_POSITION_FRAME) != 0)
            {
                m_positionFrameId.set(row, set.positionFrameId);
                m_changes.noteRow(row);
            }
            if ((blank & NodeCard::BLANK_DISPLACEMENT_FRAME) != 0)
                m_displacementFrameId.set(row, set.displacementFrameId);
            if ((blank & NodeCard::BLANK_PERMANENT_CONSTRAINTS) != 0)
                m_permanentConstraints.set(row, set.permanentConstraints);
            if ((blank & NodeCard::BLANK_SUPERELEMENT) != 0)
                m_superelementId.set(row, set.superelementId);
        }
        break; // only one GRDSET counts (NodeDefaultsTable::finishRead reports a second one)
    }
    // The defaults are applied: the grids no longer remember what was blank.
    m_blankFields = Column<std::uint8_t>(0);
    m_blankFields.resize(m_rows.rowCount());
}

} // namespace oor::model
