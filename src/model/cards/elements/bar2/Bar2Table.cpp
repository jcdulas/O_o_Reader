#include "oor/model/cards/elements/bar2/Bar2Table.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/elements/BarOffsetKinds.hpp"
#include "oor/model/cards/elements/bar2Defaults/Bar2DefaultsTable.hpp"
#include "oor/model/core/Model.hpp"

#include <variant>

namespace oor::model
{

namespace
{

// A pin flag is a set of the digits 1 to 6 with no repeat ("456"), or 0 for no pin.
bool isPinFlag(std::int32_t digits)
{
    if (digits == 0)
        return true;
    if (digits < 0)
        return false;
    bool seen[7] = {};
    for (std::int32_t rest = digits; rest > 0; rest /= 10)
    {
        const std::int32_t digit = rest % 10;
        if (digit < 1 || digit > 6 || seen[digit])
            return false;
        seen[digit] = true;
    }
    return true;
}

} // namespace

std::optional<Bar2Card> Bar2Format::read(FieldReader& reader)
{
    Bar2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId =
        reader.integer(2, "propId", card.id); // format reference: PID defaults to EID (unless a BAROR says otherwise)
    card.nodeA = reader.requiredInteger(3, "nodeA");
    card.nodeB = reader.requiredInteger(4, "nodeB");
    // Field 5 is G0 (an integer) or X1 (a real); a blank field 5 is the vector form too.
    // Fields 6 and 7 are only X2 and X3 in the vector form.
    const auto orientation = reader.integerOrReal(5, "orientation1/orientationNode");
    if (orientation && std::holds_alternative<std::int32_t>(*orientation))
    {
        card.orientationNode = std::get<std::int32_t>(*orientation);
        if (card.orientationNode <= 0)
            reader.error("card.bad-value", std::string("orientationNode") + " must be a positive grid point ID",
                         "orientationNode");
    }
    else
    {
        card.orientation[0] = orientation ? std::get<double>(*orientation) : 0.0;
        card.orientation[1] = reader.real(6, "orientation2", 0.0);
        card.orientation[2] = reader.real(7, "orientation3", 0.0);
    }
    if (const std::string_view offsetKind = reader.text(8); !offsetKind.empty())
        card.offsetKind = std::string(offsetKind);
    card.pinFlagsA = base::sortedDigits(reader.integer(9, "pinFlagsA", 0));
    card.pinFlagsB = base::sortedDigits(reader.integer(10, "pinFlagsB", 0));
    for (int index = 0; index < 3; ++index)
    {
        static constexpr const char* NAMES_A[3] = {"offset1A", "offset2A", "offset3A"};
        static constexpr const char* NAMES_B[3] = {"offset1B", "offset2B", "offset3B"};
        card.offsetA[static_cast<std::size_t>(index)] = reader.real(11 + index, NAMES_A[index], 0.0);
        card.offsetB[static_cast<std::size_t>(index)] = reader.real(14 + index, NAMES_B[index], 0.0);
    }
    // Which of the fields a BAROR gives a default to were blank (a zero is not blank).
    if (reader.isBlank(2))
        card.blankFields |= Bar2Card::BLANK_PROP_ID;
    if (reader.isBlank(5))
        card.blankFields |= Bar2Card::BLANK_ORIENTATION1;
    if (reader.isBlank(6))
        card.blankFields |= Bar2Card::BLANK_ORIENTATION2;
    if (reader.isBlank(7))
        card.blankFields |= Bar2Card::BLANK_ORIENTATION3;
    if (reader.isBlank(8))
        card.blankFields |= Bar2Card::BLANK_OFFSET_KIND;
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> Bar2Format::check(const Bar2Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "propId"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    if (card.nodeA <= 0 || card.nodeB <= 0)
        return CardProblem{"card.bad-id", "the grid point IDs must be positive integers", "nodeA"};
    if (card.nodeA == card.nodeB)
        return CardProblem{"card.duplicate-node", "{field:nodeA} and {field} must be different", "nodeB"};
    if (card.orientationNode < 0)
        return CardProblem{"card.bad-value", "{field} must be a positive grid point ID", "orientationNode"};
    if (card.orientationNode == card.nodeA || card.orientationNode == card.nodeB)
        return CardProblem{"card.duplicate-node", "{field} cannot be {field:nodeA} or {field:nodeB}",
                           "orientationNode"};
    if (!isBarOffsetKind(card.offsetKind))
        return CardProblem{
            "card.bad-value",
            "{field} must be an offset kind such as OffsetsGlobalGlobalGlobal or OffsetsBasicGlobalOrient (or an "
            "old code: 1, 2, 21, 22, 41, 42)",
            "offsetKind"};
    if (!isPinFlag(card.pinFlagsA))
        return CardProblem{"card.bad-value", "{field} must be digits 1 to 6 with no repeat", "pinFlagsA"};
    if (!isPinFlag(card.pinFlagsB))
        return CardProblem{"card.bad-value", "{field} must be digits 1 to 6 with no repeat", "pinFlagsB"};
    return std::nullopt;
}

void Bar2Format::write(const Bar2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integer(3, card.nodeA);
    writer.integer(4, card.nodeB);
    if (card.usesGridPoint())
    {
        writer.integer(5, card.orientationNode);
    }
    else
    {
        // With a BAROR in the model, a blank field would take its default when the deck is read again, so the vector is
        // written as it is, zeros included. Otherwise a zero vector is written as blanks.
        if (writer.governedFieldsExplicit())
        {
            writer.real(5, card.orientation[0]);
            writer.real(6, card.orientation[1]);
            writer.real(7, card.orientation[2]);
        }
        else
        {
            writer.realUnlessDefault(5, card.orientation[0], 0.0);
            writer.realUnlessDefault(6, card.orientation[1], 0.0);
            writer.realUnlessDefault(7, card.orientation[2], 0.0);
        }
    }
    if (writer.governedFieldsExplicit())
        writer.text(8, card.offsetKind);
    else
        writer.textUnlessDefault(8, card.offsetKind, Bar2Card::DEFAULT_OFFSET_KIND);
    writer.integerUnlessDefault(9, card.pinFlagsA, 0);
    writer.integerUnlessDefault(10, card.pinFlagsB, 0);
    for (int index = 0; index < 3; ++index)
    {
        writer.realUnlessDefault(11 + index, card.offsetA[static_cast<std::size_t>(index)], 0.0);
        writer.realUnlessDefault(14 + index, card.offsetB[static_cast<std::size_t>(index)], 0.0);
    }
}

std::uint32_t Bar2Table::append(const Bar2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodeA.push(card.nodeA);
    m_nodeB.push(card.nodeB);
    m_orientation.push(card.orientation);
    m_orientationNode.push(card.orientationNode);
    m_offsetKind.push(card.offsetKind);
    m_pinFlagsA.push(card.pinFlagsA);
    m_pinFlagsB.push(card.pinFlagsB);
    m_offsetA.push(card.offsetA);
    m_offsetB.push(card.offsetB);
    m_blankFields.push(card.blankFields);
    return row;
}

Bar2Card Bar2Table::card(std::uint32_t row) const
{
    return Bar2Card{m_id.get(row),         m_propId.get(row),      m_nodeA.get(row),
                       m_nodeB.get(row),      m_orientation.get(row), m_orientationNode.get(row),
                       m_offsetKind.get(row), m_pinFlagsA.get(row),   m_pinFlagsB.get(row),
                       m_offsetA.get(row),    m_offsetB.get(row),     m_blankFields.get(row)};
}

void Bar2Table::store(std::uint32_t row, const Bar2Card& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodeA.set(row, card.nodeA);
    m_nodeB.set(row, card.nodeB);
    m_orientation.set(row, card.orientation);
    m_orientationNode.set(row, card.orientationNode);
    m_offsetKind.set(row, card.offsetKind);
    m_pinFlagsA.set(row, card.pinFlagsA);
    m_pinFlagsB.set(row, card.pinFlagsB);
    m_offsetA.set(row, card.offsetA);
    m_offsetB.set(row, card.offsetB);
    m_blankFields.set(row, card.blankFields);
}

void Bar2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodeA.keepRows(rows);
    m_nodeB.keepRows(rows);
    m_orientation.keepRows(rows);
    m_orientationNode.keepRows(rows);
    m_offsetKind.keepRows(rows);
    m_pinFlagsA.keepRows(rows);
    m_pinFlagsB.keepRows(rows);
    m_offsetA.keepRows(rows);
    m_offsetB.keepRows(rows);
    m_blankFields.keepRows(rows);
}

void Bar2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    m_nodeA.set(row, visitor.visit("nodeA", ReferenceTarget::Node, m_nodeA.get(row)));
    m_nodeB.set(row, visitor.visit("nodeB", ReferenceTarget::Node, m_nodeB.get(row)));
    if (const base::EntityId orientationNode = m_orientationNode.get(row); orientationNode > 0)
        m_orientationNode.set(row, visitor.visit("orientationNode", ReferenceTarget::Node, orientationNode));
}

