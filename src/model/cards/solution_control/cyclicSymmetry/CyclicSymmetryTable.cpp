#include "oor/model/cards/solution_control/cyclicSymmetry/CyclicSymmetryTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int SEGMENT_COUNT_FIELD = 1;
constexpr int SYMMETRY_KIND_FIELD = 2;

} // namespace

std::optional<CyclicSymmetryCard> CyclicSymmetryFormat::read(FieldReader& reader)
{
    CyclicSymmetryCard card;
    card.segments = reader.requiredInteger(SEGMENT_COUNT_FIELD, "segmentCount");
    card.type = readRequiredKeyword<CyclicSymmetryType>(reader, SYMMETRY_KIND_FIELD, "symmetryKind",
                                                        CYCLIC_SYMMETRY_TYPE_WORDS);
    if (reader.lastUsedField() > SYMMETRY_KIND_FIELD)
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " has no field beyond " + std::string("symmetryKind"),
                     "symmetryKind");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CyclicSymmetryFormat::check(const CyclicSymmetryCard& card)
{
    if (card.segments <= 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "segmentCount"};
    return std::nullopt;
}

void CyclicSymmetryFormat::write(const CyclicSymmetryCard& card, FieldWriter& writer)
{
    writer.integer(SEGMENT_COUNT_FIELD, card.segments);
    writeKeyword(writer, SYMMETRY_KIND_FIELD, CYCLIC_SYMMETRY_TYPE_WORDS,
                 card.type); // required: no default to leave blank
}

std::uint32_t CyclicSymmetryTable::append(const CyclicSymmetryCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_segments.push(card.segments);
    m_cards.push(card);
    return row;
}

void CyclicSymmetryTable::store(std::uint32_t row, const CyclicSymmetryCard& card)
{
    m_segments.set(row, card.segments);
    m_cards.set(row, card);
}

void CyclicSymmetryTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_segments.keepRows(rows);
    m_cards.keepRows(rows);
}

} // namespace oor::model
