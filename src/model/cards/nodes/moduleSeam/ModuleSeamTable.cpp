#include "oor/model/cards/nodes/moduleSeam/ModuleSeamTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/cards/nodes/ModuleConnectionSupport.hpp"

#include <string_view>
#include <variant>

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 16;
constexpr base::EntityId ID_LIMIT = 100000000; // an element ID is below it (format reference, EID)

using IntegerOrReal = std::optional<std::variant<std::int32_t, double>>;

[[nodiscard]] bool isReal(const IntegerOrReal& value)
{
    return value && std::holds_alternative<double>(*value);
}

// The number of a field that holds an integer or a real, as a real (blank gives nothing).
[[nodiscard]] std::optional<double> realOf(const IntegerOrReal& value)
{
    if (!value)
        return std::nullopt;
    if (const std::int32_t* integer = std::get_if<std::int32_t>(&*value))
        return static_cast<double>(*integer);
    return std::get<double>(*value);
}

// The number of a field that holds an integer or a real, as an ID (blank gives 0; a real is not an ID: the caller only
// asks for it when no field of the card is a real).
[[nodiscard]] base::EntityId idOf(const IntegerOrReal& value)
{
    if (!value)
        return 0;
    return std::get<std::int32_t>(*value);
}

[[nodiscard]] bool anyGiven(const std::array<std::optional<double>, 3>& values)
{
    return values[0] || values[1] || values[2];
}

} // namespace

