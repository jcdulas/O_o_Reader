#include "oor/model/cards/loads/beamDistributedLoad/BeamDistributedLoadTable.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/SetSelection.hpp"
#include "oor/query/named/topology/t8_loads_constraints/LoadConstraintQueries.hpp"

#include <algorithm>

namespace oor::query
{

namespace
{

// lineLoadsTable: the distributed loads on bars and beams (PLOAD1).
class LineLoadsTableQuery final : public QueryBase
{
public:
    LineLoadsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        // the cards of every load set, read once for all the sets
        const analysis::LoadSetIndex loadSets(model);
        // a model with no PLOAD1 has no line load: nothing to expand
        if (!loadSets.hasType(model::typeIndexOf<model::BeamDistributedLoadTable>()))
            return emptyTable(info().defaultColumns());
        const LoadSetListing listing(model, loadSets, run.arguments,
                                     askedSets(model, run.arguments, "LOAD").value_or(allLoadSets(model)));
        const auto only = sortedIds(run.arguments.ids("elementIds"));
        std::vector<std::int32_t> sid;
        std::vector<std::string> card;
        std::vector<std::int32_t> eid;
        std::vector<std::string> loadType;
        std::vector<std::string> scaleType;
        std::array<std::vector<double>, 4> values;
        std::vector<double> scale;
        std::vector<std::uint8_t> concentrated;
        for (const base::EntityId set : listing.sets())
        {
            for (const analysis::ElementaryLoad& load :
                 listing.loadsOf(set, sortedTypesOf<model::BeamDistributedLoadTable>()))
            {
                if (load.card.table != model::typeIndexOf<model::BeamDistributedLoadTable>())
                    continue;
                const model::BeamDistributedLoadCard data =
                    static_cast<const model::BeamDistributedLoadTable&>(*model.findTable(load.card.table))
                        .card(load.card.row);
                if (only && !std::binary_search(only->begin(), only->end(), static_cast<std::int64_t>(data.elemId)))
                    continue;
                sid.push_back(set);
                card.emplace_back(model::BeamDistributedLoadTable::NAME);
                eid.push_back(data.elemId);
                loadType.emplace_back(model::BeamDistributedLoadFormat::typeName(data.kind));
                scaleType.emplace_back(model::BeamDistributedLoadFormat::scaleName(data.scaleFactor));
                values[0].push_back(data.xCoord1);
                values[1].push_back(data.pressure1);
                values[2].push_back(data.xCoord2);
                values[3].push_back(data.effectiveP2()); // a blank P2 as the solver reads it (BeamDistributedLoadCard)
                scale.push_back(load.scale);
                concentrated.push_back(data.isConcentrated() ? 1 : 0);
            }
        }
        TableBuilder builder;
        builder.int32("setId", std::move(sid));
        builder.dictionary("card", card);
        builder.int32("elementId", std::move(eid));
        builder.dictionary("loadType", loadType);
        builder.dictionary("scaleType", scaleType);
        static const char* const NAMES[4] = {"x1", "p1", "x2", "p2"};
        for (std::size_t item = 0; item < 4; ++item)
            builder.real(NAMES[item], std::move(values[item]));
        builder.real("scale", std::move(scale));
        builder.flag("isConcentrated", concentrated);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "lineLoadsTable";
        info.family = "T8";
        info.description =
            "The distributed loads on bars and beams (PLOAD1) of the sets asked for, LOAD combinations expanded: the "
            "element, "
            "the load type (FX FY FZ FXE FYE FZE MX MY MZ MXE MYE MZE: F force, M moment, the axis in the basic "
            "system, E at "
            "the end the element system), the scale type (LE, FR, LEPR, FRPR), the positions x1 and x2 and the loads "
            "p1 and "
            "p2 as the solver reads them (a concentrated load has x2 = x1; a blank P2 of a distributed load is 0, as "
            "the "
            "public pload1.bdf shows), `scale`, the product of the LOAD factors, and isConcentrated (X2 blank or "
            "equal "
            "to X1: a load P1 at X1). "
            "expandCombinations=False leaves the LOAD combinations unexpanded: each load set once, "
            "its own cards (setId the set of the card, scale 1), a combination asked for replaced by the sets it "
            "reaches; loadCombinationsTable gives its factors, and the rows of a set times its combinedFactor "
            "are the expanded rows of the combination that reaches it once, to the bit.";
        info.parameters = {parameter("setIds", ParameterKind::IdList, "None", "the load sets"),
                           parameter("elementIds", ParameterKind::IdList, "None", "keep these elements"),
                           expandCombinationsParameter()};
        info.defaultSort = "setId (then the order of the cards)";
        ColumnListBuilder columns;
        columns.int32("setId").dictionary("card").int32("elementId").dictionary("loadType").dictionary("scaleType");
        columns.reals("x1 p1 x2 p2 scale").flag("isConcentrated");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addLineLoadsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<LineLoadsTableQuery>());
}

} // namespace oor::query
