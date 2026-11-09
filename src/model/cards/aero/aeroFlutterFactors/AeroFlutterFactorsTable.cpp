#include "oor/model/cards/aero/aeroFlutterFactors/AeroFlutterFactorsTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/aero/AeroSupport.hpp"
#include "oor/model/cards/aero/BlankReal.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <algorithm>
#include <cmath>

namespace oor::model
{

namespace
{

constexpr int FIRST_FACTOR_FIELD = 2;
constexpr int THRU_FIELD = 3;
constexpr int LAST_THRU_FIELD = 6;

} // namespace

AeroFlutterFactorsTable::AeroFlutterFactorsTable() : m_factorMid(BLANK_REAL) {}

double aeroFlutterFactorsMiddle(const AeroFlutterFactorsCard& card)
{
    const double low = std::min(card.factor1, card.factorLast);
    const double high = std::max(card.factor1, card.factorLast);
    if (card.factorMid && *card.factorMid > low && *card.factorMid < high)
        return *card.factorMid;
    return (card.factor1 + card.factorLast) / 2.0;
}

std::vector<double> aeroFlutterFactorsValues(const AeroFlutterFactorsCard& card)
{
    if (!card.isRange)
        return card.factors;
    const auto count = static_cast<std::size_t>(std::max(card.factorCount, 0));
    std::vector<double> values(count, card.factor1);
    if (count < 2 || card.factor1 == card.factorLast)
        return values;
    values.back() = card.factorLast;
    // The format reference formula for i = 1 to NF, whose terms are both of the sign of FNF - F1 between the two ends:
    // the weight of F1 falls and the weight of FNF rises with i, and the factor of the middle index is FMID.
    const double middle = aeroFlutterFactorsMiddle(card);
    const double total = card.factorCount;
    for (std::size_t index = 1; index + 1 < count; ++index)
    {
        const double position = static_cast<double>(index + 1);
        const double towardEnd = (card.factorLast - middle) * (total - position);
        const double fromStart = (middle - card.factor1) * (position - 1.0);
        values[index] = (card.factor1 * towardEnd + card.factorLast * fromStart) / (towardEnd + fromStart);
    }
    return values;
}

std::optional<AeroFlutterFactorsCard> AeroFlutterFactorsFormat::read(FieldReader& reader)
{
    AeroFlutterFactorsCard card;
    card.id = reader.requiredInteger(1, "id");
    if ((reader.text(THRU_FIELD) == "Through"))
    {
        card.isRange = true;
        card.factor1 = reader.requiredReal(FIRST_FACTOR_FIELD, "factor1");
        card.factorLast = reader.requiredReal(4, "factorLast");
        card.factorCount = reader.requiredInteger(5, "factorCount");
        card.factorMid = reader.optionalReal(6, "factorMid");
        if (reader.lastUsedField() > LAST_THRU_FIELD)
            reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " with THRU has 6 fields", "");
    }
    else
    {
        card.factors = readRealList(reader, FIRST_FACTOR_FIELD, reader.slotCount(), "factors");
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroFlutterFactorsFormat::check(const AeroFlutterFactorsCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (!card.isRange)
    {
        // The members of the THRU form are not written by this form: they would be lost.
        if (card.factor1 != 0.0 || card.factorLast != 0.0 || card.factorCount != 0 || card.factorMid)
            return CardProblem{"card.bad-value",
                               "{field:factor1}, {field:factorLast}, {field} and {field:factorMid} belong to the THRU "
                               "form of the card",
                               "factorCount"};
        if (card.factors.empty())
            return CardProblem{"card.missing-field", "the card needs at least one factor ({field})", "factor1"};
        return checkRealListFinite(card.factors, "factors");
    }
    if (!card.factors.empty())
        return CardProblem{"card.bad-value", "a card in the THRU form lists no factors", "factor1"};
    if (const auto problem = checkReal(card.factor1, "factor1", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.factorLast, "factorLast", RealRule::Finite))
        return problem;
    if (card.factorCount < 1)
        return CardProblem{"card.bad-value", "{field} must be a positive integer", "factorCount"};
    if (card.factorCount > MAX_FLUTTER_FACTOR_COUNT)
        return CardProblem{"card.bad-value", "{field} is at most 1000000", "factorCount"};
    return checkOptionalReal(card.factorMid, "factorMid", RealRule::Finite);
}

void AeroFlutterFactorsFormat::write(const AeroFlutterFactorsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (!card.isRange)
    {
        writeRealList(writer, FIRST_FACTOR_FIELD, card.factors);
        return;
    }
    writer.real(FIRST_FACTOR_FIELD, card.factor1);
    writer.text(THRU_FIELD, "Through");
    writer.real(4, card.factorLast);
    writer.integer(5, card.factorCount);
    if (card.factorMid)
        writer.real(6, *card.factorMid);
}

std::uint32_t AeroFlutterFactorsTable::append(const AeroFlutterFactorsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_factors.push(card.factors);
    m_isRange.push(static_cast<std::uint8_t>(card.isRange));
    m_factor1.push(card.factor1);
    m_factorLast.push(card.factorLast);
    m_factorCount.push(card.factorCount);
    m_factorMid.push(realToColumn(card.factorMid));
    return row;
}

AeroFlutterFactorsCard AeroFlutterFactorsTable::card(std::uint32_t row) const
{
    AeroFlutterFactorsCard card;
    card.id = m_id.get(row);
    const std::span<const double> factors = m_factors.get(row);
    card.factors.assign(factors.begin(), factors.end());
    card.isRange = m_isRange.get(row) != 0;
    card.factor1 = m_factor1.get(row);
    card.factorLast = m_factorLast.get(row);
    card.factorCount = m_factorCount.get(row);
    card.factorMid = realFromColumn(m_factorMid.get(row));
    return card;
}

void AeroFlutterFactorsTable::store(std::uint32_t row, const AeroFlutterFactorsCard& card)
{
    m_id.set(row, card.id);
    m_factors.set(row, card.factors);
    m_isRange.set(row, static_cast<std::uint8_t>(card.isRange));
    m_factor1.set(row, card.factor1);
    m_factorLast.set(row, card.factorLast);
    m_factorCount.set(row, card.factorCount);
    m_factorMid.set(row, realToColumn(card.factorMid));
}

void AeroFlutterFactorsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_factors.keepRows(rows);
    m_isRange.keepRows(rows);
    m_factor1.keepRows(rows);
    m_factorLast.keepRows(rows);
    m_factorCount.keepRows(rows);
    m_factorMid.keepRows(rows);
}

void AeroFlutterFactorsTable::visitReferences(std::uint32_t, ReferenceVisitor&)
{
    // The factors are reals: an FLFACT names no other card.
}

} // namespace oor::model
