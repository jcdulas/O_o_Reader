#include "AeroQuerySupport.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/aero/aeroControlSchedule/AeroControlScheduleTable.hpp"
#include "oor/model/cards/aero/aeroControlSurface/AeroControlSurfaceTable.hpp"
#include "oor/model/cards/aero/aeroController/AeroControllerTable.hpp"
#include "oor/model/cards/aero/aeroRigidTrimVariable/AeroRigidTrimVariableTable.hpp"
#include "oor/model/cards/aero/aeroTrim/AeroTrimTable.hpp"
#include "oor/model/cards/aero/aeroTrim2/AeroTrim2Table.hpp"
#include "oor/model/cards/aero/aeroTrimLink/AeroTrimLinkTable.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t10_aero/AeroQueries.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <unordered_map>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// The labels of the rigid body motions the program knows: any other AESTAT label is
// a variable whose forces the user supplies.
constexpr std::array<std::string_view, 11> RIGID_BODY_LABELS{
    "AngleOfAttack", "SideSlip",    "RollRate",           "PitchRate",          "YawRate",           "RigidAccelX",
    "RigidAccelY",   "RigidAccelZ", "RigidAngularAccelX", "RigidAngularAccelY", "RigidAngularAccelZ"};

enum class State : std::uint8_t
{
    Fixed,
    Free,
    Linked,
    Scheduled
};

std::string_view stateName(State state)
{
    constexpr std::array<std::string_view, 4> NAMES{"fixed", "free", "linked", "scheduled"};
    return NAMES[static_cast<std::size_t>(state)];
}

// A trim variable of the model: the AESTAT, AESURF or AEPARM card that defines its label.
struct Variable
{
    std::string label;     // as written on the card
    std::string_view kind; // rigidBody, controlSurface or user
};

// What a TRIM or TRIM2 card says of one variable.
struct NamedVariable
{
    std::string label; // as written on the card
    State state = State::Fixed;
    double value = 0.0;
};

struct TrimCase
{
    base::EntityId sid = 0;
    double mach = 0.0;
    double dynamicPressure = 0.0;
    double rigidFlag = 1.0;
    bool isTrim2 = false; // a variable no pair names is fixed to zero
    std::vector<NamedVariable> named;
};

// The label as the cards compare it: without regard to the case.
std::string keyOf(const std::string& label)
{
    return std::string(label);
}

// The variables of the model: the AESTAT cards, then the AESURF cards, then the AEPARM cards, each by ID. A label that
// two cards define belongs to the first (the check `aero.label-repeated` reports the second).
std::vector<Variable> readVariables(const model::Model& model)
{
    std::vector<Variable> variables;
    std::set<std::string> seen;
    const auto add = [&](const std::string& label, std::string_view kind)
    {
        if (seen.insert(keyOf(label)).second)
            variables.push_back(Variable{label, kind});
    };
    std::vector<model::AeroRigidTrimVariableCard> statics = aero::cardsOf<model::AeroRigidTrimVariableTable>(model);
    std::ranges::stable_sort(statics, {}, &model::AeroRigidTrimVariableCard::id);
    for (const model::AeroRigidTrimVariableCard& card : statics)
    {
        const bool rigid =
            std::ranges::any_of(RIGID_BODY_LABELS, [&card](std::string_view name) { return ((card.label) == (name)); });
        add(card.label, rigid ? "rigidBody" : "user");
    }
    std::vector<model::AeroControlSurfaceCard> surfaces = aero::cardsOf<model::AeroControlSurfaceTable>(model);
    std::ranges::stable_sort(surfaces, {}, &model::AeroControlSurfaceCard::id);
    for (const model::AeroControlSurfaceCard& card : surfaces)
        add(card.label, "controlSurface");
    std::vector<model::AeroControllerCard> parameters = aero::cardsOf<model::AeroControllerTable>(model);
    std::ranges::stable_sort(parameters, {}, &model::AeroControllerCard::id);
    for (const model::AeroControllerCard& card : parameters)
        add(card.label, "user");
    return variables;
}

State stateOfWord(model::TrimState word)
{
    switch (word)
    {
    case model::TrimState::Free:
        return State::Free;
    case model::TrimState::Linked:
        return State::Linked;
    case model::TrimState::Scheduled:
        return State::Scheduled;
    }
    return State::Free;
}

