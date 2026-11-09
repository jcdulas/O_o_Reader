#include "oor/analysis/mass/Laminate.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/common/ElementData.hpp"
#include "oor/query/named/common/LaminateCards.hpp"
#include "oor/query/named/topology/t12_laminates/LaminateQueries.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <unordered_map>

namespace oor::query
{

namespace
{

// A fibre angle brought to (-90, 90]: a ply at 135 degrees is a ply at -45.
double foldedAngle(double degrees)
{
    double folded = std::fmod(degrees + 90.0, 180.0);
    if (folded < 0.0)
        folded += 180.0;
    return folded - 90.0;
}

// laminateSummary: one row for each laminate.
class LaminateSummaryQuery final : public QueryBase
{
public:
    LaminateSummaryQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const double tolerance = run.arguments.realOr("angleTolerance", 1.0);
        std::vector<std::uint16_t> types;
        for (const std::uint16_t type : typesInSpace(model::CardCategory::Property, base::IdSpace::Property))
        {
            if (analysis::Laminate::supports(type))
                types.push_back(type);
        }
        const CardList list = listCards(model, types, run.arguments.ids("propertyIds"));
        if (list.size() == 0)
            return emptyTable(info().defaultColumns());

        // The elements of each laminate.
        const CardList elements = listCards(model, typesOfCategory(model::CardCategory::Element));
        std::unordered_map<std::int32_t, std::array<double, 3>> perProperty; // elements, area, mass
        if (elements.size() > 0)
        {
            ElementData data(run.model, elements, run.context);
            const std::vector<std::int32_t>& pids = data.properties();
            const std::vector<double>& areas = data.areas();
            const ElementMassData& masses = data.masses();
            for (std::size_t index = 0; index < elements.size(); ++index)
            {
                auto& totals = perProperty[pids[index]];
                totals[0] += 1.0;
                if (std::isfinite(areas[index]))
                    totals[1] += areas[index];
                if (std::isfinite(masses.mass[index]))
                    totals[2] += masses.mass[index];
            }
        }

