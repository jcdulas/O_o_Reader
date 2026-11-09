#include "oor/model/cards/nodes/moduleFastener/ModuleFastenerTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/cards/nodes/ModuleConnectionSupport.hpp"

#include <array>

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 16;
constexpr base::EntityId ID_LIMIT = 100000000; // an element ID is below it (format reference, EID)
constexpr std::array<const char*, 3> LOCATION_NAMES{"xStation", "yStation", "zStation"};

} // namespace

std::optional<ModuleFastenerCard> ModuleFastenerFormat::read(FieldReader& reader)
{
    ModuleFastenerCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.integer(2, "propId", card.id); // format reference: PID defaults to EID
    card.type = readRequiredKeyword<ModuleFastenerType>(reader, 3, "kind", MODULE_FASTENER_TYPE_WORDS);
    card.patchIdA = reader.requiredInteger(4, "patchIdA");
    card.patchIdB = reader.requiredInteger(5, "patchIdB");
    card.spotNode = reader.integer(6, "spotNode", 0);
    card.nodeA = reader.integer(7, "nodeA", 0);
    card.nodeB = reader.integer(8, "nodeB", 0);
    // The location starts the continuation line: the three coordinates are all given or all blank.
    std::array<std::optional<double>, 3> location;
    for (std::size_t index = 0; index < location.size(); ++index)
        location[index] = reader.optionalReal(9 + static_cast<int>(index), LOCATION_NAMES[index]);
    if (location[0] && location[1] && location[2])
        card.location = std::array<double, 3>{*location[0], *location[1], *location[2]};
    else if (location[0] || location[1] || location[2])
        reader.error("card.missing-field",
                     std::string("xStation") + ", " + std::string("yStation") + " and " + std::string("zStation") +
                         " must be all given or all blank",
                     "xStation");
    // Fields 12 to 14 are blank cells of the layout.
    card.moduleIdA = reader.integer(15, "moduleIdA", 0);
    card.moduleIdB = reader.integer(16, "moduleIdB", 0);
    requireBlankFields(reader, {12, 13, 14}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleFastenerFormat::check(const ModuleFastenerCard& card)
{
    if (card.id <= 0 || card.id >= ID_LIMIT)
        return CardProblem{"card.bad-id", "the element ID must be an integer from 1 to 99999999", "id"};
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    if (const auto problem = checkRequiredId(card.patchIdA, "patchIdA"))
        return problem;
    if (const auto problem = checkRequiredId(card.patchIdB, "patchIdB"))
        return problem;
    if (const auto problem = checkModuleNumber(card.moduleIdA, "moduleIdA"))
        return problem;
    if (const auto problem = checkModuleNumber(card.moduleIdB, "moduleIdB"))
        return problem;
    // IDA and IDB differ (format reference); two patches of two modules may have the same number, so only the patches
    // of one module are told apart.
    if (card.moduleIdA == card.moduleIdB && card.patchIdA == card.patchIdB)
        return CardProblem{
            "card.duplicate-node",
            CardProblem::fieldNamed("patchIdA") + " and {field} must differ when they are in the same module",
            "patchIdB"};
    if (const auto problem = checkOptionalId(card.spotNode, "spotNode"))
        return problem;
    if (const auto problem = checkOptionalId(card.nodeA, "nodeA"))
        return problem;
    if (const auto problem = checkOptionalId(card.nodeB, "nodeB"))
        return problem;
    if (card.location)
    {
        for (std::size_t index = 0; index < LOCATION_NAMES.size(); ++index)
        {
            if (const auto problem = checkReal((*card.location)[index], LOCATION_NAMES[index], RealRule::Finite))
                return problem;
        }
    }
    else if (card.spotNode == 0 && card.nodeA == 0)
    {
        return CardProblem{"card.missing-field",
                           "the location is required: {field}, " + CardProblem::fieldNamed("nodeA") + " or " +
                               CardProblem::fieldNamed("xStation") + ", " + CardProblem::fieldNamed("yStation") +
                               " and " + CardProblem::fieldNamed("zStation"),
                           "spotNode"};
    }
    return std::nullopt;
}

void ModuleFastenerFormat::write(const ModuleFastenerCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId); // stored resolved (the default is EID): always written
    writeKeyword(writer, 3, MODULE_FASTENER_TYPE_WORDS, card.type);
    writer.integer(4, card.patchIdA);
    writer.integer(5, card.patchIdB);
    writer.integerUnlessDefault(6, card.spotNode, 0);
    writer.integerUnlessDefault(7, card.nodeA, 0);
    writer.integerUnlessDefault(8, card.nodeB, 0);
    if (card.location)
    {
        for (std::size_t index = 0; index < card.location->size(); ++index)
            writer.real(9 + static_cast<int>(index), (*card.location)[index]);
    }
    writer.integerUnlessDefault(15, card.moduleIdA, 0);
    writer.integerUnlessDefault(16, card.moduleIdB, 0);
}

std::uint32_t ModuleFastenerTable::append(const ModuleFastenerCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleFastenerTable::store(std::uint32_t row, const ModuleFastenerCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleFastenerTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleFastenerTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleFastenerTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleFastenerCard& card = m_cards.edit(row);
    visitId(visitor, "propId", ReferenceTarget::Property, card.propId); // a PFAST of the main Bulk Data
    // IDA and IDB are properties (PROP) or shell elements (ELEM) of the module that MODA and MODB name: looked up only
    // in the main Bulk Data (ModuleConnectionSupport.hpp).
    const ReferenceTarget patches =
        card.type == ModuleFastenerType::Element ? ReferenceTarget::Element : ReferenceTarget::Property;
    visitIdInModule(visitor, "patchIdA", patches, card.moduleIdA, card.patchIdA);
    visitIdInModule(visitor, "patchIdB", patches, card.moduleIdB, card.patchIdB);
    // The format reference puts the grid points of the fastener in the main Bulk Data.
    visitId(visitor, "spotNode", ReferenceTarget::Node, card.spotNode);
    visitId(visitor, "nodeA", ReferenceTarget::Node, card.nodeA);
    visitId(visitor, "nodeB", ReferenceTarget::Node, card.nodeB);
}

} // namespace oor::model
