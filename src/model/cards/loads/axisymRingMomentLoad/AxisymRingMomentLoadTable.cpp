#include "oor/model/cards/loads/axisymRingMomentLoad/AxisymRingMomentLoadTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <charconv>
#include <string>

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 7; // MZ

// Reads "n" at the start of `text` as an integer; the rest of the text follows in `rest`.
bool leadingInteger(std::string_view text, std::int32_t& value, std::string_view& rest)
{
    const char* const begin = text.data();
    const char* const end = begin + text.size();
    const auto [stop, error] = std::from_chars(begin, end, value);
    if (error != std::errc{} || stop == begin)
        return false;
    rest = std::string_view(stop, static_cast<std::size_t>(end - stop));
    return true;
}

} // namespace

std::optional<AxisymRingMomentLoadCard> AxisymRingMomentLoadFormat::read(FieldReader& reader)
{
    AxisymRingMomentLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.ringId = reader.requiredInteger(2, "ringId");
    // HID is a harmonic, or a sequence "Sn1Tn2".
    if (const std::string_view text = reader.text(3); !text.empty() && (text.front() == 'S' || text.front() == 's'))
    {
        std::string_view rest;
        std::int32_t last = 0;
        if (leadingInteger(text.substr(1), card.harmonicId, rest) && !rest.empty() &&
            (rest.front() == 'T' || rest.front() == 't') && leadingInteger(rest.substr(1), last, rest) && rest.empty())
            card.harmonicEnd = last;
        else
            reader.error("card.bad-value",
                         std::string("harmonicId") + " must be a harmonic or a sequence like S0T10, found \"" +
                             std::string(text) + "\"",
                         "harmonicId");
    }
    else
    {
        card.harmonicId = reader.requiredInteger(3, "harmonicId");
    }
    card.scaleFactor = reader.requiredReal(4, "scaleFactor");
    card.radialMoment = reader.real(5, "radialMoment", 0.0);
    card.azimuthalMoment = reader.real(6, "azimuthalMoment", 0.0);
    card.axialMoment = reader.real(7, "axialMoment", 0.0);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "the card has 7 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymRingMomentLoadFormat::check(const AxisymRingMomentLoadCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.ringId, "ringId"))
        return problem;
    // The format says HID is zero or more (the first harmonic of a conical shell is 0).
    if (card.harmonicId < 0)
        return CardProblem{"card.bad-value", "the harmonic cannot be negative", "harmonicId"};
    if (card.isSequence() && card.harmonicEnd < card.harmonicId)
        return CardProblem{"card.bad-value", "the end of a sequence of harmonics cannot be below its start",
                           "harmonicId"};
    if (const auto problem = checkReal(card.scaleFactor, "scaleFactor", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.radialMoment, "radialMoment", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.azimuthalMoment, "azimuthalMoment", RealRule::Finite))
        return problem;
    return checkReal(card.axialMoment, "axialMoment", RealRule::Finite);
}

void AxisymRingMomentLoadFormat::write(const AxisymRingMomentLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.ringId);
    if (card.isSequence())
        writer.text(3, "S" + std::to_string(card.harmonicId) + "T" + std::to_string(card.harmonicEnd));
    else
        writer.integer(3, card.harmonicId);
    writer.real(4, card.scaleFactor);
    writer.realUnlessDefault(5, card.radialMoment, 0.0);
    writer.realUnlessDefault(6, card.azimuthalMoment, 0.0);
    writer.realUnlessDefault(7, card.axialMoment, 0.0);
}

std::uint32_t AxisymRingMomentLoadTable::append(const AxisymRingMomentLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_ringId.push(card.ringId);
    m_harmonicId.push(card.harmonicId);
    m_harmonicEnd.push(card.harmonicEnd);
    m_scaleFactor.push(card.scaleFactor);
    m_radialMoment.push(card.radialMoment);
    m_azimuthalMoment.push(card.azimuthalMoment);
    m_axialMoment.push(card.axialMoment);
    return row;
}

AxisymRingMomentLoadCard AxisymRingMomentLoadTable::card(std::uint32_t row) const
{
    AxisymRingMomentLoadCard card;
    card.id = m_id.get(row);
    card.ringId = m_ringId.get(row);
    card.harmonicId = m_harmonicId.get(row);
    card.harmonicEnd = m_harmonicEnd.get(row);
    card.scaleFactor = m_scaleFactor.get(row);
    card.radialMoment = m_radialMoment.get(row);
    card.azimuthalMoment = m_azimuthalMoment.get(row);
    card.axialMoment = m_axialMoment.get(row);
    return card;
}

void AxisymRingMomentLoadTable::store(std::uint32_t row, const AxisymRingMomentLoadCard& card)
{
    m_id.set(row, card.id);
    m_ringId.set(row, card.ringId);
    m_harmonicId.set(row, card.harmonicId);
    m_harmonicEnd.set(row, card.harmonicEnd);
    m_scaleFactor.set(row, card.scaleFactor);
    m_radialMoment.set(row, card.radialMoment);
    m_azimuthalMoment.set(row, card.azimuthalMoment);
    m_axialMoment.set(row, card.axialMoment);
}

void AxisymRingMomentLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_ringId.keepRows(rows);
    m_harmonicId.keepRows(rows);
    m_harmonicEnd.keepRows(rows);
    m_scaleFactor.keepRows(rows);
    m_radialMoment.keepRows(rows);
    m_azimuthalMoment.keepRows(rows);
    m_axialMoment.keepRows(rows);
}

void AxisymRingMomentLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ring is a RINGAX, a point of the node ID space.
    if (const base::EntityId ringId = m_ringId.get(row); ringId > 0)
        m_ringId.set(row, visitor.visit("ringId", ReferenceTarget::Node, ringId));
}

} // namespace oor::model
