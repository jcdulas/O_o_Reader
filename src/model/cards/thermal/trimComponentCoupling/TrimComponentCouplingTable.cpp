#include "oor/model/cards/thermal/trimComponentCoupling/TrimComponentCouplingTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 18; // CRIMFR

// A compression ratio is in the range 0 (excluded) to 1.
std::optional<CardProblem> checkCompressionRatio(double ratio, const char* field)
{
    if (!(ratio > 0.0 && ratio <= 1.0))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be greater than 0.0 and at most 1.0", field};
    return std::nullopt;
}

// A SET1 or SET3 of the trim component TID: the format puts it in the Bulk Data section BEGIN
// BULK TRMC=TID, a part of the deck, while the card itself is in the main Bulk Data. A blank set (0) is not visited.
void visitTrimSet(ReferenceVisitor& visitor, std::string_view field, base::EntityId componentId, base::EntityId& set)
{
    if (set > 0)
        set = visitor.visitInPart(field, ReferenceTarget::Set, set, "TrimComponent", componentId);
}

} // namespace

std::optional<TrimComponentCouplingCard> TrimComponentCouplingFormat::read(FieldReader& reader)
{
    TrimComponentCouplingCard card;
    card.id = reader.requiredInteger(1, "id");
    card.gluedSetId = reader.integer(2, "gluedSetId", 0);
    card.slidingSetId = reader.integer(3, "slidingSetId", 0);
    card.openSetId = reader.integer(4, "openSetId", 0);
    card.imperviousSetId = reader.integer(5, "imperviousSetId", 0);
    card.outOfCoreBlockCount = reader.integer(6, "outOfCoreBlockCount", 1);
    card.singlePrecisionFlag = reader.integer(7, "singlePrecisionFlag", 0);
    card.airGapSetId = reader.integer(8, "airGapSetId", 0);
    card.fixedTranslationXSetId = reader.integer(9, "fixedTranslationXSetId", 0);
    card.fixedTranslationYSetId = reader.integer(10, "fixedTranslationYSetId", 0);
    card.fixedTranslationZSetId = reader.integer(11, "fixedTranslationZSetId", 0);
    card.fixedRotationXSetId = reader.integer(12, "fixedRotationXSetId", 0);
    card.fixedRotationYSetId = reader.integer(13, "fixedRotationYSetId", 0);
    card.fixedRotationZSetId = reader.integer(14, "fixedRotationZSetId", 0);
    card.fixedPressureSetId = reader.integer(15, "fixedPressureSetId", 0);
    card.regionId = reader.integer(16, "regionId", 0);
    card.structureCompressionRatio = reader.real(17, "structureCompressionRatio", TRIM_COUPLING_DEFAULT_COMPRESSION);
    card.cavityCompressionRatio = reader.real(18, "cavityCompressionRatio", TRIM_COUPLING_DEFAULT_COMPRESSION);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("cavityCompressionRatio"),
                     "cavityCompressionRatio");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TrimComponentCouplingFormat::check(const TrimComponentCouplingCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.gluedSetId, "gluedSetId"))
        return problem;
    if (const auto problem = checkOptionalId(card.slidingSetId, "slidingSetId"))
        return problem;
    if (const auto problem = checkOptionalId(card.openSetId, "openSetId"))
        return problem;
    if (const auto problem = checkOptionalId(card.imperviousSetId, "imperviousSetId"))
        return problem;
    if (card.outOfCoreBlockCount < 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be at least 1",
                           "outOfCoreBlockCount"};
    if (card.singlePrecisionFlag < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "singlePrecisionFlag"};
    if (const auto problem = checkOptionalId(card.airGapSetId, "airGapSetId"))
        return problem;
    if (const auto problem = checkOptionalId(card.fixedTranslationXSetId, "fixedTranslationXSetId"))
        return problem;
    if (const auto problem = checkOptionalId(card.fixedTranslationYSetId, "fixedTranslationYSetId"))
        return problem;
    if (const auto problem = checkOptionalId(card.fixedTranslationZSetId, "fixedTranslationZSetId"))
        return problem;
    if (const auto problem = checkOptionalId(card.fixedRotationXSetId, "fixedRotationXSetId"))
        return problem;
    if (const auto problem = checkOptionalId(card.fixedRotationYSetId, "fixedRotationYSetId"))
        return problem;
    if (const auto problem = checkOptionalId(card.fixedRotationZSetId, "fixedRotationZSetId"))
        return problem;
    if (const auto problem = checkOptionalId(card.fixedPressureSetId, "fixedPressureSetId"))
        return problem;
    if (card.regionId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "regionId"};
    if (const auto problem = checkCompressionRatio(card.structureCompressionRatio, "structureCompressionRatio"))
        return problem;
    return checkCompressionRatio(card.cavityCompressionRatio, "cavityCompressionRatio");
}

