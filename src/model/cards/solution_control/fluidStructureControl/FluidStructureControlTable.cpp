#include "oor/model/cards/solution_control/fluidStructureControl/FluidStructureControlTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/cards/solution_control/ExternalName.hpp"

namespace oor::model
{

namespace
{

constexpr int SERVICE_NAME_FIELD = 1;
constexpr int TYPE_FIELD = 2;
constexpr int FREQUENCY_FIELD = 3;
constexpr int ANALYSIS_FIELD = 4; // the last field of the card

} // namespace

std::optional<FluidStructureControlCard> FluidStructureControlFormat::read(FieldReader& reader)
{
    FluidStructureControlCard card;
    card.serviceName = std::string(reader.text(SERVICE_NAME_FIELD));
    card.type = readKeyword(reader, TYPE_FIELD, "couplingKind", FLUID_STRUCTURE_COUPLING_WORDS,
                            FluidStructureControlCard::DEFAULT_TYPE);
    card.frequency = reader.integer(FREQUENCY_FIELD, "frequency", FluidStructureControlCard::DEFAULT_FREQUENCY);
    card.analysis = readKeyword(reader, ANALYSIS_FIELD, "interfaceKind", FLUID_STRUCTURE_INTERFACE_WORDS,
                                FluidStructureControlCard::DEFAULT_ANALYSIS);
    requireBlankFields(reader, {}, ANALYSIS_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FluidStructureControlFormat::check(const FluidStructureControlCard& card)
{
    if (const auto problem = checkExternalName(card.serviceName, "serviceName"))
        return problem;
    if (card.frequency <= 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "frequency"};
    return std::nullopt;
}

void FluidStructureControlFormat::write(const FluidStructureControlCard& card, FieldWriter& writer)
{
    writer.text(SERVICE_NAME_FIELD, card.serviceName);
    writeKeywordUnlessDefault(writer, TYPE_FIELD, FLUID_STRUCTURE_COUPLING_WORDS, card.type,
                              FluidStructureControlCard::DEFAULT_TYPE);
    writer.integerUnlessDefault(FREQUENCY_FIELD, card.frequency, FluidStructureControlCard::DEFAULT_FREQUENCY);
    writeKeywordUnlessDefault(writer, ANALYSIS_FIELD, FLUID_STRUCTURE_INTERFACE_WORDS, card.analysis,
                              FluidStructureControlCard::DEFAULT_ANALYSIS);
}

std::uint32_t FluidStructureControlTable::append(const FluidStructureControlCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_serviceName.push(card.serviceName);
    m_cards.push(card);
    return row;
}

void FluidStructureControlTable::store(std::uint32_t row, const FluidStructureControlCard& card)
{
    m_serviceName.set(row, card.serviceName);
    m_cards.set(row, card);
}

void FluidStructureControlTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_serviceName.keepRows(rows);
    m_cards.keepRows(rows);
}

} // namespace oor::model
