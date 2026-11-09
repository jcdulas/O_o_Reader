#include "oor/model/cards/nodes/adaptiveRefineControl/AdaptiveRefineControlTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

namespace
{

// The layout: fields 2 and 3 are blank, MAXLEVEL is field 9 (the only field of the second line).
constexpr int LAST_FIELD = 9;
constexpr std::int32_t DEFAULT_SNAP_METHOD = 0; // no projection
constexpr std::int32_t LAST_SNAP_METHOD = 1;    // projection onto the smooth boundary

} // namespace

std::optional<AdaptiveRefineControlCard> AdaptiveRefineControlFormat::read(FieldReader& reader)
{
    AdaptiveRefineControlCard card;
    card.id = reader.requiredInteger(1, "id");
    requireBlankFields(reader, {2, 3}, LAST_FIELD);
    card.repeatCount = reader.integer(4, "repeatCount", ADAPTIVE_REFINE_DEFAULT_REPEAT);
    card.criterionId = reader.requiredInteger(5, "criterionId");
    card.regionMethod = readKeyword(reader, 6, "regionMethod", ADAPTIVE_REFINE_REGION_WORDS, AdaptivityRegion::All);
    card.regionId = reader.integer(7, "regionId", 0);
    card.snapMethod = reader.integer(8, "snapMethod", DEFAULT_SNAP_METHOD);
    // A blank MAXLEVEL is REPEAT, stored resolved so that the card does not change meaning when REPEAT is edited.
    card.maximumLevel = reader.integer(9, "maximumLevel", card.repeatCount);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AdaptiveRefineControlFormat::check(const AdaptiveRefineControlCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.repeatCount < 0)
        return CardProblem{"card.bad-value", "{field} must not be negative", "repeatCount"};
    if (const auto problem = checkRequiredId(card.criterionId, "criterionId"))
        return problem;
    if (const auto problem = checkOptionalId(card.regionId, "regionId"))
        return problem;
    // A property number is positive. A superelement may be 0, the residual structure, so SUPER accepts a zero.
    if (card.regionMethod == AdaptivityRegion::Prop && card.regionId == 0)
        return CardProblem{"card.missing-field",
                           "{field} must be the property when " + CardProblem::fieldNamed("regionMethod") + " is PROP",
                           "regionId"};
    if (card.snapMethod < DEFAULT_SNAP_METHOD || card.snapMethod > LAST_SNAP_METHOD)
        return CardProblem{"card.bad-value", "{field} must be 0 or 1", "snapMethod"};
    if (card.maximumLevel <= 0)
        return CardProblem{
            "card.bad-value",
            "{field} must be a positive integer (it is " + CardProblem::fieldNamed("repeatCount") + " when blank)",
            "maximumLevel"};
    return std::nullopt;
}

void AdaptiveRefineControlFormat::write(const AdaptiveRefineControlCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(4, card.repeatCount, ADAPTIVE_REFINE_DEFAULT_REPEAT);
    writer.integer(5, card.criterionId);
    writeKeywordUnlessDefault(writer, 6, ADAPTIVE_REFINE_REGION_WORDS, card.regionMethod, AdaptivityRegion::All);
    writer.integerUnlessDefault(7, card.regionId, 0);
    writer.integerUnlessDefault(8, card.snapMethod, DEFAULT_SNAP_METHOD);
    // The default of MAXLEVEL depends on REPEAT: always written.
    writer.integer(9, card.maximumLevel);
}

std::uint32_t AdaptiveRefineControlTable::append(const AdaptiveRefineControlCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AdaptiveRefineControlTable::store(std::uint32_t row, const AdaptiveRefineControlCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AdaptiveRefineControlTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AdaptiveRefineControlTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AdaptiveRefineControlTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    AdaptiveRefineControlCard& card = m_cards.edit(row);
    visitId(visitor, "criterionId", ReferenceTarget::AdaptivityCriterion, card.criterionId);
    switch (card.regionMethod)
    {
    case AdaptivityRegion::Prop:
        visitId(visitor, "regionId", ReferenceTarget::Property, card.regionId);
        break;
    case AdaptivityRegion::Super:
        visitSuperelement(visitor, "regionId", card.regionId); // 0, the residual structure, is not visited
        break;
    case AdaptivityRegion::All:
        break; // WHEREID is ignored for ALL
    }
}

} // namespace oor::model