void TrimComponentCouplingFormat::write(const TrimComponentCouplingCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.gluedSetId, 0);
    writer.integerUnlessDefault(3, card.slidingSetId, 0);
    writer.integerUnlessDefault(4, card.openSetId, 0);
    writer.integerUnlessDefault(5, card.imperviousSetId, 0);
    writer.integerUnlessDefault(6, card.outOfCoreBlockCount, 1);
    writer.integerUnlessDefault(7, card.singlePrecisionFlag, 0);
    writer.integerUnlessDefault(8, card.airGapSetId, 0);
    writer.integerUnlessDefault(9, card.fixedTranslationXSetId, 0);
    writer.integerUnlessDefault(10, card.fixedTranslationYSetId, 0);
    writer.integerUnlessDefault(11, card.fixedTranslationZSetId, 0);
    writer.integerUnlessDefault(12, card.fixedRotationXSetId, 0);
    writer.integerUnlessDefault(13, card.fixedRotationYSetId, 0);
    writer.integerUnlessDefault(14, card.fixedRotationZSetId, 0);
    writer.integerUnlessDefault(15, card.fixedPressureSetId, 0);
    writer.integerUnlessDefault(16, card.regionId, 0);
    writer.realUnlessDefault(17, card.structureCompressionRatio, TRIM_COUPLING_DEFAULT_COMPRESSION);
    writer.realUnlessDefault(18, card.cavityCompressionRatio, TRIM_COUPLING_DEFAULT_COMPRESSION);
}

std::uint32_t TrimComponentCouplingTable::append(const TrimComponentCouplingCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void TrimComponentCouplingTable::store(std::uint32_t row, const TrimComponentCouplingCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void TrimComponentCouplingTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void TrimComponentCouplingTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void TrimComponentCouplingTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The TID is the number of a trim component and the RID the number of one of its regions: no card defines either.
    // The twelve sets are SET1 or SET3 cards of the part TRMC=TID.
    TrimComponentCouplingCard& card = m_cards.edit(row);
    visitTrimSet(visitor, "gluedSetId", card.id, card.gluedSetId);
    visitTrimSet(visitor, "slidingSetId", card.id, card.slidingSetId);
    visitTrimSet(visitor, "openSetId", card.id, card.openSetId);
    visitTrimSet(visitor, "imperviousSetId", card.id, card.imperviousSetId);
    visitTrimSet(visitor, "airGapSetId", card.id, card.airGapSetId);
    visitTrimSet(visitor, "fixedTranslationXSetId", card.id, card.fixedTranslationXSetId);
    visitTrimSet(visitor, "fixedTranslationYSetId", card.id, card.fixedTranslationYSetId);
    visitTrimSet(visitor, "fixedTranslationZSetId", card.id, card.fixedTranslationZSetId);
    visitTrimSet(visitor, "fixedRotationXSetId", card.id, card.fixedRotationXSetId);
    visitTrimSet(visitor, "fixedRotationYSetId", card.id, card.fixedRotationYSetId);
    visitTrimSet(visitor, "fixedRotationZSetId", card.id, card.fixedRotationZSetId);
    visitTrimSet(visitor, "fixedPressureSetId", card.id, card.fixedPressureSetId);
}

} // namespace oor::model
