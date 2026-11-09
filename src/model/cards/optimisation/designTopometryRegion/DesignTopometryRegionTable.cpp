#include "oor/model/cards/optimisation/designTopometryRegion/DesignTopometryRegionTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/optimisation/DesignTargets.hpp"

#include <algorithm>
#include <array>

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_LINE = 8;
constexpr int FIRST_LINE_FIELD = 9; // the keyword of the first line after the card line
constexpr int SYMMETRY_PROPS_PER_LATER_LINE = FIELDS_PER_LINE - 1;

constexpr std::array<std::string_view, 3> MIRROR_PLANES{"PlaneXy", "PlaneYz", "PlaneZx"};
constexpr std::array<std::string_view, 3> AXES{"XDirection", "YDirection", "ZDirection"};

// The keywords of the lines, in the order the card writes them.
enum class LineKind : std::uint8_t
{
    Link,
    DiscreteValues,
    Stress,
    Symmetry
};

// The key words of the lines after the first: the first two are cards, by their neutral names; Stress and Symmetry are
// words of the format.
constexpr std::array<std::string_view, 4> LINE_WORDS{"DesignVarLink", "DesignDiscreteValues", "Stress", "Symmetry"};

template <std::size_t N>
bool isOneOf(const std::string& word, const std::array<std::string_view, N>& words) noexcept
{
    return std::find(words.begin(), words.end(), word) != words.end();
}

bool isBlankLine(const FieldReader& reader, int line)
{
    for (int field = line; field < line + FIELDS_PER_LINE; ++field)
    {
        if (!reader.isBlank(field))
            return false;
    }
    return true;
}

// A line has its keyword and `values` values; the rest of the line is blank.
void requireBlankRest(FieldReader& reader, int line, int values, std::string_view keyword)
{
    for (int field = line + 1 + values; field < line + FIELDS_PER_LINE; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "a " + std::string(keyword) + " line of a " + std::string(reader.cardName()) + " has " +
                             std::to_string(values) + " values",
                         "lineKeyword");
            return;
        }
    }
}

// The field of the SYMPID number `index` (0 is the first) of a SYM line that starts at `line`: one on the keyword line,
// then seven on each line after it, after a blank first field.
int symmetricPropertyField(int line, std::size_t index)
{
    if (index == 0)
        return line + FIELDS_PER_LINE - 1;
    const int later = static_cast<int>(index) - 1;
    return line + FIELDS_PER_LINE * (1 + later / SYMMETRY_PROPS_PER_LATER_LINE) + 1 +
           later % SYMMETRY_PROPS_PER_LATER_LINE;
}

DesignTopometryRegionSymmetry readSymmetry(FieldReader& reader, int line)
{
    DesignTopometryRegionSymmetry symmetry;
    symmetry.frameId = reader.integer(line + 1, "frameId", 0);
    for (int plane = 0; plane < 3; ++plane)
        symmetry.mirrorPlanes[static_cast<std::size_t>(plane)] = std::string(reader.text(line + 2 + plane));
    symmetry.cyclicAxis = std::string(reader.text(line + 5));
    symmetry.cyclicSegments = reader.integer(line + 6, "cyclicSegments", 0);
    if (!reader.isBlank(line + 7))
        symmetry.symmetricProperties.push_back(reader.requiredInteger(line + 7, "symmetryPropId"));
    return symmetry;
}

} // namespace

