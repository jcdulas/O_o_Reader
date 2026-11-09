#include "oor/model/cards/optimisation/designResp1/DesignResp1Table.hpp"
#include "oor/model/cards/optimisation/designResp2/DesignResp2Table.hpp"
#include "oor/model/cards/optimisation/designResp3/DesignResp3Table.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t9_design/DesignQueries.hpp"

#include <algorithm>
#include <limits>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

struct ArgumentRow
{
    std::int32_t response = 0;
    std::string kind;
    std::int32_t position = 0;
    std::int32_t id = 0;
    bool hasId = false;
    std::int32_t component = 0; // 0: null
    std::string label;          // empty: null
    double value = NOT_A_NUMBER;
};

template <typename TableT, typename Body>
void forEachCard(const model::Model& model, Body&& body)
{
    const TableT* table = model.findTable<TableT>();
    if (table == nullptr)
        return;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (table->rows().isAlive(row))
            body(table->card(row));
    }
}

// The ATTi of a DRESP1, as the deck writes them: an integer is an `id`, a real a `value`, a word (THRU, ALL, a
// function name) a `label`; a blank inside the list is a row with nothing.
void addAttributes(std::vector<ArgumentRow>& rows, const model::DesignResp1Card& card)
{
    for (std::size_t index = 0; index < card.attributes.size(); ++index)
    {
        const model::DesignValue& value = card.attributes[index];
        ArgumentRow row;
        row.response = card.id;
        row.kind = "attribute";
        row.position = static_cast<std::int32_t>(index);
        if (value.isInteger())
        {
            row.id = value.integer;
            row.hasId = true;
        }
        else if (value.isReal())
            row.value = value.real;
        else if (value.isWord())
            row.label = value.word;
        rows.push_back(std::move(row));
    }
}

// The arguments of a DRESP2 or a DRESP3 in the order of the groups, which is the order of the argument list of the
// equation: one row per design variable, constant, response, relation, and (grid
// point, component) pair of DNODE. The text of USRDATA is no argument.
void addGroups(std::vector<ArgumentRow>& rows, std::int32_t response, const std::vector<model::DesignGroup>& groups)
{
    std::int32_t position = 0;
    for (const model::DesignGroup& group : groups)
    {
        if (group.keyword == "UserData")
            continue;
        // The kind is the neutral word of the group: a card name (DesignVar, DesignResp1, ...) or DesignNodes.
        const std::string& kind = group.keyword;
        if (group.keyword == "DesignConstants")
        {
            for (const std::string& label : group.words)
            {
                ArgumentRow row;
                row.response = response;
                row.kind = kind;
                row.position = position++;
                row.label = label;
                rows.push_back(std::move(row));
            }
            continue;
        }
        const std::size_t step = group.keyword == "DesignNodes" ? 2 : 1;
        for (std::size_t index = 0; index + step <= group.ids.size(); index += step)
        {
            ArgumentRow row;
            row.response = response;
            row.kind = kind;
            row.position = position++;
            row.id = group.ids[index];
            row.hasId = true;
            if (step == 2)
                row.component = group.ids[index + 1];
            rows.push_back(std::move(row));
        }
    }
}

// designResponseArgumentsTable: the ATTi of each DRESP1 and the arguments of each DRESP2 and DRESP3.
class DesignResponseArgumentsTableQuery final : public QueryBase
{
public:
    DesignResponseArgumentsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const std::optional<std::vector<std::int64_t>> wanted = sortedIds(run.arguments.ids("responseIds"));
        const auto keep = [&](std::int32_t id)
        { return !wanted || std::binary_search(wanted->begin(), wanted->end(), static_cast<std::int64_t>(id)); };
        std::vector<ArgumentRow> rows;
        forEachCard<model::DesignResp1Table>(model,
                                             [&](const model::DesignResp1Card& card)
                                             {
                                                 if (keep(card.id))
                                                     addAttributes(rows, card);
                                             });
        forEachCard<model::DesignResp2Table>(model,
                                             [&](const model::DesignResp2Card& card)
                                             {
                                                 if (keep(card.id))
                                                     addGroups(rows, card.id, card.groups);
                                             });
        forEachCard<model::DesignResp3Table>(model,
                                             [&](const model::DesignResp3Card& card)
                                             {
                                                 if (keep(card.id))
                                                     addGroups(rows, card.id, card.groups);
                                             });
        if (rows.empty())
            return emptyTable(info().defaultColumns());
        std::stable_sort(rows.begin(), rows.end(), [](const ArgumentRow& left, const ArgumentRow& right)
                         { return left.response < right.response; });
        const std::size_t count = rows.size();
        std::vector<std::int32_t> response(count), position(count), id(count), component(count);
        std::vector<std::string> kind(count), label(count);
        std::vector<double> value(count);
        std::vector<std::uint8_t> idValid(count), componentValid(count), labelValid(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            const ArgumentRow& row = rows[index];
            response[index] = row.response;
            kind[index] = row.kind;
            position[index] = row.position;
            id[index] = row.id;
            idValid[index] = row.hasId ? 1 : 0;
            component[index] = row.component;
            componentValid[index] = row.component != 0 ? 1 : 0;
            label[index] = row.label;
            labelValid[index] = row.label.empty() ? 0 : 1;
            value[index] = row.value;
        }
        TableBuilder builder;
        builder.int32("responseId", std::move(response));
        builder.dictionary("kind", kind);
        builder.int32("position", std::move(position));
        builder.int32("id", std::move(id), std::move(idValid));
        builder.int32("component", std::move(component), std::move(componentValid));
        builder.text("label", label, std::move(labelValid));
        builder.real("value", std::move(value));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "designResponseArgumentsTable";
        info.family = "T9";
        info.description =
            "What each design response takes, one row per item. A DRESP1 has its ATTi (`kind` attribute, `position` "
            "from 0 in the order of the card), as the deck writes them: an integer in `id`, a real in `value`, a word "
            "(Through, All) in `label`, nothing for a blank. A DRESP2 or a DRESP3 has the arguments of its equation or "
            "routine in the order of its argument list (`position` from 0): `kind` "
            "is the neutral word of the group (`DesignVar`, `DesignConstants`, `DesignResp1`, `DesignNodes`, "
            "`DesignPropRelation1`, `DesignElemRelation1`, `DesignMatRelation1`, `DesignPropRelation2`, "
            "`DesignElemRelation2`, `DesignMatRelation2`, `DesignResp2`, `DesignLoadRelation1`), `id` the design "
            "variable, response, relation or "
            "grid point, `component` the component of a DNODE, `label` the DTABLE constant. The USRDATA text of a "
            "DRESP3 is not an argument.";
        info.parameters = {parameter("responseIds", ParameterKind::IdList, "None", "keep these responses")};
        info.defaultSort = "responseId, position";
        ColumnListBuilder columns;
        columns.int32("responseId").dictionary("kind").int32("position").int32("id").int32("component");
        columns.text("label").real("value");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addDesignResponseArgumentsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DesignResponseArgumentsTableQuery>());
}

} // namespace oor::query
