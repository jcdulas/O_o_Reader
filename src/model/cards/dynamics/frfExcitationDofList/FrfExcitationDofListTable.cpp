#include "oor/model/cards/dynamics/frfExcitationDofList/FrfExcitationDofListTable.hpp"

#include "oor/base/text/DigitSets.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 2;

} // namespace

std::optional<FrfExcitationDofListCard> FrfExcitationDofListFormat::read(FieldReader& reader)
{
    FrfExcitationDofListCard card;
    card.components = base::sortedDigits(reader.integer(1, "components", 0));
    card.pointIds = readIdRuns(reader, FIRST_POINT_FIELD, "pointIds");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FrfExcitationDofListFormat::check(const FrfExcitationDofListCard& card)
{
    if (!base::isComponentSet(card.components))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be 0 or a list of the digits 1 to 6, each at most once",
            "components"};
    return checkIdRuns(card.pointIds, "pointIds");
}

void FrfExcitationDofListFormat::write(const FrfExcitationDofListCard& card, FieldWriter& writer)
{
    writer.integerUnlessDefault(1, card.components, 0);
    writeIdRuns(writer, FIRST_POINT_FIELD, card.pointIds, writer.compressesRanges());
}

std::uint32_t FrfExcitationDofListTable::append(const FrfExcitationDofListCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_components.push(card.components);
    m_runs.push(toPairs(card.pointIds));
    return row;
}

FrfExcitationDofListCard FrfExcitationDofListTable::card(std::uint32_t row) const
{
    FrfExcitationDofListCard card;
    card.components = m_components.get(row);
    card.pointIds = toIdRuns(m_runs.get(row));
    return card;
}

void FrfExcitationDofListTable::store(std::uint32_t row, const FrfExcitationDofListCard& card)
{
    m_components.set(row, card.components);
    m_runs.set(row, toPairs(card.pointIds));
}

void FrfExcitationDofListTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_components.keepRows(rows);
    m_runs.keepRows(rows);
}

void FrfExcitationDofListTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // A single point is a grid point or a scalar point (a scalar point that no SPOINT defines is not reported); the two
    // ends of a THRU range are visited as the ends of a range, and the points between them keep their IDs (the points
    // of a range need not all exist).
    visitIdRuns(m_runs, row, visitor, "pointIds", ReferenceTarget::ScalarOrGridPoint, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
