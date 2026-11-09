#include "ForceBlocks.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r3_forces/ForceQueries.hpp"

#include <cmath>
#include <map>
#include <tuple>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// fastenerForces: the forces of the CBUSH, CFAST, CWELD and scalar spring elements along the fastener axis; with
// `envelope` the extremes over the subcases and steps.
class FastenerForcesQuery final : public ResultQuery
{
public:
    FastenerForcesQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const ElementRequest request = ElementRequest::from(context.arguments());
        if (request.axisVector && request.axisName.empty() && !context.hasModel())
            throw QueryError(
                "an axis given as a vector is turned with the element system of each fastener: it needs the model");
        const ElementResolution resolution =
            resolveElements(context, fastenerForceChain(), request, "the forces of the fasteners", fastenerFields());
        const bool envelope = context.arguments().boolOr("envelope", false);
        if (!envelope)
        {
            ForceColumns columns;
            columns.tension = true;
            return forceTable(context, info(), resolution, fastenerFields(), columns, {"elementId"});
        }
        return envelopeOf(context, resolution);
    }

private:
    [[nodiscard]] std::shared_ptr<const QueryTable> envelopeOf(const ResultQueryContext& context,
                                                               const ElementResolution& resolution) const
    {
        struct Extremes
        {
            std::string card;
            std::size_t set = 0;
            double maxTension = NOT_A_NUMBER;
            std::int32_t maxTensionSubcase = 0;
            double maxShear = NOT_A_NUMBER;
            std::int32_t maxShearSubcase = 0;
            double minAxial = NOT_A_NUMBER;
            std::int32_t minAxialSubcase = 0;
            bool single = true;
        };
        std::map<std::int32_t, Extremes> found;
        for (const ElementFlat& flat : resolution.set.flats())
        {
            if (flat.isComplex())
                continue;
            const std::vector<double>& axial = flat.values.at("axial");
            const std::vector<double>& shear = flat.values.at("shear");
            for (std::size_t row = 0; row < flat.rows(); ++row)
            {
                Extremes& extremes = found[flat.eid[row]];
                extremes.card = flat.card;
                extremes.set = flat.set;
                extremes.single = extremes.single && flat.precision == Precision::Single;
                if (!std::isnan(axial[row]))
                {
                    const double tension = std::max(axial[row], 0.0);
                    if (std::isnan(extremes.maxTension) || tension > extremes.maxTension)
                    {
                        extremes.maxTension = tension;
                        extremes.maxTensionSubcase = flat.subcase;
                    }
                    if (std::isnan(extremes.minAxial) || axial[row] < extremes.minAxial)
                    {
                        extremes.minAxial = axial[row];
                        extremes.minAxialSubcase = flat.subcase;
                    }
                }
                if (!std::isnan(shear[row]) && (std::isnan(extremes.maxShear) || shear[row] > extremes.maxShear))
                {
                    extremes.maxShear = shear[row];
                    extremes.maxShearSubcase = flat.subcase;
                }
            }
        }
        if (found.empty())
            return emptyTable(info().columnsFor("envelope"));
        std::vector<std::int32_t> eid, pid, tensionSubcase, shearSubcase, axialSubcase;
        std::vector<std::uint8_t> pidValid;
        std::vector<std::string> type;
        std::vector<double> maxTension, maxShear, minAxial;
        bool single = true;
        for (const auto& [id, extremes] : found)
        {
            eid.push_back(id);
            type.push_back(extremes.card);
            const std::int32_t property = context.elementCard(extremes.set, id).pid;
            pid.push_back(property);
            pidValid.push_back(property > 0 ? 1 : 0);
            maxTension.push_back(extremes.maxTension);
            tensionSubcase.push_back(extremes.maxTensionSubcase);
            maxShear.push_back(extremes.maxShear);
            shearSubcase.push_back(extremes.maxShearSubcase);
            minAxial.push_back(extremes.minAxial);
            axialSubcase.push_back(extremes.minAxialSubcase);
            single = single && extremes.single;
        }
        RowBlock block(eid.size());
        block.ints("elementId", std::move(eid));
        block.texts("type", std::move(type));
        block.ints("propId", std::move(pid), std::move(pidValid));
        block.reals("maxTension", std::move(maxTension));
        block.ints("maxTensionSubcase", std::move(tensionSubcase));
        block.reals("maxShear", std::move(maxShear));
        block.ints("maxShearSubcase", std::move(shearSubcase));
        block.reals("minAxial", std::move(minAxial));
        block.ints("minAxialSubcase", std::move(axialSubcase));
        block.precision(single ? Precision::Single : Precision::Double);
        RowAccumulator accumulator;
        std::vector<std::int16_t> providers;
        for (const ProviderInfo& provider : resolution.set.providers())
            providers.push_back(accumulator.addProvider(provider.name, provider.approximate));
        if (providers.size() == 1)
            block.provider(providers.front());
        else
            noteInputs(accumulator, resolution.set, {"maxTension", "maxShear", "minAxial"});
        accumulator.add(std::move(block));
        return accumulator.finish(context, info(), "True");
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "fastenerForces";
        info.family = "R3";
        info.resultQuery = true;
        info.detailParameter = "envelope";
        info.description =
            "The forces of the fasteners (CBUSH, CFAST, CWELD and the scalar springs) along the fastener axis: the "
            "axial force "
            "(signed along the axis), its tension (`max(axial, 0)`), the two shears and their resultant, the torque "
            "and the "
            "two moments, in the element system as stored (null where the table does not give one). They come from the "
            "force table, else from the grid point forces of the element (axial, shear resultant and torque only, "
            "along "
            "the axis of the `fastenersTable`). `axis` gives the forces along another axis: \"x\", \"y\" or \"z\" of "
            "the "
            "element system, or a vector in the basic system (the forces are then turned to the basic system with the "
            "element system and only the axial force, the shear resultant and the torque are given). With `envelope` "
            "each fastener has one row: its largest tension, its largest shear and its smallest axial force, with the "
            "subcase of each.";
        info.parameters = {
            parameter("elementIds", ParameterKind::IdList, "None", "the fasteners to list"),
            parameter("types", ParameterKind::TextList, "None", "the element types (\"CBUSH\", ...)"),
            parameter("envelope", ParameterKind::Bool, "False", "one row for each fastener with the extremes"),
            parameter("axis", ParameterKind::RealsOrText, "None",
                      "the axis the forces are given along: a vector, or \"x\", \"y\", \"z\" of the element system")};
        info.defaultSort = "subcase, [step], elementId";
        ColumnListBuilder rows;
        rows.int32("subcase")
            .step()
            .int32("elementId")
            .dictionary("type")
            .int32("propId")
            .results("axial tension shear1 shear2 shear torque moment1 moment2");
        info.columnSets.push_back(rows.build("False"));
        ColumnListBuilder envelope;
        envelope.int32("elementId").dictionary("type").int32("propId").result("maxTension").int32("maxTensionSubcase");
        envelope.result("maxShear").int32("maxShearSubcase").result("minAxial").int32("minAxialSubcase");
        info.columnSets.push_back(envelope.build("True"));
        return info;
    }
};

} // namespace

void addFastenerForcesQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<FastenerForcesQuery>());
}

} // namespace oor::query
