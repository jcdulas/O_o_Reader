#include "oor/model/cards/aero/aeroStripProp4/AeroStripProp4Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <array>
#include <string>

namespace oor::model
{

namespace
{

// The strips are triples of consecutive fields from field 6 on (DOCi, CAOCi, GAPOCi), across the continuation lines.
constexpr int FIRST_STRIP_FIELD = 6;
constexpr int FIELDS_PER_STRIP = 3;

// The field of the DOC of strip `index` (counted from 0); CAOC and GAPOC follow it.
int docField(std::size_t index) noexcept
{
    return FIRST_STRIP_FIELD + FIELDS_PER_STRIP * static_cast<int>(index);
}

// The field names of the first strips. A CardProblem keeps a pointer to the name of its field, so the names are
// literals that outlive `check`; a strip beyond the table reports the name of the whole list.
constexpr std::array<const char*, 16> HINGE_OFFSET_RATIO_NAMES = {
    "hingeOffsetRatio1",  "hingeOffsetRatio2",  "hingeOffsetRatio3",  "hingeOffsetRatio4",
    "hingeOffsetRatio5",  "hingeOffsetRatio6",  "hingeOffsetRatio7",  "hingeOffsetRatio8",
    "hingeOffsetRatio9",  "hingeOffsetRatio10", "hingeOffsetRatio11", "hingeOffsetRatio12",
    "hingeOffsetRatio13", "hingeOffsetRatio14", "hingeOffsetRatio15", "hingeOffsetRatio16"};
constexpr std::array<const char*, 16> CONTROL_CHORD_RATIO_NAMES = {
    "controlChordRatio1",  "controlChordRatio2",  "controlChordRatio3",  "controlChordRatio4",
    "controlChordRatio5",  "controlChordRatio6",  "controlChordRatio7",  "controlChordRatio8",
    "controlChordRatio9",  "controlChordRatio10", "controlChordRatio11", "controlChordRatio12",
    "controlChordRatio13", "controlChordRatio14", "controlChordRatio15", "controlChordRatio16"};
constexpr std::array<const char*, 16> GAP_RATIO_NAMES = {
    "gapRatio1", "gapRatio2",  "gapRatio3",  "gapRatio4",  "gapRatio5",  "gapRatio6",  "gapRatio7",  "gapRatio8",
    "gapRatio9", "gapRatio10", "gapRatio11", "gapRatio12", "gapRatio13", "gapRatio14", "gapRatio15", "gapRatio16"};

const char* stripFieldName(const std::array<const char*, 16>& names, const char* unnumbered, std::size_t index) noexcept
{
    return index < names.size() ? names[index] : unnumbered;
}

} // namespace

std::optional<AeroStripProp4Card> AeroStripProp4Format::read(FieldReader& reader)
{
    AeroStripProp4Card card;
    card.id = reader.requiredInteger(1, "id");
    card.liftSlopeOption = reader.integer(2, "liftSlopeOption", 0);
    card.liftSlopeListId = reader.integer(3, "liftSlopeListId", 0);
    card.circulationOption = reader.integer(4, "circulationOption", 0);
    card.circulationListId = reader.integer(5, "circulationListId", 0);
    const int last = reader.lastUsedField();
    for (std::size_t index = 0; docField(index) <= last; ++index)
    {
        // A strip has its three ratios or none: a blank inside the list is an error.
        const int field = docField(index);
        const char* hingeOffsetRatioName = stripFieldName(HINGE_OFFSET_RATIO_NAMES, "hingeOffsetRatios", index);
        const char* controlChordRatioName = stripFieldName(CONTROL_CHORD_RATIO_NAMES, "controlChordRatios", index);
        const char* gapRatioName = stripFieldName(GAP_RATIO_NAMES, "gapRatios", index);
        AeroStripProp4Strip strip;
        strip.hingeOffsetRatio = reader.requiredReal(field, hingeOffsetRatioName);
        strip.controlChordRatio = reader.requiredReal(field + 1, controlChordRatioName);
        strip.gapRatio = reader.requiredReal(field + 2, gapRatioName);
        card.strips.push_back(strip);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroStripProp4Format::check(const AeroStripProp4Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.liftSlopeOption < -1 || card.liftSlopeOption > 1)
        return CardProblem{"card.bad-value", "{field} must be -1, 0 or 1", "liftSlopeOption"};
    if (const auto problem = checkOptionalId(card.liftSlopeListId, "liftSlopeListId"))
        return problem;
    // The lift curve slope data are in the AEFACT whenever the selector asks for them.
    if (card.liftSlopeOption != 0 && card.liftSlopeListId == 0)
        return CardProblem{"card.missing-field", "{field} is required when {field:liftSlopeOption} is not 0",
                           "liftSlopeListId"};
    if (card.circulationOption < 0 || card.circulationOption > 3)
        return CardProblem{"card.bad-value", "{field} must be 0, 1, 2 or 3", "circulationOption"};
    if (card.circulationOption != 0 && card.liftSlopeOption != 0)
        return CardProblem{"card.bad-value", "{field} must be 0 when {field:liftSlopeOption} is not 0",
                           "circulationOption"};
    if (const auto problem = checkOptionalId(card.circulationListId, "circulationListId"))
        return problem;
    if (card.circulationOption != 0 && card.circulationListId == 0)
        return CardProblem{"card.missing-field", "{field} is required when {field:circulationOption} is not 0",
                           "circulationListId"};
    if (card.strips.empty())
        return CardProblem{"card.missing-field",
                           "at least one strip ({field}, {field:controlChordRatio1}, {field:gapRatio1}) is required",
                           "hingeOffsetRatio1"};
    for (std::size_t index = 0; index < card.strips.size(); ++index)
    {
        const AeroStripProp4Strip& strip = card.strips[index];
        const char* hingeOffsetRatioName = stripFieldName(HINGE_OFFSET_RATIO_NAMES, "hingeOffsetRatios", index);
        const char* controlChordRatioName = stripFieldName(CONTROL_CHORD_RATIO_NAMES, "controlChordRatios", index);
        const char* gapRatioName = stripFieldName(GAP_RATIO_NAMES, "gapRatios", index);
        if (const auto problem = checkReal(strip.hingeOffsetRatio, hingeOffsetRatioName, RealRule::NonNegative))
            return problem;
        if (const auto problem = checkReal(strip.controlChordRatio, controlChordRatioName, RealRule::NonNegative))
            return problem;
        if (const auto problem = checkReal(strip.gapRatio, gapRatioName, RealRule::NonNegative))
            return problem;
    }
    return std::nullopt;
}

void AeroStripProp4Format::write(const AeroStripProp4Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.liftSlopeOption, 0);
    writer.integerUnlessDefault(3, card.liftSlopeListId, 0);
    writer.integerUnlessDefault(4, card.circulationOption, 0);
    writer.integerUnlessDefault(5, card.circulationListId, 0);
    // The strips are required, so their ratios are written even when they are zero (no control surface).
    for (std::size_t index = 0; index < card.strips.size(); ++index)
    {
        const int field = docField(index);
        writer.real(field, card.strips[index].hingeOffsetRatio);
        writer.real(field + 1, card.strips[index].controlChordRatio);
        writer.real(field + 2, card.strips[index].gapRatio);
    }
}

std::uint32_t AeroStripProp4Table::append(const AeroStripProp4Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroStripProp4Table::store(std::uint32_t row, const AeroStripProp4Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroStripProp4Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroStripProp4Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AeroStripProp4Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    AeroStripProp4Card& card = m_cards.edit(row);
    visitId(visitor, "liftSlopeListId", ReferenceTarget::AeroFactor, card.liftSlopeListId);
    visitId(visitor, "circulationListId", ReferenceTarget::AeroFactor, card.circulationListId);
}

} // namespace oor::model