// The TRIM and TRIM2 cards by SID (a TRIM before a TRIM2 of the same SID, which a check reports).
std::vector<TrimCase> readCases(const model::Model& model)
{
    std::vector<TrimCase> cases;
    for (const model::AeroTrimCard& card : aero::cardsOf<model::AeroTrimTable>(model))
    {
        TrimCase trim{card.id, card.machNumber, card.dynamicPressure, card.equivalentAspectRatio, false, {}};
        for (const model::AeroTrimVariable& variable : card.variables)
            trim.named.push_back(NamedVariable{variable.label, State::Fixed, variable.value});
        cases.push_back(std::move(trim));
    }
    for (const model::AeroTrim2Card& card : aero::cardsOf<model::AeroTrim2Table>(model))
    {
        TrimCase trim{card.id, card.machNumber, card.dynamicPressure, card.equivalentAspectRatio, true, {}};
        for (const model::AeroTrim2Variable& variable : card.variables)
        {
            if (const double* number = std::get_if<double>(&variable.value))
                trim.named.push_back(NamedVariable{variable.label, State::Fixed, *number});
            else
                trim.named.push_back(
                    NamedVariable{variable.label, stateOfWord(std::get<model::TrimState>(variable.value)), 0.0});
        }
        cases.push_back(std::move(trim));
    }
    std::ranges::stable_sort(cases, {}, &TrimCase::sid);
    return cases;
}

// What links or schedules the variables of each trim set. An AELINK with the ID of the set, or with ID 0 (every
// subcase), makes its LABLD depend on other variables. A CSSCHD schedules the AESURF of its AESID: it applies to a trim
// set when a subcase that selects the set also selects the CSSCHD (Case Control CSSCHD).
struct Constraints
{
    std::vector<std::pair<base::EntityId, std::string>> links;   // (AELINK ID, key of LABLD)
    std::map<base::EntityId, std::set<std::string>> scheduledBy; // trim SID -> keys of the scheduled surfaces

    [[nodiscard]] bool linked(base::EntityId sid, const std::string& key) const
    {
        return std::ranges::any_of(
            links, [&](const auto& link) { return link.second == key && (link.first == 0 || link.first == sid); });
    }
    [[nodiscard]] bool scheduled(base::EntityId sid, const std::string& key) const
    {
        const auto found = scheduledBy.find(sid);
        return found != scheduledBy.end() && found->second.contains(key);
    }
};

Constraints readConstraints(const model::Model& model)
{
    Constraints constraints;
    for (const model::AeroTrimLinkCard& card : aero::cardsOf<model::AeroTrimLinkTable>(model))
        constraints.links.emplace_back(card.id, keyOf(card.dependentLabel));
    std::unordered_map<base::EntityId, std::string> surfaceLabels;
    for (const model::AeroControlSurfaceCard& card : aero::cardsOf<model::AeroControlSurfaceTable>(model))
        surfaceLabels.try_emplace(card.id, keyOf(card.label));
    std::unordered_map<base::EntityId, std::vector<base::EntityId>> surfacesOfSchedule;
    for (const model::AeroControlScheduleCard& card : aero::cardsOf<model::AeroControlScheduleTable>(model))
        surfacesOfSchedule[card.id].push_back(card.controlSurfaceId);
    const model::CaseControl& caseControl = model.control().caseControl;
    const model::CaseLineages lineages(caseControl);
    for (const model::Subcase* subcase : analysis::analysisSubcases(caseControl))
    {
        const std::optional<std::int64_t> trim = analysis::effectiveInteger(lineages, *subcase, "TrimSelection");
        const std::optional<std::int64_t> schedule =
            analysis::effectiveInteger(lineages, *subcase, "ControlScheduleSelection");
        if (!trim || !schedule)
            continue;
        const std::optional<base::EntityId> trimSid = base::toEntityId(*trim);
        const std::optional<base::EntityId> scheduleSid = base::toEntityId(*schedule);
        if (!trimSid || !scheduleSid)
            continue;
        const auto surfaces = surfacesOfSchedule.find(*scheduleSid);
        if (surfaces == surfacesOfSchedule.end())
            continue;
        for (const base::EntityId surface : surfaces->second)
        {
            if (const auto label = surfaceLabels.find(surface); label != surfaceLabels.end())
                constraints.scheduledBy[*trimSid].insert(label->second);
        }
    }
    return constraints;
}

// aeroTrimCasesTable: for every TRIM and TRIM2 card, one row for each trim variable of the model, with what the case
// does with it.
class TrimCasesTableQuery final : public QueryBase
{
public:
    TrimCasesTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const std::optional<std::vector<std::int64_t>> wanted = sortedIds(run.arguments.ids("setIds"));
        std::vector<TrimCase> cases = readCases(model);
        if (wanted)
        {
            std::erase_if(cases, [&](const TrimCase& trim)
                          { return !std::binary_search(wanted->begin(), wanted->end(), std::int64_t{trim.sid}); });
        }
        if (cases.empty())
            return emptyTable(info().defaultColumns());

        const std::vector<Variable> variables = readVariables(model);
        const Constraints constraints = readConstraints(model);
        const std::map<base::EntityId, std::vector<std::int32_t>> subcasesOf =
            aero::subcasesSelecting(model, "TrimSelection");

