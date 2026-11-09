#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/loads/defaultTemperatureLoad/DefaultTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/lineElemTemperatureLoad/LineElemTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/nodeTemperatureLoad/NodeTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/plateTemperatureLoad/PlateTemperatureLoadTable.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/common/SetSelection.hpp"
#include "oor/query/named/topology/t8_loads_constraints/LoadConstraintQueries.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// temperatureTable: the temperatures of the sets asked for.
class TemperatureTableQuery final : public QueryBase
{
public:
    TemperatureTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        std::vector<base::EntityId> sets;
        if (const auto ids = sortedIds(run.arguments.ids("setIds")))
        {
            for (const std::int64_t id : *ids)
                sets.push_back(static_cast<base::EntityId>(id));
        }
        else
        {
            const auto collect = [&](auto&& table)
            {
                if (table == nullptr)
                    return;
                for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
                {
                    if (table->rows().isAlive(row))
                        sets.push_back(table->idAt(row));
                }
            };
            collect(model.findTable<model::NodeTemperatureLoadTable>());
            collect(model.findTable<model::DefaultTemperatureLoadTable>());
            collect(model.findTable<model::PlateTemperatureLoadTable>());
            collect(model.findTable<model::LineElemTemperatureLoadTable>());
            std::sort(sets.begin(), sets.end());
            sets.erase(std::unique(sets.begin(), sets.end()), sets.end());
        }
        std::vector<std::int32_t> sid;
        std::vector<std::string> card;
        std::vector<std::string> targetType;
        std::vector<std::int32_t> targetId;
        std::vector<std::uint8_t> targetValid;
        std::vector<double> temperature;
        std::vector<double> gradient;
        // The columns of a TEMPRB row (NaN, so null, for the other cards): TB, the four gradients and the eight point
        // temperatures, which stay null when the card gives none.
        std::vector<double> temperatureB;
        std::array<std::vector<double>, 4> gradients;
        std::vector<std::int64_t> pointOffsets{0};
        std::vector<double> points;
        std::vector<std::uint8_t> pointsValid;
        const auto add = [&](base::EntityId set, const char* name, const char* type, std::int32_t id, bool known,
                             double value, double slope, const model::LineElemTemperatureLoadCard* bar = nullptr)
        {
            temperatureB.push_back(bar != nullptr ? bar->temperatureB : NOT_A_NUMBER);
            const std::array<double, 4> barGradients =
                bar != nullptr
                    ? std::array<double, 4>{bar->gradientY[0], bar->gradientY[1], bar->gradientZ[0], bar->gradientZ[1]}
                    : std::array<double, 4>{NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER};
            for (std::size_t item = 0; item < 4; ++item)
                gradients[item].push_back(barGradients[item]);
            const bool hasPoints = bar != nullptr && bar->hasPointTemperatures();
            if (hasPoints)
            {
                points.insert(points.end(), bar->pointsA.begin(), bar->pointsA.end());
                points.insert(points.end(), bar->pointsB.begin(), bar->pointsB.end());
            }
            pointOffsets.push_back(static_cast<std::int64_t>(points.size()));
            pointsValid.push_back(hasPoints ? 1 : 0);
            sid.push_back(set);
            card.emplace_back(name);
            targetType.emplace_back(type);
            targetId.push_back(id);
            targetValid.push_back(known ? 1 : 0);
            temperature.push_back(value);
            gradient.push_back(slope);
        };
        const CardList elementList = listCards(model, typesOfCategory(model::CardCategory::Element));
        for (const base::EntityId set : sets)
        {
            if (const auto* table = model.findTable<model::NodeTemperatureLoadTable>())
            {
                for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
                {
                    if (!table->rows().isAlive(row) || table->idAt(row) != set)
                        continue;
                    for (const model::NodeTemperature& point : table->card(row).points)
                        add(set, "NodeTemperatureLoad", "node", point.node, true, point.temperature, NOT_A_NUMBER);
                }
            }
            if (const auto* table = model.findTable<model::DefaultTemperatureLoadTable>())
            {
                for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
                {
                    if (table->rows().isAlive(row) && table->idAt(row) == set)
                        add(set, "DefaultTemperatureLoad", "default", 0, false, table->card(row).temperature,
                            NOT_A_NUMBER);
                }
            }
            if (const auto* table = model.findTable<model::PlateTemperatureLoadTable>())
            {
                for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
                {
                    if (!table->rows().isAlive(row) || table->idAt(row) != set)
                        continue;
                    const model::PlateTemperatureLoadCard data = table->card(row);
                    for (const model::ElementRun& run1 : data.elements)
                    {
                        for (const std::int32_t eid : idsInRun(elementList.ids, run1.first, run1.last))
                            add(set, "PlateTemperatureLoad", "element", eid, true, data.meanTemperature,
                                data.temperatureGradient);
                    }
                }
            }
            if (const auto* table = model.findTable<model::LineElemTemperatureLoadTable>())
            {
                for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
                {
                    if (!table->rows().isAlive(row) || table->idAt(row) != set)
                        continue;
                    const model::LineElemTemperatureLoadCard data = table->card(row);
                    for (const model::ElementRun& run1 : data.elements)
                    {
                        for (const std::int32_t eid : idsInRun(elementList.ids, run1.first, run1.last))
                            add(set, "LineElemTemperatureLoad", "element", eid, true, data.temperatureA,
                                data.temperatureB - data.temperatureA, &data);
                    }
                }
            }
        }
        TableBuilder builder;
        builder.int32("setId", std::move(sid));
        builder.dictionary("card", card);
        builder.dictionary("targetType", targetType);
        builder.int32("targetId", std::move(targetId), std::move(targetValid));
        builder.real("temperature", std::move(temperature));
        builder.real("gradient", std::move(gradient));
        builder.real("temperatureB", std::move(temperatureB));
        static const char* const GRADIENTS[4] = {"gradient1a", "gradient1b", "gradient2a", "gradient2b"};
        for (std::size_t item = 0; item < 4; ++item)
            builder.real(GRADIENTS[item], std::move(gradients[item]));
        builder.realList("stressPointTemperatures", std::move(pointOffsets), std::move(points), std::move(pointsValid));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "temperatureTable";
        info.family = "T8";
        info.description =
            "The temperatures of the sets asked for, card the card type: NodeTemperatureLoad (a TEMP, a row for each "
            "grid point), DefaultTemperatureLoad (a TEMPD, the default, target default with no ID), "
            "PlateTemperatureLoad (a TEMPP1, a row for each element it names, a THRU run giving the elements of the "
            "model in it; temperature TBAR and gradient TPRIME, the temperature gradient through the thickness) and "
            "LineElemTemperatureLoad (a TEMPRB, temperature TA at end A, gradient TB - TA, the change along the "
            "element). A TEMPRB row also gives temperatureB (TB), the "
            "gradients through the element (gradient1a, gradient1b: TP1A, TP1B in the element y direction; "
            "gradient2a, gradient2b: TP2A, TP2B in z) and stressPointTemperatures (TCA, TDA, TEA, TFA, TCB, TDB, "
            "TEB, TFB; null when the card gives none, the program then assumes "
            "linear "
            "gradients); these columns are null for the other cards. Without setIds, every set.";
        info.parameters = {parameter("setIds", ParameterKind::IdList, "None", "the temperature sets")};
        info.defaultSort = "setId (then the order of the cards)";
        ColumnListBuilder columns;
        columns.int32("setId")
            .dictionary("card")
            .dictionary("targetType")
            .int32("targetId")
            .reals("temperature gradient temperatureB gradient1a gradient1b gradient2a gradient2b")
            .realList("stressPointTemperatures");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addTemperatureTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<TemperatureTableQuery>());
}

} // namespace oor::query
