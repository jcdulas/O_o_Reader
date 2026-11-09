#include "oor/model/cards/optimisation/designBeadRegion/DesignBeadRegionTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <algorithm>

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_LINE = 8;
constexpr int FIRST_LINE_FIELD = 9; // the keyword of the first line after the card line
constexpr double MAXIMUM_DRAW_ANGLE = 90.0;
// The key words of the two lines after the card line: the design line and the line of the node sets.
constexpr std::string_view DESIGN_LINE_KEY = "DesignVar";
constexpr std::string_view NODES_LINE_KEY = "Nodes";

bool isBlankLine(const FieldReader& reader, int line)
{
    for (int field = line; field < line + FIELDS_PER_LINE; ++field)
    {
        if (!reader.isBlank(field))
            return false;
    }
    return true;
}

// The fields after the `values` of a line must be blank.
void requireBlankRest(FieldReader& reader, int line, int values, const std::string& keyword)
{
    for (int field = line + 1 + values; field < line + FIELDS_PER_LINE; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "a " + keyword + " line of a " + std::string(reader.cardName()) + " has " +
                             std::to_string(values) + " values",
                         "lineKeyword");
            return;
        }
    }
}

BeadDesignLine readDesignLine(FieldReader& reader, int line)
{
    BeadDesignLine design;
    // The direction is the word NORM (or a blank): the normals of the elements, or the vector XD, YD, ZD.
    const std::string direction(reader.text(line + 1));
    if (direction.empty() || direction == "ElementNormals")
    {
        if (!reader.isBlank(line + 2) || !reader.isBlank(line + 3))
            reader.error("card.bad-value",
                         std::string("directionY") + " and " + std::string("directionZ") + " go with an " +
                             std::string("directionX") + ": after NORM or a blank field they are blank",
                         "directionY");
    }
    else
    {
        design.direction = std::array<double, 3>{reader.requiredReal(line + 1, "directionX"),
                                                 reader.requiredReal(line + 2, "directionY"),
                                                 reader.requiredReal(line + 3, "directionZ")};
    }
    design.frameId = reader.integer(line + 4, "frameId", 0);
    design.lowerBound = reader.real(line + 5, "lowerBound", BeadDesignLine::DEFAULT_LOWER_BOUND);
    design.upperBound = reader.real(line + 6, "upperBound", BeadDesignLine::DEFAULT_UPPER_BOUND);
    design.designVarMoveLimit = reader.real(line + 7, "designVarMoveLimit", BeadDesignLine::DEFAULT_MOVE_LIMIT);
    return design;
}

BeadGridLine readGridLine(FieldReader& reader, int line)
{
    BeadGridLine grids;
    grids.excludedNodeSetId = reader.integer(line + 1, "excludedNodeSetId", 0);
    grids.addedNodeSetId = reader.integer(line + 2, "addedNodeSetId", 0);
    requireBlankRest(reader, line, 2, std::string(NODES_LINE_KEY));
    return grids;
}

// "ShellProp, ShearPanelProp, CompProp or CompPropGlobal" as check() writes them: the reader puts the deck keywords
// there.
std::string propertyTypePhrase()
{
    std::string phrase;
    for (std::size_t index = 0; index < DESIGN_BEAD_REGION_PROPERTY_TYPES.size(); ++index)
    {
        if (index > 0)
            phrase += index + 1 == DESIGN_BEAD_REGION_PROPERTY_TYPES.size() ? " or " : ", ";
        phrase += CardProblem::cardNamed(DESIGN_BEAD_REGION_PROPERTY_TYPES[index]);
    }
    return phrase;
}

} // namespace