        const std::size_t count = list.size();
        std::vector<std::int32_t> pid(count);
        std::vector<std::string> type(count);
        std::vector<std::int64_t> plyCount(count);
        std::vector<double> thickness(count);
        std::vector<double> z0(count);
        std::vector<std::string> lam(count);
        std::vector<std::uint8_t> lamValid(count);
        std::vector<std::uint8_t> symmetric(count);
        std::vector<std::uint8_t> balanced(count);
        std::array<std::vector<double>, 4> fractions;
        for (auto& values : fractions)
            values.assign(count, 0.0);
        std::vector<std::int64_t> materialOffsets{0};
        std::vector<std::int32_t> materials;
        std::vector<std::int64_t> elementCount(count);
        std::vector<double> area(count);
        std::vector<double> mass(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            const auto info = readLaminate(model, list.location(index));
            pid[index] = list.ids[index];
            type[index] = info->type;
            plyCount[index] = static_cast<std::int64_t>(info->plies.size());
            thickness[index] = info->thickness();
            z0[index] = info->z0;
            lam[index] = info->option;
            lamValid[index] = info->option.empty() ? 0 : 1;
            // Symmetric: the SYM option, or a stack that is the same read from the top.
            bool same = true;
            for (std::size_t ply = 0; ply < info->plies.size() / 2 && same; ++ply)
            {
                const PlyInfo& low = info->plies[ply];
                const PlyInfo& high = info->plies[info->plies.size() - 1 - ply];
                same = low.material == high.material &&
                       std::fabs(low.thickness - high.thickness) <= 1e-12 * std::max(1.0, low.thickness) &&
                       std::fabs(foldedAngle(low.angle) - foldedAngle(high.angle)) <= 1e-9;
            }
            symmetric[index] = (info->option == "Symmetry" || same) ? 1 : 0;
            // The share of the thickness at each angle, and whether every angle off the axes has its opposite.
            double total = 0.0;
            std::map<long long, double> byAngle; // the folded angle in micro-degrees -> thickness
            for (const PlyInfo& ply : info->plies)
            {
                const double folded = foldedAngle(ply.angle);
                total += ply.thickness;
                byAngle[std::llround(folded * 1.0e6)] += ply.thickness;
                const double magnitude = std::fabs(folded);
                std::size_t slot = 3;
                if (magnitude <= tolerance)
                    slot = 0;
                else if (std::fabs(magnitude - 45.0) <= tolerance)
                    slot = 1;
                else if (std::fabs(magnitude - 90.0) <= tolerance)
                    slot = 2;
                fractions[slot][index] += ply.thickness;
            }
            if (total > 0.0)
            {
                for (auto& values : fractions)
                    values[index] /= total;
            }
            bool balance = true;
            for (const auto& [angle, amount] : byAngle)
            {
                const double degrees = static_cast<double>(angle) / 1.0e6;
                if (std::fabs(degrees) <= 1e-6 || std::fabs(std::fabs(degrees) - 90.0) <= 1e-6)
                    continue;
                const auto opposite = byAngle.find(std::llround(-degrees * 1.0e6));
                balance = balance && opposite != byAngle.end() &&
                          std::fabs(opposite->second - amount) <= 1e-9 * std::max(1.0, amount);
            }
            balanced[index] = balance ? 1 : 0;
            std::vector<std::int32_t> distinct;
            for (const PlyInfo& ply : info->plies)
            {
                if (std::find(distinct.begin(), distinct.end(), ply.material) == distinct.end())
                    distinct.push_back(ply.material);
            }
            materials.insert(materials.end(), distinct.begin(), distinct.end());
            materialOffsets.push_back(static_cast<std::int64_t>(materials.size()));
            if (const auto found = perProperty.find(list.ids[index]); found != perProperty.end())
            {
                elementCount[index] = static_cast<std::int64_t>(found->second[0]);
                area[index] = found->second[1];
                mass[index] = found->second[2];
            }
        }
        TableBuilder builder;
        builder.int32("propId", std::move(pid));
        builder.dictionary("type", type);
        builder.int64("plyCount", std::move(plyCount));
        builder.real("thickness", std::move(thickness));
        builder.real("z0", std::move(z0));
        builder.dictionary("laminateOption", lam, std::move(lamValid));
        builder.flag("isSymmetric", symmetric);
        builder.flag("isBalanced", balanced);
        builder.real("fraction0", std::move(fractions[0]));
        builder.real("fraction45", std::move(fractions[1]));
        builder.real("fraction90", std::move(fractions[2]));
        builder.real("fractionOther", std::move(fractions[3]));
        builder.intList("materialIds", std::move(materialOffsets), std::move(materials));
        builder.int64("elementCount", std::move(elementCount));
        builder.real("area", std::move(area));
        builder.real("mass", std::move(mass));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "laminateSummary";
        info.family = "T12";
        info.description =
            "One row for each laminate: the number of plies (the plies of the SYM option included), the total "
            "thickness, Z0, the "
            "lamination option as written (null for none), whether the stack is symmetric (the SYM option, or the same "
            "plies "
            "read from the top) and balanced (for every angle off the axes the plies at the opposite angle have the "
            "same "
            "thickness), the share of the thickness at 0, at +45 and -45, at 90 degrees and at other angles (an angle "
            "counts "
            "as one of them when it is within `angleTolerance` degrees of it; angles are taken modulo 180), the "
            "materials, "
            "and the elements, area and mass of the elements of the property.";
        info.parameters = {parameter("propertyIds", ParameterKind::IdList, "None", "keep these laminates"),
                           parameter("angleTolerance", ParameterKind::Real, "1.0", "degrees")};
        info.defaultSort = "propId";
        ColumnListBuilder columns;
        columns.int32("propId")
            .dictionary("type")
            .int64("plyCount")
            .real("thickness")
            .real("z0")
            .dictionary("laminateOption");
        columns.flag("isSymmetric").flag("isBalanced").reals("fraction0 fraction45 fraction90 fractionOther");
        columns.intList("materialIds").int64("elementCount").reals("area mass");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addLaminateSummaryQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<LaminateSummaryQuery>());
}

} // namespace oor::query
