#include "DesignQuerySupport.hpp"
#include "oor/model/cards/optimisation/designResp1/DesignResp1References.hpp"
#include "oor/model/cards/optimisation/designResp1/DesignResp1Table.hpp"
#include "oor/model/cards/optimisation/designResp2/DesignResp2Table.hpp"
#include "oor/model/cards/optimisation/designResp3/DesignResp3Table.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t9_design/DesignQueries.hpp"

#include <algorithm>

namespace oor::query
{

namespace
{

// Collects the numbers a DRESP1 names.
class TargetCollector final : public model::ReferenceVisitor
{
public:
    base::EntityId visit(std::string_view, model::ReferenceTarget, base::EntityId id) override
    {
        if (id > 0)
            ids.push_back(id);
        return id;
    }
    std::vector<base::EntityId> ids;
};

// The response types whose ATTi name no card but are what the response measures, given as integers: the component 1
// to 6 of a STABDER (its ATT1, format reference DRESP1). Their targets are their integer ATTi as written.
bool attributesAreTargets(const std::string& rtype)
{
    return rtype == "StabilityDerivative";
}

// One response of any of the three cards, as the table shows it.
struct ResponseRow
{
    std::int32_t id = 0;
    std::string label;
    std::string type;
    std::string propertyType; // empty: null
    std::int32_t region = 0;
    std::string attributeA; // empty: null
    std::string attributeB;
    std::vector<std::int32_t> targets;
    std::string card;          // DRESP1, DRESP2, DRESP3
    std::int32_t equation = 0; // the DEQATN of a DRESP2; 0: null
    std::string function;      // the FUNC of a DRESP2
    std::string group;         // the GROUP of a DRESP3
    std::string userType;      // the TYPE of a DRESP3
};

std::string textOf(const model::DesignValue& value)
{
    return value.isBlank() ? std::string() : model::describeDesignValue(value);
}

template <typename TableT, typename Body>
void forEachCard(const model::Model& model, Body&& body)
{
    const TableT* table = model.findTable<TableT>();
    if (table == nullptr)
        return;
    const model::RowBook& rows = table->rows();
    for (std::uint32_t row = 0; row < rows.rowCount(); ++row)
    {
        if (rows.isAlive(row))
            body(table->card(row));
    }
}

// What a DRESP2 or DRESP3 combines: the responses of its DRESP1 and DRESP2 groups and the grid points of its DNODE
// group.
std::vector<std::int32_t> argumentsOf(const std::vector<model::DesignGroup>& groups)
{
    std::vector<std::int32_t> ids;
    for (const model::DesignGroup& group : groups)
    {
        if (group.keyword == "DesignResp1" || group.keyword == "DesignResp2")
            ids.insert(ids.end(), group.ids.begin(), group.ids.end());
        else if (group.keyword == "DesignNodes")
        {
            for (std::size_t index = 0; index < group.ids.size(); index += 2)
                ids.push_back(group.ids[index]);
        }
    }
    return ids;
}

// designResponsesTable: DRESP1, DRESP2 and DRESP3, with the objective the Case Control picks.
class DesignResponsesTableQuery final : public QueryBase
{
public:
    DesignResponsesTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        std::vector<ResponseRow> rows;
        forEachCard<model::DesignResp1Table>(model,
                                             [&](model::DesignResp1Card card)
                                             {
                                                 ResponseRow row;
                                                 row.id = card.id;
                                                 row.label = card.entryLabel;
                                                 row.type = card.responseKind;
                                                 row.propertyType = textOf(card.ownerKind);
                                                 row.region = card.regionId;
                                                 row.attributeA = textOf(card.attributeA);
                                                 row.attributeB = textOf(card.attributeB);
                                                 TargetCollector collector;
                                                 model::visitDesignResp1References(card, collector);
                                                 row.targets.assign(collector.ids.begin(), collector.ids.end());
                                                 if (attributesAreTargets(card.responseKind))
                                                     for (const model::DesignValue& value : card.attributes)
                                                         if (value.isInteger())
                                                             row.targets.push_back(value.integer);
                                                 row.card = "DesignResp1";
                                                 rows.push_back(std::move(row));
                                             });
        forEachCard<model::DesignResp2Table>(model,
                                             [&](const model::DesignResp2Card& card)
                                             {
                                                 ResponseRow row;
                                                 row.id = card.id;
                                                 row.label = card.entryLabel;
                                                 row.type = "DesignResp2";
                                                 row.region = card.regionId;
                                                 row.targets = argumentsOf(card.groups);
                                                 row.card = "DesignResp2";
                                                 row.equation = card.equationId;
                                                 row.function = card.function;
                                                 rows.push_back(std::move(row));
                                             });
        forEachCard<model::DesignResp3Table>(model,
                                             [&](const model::DesignResp3Card& card)
                                             {
                                                 ResponseRow row;
                                                 row.id = card.id;
                                                 row.label = card.entryLabel;
                                                 row.type = "DesignResp3";
                                                 row.region = card.regionId;
                                                 row.targets = argumentsOf(card.groups);
                                                 row.card = "DesignResp3";
                                                 row.group = card.groupName;
                                                 row.userType = card.type;
                                                 rows.push_back(std::move(row));
                                             });
        if (rows.empty())
            return emptyTable(info().defaultColumns());
        std::stable_sort(rows.begin(), rows.end(),
                         [](const ResponseRow& left, const ResponseRow& right) { return left.id < right.id; });