bool Bar2Table::fieldsGovernedByDefaults(const Model& model) const
{
    const Bar2DefaultsTable* defaults = model.findTable<Bar2DefaultsTable>();
    return defaults != nullptr && defaults->rows().aliveCount() > 0;
}

void Bar2Table::finishRead(Model& model, const ReadContext&)
{
    if (!m_blankFields.isMaterialised())
        return;
    // The defaults of the (only) BAROR, for the fields that were blank on the card. Without a BAROR the blank fields
    // stay noted: one added later (and Model::applyDefaults) gives them their defaults.
    const Bar2DefaultsTable* defaults = model.findTable<Bar2DefaultsTable>();
    if (defaults == nullptr || defaults->rows().aliveCount() == 0)
        return;
    for (std::uint32_t first = 0; first < defaults->rows().rowCount(); ++first)
    {
        if (!defaults->rows().isAlive(first))
            continue;
        const Bar2DefaultsCard set = defaults->card(first);
        for (std::uint32_t row = 0; row < m_rows.rowCount(); ++row)
        {
            const std::uint8_t blank = m_blankFields.get(row);
            if (blank == 0)
                continue;
            if ((blank & Bar2Card::BLANK_PROP_ID) != 0 && set.propId > 0)
                m_propId.set(row, set.propId);
            if ((blank & Bar2Card::BLANK_OFFSET_KIND) != 0)
                m_offsetKind.set(row, set.offsetKind);
            if (m_orientationNode.get(row) != 0)
                continue; // the card gave a grid point of its own: the vector defaults do not apply
            std::array<double, 3> vector = m_orientation.get(row);
            if ((blank & Bar2Card::BLANK_ORIENTATION1) != 0 && set.usesGridPoint())
            {
                // A blank X1/G0 takes the grid point of the BAROR; the vector fields of the card are then unused.
                m_orientationNode.set(row, set.orientationNode);
                m_orientation.set(row, std::array<double, 3>{});
                continue;
            }
            if ((blank & Bar2Card::BLANK_ORIENTATION1) != 0)
                vector[0] = set.orientation[0];
            if ((blank & Bar2Card::BLANK_ORIENTATION2) != 0)
                vector[1] = set.orientation[1];
            if ((blank & Bar2Card::BLANK_ORIENTATION3) != 0)
                vector[2] = set.orientation[2];
            m_orientation.set(row, vector);
        }
        break; // only one BAROR counts (Bar2DefaultsTable::finishRead reports a second one)
    }
    // The defaults are applied: the cards no longer remember what was blank.
    m_blankFields = Column<std::uint8_t>(0);
    m_blankFields.resize(m_rows.rowCount());
}

} // namespace oor::model
