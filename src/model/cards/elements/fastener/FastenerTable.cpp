#include "oor/model/cards/elements/fastener/FastenerTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

std::optional<FastenerCard> FastenerFormat::read(FieldReader& reader)
{
    static constexpr const char* NAMES[3] = {"locationX", "locationY", "locationZ"};
    FastenerCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.integer(2, "propId", card.id); // format reference: PID defaults to EID
    card.kind = std::string(reader.word(3));
    card.patchIdA = reader.requiredInteger(4, "patchIdA");
    card.patchIdB = reader.requiredInteger(5, "patchIdB");
    card.spotNode = reader.integer(6, "spotNode", 0);
    card.nodeA = reader.integer(7, "nodeA", 0);
    card.nodeB = reader.integer(8, "nodeB", 0);
    // Field 9 is blank: the location starts on the continuation line.
    for (std::size_t index = 0; index < 3; ++index)
        card.location[index] = reader.real(9 + static_cast<int>(index), NAMES[index], FastenerCard::NO_COORDINATE);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FastenerFormat::check(const FastenerCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    if (card.kind != "Property" && card.kind != "Elements")
        return CardProblem{"card.bad-value", "{field} must be Property or Elements", "kind"};
    if (card.patchIdA <= 0 || card.patchIdB <= 0)
        return CardProblem{"card.bad-id", "{field} and {field:patchIdB} must be positive integers", "patchIdA"};
    if (card.spotNode < 0 || card.nodeA < 0 || card.nodeB < 0)
        return CardProblem{"card.bad-id",
                           "{field}, {field:nodeA} and {field:nodeB} must be 0 (blank) or grid point IDs", "spotNode"};
    const bool allLocation =
        !std::isnan(card.location[0]) && !std::isnan(card.location[1]) && !std::isnan(card.location[2]);
    if (card.hasLocation() && !allLocation)
        return CardProblem{"card.missing-field",
                           "{field}, {field:locationY} and {field:locationZ} must be all given or all blank",
                           "locationX"};
    if (card.spotNode == 0 && card.nodeA == 0 && !card.hasLocation())
        return CardProblem{"card.missing-field",
                           "the location is required: {field}, {field:nodeA} or {field:locationX}, {field:locationY} "
                           "and {field:locationZ}",
                           "spotNode"};
    return std::nullopt;
}

void FastenerFormat::write(const FastenerCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.text(3, card.kind);
    writer.integer(4, card.patchIdA);
    writer.integer(5, card.patchIdB);
    writer.integerUnlessDefault(6, card.spotNode, 0);
    writer.integerUnlessDefault(7, card.nodeA, 0);
    writer.integerUnlessDefault(8, card.nodeB, 0);
    if (card.hasLocation())
    {
        for (std::size_t index = 0; index < 3; ++index)
            writer.real(9 + static_cast<int>(index), card.location[index]);
    }
}

std::uint32_t FastenerTable::append(const FastenerCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_kind.push(card.kind);
    m_patchIdA.push(card.patchIdA);
    m_patchIdB.push(card.patchIdB);
    m_spotNode.push(card.spotNode);
    m_nodeA.push(card.nodeA);
    m_nodeB.push(card.nodeB);
    m_location.push(card.location);
    return row;
}

FastenerCard FastenerTable::card(std::uint32_t row) const
{
    return FastenerCard{m_id.get(row),       m_propId.get(row),   m_kind.get(row),
                            m_patchIdA.get(row), m_patchIdB.get(row), m_spotNode.get(row),
                            m_nodeA.get(row),    m_nodeB.get(row),    m_location.get(row)};
}

void FastenerTable::store(std::uint32_t row, const FastenerCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_kind.set(row, card.kind);
    m_patchIdA.set(row, card.patchIdA);
    m_patchIdB.set(row, card.patchIdB);
    m_spotNode.set(row, card.spotNode);
    m_nodeA.set(row, card.nodeA);
    m_nodeB.set(row, card.nodeB);
    m_location.set(row, card.location);
}

void FastenerTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_kind.keepRows(rows);
    m_patchIdA.keepRows(rows);
    m_patchIdB.keepRows(rows);
    m_spotNode.keepRows(rows);
    m_nodeA.keepRows(rows);
    m_nodeB.keepRows(rows);
    m_location.keepRows(rows);
}

void FastenerTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, m_propId.get(row)));
    // IDA and IDB are two properties (TYPE = PROP) or two shell elements (TYPE = ELEM).
    const ReferenceTarget patches =
        m_kind.get(row) == "Elements" ? ReferenceTarget::Element : ReferenceTarget::Property;
    m_patchIdA.set(row, visitor.visit("patchIdA", patches, m_patchIdA.get(row)));
    m_patchIdB.set(row, visitor.visit("patchIdB", patches, m_patchIdB.get(row)));
    if (const base::EntityId spotNode = m_spotNode.get(row); spotNode > 0)
        m_spotNode.set(row, visitor.visit("spotNode", ReferenceTarget::Node, spotNode));
    if (const base::EntityId nodeA = m_nodeA.get(row); nodeA > 0)
        m_nodeA.set(row, visitor.visit("nodeA", ReferenceTarget::Node, nodeA));
    if (const base::EntityId nodeB = m_nodeB.get(row); nodeB > 0)
        m_nodeB.set(row, visitor.visit("nodeB", ReferenceTarget::Node, nodeB));
}

} // namespace oor::model
