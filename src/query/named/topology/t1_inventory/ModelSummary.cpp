#include "oor/analysis/coordinates/ModelBounds.hpp"
#include "oor/analysis/mass/MassCalculator.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ElementData.hpp"
#include "oor/query/named/topology/t1_inventory/InventoryQueries.hpp"

#include <cmath>
#include <limits>

namespace oor::query
{

namespace
{

// modelSummary: the cards counted by type, the mass and the bounding box, as (category, item, value) rows.
class ModelSummaryQuery final : public QueryBase
{
public:
    ModelSummaryQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        std::vector<std::string> categories;
        std::vector<std::string> items;
        std::vector<double> values;
        const auto add = [&](const char* category, std::string item, double value)
        {
            categories.emplace_back(category);
            items.push_back(std::move(item));
            values.push_back(value);
        };
        double total = 0.0;
        const model::CardRegistry& registry = model::CardRegistry::instance();
        for (std::uint16_t type = 0; type < registry.size(); ++type)
        {
            const model::CardTable* table = model.findTable(type);
            if (table == nullptr || table->rows().aliveCount() == 0)
                continue;
            const std::string name = type == model::CardRegistry::UNKNOWN_TYPE
                                         ? std::string("(text)")
                                         : std::string(registry.entry(type).info.name);
            add("cardCount", name, static_cast<double>(table->rows().aliveCount()));
            total += static_cast<double>(table->rows().aliveCount());
        }
        add("cardCount", "total", total);

        // The mass of the model; NaN when an element cannot be computed does not count (as model.mass() does).
        double mass = 0.0;
        {
            const MassSetting setting(model, run.context.pool());
            const analysis::MassTotals totals = analysis::MassCalculator::total(
                model, analysis::MassCalculator::allElements(model), setting.options(), run.context.pool());
            mass = totals.sums.mass();
        }
        add("mass", "total", mass);

        constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();
        std::optional<analysis::BoundingBox> box;
        try
        {
            box = analysis::boundingBox(model, run.context.pool());
        }
        catch (const base::OperationCancelled&)
        {
            throw;
        }
        catch (const base::OorError&)
        {
        }
        const char* const names[6] = {"xmin", "ymin", "zmin", "xmax", "ymax", "zmax"};
        for (int index = 0; index < 6; ++index)
        {
            double value = NOT_A_NUMBER;
            if (box)
                value = index < 3 ? box->minimum[static_cast<std::size_t>(index)]
                                  : box->maximum[static_cast<std::size_t>(index - 3)];
            add("boundingBox", names[index], value);
        }
        TableBuilder builder;
        builder.dictionary("category", categories);
        builder.text("item", items);
        builder.real("value", std::move(values));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "modelSummary";
        info.family = "T1";
        info.description =
            "The model in a few rows: category cardCount (the number of cards of each type, then the total), mass "
            "(the mass of "
            "the model) and boundingBox (xmin ... zmax of the grid points in the basic system; null without grid "
            "points).";
        info.defaultSort = "category, item (the order of the rows: card counts by card type, then the totals)";
        ColumnListBuilder columns;
        columns.dictionary("category").text("item").real("value");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addModelSummaryQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ModelSummaryQuery>());
}

} // namespace oor::query
