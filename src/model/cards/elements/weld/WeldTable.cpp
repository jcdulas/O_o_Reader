#include "oor/model/cards/elements/weld/WeldTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>
#include <string>

namespace oor::model
{

namespace
{

constexpr int FIRST_PATCH_FIELD = 9;     // PIDA/SHIDA, then PIDB/SHIDB; GA1 to GA8 in the GRIDID format
constexpr int FIRST_LOCATION_FIELD = 17; // XS YS ZS; GB1 to GB8 in the GRIDID format

bool isKnownType(const std::string& type)
{
    return type == "PartPatch" || type == "ElemPatch" || type == "ElemIdPatch" || type == "NodeIdPatch" ||
           type == "AlignedNodes";
}

bool hasLocationType(const std::string& type)
{
    return type == "PartPatch" || type == "ElemPatch";
}

bool isKnownPatchShapes(const std::string& patchShapes)
{
    return patchShapes == "QuadQuadPatch" || patchShapes == "QuadTriaPatch" || patchShapes == "TriaTriaPatch" ||
           patchShapes == "TriaQuadPatch" || patchShapes == "QuadPatch" || patchShapes == "TriaPatch";
}

// True when the format of `type` has a field in the position `field` (1 is EWID).
bool usesField(const std::string& type, int field)
{
    if (field <= 6 || field == 8)
        return true;
    if (field == 7)
        return type == "NodeIdPatch";
    if (type == "NodeIdPatch")
        return field <= 24;
    if (type == "AlignedNodes")
        return false;
    if (field == FIRST_PATCH_FIELD || field == FIRST_PATCH_FIELD + 1)
        return true;
    return hasLocationType(type) && field >= FIRST_LOCATION_FIELD && field <= FIRST_LOCATION_FIELD + 2;
}

} // namespace

std::optional<WeldCard> WeldFormat::read(FieldReader& reader)
{
    static constexpr const char* NAMES_XYZ[3] = {"locationX", "locationY", "locationZ"};
    static constexpr const char* NAMES_NODE_A[WeldCard::GRID_COUNT] = {"nodeA1", "nodeA2", "nodeA3", "nodeA4",
                                                                           "nodeA5", "nodeA6", "nodeA7", "nodeA8"};
    static constexpr const char* NAMES_NODE_B[WeldCard::GRID_COUNT] = {"nodeB1", "nodeB2", "nodeB3", "nodeB4",
                                                                           "nodeB5", "nodeB6", "nodeB7", "nodeB8"};
    WeldCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    card.spotNode = reader.integer(3, "spotNode", 0);
    card.kind = std::string(reader.word(4));
    card.nodeA = reader.integer(5, "nodeA", 0);
    card.nodeB = reader.integer(6, "nodeB", 0);
    card.matFrameId = reader.integer(8, "matFrameId", WeldCard::NO_MAT_FRAME_ID);
    if (!isKnownType(card.kind))
    {
        reader.error("card.bad-value",
                     std::string("kind") + " must be PartPatch, ElemPatch, ElemIdPatch, NodeIdPatch or AlignedNodes",
                     "kind");
        return card;
    }
    if (card.kind == "NodeIdPatch")
    {
        card.patchShapes = std::string(reader.word(7));
        for (std::size_t index = 0; index < WeldCard::GRID_COUNT; ++index)
        {
            card.patchNodesA[index] =
                reader.integer(FIRST_PATCH_FIELD + static_cast<int>(index), NAMES_NODE_A[index], 0);
            card.patchNodesB[index] =
                reader.integer(FIRST_LOCATION_FIELD + static_cast<int>(index), NAMES_NODE_B[index], 0);
        }
    }
    else if (card.kind != "AlignedNodes")
    {
        card.patchIdA = reader.requiredInteger(FIRST_PATCH_FIELD, card.kind == "PartPatch" ? "propIdA" : "shellIdA");
        // ELEMID: a blank SHIDB is a connection of the point GS to the patch A.
        card.patchIdB =
            card.kind == "ElemIdPatch"
                ? reader.integer(FIRST_PATCH_FIELD + 1, "shellIdB", 0)
                : reader.requiredInteger(FIRST_PATCH_FIELD + 1, card.kind == "PartPatch" ? "propIdB" : "shellIdB");
        if (hasLocationType(card.kind))
        {
            for (std::size_t index = 0; index < 3; ++index)
            {
                card.location[index] = reader.real(FIRST_LOCATION_FIELD + static_cast<int>(index), NAMES_XYZ[index],
                                                   WeldCard::NO_COORDINATE);
            }
        }
    }
    // A field the format of this TYPE does not have must be blank: nothing is dropped silently.
    for (int field = 1; field <= reader.lastUsedField(); ++field)
    {
        if (!reader.isBlank(field) && !usesField(card.kind, field))
        {
            reader.error("card.bad-value", "field " + std::to_string(field) + " is not used by the " + card.kind +
                                               " format of a " + std::string(reader.cardName()));
            break;
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> WeldFormat::check(const WeldCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    if (!isKnownType(card.kind))
        return CardProblem{"card.bad-value",
                           "{field} must be PartPatch, ElemPatch, ElemIdPatch, NodeIdPatch or AlignedNodes", "kind"};
    if (card.spotNode < 0 || card.nodeA < 0 || card.nodeB < 0)
        return CardProblem{"card.bad-id",
                           "{field}, {field:nodeA} and {field:nodeB} must be 0 (blank) or grid point IDs", "spotNode"};
    if (card.matFrameId < WeldCard::NO_MAT_FRAME_ID)
        return CardProblem{"card.bad-value", "{field} must be -1 or a coordinate system ID", "matFrameId"};

    // What the format of this TYPE does not have stays blank.
    const bool isGrid = card.kind == "NodeIdPatch";
    const bool isAlign = card.kind == "AlignedNodes";
    const bool anyGrid =
        std::any_of(card.patchNodesA.begin(), card.patchNodesA.end(), [](base::EntityId id) { return id != 0; }) ||
        std::any_of(card.patchNodesB.begin(), card.patchNodesB.end(), [](base::EntityId id) { return id != 0; });
    if (!isGrid && (anyGrid || !card.patchShapes.empty()))
        return CardProblem{"card.exclusive-fields",
                           "{field:nodeA1} to {field:nodeB8} and {field} belong to the NodeIdPatch format",
                           "patchShapes"};
    if ((isGrid || isAlign) && (card.patchIdA != 0 || card.patchIdB != 0))
        return CardProblem{"card.exclusive-fields",
                           "{field:patchIdA} and {field:patchIdB} belong to the PARTPAT, ELPAT and ELEMID formats",
                           "patchIdA"};
    if (!hasLocationType(card.kind) && card.hasLocation())
        return CardProblem{"card.exclusive-fields",
                           "{field}, {field:locationY} and {field:locationZ} belong to the PARTPAT and ELPAT formats",
                           "locationX"};

    if (isAlign)
    {
        if (card.nodeA <= 0 || card.nodeB <= 0)
            return CardProblem{"card.missing-field", "{field} and {field:nodeB} are required by the ALIGN format",
                               "nodeA"};
        if (card.nodeA == card.nodeB)
            return CardProblem{"card.duplicate-node", "{field:nodeA} and {field} must be different", "nodeB"};
        return std::nullopt;
    }
    if (isGrid)
    {
        if (!isKnownPatchShapes(card.patchShapes))
            return CardProblem{
                "card.bad-value",
                "{field} must be QuadQuadPatch, QuadTriaPatch, TriaTriaPatch, TriaQuadPatch, QuadPatch or TriaPatch",
                "patchShapes"};
        if (std::any_of(card.patchNodesA.begin(), card.patchNodesA.end(), [](base::EntityId id) { return id < 0; }) ||
            std::any_of(card.patchNodesB.begin(), card.patchNodesB.end(), [](base::EntityId id) { return id < 0; }))
            return CardProblem{"card.bad-id", "the grid point IDs must not be negative", "nodeA1"};
        if (card.patchNodesA[0] <= 0 || card.patchNodesA[1] <= 0 || card.patchNodesA[2] <= 0)
            return CardProblem{"card.missing-field",
                               "{field}, {field:nodeA2} and {field:nodeA3} are required by the GRIDID format",
                               "nodeA1"};
        // SPTYP Q or T: the patch B is not given, the point GS is connected to the patch A.
        const bool pointToPatch = card.patchShapes == "QuadPatch" || card.patchShapes == "TriaPatch";
        if (pointToPatch &&
            std::any_of(card.patchNodesB.begin(), card.patchNodesB.end(), [](base::EntityId id) { return id != 0; }))
            return CardProblem{"card.exclusive-fields",
                               "{field:patchShapes} QuadPatch or TriaPatch connects a point to a patch: {field} to "
                               "{field:nodeB8} are blank",
                               "nodeB1"};
        if (!pointToPatch && (card.patchNodesB[0] <= 0 || card.patchNodesB[1] <= 0 || card.patchNodesB[2] <= 0))
            return CardProblem{
                "card.missing-field",
                "{field}, {field:nodeB2} and {field:nodeB3} are required for a patch to patch connection", "nodeB1"};
        return std::nullopt;
    }
    if (card.patchIdA <= 0)
        return CardProblem{"card.bad-id",
                           "the first patch ({field:propIdA} or {field:shellIdA}) must be a positive integer",
                           "patchIdA"};
    if (card.kind == "ElemIdPatch")
    {
        if (card.patchIdB < 0)
            return CardProblem{"card.bad-id", "{field:shellIdB} must be 0 (blank) or an element ID", "patchIdB"};
    }
    else if (card.patchIdB <= 0)
    {
        return CardProblem{"card.bad-id",
                           "the second patch ({field:propIdB} or {field:shellIdB}) must be a positive integer",
                           "patchIdB"};
    }
    if (card.patchIdA == card.patchIdB)
        return CardProblem{"card.duplicate-node", "the two patches must be different", "patchIdB"};
    if (card.hasLocation())
    {
        const bool all =
            !std::isnan(card.location[0]) && !std::isnan(card.location[1]) && !std::isnan(card.location[2]);
        if (!all)
            return CardProblem{"card.missing-field",
                               "{field}, {field:locationY} and {field:locationZ} must be all given or all blank",
                               "locationX"};
    }
    else if (hasLocationType(card.kind) && card.spotNode == 0 && card.nodeA == 0)
    {
        return CardProblem{"card.missing-field",
                           "the location is required: {field}, {field:nodeA} or {field:locationX}, {field:locationY} "
                           "and {field:locationZ}",
                           "spotNode"};
    }
    return std::nullopt;
}

void WeldFormat::write(const WeldCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integerUnlessDefault(3, card.spotNode, 0);
    writer.text(4, card.kind);
    writer.integerUnlessDefault(5, card.nodeA, 0);
    writer.integerUnlessDefault(6, card.nodeB, 0);
    if (card.kind == "NodeIdPatch")
        writer.text(7, card.patchShapes);
    writer.integerUnlessDefault(8, card.matFrameId, WeldCard::NO_MAT_FRAME_ID);
    if (card.kind == "NodeIdPatch")
    {
        for (std::size_t index = 0; index < WeldCard::GRID_COUNT; ++index)
        {
            writer.integerUnlessDefault(FIRST_PATCH_FIELD + static_cast<int>(index), card.patchNodesA[index], 0);
            writer.integerUnlessDefault(FIRST_LOCATION_FIELD + static_cast<int>(index), card.patchNodesB[index], 0);
        }
        return;
    }
    if (card.kind == "AlignedNodes")
        return;
    writer.integer(FIRST_PATCH_FIELD, card.patchIdA);
    if (card.kind == "ElemIdPatch")
        writer.integerUnlessDefault(FIRST_PATCH_FIELD + 1, card.patchIdB, 0);
    else
        writer.integer(FIRST_PATCH_FIELD + 1, card.patchIdB);
    if (card.hasLocation())
    {
        for (std::size_t index = 0; index < 3; ++index)
            writer.real(FIRST_LOCATION_FIELD + static_cast<int>(index), card.location[index]);
    }
}

std::uint32_t WeldTable::append(const WeldCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_spotNode.push(card.spotNode);
    m_kind.push(card.kind);
    m_nodeA.push(card.nodeA);
    m_nodeB.push(card.nodeB);
    m_patchShapes.push(card.patchShapes);
    m_matFrameId.push(card.matFrameId);
    m_patchIdA.push(card.patchIdA);
    m_patchIdB.push(card.patchIdB);
    m_location.push(card.location);
    m_patchNodesA.push(card.patchNodesA);
    m_patchNodesB.push(card.patchNodesB);
    return row;
}

WeldCard WeldTable::card(std::uint32_t row) const
{
    return WeldCard{m_id.get(row),         m_propId.get(row),   m_spotNode.get(row),    m_kind.get(row),
                        m_nodeA.get(row),      m_nodeB.get(row),    m_patchShapes.get(row), m_matFrameId.get(row),
                        m_patchIdA.get(row),   m_patchIdB.get(row), m_location.get(row),    m_patchNodesA.get(row),
                        m_patchNodesB.get(row)};
}

void WeldTable::store(std::uint32_t row, const WeldCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_spotNode.set(row, card.spotNode);
    m_kind.set(row, card.kind);
    m_nodeA.set(row, card.nodeA);
    m_nodeB.set(row, card.nodeB);
    m_patchShapes.set(row, card.patchShapes);
    m_matFrameId.set(row, card.matFrameId);
    m_patchIdA.set(row, card.patchIdA);
    m_patchIdB.set(row, card.patchIdB);
    m_location.set(row, card.location);
    m_patchNodesA.set(row, card.patchNodesA);
    m_patchNodesB.set(row, card.patchNodesB);
}

void WeldTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_spotNode.keepRows(rows);
    m_kind.keepRows(rows);
    m_nodeA.keepRows(rows);
    m_nodeB.keepRows(rows);
    m_patchShapes.keepRows(rows);
    m_matFrameId.keepRows(rows);
    m_patchIdA.keepRows(rows);
    m_patchIdB.keepRows(rows);
    m_location.keepRows(rows);
    m_patchNodesA.keepRows(rows);
    m_patchNodesB.keepRows(rows);
}

void WeldTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, m_propId.get(row)));
    if (const base::EntityId spotNode = m_spotNode.get(row); spotNode > 0)
        m_spotNode.set(row, visitor.visit("spotNode", ReferenceTarget::Node, spotNode));
    if (const base::EntityId nodeA = m_nodeA.get(row); nodeA > 0)
        m_nodeA.set(row, visitor.visit("nodeA", ReferenceTarget::Node, nodeA));
    if (const base::EntityId nodeB = m_nodeB.get(row); nodeB > 0)
        m_nodeB.set(row, visitor.visit("nodeB", ReferenceTarget::Node, nodeB));
    // MCID -1 is blank and 0 the basic system: neither refers to a card.
    if (const base::EntityId matFrameId = m_matFrameId.get(row); matFrameId > 0)
        m_matFrameId.set(row, visitor.visit("matFrameId", ReferenceTarget::Coordinate, matFrameId));
    // IDA and IDB are two PSHELL properties (PARTPAT) or two shell elements (ELPAT, ELEMID).
    const std::string& type = m_kind.get(row);
    if (type == "PartPatch" || type == "ElemPatch" || type == "ElemIdPatch")
    {
        const ReferenceTarget patches = type == "PartPatch" ? ReferenceTarget::Property : ReferenceTarget::Element;
        m_patchIdA.set(row, visitor.visit("patchIdA", patches, m_patchIdA.get(row)));
        if (const base::EntityId patchIdB = m_patchIdB.get(row); patchIdB > 0)
            m_patchIdB.set(row, visitor.visit("patchIdB", patches, patchIdB));
    }
    if (type == "NodeIdPatch")
    {
        auto patchNodesA = m_patchNodesA.get(row);
        auto patchNodesB = m_patchNodesB.get(row);
        bool changed = false;
        for (auto* list : {&patchNodesA, &patchNodesB})
        {
            for (base::EntityId& id : *list)
            {
                if (id <= 0)
                    continue;
                const base::EntityId updated = visitor.visit("nodeA1", ReferenceTarget::Node, id);
                changed = changed || updated != id;
                id = updated;
            }
        }
        if (changed)
        {
            m_patchNodesA.set(row, patchNodesA);
            m_patchNodesB.set(row, patchNodesB);
        }
    }
}

} // namespace oor::model
