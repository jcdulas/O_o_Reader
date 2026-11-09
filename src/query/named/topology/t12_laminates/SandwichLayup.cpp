#include "oor/analysis/mass/LaminateStrains.hpp"
#include "oor/analysis/mass/MassErrors.hpp"
#include "oor/model/cards/properties/compProp/CompPropTable.hpp"
#include "oor/model/cards/properties/compPropGlobal/CompPropGlobalTable.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t12_laminates/LaminateQueries.hpp"
#include "oor/query/providers/quantities/MaterialAllowables.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// The allowable shear stress of the core material: the S of a MAT8, the SS of a MAT1; NaN when blank or another card.
double coreShearAllowable(const model::Model& model, std::int32_t mid)
{
    if (const std::optional<PlyMaterial> ortho = orthoMat2DAllowables(model, mid))
        return ortho->s > 0.0 ? ortho->s : NOT_A_NUMBER;
    if (const auto iso = isoMatStressLimits(model, mid))
        return (*iso)[2] > 0.0 ? (*iso)[2] : NOT_A_NUMBER;
    return NOT_A_NUMBER;
}

// sandwichLayup: the laminates seen as sandwiches, faces and core.
class SandwichLayupQuery final : public QueryBase
{
public:
    SandwichLayupQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        std::optional<std::vector<std::int64_t>> wanted = run.arguments.ids("propIds");
        if (wanted)
            std::sort(wanted->begin(), wanted->end());
        std::optional<std::size_t> core;
        if (run.arguments.has("core"))
        {
            const std::int64_t number = run.arguments.integerOr("core", 0);
            if (number < 1)
                throw QueryError(
                    "sandwichLayup: core is the number of the core ply (from 1), or None for the thickest");
            core = static_cast<std::size_t>(number);
        }
        std::vector<std::int32_t> pid;
        std::vector<std::string> type;
        std::vector<std::int32_t> corePly;
        std::vector<std::int32_t> coreMid;
        std::vector<double> coreThickness;
        std::vector<double> coreZ;
        std::vector<double> coreG1z;
        std::vector<double> coreG2z;
        std::vector<double> coreS;
        std::vector<std::int32_t> bottomPlies;
        std::vector<double> bottomThickness;
        std::vector<std::int32_t> topPlies;
        std::vector<double> topThickness;
        std::vector<double> faceDistance;
        std::array<std::vector<double>, 6> bottomA;
        std::array<std::vector<double>, 6> topA;
        static constexpr std::array<std::size_t, 6> TERMS = {0, 1, 2, 4, 5, 8}; // a11 a12 a16 a22 a26 a66
        const auto addProperties = [&](std::uint16_t table, const char* name)
        {
            const model::CardTable* cards = model.findTable(table);
            if (cards == nullptr)
                return;
            for (std::uint32_t row = 0; row < cards->rows().rowCount(); ++row)
            {
                if (!cards->rows().isAlive(row))
                    continue;
                const auto id = static_cast<std::int32_t>(cards->idAt(row));
                if (wanted && !std::binary_search(wanted->begin(), wanted->end(), id))
                    continue;
                std::optional<analysis::SandwichLayup> layup;
                try
                {
                    layup = analysis::sandwichLayup(model, model::CardLocation{table, row}, core);
                }
                catch (const analysis::MassError& error)
                {
                    throw QueryError("sandwichLayup: the PCOMP " + std::to_string(id) + ": " + error.what());
                }
                if (!layup)
                    continue; // a core ply number the stack does not have
                pid.push_back(id);
                type.emplace_back(name);
                corePly.push_back(static_cast<std::int32_t>(layup->corePly));
                coreMid.push_back(static_cast<std::int32_t>(layup->coreMaterial));
                coreThickness.push_back(layup->coreThickness);
                coreZ.push_back(0.5 * (layup->coreZBottom + layup->coreZTop));
                coreG1z.push_back(layup->coreShear[0]);
                coreG2z.push_back(layup->coreShear[1]);
                coreS.push_back(coreShearAllowable(model, static_cast<std::int32_t>(layup->coreMaterial)));
                bottomPlies.push_back(static_cast<std::int32_t>(layup->bottom.plies));
                bottomThickness.push_back(layup->bottom.thickness);
                topPlies.push_back(static_cast<std::int32_t>(layup->top.plies));
                topThickness.push_back(layup->top.thickness);
                faceDistance.push_back(layup->bottom.plies > 0 && layup->top.plies > 0 ? layup->faceDistance()
                                                                                       : NOT_A_NUMBER);
                for (std::size_t term = 0; term < TERMS.size(); ++term)
                {
                    bottomA[term].push_back(layup->bottom.a[TERMS[term]]);
                    topA[term].push_back(layup->top.a[TERMS[term]]);
                }
            }
        };
        addProperties(model::typeIndexOf<model::CompPropTable>(), "CompProp");
        addProperties(model::typeIndexOf<model::CompPropGlobalTable>(), "CompPropGlobal");
        // by property ID
        std::vector<std::size_t> order(pid.size());
        for (std::size_t index = 0; index < order.size(); ++index)
            order[index] = index;
        std::stable_sort(order.begin(), order.end(),
                         [&](std::size_t one, std::size_t other) { return pid[one] < pid[other]; });
        const auto sorted = [&](const auto& values)
        {
            std::decay_t<decltype(values)> result;
            for (const std::size_t index : order)
                result.push_back(values[index]);
            return result;
        };
        const auto valid = [](const std::vector<double>& values)
        {
            std::vector<std::uint8_t> flags(values.size());
            for (std::size_t index = 0; index < values.size(); ++index)
                flags[index] = std::isnan(values[index]) ? 0 : 1;
            return flags;
        };
        const auto real = [&](TableBuilder& builder, const char* name, const std::vector<double>& values)
        {
            std::vector<double> column = sorted(values);
            std::vector<std::uint8_t> flags = valid(column);
            builder.real(name, std::move(column), std::move(flags));
        };
        TableBuilder builder;
        builder.int32("propId", sorted(pid));
        builder.dictionary("type", sorted(type));
        builder.int32("corePly", sorted(corePly));
        builder.int32("coreMatId", sorted(coreMid));
        real(builder, "coreThickness", coreThickness);
        real(builder, "coreZ", coreZ);
        real(builder, "coreG1z", coreG1z);
        real(builder, "coreG2z", coreG2z);
        real(builder, "coreS", coreS);
        builder.int32("bottomPlies", sorted(bottomPlies));
        real(builder, "bottomThickness", bottomThickness);
        builder.int32("topPlies", sorted(topPlies));
        real(builder, "topThickness", topThickness);
        real(builder, "faceDistance", faceDistance);
        static constexpr std::array<const char*, 6> BOTTOM = {"bottomA11", "bottomA12", "bottomA16",
                                                              "bottomA22", "bottomA26", "bottomA66"};
        static constexpr std::array<const char*, 6> TOP = {"topA11", "topA12", "topA16", "topA22", "topA26", "topA66"};
        for (std::size_t term = 0; term < TERMS.size(); ++term)
            real(builder, BOTTOM[term], bottomA[term]);
        for (std::size_t term = 0; term < TERMS.size(); ++term)
            real(builder, TOP[term], topA[term]);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "sandwichLayup";
        info.family = "T12";
        info.description =
            "The laminates (PCOMP, PCOMPG) seen as sandwiches, one row per property: the core ply "
            "(`core`, its number in the stack from 1, both halves of a SYM laminate; the thickest ply when None), its "
            "material, thickness and height (`coreZ`, the middle of the core from the reference plane), its "
            "transverse shear moduli (`coreG1z`, `coreG2z`: G1Z and G2Z of a MAT8, G of a MAT1, infinite for a MAT8 "
            "whose G1Z or G2Z is blank) and the shear allowable of its material (`coreS`: the S of a MAT8, the SS "
            "of a MAT1); the plies and thickness of the face below the core and of the face above it, the distance "
            "between their middles (the lever arm of the bending; null when a face has no ply), and the membrane "
            "matrix A of each face in the material axes (a11 a12 a16 a22 a26 a66). readBlock(kind=\"sandwich\") "
            "gives the face fluxes and the core shear of the elements.";
        info.parameters = {parameter("propIds", ParameterKind::IdList, "None", "the laminates (all when None)"),
                           parameter("core", ParameterKind::Integer, "None",
                                     "the number of the core ply in the stack, from 1 (the thickest when None)")};
        info.defaultSort = "propId";
        ColumnListBuilder columns;
        columns.int32("propId").dictionary("type").int32("corePly").int32("coreMatId");
        columns.reals("coreThickness coreZ coreG1z coreG2z coreS");
        columns.int32("bottomPlies").real("bottomThickness").int32("topPlies").real("topThickness");
        columns.real("faceDistance");
        columns.reals("bottomA11 bottomA12 bottomA16 bottomA22 bottomA26 bottomA66");
        columns.reals("topA11 topA12 topA16 topA22 topA26 topA66");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addSandwichLayupQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<SandwichLayupQuery>());
}

} // namespace oor::query
