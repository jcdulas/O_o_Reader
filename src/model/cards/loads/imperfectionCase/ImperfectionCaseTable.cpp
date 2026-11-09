#include "oor/model/cards/loads/imperfectionCase/ImperfectionCaseTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_IMPERFECTION_ID_FIELD = 2;

} // namespace

std::optional<ImperfectionCaseCard> ImperfectionCaseFormat::read(FieldReader& reader)
{
    ImperfectionCaseCard card;
    card.id = reader.requiredInteger(1, "id");
    card.imperfections = readIdRuns(reader, FIRST_IMPERFECTION_ID_FIELD, "imperfectionIds");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ImperfectionCaseFormat::check(const ImperfectionCaseCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    return checkIdRuns(card.imperfections, "imperfectionIds", IdSign::Positive);
}

void ImperfectionCaseFormat::write(const ImperfectionCaseCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeIdRuns(writer, FIRST_IMPERFECTION_ID_FIELD, card.imperfections, writer.compressesRanges());
}

std::uint32_t ImperfectionCaseTable::append(const ImperfectionCaseCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_runs.push(toPairs(card.imperfections));
    return row;
}

ImperfectionCaseCard ImperfectionCaseTable::card(std::uint32_t row) const
{
    ImperfectionCaseCard card;
    card.id = m_id.get(row);
    card.imperfections = toIdRuns(m_runs.get(row));
    return card;
}

void ImperfectionCaseTable::store(std::uint32_t row, const ImperfectionCaseCard& card)
{
    m_id.set(row, card.id);
    m_runs.set(row, toPairs(card.imperfections));
}

void ImperfectionCaseTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_runs.keepRows(rows);
}

void ImperfectionCaseTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID of an IMPGEOM is a number of the category ImperfectionGeometry; a range names its two ends.
    visitIdRuns(m_runs, row, visitor, "imperfectionIds", ReferenceTarget::ImperfectionGeometry,
                ReferenceTarget::ImperfectionGeometry);
}

} // namespace oor::model