        const DesignCase designCase = readDesignCase(model);
        const std::size_t count = rows.size();
        std::vector<std::int32_t> id(count), region(count);
        std::vector<std::string> label(count), type(count), propertyType(count), attributeA(count), attributeB(count),
            sense(count);
        std::vector<std::uint8_t> propertyValid(count), regionValid(count), aValid(count), bValid(count),
            objective(count), senseValid(count), equationValid(count), functionValid(count), groupValid(count),
            userTypeValid(count);
        std::vector<std::string> responseCard(count), function(count), group(count), userType(count);
        std::vector<std::int32_t> equation(count);
        std::vector<std::int64_t> offsets{0};
        std::vector<std::int32_t> targets;
        for (std::size_t index = 0; index < count; ++index)
        {
            const ResponseRow& row = rows[index];
            id[index] = row.id;
            label[index] = row.label;
            type[index] = row.type;
            propertyType[index] = row.propertyType;
            propertyValid[index] = row.propertyType.empty() ? 0 : 1;
            region[index] = row.region;
            regionValid[index] = row.region > 0 ? 1 : 0;
            attributeA[index] = row.attributeA;
            aValid[index] = row.attributeA.empty() ? 0 : 1;
            attributeB[index] = row.attributeB;
            bValid[index] = row.attributeB.empty() ? 0 : 1;
            targets.insert(targets.end(), row.targets.begin(), row.targets.end());
            offsets.push_back(static_cast<std::int64_t>(targets.size()));
            const bool isObjective = designCase.objective && *designCase.objective == row.id;
            objective[index] = isObjective ? 1 : 0;
            senseValid[index] = isObjective ? 1 : 0;
            sense[index] = designCase.maximise ? "max" : "min";
            responseCard[index] = row.card;
            equation[index] = row.equation;
            equationValid[index] = row.equation != 0 ? 1 : 0;
            function[index] = row.function;
            functionValid[index] = row.function.empty() ? 0 : 1;
            group[index] = row.group;
            groupValid[index] = row.group.empty() ? 0 : 1;
            userType[index] = row.userType;
            userTypeValid[index] = row.userType.empty() ? 0 : 1;
        }
        TableBuilder builder;
        builder.int32("id", std::move(id));
        builder.text("label", label);
        builder.dictionary("responseType", type);
        builder.dictionary("propertyType", propertyType, propertyValid);
        builder.int32("region", std::move(region), std::move(regionValid));
        builder.text("attributeA", attributeA, aValid);
        builder.text("attributeB", attributeB, bValid);
        builder.intList("targets", std::move(offsets), std::move(targets));
        builder.flag("isObjective", objective);
        builder.dictionary("objectiveSense", sense, senseValid);
        builder.dictionary("responseCard", responseCard);
        builder.int32("equationId", std::move(equation), std::move(equationValid));
        builder.dictionary("function", function, std::move(functionValid));
        builder.text("group", group, std::move(groupValid));
        builder.text("userType", userType, std::move(userTypeValid));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "designResponsesTable";
        info.family = "T9";
        info.description =
            "The design responses (DRESP1, DRESP2 and DRESP3 cards): label, type of response (the neutral RTYPE word "
            "of a "
            "DRESP1: Displacement, Stress, Weight, ...; DesignResp2 or DesignResp3 for the others), property type (the "
            "PTYPE of a DRESP1: a neutral card name such as ShellProp, or Elements), region, the attributes ATTA and "
            "ATTB of a DRESP1 as the deck writes them (null for the others), and what they measure: the elements, grid "
            "points, properties or modes a DRESP1 names (the component, ATT1, of a STABDER, whose ATTi name no card), "
            "the "
            "responses and grid points a DRESP2 or DRESP3 combines. "
            "`isObjective` is true for the response the Case Control DESOBJ names, with its `objectiveSense` (`min` "
            "unless DESOBJ(MAX)). `responseCard` is the card type (DesignResp1, DesignResp2 or DesignResp3); a DRESP2 "
            "gives its DEQATN (`equationId`) or its "
            "`function` (Sum, Average, ...), a DRESP3 its `group` and `userType`. The ATTi of a DRESP1 and the "
            "arguments "
            "of a DRESP2 and a DRESP3 are in `designResponseArgumentsTable`.";
        info.defaultSort = "id";
        ColumnListBuilder columns;
        columns.int32("id").text("label").dictionary("responseType").dictionary("propertyType").int32("region");
        columns.text("attributeA").text("attributeB").intList("targets").flag("isObjective");
        columns.dictionary("objectiveSense").dictionary("responseCard").int32("equationId").dictionary("function");
        columns.text("group").text("userType");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addDesignResponsesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DesignResponsesTableQuery>());
}

} // namespace oor::query