std::optional<DesignBeadRegionCard> DesignBeadRegionFormat::read(FieldReader& reader)
{
    DesignBeadRegionCard card;
    card.id = reader.requiredInteger(1, "id");
    // PTYPE is a property card, by its neutral card name (the check reports a name that is none of the four).
    card.propType = std::string(reader.text(2));
    card.propId = reader.requiredInteger(3, "propId");
    card.minWidth = reader.requiredReal(4, "minWidth");
    card.maxHeight = reader.requiredReal(5, "maxHeight");
    card.angle = reader.requiredReal(6, "angle");
    card.bufferZone = readKeyword<BeadBuffer>(reader, 7, "bufferZone", BEAD_BUFFER_WORDS, BeadBuffer::Yes);
    card.skippedNodes = readKeyword<BeadSkip>(reader, 8, "skippedNodes", BEAD_SKIP_WORDS, BeadSkip::Both);
    for (int line = FIRST_LINE_FIELD; line <= reader.lastUsedField() && !reader.failed(); line += FIELDS_PER_LINE)
    {
        if (isBlankLine(reader, line))
            continue; // padding
        const std::string keyword(reader.text(line));
        const std::string designWord(DESIGN_LINE_KEY);
        const std::string gridWord(NODES_LINE_KEY);
        if (keyword == designWord && !card.design)
        {
            card.design = readDesignLine(reader, line);
        }
        else if (keyword == gridWord && !card.grids)
        {
            card.grids = readGridLine(reader, line);
        }
        else
        {
            reader.error("card.bad-value",
                         keyword == designWord || keyword == gridWord
                             ? keyword + " is given twice"
                             : "a line of a " + std::string(reader.cardName()) + " starts with " + designWord + " or " +
                                   gridWord + ", not \"" + keyword + "\"",
                         "lineKeyword");
            break;
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignBeadRegionFormat::check(const DesignBeadRegionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (std::find(DESIGN_BEAD_REGION_PROPERTY_TYPES.begin(), DESIGN_BEAD_REGION_PROPERTY_TYPES.end(), card.propType) ==
        DESIGN_BEAD_REGION_PROPERTY_TYPES.end())
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be " + propertyTypePhrase() + ", not " +
                               CardProblem::cardNamed(card.propType),
                           "propType"};
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    if (const auto problem = checkReal(card.minWidth, "minWidth", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.maxHeight, "maxHeight", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.angle, "angle", RealRule::Positive))
        return problem;
    if (card.angle >= MAXIMUM_DRAW_ANGLE)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be below 90 degrees", "angle"};
    if (card.design)
    {
        const BeadDesignLine& design = *card.design;
        if (design.direction)
        {
            static constexpr std::array<const char*, 3> NAMES{"directionX", "directionY", "directionZ"};
            for (std::size_t index = 0; index < NAMES.size(); ++index)
            {
                if (const auto problem = checkReal((*design.direction)[index], NAMES[index], RealRule::Finite))
                    return problem;
            }
        }
        if (const auto problem = checkOptionalId(design.frameId, "frameId"))
            return problem;
        if (const auto problem = checkReal(design.lowerBound, "lowerBound", RealRule::Finite))
            return problem;
        if (const auto problem = checkReal(design.upperBound, "upperBound", RealRule::Finite))
            return problem;
        if (design.lowerBound >= design.upperBound)
            return CardProblem{
                "card.bad-value",
                std::string(CardProblem::FIELD) + " must be below " + CardProblem::fieldNamed("upperBound"),
                "lowerBound"};
        if (const auto problem = checkReal(design.designVarMoveLimit, "designVarMoveLimit", RealRule::Positive))
            return problem;
    }
    if (card.grids)
    {
        if (const auto problem = checkOptionalId(card.grids->excludedNodeSetId, "excludedNodeSetId"))
            return problem;
        if (const auto problem = checkOptionalId(card.grids->addedNodeSetId, "addedNodeSetId"))
            return problem;
    }
    return std::nullopt;
}

void DesignBeadRegionFormat::write(const DesignBeadRegionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.propType);
    writer.integer(3, card.propId);
    writer.real(4, card.minWidth);
    writer.real(5, card.maxHeight);
    writer.real(6, card.angle);
    writeKeywordUnlessDefault(writer, 7, BEAD_BUFFER_WORDS, card.bufferZone, BeadBuffer::Yes);
    writeKeywordUnlessDefault(writer, 8, BEAD_SKIP_WORDS, card.skippedNodes, BeadSkip::Both);
    int line = FIRST_LINE_FIELD;
    if (card.design)
    {
        const BeadDesignLine& design = *card.design;
        writer.text(line, DESIGN_LINE_KEY);
        if (design.direction)
        {
            for (std::size_t index = 0; index < design.direction->size(); ++index)
                writer.real(line + 1 + static_cast<int>(index), (*design.direction)[index]);
        }
        writer.integerUnlessDefault(line + 4, design.frameId, 0);
        writer.realUnlessDefault(line + 5, design.lowerBound, BeadDesignLine::DEFAULT_LOWER_BOUND);
        writer.realUnlessDefault(line + 6, design.upperBound, BeadDesignLine::DEFAULT_UPPER_BOUND);
        writer.realUnlessDefault(line + 7, design.designVarMoveLimit, BeadDesignLine::DEFAULT_MOVE_LIMIT);
        line += FIELDS_PER_LINE;
    }
    if (card.grids)
    {
        writer.text(line, NODES_LINE_KEY);
        writer.integerUnlessDefault(line + 1, card.grids->excludedNodeSetId, 0);
        writer.integerUnlessDefault(line + 2, card.grids->addedNodeSetId, 0);
    }
}

std::uint32_t DesignBeadRegionTable::append(const DesignBeadRegionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_cards.push(card);
    return row;
}

void DesignBeadRegionTable::store(std::uint32_t row, const DesignBeadRegionCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_cards.set(row, card);
}

void DesignBeadRegionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_cards.keepRows(rows);
}

void DesignBeadRegionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void DesignBeadRegionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    DesignBeadRegionCard& card = m_cards.edit(row);
    visitId(visitor, "propId", ReferenceTarget::Property, card.propId);
    m_propId.set(row, card.propId);
    if (card.design)
        visitId(visitor, "frameId", ReferenceTarget::Coordinate, card.design->frameId);
    if (card.grids)
    {
        visitId(visitor, "excludedNodeSetId", ReferenceTarget::Set, card.grids->excludedNodeSetId);
        visitId(visitor, "addedNodeSetId", ReferenceTarget::Set, card.grids->addedNodeSetId);
    }
}

} // namespace oor::model
