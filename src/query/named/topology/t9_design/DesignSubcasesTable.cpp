#include "DesignQuerySupport.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t9_design/DesignQueries.hpp"

#include <algorithm>
#include <set>

namespace oor::query
{

namespace
{

// The effective DESOBJ of a section: its set and whether it maximises (DESOBJ(MAX)); DESOBJ and DESOBJ(MIN) minimise.
struct Objective
{
    std::int32_t response = 0; // 0: none
    bool maximise = false;
};

Objective objectiveOf(const model::CaseLineages& lineages, const model::Subcase& section)
{
    for (const char* variant : {"MinimumObjective", "MaximumObjective", ""})
    {
        const model::CaseEntry* entry = lineages.command(section.key, "DesignObjectiveSelection", variant);
        if (entry != nullptr && entry->value.kind == model::CaseValueKind::Integer)
            return Objective{static_cast<std::int32_t>(entry->value.integer),
                             std::string_view(variant) == "MaximumObjective"};
    }
    return Objective{};
}

std::int32_t integerOf(const model::CaseLineages& lineages, const model::Subcase& section, const char* command)
{
    const model::CaseEntry* entry = lineages.command(section.key, command);
    if (entry == nullptr || entry->value.kind != model::CaseValueKind::Integer)
        return 0;
    return static_cast<std::int32_t>(entry->value.integer);
}

std::string textOf(const model::CaseLineages& lineages, const model::Subcase& section, const char* command)
{
    const model::CaseEntry* entry = lineages.command(section.key, command);
    if (entry == nullptr ||
        (entry->value.kind != model::CaseValueKind::Text && entry->value.kind != model::CaseValueKind::Keyword))
        return {};
    return entry->value.text;
}

// designSubcasesTable: the design commands of the global section and of each subcase, effective.
class DesignSubcasesTableQuery final : public QueryBase
{
public:
    DesignSubcasesTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const model::CaseControl& caseControl = model.control().caseControl;
        const model::CaseLineages lineages(caseControl);
        std::vector<const model::Subcase*> sections{&caseControl.global()};
        for (const model::Subcase* subcase : analysis::analysisSubcases(caseControl))
        {
            if (subcase != &caseControl.global())
                sections.push_back(subcase);
        }
        const std::size_t count = sections.size();
        std::vector<std::int32_t> number(count), desobj(count), dessub(count), desglb(count);
        std::vector<std::string> analysisName(count), sense(count);
        std::vector<std::uint8_t> analysisValid(count), desobjValid(count), senseValid(count), dessubValid(count),
            desglbValid(count);
        std::vector<std::int64_t> offsets{0};
        std::vector<std::int32_t> sets;
        for (std::size_t index = 0; index < count; ++index)
        {
            const model::Subcase& section = *sections[index];
            number[index] = index == 0 ? 0 : static_cast<std::int32_t>(section.id);
            analysisName[index] = textOf(lineages, section, "AnalysisKind");
            analysisValid[index] = analysisName[index].empty() ? 0 : 1;
            const Objective objective = objectiveOf(lineages, section);
            desobj[index] = objective.response;
            desobjValid[index] = objective.response != 0 ? 1 : 0;
            sense[index] = objective.maximise ? "max" : "min";
            senseValid[index] = desobjValid[index];
            dessub[index] = integerOf(lineages, section, "SubcaseDesignConstrSelection");
            dessubValid[index] = dessub[index] != 0 ? 1 : 0;
            desglb[index] = integerOf(lineages, section, "GlobalDesignConstrSelection");
            desglbValid[index] = desglb[index] != 0 ? 1 : 0;
            std::set<base::EntityId> selected;
            for (const std::int32_t value : {dessub[index], desglb[index]})
            {
                if (value <= 0)
                    continue;
                for (const base::EntityId set : selectedConstraintSets(model, value))
                    selected.insert(set);
            }
            sets.insert(sets.end(), selected.begin(), selected.end());
            offsets.push_back(static_cast<std::int64_t>(sets.size()));
        }
        TableBuilder builder;
        builder.int32("subcase", std::move(number));
        builder.dictionary("analysis", analysisName, std::move(analysisValid));
        builder.int32("designObjectiveSelection", std::move(desobj), std::move(desobjValid));
        builder.dictionary("objectiveSense", sense, std::move(senseValid));
        builder.int32("subcaseDesignConstrSelection", std::move(dessub), std::move(dessubValid));
        builder.int32("globalDesignConstrSelection", std::move(desglb), std::move(desglbValid));
        builder.intList("constraintSets", std::move(offsets), std::move(sets));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "designSubcasesTable";
        info.family = "T9";
        info.description =
            "The design selections of the Case Control: one row for the global section (`subcase` 0), then one for "
            "each analysis subcase, with the value in force there (a subcase inherits from the global section): "
            "ANALYSIS, DESOBJ with its `objectiveSense` (`max` for DESOBJ(MAX), else `min`), DESSUB and DESGLB, and "
            "`constraintSets`: the DCONSTR sets that the DESSUB and DESGLB of the row select, a DCONADD replaced by "
            "the sets it adds, ascending.";
        info.defaultSort = "subcase 0, then the order of the Case Control";
        ColumnListBuilder columns;
        columns.int32("subcase").dictionary("analysis").int32("designObjectiveSelection").dictionary("objectiveSense");
        columns.int32("subcaseDesignConstrSelection").int32("globalDesignConstrSelection").intList("constraintSets");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addDesignSubcasesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DesignSubcasesTableQuery>());
}

} // namespace oor::query
