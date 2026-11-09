#include "oor/model/cards/optimisation/designTopologyRegion/DesignTopologyRegionTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/optimisation/DesignTargets.hpp"
#include "oor/model/cards/optimisation/DesignValue.hpp"

#include <algorithm>
#include <array>

namespace oor::model
{

namespace
{

constexpr int FIRST_LINE_FIELD = 9; // the keyword of the first line after the card line
constexpr int FIELDS_PER_LINE = 8;

constexpr std::array<std::string_view, 3> MIRROR_PLANES{"PlaneXy", "PlaneYz", "PlaneZx"};
constexpr std::array<std::string_view, 3> AXES{"XDirection", "YDirection", "ZDirection"};
constexpr std::array<std::string_view, 6> DIRECTIONS{"XDirection",         "YDirection",         "ZDirection",
                                                     "NegativeXDirection", "NegativeYDirection", "NegativeZDirection"};
constexpr std::array<std::string_view, 2> ALIGNMENTS{"Yes", "No"};

template <std::size_t N>
bool isOneOf(const std::string& word, const std::array<std::string_view, N>& words) noexcept
{
    return std::find(words.begin(), words.end(), word) != words.end();
}

// A word of a line that may be blank: as written, empty when blank.
std::string wordText(const FieldReader& reader, int field)
{
    return std::string(reader.text(field));
}

std::optional<CardProblem> badWord(const char* field, const std::string& word, const char* allowed)
{
    return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be " + allowed + ", not " + word,
                       field};
}

void readSymmetry(FieldReader& reader, int line, DesignTopologyRegionCard& card)
{
    DesignTopologyRegionSymmetry symmetry;
    symmetry.frameId = reader.integer(line + 1, "frameId", 0);
    for (int plane = 0; plane < 3; ++plane)
        symmetry.mirrorPlanes[static_cast<std::size_t>(plane)] = wordText(reader, line + 2 + plane);
    symmetry.cyclicAxis = wordText(reader, line + 5);
    symmetry.cyclicSegments = reader.integer(line + 6, "cyclicSegments", 0);
    card.symmetry = std::move(symmetry);
}

void readCasting(FieldReader& reader, int line, DesignTopologyRegionCard& card)
{
    DesignTopologyRegionCasting casting;
    casting.frameId = reader.integer(line + 1, "frameId", 0);
    casting.direction = wordText(reader, line + 2);
    casting.dies = reader.integer(line + 3, "dieCount", 1);
    if (const std::string alignment = wordText(reader, line + 4); !alignment.empty())
        casting.alignment = alignment;
    card.casting = std::move(casting);
}

void readExtrusion(FieldReader& reader, int line, DesignTopologyRegionCard& card)
{
    DesignTopologyRegionExtrusion extrusion;
    extrusion.frameId = reader.integer(line + 1, "frameId", 0);
    extrusion.direction = wordText(reader, line + 2);
    if (const std::string alignment = wordText(reader, line + 3); !alignment.empty())
        extrusion.alignment = alignment;
    card.extrusion = std::move(extrusion);
}

void readOverhang(FieldReader& reader, int line, DesignTopologyRegionCard& card)
{
    DesignTopologyRegionOverhang overhang;
    overhang.frameId = reader.integer(line + 1, "frameId", 0);
    overhang.direction = wordText(reader, line + 2);
    card.overhang = std::move(overhang);
}

// The number of fields a line of each keyword uses, after the keyword.
int valueCountOf(const std::string& keyword) noexcept
{
    if (keyword == "Symmetry")
        return 6;
    if (keyword == "Casting")
        return 4;
    if (keyword == "Extrusion")
        return 3;
    if (keyword == "Overhang")
        return 2;
    if (keyword == "MemberSize")
        return 2;
    return 1; // STRESS
}

} // namespace

