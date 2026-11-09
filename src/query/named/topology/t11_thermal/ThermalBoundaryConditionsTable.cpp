#include "ThermalQuerySupport.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/constraints/singlePointConstr/SinglePointConstrTable.hpp"
#include "oor/model/cards/constraints/singlePointConstrList/SinglePointConstrListTable.hpp"
#include "oor/model/cards/loads/defaultTemperatureLoad/DefaultTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/nodeTemperatureLoad/NodeTemperatureLoadTable.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/thermal/temperatureBoundary/TemperatureBoundaryTable.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t11_thermal/ThermalQueries.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <string>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

struct Row
{
    std::int32_t sid = 0;
    const char* card = "";
    const char* kind = "fixed";
    std::int32_t node = 0;
    bool nodeValid = true;
    double temperature = NOT_A_NUMBER;
};

// True when the components of an SPC (a number of digits, 123456) include the temperature of a heat transfer analysis
// (1).
bool constrainsTemperature(std::int32_t components)
{
    return std::to_string(components).find('1') != std::string::npos;
}

// The sets the call asks for: the `setIds`, and the sets the commands of the `subcases` select (SPC, LOAD, DLOAD, and
// the four forms of TEMPERATURE). Nothing when the caller gave neither.
std::optional<std::set<base::EntityId>> askedSets(const model::Model& model, const QueryArguments& arguments)
{
    if (!arguments.has("setIds") && !arguments.has("subcases"))
        return std::nullopt;
    std::set<base::EntityId> sets;
    if (const auto ids = arguments.ids("setIds"))
    {
        for (const std::int64_t id : *ids)
        {
            if (const auto converted = base::toEntityId(id))
                sets.insert(*converted);
        }
    }
    if (const auto numbers = arguments.ids("subcases"))
    {
        const model::CaseControl& caseControl = model.control().caseControl;
        const model::CaseLineages lineages(caseControl);
        const auto add = [&](const model::CaseEntry* entry)
        {
            if (entry != nullptr && entry->value.kind == model::CaseValueKind::Integer)
            {
                if (const auto converted = base::toEntityId(entry->value.integer))
                    sets.insert(*converted);
            }
        };
        for (const model::Subcase* subcase : analysis::analysisSubcases(caseControl))
        {
            if (std::find(numbers->begin(), numbers->end(), subcase->id) == numbers->end())
                continue;
            for (const char* command : {"SinglePointConstrSelection", "StaticLoadSelection", "DynamicLoadSelection"})
                add(lineages.command(subcase->key, command));
            for (const char* variant :
                 {"TemperatureLoadPart", "InitialTemperature", "MaterialTemperature", "BothLoadAndInitial"})
                add(lineages.command(subcase->key, "TemperatureSelection", variant));
        }
    }
    return sets;
}

// The sets some subcase (or the global section) selects with TEMPERATURE(INITIAL): their TEMP cards are the starting
// temperatures.
std::set<base::EntityId> initialTemperatureSets(const model::Model& model)
{
    std::set<base::EntityId> sets;
    const model::CaseControl& caseControl = model.control().caseControl;
    const model::CaseLineages lineages(caseControl);
    for (const model::Subcase* subcase : analysis::analysisSubcases(caseControl))
    {
        const model::CaseEntry* entry = lineages.command(subcase->key, "TemperatureSelection", "InitialTemperature");
        if (entry != nullptr && entry->value.kind == model::CaseValueKind::Integer)
        {
            if (const auto converted = base::toEntityId(entry->value.integer))
                sets.insert(*converted);
        }
    }
    return sets;
}

// thermalBoundaryConditionsTable: the temperatures a heat transfer or thermal stress analysis imposes.
class ThermalBoundaryConditionsTableQuery final : public QueryBase
{
public:
    ThermalBoundaryConditionsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const std::optional<std::set<base::EntityId>> sets = askedSets(model, run.arguments);
        const auto selected = [&](base::EntityId sid) { return !sets || sets->contains(sid); };
        std::vector<Row> rows;

