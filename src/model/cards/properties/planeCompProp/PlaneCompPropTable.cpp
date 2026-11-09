#include "oor/model/cards/properties/planeCompProp/PlaneCompPropTable.hpp"

#include <set>

namespace oor::model
{

namespace
{

constexpr int FIRST_LINE_FIELD = 9; // the keyword lines and the plies are continuation lines, one for each

} // namespace

std::optional<PlaneCompPropCard> PlaneCompPropFormat::read(FieldReader& reader)
{
    PlaneCompPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.direction = reader.integer(2, "direction", 1);
    card.thicknessOption = reader.real(3, "thicknessOption", 1.0);
    card.bondAllowable = reader.optionalReal(4, "bondAllowable");
    card.analysisType =
        readKeyword(reader, 5, "analysisType", PROPERTY_ANALYSIS_WORDS, PropertyAnalysis::StructuralAndHeat);
    requireUnusedFields(reader, {6, 7, 8});

    // A continuation line is a keyword line (C4 or C8) or a ply (it starts with the global ply ID); a line with nothing
    // in it is padding.
    std::array<bool, 2> given{};
    for (int line = FIRST_LINE_FIELD; line <= reader.lastUsedField(); line += EXTENSION_LINE_FIELDS)
    {
        bool blank = true;
        for (int offset = 0; offset < EXTENSION_LINE_FIELDS; ++offset)
            blank = blank && reader.isBlank(line + offset);
        if (blank)
            continue;
        if (const std::optional<std::size_t> index = findBehaviorRules(PLANE_COMP_PROP_GROUPS, reader.text(line)))
        {
            if (given[*index])
                reader.error("card.bad-value",
                             std::string("the keyword ") + PLANE_COMP_PROP_GROUPS[*index].keyword + " is given twice",
                             PLANE_COMP_PROP_GROUPS[*index].keyword);
            given[*index] = true;
            card.groups[*index] = readBehaviorGroup(reader, line, PLANE_COMP_PROP_GROUPS[*index]);
            continue;
        }
        PlaneCompPropPly ply;
        ply.globalId = reader.requiredInteger(line, "globalPlyId1");
        ply.matId = reader.requiredInteger(line + 1, "matId1");
        ply.thickness = reader.requiredReal(line + 2, "thickness1");
        ply.thetaInDegrees = reader.real(line + 3, "materialAngle1", 0.0);
        for (int offset = 4; offset < EXTENSION_LINE_FIELDS; ++offset)
        {
            if (!reader.isBlank(line + offset))
            {
                reader.error("card.bad-value",
                             "the last fields of a ply line of a " + std::string(reader.cardName()) + " must be blank",
                             "materialAngle1");
                break;
            }
        }
        card.plies.push_back(ply);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PlaneCompPropFormat::check(const PlaneCompPropCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    const int direction = card.direction < 0 ? -card.direction : card.direction;
    if (direction < 1 || direction > 2)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be 1 or 2, or one of them negative", "direction"};
    // The format reference gives THICKOP no range: only a finite number.
    if (const auto problem = checkReal(card.thicknessOption, "thicknessOption", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.bondAllowable, "bondAllowable", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkBehaviorLines(PLANE_COMP_PROP_GROUPS, card.groups))
        return problem;
    if (card.plies.empty())
        return CardProblem{"card.missing-field", "a laminate needs at least one ply", "globalPlyId1"};
    std::set<base::EntityId> seen;
    for (const PlaneCompPropPly& ply : card.plies)
    {
        if (ply.globalId <= 0)
            return CardProblem{"card.bad-id", "a ply needs a global ply ID greater than 0", "globalPlyId1"};
        if (!seen.insert(ply.globalId).second)
            return CardProblem{"card.duplicate-id", "the global ply IDs must be different in a card", "globalPlyId1"};
        if (ply.matId <= 0)
            return CardProblem{"card.bad-id", "a ply needs a material ID greater than 0", "matId1"};
        if (const auto problem = checkReal(ply.thickness, "thickness1", RealRule::Positive))
            return problem;
        if (const auto problem = checkReal(ply.thetaInDegrees, "materialAngle1", RealRule::Finite))
            return problem;
    }
    return std::nullopt;
}

void PlaneCompPropFormat::write(const PlaneCompPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.direction, 1);
    writer.realUnlessDefault(3, card.thicknessOption, 1.0);
    if (card.bondAllowable)
        writer.real(4, *card.bondAllowable);
    writeKeywordUnlessDefault(writer, 5, PROPERTY_ANALYSIS_WORDS, card.analysisType,
                              PropertyAnalysis::StructuralAndHeat);
    int field = writeBehaviorLines(writer, FIRST_LINE_FIELD, PLANE_COMP_PROP_GROUPS, card.groups);
    for (const PlaneCompPropPly& ply : card.plies)
    {
        writer.integer(field, ply.globalId);
        writer.integer(field + 1, ply.matId);
        writer.real(field + 2, ply.thickness);
        writer.realUnlessDefault(field + 3, ply.thetaInDegrees, 0.0);
        field += EXTENSION_LINE_FIELDS;
    }
}

std::uint32_t PlaneCompPropTable::append(const PlaneCompPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void PlaneCompPropTable::store(std::uint32_t row, const PlaneCompPropCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void PlaneCompPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void PlaneCompPropTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void PlaneCompPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The material of a ply may be a MAT4 or MAT5 for a heat ply: the Material target falls back to them.
    for (PlaneCompPropPly& ply : m_cards.edit(row).plies)
        ply.matId = visitor.visit("matId1", ReferenceTarget::Material, ply.matId);
}

} // namespace oor::model
