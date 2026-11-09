#include "oor/model/cards/nonlinear_contact/nonlinearStepControl/NonlinearStepControlTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <cmath>
#include <limits>
#include <type_traits>
#include <variant>

namespace oor::model
{

namespace
{

constexpr std::string_view GENERAL = "General";
constexpr std::string_view LCNT = "LoadCaseContinue";
constexpr std::string_view FIXED = "FixedStepping";
constexpr std::string_view ADAPT = "AdaptiveStepping";
constexpr std::string_view ARCLN = "ArcLengthMethod";
constexpr std::string_view HEAT = "HeatAnalysis";
constexpr std::string_view MECH = "MechanicalAnalysis";
constexpr std::string_view COUP = "CoupledAnalysis";

// The layout of the format reference: the keyword of a section, the field, the line of the section and the place in it.
constexpr ContactFieldPosition POSITIONS[] = {
    {GENERAL, {"MaxIterations", ContactParameterKind::Integer}, 0, 2},
    {GENERAL, {"MinIterations", ContactParameterKind::Integer}, 0, 3},
    {GENERAL, {"MaxBisections", ContactParameterKind::Integer}, 0, 4},
    {GENERAL, {"CreepOption", ContactParameterKind::Integer}, 0, 5},
    {LCNT, {"IncrementCountCoupled", ContactParameterKind::Integer}, 0, 2},
    {LCNT, {"ConvergenceCoupled", ContactParameterKind::Text}, 0, 3},
    {LCNT, {"DisplacementToleranceCoupled", ContactParameterKind::Real}, 0, 4},
    {LCNT, {"LoadToleranceCoupled", ContactParameterKind::Real}, 0, 5},
    {LCNT, {"EnergyToleranceCoupled", ContactParameterKind::Real}, 0, 6},
    {LCNT, {"MaxDivergencesCoupled", ContactParameterKind::Integer}, 0, 7},
    {LCNT, {"MaxBisectionsCoupled", ContactParameterKind::Integer}, 0, 8},
    {LCNT, {"MaxIterationsCoupled", ContactParameterKind::Integer}, 1, 2},
    {LCNT, {"MinIterationsCoupled", ContactParameterKind::Integer}, 1, 3},
    {FIXED, {"IncrementCount", ContactParameterKind::Integer}, 0, 2},
    {FIXED, {"OutputInterval", ContactParameterKind::Integer}, 0, 3},
    {ADAPT, {"InitialTimeStepFactor", ContactParameterKind::Real}, 0, 2},
    {ADAPT, {"MinTimeStepFactor", ContactParameterKind::Real}, 0, 3},
    {ADAPT, {"MaxTimeStepFactor", ContactParameterKind::Real}, 0, 4},
    {ADAPT, {"DesiredIterations", ContactParameterKind::Integer}, 0, 5},
    {ADAPT, {"TimeStepScaleFactor", ContactParameterKind::Real}, 0, 6},
    {ADAPT, {"IntermediateOutput", ContactParameterKind::Integer}, 0, 7},
    {ADAPT, {"MaxSubsteps", ContactParameterKind::Integer}, 0, 8},
    {ADAPT, {"DampingKind", ContactParameterKind::Integer}, 1, 2},
    {ADAPT, {"DampingFactor", ContactParameterKind::Real}, 1, 3},
    {ADAPT, {"CriteriaTableId", ContactParameterKind::Integer}, 1, 4},
    {ADAPT, {"PhysicsOption", ContactParameterKind::Integer}, 1, 5},
    {ADAPT, {"TargetLimit", ContactParameterKind::Integer}, 1, 6},
    {ADAPT, {"MinStepRatio", ContactParameterKind::Real}, 1, 7},
    {ADAPT, {"MaxStepRatio", ContactParameterKind::Real}, 1, 8},
    {ADAPT, {"TimeStepAdjustInterval", ContactParameterKind::Integer}, 2, 2},
    {ADAPT, {"StepMultiplier", ContactParameterKind::Integer}, 2, 3},
    {ADAPT, {"TimeStepHoldBounds", ContactParameterKind::Real}, 2, 4},
    {ADAPT, {"DisplacementConvergence", ContactParameterKind::Real}, 2, 5},
    {ARCLN, {"SteppingMethod", ContactParameterKind::Text}, 0, 2},
    {ARCLN, {"InitialTimeStepFactorAdapted", ContactParameterKind::Real}, 0, 3},
    {ARCLN, {"MinAdaptiveRatio", ContactParameterKind::Real}, 0, 4},
    {ARCLN, {"MaxAdaptiveRatio", ContactParameterKind::Real}, 0, 5},
    {ARCLN, {"DesiredIterationsAdapted", ContactParameterKind::Integer}, 0, 7},
    {ARCLN, {"MaxSubstepsAdapted", ContactParameterKind::Integer}, 0, 8},
    {HEAT, {"ConvergenceHeat", ContactParameterKind::Text}, 0, 2},
    {HEAT, {"DisplacementToleranceHeat", ContactParameterKind::Real}, 0, 3},
    {HEAT, {"LoadToleranceHeat", ContactParameterKind::Real}, 0, 4},
    {HEAT, {"EnergyToleranceHeat", ContactParameterKind::Real}, 0, 5},
    {HEAT, {"StiffnessMethodHeat", ContactParameterKind::Text}, 0, 6},
    {HEAT, {"StiffnessStepHeat", ContactParameterKind::Integer}, 0, 7},
    {HEAT, {"MaxQuasiNewtonVectorsHeat", ContactParameterKind::Integer}, 1, 2},
    {HEAT, {"MaxLineSearchesHeat", ContactParameterKind::Integer}, 1, 3},
    {HEAT, {"LineSearchToleranceHeat", ContactParameterKind::Real}, 1, 4},
    {MECH, {"ConvergenceCriteria", ContactParameterKind::Text}, 0, 2},
    {MECH, {"DisplacementTolerance", ContactParameterKind::Real}, 0, 3},
    {MECH, {"LoadTolerance", ContactParameterKind::Real}, 0, 4},
    {MECH, {"EnergyTolerance", ContactParameterKind::Real}, 0, 5},
    {MECH, {"StiffnessMethod", ContactParameterKind::Text}, 0, 6},
    {MECH, {"StiffnessUpdateStep", ContactParameterKind::Integer}, 0, 7},
    {MECH, {"MassRatioConvergence", ContactParameterKind::Integer}, 0, 8},
    {MECH, {"MaxQuasiNewtonVectors", ContactParameterKind::Integer}, 1, 2},
    {MECH, {"MaxLineSearches", ContactParameterKind::Integer}, 1, 3},
    {MECH, {"LineSearchTolerance", ContactParameterKind::Real}, 1, 4},
    {MECH, {"FrictionStress", ContactParameterKind::Real}, 1, 5},
    {COUP, {"HeatFromPlasticWork", ContactParameterKind::Real}, 0, 2},
    {COUP, {"HeatFromFriction", ContactParameterKind::Real}, 0, 3},
};

constexpr std::string_view KEYWORDS[] = {GENERAL, LCNT, FIXED, ADAPT, ARCLN, HEAT, MECH, COUP};

// The integer value of the CRITTID of the ADAPT line, or null when the card has none (the sections may be const).
template <typename Sections>
auto* findCriteriaTable(Sections& sections)
{
    using Value = std::conditional_t<std::is_const_v<Sections>, const std::int64_t, std::int64_t>;
    for (auto& section : sections)
    {
        if (section.keyword != ADAPT)
            continue;
        for (auto& parameter : section.params)
        {
            if (parameter.name == "CriteriaTableId")
                return std::get_if<std::int64_t>(&parameter.value); // null for a blank value
        }
    }
    return static_cast<Value*>(nullptr);
}

} // namespace

std::span<const ContactParameterRule> NonlinearStepControlFormat::rules()
{
    static const std::vector<ContactParameterRule> ALL = contactFieldRules(POSITIONS);
    return ALL;
}

base::EntityId NonlinearStepControlFormat::criteriaTable(const NonlinearStepControlCard& card)
{
    const std::int64_t* table = findCriteriaTable(card.sections);
    if (table == nullptr || *table <= 0 || *table > std::numeric_limits<base::EntityId>::max())
        return 0;
    return static_cast<base::EntityId>(*table);
}

std::optional<NonlinearStepControlCard> NonlinearStepControlFormat::read(FieldReader& reader)
{
    NonlinearStepControlCard card;
    card.id = reader.requiredInteger(1, "id");
    card.totalTime = reader.real(2, "totalTime", 1.0);
    card.controlSetting = std::string(reader.word(3));
    requireUnusedBlank(reader, {4, 5, 6, 7, 8}, reader.lastUsedField());
    card.sections = readContactNamedSections(reader, KEYWORDS, POSITIONS, NonlinearStepControlTable::NAME);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonlinearStepControlFormat::check(const NonlinearStepControlCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (!std::isfinite(card.totalTime))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number", "totalTime"};
    if (!card.controlSetting.empty())
    {
        if (auto problem = checkKeyword(
                card.controlSetting, "controlSetting",
                {"QuasiLinear", "MildlyNonlinear", "SeverelyNonlinear", "LoadCasePerformance", "LoadCaseAccumulation"}))
            return problem;
    }
    if (auto problem =
            checkContactNamedSections(card.sections, KEYWORDS, POSITIONS, rules(), NonlinearStepControlTable::NAME))
        return problem;
    int procedures = 0;
    for (const ContactNamedSection& section : card.sections)
    {
        // one of LCNT, FIXED, ADAPT and ARCLN.
        if (section.keyword == LCNT || section.keyword == FIXED || section.keyword == ADAPT || section.keyword == ARCLN)
            ++procedures;
        for (const ContactParameter& parameter : section.params)
        {
            const auto* word = std::get_if<std::string>(&parameter.value);
            const bool arcLengthMethod =
                word != nullptr && (*word == "Crisfield" || *word == "Riks" || *word == "ModifiedRiks");
            if (parameter.name == "SteppingMethod" && word != nullptr && !arcLengthMethod)
                return CardProblem{"card.bad-value", std::string("SteppingMethod") + " must be CRIS, RIKS or MRIKS",
                                   "sections"};
        }
    }
    if (procedures > 1)
        return CardProblem{"card.bad-value", "only one of LCNT, FIXED, ADAPT and ARCLN may be given", "sections"};
    return std::nullopt;
}

void NonlinearStepControlFormat::write(const NonlinearStepControlCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.totalTime, 1.0);
    if (!card.controlSetting.empty())
        writer.text(3, card.controlSetting);
    writeContactNamedSections(writer, card.sections, POSITIONS);
}

std::uint32_t NonlinearStepControlTable::append(const NonlinearStepControlCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_totalTime.push(card.totalTime);
    m_cards.push(card);
    return row;
}

void NonlinearStepControlTable::store(std::uint32_t row, const NonlinearStepControlCard& card)
{
    m_id.set(row, card.id);
    m_totalTime.set(row, card.totalTime);
    m_cards.set(row, card);
}

void NonlinearStepControlTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_totalTime.keepRows(rows);
    m_cards.keepRows(rows);
}

void NonlinearStepControlTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void NonlinearStepControlTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The CRITTID of the ADAPT line names a TABSCTL: the user criteria that control the load step.
    std::int64_t* table = findCriteriaTable(m_cards.edit(row).sections);
    if (table != nullptr && *table > 0 && *table <= std::numeric_limits<base::EntityId>::max())
        *table =
            visitor.visit("criterionTableId", ReferenceTarget::LoadStepControl, static_cast<base::EntityId>(*table));
}

} // namespace oor::model
