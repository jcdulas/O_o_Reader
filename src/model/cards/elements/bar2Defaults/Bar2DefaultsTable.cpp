#include "oor/model/cards/elements/bar2Defaults/Bar2DefaultsTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/elements/BarOffsetKinds.hpp"
#include "oor/model/core/Model.hpp"

#include <variant>

namespace oor::model
{

std::optional<Bar2DefaultsCard> Bar2DefaultsFormat::read(FieldReader& reader)
{
    Bar2DefaultsCard card;
    card.propId = reader.integer(2, "propId", 0);
    // Field 5 is G0 (an integer) or X1 (a real); a blank field 5 gives no orientation. Fields 6 and 7 are X2 and X3 of
    // the vector form.
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
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> Bar2DefaultsFormat::check(const Bar2DefaultsCard& card)
{
    if (card.propId < 0)
        return CardProblem{"card.bad-id", "{field} must be 0 (blank) or a property ID", "propId"};
    if (card.orientationNode < 0)
        return CardProblem{"card.bad-value", "{field} must be a positive grid point ID", "orientationNode"};
    if (card.orientationNode != 0 &&
        (card.orientation[0] != 0.0 || card.orientation[1] != 0.0 || card.orientation[2] != 0.0))
        return CardProblem{"card.bad-value", "give the orientation as a vector or as a grid point, not both",
                           "orientationNode"};
    if (!isBarOffsetKind(card.offsetKind))
        return CardProblem{
            "card.bad-value",
            "{field} must be an offset kind such as OffsetsGlobalGlobalGlobal or OffsetsBasicGlobalOrient (or an "
            "old code: 1, 2, 21, 22, 41, 42)",
            "offsetKind"};
    return std::nullopt;
}

void Bar2DefaultsFormat::write(const Bar2DefaultsCard& card, FieldWriter& writer)
{
    writer.integerUnlessDefault(2, card.propId, 0);
    if (card.usesGridPoint())
    {
        writer.integer(5, card.orientationNode);
    }
    else
    {
        writer.realUnlessDefault(5, card.orientation[0], 0.0);
        writer.realUnlessDefault(6, card.orientation[1], 0.0);
        writer.realUnlessDefault(7, card.orientation[2], 0.0);
    }
    writer.textUnlessDefault(8, card.offsetKind, Bar2DefaultsCard::DEFAULT_OFFSET_KIND);
}

std::uint32_t Bar2DefaultsTable::append(const Bar2DefaultsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_propId.push(card.propId);
    m_orientation.push(card.orientation);
    m_orientationNode.push(card.orientationNode);
    m_offsetKind.push(card.offsetKind);
    return row;
}

Bar2DefaultsCard Bar2DefaultsTable::card(std::uint32_t row) const
{
    return Bar2DefaultsCard{m_propId.get(row), m_orientation.get(row), m_orientationNode.get(row),
                               m_offsetKind.get(row)};
}

void Bar2DefaultsTable::store(std::uint32_t row, const Bar2DefaultsCard& card)
{
    m_propId.set(row, card.propId);
    m_orientation.set(row, card.orientation);
    m_orientationNode.set(row, card.orientationNode);
    m_offsetKind.set(row, card.offsetKind);
}

void Bar2DefaultsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_propId.keepRows(rows);
    m_orientation.keepRows(rows);
    m_orientationNode.keepRows(rows);
    m_offsetKind.keepRows(rows);
}

void Bar2DefaultsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    if (const base::EntityId orientationNode = m_orientationNode.get(row); orientationNode > 0)
        m_orientationNode.set(row, visitor.visit("orientationNode", ReferenceTarget::Node, orientationNode));
}

void Bar2DefaultsTable::finishRead(Model&, const ReadContext& context)
{
    if (m_rows.aliveCount() > 1)
        context.sink.error("deck.duplicate-bar-elem-defaults",
                           "only one " + std::string(NAME) + " may appear in the Bulk Data; the first one is used");
}

} // namespace oor::model
