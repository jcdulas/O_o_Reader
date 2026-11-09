#include "oor/model/cards/properties/compSolidProp/CompSolidPropTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <algorithm>
#include <set>

namespace oor::model
{

namespace
{

constexpr int FIRST_PLY_FIELD = 9; // the plies are continuation lines, one each
constexpr int PLY_FIELDS = 8;

bool isStackingDirection(std::int32_t direction)
{
    return std::ranges::find(COMP_SOLID_PROP_STACKING_DIRECTIONS, direction) !=
           COMP_SOLID_PROP_STACKING_DIRECTIONS.end();
}

// A word of a ply (a failure theory, the eighth field) may be blank; when it is not it has to read back from every
// format.
std::optional<CardProblem> checkOptionalWord(const std::string& word, const char* field)
{
    if (word.empty())
        return std::nullopt;
    return checkWordOrName(word, field);
}

} // namespace

std::optional<CompSolidPropCard> CompSolidPropFormat::read(FieldReader& reader)
{
    CompSolidPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matFrameId = reader.integer(2, "matFrameId", 0);
    card.stackingDirection = reader.integer(3, "stackingDirection", 13);
    card.bondAllowable = reader.optionalReal(4, "bondAllowable");
    card.normalBondAllowable = reader.optionalReal(5, "normalBondAllowable");
    card.referenceTemperature = reader.real(6, "referenceTemperature", 0.0);
    card.structuralDamping = reader.real(7, "structuralDamping", 0.0);
    // Field 8 of the first line is blank in the layout: a value there is not understood.
    if (!reader.isBlank(8))
        reader.error("card.bad-value",
                     "field 8 of the first line of a " + std::string(reader.cardName()) + " must be blank");

    // A line with nothing in it is padding; every other line is a ply.
    for (int line = FIRST_PLY_FIELD; line <= reader.lastUsedField(); line += PLY_FIELDS)
    {
        bool blank = true;
        for (int offset = 0; offset < PLY_FIELDS; ++offset)
            blank = blank && reader.isBlank(line + offset);
        if (blank)
            continue;
        CompSolidPropPly ply;
        ply.globalId = reader.requiredInteger(line, "globalPlyId");
        ply.matId = reader.requiredInteger(line + 1, "matId");
        ply.thickness = reader.requiredReal(line + 2, "thickness");
        ply.thetaInDegrees = reader.real(line + 3, "materialAngle", 0.0);
        ply.failureTheory = std::string(reader.text(line + 4));
        ply.interlaminarTheory = std::string(reader.text(line + 5));
        ply.output =
            readKeyword(reader, line + 6, "plyStressOutput", COMP_SOLID_PROP_OUTPUT_WORDS, CompSolidPropOutput::No);
        ply.tail = std::string(reader.text(line + 7));
        card.plies.push_back(ply);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CompSolidPropFormat::check(const CompSolidPropCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.matFrameId < -1)
        return CardProblem{
            "card.bad-id",
            std::string(CardProblem::FIELD) + " must be 0 (basic), -1 (element) or a coordinate system ID",
            "matFrameId"};
    if (!isStackingDirection(card.stackingDirection))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 12, 13, 21, 23, 31 or 32",
                           "stackingDirection"};
    if (const auto problem = checkOptionalReal(card.bondAllowable, "bondAllowable", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkOptionalReal(card.normalBondAllowable, "normalBondAllowable", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.referenceTemperature, "referenceTemperature", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.structuralDamping, "structuralDamping", RealRule::Finite))
        return problem;
    if (card.plies.empty())
        return CardProblem{"card.missing-field", "a laminate needs at least one ply", "globalPlyId"};
    std::set<base::EntityId> seen;
    for (const CompSolidPropPly& ply : card.plies)
    {
        if (ply.globalId <= 0)
            return CardProblem{"card.bad-id", "a ply needs a global ply ID greater than 0", "globalPlyId"};
        if (!seen.insert(ply.globalId).second)
            return CardProblem{"card.duplicate-id", "the global ply IDs must be different in a card", "globalPlyId"};
        if (ply.matId <= 0)
            return CardProblem{"card.bad-id", "a ply needs a material ID greater than 0", "matId"};
        if (const auto problem = checkReal(ply.thickness, "thickness", RealRule::Positive))
            return problem;
        if (const auto problem = checkReal(ply.thetaInDegrees, "materialAngle", RealRule::Finite))
            return problem;
        if (const auto problem = checkOptionalWord(ply.failureTheory, "failureTheory"))
            return problem;
        if (const auto problem = checkOptionalWord(ply.interlaminarTheory, "interlaminarTheory"))
            return problem;
        if (const auto problem = checkOptionalWord(ply.tail, "plyField8"))
            return problem;
    }
    return std::nullopt;
}

void CompSolidPropFormat::write(const CompSolidPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.matFrameId, 0);
    writer.integerUnlessDefault(3, card.stackingDirection, 13);
    if (card.bondAllowable)
        writer.real(4, *card.bondAllowable);
    if (card.normalBondAllowable)
        writer.real(5, *card.normalBondAllowable);
    writer.realUnlessDefault(6, card.referenceTemperature, 0.0);
    writer.realUnlessDefault(7, card.structuralDamping, 0.0);
    int field = FIRST_PLY_FIELD;
    for (const CompSolidPropPly& ply : card.plies)
    {
        writer.integer(field, ply.globalId);
        writer.integer(field + 1, ply.matId);
        writer.real(field + 2, ply.thickness);
        // Always written, although a blank THETA is read as 0.0.
        writer.real(field + 3, ply.thetaInDegrees);
        if (!ply.failureTheory.empty())
            writer.text(field + 4, ply.failureTheory);
        if (!ply.interlaminarTheory.empty())
            writer.text(field + 5, ply.interlaminarTheory);
        writeKeywordUnlessDefault(writer, field + 6, COMP_SOLID_PROP_OUTPUT_WORDS, ply.output, CompSolidPropOutput::No);
        if (!ply.tail.empty())
            writer.text(field + 7, ply.tail);
        field += PLY_FIELDS;
    }
}

std::uint32_t CompSolidPropTable::append(const CompSolidPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void CompSolidPropTable::store(std::uint32_t row, const CompSolidPropCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void CompSolidPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void CompSolidPropTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void CompSolidPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    CompSolidPropCard& card = m_cards.edit(row);
    for (CompSolidPropPly& ply : card.plies)
        ply.matId = visitor.visit("matId", ReferenceTarget::Material, ply.matId);
    // 0 is the basic system and -1 the element system: neither is a card.
    if (card.matFrameId > 0)
        card.matFrameId = visitor.visit("matFrameId", ReferenceTarget::Coordinate, card.matFrameId);
}

} // namespace oor::model
