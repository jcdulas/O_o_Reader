#include "oor/model/cards/thermal/radiationBands/RadiationBandsTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/core/Model.hpp"

#include <cstddef>
#include <string>

namespace oor::model
{

namespace
{

constexpr int FIRST_WAVELENGTH_FIELD = 3;
// The fewest wave bands: one wavelength splits the spectrum in two.
constexpr base::EntityId MIN_NUMBER = 2;

} // namespace

std::optional<RadiationBandsCard> RadiationBandsFormat::read(FieldReader& reader)
{
    RadiationBandsCard card;
    card.bandCount = reader.requiredInteger(1, "bandCount");
    card.planckSecondConstant = reader.requiredReal(2, "planckSecondConstant");
    // The wavelengths follow in consecutive fields over as many lines as the card has. The wavelengths below NUMBER are
    // required, so a blank field between two of them is an error and not padding.
    const int last = reader.lastUsedField();
    for (int field = FIRST_WAVELENGTH_FIELD; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
        {
            reader.error("card.missing-field",
                         "a wavelength " + std::string("bandWavelength") + "i may not be blank before the last one",
                         "bandWavelength");
            break;
        }
        card.wavelengths.push_back(reader.requiredReal(field, "bandWavelength"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RadiationBandsFormat::check(const RadiationBandsCard& card)
{
    if (card.bandCount < MIN_NUMBER)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + ", the number of wave bands, must be 2 or more",
                           "bandCount"};
    if (const auto problem = checkReal(card.planckSecondConstant, "planckSecondConstant", RealRule::Positive))
        return problem;
    // The first band ends at LAMBDA1 and the last starts at LAMBDAn, n = NUMBER - 1; the fields from NUMBER
    // on are blank.
    if (card.wavelengths.size() != static_cast<std::size_t>(card.bandCount - 1))
        return CardProblem{"card.bad-value",
                           "a card of " + std::to_string(card.bandCount) + " bands has " +
                               std::to_string(card.bandCount - 1) + " wavelengths " + std::string(CardProblem::FIELD) +
                               "i, not " + std::to_string(card.wavelengths.size()),
                           "bandWavelength"};
    for (std::size_t index = 0; index < card.wavelengths.size(); ++index)
    {
        if (const auto problem = checkReal(card.wavelengths[index], "bandWavelength", RealRule::NonNegative))
            return problem;
        // Ascending, and a repeated value is a band of zero width.
        if (index > 0 && card.wavelengths[index] < card.wavelengths[index - 1])
            return CardProblem{"card.bad-value",
                               "the wavelengths " + std::string(CardProblem::FIELD) + "i must be in ascending order",
                               "bandWavelength"};
    }
    return std::nullopt;
}

void RadiationBandsFormat::write(const RadiationBandsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.bandCount);
    writer.real(2, card.planckSecondConstant);
    int field = FIRST_WAVELENGTH_FIELD;
    for (const double wavelength : card.wavelengths)
        writer.real(field++, wavelength);
}

std::uint32_t RadiationBandsTable::append(const RadiationBandsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_bandCount.push(card.bandCount);
    m_planckSecondConstant.push(card.planckSecondConstant);
    m_wavelengths.push(card.wavelengths);
    return row;
}

RadiationBandsCard RadiationBandsTable::card(std::uint32_t row) const
{
    RadiationBandsCard card;
    card.bandCount = m_bandCount.get(row);
    card.planckSecondConstant = m_planckSecondConstant.get(row);
    const auto wavelengths = m_wavelengths.get(row);
    card.wavelengths.assign(wavelengths.begin(), wavelengths.end());
    return card;
}

void RadiationBandsTable::store(std::uint32_t row, const RadiationBandsCard& card)
{
    m_bandCount.set(row, card.bandCount);
    m_planckSecondConstant.set(row, card.planckSecondConstant);
    m_wavelengths.set(row, card.wavelengths);
}

void RadiationBandsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_bandCount.keepRows(rows);
    m_planckSecondConstant.keepRows(rows);
    m_wavelengths.keepRows(rows);
}

void RadiationBandsTable::finishRead(Model&, const ReadContext& context)
{
    if (m_rows.aliveCount() > 1)
        context.sink.error("deck.duplicate-radiation-bands",
                           "only one " + std::string(NAME) + " may appear in the Bulk Data; the first one is used");
}

} // namespace oor::model
