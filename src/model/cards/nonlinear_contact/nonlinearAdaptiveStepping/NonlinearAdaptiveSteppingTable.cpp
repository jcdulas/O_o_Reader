#include "oor/model/cards/nonlinear_contact/nonlinearAdaptiveStepping/NonlinearAdaptiveSteppingTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

namespace oor::model
{

namespace
{

constexpr std::string_view STEP = "Stepping";
constexpr std::string_view CREEP = "Creep";

// The layout of the format reference: the keyword of a section, the field, the line of the section and the place in it.
constexpr ContactFieldPosition POSITIONS[] = {
    {STEP, {"MinStepRatio", ContactParameterKind::Real}, 0, 2},
    {STEP, {"MaxStepRatio", ContactParameterKind::Real}, 0, 3},
    {STEP, {"MinTimeStep", ContactParameterKind::Real}, 0, 4},
    {STEP, {"MaxTimeStep", ContactParameterKind::Real}, 0, 5},
    {STEP, {"MaxSubsteps", ContactParameterKind::Integer}, 0, 6},
    {STEP, {"RecycleCount", ContactParameterKind::Integer}, 0, 7},
    {STEP, {"TimeStepScaleFactor", ContactParameterKind::Real}, 0, 8},
    {STEP, {"DampingKind", ContactParameterKind::Integer}, 1, 2},
    {STEP, {"DampingFactor", ContactParameterKind::Real}, 1, 3},
    {CREEP, {"CreepStepCheck", ContactParameterKind::Integer}, 0, 2},
    {CREEP, {"TensionCutoffStrain", ContactParameterKind::Real}, 0, 3},
    {CREEP, {"TensionCutoffStress", ContactParameterKind::Real}, 0, 4},
    {CREEP, {"CutoffTime", ContactParameterKind::Real}, 0, 5},
};

constexpr std::string_view KEYWORDS[] = {STEP, CREEP};

} // namespace

std::span<const ContactParameterRule> NonlinearAdaptiveSteppingFormat::rules()
{
    static const std::vector<ContactParameterRule> ALL = contactFieldRules(POSITIONS);
    return ALL;
}

std::optional<NonlinearAdaptiveSteppingCard> NonlinearAdaptiveSteppingFormat::read(FieldReader& reader)
{
    NonlinearAdaptiveSteppingCard card;
    card.id = reader.requiredInteger(1, "id");
    requireUnusedBlank(reader, {2, 3, 4, 5, 6, 7, 8}, reader.lastUsedField());
    card.sections = readContactNamedSections(reader, KEYWORDS, POSITIONS, NonlinearAdaptiveSteppingTable::NAME);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonlinearAdaptiveSteppingFormat::check(const NonlinearAdaptiveSteppingCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (auto problem = checkContactNamedSections(card.sections, KEYWORDS, POSITIONS, rules(),
                                                 NonlinearAdaptiveSteppingTable::NAME))
        return problem;
    for (const ContactNamedSection& section : card.sections)
    {
        for (const ContactParameter& parameter : section.params)
        {
            const auto* integer = std::get_if<std::int64_t>(&parameter.value);
            if (integer == nullptr)
                continue;
            if (parameter.name == "DampingKind" && *integer != 0 && *integer != 4)
                return CardProblem{"card.bad-value", std::string("DampingKind") + " must be 0 or 4", "sections"};
            if (parameter.name == "CreepStepCheck" && *integer != 0 && *integer != 1)
                return CardProblem{"card.bad-value", std::string("CreepStepCheck") + " must be 0 or 1", "sections"};
        }
    }
    return std::nullopt;
}

void NonlinearAdaptiveSteppingFormat::write(const NonlinearAdaptiveSteppingCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeContactNamedSections(writer, card.sections, POSITIONS);
}

std::uint32_t NonlinearAdaptiveSteppingTable::append(const NonlinearAdaptiveSteppingCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void NonlinearAdaptiveSteppingTable::store(std::uint32_t row, const NonlinearAdaptiveSteppingCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void NonlinearAdaptiveSteppingTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void NonlinearAdaptiveSteppingTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
