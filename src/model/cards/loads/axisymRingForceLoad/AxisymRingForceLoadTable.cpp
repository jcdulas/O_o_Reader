#include "oor/model/cards/loads/axisymRingForceLoad/AxisymRingForceLoadTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <charconv>
#include <string>

namespace oor::model
{

namespace
{

// Reads "n" at the start of `text` as a non-negative integer; the rest of the text follows in `rest`.
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

std::optional<AxisymRingForceLoadCard> AxisymRingForceLoadFormat::read(FieldReader& reader)
{
    AxisymRingForceLoadCard card;
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
    card.radialForce = reader.real(5, "radialForce", 0.0);
    card.azimuthalForce = reader.real(6, "azimuthalForce", 0.0);
    card.axialForce = reader.real(7, "axialForce", 0.0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymRingForceLoadFormat::check(const AxisymRingForceLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    if (card.ringId <= 0)
        return CardProblem{"card.bad-id", "the ring ID must be a positive integer", "ringId"};
    // The harmonic 0 is accepted, so HID is not limited to positive values.
    if (card.harmonicId < 0)
        return CardProblem{"card.bad-value", "the harmonic cannot be negative", "harmonicId"};
    if (card.isSequence() && card.harmonicEnd < card.harmonicId)
        return CardProblem{"card.bad-value", "the end of a sequence of harmonics cannot be below its start",
                           "harmonicId"};
    return std::nullopt;
}

void AxisymRingForceLoadFormat::write(const AxisymRingForceLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.ringId);
    if (card.isSequence())
        writer.text(3, "S" + std::to_string(card.harmonicId) + "T" + std::to_string(card.harmonicEnd));
    else
        writer.integer(3, card.harmonicId);
    writer.real(4, card.scaleFactor);
    writer.realUnlessDefault(5, card.radialForce, 0.0);
    writer.realUnlessDefault(6, card.azimuthalForce, 0.0);
    writer.realUnlessDefault(7, card.axialForce, 0.0);
}

std::uint32_t AxisymRingForceLoadTable::append(const AxisymRingForceLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_ringId.push(card.ringId);
    m_harmonicId.push(card.harmonicId);
    m_harmonicEnd.push(card.harmonicEnd);
    m_scaleFactor.push(card.scaleFactor);
    m_radialForce.push(card.radialForce);
    m_azimuthalForce.push(card.azimuthalForce);
    m_axialForce.push(card.axialForce);
    return row;
}

AxisymRingForceLoadCard AxisymRingForceLoadTable::card(std::uint32_t row) const
{
    AxisymRingForceLoadCard card;
    card.id = m_id.get(row);
    card.ringId = m_ringId.get(row);
    card.harmonicId = m_harmonicId.get(row);
    card.harmonicEnd = m_harmonicEnd.get(row);
    card.scaleFactor = m_scaleFactor.get(row);
    card.radialForce = m_radialForce.get(row);
    card.azimuthalForce = m_azimuthalForce.get(row);
    card.axialForce = m_axialForce.get(row);
    return card;
}

void AxisymRingForceLoadTable::store(std::uint32_t row, const AxisymRingForceLoadCard& card)
{
    m_id.set(row, card.id);
    m_ringId.set(row, card.ringId);
    m_harmonicId.set(row, card.harmonicId);
    m_harmonicEnd.set(row, card.harmonicEnd);
    m_scaleFactor.set(row, card.scaleFactor);
    m_radialForce.set(row, card.radialForce);
    m_azimuthalForce.set(row, card.azimuthalForce);
    m_axialForce.set(row, card.axialForce);
}

void AxisymRingForceLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_ringId.keepRows(rows);
    m_harmonicId.keepRows(rows);
    m_harmonicEnd.keepRows(rows);
    m_scaleFactor.keepRows(rows);
    m_radialForce.keepRows(rows);
    m_azimuthalForce.keepRows(rows);
    m_axialForce.keepRows(rows);
}

void AxisymRingForceLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ring is a RINGAX, a point of the node ID space.
    m_ringId.set(row, visitor.visit("ringId", ReferenceTarget::Node, m_ringId.get(row)));
}

} // namespace oor::model
