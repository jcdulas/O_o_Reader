#include "oor/model/cards/elements/seam/SeamTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <string_view>
#include <variant>

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 14;
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

// The number of a field that holds an integer or a real, as an ID (blank gives 0; a real is not an ID: the caller
// only asks for it when no field of the card is a real).
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

// The three coordinates of a card as a column holds them: a NaN for a blank one.
[[nodiscard]] std::array<double, 3> toColumn(const std::array<std::optional<double>, 3>& values)
{
    return {realToColumn(values[0]), realToColumn(values[1]), realToColumn(values[2])};
}

[[nodiscard]] std::array<std::optional<double>, 3> fromColumn(const std::array<double, 3>& values)
{
    return {realFromColumn(values[0]), realFromColumn(values[1]), realFromColumn(values[2])};
}

} // namespace

std::optional<SeamCard> SeamFormat::read(FieldReader& reader)
{
    SeamCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    // The alternate format of the format reference leaves this cell blank; a seam line name written there is kept.
    card.seamLine = std::string(reader.word(3));
    card.patchKind =
        readKeyword<SeamPatchType>(reader, 4, "patchKind", SEAM_PATCH_TYPE_WORDS, SeamPatchType::Property);
    card.patchAStart = reader.requiredInteger(5, "patchAStart");
    card.patchBStart = reader.integer(6, "patchBStart", 0);
    card.patchAEnd = reader.integer(7, "patchAEnd", 0);
    card.patchBEnd = reader.integer(8, "patchBEnd", 0);

    // Fields 9 and 10 are GS and GE (integers) or XS and YS (reals), and fields 11 to 14 exist only in the alternate
    // format: the card is the alternate one when one of them is used or when field 9 or 10 is a real.
    const IntegerOrReal first = reader.integerOrReal(9, "startNode");
    const IntegerOrReal second = reader.integerOrReal(10, "endNode");
    bool coordinates = isReal(first) || isReal(second);
    for (int field = 11; field <= 14; ++field)
        coordinates = coordinates || !reader.isBlank(field);
    if (coordinates)
    {
        card.ends = SeamEnds::Coordinates;
        card.startXyz = {realOf(first), realOf(second), reader.optionalReal(11, "startZ")};
        card.endXyz = {reader.optionalReal(12, "endX"), reader.optionalReal(13, "endY"),
                       reader.optionalReal(14, "endZ")};
    }
    else
    {
        card.nodes = {idOf(first), idOf(second)};
    }
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SeamFormat::check(const SeamCard& card)
{
    if (card.id <= 0 || card.id >= ID_LIMIT)
        return CardProblem{"card.bad-id", "the element ID must be an integer from 1 to 99999999", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    if (!card.seamLine.empty())
    {
        if (const auto problem = checkNameWord(card.seamLine, "seamLine"))
            return problem;
    }
    if (card.patchAStart <= 0)
        return CardProblem{"card.bad-id", "{field} is required and must be a positive integer", "patchAStart"};
    if (card.patchBStart < 0)
        return CardProblem{"card.bad-id", "{field} must be a positive integer or blank", "patchBStart"};
    if (card.patchAEnd < 0)
        return CardProblem{"card.bad-id", "{field} must be a positive integer or blank", "patchAEnd"};
    if (card.patchBEnd < 0)
        return CardProblem{"card.bad-id", "{field} must be a positive integer or blank", "patchBEnd"};
    if (card.patchKind == SeamPatchType::Element)
    {
        // The patches are named by elements, so there are two of them: a blank or equal IDBS (the two-sided patch of
        // the property form) means nothing here.
        if (card.patchBStart == 0)
            return CardProblem{"card.missing-field", "{field} is required when {field:patchKind} is ELEM",
                               "patchBStart"};
        if (card.patchBStart == card.patchAStart)
            return CardProblem{"card.bad-value",
                               "{field:patchAStart} and {field} must differ when {field:patchKind} is ELEM",
                               "patchBStart"};
        if (card.patchAEnd > 0 && card.patchAEnd == card.patchBEnd)
            return CardProblem{"card.bad-value",
                               "{field:patchAEnd} and {field} must differ when {field:patchKind} is ELEM", "patchBEnd"};
    }
    if (card.hasCoordinates())
    {
        if (card.nodes[0] != 0 || card.nodes[1] != 0)
            return CardProblem{
                "card.bad-value",
                "the alternate format gives coordinates, not the grid points {field} and {field:endNode}", "startNode"};
        if (!anyGiven(card.startXyz) && !anyGiven(card.endXyz))
            return CardProblem{
                "card.missing-field",
                "give at least one of the coordinates {field} to {field:endZ} (a card with none is the format with "
                "the grid points {field:startNode} and {field:endNode})",
                "startX"};
        if (const auto problem = checkOptionalReal(card.startXyz[0], "startX", RealRule::Finite))
            return problem;
        if (const auto problem = checkOptionalReal(card.startXyz[1], "startY", RealRule::Finite))
            return problem;
        if (const auto problem = checkOptionalReal(card.startXyz[2], "startZ", RealRule::Finite))
            return problem;
        if (const auto problem = checkOptionalReal(card.endXyz[0], "endX", RealRule::Finite))
            return problem;
        if (const auto problem = checkOptionalReal(card.endXyz[1], "endY", RealRule::Finite))
            return problem;
        return checkOptionalReal(card.endXyz[2], "endZ", RealRule::Finite);
    }
    if (card.nodes[0] <= 0)
        return CardProblem{"card.bad-id", "{field} is required and must be a positive integer", "startNode"};
    if (card.nodes[1] <= 0)
        return CardProblem{"card.bad-id", "{field} is required and must be a positive integer", "endNode"};
    if (anyGiven(card.startXyz) || anyGiven(card.endXyz))
        return CardProblem{"card.bad-value",
                           "the format with the grid points {field:startNode} and {field:endNode} has no coordinates",
                           "startX"};
    return std::nullopt;
}

void SeamFormat::write(const SeamCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    if (!card.seamLine.empty())
        writer.text(3, card.seamLine);
    writeKeywordUnlessDefault(writer, 4, SEAM_PATCH_TYPE_WORDS, card.patchKind, SeamPatchType::Property);
    writer.integer(5, card.patchAStart);
    if (card.patchBStart != 0)
        writer.integer(6, card.patchBStart);
    if (card.patchAEnd != 0)
        writer.integer(7, card.patchAEnd);
    if (card.patchBEnd != 0)
        writer.integer(8, card.patchBEnd);
    if (card.hasCoordinates())
    {
        for (std::size_t index = 0; index < card.startXyz.size(); ++index)
        {
            if (card.startXyz[index])
                writer.real(9 + static_cast<int>(index), *card.startXyz[index]);
            if (card.endXyz[index])
                writer.real(12 + static_cast<int>(index), *card.endXyz[index]);
        }
        return;
    }
    writer.integer(9, card.nodes[0]);
    writer.integer(10, card.nodes[1]);
}

std::uint32_t SeamTable::append(const SeamCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_seamLine.push(card.seamLine);
    m_patchKind.push(static_cast<std::uint8_t>(card.patchKind));
    m_patchAStart.push(card.patchAStart);
    m_patchBStart.push(card.patchBStart);
    m_patchAEnd.push(card.patchAEnd);
    m_patchBEnd.push(card.patchBEnd);
    m_ends.push(static_cast<std::uint8_t>(card.ends));
    m_nodes.push(card.nodes);
    m_start.push(toColumn(card.startXyz));
    m_end.push(toColumn(card.endXyz));
    return row;
}

SeamCard SeamTable::card(std::uint32_t row) const
{
    SeamCard card;
    card.id = m_id.get(row);
    card.propId = m_propId.get(row);
    card.seamLine = m_seamLine.get(row);
    card.patchKind = static_cast<SeamPatchType>(m_patchKind.get(row));
    card.patchAStart = m_patchAStart.get(row);
    card.patchBStart = m_patchBStart.get(row);
    card.patchAEnd = m_patchAEnd.get(row);
    card.patchBEnd = m_patchBEnd.get(row);
    card.ends = static_cast<SeamEnds>(m_ends.get(row));
    card.nodes = m_nodes.get(row);
    card.startXyz = fromColumn(m_start.get(row));
    card.endXyz = fromColumn(m_end.get(row));
    return card;
}

void SeamTable::store(std::uint32_t row, const SeamCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_seamLine.set(row, card.seamLine);
    m_patchKind.set(row, static_cast<std::uint8_t>(card.patchKind));
    m_patchAStart.set(row, card.patchAStart);
    m_patchBStart.set(row, card.patchBStart);
    m_patchAEnd.set(row, card.patchAEnd);
    m_patchBEnd.set(row, card.patchBEnd);
    m_ends.set(row, static_cast<std::uint8_t>(card.ends));
    m_nodes.set(row, card.nodes);
    m_start.set(row, toColumn(card.startXyz));
    m_end.set(row, toColumn(card.endXyz));
}

void SeamTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_seamLine.keepRows(rows);
    m_patchKind.keepRows(rows);
    m_patchAStart.keepRows(rows);
    m_patchBStart.keepRows(rows);
    m_patchAEnd.keepRows(rows);
    m_patchBEnd.keepRows(rows);
    m_ends.keepRows(rows);
    m_nodes.keepRows(rows);
    m_start.keepRows(rows);
    m_end.keepRows(rows);
}

void SeamTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId)); // a PSEAM
    // The four patch numbers are the properties of PSHELL cards or element IDs, as CTYPE says.
    const ReferenceTarget patchTarget = m_patchKind.get(row) == static_cast<std::uint8_t>(SeamPatchType::Element)
                                            ? ReferenceTarget::Element
                                            : ReferenceTarget::Property;
    const auto visitPatch = [&](std::string_view field, Column<base::EntityId>& column)
    {
        if (const base::EntityId id = column.get(row); id > 0)
            column.set(row, visitor.visit(field, patchTarget, id));
    };
    visitPatch("patchAStart", m_patchAStart);
    visitPatch("patchBStart", m_patchBStart);
    visitPatch("patchAEnd", m_patchAEnd);
    visitPatch("patchBEnd", m_patchBEnd);
    // GS and GE are blank (0) in the form that gives the coordinates.
    std::array<base::EntityId, 2> nodes = m_nodes.get(row);
    bool changed = false;
    for (base::EntityId& node : nodes)
    {
        if (node <= 0)
            continue;
        const base::EntityId updated = visitor.visit("nodes", ReferenceTarget::Node, node);
        changed = changed || updated != node;
        node = updated;
    }
    if (changed)
        m_nodes.set(row, nodes);
}

} // namespace oor::model
