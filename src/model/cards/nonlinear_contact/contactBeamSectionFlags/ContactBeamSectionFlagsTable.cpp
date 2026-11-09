#include "oor/model/cards/nonlinear_contact/contactBeamSectionFlags/ContactBeamSectionFlagsTable.hpp"

#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_PAIR_FIELD = 2;

// "3" cannot be used alone and "4" only with "1" or "2".
bool isSegmentCode(std::int32_t code) noexcept
{
    switch (code)
    {
    case 0:
    case 1:
    case 2:
    case 12:
    case 13:
    case 14:
    case 23:
    case 24:
    case 123:
        return true;
    default:
        return false;
    }
}

} // namespace

std::optional<ContactBeamSectionFlagsCard> ContactBeamSectionFlagsFormat::read(FieldReader& reader)
{
    ContactBeamSectionFlagsCard card;
    card.id = reader.requiredInteger(1, "id");
    const int last = reader.lastUsedField();
    for (std::size_t index = 0;; ++index)
    {
        const int field = contactPairField(FIRST_PAIR_FIELD, index);
        if (field > last || reader.failed())
            break;
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        card.segments.push_back({reader.integer(field, "branch", 0), reader.integer(field + 1, "segmentFlag", 0)});
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactBeamSectionFlagsFormat::check(const ContactBeamSectionFlagsCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.segments.empty())
        return CardProblem{"card.missing-field", "the card needs at least one " + std::string(CardProblem::FIELD),
                           "segmentFlag"};
    for (const auto& segment : card.segments)
    {
        if (segment[0] < 0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "branch"};
        if (!isSegmentCode(segment[1]))
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) +
                                   " must be 0, 1, 2, 12, 13, 14, 23, 24 or 123 (3 alone and 4 alone are not allowed)",
                               "segmentFlag"};
    }
    return std::nullopt;
}

void ContactBeamSectionFlagsFormat::write(const ContactBeamSectionFlagsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t index = 0; index < card.segments.size(); ++index)
    {
        const int field = contactPairField(FIRST_PAIR_FIELD, index);
        writer.integer(field, card.segments[index][0]);
        writer.integer(field + 1, card.segments[index][1]);
    }
}

std::uint32_t ContactBeamSectionFlagsTable::append(const ContactBeamSectionFlagsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_segments.push(card.segments);
    return row;
}

ContactBeamSectionFlagsCard ContactBeamSectionFlagsTable::card(std::uint32_t row) const
{
    ContactBeamSectionFlagsCard card;
    card.id = m_id.get(row);
    const auto segments = m_segments.get(row);
    card.segments.assign(segments.begin(), segments.end());
    return card;
}

void ContactBeamSectionFlagsTable::store(std::uint32_t row, const ContactBeamSectionFlagsCard& card)
{
    m_id.set(row, card.id);
    m_segments.set(row, card.segments);
}

void ContactBeamSectionFlagsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_segments.keepRows(rows);
}

void ContactBeamSectionFlagsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitContactIdAt(m_id, row, visitor, "id", ReferenceTarget::Property);
}

} // namespace oor::model
