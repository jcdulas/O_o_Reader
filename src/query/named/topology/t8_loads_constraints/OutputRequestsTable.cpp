#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CaseRequests.hpp"
#include "oor/query/named/topology/t8_loads_constraints/LoadConstraintQueries.hpp"

#include <algorithm>

namespace oor::query
{

namespace
{

// outputRequestsTable: one row for each output request of each subcase, effective.
class OutputRequestsTableQuery final : public QueryBase
{
public:
    OutputRequestsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::CaseControl& caseControl = run.model->control().caseControl;
        const model::CaseLineages lineages(caseControl);
        const auto onlySubcases = run.arguments.ids("subcases");
        // The requests asked for by their neutral names, or by their keywords in the dictionary of the model.
        std::vector<std::string> onlyRequests;
        for (const std::string& name : run.arguments.texts("requests"))
            onlyRequests.push_back(std::string(name));
        std::vector<std::int32_t> subcase;
        std::vector<std::string> request;
        std::vector<std::int64_t> describerOffsets{0};
        std::vector<std::string> describers;
        std::vector<std::string> set;
        std::vector<std::uint8_t> setValid;
        std::vector<std::int32_t> setId;
        std::vector<std::uint8_t> setIdValid;
        std::vector<std::uint8_t> inherited;
        for (const model::Subcase* section : analysis::analysisSubcases(caseControl))
        {
            if (onlySubcases &&
                std::find(onlySubcases->begin(), onlySubcases->end(), section->id) == onlySubcases->end())
                continue;
            for (const OutputRequest& found : effectiveRequests(lineages.of(section->key)))
            {
                const model::CaseEntry& entry = *found.entry;
                if (!onlyRequests.empty() &&
                    std::find(onlyRequests.begin(), onlyRequests.end(), entry.name) == onlyRequests.end())
                    continue;
                subcase.push_back(static_cast<std::int32_t>(section->id));
                request.push_back(entry.name);
                for (std::string& describer : fullDescribers(entry))
                    describers.push_back(std::move(describer));
                describerOffsets.push_back(static_cast<std::int64_t>(describers.size()));
                // The selection: a word (ALL, NONE; YES or NO for a MEFFMASS) or the ID of a SET; none for a request
                // written without "= ...".
                const bool isWord =
                    entry.value.kind == model::CaseValueKind::Keyword || entry.value.kind == model::CaseValueKind::Text;
                const bool isSet = entry.value.kind == model::CaseValueKind::Integer;
                set.push_back(isWord ? entry.value.text : std::string());
                setValid.push_back(isWord ? 1 : 0);
                setId.push_back(isSet ? static_cast<std::int32_t>(entry.value.integer) : 0);
                setIdValid.push_back(isSet ? 1 : 0);
                inherited.push_back(found.isInherited ? 1 : 0);
            }
        }
        TableBuilder builder;
        builder.int32("subcase", std::move(subcase));
        builder.dictionary("request", request);
        builder.textList("describers", std::move(describerOffsets), describers);
        builder.dictionary("set", set, std::move(setValid));
        builder.int32("setId", std::move(setId), std::move(setIdValid));
        builder.flag("isInherited", inherited);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "outputRequestsTable";
        info.family = "T8";
        info.description =
            "One row for each effective output request of each analysis subcase, or the global section when there "
            "is no subcase. A subcase uses its own requests of a name when present and otherwise inherits the global "
            "requests (isInherited). request is the exact neutral command name, such as DisplacementOutput or "
            "StressOutput; describers lists the exact neutral options in their original order, including named "
            "values such as ReferenceNode=12. set is the selection word (All, None, Yes or No); setId is the case "
            "set identifier (caseSetsTable gives its members). Both are null for a request without a selection.";
        info.parameters = {
            parameter("subcases", ParameterKind::IdList, "None", "keep these subcases"),
            parameter("requests", ParameterKind::TextList, "None", "keep these requests by exact neutral name")};
        info.defaultSort = "(the order of the Case Control)";
        ColumnListBuilder columns;
        columns.int32("subcase").dictionary("request").textList("describers").dictionary("set").int32("setId");
        columns.flag("isInherited");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addOutputRequestsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<OutputRequestsTableQuery>());
}

} // namespace oor::query
