#include "oor/model/cards/solution_control/campbellDiagram/CampbellDiagramTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/cards/solution_control/RotorSupport.hpp"

namespace oor::model
{

namespace
{

constexpr std::string_view SPEED_WORD = "Speed";

} // namespace

std::optional<CampbellDiagramCard> CampbellDiagramFormat::read(FieldReader& reader)
{
    CampbellDiagramCard card;
    card.id = reader.requiredInteger(1, "id");
    card.varyingParameter = std::string(reader.text(2));
    card.discreteValuesId = reader.requiredInteger(3, "discreteValuesId");
    card.type = readOptionalKeyword<RotorSpeedUnit>(reader, 4, "speedUnit", ROTOR_SPEED_UNIT_WORDS);
    card.modeTrackingMethod =
        reader.integer(9, "modeTrackingMethod", CampbellDiagramCard::DEFAULT_MODE_TRACKING_METHOD);
    card.correlationThreshold =
        reader.real(10, "correlationThreshold", CampbellDiagramCard::DEFAULT_CORRELATION_THRESHOLD);
    card.trackingSwitch = reader.integer(11, "trackingSwitch", CampbellDiagramCard::DEFAULT_TRACKING_SWITCH);
    card.trackedModeCount = reader.optionalInteger(12, "trackedModeCount");
    card.printCorrelation = reader.integer(13, "printCorrelation", CampbellDiagramCard::DEFAULT_PRINT_CORRELATION);
    requireOnlyFields(reader, {1, 2, 3, 4, 9, 10, 11, 12, 13});
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CampbellDiagramFormat::check(const CampbellDiagramCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (!card.varyingParameter.empty() && card.varyingParameter != SPEED_WORD)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be Speed, the only variable parameter, or blank",
                           "varyingParameter"};
    if (const auto problem = checkRequiredId(card.discreteValuesId, "discreteValuesId"))
        return problem;
    if (card.modeTrackingMethod < 0 || card.modeTrackingMethod > 2)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) +
                               " must be 0 (numerical), 1 (numerical and eigenvector based) or 2 (eigenvector based)",
                           "modeTrackingMethod"};
    if (const auto problem = checkReal(card.correlationThreshold, "correlationThreshold", RealRule::Finite))
        return problem;
    if (card.trackingSwitch != 0 && card.trackingSwitch != 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 or 1", "trackingSwitch"};
    if (card.trackedModeCount && *card.trackedModeCount <= 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "trackedModeCount"};
    if (card.printCorrelation != 0 && card.printCorrelation != 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 or 1", "printCorrelation"};
    return std::nullopt;
}

void CampbellDiagramFormat::write(const CampbellDiagramCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.varyingParameter);
    writer.integer(3, card.discreteValuesId);
    writeOptionalKeyword(writer, 4, ROTOR_SPEED_UNIT_WORDS, card.type);
    writer.integerUnlessDefault(9, card.modeTrackingMethod, CampbellDiagramCard::DEFAULT_MODE_TRACKING_METHOD);
    writer.realUnlessDefault(10, card.correlationThreshold, CampbellDiagramCard::DEFAULT_CORRELATION_THRESHOLD);
    writer.integerUnlessDefault(11, card.trackingSwitch, CampbellDiagramCard::DEFAULT_TRACKING_SWITCH);
    if (card.trackedModeCount)
        writer.integer(12, *card.trackedModeCount);
    writer.integerUnlessDefault(13, card.printCorrelation, CampbellDiagramCard::DEFAULT_PRINT_CORRELATION);
}

std::uint32_t CampbellDiagramTable::append(const CampbellDiagramCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_discreteValuesId.push(card.discreteValuesId);
    m_cards.push(card);
    return row;
}

void CampbellDiagramTable::store(std::uint32_t row, const CampbellDiagramCard& card)
{
    m_id.set(row, card.id);
    m_discreteValuesId.set(row, card.discreteValuesId);
    m_cards.set(row, card);
}

void CampbellDiagramTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_discreteValuesId.keepRows(rows);
    m_cards.keepRows(rows);
}

void CampbellDiagramTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void CampbellDiagramTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    CampbellDiagramCard& card = m_cards.edit(row);
    visitId(visitor, "discreteValuesId", ReferenceTarget::DiscreteValueSet, card.discreteValuesId);
    m_discreteValuesId.set(row, card.discreteValuesId);
}

} // namespace oor::model
