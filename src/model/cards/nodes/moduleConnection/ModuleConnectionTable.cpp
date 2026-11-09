#include "oor/model/cards/nodes/moduleConnection/ModuleConnectionTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/nodes/ModuleBoundarySupport.hpp"

#include <string>
#include <variant>

namespace oor::model
{

namespace
{

// The pairs of (module, point) start on the first continuation line, four to a line.
constexpr int FIRST_PAIR_FIELD = 9;

} // namespace

std::optional<ModuleConnectionCard> ModuleConnectionFormat::read(FieldReader& reader)
{
    ModuleConnectionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.type = readKeyword(reader, 2, "kind", MODULE_CONNECTION_TYPE_WORDS, ModuleConnectionType::Mrbe2);
    card.tolerance = reader.real(3, "tolerance", 0.0);
    // Field 4 tells the two layouts: an integer above zero is the GRID of the alternate layout, any other number (or
    // nothing) is the X of the first one.
    const auto first = reader.integerOrReal(4, "xCoord");
    if (first && std::holds_alternative<std::int32_t>(*first) && std::get<std::int32_t>(*first) > 0)
    {
        card.locationNode = std::get<std::int32_t>(*first);
        // In this layout fields 5, 6 and 8 are blank; field 7 is the MODID.
        for (const int field : {5, 6, 8})
        {
            if (!reader.isBlank(field))
                reader.error("card.bad-value",
                             "field " + std::to_string(field) + " of an " + std::string(reader.cardName()) +
                                 " with a " + std::string("locationNode") + " is not used and must be blank",
                             "");
        }
        card.moduleId = reader.integer(7, "moduleId", 0);
    }
    else
    {
        if (first)
            card.xCoord = std::holds_alternative<std::int32_t>(*first)
                              ? static_cast<double>(std::get<std::int32_t>(*first))
                              : std::get<double>(*first);
        card.yCoord = reader.real(5, "yCoord", 0.0);
        card.zCoord = reader.real(6, "zCoord", 0.0);
        if (!reader.isBlank(7))
            reader.error("card.bad-value",
                         "field 7 of an " + std::string(reader.cardName()) + " without a " +
                             std::string("locationNode") + " is not used and must be blank",
                         "");
        card.frameId = reader.integer(8, "frameId", 0);
    }
    const int last = reader.lastUsedField();
    for (int field = FIRST_PAIR_FIELD; field <= last && !reader.failed(); field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue; // the padding of a line
        ModuleId pair;
        pair.module = reader.integer(field, "memberModuleId", 0);
        pair.id = reader.integer(field + 1, "memberNodeId", 0);
        card.points.push_back(pair);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleConnectionFormat::check(const ModuleConnectionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.tolerance, "tolerance", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.xCoord, "xCoord", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.yCoord, "yCoord", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.zCoord, "zCoord", RealRule::Finite))
        return problem;
    if (card.frameId < 0)
        return CardProblem{"card.bad-id", "{field} must be zero or a positive coordinate system ID", "frameId"};
    if (card.locationNode < 0)
        return CardProblem{"card.bad-id", "{field} must be a positive grid point ID", "locationNode"};
    if (const auto problem = checkBoundaryModule(card.moduleId, "moduleId"))
        return problem;
    // The two layouts do not mix: a GRID has no X, Y, Z or CID, and a MODID needs the GRID it belongs to.
    if (card.usesGrid() && (card.xCoord != 0.0 || card.yCoord != 0.0 || card.zCoord != 0.0 || card.frameId != 0))
        return CardProblem{"card.exclusive-fields",
                           "the location is a {field} or a point " + CardProblem::fieldNamed("xCoord") + ", " +
                               CardProblem::fieldNamed("yCoord") + ", " + CardProblem::fieldNamed("zCoord") + " with " +
                               CardProblem::fieldNamed("frameId") + ", not both",
                           "locationNode"};
    if (!card.usesGrid() && card.moduleId != 0)
        return CardProblem{"card.missing-field",
                           CardProblem::fieldNamed("moduleId") + " needs the {field} of the module that holds it",
                           "locationNode"};
    for (const ModuleId& pair : card.points)
    {
        if (const auto problem = checkBoundaryModule(pair.module, "memberModuleId"))
            return problem;
        if (pair.id < 0)
            return CardProblem{"card.bad-id", "{field} must be zero (every grid point of the module) or a point ID",
                               "memberNodeId"};
    }
    return std::nullopt;
}

void ModuleConnectionFormat::write(const ModuleConnectionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, 2, MODULE_CONNECTION_TYPE_WORDS, card.type, ModuleConnectionType::Mrbe2);
    writer.realUnlessDefault(3, card.tolerance, 0.0);
    if (card.usesGrid())
    {
        writer.integer(4, card.locationNode);
        writer.integerUnlessDefault(7, card.moduleId, 0);
    }
    else
    {
        writer.realUnlessDefault(4, card.xCoord, 0.0);
        writer.realUnlessDefault(5, card.yCoord, 0.0);
        writer.realUnlessDefault(6, card.zCoord, 0.0);
        writer.integerUnlessDefault(8, card.frameId, 0);
    }
    int field = FIRST_PAIR_FIELD;
    for (const ModuleId& pair : card.points)
    {
        // A pair of two blanks is the padding of a line, so a pair that would be blank (module 0, every grid point)
        // keeps its module written.
        if (pair.id == 0)
            writer.integer(field, pair.module);
        else
            writer.integerUnlessDefault(field, pair.module, 0);
        writer.integerUnlessDefault(field + 1, pair.id, 0);
        field += 2;
    }
}

std::uint32_t ModuleConnectionTable::append(const ModuleConnectionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleConnectionTable::store(std::uint32_t row, const ModuleConnectionCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleConnectionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleConnectionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleConnectionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleConnectionCard& card = m_cards.edit(row);
    visitId(visitor, "frameId", ReferenceTarget::Coordinate, card.frameId);
    visitBoundaryId(visitor, "locationNode", ReferenceTarget::Node, card.moduleId, card.locationNode);
    // A pair names a grid point or a scalar point; it is looked up only when its module is the main Bulk Data.
    for (ModuleId& pair : card.points)
        visitModuleId(visitor, "memberNodeId", ReferenceTarget::ScalarOrGridPoint, pair);
}

} // namespace oor::model
