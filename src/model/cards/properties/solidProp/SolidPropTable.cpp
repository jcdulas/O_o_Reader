#include "oor/model/cards/properties/solidProp/SolidPropTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

namespace
{

// What each of the four fields accepts: a spelling as written (an integer code or a word, short ones included) and the
// word it stands for.
struct Spelling
{
    SolidPropOption option;
    std::string_view written;
    std::string_view word;
};
constexpr Spelling SPELLINGS[] = {{SolidPropOption::Integration, "0", "BubbleFunctions"},
                                  {SolidPropOption::Integration, "BubbleFunctions", "BubbleFunctions"},
                                  {SolidPropOption::Integration, "1", "Gauss"},
                                  {SolidPropOption::Integration, "Gauss", "Gauss"},
                                  {SolidPropOption::Integration, "2", "TwoPoints"},
                                  {SolidPropOption::Integration, "TwoPoints", "TwoPoints"},
                                  {SolidPropOption::Integration, "3", "ThreePoints"},
                                  {SolidPropOption::Integration, "ThreePoints", "ThreePoints"},
                                  {SolidPropOption::Stress, "0", "Node"},
                                  {SolidPropOption::Stress, "Node", "Node"},
                                  {SolidPropOption::Stress, "1", "Gauss"},
                                  {SolidPropOption::Stress, "Gauss", "Gauss"},
                                  {SolidPropOption::Isop, "0", "Reduced"},
                                  {SolidPropOption::Isop, "Reduced", "Reduced"},
                                  {SolidPropOption::Isop, "1", "Full"},
                                  {SolidPropOption::Isop, "Full", "Full"},
                                  {SolidPropOption::Function, "SolidMechanics", "SolidMechanics"},
                                  {SolidPropOption::Function, "SolidMechanics", "SolidMechanics"},
                                  {SolidPropOption::Function, "PressureFluid", "PressureFluid"},
                                  {SolidPropOption::Function, "PressureFluid", "PressureFluid"},
                                  {SolidPropOption::Function, "FreeFluid", "FreeFluid"},
                                  {SolidPropOption::Function, "FreeFluid", "FreeFluid"},
                                  {SolidPropOption::Function, "Poroelastic", "Poroelastic"},
                                  {SolidPropOption::Function, "SolidShell", "SolidShell"},
                                  {SolidPropOption::Function, "SolidShell", "SolidShell"}};

} // namespace

std::optional<std::string> SolidPropFormat::canonicalOption(SolidPropOption which, std::string_view text)
{
    const std::string_view trimmed = base::trimAscii(text);
    if (trimmed.empty())
        return which == SolidPropOption::Function ? std::string(SolidPropCard::DEFAULT_ELEMENT_FUNCTION)
                                                  : std::string();
    for (const Spelling& spelling : SPELLINGS)
    {
        if (spelling.option == which && (trimmed == spelling.written))
            return std::string(spelling.word);
    }
    return std::nullopt;
}

std::optional<SolidPropCard> SolidPropFormat::read(FieldReader& reader)
{
    SolidPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.matFrameId = reader.integer(3, "matFrameId", 0);
    const auto readOption =
        [&](SolidPropOption which, int field, const char* name, const char* accepted, std::string& target)
    {
        if (const auto word = canonicalOption(which, reader.text(field)))
            target = *word;
        else
            reader.error(
                "card.bad-value",
                std::string(name) + " must be " + accepted + ", found \"" + std::string(reader.text(field)) + "\"",
                name);
    };
    readOption(SolidPropOption::Integration, 4, "integration", "BUBBLE, TWO, THREE (or 0, 2, 3) or blank",
               card.integration);
    readOption(SolidPropOption::Stress, 5, "stressLocation", "GRID, GAUSS (or 0, 1) or blank", card.stressLocation);
    readOption(SolidPropOption::Isop, 6, "integrationOrder", "REDUCED, FULL (or 0, 1) or blank", card.integrationOrder);
    readOption(SolidPropOption::Function, 7, "elementFunction", "SMECH, PFLUID, FFLUID, PORO, PSLDSHL or blank",
               card.elementFunction);
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has seven fields", "elementFunction");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SolidPropFormat::check(const SolidPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.matId <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "matId"};
    if (card.matFrameId < -2)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a coordinate system ID, 0, -1 or -2",
                           "matFrameId"};
    if (canonicalOption(SolidPropOption::Integration, card.integration) != card.integration)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be BUBBLE, TWO, THREE or blank",
                           "integration"};
    if (canonicalOption(SolidPropOption::Stress, card.stressLocation) != card.stressLocation)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be GRID, GAUSS or blank",
                           "stressLocation"};
    if (canonicalOption(SolidPropOption::Isop, card.integrationOrder) != card.integrationOrder)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be REDUCED, FULL or blank",
                           "integrationOrder"};
    if (canonicalOption(SolidPropOption::Function, card.elementFunction) != card.elementFunction)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be SMECH, PFLUID, FFLUID, PORO or PSLDSHL",
                           "elementFunction"};
    return std::nullopt;
}

void SolidPropFormat::write(const SolidPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.integerUnlessDefault(3, card.matFrameId, 0);
    if (!card.integration.empty())
        writer.text(4, card.integration);
    if (!card.stressLocation.empty())
        writer.text(5, card.stressLocation);
    if (!card.integrationOrder.empty())
        writer.text(6, card.integrationOrder);
    writer.textUnlessDefault(7, card.elementFunction, SolidPropCard::DEFAULT_ELEMENT_FUNCTION);
}

std::uint32_t SolidPropTable::append(const SolidPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_matFrameId.push(card.matFrameId);
    m_integration.push(card.integration);
    m_stressLocation.push(card.stressLocation);
    m_integrationOrder.push(card.integrationOrder);
    m_elementFunction.push(card.elementFunction);
    return row;
}

SolidPropCard SolidPropTable::card(std::uint32_t row) const
{
    SolidPropCard card;
    card.id = m_id.get(row);
    card.matId = m_matId.get(row);
    card.matFrameId = m_matFrameId.get(row);
    card.integration = m_integration.get(row);
    card.stressLocation = m_stressLocation.get(row);
    card.integrationOrder = m_integrationOrder.get(row);
    card.elementFunction = m_elementFunction.get(row);
    return card;
}

void SolidPropTable::store(std::uint32_t row, const SolidPropCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_matFrameId.set(row, card.matFrameId);
    m_integration.set(row, card.integration);
    m_stressLocation.set(row, card.stressLocation);
    m_integrationOrder.set(row, card.integrationOrder);
    m_elementFunction.set(row, card.elementFunction);
}

void SolidPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_matFrameId.keepRows(rows);
    m_integration.keepRows(rows);
    m_stressLocation.keepRows(rows);
    m_integrationOrder.keepRows(rows);
    m_elementFunction.keepRows(rows);
}

void SolidPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
    // CORDM 0, -1 and -2 are not coordinate systems of the model.
    if (const base::EntityId matFrameId = m_matFrameId.get(row); matFrameId > 0)
        m_matFrameId.set(row, visitor.visit("matFrameId", ReferenceTarget::Coordinate, matFrameId));
}

} // namespace oor::model
