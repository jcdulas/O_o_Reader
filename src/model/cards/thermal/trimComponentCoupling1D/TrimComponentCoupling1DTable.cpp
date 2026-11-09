#include "oor/model/cards/thermal/trimComponentCoupling1D/TrimComponentCoupling1DTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int REGION_ID_FIELD = 16; // the last cell of the second line
constexpr int METHOD_FIELD = 17;    // the first cell of the third line

// A SET3 of the PLTSURF surfaces of the trim component TID: it is in the Bulk Data section BEGIN BULK TRMC=TID, a part
// of the deck, as for an ACPEMCP, while the card itself is in the main Bulk Data.
// A blank set (0) is not visited.
void visitTrimSet(ReferenceVisitor& visitor, std::string_view field, base::EntityId componentId, base::EntityId& set)
{
    if (set > 0)
        set = visitor.visitInPart(field, ReferenceTarget::Set, set, "TrimComponent", componentId);
}

} // namespace

std::optional<TrimComponentCoupling1DCard> TrimComponentCoupling1DFormat::read(FieldReader& reader)
{
    TrimComponentCoupling1DCard card;
    card.id = reader.requiredInteger(1, "id");
    card.gluedSetId = reader.integer(2, "gluedSetId", 0);
    card.slidingSetId = reader.integer(3, "slidingSetId", 0);
    card.openSetId = reader.integer(4, "openSetId", 0);
    card.imperviousSetId = reader.integer(5, "imperviousSetId", 0);
    card.regionId = reader.integer(REGION_ID_FIELD, "regionId", 0);
    card.method = readOptionalKeyword<TrimComponentCoupling1DMethod>(reader, METHOD_FIELD, "methodName",
                                                                     TRIM_COUPLING_1D_METHOD_WORDS);
    // The cells of the grid that the format leaves blank, and the end of the card.
    requireUnusedBlank(reader, {6, 7, 8, 9, 10, 11, 12, 13, 14, 15}, METHOD_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TrimComponentCoupling1DFormat::check(const TrimComponentCoupling1DCard& card)
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
    if (card.regionId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "regionId"};
    return std::nullopt;
}

void TrimComponentCoupling1DFormat::write(const TrimComponentCoupling1DCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.gluedSetId, 0);
    writer.integerUnlessDefault(3, card.slidingSetId, 0);
    writer.integerUnlessDefault(4, card.openSetId, 0);
    writer.integerUnlessDefault(5, card.imperviousSetId, 0);
    writer.integerUnlessDefault(REGION_ID_FIELD, card.regionId, 0);
    writeOptionalKeyword(writer, METHOD_FIELD, TRIM_COUPLING_1D_METHOD_WORDS, card.method);
}

std::uint32_t TrimComponentCoupling1DTable::append(const TrimComponentCoupling1DCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void TrimComponentCoupling1DTable::store(std::uint32_t row, const TrimComponentCoupling1DCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void TrimComponentCoupling1DTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void TrimComponentCoupling1DTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void TrimComponentCoupling1DTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The TID and the RID are numbers of a trim component and of one of its regions: no card defines either. The four
    // sets are SET3 cards of PLTSURF surfaces, in the part TRMC=TID.
    TrimComponentCoupling1DCard& card = m_cards.edit(row);
    visitTrimSet(visitor, "gluedSetId", card.id, card.gluedSetId);
    visitTrimSet(visitor, "slidingSetId", card.id, card.slidingSetId);
    visitTrimSet(visitor, "openSetId", card.id, card.openSetId);
    visitTrimSet(visitor, "imperviousSetId", card.id, card.imperviousSetId);
}

} // namespace oor::model