std::optional<DesignTopologyRegionCard> DesignTopologyRegionFormat::read(FieldReader& reader)
{
    DesignTopologyRegionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.entryLabel = std::string(reader.text(2));
    if (card.entryLabel.empty())
        reader.error("card.missing-field", std::string("entryLabel") + " is required", "entryLabel");
    card.propType = std::string(reader.text(3));
    card.initialValue = reader.optionalReal(4, "initialValue");
    card.lowerBound = reader.real(5, "lowerBound", DesignTopologyRegionCard::DEFAULT_LOWER_BOUND);
    card.designVarMoveLimit = reader.optionalReal(6, "designVarMoveLimit");
    card.penaltyPower = reader.optionalReal(7, "penaltyPower");
    card.propId = reader.requiredInteger(8, "propId");

    std::array<bool, 6> seen{};
    for (int line = FIRST_LINE_FIELD; line <= reader.lastUsedField(); line += FIELDS_PER_LINE)
    {
        const std::string keyword = wordText(reader, line);
        static constexpr std::array<std::string_view, 6> KEYWORDS{"Symmetry", "Casting",    "Extrusion",
                                                                  "Overhang", "MemberSize", "Stress"};
        const auto found = std::find(KEYWORDS.begin(), KEYWORDS.end(), keyword);
        if (found == KEYWORDS.end())
        {
            reader.error("card.bad-value",
                         keyword.empty() ? "a line of a " + std::string(reader.cardName()) +
                                               " starts with SYM, CAST, EXT, PRINT, TDMIN or STRESS"
                                         : "\"" + keyword + "\" is not SYM, CAST, EXT, PRINT, TDMIN or STRESS",
                         "lineKeyword");
            break;
        }
        const std::size_t index = static_cast<std::size_t>(found - KEYWORDS.begin());
        if (seen[index])
        {
            reader.error("card.bad-value", keyword + " is given twice", "lineKeyword");
            break;
        }
        seen[index] = true;
        for (int field = line + 1 + valueCountOf(keyword); field < line + FIELDS_PER_LINE; ++field)
        {
            if (!reader.isBlank(field))
                reader.error("card.bad-value",
                             "a " + keyword + " line has " + std::to_string(valueCountOf(keyword)) + " values",
                             "lineKeyword");
        }
        if (keyword == "Symmetry")
            readSymmetry(reader, line, card);
        else if (keyword == "Casting")
            readCasting(reader, line, card);
        else if (keyword == "Extrusion")
            readExtrusion(reader, line, card);
        else if (keyword == "Overhang")
            readOverhang(reader, line, card);
        else if (keyword == "MemberSize")
            card.memberSize = DesignTopologyRegionMemberSize{reader.optionalReal(line + 1, "minMemberSize"),
                                                             reader.optionalReal(line + 2, "maxMemberSize")};
        else
            card.stressLimit = reader.optionalReal(line + 1, "stressLimit");
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignTopologyRegionFormat::check(const DesignTopologyRegionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkDesignLabel(card.entryLabel, "entryLabel"))
        return problem;
    if (const auto problem = checkNeutralName(card.propType, "propType"))
        return problem;
    if (const auto problem = checkOptionalReal(card.initialValue, "initialValue", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.lowerBound, "lowerBound", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.designVarMoveLimit, "designVarMoveLimit", RealRule::Positive))
        return problem;
    if (card.penaltyPower)
    {
        if (const auto problem = checkReal(*card.penaltyPower, "penaltyPower", RealRule::Finite))
            return problem;
        if (*card.penaltyPower <= 1.0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 1.0",
                               "penaltyPower"};
    }
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;

    if (card.symmetry)
    {
        const DesignTopologyRegionSymmetry& symmetry = *card.symmetry;
        if (symmetry.frameId < 0)
            return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "frameId"};
        for (const std::string& plane : symmetry.mirrorPlanes)
        {
            if (!plane.empty() && !isOneOf(plane, MIRROR_PLANES))
                return badWord("mirrorPlanes", plane, "XY, YZ or ZX");
        }
        if (!symmetry.cyclicAxis.empty() && !isOneOf(symmetry.cyclicAxis, AXES))
            return badWord("cyclicAxis", symmetry.cyclicAxis, "X, Y or Z");
        if (symmetry.cyclicSegments < 0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                               "cyclicSegments"};
        if (symmetry.cyclicAxis.empty() != (symmetry.cyclicSegments == 0))
            return CardProblem{"card.bad-value",
                               CardProblem::fieldNamed("cyclicAxis") + " and " + std::string(CardProblem::FIELD) +
                                   " go together: the axis and the number of segments",
                               "cyclicSegments"};
    }
    if (card.casting)
    {
        const DesignTopologyRegionCasting& casting = *card.casting;
        if (casting.frameId < 0)
            return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "frameId"};
        if (!isOneOf(casting.direction, DIRECTIONS))
            return badWord("castDirection", casting.direction, "X, Y, Z, X-, Y- or Z-");
        if (casting.dies != 1 && casting.dies != 2)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1 or 2", "dieCount"};
        if (!isOneOf(casting.alignment, ALIGNMENTS))
            return badWord("alignment", casting.alignment, "YES or NO");
    }
    if (card.extrusion)
    {
        const DesignTopologyRegionExtrusion& extrusion = *card.extrusion;
        if (extrusion.frameId < 0)
            return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "frameId"};
        if (!isOneOf(extrusion.direction, AXES))
            return badWord("extrusionDirection", extrusion.direction, "X, Y or Z");
        if (!isOneOf(extrusion.alignment, ALIGNMENTS))
            return badWord("alignment", extrusion.alignment, "YES or NO");
    }
    if (card.overhang)
    {
        if (card.overhang->frameId < 0)
            return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "frameId"};
        if (!isOneOf(card.overhang->direction, DIRECTIONS))
            return badWord("overhangDirection", card.overhang->direction, "X, Y, Z, X-, Y- or Z-");
    }
    if (card.memberSize)
    {
        if (const auto problem = checkOptionalReal(card.memberSize->minimum, "minMemberSize", RealRule::NonNegative))
            return problem;
        if (const auto problem = checkOptionalReal(card.memberSize->maximum, "maxMemberSize", RealRule::Positive))
            return problem;
        if (card.memberSize->minimum && card.memberSize->maximum &&
            *card.memberSize->maximum <= *card.memberSize->minimum)
            return CardProblem{
                "card.bad-value",
                std::string(CardProblem::FIELD) + " must be greater than " + CardProblem::fieldNamed("minMemberSize"),
                "maxMemberSize"};
    }
    return checkOptionalReal(card.stressLimit, "stressLimit", RealRule::Positive);
}