std::optional<ModuleSeamCard> ModuleSeamFormat::read(FieldReader& reader)
{
    ModuleSeamCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    // The alternate format of the format reference leaves this cell blank; a seam line name written there is kept.
    card.seamLineName = std::string(reader.word(3));
    card.patchKind = readKeyword<ModuleSeamPatchType>(reader, 4, "patchKind", MODULE_SEAM_PATCH_TYPE_WORDS,
                                                          ModuleSeamPatchType::Property);
    card.startPatchIdA = reader.requiredInteger(5, "startPatchIdA");
    card.startPatchIdB = reader.integer(6, "startPatchIdB", 0);
    card.endPatchIdA = reader.integer(7, "endPatchIdA", 0);
    card.endPatchIdB = reader.integer(8, "endPatchIdB", 0);

    // Fields 9 and 10 are GS and GE (integers) or XS and YS (reals), and fields 11 to 14 exist only in the alternate
    // format: the card is the alternate one when one of them is used or when field 9 or 10 is a real.
    const IntegerOrReal first = reader.integerOrReal(9, "startNode");
    const IntegerOrReal second = reader.integerOrReal(10, "endNode");
    bool coordinates = isReal(first) || isReal(second);
    for (int field = 11; field <= 14; ++field)
        coordinates = coordinates || !reader.isBlank(field);
    if (coordinates)
    {
        card.ends = ModuleSeamEnds::Coordinates;
        card.startXyz = {realOf(first), realOf(second), reader.optionalReal(11, "zStation")};
        card.endXyz = {reader.optionalReal(12, "xEnd"), reader.optionalReal(13, "yEnd"),
                       reader.optionalReal(14, "zEnd")};
    }
    else
    {
        card.startNode = idOf(first);
        card.endNode = idOf(second);
    }
    card.moduleIdA = reader.integer(15, "moduleIdA", 0);
    card.moduleIdB = reader.integer(16, "moduleIdB", 0);
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleSeamFormat::check(const ModuleSeamCard& card)
{
    if (card.id <= 0 || card.id >= ID_LIMIT)
        return CardProblem{"card.bad-id", "the element ID must be an integer from 1 to 99999999", "id"};
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    if (!card.seamLineName.empty())
    {
        if (const auto problem = checkNameWord(card.seamLineName, "seamLineName"))
            return problem;
    }
    if (const auto problem = checkRequiredId(card.startPatchIdA, "startPatchIdA"))
        return problem;
    if (const auto problem = checkOptionalId(card.startPatchIdB, "startPatchIdB"))
        return problem;
    if (const auto problem = checkOptionalId(card.endPatchIdA, "endPatchIdA"))
        return problem;
    if (const auto problem = checkOptionalId(card.endPatchIdB, "endPatchIdB"))
        return problem;
    if (const auto problem = checkModuleNumber(card.moduleIdA, "moduleIdA"))
        return problem;
    if (const auto problem = checkModuleNumber(card.moduleIdB, "moduleIdB"))
        return problem;
    if (card.patchKind == ModuleSeamPatchType::Element)
    {
        // The patches are named by elements, so there are two of them: a blank or equal IDBS (the two-sided patch of
        // the property form) means nothing here. Equal numbers are only the same element within one module.
        if (card.startPatchIdB == 0)
            return CardProblem{"card.missing-field",
                               "{field} is required when " + CardProblem::fieldNamed("patchKind") + " is ELEM",
                               "startPatchIdB"};
        if (card.moduleIdA == card.moduleIdB && card.startPatchIdB == card.startPatchIdA)
            return CardProblem{"card.bad-value",
                               CardProblem::fieldNamed("startPatchIdA") + " and {field} must differ when " +
                                   CardProblem::fieldNamed("patchKind") +
                                   " is ELEM and they are in the same "
                                   "module",
                               "startPatchIdB"};
        if (card.moduleIdA == card.moduleIdB && card.endPatchIdA > 0 && card.endPatchIdA == card.endPatchIdB)
            return CardProblem{"card.bad-value",
                               CardProblem::fieldNamed("endPatchIdA") + " and {field} must differ when " +
                                   CardProblem::fieldNamed("patchKind") +
                                   " is ELEM and they are in the same "
                                   "module",
                               "endPatchIdB"};
    }
    if (card.hasCoordinates())
    {
        if (card.startNode != 0 || card.endNode != 0)
            return CardProblem{"card.bad-value",
                               "the alternate format gives coordinates, not the grid points {field} and " +
                                   CardProblem::fieldNamed("endNode"),
                               "startNode"};
        if (!anyGiven(card.startXyz) && !anyGiven(card.endXyz))
            return CardProblem{"card.missing-field",
                               "give at least one of the coordinates {field} to " + CardProblem::fieldNamed("zEnd") +
                                   " (a card with none is the format with the "
                                   "grid points " +
                                   CardProblem::fieldNamed("startNode") + " and " + CardProblem::fieldNamed("endNode") +
                                   ")",
                               "xStation"};
        if (const auto problem = checkOptionalReal(card.startXyz[0], "xStation", RealRule::Finite))
            return problem;
        if (const auto problem = checkOptionalReal(card.startXyz[1], "yStation", RealRule::Finite))
            return problem;
        if (const auto problem = checkOptionalReal(card.startXyz[2], "zStation", RealRule::Finite))
            return problem;
        if (const auto problem = checkOptionalReal(card.endXyz[0], "xEnd", RealRule::Finite))
            return problem;
        if (const auto problem = checkOptionalReal(card.endXyz[1], "yEnd", RealRule::Finite))
            return problem;
        return checkOptionalReal(card.endXyz[2], "zEnd", RealRule::Finite);
    }
    if (card.startNode <= 0)
        return CardProblem{"card.bad-id", "{field} is required and must be a positive integer", "startNode"};
    if (card.endNode <= 0)
        return CardProblem{"card.bad-id", "{field} is required and must be a positive integer", "endNode"};
    if (anyGiven(card.startXyz) || anyGiven(card.endXyz))
        return CardProblem{"card.bad-value",
                           "the format with the grid points " + CardProblem::fieldNamed("startNode") + " and " +
                               CardProblem::fieldNamed("endNode") + " has no coordinates",
                           "xStation"};
    return std::nullopt;
}

void ModuleSeamFormat::write(const ModuleSeamCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    if (!card.seamLineName.empty())
        writer.text(3, card.seamLineName);
    writeKeywordUnlessDefault(writer, 4, MODULE_SEAM_PATCH_TYPE_WORDS, card.patchKind,
                              ModuleSeamPatchType::Property);
    writer.integer(5, card.startPatchIdA);
    if (card.startPatchIdB != 0)
        writer.integer(6, card.startPatchIdB);
    if (card.endPatchIdA != 0)
        writer.integer(7, card.endPatchIdA);
    if (card.endPatchIdB != 0)
        writer.integer(8, card.endPatchIdB);
    if (card.hasCoordinates())
    {
        for (std::size_t index = 0; index < card.startXyz.size(); ++index)
        {
            if (card.startXyz[index])
                writer.real(9 + static_cast<int>(index), *card.startXyz[index]);
            if (card.endXyz[index])
                writer.real(12 + static_cast<int>(index), *card.endXyz[index]);
        }
    }
    else
    {
        writer.integer(9, card.startNode);
        writer.integer(10, card.endNode);
    }
    writer.integerUnlessDefault(15, card.moduleIdA, 0);
    writer.integerUnlessDefault(16, card.moduleIdB, 0);
}

std::uint32_t ModuleSeamTable::append(const ModuleSeamCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleSeamTable::store(std::uint32_t row, const ModuleSeamCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleSeamTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleSeamTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleSeamTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleSeamCard& card = m_cards.edit(row);
    visitId(visitor, "propId", ReferenceTarget::Property, card.propId); // a PSEAM of the main Bulk Data
    // The four patch numbers are the properties of PSHELL cards or element IDs, as CTYPE says, in the module MODA (the
    // patch A) or MODB (the patch B): looked up only in the main Bulk Data (ModuleConnectionSupport.hpp).
    const ReferenceTarget patches =
        card.patchKind == ModuleSeamPatchType::Element ? ReferenceTarget::Element : ReferenceTarget::Property;
    visitIdInModule(visitor, "startPatchIdA", patches, card.moduleIdA, card.startPatchIdA);
    visitIdInModule(visitor, "startPatchIdB", patches, card.moduleIdB, card.startPatchIdB);
    visitIdInModule(visitor, "endPatchIdA", patches, card.moduleIdA, card.endPatchIdA);
    visitIdInModule(visitor, "endPatchIdB", patches, card.moduleIdB, card.endPatchIdB);
    // GS and GE are blank (0) in the form that gives the coordinates.
    visitId(visitor, "startNode", ReferenceTarget::Node, card.startNode);
    visitId(visitor, "endNode", ReferenceTarget::Node, card.endNode);
}

} // namespace oor::model
