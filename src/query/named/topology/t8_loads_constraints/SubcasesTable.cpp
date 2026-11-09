#include "oor/analysis/components/SubcaseCombination.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CaseRequests.hpp"
#include "oor/query/named/topology/t8_loads_constraints/LoadConstraintQueries.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <map>
#include <utility>

namespace oor::query
{

namespace
{

// The neutral names of the commands the table reads more than once.
constexpr std::string_view TEMPERATURE = "TemperatureSelection";
constexpr std::string_view OBJECTIVE = "DesignObjectiveSelection";

// The command `name` that applies to a subcase among those whose variant `accepts`: the nearest section (the subcase,
// then the sections it inherits from) that has one gives its last one. Null when no section has one.
const model::CaseEntry* nearestCommand(const model::CaseLineages& lineages, const model::Subcase& subcase,
                                       std::string_view name, const std::function<bool(std::string_view)>& accepts)
{
    for (const model::Subcase* section : lineages.of(subcase.key))
    {
        for (auto entry = section->entries.rbegin(); entry != section->entries.rend(); ++entry)
        {
            if (entry->kind == model::EntryKind::Command && entry->name == name && accepts(entry->variant))
                return &*entry;
        }
    }
    return nullptr;
}

// The effective integer of a command (nullopt when absent or not an integer).
std::optional<std::int64_t> integerOf(const model::CaseLineages& lineages, const model::Subcase& subcase,
                                      std::string_view name, std::string_view variant = {})
{
    const model::CaseEntry* entry = lineages.command(subcase.key, name, variant);
    if (entry == nullptr || entry->value.kind != model::CaseValueKind::Integer)
        return std::nullopt;
    return entry->value.integer;
}

// The effective text of a command: a Text value as written, a Keyword in upper case.
std::optional<std::string> textOf(const model::CaseLineages& lineages, const model::Subcase& subcase,
                                  std::string_view name)
{
    const model::CaseEntry* entry = lineages.command(subcase.key, name);
    if (entry == nullptr ||
        (entry->value.kind != model::CaseValueKind::Text && entry->value.kind != model::CaseValueKind::Keyword))
        return std::nullopt;
    return entry->value.text;
}

// format reference TEMPERATURE: TEMPERATURE(LOAD) and TEMPERATURE(MATERIAL) select a set for one use each;
// TEMPERATURE(BOTH) and a bare TEMPERATURE, which the format reference reads as BOTH, select it for both. The nearest
// section that selects a set for the use gives it.
std::optional<std::int64_t> temperatureFor(const model::CaseLineages& lineages, const model::Subcase& subcase,
                                           std::string_view use)
{
    const model::CaseEntry* entry =
        nearestCommand(lineages, subcase, TEMPERATURE, [use](std::string_view variant)
                       { return variant == use || variant == "BothLoadAndInitial" || variant.empty(); });
    if (entry == nullptr || entry->value.kind != model::CaseValueKind::Integer)
        return std::nullopt;
    return entry->value.integer;
}

// The integer columns of the table: the column, the command and its variant (neutral names). The first ten come before
// the output requests; the others, added for the solver package, after the combinations.
struct SelectionColumn
{
    const char* column;
    const char* command;
    const char* variant;
};
constexpr std::size_t FIRST_SELECTIONS = 10;
constexpr std::array<SelectionColumn, 17> SELECTIONS{{
    {"spc", "SinglePointConstrSelection", ""},
    {"mpc", "MultiPointConstrSelection", ""},
    {"load", "StaticLoadSelection", ""},
    {"dynamicLoadSelection", "DynamicLoadSelection", ""},
    {"method", "EigenMethodSelection", ""},
    {"temperatureLoad", "TemperatureSelection", "TemperatureLoadPart"},
    {"temperatureInitial", "TemperatureSelection", "InitialTemperature"},
    {"nonlinearStaticParamsSelection", "NonlinearStaticParamsSelection", ""},
    {"freq", "FreqSetSelection", ""},
    {"timeStepSelection", "TimeStepSelection", ""},
    {"temperatureMaterial", "TemperatureSelection", "MaterialTemperature"},
    {"nonStructMass", "NonStructMassSelection", ""},
    {"staticSubcaseSelection", "StaticSubcaseSelection", ""},
    {"freeBodySupportSelection", "FreeBodySupportSelection", ""},
    {"designObjectiveSelection", "DesignObjectiveSelection", ""},
    {"subcaseDesignConstrSelection", "SubcaseDesignConstrSelection", ""},
    {"globalDesignConstrSelection", "GlobalDesignConstrSelection", ""},
}};

// format reference SUPER: the superelements a subcase is assigned to. `kind` is "superelement" (SUPER = i), "set"
// (SUPER = n, a Case Control SET of superelement numbers: its members) or "all" (SUPER = ALL); the load sequence number
// is 1 when SUPER = i or n does not give one, and none for ALL (every loading condition).
struct SuperAssignment
{
    std::string kind;
    std::vector<std::int32_t> ids;
    std::optional<std::int32_t> set;
    std::optional<std::int32_t> loadSequence;
};

std::optional<SuperAssignment> superAssignmentOf(const model::CaseLineages& lineages, const model::Subcase& subcase)
{
    const model::CaseEntry* entry = lineages.command(subcase.key, "SuperelementSelection");
    if (entry == nullptr)
        return std::nullopt;
    const model::CaseValue& value = entry->value;
    SuperAssignment assignment;
    if (value.kind == model::CaseValueKind::Keyword && value.text == "All")
    {
        assignment.kind = "all";
        return assignment;
    }
    if (value.kind != model::CaseValueKind::Integer)
        return std::nullopt;
    assignment.loadSequence = static_cast<std::int32_t>(value.integers.empty() ? 1 : value.integers.front());
    // the number of a SET differs from every superelement number
    if (const model::CaseEntry* set = lineages.set(subcase.key, value.integer); set != nullptr)
    {
        assignment.kind = "set";
        assignment.set = static_cast<std::int32_t>(value.integer);
        for (const std::int64_t member : set->set.integers)
            assignment.ids.push_back(static_cast<std::int32_t>(member));
        return assignment;
    }
    assignment.kind = "superelement";
    assignment.ids.push_back(static_cast<std::int32_t>(value.integer));
    return assignment;
}

// The texts of the table after the selections: the column and the command.
constexpr std::array<std::pair<const char*, const char*>, 2> TEXTS{
    {{"analysis", "AnalysisKind"}, {"designModelName", "DesignModelName"}}};

// subcasesTable: the subcases with their effective selections.
class SubcasesTableQuery final : public QueryBase
{
public:
    SubcasesTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::CaseControl& caseControl = run.model->control().caseControl;
        const model::CaseLineages lineages(caseControl); // once: every subcase asks for some 25 effective commands
        // The analysis subcases, then those of the auxiliary models (format reference AUXCASE), each AUXCASE section a
        // list of its own for the combinations of its subcases.
        std::vector<std::vector<const model::Subcase*>> sections{analysis::analysisSubcases(caseControl)};
        const model::Subcase* currentAuxcase = nullptr;
        for (const model::Subcase* subcase : analysis::auxiliarySubcases(caseControl))
        {
            const model::Subcase* auxcase = lineages.auxiliaryCase(subcase->key);
            if (auxcase != currentAuxcase)
                sections.emplace_back();
            currentAuxcase = auxcase;
            sections.back().push_back(subcase);
        }
        std::vector<const model::Subcase*> subcases;
        std::vector<std::int32_t> auxcaseNumber; // 0 for the primary Case Control, n for the n-th AUXCASE section
        std::vector<std::pair<std::size_t, std::size_t>> placeInSection;
        for (std::size_t section = 0; section < sections.size(); ++section)
        {
            for (std::size_t index = 0; index < sections[section].size(); ++index)
            {
                subcases.push_back(sections[section][index]);
                auxcaseNumber.push_back(static_cast<std::int32_t>(section));
                placeInSection.emplace_back(section, index);
            }
        }
        // The ordinal of every AUXCASE section in the deck (a section with no subcase still counts).
        std::map<const model::Subcase*, std::int32_t> auxcaseOrdinal;
        for (const model::Subcase& segment : caseControl.segments())
        {
            if (segment.kind == model::SubcaseKind::AuxiliaryCase)
                auxcaseOrdinal.emplace(&segment, static_cast<std::int32_t>(auxcaseOrdinal.size() + 1));
        }
        const std::size_t count = subcases.size();
        std::vector<std::int32_t> auxmodel(count, 0);
        std::vector<std::uint8_t> auxmodelValid(count, 0);
        std::vector<std::int32_t> bcontact(count, 0);
        std::vector<std::uint8_t> bcontactValid(count, 0);
        std::vector<std::int32_t> number(count);
        std::vector<std::string> kind(count);
        std::array<std::vector<std::string>, 3> words;
        std::array<std::vector<std::uint8_t>, 3> wordValid;
        for (std::size_t item = 0; item < 3; ++item)
        {
            words[item].resize(count);
            wordValid[item].assign(count, 0);
        }
        std::array<std::vector<std::int32_t>, SELECTIONS.size()> selected;
        std::array<std::vector<std::uint8_t>, SELECTIONS.size()> selectedValid;
        for (std::size_t item = 0; item < SELECTIONS.size(); ++item)
        {
            selected[item].assign(count, 0);
            selectedValid[item].assign(count, 0);
        }
        std::vector<std::string> sense(count);
        std::vector<std::uint8_t> senseValid(count, 0);
        std::array<std::vector<std::string>, TEXTS.size() + 1> texts; // analysis, desmod, dsaprt
        std::array<std::vector<std::uint8_t>, TEXTS.size() + 1> textValid;
        for (std::size_t item = 0; item < texts.size(); ++item)
        {
            texts[item].resize(count);
            textValid[item].assign(count, 0);
        }
        std::vector<std::int64_t> outputOffsets{0};
        std::vector<std::string> outputs;
        std::vector<std::int64_t> combinedOffsets{0};
        std::vector<std::int32_t> combined;
        std::vector<std::int64_t> coefficientOffsets{0};
        std::vector<double> coefficients;
        std::vector<std::string> superKind(count);
        std::vector<std::uint8_t> superKindValid(count, 0);
        std::vector<std::int64_t> superOffsets{0};
        std::vector<std::int32_t> superIds;
        std::vector<std::int32_t> superSet(count, 0);
        std::vector<std::uint8_t> superSetValid(count, 0);
        std::vector<std::int32_t> loadSequence(count, 0);
        std::vector<std::uint8_t> loadSequenceValid(count, 0);
        std::vector<std::int32_t> deform(count, 0);
        std::vector<std::uint8_t> deformValid(count, 0);
        for (std::size_t index = 0; index < count; ++index)
        {
            const model::Subcase& subcase = *subcases[index];
            number[index] = static_cast<std::int32_t>(subcase.id);
            kind[index] = std::string(model::subcaseKindName(subcase.kind));
            if (const model::Subcase* auxcase = lineages.auxiliaryCase(subcase.key))
            {
                kind[index] = "auxiliary";
                auxcaseNumber[index] = auxcaseOrdinal.at(auxcase);
                if (const auto auxiliaryModel = integerOf(lineages, subcase, "AuxiliaryModelSelection"))
                {
                    auxmodel[index] = static_cast<std::int32_t>(*auxiliaryModel);
                    auxmodelValid[index] = 1;
                }
            }
            const char* const wordNames[3] = {"TitleText", "SubtitleText", "LabelText"};
            for (std::size_t item = 0; item < 3; ++item)
            {
                if (const auto text = textOf(lineages, subcase, wordNames[item]))
                {
                    words[item][index] = *text;
                    wordValid[item][index] = 1;
                }
            }
            for (std::size_t item = 0; item < SELECTIONS.size(); ++item)
            {
                const SelectionColumn& selection = SELECTIONS[item];
                const std::string_view command = selection.command;
                const std::string_view variant = selection.variant;
                std::optional<std::int64_t> value;
                if (command == TEMPERATURE && variant != "InitialTemperature")
                    value = temperatureFor(lineages, subcase, variant);
                else if (command == OBJECTIVE)
                {
                    // DESOBJ(MAX) = n maximises, DESOBJ = n and DESOBJ(MIN) = n minimise (format reference DESOBJ).
                    if (const model::CaseEntry* entry =
                            nearestCommand(lineages, subcase, OBJECTIVE, [](std::string_view) { return true; });
                        entry != nullptr && entry->value.kind == model::CaseValueKind::Integer)
                    {
                        value = entry->value.integer;
                        sense[index] = entry->variant == "MaximumObjective" ? "max" : "min";
                        senseValid[index] = 1;
                    }
                }
                else
                    value = integerOf(lineages, subcase, command, variant);
                if (value)
                {
                    selected[item][index] = static_cast<std::int32_t>(*value);
                    selectedValid[item][index] = 1;
                }
            }
            // format reference BCONTACT: the contact table of the subcase; NONE (no contact) and ALLBODY name no table
            if (const auto table = integerOf(lineages, subcase, "ContactSelection"))
            {
                bcontact[index] = static_cast<std::int32_t>(*table);
                bcontactValid[index] = 1;
            }
            // format reference DEFORM (Case): the element deformation set; a SUBCOM gives its own or inherits the
            // global one (the deformations of the subcases it combines are not taken
            // from them)
            if (const auto set = integerOf(lineages, subcase, "DeformationLoadSelection"))
            {
                deform[index] = static_cast<std::int32_t>(*set);
                deformValid[index] = 1;
            }
            for (std::size_t item = 0; item < TEXTS.size(); ++item)
            {
                if (const auto text = textOf(lineages, subcase, TEXTS[item].second))
                {
                    texts[item][index] = *text;
                    textValid[item][index] = 1;
                }
            }
            for (const OutputRequest& request : effectiveRequests(lineages.of(subcase.key)))
            {
                if (request.entry->name == "DesignSensitivityOutput")
                {
                    texts[TEXTS.size()][index] = requestText(*request.entry);
                    textValid[TEXTS.size()][index] = 1;
                }
                outputs.push_back(requestText(*request.entry));
            }
            outputOffsets.push_back(static_cast<std::int64_t>(outputs.size()));
            const std::vector<const model::Subcase*>& section = sections[placeInSection[index].first];
            for (const analysis::CombinationTerm& term :
                 analysis::SubcaseCombination::termsOf(section, placeInSection[index].second))
            {
                combined.push_back(static_cast<std::int32_t>(section[term.subcaseIndex]->id));
                coefficients.push_back(term.coefficient);
            }
            combinedOffsets.push_back(static_cast<std::int64_t>(combined.size()));
            coefficientOffsets.push_back(static_cast<std::int64_t>(coefficients.size()));
            if (const std::optional<SuperAssignment> assignment = superAssignmentOf(lineages, subcase))
            {
                superKind[index] = assignment->kind;
                superKindValid[index] = 1;
                superIds.insert(superIds.end(), assignment->ids.begin(), assignment->ids.end());
                if (assignment->set)
                {
                    superSet[index] = *assignment->set;
                    superSetValid[index] = 1;
                }
                if (assignment->loadSequence)
                {
                    loadSequence[index] = *assignment->loadSequence;
                    loadSequenceValid[index] = 1;
                }
            }
            superOffsets.push_back(static_cast<std::int64_t>(superIds.size()));
        }
        TableBuilder builder;
        builder.int32("subcase", std::move(number));
        builder.dictionary("kind", kind);
        builder.text("title", words[0], std::move(wordValid[0]));
        builder.text("subtitle", words[1], std::move(wordValid[1]));
        builder.text("label", words[2], std::move(wordValid[2]));
        for (std::size_t item = 0; item < FIRST_SELECTIONS; ++item)
            builder.int32(SELECTIONS[item].column, std::move(selected[item]), std::move(selectedValid[item]));
        builder.textList("outputs", std::move(outputOffsets), outputs);
        builder.intList("combinedSubcases", std::move(combinedOffsets), std::move(combined));
        builder.realList("coefficients", std::move(coefficientOffsets), std::move(coefficients));
        for (std::size_t item = FIRST_SELECTIONS; item < SELECTIONS.size(); ++item)
        {
            builder.int32(SELECTIONS[item].column, std::move(selected[item]), std::move(selectedValid[item]));
            if (std::string_view(SELECTIONS[item].column) == "designObjectiveSelection")
                builder.dictionary("objectiveSense", sense, std::move(senseValid));
        }
        builder.dictionary(TEXTS[0].first, texts[0], std::move(textValid[0]));
        builder.text("designSensitivityOutput", texts[TEXTS.size()], std::move(textValid[TEXTS.size()]));
        builder.text(TEXTS[1].first, texts[1], std::move(textValid[1]));
        builder.int32("auxiliaryCase", std::move(auxcaseNumber));
        builder.int32("auxiliaryModelSelection", std::move(auxmodel), std::move(auxmodelValid));
        builder.int32("contactSelection", std::move(bcontact), std::move(bcontactValid));
        builder.dictionary("superKind", superKind, std::move(superKindValid));
        builder.intList("superIds", std::move(superOffsets), std::move(superIds));
        builder.int32("superSet", std::move(superSet), std::move(superSetValid));
        builder.int32("loadSequence", std::move(loadSequence), std::move(loadSequenceValid));
        builder.int32("deform", std::move(deform), std::move(deformValid));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "subcasesTable";
        info.family = "T8";
        info.description =
            "The analysis subcases of the Case Control (SUBCASE, SUBCOM, SYM, SYMCOM, REPCASE; the global section when "
            "there is no subcase) with the effective value of their commands, inherited from the global section: the "
            "title, subtitle and label, the sets selected (SPC, MPC, LOAD, DLOAD, METHOD, TEMPERATURE(LOAD), "
            "TEMPERATURE(INITIAL), TEMPERATURE(MATERIAL), NLPARM, FREQUENCY, TSTEP, NSM, STATSUB, SUPORT1; "
            "TEMPERATURE(BOTH) and a bare TEMPERATURE, which the format reference reads as both, fill temperatureLoad "
            "and "
            "temperatureMaterial), the design commands (DESOBJ with objectiveSense min or max, DESSUB, DESGLB, "
            "ANALYSIS, DSAPRT as the request text, DESMOD), the output requests as text (DISPLACEMENT(SORT1,REAL) = "
            "ALL), and for a SUBCOM or SYMCOM the subcases it combines and their factors as the solver reads them: "
            "SUBSEQ "
            "aligned on the last subcases before the SUBCOM (a SUBCOM between them "
            "is not "
            "counted, a short list gives the earlier ones 0), SUBSEQ1 its named subcases times S0, SYMSEQ the SYM "
            "subcases (1.0 each without SYMSEQ). The subcases of the auxiliary models follow (format reference "
            "AUXCASE, SOL 200): "
            "kind is the kind of the section (global, subcase, subcaseCombination, symmetrySubcase, "
            "symmetryCombination, repeatSubcase), `auxiliary` for those, auxcase the ordinal of their AUXCASE "
            "section (0 for the primary Case Control; an AUXCASE "
            "section with no SUBCASE is one subcase 0), auxmodel the AUXMODEL that applies (the subcase's, else its "
            "AUXCASE section's); their values are inherited from their AUXCASE section, never from the primary "
            "sections. bcontact is the contact table the subcase selects (BCONTACT = n, effective; null for NONE, "
            "ALLBODY or no BCONTACT), for information: the components use every contact table of the Bulk Data "
            "(contactTablesTable lists the tables and the subcases that select them). The SUPER command (format "
            "reference SUPER, "
            "effective): superKind superelement (SUPER = i), set (SUPER = n, a Case Control SET of superelement "
            "numbers) or all (SUPER = ALL), null without SUPER (which the solver reads as ALL when no subcase has "
            "one); "
            "superIds the superelements (i, or the members of the SET; empty for ALL); superSet the SET number; "
            "loadSequence the load sequence number l of SUPER = i, l (1 when not given, null for ALL). deform is the "
            "element deformation set (format reference DEFORM, effective; null for none); a SUBCOM or SYMCOM has its "
            "own or the "
            "global section's, never that of the subcases it combines.";
        info.defaultSort = "(the order of the Case Control)";
        ColumnListBuilder columns;
        columns.int32("subcase").dictionary("kind").text("title").text("subtitle").text("label");
        columns.int32("spc")
            .int32("mpc")
            .int32("load")
            .int32("dynamicLoadSelection")
            .int32("method")
            .int32("temperatureLoad");
        columns.int32("temperatureInitial")
            .int32("nonlinearStaticParamsSelection")
            .int32("freq")
            .int32("timeStepSelection");
        columns.textList("outputs").intList("combinedSubcases").realList("coefficients");
        columns.int32("temperatureMaterial")
            .int32("nonStructMass")
            .int32("staticSubcaseSelection")
            .int32("freeBodySupportSelection")
            .int32("designObjectiveSelection");
        columns.dictionary("objectiveSense")
            .int32("subcaseDesignConstrSelection")
            .int32("globalDesignConstrSelection")
            .dictionary("analysis")
            .text("designSensitivityOutput");
        columns.text("designModelName")
            .int32("auxiliaryCase")
            .int32("auxiliaryModelSelection")
            .int32("contactSelection");
        columns.dictionary("superKind").intList("superIds").int32("superSet").int32("loadSequence");
        columns.int32("deform");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addSubcasesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<SubcasesTableQuery>());
}

} // namespace oor::query