std::optional<DesignTopometryRegionCard> DesignTopometryRegionFormat::read(FieldReader& reader)
{
    DesignTopometryRegionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.kind = std::string(reader.text(2));
    if (card.kind.empty())
        reader.error("card.missing-field", std::string("kind") + " is required", "kind");
    card.propId = reader.requiredInteger(3, "propId");
    card.field = readDesignField(reader, 4, "fieldName");
    card.initialValue = reader.optionalReal(5, "initialValue");
    card.lowerBound = reader.optionalReal(6, "lowerBound");
    card.upperBound = reader.optionalReal(7, "upperBound");
    card.designVarMoveLimit = reader.real(8, "designVarMoveLimit", DesignTopometryRegionCard::DEFAULT_MOVE_LIMIT);
    card.resolveDefaultBounds();

    std::array<bool, LINE_WORDS.size()> seen{};
    bool inSymmetry = false; // the lines with a blank first field go on with the SYMPIDs of a SYM line
    for (int line = FIRST_LINE_FIELD; line <= reader.lastUsedField() && !reader.failed(); line += FIELDS_PER_LINE)
    {
        if (isBlankLine(reader, line))
            continue; // padding
        if (reader.isBlank(line))
        {
            if (!inSymmetry)
            {
                reader.error(
                    "card.bad-value",
                    "a line of a " + std::string(reader.cardName()) + " starts with " + keywordList(LINE_WORDS),
                    "lineKeyword");
                break;
            }
            for (int field = line + 1; field < line + FIELDS_PER_LINE; ++field)
            {
                if (!reader.isBlank(field))
                    card.symmetry->symmetricProperties.push_back(reader.requiredInteger(field, "symmetryPropId"));
            }
            continue;
        }
        const std::optional<LineKind> kind = parseKeyword<LineKind>(LINE_WORDS, reader.text(line));
        if (!kind)
        {
            reader.error("card.bad-value",
                         "a line of a " + std::string(reader.cardName()) + " starts with " + keywordList(LINE_WORDS) +
                             ", not \"" + std::string(reader.text(line)) + "\"",
                         "lineKeyword");
            break;
        }
        if (seen[static_cast<std::size_t>(*kind)])
        {
            reader.error("card.bad-value", std::string(keywordText(LINE_WORDS, *kind)) + " is given twice",
                         "lineKeyword");
            break;
        }
        seen[static_cast<std::size_t>(*kind)] = true;
        inSymmetry = *kind == LineKind::Symmetry;
        const std::string_view word = keywordText(LINE_WORDS, *kind);
        switch (*kind)
        {
        case LineKind::Link:
            card.link = DesignTopometryRegionLink{reader.requiredInteger(line + 1, "tableId"),
                                                  reader.real(line + 2, "constant0", 0.0),
                                                  reader.requiredReal(line + 3, "coefficient1")};
            requireBlankRest(reader, line, 3, word);
            break;
        case LineKind::DiscreteValues:
            card.discreteValuesId = reader.requiredInteger(line + 1, "discreteValuesId");
            if (!reader.failed() && card.discreteValuesId <= 0)
                reader.error("card.bad-id", std::string("discreteValuesId") + " must be a positive integer",
                             "discreteValuesId");
            requireBlankRest(reader, line, 1, word);
            break;
        case LineKind::Stress:
            card.stressLimit = reader.requiredReal(line + 1, "stressLimit");
            requireBlankRest(reader, line, 1, word);
            break;
        case LineKind::Symmetry:
            card.symmetry = readSymmetry(reader, line);
            break;
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignTopometryRegionFormat::check(const DesignTopometryRegionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkNeutralName(card.kind, "kind"))
        return problem;
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    if (const auto problem = checkDesignField(card.field, "fieldName"))
        return problem;
    if (const auto problem = checkOptionalReal(card.initialValue, "initialValue", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.lowerBound, "lowerBound", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.upperBound, "upperBound", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.designVarMoveLimit, "designVarMoveLimit", RealRule::Positive))
        return problem;
    if (card.link)
    {
        if (const auto problem = checkRequiredId(card.link->tableId, "tableId"))
            return problem;
        if (const auto problem = checkReal(card.link->constant0, "constant0", RealRule::Finite))
            return problem;
        if (const auto problem = checkReal(card.link->coefficient1, "coefficient1", RealRule::Finite))
            return problem;
    }
    if (const auto problem = checkOptionalId(card.discreteValuesId, "discreteValuesId"))
        return problem;
    if (const auto problem = checkOptionalReal(card.stressLimit, "stressLimit", RealRule::Positive))
        return problem;
    if (card.symmetry)
    {
        const DesignTopometryRegionSymmetry& symmetry = *card.symmetry;
        if (symmetry.frameId < 0)
            return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "frameId"};
        for (const std::string& plane : symmetry.mirrorPlanes)
        {
            if (!plane.empty() && !isOneOf(plane, MIRROR_PLANES))
                return CardProblem{"card.bad-value",
                                   std::string(CardProblem::FIELD) + " must be XY, YZ or ZX, not " + plane,
                                   "mirrorPlanes"};
        }
        if (!symmetry.cyclicAxis.empty() && !isOneOf(symmetry.cyclicAxis, AXES))
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must be X, Y or Z, not " + symmetry.cyclicAxis,
                               "cyclicAxis"};
        if (symmetry.cyclicSegments < 0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                               "cyclicSegments"};
        if (symmetry.cyclicAxis.empty() != (symmetry.cyclicSegments == 0))
            return CardProblem{"card.bad-value",
                               CardProblem::fieldNamed("cyclicAxis") + " and " + std::string(CardProblem::FIELD) +
                                   " go together: the axis and the number of segments",
                               "cyclicSegments"};
        for (const base::EntityId property : symmetry.symmetricProperties)
        {
            if (const auto problem = checkRequiredId(property, "symmetryPropId"))
                return problem;
        }
    }
    return std::nullopt;
}