        std::vector<std::int32_t> sid;
        std::vector<std::int64_t> subcaseOffsets{0};
        std::vector<std::int32_t> subcases;
        std::vector<double> mach, dynamicPressure, rigidFlag, value;
        std::vector<std::string> labels, state, kind;
        std::vector<std::uint8_t> fixed, kindValid;
        const auto addRow = [&](const TrimCase& trim, const std::string& label, std::string_view variableKind,
                                State rowState, double rowValue)
        {
            sid.push_back(trim.sid);
            if (const auto found = subcasesOf.find(trim.sid); found != subcasesOf.end())
                subcases.insert(subcases.end(), found->second.begin(), found->second.end());
            subcaseOffsets.push_back(static_cast<std::int64_t>(subcases.size()));
            mach.push_back(trim.mach);
            dynamicPressure.push_back(trim.dynamicPressure);
            rigidFlag.push_back(trim.rigidFlag);
            labels.push_back(label);
            value.push_back(rowState == State::Fixed ? rowValue : NOT_A_NUMBER);
            fixed.push_back(rowState == State::Fixed ? 1 : 0);
            state.emplace_back(stateName(rowState));
            kind.emplace_back(variableKind);
            kindValid.push_back(variableKind.empty() ? 0 : 1);
        };
        for (const TrimCase& trim : cases)
        {
            std::map<std::string, const NamedVariable*> named;
            for (const NamedVariable& entry : trim.named)
                named.try_emplace(keyOf(entry.label), &entry);
            std::set<std::string> defined;
            for (const Variable& variable : variables)
            {
                const std::string key = keyOf(variable.label);
                defined.insert(key);
                if (const auto found = named.find(key); found != named.end())
                    addRow(trim, variable.label, variable.kind, found->second->state, found->second->value);
                else if (constraints.linked(trim.sid, key))
                    addRow(trim, variable.label, variable.kind, State::Linked, 0.0);
                else if (constraints.scheduled(trim.sid, key))
                    addRow(trim, variable.label, variable.kind, State::Scheduled, 0.0);
                else if (trim.isTrim2)
                    addRow(trim, variable.label, variable.kind, State::Fixed, 0.0);
                else
                    addRow(trim, variable.label, variable.kind, State::Free, 0.0);
            }
            // a label the card names that no AESTAT, AESURF or AEPARM defines (`aero.label-undefined`)
            std::set<std::string> listed;
            for (const NamedVariable& entry : trim.named)
            {
                const std::string key = keyOf(entry.label);
                if (!defined.contains(key) && listed.insert(key).second)
                    addRow(trim, entry.label, std::string_view(), named.at(key)->state, named.at(key)->value);
            }
        }
        TableBuilder builder;
        builder.int32("setId", std::move(sid));
        builder.intList("subcases", std::move(subcaseOffsets), std::move(subcases));
        builder.real("mach", std::move(mach));
        builder.real("dynamicPressure", std::move(dynamicPressure));
        builder.real("equivalentAspectRatio", std::move(rigidFlag));
        builder.text("variable", labels);
        builder.real("value", std::move(value));
        builder.flag("isFixed", fixed);
        builder.dictionary("state", state);
        builder.dictionary("variableKind", kind, std::move(kindValid));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "aeroTrimCasesTable";
        info.family = "T10";
        info.description =
            "The trim cases (TRIM and TRIM2 cards), one row for each variable of the model (the AESTAT, AESURF and "
            "AEPARM cards, in this order and by ID) and for each label the card names that no card defines (last; "
            "`variableKind` is null). `subcases` are the subcases whose Case Control TRIM selects the set. A variable "
            "a TRIM fixes is `fixed` with its `value`; one it does not name is solved for (`free`) unless an AELINK of "
            "the set (or of ID 0) makes it depend on others (`linked`) or a CSSCHD of a subcase of the case schedules "
            "it (`scheduled`); on a TRIM2 a variable no pair, AELINK or CSSCHD names is fixed to zero "
            "and the words FREE, LINKED and SCHED give the state. `value` is null unless the variable is fixed. "
            "`variableKind` is `rigidBody` for the AESTAT labels the program knows (AngleOfAttack, SideSlip, "
            "RollRate, PitchRate, YawRate, RigidAccelX to RigidAngularAccelZ), `controlSurface` for an AESURF and "
            "`user` for an AEPARM and for any other AESTAT label.";
        info.parameters = {parameter("setIds", ParameterKind::IdList, "None", "the trim sets to list")};
        info.defaultSort = "setId (a TRIM before a TRIM2 of the same SID), then the variables as above";
        ColumnListBuilder columns;
        columns.int32("setId")
            .intList("subcases")
            .real("mach")
            .real("dynamicPressure")
            .real("equivalentAspectRatio")
            .text("variable");
        columns.real("value").flag("isFixed").dictionary("state").dictionary("variableKind");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addTrimCasesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<TrimCasesTableQuery>());
}

} // namespace oor::query