        // The grid points that exist, for the THRU lists of a TEMPBC.
        std::vector<base::EntityId> gridIds;
        if (const model::NodeTable* grids = model.findTable<model::NodeTable>())
        {
            for (std::uint32_t row = 0; row < grids->rows().rowCount(); ++row)
            {
                if (grids->rows().isAlive(row))
                    gridIds.push_back(grids->idAtRow(row));
            }
            std::sort(gridIds.begin(), gridIds.end());
        }
        if (const auto* table = model.findTable<model::TemperatureBoundaryTable>())
        {
            for (std::uint32_t at = 0; at < table->rows().rowCount(); ++at)
            {
                if (!table->rows().isAlive(at) || !selected(table->idAt(at)))
                    continue;
                const model::TemperatureBoundaryCard card = table->card(at);
                for (const model::TemperatureBoundaryGroup& group : card.groups)
                {
                    for (const base::EntityId node : thermal::existingIdsOfRuns({group.grids}, gridIds))
                    {
                        Row row;
                        row.sid = card.id;
                        row.card = "TemperatureBoundary";
                        row.node = node;
                        // A transient TEMPBC names a function of time (an excitation), not a temperature.
                        row.temperature = card.kind == "Static" ? group.temperature : NOT_A_NUMBER;
                        rows.push_back(row);
                    }
                }
            }
        }
        // SPC and SPC1 constrain the temperature (the component 1) in a heat transfer analysis only.
        if (analysis::isHeatTransfer(model))
        {
            if (const auto* table = model.findTable<model::SinglePointConstrTable>())
            {
                for (std::uint32_t at = 0; at < table->rows().rowCount(); ++at)
                {
                    if (!table->rows().isAlive(at) || !selected(table->idAt(at)))
                        continue;
                    const model::SinglePointConstrCard card = table->card(at);
                    for (std::size_t group = 0; group < (card.hasSecondGroup() ? 2U : 1U); ++group)
                    {
                        if (card.nodes[group] == 0 || !constrainsTemperature(card.components[group]))
                            continue;
                        Row row;
                        row.sid = card.id;
                        row.card = "SinglePointConstr";
                        row.node = card.nodes[group];
                        row.temperature = card.enforced[group];
                        rows.push_back(row);
                    }
                }
            }
            if (const auto* table = model.findTable<model::SinglePointConstrListTable>())
            {
                for (std::uint32_t at = 0; at < table->rows().rowCount(); ++at)
                {
                    if (!table->rows().isAlive(at) || !selected(table->idAt(at)))
                        continue;
                    const model::SinglePointConstrListCard card = table->card(at);
                    if (!constrainsTemperature(card.components))
                        continue;
                    for (const model::NodeRun& range : card.nodes)
                    {
                        const auto first = std::lower_bound(gridIds.begin(), gridIds.end(), range.first);
                        for (auto node = first; node != gridIds.end() && *node <= range.last; ++node)
                        {
                            Row row;
                            row.sid = card.id;
                            row.card = "SinglePointConstrList";
                            row.node = *node;
                            row.temperature = 0.0; // SPC1 holds the temperature at zero
                            rows.push_back(row);
                        }
                    }
                }
            }
        }
        const std::set<base::EntityId> initial = initialTemperatureSets(model);
        if (const auto* table = model.findTable<model::NodeTemperatureLoadTable>())
        {
            for (std::uint32_t at = 0; at < table->rows().rowCount(); ++at)
            {
                if (!table->rows().isAlive(at) || !selected(table->idAt(at)))
                    continue;
                const model::NodeTemperatureLoadCard card = table->card(at);
                for (const model::NodeTemperature& point : card.points)
                {
                    Row row;
                    row.sid = table->idAt(at);
                    row.card = "NodeTemperatureLoad";
                    row.kind = initial.contains(row.sid) ? "initial" : "fixed";
                    row.node = point.node;
                    row.temperature = point.temperature;
                    rows.push_back(row);
                }
            }
        }
        if (const auto* table = model.findTable<model::DefaultTemperatureLoadTable>())
        {
            for (std::uint32_t at = 0; at < table->rows().rowCount(); ++at)
            {
                if (!table->rows().isAlive(at) || !selected(table->idAt(at)))
                    continue;
                const model::DefaultTemperatureLoadCard card = table->card(at);
                Row row;
                row.sid = table->idAt(at);
                row.card = "DefaultTemperatureLoad";
                row.kind = "default";
                row.nodeValid = false;
                row.temperature = card.temperature;
                rows.push_back(row);
            }
        }
        std::stable_sort(rows.begin(), rows.end(),
                         [](const Row& left, const Row& right) { return left.sid < right.sid; });
        const std::size_t count = rows.size();
        std::vector<std::int32_t> sid(count);
        std::vector<std::string> card(count);
        std::vector<std::string> kind(count);
        std::vector<std::int32_t> node(count);
        std::vector<std::uint8_t> nodeValid(count);
        std::vector<double> temperature(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            sid[index] = rows[index].sid;
            card[index] = rows[index].card;
            kind[index] = rows[index].kind;
            node[index] = rows[index].node;
            nodeValid[index] = rows[index].nodeValid ? 1 : 0;
            temperature[index] = rows[index].temperature;
        }
        TableBuilder builder;
        builder.int32("setId", std::move(sid));
        builder.dictionary("card", card);
        builder.dictionary("kind", kind);
        builder.int32("nodeId", std::move(node), std::move(nodeValid));
        builder.real("temperature", std::move(temperature));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "thermalBoundaryConditionsTable";
        info.family = "T11";
        info.description =
            "The temperatures the analysis imposes, a row for each grid point, card the card type: "
            "TemperatureBoundary (a TEMPBC, a fixed temperature, kind fixed; a transient TEMPBC names a function of "
            "time, so its temperature is null), SinglePointConstr and SinglePointConstrList (an SPC and an SPC1) on "
            "the component 1 (fixed temperatures, listed when the deck is a heat transfer analysis, APP HEAT; an SPC1 "
            "holds the temperature at zero), NodeTemperatureLoad (a TEMP, kind initial when a TEMPERATURE(INITIAL) "
            "command selects its set, else fixed: a temperature the load sets) and DefaultTemperatureLoad (a TEMPD, "
            "kind default, no grid point: the temperature of every point the set does not give). setIds "
            "and "
            "subcases name the sets (a subcase selects the sets its SPC, LOAD, DLOAD and TEMPERATURE commands name); "
            "with "
            "neither, every set.";
        info.parameters = {parameter("setIds", ParameterKind::IdList, "None", "the sets to list"),
                           parameter("subcases", ParameterKind::IdList, "None", "the sets these subcases select")};
        info.defaultSort = "setId (then the order of the cards)";
        ColumnListBuilder columns;
        columns.int32("setId").dictionary("card").dictionary("kind").int32("nodeId").real("temperature");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addThermalBoundaryConditionsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ThermalBoundaryConditionsTableQuery>());
}

} // namespace oor::query