void DesignTopometryRegionFormat::write(const DesignTopometryRegionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.kind);
    writer.integer(3, card.propId);
    writeDesignField(writer, 4, card.field);
    if (card.initialValue)
        writer.real(5, *card.initialValue);
    if (card.lowerBound)
        writer.real(6, *card.lowerBound);
    if (card.upperBound)
        writer.real(7, *card.upperBound);
    writer.realUnlessDefault(8, card.designVarMoveLimit, DesignTopometryRegionCard::DEFAULT_MOVE_LIMIT);

    int line = FIRST_LINE_FIELD;
    if (card.link)
    {
        writer.text(line, keywordText(LINE_WORDS, LineKind::Link));
        writer.integer(line + 1, card.link->tableId);
        writer.realUnlessDefault(line + 2, card.link->constant0, 0.0);
        writer.real(line + 3, card.link->coefficient1);
        line += FIELDS_PER_LINE;
    }
    if (card.discreteValuesId != 0)
    {
        writer.text(line, keywordText(LINE_WORDS, LineKind::DiscreteValues));
        writer.integer(line + 1, card.discreteValuesId);
        line += FIELDS_PER_LINE;
    }
    if (card.stressLimit)
    {
        writer.text(line, keywordText(LINE_WORDS, LineKind::Stress));
        writer.real(line + 1, *card.stressLimit);
        line += FIELDS_PER_LINE;
    }
    if (card.symmetry)
    {
        const DesignTopometryRegionSymmetry& symmetry = *card.symmetry;
        writer.text(line, keywordText(LINE_WORDS, LineKind::Symmetry));
        writer.integerUnlessDefault(line + 1, symmetry.frameId, 0);
        for (int plane = 0; plane < 3; ++plane)
            writer.text(line + 2 + plane, symmetry.mirrorPlanes[static_cast<std::size_t>(plane)]);
        writer.text(line + 5, symmetry.cyclicAxis);
        writer.integerUnlessDefault(line + 6, symmetry.cyclicSegments, 0);
        for (std::size_t index = 0; index < symmetry.symmetricProperties.size(); ++index)
            writer.integer(symmetricPropertyField(line, index), symmetry.symmetricProperties[index]);
    }
}

std::uint32_t DesignTopometryRegionTable::append(const DesignTopometryRegionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_cards.push(card);
    return row;
}

void DesignTopometryRegionTable::store(std::uint32_t row, const DesignTopometryRegionCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_cards.set(row, card);
}

void DesignTopometryRegionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_cards.keepRows(rows);
}

void DesignTopometryRegionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void DesignTopometryRegionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    DesignTopometryRegionCard& card = m_cards.edit(row);
    visitId(visitor, "propId", ReferenceTarget::Property, card.propId);
    m_propId.set(row, card.propId);
    visitId(visitor, "discreteValuesId", ReferenceTarget::DiscreteValueSet, card.discreteValuesId);
    if (card.symmetry)
    {
        visitId(visitor, "frameId", ReferenceTarget::Coordinate, card.symmetry->frameId);
        for (base::EntityId& property : card.symmetry->symmetricProperties)
            visitId(visitor, "symmetryPropId", ReferenceTarget::Property, property);
    }
}

} // namespace oor::model
