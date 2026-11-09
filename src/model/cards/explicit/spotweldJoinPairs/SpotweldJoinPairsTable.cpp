#include "oor/model/cards/explicit/spotweldJoinPairs/SpotweldJoinPairsTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 18;
constexpr int MULTIPLE_PAIRS_FIELD = 13;
constexpr int FAILURE_TIME_FIELD = 18;

} // namespace

std::optional<SpotweldJoinPairsCard> SpotweldJoinPairsFormat::read(FieldReader& reader)
{
    SpotweldJoinPairsCard card;
    card.id = reader.requiredInteger(1, "id");
    card.nodeSetId = reader.requiredInteger(2, "nodeSetId");
    card.tolerance = reader.real(3, "tolerance", SpotweldJoinPairsCard::DEFAULT_TOLERANCE);
    card.type = readKeyword(reader, 4, "failureKind", SPOTWELD_FAILURE_WORDS, SpotweldJoinPairsFailure::Spotweld);
    card.tensileFailureForce = reader.optionalReal(6, "tensileFailureForce");
    card.shearFailureForce = reader.optionalReal(8, "shearFailureForce");
    card.multiplePairs = readKeyword(reader, MULTIPLE_PAIRS_FIELD, "multiplePairs", SPOTWELD_MULTIPLE_PAIRS_WORDS,
                                     SpotweldJoinPairsPairs::Given);
    card.failureTime = reader.real(FAILURE_TIME_FIELD, "failureTime", SpotweldJoinPairsCard::DEFAULT_FAILURE_TIME);
    requireBlankFields(reader, {5, 7, 9, 10, 11, 12, 14, 15, 16, 17}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SpotweldJoinPairsFormat::check(const SpotweldJoinPairsCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.nodeSetId, "nodeSetId"))
        return problem;
    if (const auto problem = checkReal(card.tolerance, "tolerance", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.tensileFailureForce, "tensileFailureForce", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.shearFailureForce, "shearFailureForce", RealRule::Positive))
        return problem;
    return checkReal(card.failureTime, "failureTime", RealRule::Finite);
}

void SpotweldJoinPairsFormat::write(const SpotweldJoinPairsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.nodeSetId);
    writer.realUnlessDefault(3, card.tolerance, SpotweldJoinPairsCard::DEFAULT_TOLERANCE);
    writeKeywordUnlessDefault(writer, 4, SPOTWELD_FAILURE_WORDS, card.type, SpotweldJoinPairsFailure::Spotweld);
    if (card.tensileFailureForce)
        writer.real(6, *card.tensileFailureForce);
    if (card.shearFailureForce)
        writer.real(8, *card.shearFailureForce);
    writeKeywordUnlessDefault(writer, MULTIPLE_PAIRS_FIELD, SPOTWELD_MULTIPLE_PAIRS_WORDS, card.multiplePairs,
                              SpotweldJoinPairsPairs::Given);
    writer.realUnlessDefault(FAILURE_TIME_FIELD, card.failureTime, SpotweldJoinPairsCard::DEFAULT_FAILURE_TIME);
}

std::uint32_t SpotweldJoinPairsTable::append(const SpotweldJoinPairsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void SpotweldJoinPairsTable::store(std::uint32_t row, const SpotweldJoinPairsCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void SpotweldJoinPairsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void SpotweldJoinPairsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void SpotweldJoinPairsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "nodeSetId", ReferenceTarget::ContactBody, m_cards.edit(row).nodeSetId);
}

} // namespace oor::model
