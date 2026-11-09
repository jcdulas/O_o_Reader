#include "../r3_forces/ForceBlocks.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r10_thermal_results/ThermalResultQueries.hpp"

#include <cmath>

namespace oor::query
{

namespace
{

// heatFluxesTable: the temperature gradient and the heat flux of the elements.
class HeatFluxesTableQuery final : public ResultQuery
{
public:
    HeatFluxesTableQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const ElementRequest request = ElementRequest::from(context.arguments());
        ElementResolution resolution =
            resolveElements(context, heatFluxChain(), request, "the heat flux of the elements", heatFluxFields());
        if (resolution.set.flats().empty())
            return emptyTable(info().defaultColumns());
        RowAccumulator accumulator;
        std::vector<std::int16_t> providers;
        for (const ProviderInfo& provider : resolution.set.providers())
            providers.push_back(accumulator.addProvider(provider.name, provider.approximate));
        for (const ElementFlat& flat : resolution.set.flats())
        {
            ForceColumns columns;
            columns.frame = true;
            RowBlock block =
                forceBlock(context, flat, heatFluxFields(), columns,
                           flat.provider >= 0 ? providers[static_cast<std::size_t>(flat.provider)] : std::int16_t(-1));
            std::vector<double> magnitude(flat.rows());
            for (std::size_t row = 0; row < flat.rows(); ++row)
                magnitude[row] = std::sqrt(flat.values.at("fluxX")[row] * flat.values.at("fluxX")[row] +
                                           flat.values.at("fluxY")[row] * flat.values.at("fluxY")[row] +
                                           flat.values.at("fluxZ")[row] * flat.values.at("fluxZ")[row]);
            block.reals("fluxMagnitude", std::move(magnitude));
            accumulator.add(std::move(block));
        }
        std::vector<std::string> keys = leadingKeys();
        keys.push_back("elementId");
        return sortedResult(accumulator.finish(context, info()), context.exec(), keys);
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "heatFluxesTable";
        info.family = "R10";
        info.resultQuery = true;
        info.description =
            "The temperature gradient and the heat flux of the elements of a heat transfer analysis, one row for each "
            "subcase, "
            "step and element, with the norm of the flux and the frame the table writes them in. They come from the "
            "heat flux "
            "table; for a table that has the gradient and no flux, minus the conductivity K of the MAT4 of the element "
            "times the stored gradient is the other way (marked approximate: it takes the K of the card, so it gives "
            "nothing for a MAT5, a blank K or a MATT4 table of K).";
        info.parameters = {parameter("elementIds", ParameterKind::IdList, "None", "the elements to list"),
                           parameter("types", ParameterKind::TextList, "None", "the element types")};
        info.defaultSort = "subcase, [step], elementId";
        ColumnListBuilder columns;
        columns.int32("subcase").step().int32("elementId").dictionary("type").int32("propId");
        columns.results("gradientX gradientY gradientZ fluxX fluxY fluxZ fluxMagnitude").dictionary("frame");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addHeatFluxesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<HeatFluxesTableQuery>());
}

} // namespace oor::query