void DesignTopologyRegionFormat::write(const DesignTopologyRegionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.entryLabel);
    writer.text(3, card.propType);
    if (card.initialValue)
        writer.real(4, *card.initialValue);
    writer.realUnlessDefault(5, card.lowerBound, DesignTopologyRegionCard::DEFAULT_LOWER_BOUND);
    if (card.designVarMoveLimit)
        writer.real(6, *card.designVarMoveLimit);
    if (card.penaltyPower)
        writer.real(7, *card.penaltyPower);
    writer.integer(8, card.propId);

    int line = FIRST_LINE_FIELD;
    if (card.symmetry)
    {
        const DesignTopologyRegionSymmetry& symmetry = *card.symmetry;
        writer.text(line, "Symmetry");
        writer.integerUnlessDefault(line + 1, symmetry.frameId, 0);
        for (int plane = 0; plane < 3; ++plane)
            writer.text(line + 2 + plane, symmetry.mirrorPlanes[static_cast<std::size_t>(plane)]);
        writer.text(line + 5, symmetry.cyclicAxis);
        writer.integerUnlessDefault(line + 6, symmetry.cyclicSegments, 0);
        line += FIELDS_PER_LINE;
    }
    if (card.casting)
    {
        writer.text(line, "Casting");
        writer.integerUnlessDefault(line + 1, card.casting->frameId, 0);
        writer.text(line + 2, card.casting->direction);
        writer.integerUnlessDefault(line + 3, card.casting->dies, 1);
        writer.textUnlessDefault(line + 4, card.casting->alignment, "No");
        line += FIELDS_PER_LINE;
    }
    if (card.extrusion)
    {
        writer.text(line, "Extrusion");
        writer.integerUnlessDefault(line + 1, card.extrusion->frameId, 0);
        writer.text(line + 2, card.extrusion->direction);
        writer.textUnlessDefault(line + 3, card.extrusion->alignment, "No");
        line += FIELDS_PER_LINE;
    }
    if (card.overhang)
    {
        writer.text(line, "Overhang");
        writer.integerUnlessDefault(line + 1, card.overhang->frameId, 0);
        writer.text(line + 2, card.overhang->direction);
        line += FIELDS_PER_LINE;
    }
    if (card.memberSize)
    {
        writer.text(line, "MemberSize");
        if (card.memberSize->minimum)
            writer.real(line + 1, *card.memberSize->minimum);
        if (card.memberSize->maximum)
            writer.real(line + 2, *card.memberSize->maximum);
        line += FIELDS_PER_LINE;
    }
    if (card.stressLimit)
    {
        writer.text(line, "Stress");
        writer.real(line + 1, *card.stressLimit);
    }
}

std::uint32_t DesignTopologyRegionTable::append(const DesignTopologyRegionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void DesignTopologyRegionTable::store(std::uint32_t row, const DesignTopologyRegionCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void DesignTopologyRegionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void DesignTopologyRegionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void DesignTopologyRegionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    DesignTopologyRegionCard& card = m_cards.edit(row);
    visitId(visitor, "propId", ReferenceTarget::Property, card.propId);
    if (card.symmetry)
        visitId(visitor, "frameId", ReferenceTarget::Coordinate, card.symmetry->frameId);
    if (card.casting)
        visitId(visitor, "frameId", ReferenceTarget::Coordinate, card.casting->frameId);
    if (card.extrusion)
        visitId(visitor, "frameId", ReferenceTarget::Coordinate, card.extrusion->frameId);
    if (card.overhang)
        visitId(visitor, "frameId", ReferenceTarget::Coordinate, card.overhang->frameId);
}

} // namespace oor::model
