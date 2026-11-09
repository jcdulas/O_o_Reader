#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t8_loads_constraints/LoadConstraintQueries.hpp"

#include <algorithm>
#include <limits>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// The name of an output block in the `section` column, from the kind of the output block line (Plot, Postprocessing,
// CurvePlot or CurveOutput).
std::string blockName(const std::string& kind)
{
    if (kind == "Postprocessing")
        return "post";
    if (kind == "CurvePlot" || kind == "CurveOutput")
        return "xyplot";
    return "plot";
}

// The rows of caseSetsTable while it is built.
struct SetRows
{
    std::vector<std::int32_t> subcase;
    std::vector<std::string> section;
    std::vector<std::int32_t> setId;
    std::vector<std::int32_t> position;
    std::vector<std::int32_t> id;
    std::vector<std::uint8_t> idValid;
    std::vector<double> value;
    std::vector<std::string> item;
    std::vector<std::uint8_t> itemValid;

    void add(std::int64_t number, const std::string& block, std::int64_t set, std::size_t at,
             std::optional<std::int64_t> member, double real, const std::string* word)
    {
        subcase.push_back(static_cast<std::int32_t>(number));
        section.push_back(block);
        setId.push_back(static_cast<std::int32_t>(set));
        position.push_back(static_cast<std::int32_t>(at));
        id.push_back(member ? static_cast<std::int32_t>(*member) : 0);
        idValid.push_back(member ? 1 : 0);
        value.push_back(real);
        item.push_back(word != nullptr ? *word : std::string());
        itemValid.push_back(word != nullptr ? 1 : 0);
    }
};

// caseSetsTable: the members of every SET of the Case Control, THRU and EXCEPT expanded.
class CaseSetsTableQuery final : public QueryBase
{
public:
    CaseSetsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::CaseControl& caseControl = run.model->control().caseControl;
        const auto onlySubcases = run.arguments.ids("subcases");
        const auto onlySets = run.arguments.ids("setIds");
        SetRows rows;
        for (const model::Subcase& segment : caseControl.segments())
        {
            if (onlySubcases &&
                std::find(onlySubcases->begin(), onlySubcases->end(), segment.id) == onlySubcases->end())
                continue;
            addSection(segment, segment.id, onlySets, rows);
        }
        TableBuilder builder;
        builder.int32("subcase", std::move(rows.subcase));
        builder.int32("setId", std::move(rows.setId));
        builder.int32("position", std::move(rows.position));
        builder.int32("id", std::move(rows.id), std::move(rows.idValid));
        builder.real("value", std::move(rows.value));
        builder.dictionary("section", rows.section);
        builder.text("item", rows.item, std::move(rows.itemValid));
        return builder.finish();
    }

private:
    // The sets of one section and of its steps, under the number of the subcase they belong to. A SET that follows an
    // OUTPUT(PLOT), OUTPUT(POST) or OUTPUT(XYPLOT) line belongs to that block (CaseControl::blockExtent).
    static void addSection(const model::Subcase& section, std::int64_t number,
                           const std::optional<std::vector<std::int64_t>>& onlySets, SetRows& rows)
    {
        std::string block = "case";
        std::size_t blockEnd = 0;
        for (std::size_t index = 0; index < section.entries.size(); ++index)
        {
            const model::CaseEntry& entry = section.entries[index];
            if (index >= blockEnd)
                block = "case";
            if (entry.kind == model::EntryKind::OutputBlock)
            {
                block = blockName(entry.variant);
                blockEnd = model::CaseControl::blockExtent(section, index).last;
                continue;
            }
            if (entry.kind != model::EntryKind::Set)
                continue;
            if (onlySets && std::find(onlySets->begin(), onlySets->end(), entry.number) == onlySets->end())
                continue;
            addMembers(entry, number, block, rows);
        }
        for (const model::Subcase& child : section.children)
            addSection(child, number, onlySets, rows);
    }

    static void addMembers(const model::CaseEntry& entry, std::int64_t number, const std::string& block, SetRows& rows)
    {
        const model::CaseSet& members = entry.set;
        switch (members.kind)
        {
        case model::CaseSetKind::Integers:
            for (std::size_t at = 0; at < members.integers.size(); ++at)
                rows.add(number, block, entry.number, at + 1, members.integers[at], NOT_A_NUMBER, nullptr);
            break;
        case model::CaseSetKind::Reals:
            for (std::size_t at = 0; at < members.reals.size(); ++at)
                rows.add(number, block, entry.number, at + 1, std::nullopt, members.reals[at], nullptr);
            break;
        case model::CaseSetKind::All:
        {
            const std::string all = "All";
            rows.add(number, block, entry.number, 1, std::nullopt, NOT_A_NUMBER, &all);
            break;
        }
        case model::CaseSetKind::Words:
            for (std::size_t at = 0; at < members.words.size(); ++at)
                rows.add(number, block, entry.number, at + 1, std::nullopt, NOT_A_NUMBER, &members.words[at]);
            break;
        }
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "caseSetsTable";
        info.family = "T8";
        info.description =
            "The members of every SET of the Case Control, one row each in the order of the set: subcase is the "
            "number of the subcase the SET is written in (0: the global section; a SET of a step goes with its "
            "subcase), position counts from 1, id is a member of a set of IDs (THRU, BY and EXCEPT expanded as the "
            "format reference says: the IDs after EXCEPT are removed while they stay in the THRU range before it) and "
            "value a "
            "member of a set of reals (frequencies, times). section says where the SET is: `case` for the sets the "
            "output requests and the other commands name, `plot`, `post` or `xyplot` for a SET of an OUTPUT(PLOT), "
            "OUTPUT(POST) or OUTPUT(XYPLOT) block. A SET whose members are not numbers (SET 5 = ALL, the element "
            "keywords of a plot set such as ALL EXCEPT PLOTEL, ID/component pairs) gives one row per item as "
            "written in `item`, with id and value null.";
        info.parameters = {parameter("subcases", ParameterKind::IdList, "None", "keep the sets of these subcases"),
                           parameter("setIds", ParameterKind::IdList, "None", "keep these sets")};
        info.defaultSort = "(the order of the Case Control)";
        ColumnListBuilder columns;
        columns.int32("subcase").int32("setId").int32("position").int32("id").real("value");
        columns.dictionary("section").text("item");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addCaseSetsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<CaseSetsTableQuery>());
}

} // namespace oor::query
