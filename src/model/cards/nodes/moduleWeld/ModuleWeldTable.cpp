#include "oor/model/cards/nodes/moduleWeld/ModuleWeldTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/cards/nodes/ModuleConnectionSupport.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace oor::model
{

namespace
{

constexpr int FIRST_PATCH_FIELD = 9;  // PIDA or SHIDA, then PIDB or SHIDB
constexpr int SPOT_MODULE_FIELD = 11; // ELEMID only
constexpr int MODULE_A_FIELD = 13;    // every format
constexpr int MODULE_B_FIELD = 14;
constexpr int FIRST_LOCATION_FIELD = 17; // XS YS ZS (PARTPAT, ELPAT); GA1 to GA8 (GRIDID)
constexpr int FIRST_NODE_B_FIELD = 25;   // GB1 to GB8 (GRIDID)
constexpr int LAST_GRID_FIELD = 32;
constexpr base::EntityId ID_LIMIT = 100000000; // an element ID is below it (format reference, EWID)

constexpr std::array<const char*, 3> LOCATION_NAMES{"xStation", "yStation", "zStation"};
constexpr std::array<const char*, ModuleWeldCard::GRID_COUNT> NODE_A_NAMES{"nodeA1", "nodeA2", "nodeA3", "nodeA4",
                                                                               "nodeA5", "nodeA6", "nodeA7", "nodeA8"};
constexpr std::array<const char*, ModuleWeldCard::GRID_COUNT> NODE_B_NAMES{"nodeB1", "nodeB2", "nodeB3", "nodeB4",
                                                                               "nodeB5", "nodeB6", "nodeB7", "nodeB8"};

bool hasLocationType(ModuleWeldType type)
{
    return type == ModuleWeldType::PartPatch || type == ModuleWeldType::ElementPatch;
}

bool anyUsed(const std::array<base::EntityId, ModuleWeldCard::GRID_COUNT>& ids)
{
    return std::any_of(ids.begin(), ids.end(), [](base::EntityId id) { return id != 0; });
}

bool anyNegative(const std::array<base::EntityId, ModuleWeldCard::GRID_COUNT>& ids)
{
    return std::any_of(ids.begin(), ids.end(), [](base::EntityId id) { return id < 0; });
}

// The fields the layout of a TYPE uses: every other cell is blank and must be blank.
std::vector<int> usedFieldsOf(ModuleWeldType type)
{
    std::vector<int> fields{1, 2, 4, 5, 6, 8, MODULE_A_FIELD, MODULE_B_FIELD};
    if (type != ModuleWeldType::Align)
        fields.push_back(3); // GS
    switch (type)
    {
    case ModuleWeldType::PartPatch:
    case ModuleWeldType::ElementPatch:
        fields.insert(fields.end(), {FIRST_PATCH_FIELD, FIRST_PATCH_FIELD + 1, FIRST_LOCATION_FIELD,
                                     FIRST_LOCATION_FIELD + 1, FIRST_LOCATION_FIELD + 2});
        break;
    case ModuleWeldType::ElementId:
        fields.insert(fields.end(), {FIRST_PATCH_FIELD, FIRST_PATCH_FIELD + 1, SPOT_MODULE_FIELD});
        break;
    case ModuleWeldType::GridId:
        fields.push_back(7); // SPTYP
        for (int field = FIRST_LOCATION_FIELD; field <= LAST_GRID_FIELD; ++field)
            fields.push_back(field);
        break;
    case ModuleWeldType::Align:
        break;
    }
    return fields;
}

} // namespace

std::optional<ModuleWeldCard> ModuleWeldFormat::read(FieldReader& reader)
{
    ModuleWeldCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    card.spotNode = reader.integer(3, "spotNode", 0);
    card.type = readRequiredKeyword<ModuleWeldType>(reader, 4, "kind", MODULE_WELD_TYPE_WORDS);
    if (reader.failed())
        return card; // without a TYPE the layout of the rest of the card is not known
    card.nodeA = reader.integer(5, "nodeA", 0);
    card.nodeB = reader.integer(6, "nodeB", 0);
    card.matFrameId = reader.integer(8, "matFrameId", ModuleWeldCard::NO_MAT_FRAME_ID);
    card.moduleIdA = reader.integer(MODULE_A_FIELD, "moduleIdA", 0);
    card.moduleIdB = reader.integer(MODULE_B_FIELD, "moduleIdB", 0);
    switch (card.type)
    {
    case ModuleWeldType::PartPatch:
    case ModuleWeldType::ElementPatch:
    {
        const bool properties = card.type == ModuleWeldType::PartPatch;
        card.patchIdA = reader.requiredInteger(FIRST_PATCH_FIELD, properties ? "propIdA" : "shellIdA");
        card.patchIdB = reader.requiredInteger(FIRST_PATCH_FIELD + 1, properties ? "propIdB" : "shellIdB");
        // The location is all three coordinates or none.
        std::array<std::optional<double>, 3> location;
        for (std::size_t index = 0; index < location.size(); ++index)
            location[index] =
                reader.optionalReal(FIRST_LOCATION_FIELD + static_cast<int>(index), LOCATION_NAMES[index]);
        if (location[0] && location[1] && location[2])
            card.location = std::array<double, 3>{*location[0], *location[1], *location[2]};
        else if (location[0] || location[1] || location[2])
            reader.error("card.missing-field",
                         std::string("xStation") + ", " + std::string("yStation") + " and " + std::string("zStation") +
                             " must be all given or all blank",
                         "xStation");
        break;
    }
    case ModuleWeldType::ElementId:
        card.patchIdA = reader.requiredInteger(FIRST_PATCH_FIELD, "shellIdA");
        // A blank SHIDB connects the point GS to the patch A (the format reference of GS names
        // this case).
        card.patchIdB = reader.integer(FIRST_PATCH_FIELD + 1, "shellIdB", 0);
        card.spotModuleId = reader.integer(SPOT_MODULE_FIELD, "spotModuleId", 0);
        break;
    case ModuleWeldType::GridId:
        card.patchTypes =
            readRequiredKeyword<ModuleWeldPatchTypes>(reader, 7, "patchTypes", MODULE_WELD_PATCH_TYPE_WORDS);
        for (std::size_t index = 0; index < ModuleWeldCard::GRID_COUNT; ++index)
        {
            card.nodesA[index] = reader.integer(FIRST_LOCATION_FIELD + static_cast<int>(index), NODE_A_NAMES[index], 0);
            card.nodesB[index] = reader.integer(FIRST_NODE_B_FIELD + static_cast<int>(index), NODE_B_NAMES[index], 0);
        }
        break;
    case ModuleWeldType::Align:
        break;
    }
    // A field the format of this TYPE does not have must be blank: nothing is dropped silently.
    requireOnlyFields(reader, usedFieldsOf(card.type));
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleWeldFormat::check(const ModuleWeldCard& card)
{
    if (card.id <= 0 || card.id >= ID_LIMIT)
        return CardProblem{"card.bad-id", "the element ID must be an integer from 1 to 99999999", "id"};
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    if (const auto problem = checkOptionalId(card.spotNode, "spotNode"))
        return problem;
    if (const auto problem = checkOptionalId(card.nodeA, "nodeA"))
        return problem;
    if (const auto problem = checkOptionalId(card.nodeB, "nodeB"))
        return problem;
    if (card.matFrameId < ModuleWeldCard::NO_MAT_FRAME_ID)
        return CardProblem{"card.bad-value", "{field} must be -1 (blank) or a coordinate system ID", "matFrameId"};
    if (const auto problem = checkModuleNumber(card.spotModuleId, "spotModuleId"))
        return problem;
    if (const auto problem = checkModuleNumber(card.moduleIdA, "moduleIdA"))
        return problem;
    if (const auto problem = checkModuleNumber(card.moduleIdB, "moduleIdB"))
        return problem;

    // What the format of this TYPE does not have stays blank.
    const bool isGrid = card.type == ModuleWeldType::GridId;
    const bool isAlign = card.type == ModuleWeldType::Align;
    if (isGrid && !card.patchTypes)
        return CardProblem{"card.missing-field", "{field} is required by the GRIDID format", "patchTypes"};
    if (!isGrid && card.patchTypes)
        return CardProblem{"card.exclusive-fields", "{field} belongs to the GRIDID format", "patchTypes"};
    if (!isGrid && (anyUsed(card.nodesA) || anyUsed(card.nodesB)))
        return CardProblem{"card.exclusive-fields",
                           "{field} to " + CardProblem::fieldNamed("nodeB8") + " belong to the GRIDID format",
                           "nodeA1"};
    if ((isGrid || isAlign) && (card.patchIdA != 0 || card.patchIdB != 0))
        return CardProblem{
            "card.exclusive-fields",
            "{field} and " + CardProblem::fieldNamed("patchIdB") + " belong to the PARTPAT, ELPAT and ELEMID formats",
            "patchIdA"};
    if (!hasLocationType(card.type) && card.location)
        return CardProblem{"card.exclusive-fields",
                           "{field}, " + CardProblem::fieldNamed("yStation") + " and " +
                               CardProblem::fieldNamed("zStation") + " belong to the PARTPAT and ELPAT formats",
                           "xStation"};
    if (card.type != ModuleWeldType::ElementId && card.spotModuleId != 0)
        return CardProblem{"card.exclusive-fields", "{field} belongs to the ELEMID format", "spotModuleId"};
    if (isAlign && card.spotNode != 0)
        return CardProblem{"card.exclusive-fields", "the ALIGN format has no {field}", "spotNode"};

    if (isAlign)
    {
        if (card.nodeA <= 0 || card.nodeB <= 0)
            return CardProblem{"card.missing-field",
                               "{field} and " + CardProblem::fieldNamed("nodeB") + " are required by the ALIGN format",
                               "nodeA"};
        // The two vertex points of one module are two points; two modules may number their points alike.
        if (card.moduleIdA == card.moduleIdB && card.nodeA == card.nodeB)
            return CardProblem{"card.duplicate-node",
                               CardProblem::fieldNamed("nodeA") + " and {field} must be different", "nodeB"};
        return std::nullopt;
    }
    if (isGrid)
    {
        if (anyNegative(card.nodesA) || anyNegative(card.nodesB))
            return CardProblem{"card.bad-id", "the grid point IDs must not be negative", "nodeA1"};
        if (card.nodesA[0] <= 0 || card.nodesA[1] <= 0 || card.nodesA[2] <= 0)
            return CardProblem{"card.missing-field",
                               "{field}, " + CardProblem::fieldNamed("nodeA2") + " and " +
                                   CardProblem::fieldNamed("nodeA3") + " are required by the GRIDID format",
                               "nodeA1"};
        // SPTYP Q or T: the patch B is not given, the point GS is connected to the patch A.
        const bool pointToPatch =
            card.patchTypes == ModuleWeldPatchTypes::Quad || card.patchTypes == ModuleWeldPatchTypes::Triangle;
        if (pointToPatch && anyUsed(card.nodesB))
            return CardProblem{"card.exclusive-fields",
                               CardProblem::fieldNamed("patchTypes") +
                                   " Q or T connects a point to a patch: {field} to " +
                                   CardProblem::fieldNamed("nodeB8") + " are blank",
                               "nodeB1"};
        if (!pointToPatch && (card.nodesB[0] <= 0 || card.nodesB[1] <= 0 || card.nodesB[2] <= 0))
            return CardProblem{"card.missing-field",
                               "{field}, " + CardProblem::fieldNamed("nodeB2") + " and " +
                                   CardProblem::fieldNamed("nodeB3") + " are required for a patch to patch connection",
                               "nodeB1"};
        return std::nullopt;
    }
    if (card.patchIdA <= 0)
        return CardProblem{"card.bad-id",
                           "the first patch (" + CardProblem::fieldNamed("propIdA") + " or " +
                               CardProblem::fieldNamed("shellIdA") + ") must be a positive integer",
                           "patchIdA"};
    if (card.type == ModuleWeldType::ElementId)
    {
        if (card.patchIdB < 0)
            return CardProblem{"card.bad-id",
                               CardProblem::fieldNamed("shellIdB") + " must be 0 (blank) or an element ID", "patchIdB"};
    }
    else if (card.patchIdB <= 0)
    {
        return CardProblem{"card.bad-id",
                           "the second patch (" + CardProblem::fieldNamed("propIdB") + " or " +
                               CardProblem::fieldNamed("shellIdB") + ") must be a positive integer",
                           "patchIdB"};
    }
    // The two patches differ; two patches of two modules may have the same number.
    if (card.patchIdB != 0 && card.moduleIdA == card.moduleIdB && card.patchIdA == card.patchIdB)
        return CardProblem{"card.duplicate-node", "the two patches must be different", "patchIdB"};
    if (card.location)
    {
        for (std::size_t index = 0; index < LOCATION_NAMES.size(); ++index)
        {
            if (const auto problem = checkReal((*card.location)[index], LOCATION_NAMES[index], RealRule::Finite))
                return problem;
        }
    }
    else if (hasLocationType(card.type) && card.spotNode == 0 && card.nodeA == 0)
    {
        return CardProblem{"card.missing-field",
                           "the location is required: {field}, " + CardProblem::fieldNamed("nodeA") + " or " +
                               CardProblem::fieldNamed("xStation") + ", " + CardProblem::fieldNamed("yStation") +
                               " and " + CardProblem::fieldNamed("zStation"),
                           "spotNode"};
    }
    return std::nullopt;
}

void ModuleWeldFormat::write(const ModuleWeldCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    if (card.type != ModuleWeldType::Align)
        writer.integerUnlessDefault(3, card.spotNode, 0);
    writeKeyword(writer, 4, MODULE_WELD_TYPE_WORDS, card.type);
    writer.integerUnlessDefault(5, card.nodeA, 0);
    writer.integerUnlessDefault(6, card.nodeB, 0);
    if (card.patchTypes)
        writeKeyword(writer, 7, MODULE_WELD_PATCH_TYPE_WORDS, *card.patchTypes);
    writer.integerUnlessDefault(8, card.matFrameId, ModuleWeldCard::NO_MAT_FRAME_ID);
    switch (card.type)
    {
    case ModuleWeldType::PartPatch:
    case ModuleWeldType::ElementPatch:
        writer.integer(FIRST_PATCH_FIELD, card.patchIdA);
        writer.integer(FIRST_PATCH_FIELD + 1, card.patchIdB);
        break;
    case ModuleWeldType::ElementId:
        writer.integer(FIRST_PATCH_FIELD, card.patchIdA);
        writer.integerUnlessDefault(FIRST_PATCH_FIELD + 1, card.patchIdB, 0);
        writer.integerUnlessDefault(SPOT_MODULE_FIELD, card.spotModuleId, 0);
        break;
    case ModuleWeldType::GridId:
    case ModuleWeldType::Align:
        break;
    }
    writer.integerUnlessDefault(MODULE_A_FIELD, card.moduleIdA, 0);
    writer.integerUnlessDefault(MODULE_B_FIELD, card.moduleIdB, 0);
    if (card.type == ModuleWeldType::GridId)
    {
        for (std::size_t index = 0; index < ModuleWeldCard::GRID_COUNT; ++index)
        {
            writer.integerUnlessDefault(FIRST_LOCATION_FIELD + static_cast<int>(index), card.nodesA[index], 0);
            writer.integerUnlessDefault(FIRST_NODE_B_FIELD + static_cast<int>(index), card.nodesB[index], 0);
        }
    }
    else if (card.location)
    {
        for (std::size_t index = 0; index < card.location->size(); ++index)
            writer.real(FIRST_LOCATION_FIELD + static_cast<int>(index), (*card.location)[index]);
    }
}

std::uint32_t ModuleWeldTable::append(const ModuleWeldCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleWeldTable::store(std::uint32_t row, const ModuleWeldCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleWeldTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleWeldTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleWeldTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleWeldCard& card = m_cards.edit(row);
    visitId(visitor, "propId", ReferenceTarget::Property, card.propId); // a PWELD of the main Bulk Data
    // GS is in the module MODS for an ELEMID, which may connect the point to a patch; otherwise the format reference
    // puts it in the main Bulk Data.
    if (card.type == ModuleWeldType::ElementId)
        visitIdInModule(visitor, "spotNode", ReferenceTarget::Node, card.spotModuleId, card.spotNode);
    else
        visitId(visitor, "spotNode", ReferenceTarget::Node, card.spotNode);
    // The vertex points of an ALIGN are in the modules of the two patches; the piercing points of the other formats are
    // in the main Bulk Data.
    if (card.type == ModuleWeldType::Align)
    {
        visitIdInModule(visitor, "nodeA", ReferenceTarget::Node, card.moduleIdA, card.nodeA);
        visitIdInModule(visitor, "nodeB", ReferenceTarget::Node, card.moduleIdB, card.nodeB);
    }
    else
    {
        visitId(visitor, "nodeA", ReferenceTarget::Node, card.nodeA);
        visitId(visitor, "nodeB", ReferenceTarget::Node, card.nodeB);
    }
    visitId(visitor, "matFrameId", ReferenceTarget::Coordinate, card.matFrameId); // -1 (blank) and 0 name no system
    switch (card.type)
    {
    case ModuleWeldType::PartPatch:
        visitIdInModule(visitor, "patchIdA", ReferenceTarget::Property, card.moduleIdA, card.patchIdA);
        visitIdInModule(visitor, "patchIdB", ReferenceTarget::Property, card.moduleIdB, card.patchIdB);
        break;
    case ModuleWeldType::ElementPatch:
    case ModuleWeldType::ElementId:
        visitIdInModule(visitor, "patchIdA", ReferenceTarget::Element, card.moduleIdA, card.patchIdA);
        visitIdInModule(visitor, "patchIdB", ReferenceTarget::Element, card.moduleIdB, card.patchIdB);
        break;
    case ModuleWeldType::GridId:
        for (base::EntityId& id : card.nodesA)
            visitIdInModule(visitor, "nodeA1", ReferenceTarget::Node, card.moduleIdA, id);
        for (base::EntityId& id : card.nodesB)
            visitIdInModule(visitor, "nodeB1", ReferenceTarget::Node, card.moduleIdB, id);
        break;
    case ModuleWeldType::Align:
        break;
    }
}

} // namespace oor::model
