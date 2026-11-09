#include "oor/model/cards/nodes/axisymFluidControl/AxisymFluidControlTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/core/Model.hpp"

namespace oor::model
{

namespace
{

// The harmonics start on the first continuation line and fill eight fields on each line.
constexpr int FIRST_HARMONIC_FIELD = 9;
constexpr int FIELDS_PER_LINE = 8;
constexpr int FIELDS_OF_RANGE = 3;           // N1 THRU Ni
constexpr int FIELDS_OF_RANGE_WITH_STEP = 5; // N1 THRU Ni STEP NS

// The first field after `field` that is not blank, or 0 when there is none up to `last`.
int nextUsedField(const FieldReader& reader, int field, int last)
{
    for (int next = field + 1; next <= last; ++next)
    {
        if (!reader.isBlank(next))
            return next;
    }
    return 0;
}

// Reads the list of harmonics from the first continuation line on. A blank field is skipped (the padding of a line), a
// number may be followed by THRU and a last number, and that by STEP and a step.
std::vector<AxisymFluidControlHarmonics> readHarmonics(FieldReader& reader)
{
    std::vector<AxisymFluidControlHarmonics> harmonics;
    const int last = reader.lastUsedField();
    int field = nextUsedField(reader, FIRST_HARMONIC_FIELD - 1, last);
    while (field != 0 && !reader.failed())
    {
        AxisymFluidControlHarmonics item =
            AxisymFluidControlHarmonics::single(reader.requiredInteger(field, "harmonic"));
        int after = nextUsedField(reader, field, last);
        if (!reader.failed() && after != 0 && (reader.text(after) == "Through"))
        {
            const int endField = nextUsedField(reader, after, last);
            if (endField == 0)
            {
                reader.error("card.missing-field", "THRU needs a last harmonic", "lastHarmonic");
                break;
            }
            item.last = reader.requiredInteger(endField, "lastHarmonic");
            item.range = true;
            after = nextUsedField(reader, endField, last);
            if (!reader.failed() && after != 0 && (reader.text(after) == "Step"))
            {
                const int stepField = nextUsedField(reader, after, last);
                if (stepField == 0)
                {
                    reader.error("card.missing-field", "STEP needs a step", "harmonicStep");
                    break;
                }
                item.step = reader.requiredInteger(stepField, "harmonicStep");
                if (!reader.failed() && item.step < 1)
                    reader.error("card.bad-value", "{field} must be a positive integer", "harmonicStep");
                after = nextUsedField(reader, stepField, last);
            }
        }
        harmonics.push_back(item);
        field = after;
    }
    return harmonics;
}

// The problem with the list of harmonics, if there is one: numbers from 0 to 99, a range going up, a step only on a
// range and above zero, and every item above the one before it (the format: an increasing sequence).
std::optional<CardProblem> harmonicsProblem(const std::vector<AxisymFluidControlHarmonics>& harmonics)
{
    std::int32_t previous = -1;
    for (const AxisymFluidControlHarmonics& item : harmonics)
    {
        if (item.first < 0 || item.first > AxisymFluidControlHarmonics::MAX_HARMONIC)
            return CardProblem{"card.bad-value", "a harmonic must be an integer from 0 to 99", "harmonic"};
        if (!item.range && (item.last != item.first || item.step != AxisymFluidControlHarmonics::NO_STEP))
            return CardProblem{"card.bad-value", "an item without THRU is one harmonic and has no step", "harmonic"};
        if (item.range)
        {
            if (item.last <= item.first || item.last > AxisymFluidControlHarmonics::MAX_HARMONIC)
                return CardProblem{"card.bad-value", "the end of a range must be above its start and at most 99",
                                   "lastHarmonic"};
            if (item.step != AxisymFluidControlHarmonics::NO_STEP && item.step < 1)
                return CardProblem{"card.bad-value", "{field} must be a positive integer", "harmonicStep"};
        }
        if (item.first <= previous)
            return CardProblem{"card.bad-value", "the harmonics must be in increasing order", "harmonic"};
        previous = item.last;
    }
    return std::nullopt;
}

} // namespace

std::optional<AxisymFluidControlCard> AxisymFluidControlFormat::read(FieldReader& reader)
{
    AxisymFluidControlCard card;
    card.frameId = reader.requiredInteger(1, "frameId");
    card.gravity = reader.requiredReal(2, "gravity");
    card.defaultDensity = reader.optionalReal(3, "defaultDensity");
    card.defaultBulkModulus = reader.optionalReal(4, "defaultBulkModulus");
    card.nonsymmetric =
        readRequiredKeyword<AxisymFluidControlSymmetry>(reader, 5, "nonsymmetric", AXISYM_FLUID_SYMMETRY_WORDS);
    bool noHarmonics = false; // F = NONE
    if (!reader.isBlank(6))
    {
        if ((reader.text(6) == "None"))
            noHarmonics = true;
        else
            reader.error(
                "card.bad-value",
                std::string("noHarmonicsFlag") + " must be blank or NONE, not \"" + std::string(reader.text(6)) + "\"",
                "noHarmonicsFlag");
    }
    // The harmonics are on the continuation lines: the last two fields of the first line are not used.
    if (!reader.isBlank(7) || !reader.isBlank(8))
        reader.error("card.bad-value",
                     "fields 7 and 8 of an " + std::string(reader.cardName()) + " are not used and must be blank", "");
    card.harmonics = readHarmonics(reader);
    // F = NONE means no harmonics and no continuation line, and a blank F says the harmonics are listed.
    if (!reader.failed() && noHarmonics && !card.harmonics.empty())
        reader.error("card.bad-value", "no harmonic may be listed when " + std::string("noHarmonicsFlag") + " is NONE",
                     "noHarmonicsFlag");
    if (!reader.failed() && !noHarmonics && card.harmonics.empty())
        reader.error("card.missing-field",
                     "the harmonics must be listed when " + std::string("noHarmonicsFlag") + " is blank (or " +
                         std::string("noHarmonicsFlag") + " must be NONE)",
                     "harmonic");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymFluidControlFormat::check(const AxisymFluidControlCard& card)
{
    if (const auto problem = checkRequiredId(card.frameId, "frameId"))
        return problem;
    if (const auto problem = checkReal(card.gravity, "gravity", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.defaultDensity, "defaultDensity", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.defaultBulkModulus, "defaultBulkModulus", RealRule::Finite))
        return problem;
    // An empty list is F = NONE, so only the numbers listed can be wrong.
    return harmonicsProblem(card.harmonics);
}

void AxisymFluidControlFormat::write(const AxisymFluidControlCard& card, FieldWriter& writer)
{
    writer.integer(1, card.frameId);
    writer.real(2, card.gravity);
    if (card.defaultDensity)
        writer.real(3, *card.defaultDensity);
    if (card.defaultBulkModulus)
        writer.real(4, *card.defaultBulkModulus);
    writeKeyword(writer, 5, AXISYM_FLUID_SYMMETRY_WORDS, card.nonsymmetric);
    if (card.harmonics.empty())
        writer.text(6, "None"); // the card with no harmonics
    int field = FIRST_HARMONIC_FIELD;
    for (const AxisymFluidControlHarmonics& item : card.harmonics)
    {
        int size = 1;
        if (item.range)
            size = item.step != AxisymFluidControlHarmonics::NO_STEP ? FIELDS_OF_RANGE_WITH_STEP : FIELDS_OF_RANGE;
        // A range never goes over the end of a line: it starts the next one.
        const int offset = (field - FIRST_HARMONIC_FIELD) % FIELDS_PER_LINE;
        if (offset + size > FIELDS_PER_LINE)
            field += FIELDS_PER_LINE - offset;
        writer.integer(field, item.first);
        if (item.range)
        {
            writer.text(field + 1, "Through");
            writer.integer(field + 2, item.last);
            if (item.step != AxisymFluidControlHarmonics::NO_STEP)
            {
                writer.text(field + 3, "Step");
                writer.integer(field + 4, item.step);
            }
        }
        field += size;
    }
}

std::uint32_t AxisymFluidControlTable::append(const AxisymFluidControlCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_harmonicCount.push(static_cast<std::int32_t>(card.harmonics.size()));
    m_cards.push(card);
    return row;
}

void AxisymFluidControlTable::store(std::uint32_t row, const AxisymFluidControlCard& card)
{
    m_harmonicCount.set(row, static_cast<std::int32_t>(card.harmonics.size()));
    m_cards.set(row, card);
}

void AxisymFluidControlTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_harmonicCount.keepRows(rows);
    m_cards.keepRows(rows);
}

void AxisymFluidControlTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "frameId", ReferenceTarget::Coordinate, m_cards.edit(row).frameId);
}

void AxisymFluidControlTable::finishRead(Model&, const ReadContext& context)
{
    if (m_rows.aliveCount() > 1)
        context.sink.error("deck.duplicate-axisym-fluid-control",
                           "only one " + std::string(NAME) + " may appear in the Bulk Data; the first one is used");
}

} // namespace oor::model
