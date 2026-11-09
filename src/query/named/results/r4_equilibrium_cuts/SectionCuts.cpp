#include "CutRows.hpp"
#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/base/threading/ParallelFor.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/named/results/r4_equilibrium_cuts/EquilibriumQueries.hpp"
#include "oor/query/providers/quantities/CutGeometry.hpp"
#include "oor/query/providers/quantities/CutSession.hpp"
#include "oor/query/providers/quantities/Resultants.hpp"

#include <cmath>
#include <exception>
#include <sstream>

namespace oor::query
{

namespace
{

// One cut of the series: its name, its station along the axis (NaN for a named cut), the cut, the point of the moment
// and the normal its axes take z along.
struct SeriesCut
{
    std::string name;
    double station = std::nan("");
    std::shared_ptr<CutDefinition> cut;
    std::array<double, 3> point{};
    std::array<double, 3> axisNormal{};
    std::optional<base::Mat3> axes; // the axes the cut gives (columns, basic system)
};

// The axes a named cut gives: 9 numbers, a (3, 3) array whose columns are the axes (orthonormal and right-handed), or
// 6, the x direction and the normal (z), x projected on the plane normal to z.
base::Mat3 axesOfCut(const NamedCut& cut)
{
    const std::vector<double>& f = cut.frame;
    const std::string where = "the frame of the cut '" + cut.name + "'";
    if (f.size() == 6)
    {
        const base::Vec3 z = base::normalized(base::Vec3{f[3], f[4], f[5]});
        const base::Vec3 given{f[0], f[1], f[2]};
        const base::Vec3 inPlane = given - base::dot(given, z) * z;
        if (!(base::norm(z) > 0.5) || !(base::norm(inPlane) > 1.0e-9 * base::norm(given)))
            throw QueryError(where + " has no normal, or its x direction is along the normal");
        const base::Vec3 x = base::normalized(inPlane);
        return base::Mat3::fromColumns(x, base::cross(z, x), z);
    }
    const base::Vec3 x{f[0], f[3], f[6]};
    const base::Vec3 y{f[1], f[4], f[7]};
    const base::Vec3 z{f[2], f[5], f[8]};
    const base::Vec3 columns[3] = {x, y, z};
    for (std::size_t one = 0; one < 3; ++one)
        for (std::size_t other = 0; other < 3; ++other)
            if (!(std::fabs(base::dot(columns[one], columns[other]) - (one == other ? 1.0 : 0.0)) < 1.0e-6))
                throw QueryError(where + " is not orthonormal (the columns of a rotation: the axes 1, 2, 3)");
    if (!(base::dot(base::cross(x, y), z) > 0.0))
        throw QueryError(where + " is not right-handed");
    return base::Mat3::fromColumns(x, y, z);
}

// The extreme of one component of one cut over the rows of the call.
struct Extreme
{
    double value = std::nan("");
    const ResultantRow* row = nullptr;
};

std::string stationText(double station)
{
    std::ostringstream text;
    text << station;
    return text.str();
}

// sectionCuts: a series of cuts in one call, the grid point forces read once: named cuts, or stations along an axis.
class SectionCutsQuery final : public ResultQuery
{
public:
    SectionCutsQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const QueryArguments& arguments = context.arguments();
        const std::string method = arguments.textOr("method", "auto");
        if (method != "auto" && method != "nodePointForces" && method != "integration")
            throw QueryError("method is \"auto\", \"nodePointForces\" or \"integration\", not '" + method + "'");
        const std::string detail = arguments.textOr("detail", "resultant");
        if (detail != "resultant" && detail != "envelope")
            throw QueryError("detail is \"resultant\" or \"envelope\", not '" + detail + "'");
        const bool inCutAxes = context.selection().frameName == "cut";
        const std::optional<std::array<double, 3>> xAxis = xAxisOf(arguments);
        if (xAxis && !inCutAxes)
            throw QueryError("xAxis fixes the x axis of frame=\"cut\": give frame=\"cut\" with it");
        const std::int64_t system = context.resultantSystem();
        const CutGeometry geometry(context, arguments.ids("elements"));
        const std::vector<SeriesCut> series = cutsOf(context, arguments, geometry);
        for (const SeriesCut& item : series)
            if (item.axes && !inCutAxes)
                throw QueryError("the cut '" + item.name +
                                 "' gives its frame: its resultant is in that frame with frame=\"cut\"");
        if (series.empty())
            throw QueryError(
                "sectionCuts needs the cuts: `cuts` ({name: {\"plane\": (point, normal)}} or {name: {\"nodes\": "
                "IDs, \"sideElements\": IDs}}), or `axis=(origin, direction)` with `stations`");
        const std::vector<NamedCombination> combinations = arguments.combinations("combinations");
        // PERFORMANCE: one session for the series: the grid point forces are decoded and indexed by grid point once,
        // the rigid elements and MPC equations found once, and each cut only sums the rows at its own grid points (a
        // hundred stations of a plate of 4 000 shells: 163 ms, against 9.2 s for a hundred calls of sectionCut).
        const auto session = std::make_shared<CutSession>(context);
        // what the cuts read, made once before they run on the threads of the call
        // the stations share one normal: the shells the integration reads are sorted along it once, and each station
        // looks only at those that can reach its plane
        std::optional<std::array<double, 3>> stationNormal;
        for (const SeriesCut& item : series)
            if (!std::isnan(item.station) && item.cut && item.cut->isPlane)
            {
                stationNormal = item.cut->planeNormal;
                break;
            }
        session->prepare(method != "nodePointForces", stationNormal);
        const std::vector<ProviderInfo> providers = cutChain().info().providers;
        RowAccumulator accumulator;
        const std::vector<std::int16_t> providerIndex = registerCutProviders(providers, accumulator);
        const bool envelope = detail == "envelope";

        // Each cut: whether it can be cut (in order: the connections of the model are found once), then on the threads
        // of the call its rows, its combinations, its axes and its blocks. What a cut raises is raised in the order of
        // the cuts, as one cut after the other would.
        struct Evaluated
        {
            bool skipped = false; // a cut that is not a plane in frame="cut", or that nothing crosses
            std::vector<ResultantRow> rows;
            std::vector<ProviderAttempt> attempts;
            std::vector<RowBlock> blocks;
            std::exception_ptr error;
        };
        std::vector<Evaluated> evaluated(series.size());
        std::unique_ptr<CutConnections> everyConnection;
        for (std::size_t index = 0; index < series.size(); ++index)
            evaluated[index].skipped = (inCutAxes && !series[index].cut->isPlane && !series[index].axes) ||
                                       !hasLoadPath(context, *series[index].cut, &everyConnection);
        const auto evaluate = [&](std::size_t index)
        {
            Evaluated& result = evaluated[index];
            if (result.skipped)
                return;
            const SeriesCut& item = series[index];
            try
            {
                ResultantRequest request;
                request.cut = item.cut;
                request.method = method;
                request.point = item.point;
                request.session = session;
                std::vector<ProviderInfo> used;
                result.rows = resolveCut(context, request, used, result.attempts);
                if (result.rows.empty())
                    return;
                addCombinations(result.rows, combinations);
                if (inCutAxes)
                    turnRows(result.rows, item.axes ? *item.axes : cutAxes(item.axisNormal, xAxis));
                else
                    turnRowsToSystem(context, result.rows, system);
                if (envelope)
                    return;
                CutColumns columns;
                columns.elementCountColumn = "cutElementCount";
                columns.withCutName = true;
                columns.cutName = item.name;
                columns.station = item.station;
                result.blocks = cutBlocks(result.rows, providers, item.point, columns, providerIndex);
                // the blocks hold what the table takes of the rows
                std::vector<ResultantRow>().swap(result.rows);
            }
            catch (...)
            {
                result.error = std::current_exception();
            }
        };
        const results::DecodeContext decode = context.decode();
        if (decode.pool != nullptr && series.size() > 1)
            base::parallelFor(*decode.pool, series.size(), evaluate, decode.token);
        else
            for (std::size_t index = 0; index < series.size(); ++index)
                evaluate(index);

        std::vector<std::pair<const SeriesCut*, std::vector<ResultantRow>>> results;
        for (std::size_t index = 0; index < series.size(); ++index)
        {
            const SeriesCut& item = series[index];
            Evaluated& result = evaluated[index];
            if (inCutAxes && !item.cut->isPlane && !item.axes)
                throw QueryError(
                    "frame=\"cut\" (the axes of the cut, z along the normal) is defined for a plane "
                    "only, and the cut '" +
                    item.name + "' is a line of grid points: give it its \"frame\"");
            if (result.skipped)
            {
                if (context.selection().onMissing == OnMissing::Raise)
                    throw QueryError("the cut '" + item.name +
                                     "' passes through no grid point shared by the two sides (onMissing=\"skip\" "
                                     "leaves it out)");
                continue;
            }
            if (result.error)
                std::rethrow_exception(result.error);
            if (result.rows.empty() && result.blocks.empty())
            {
                if (context.selection().onMissing == OnMissing::Raise)
                    throw QuantityUnavailableError(
                        describeAttempts("the resultant across the cut '" + item.name + "'", result.attempts));
                continue;
            }
            for (RowBlock& block : result.blocks)
                accumulator.add(std::move(block));
            if (envelope)
                results.emplace_back(&item, std::move(result.rows));
        }
        if (envelope)
            return envelopeOf(context, results, providers);
        if (accumulator.empty())
            return emptyTable(info().columnsFor("resultant"));
        return accumulator.finish(context, info(), "resultant");
    }

private:
    // The cuts of the call: the named cuts in the order given, then the stations in the order given.
    static std::vector<SeriesCut> cutsOf(const ResultQueryContext& context, const QueryArguments& arguments,
                                         const CutGeometry& geometry)
    {
        std::vector<SeriesCut> series;
        for (const NamedCut& named : arguments.cuts("cuts"))
        {
            SeriesCut item;
            item.name = named.name;
            if (!named.plane.empty())
            {
                if (named.plane.size() != 6 || !named.nodes.empty() || !named.sideElements.empty())
                    throw QueryError("the cut '" + named.name +
                                     "' is a plane (point, normal): six numbers, the point then the normal, and "
                                     "nothing else");
                const std::array<double, 3> normal = {named.plane[3], named.plane[4], named.plane[5]};
                item.cut = std::make_shared<CutDefinition>(
                    geometry.byPlane({named.plane[0], named.plane[1], named.plane[2]}, normal));
                item.axisNormal = normal;
            }
            else if (!named.nodes.empty() && !named.sideElements.empty())
            {
                item.cut = std::make_shared<CutDefinition>(geometry.byNodes(named.nodes, named.sideElements));
            }
            else
            {
                throw QueryError("the cut '" + named.name +
                                 "' takes \"plane\": (point, normal), or \"nodes\" with \"sideElements\"");
            }
            item.point = item.cut->centroid;
            if (!named.point.empty())
                item.point = {named.point[0], named.point[1], named.point[2]};
            if (!named.frame.empty())
                item.axes = axesOfCut(named);
            series.push_back(std::move(item));
        }
        const bool hasAxis = arguments.has("axis");
        const bool hasStations = arguments.has("stations");
        if (hasAxis != hasStations)
            throw QueryError("stations along an axis need both `axis=(origin, direction)` and `stations`");
        if (!hasAxis)
            return series;
        std::vector<double> axis = arguments.reals("axis");
        if (axis.size() == 1)
        {
            // a coordinate system: its origin and its Z axis (the axis of a cylindrical system)
            const double number = axis[0];
            if (!(number >= 0.0) || number != std::floor(number))
                throw QueryError("axis is (origin, direction) or the ID of a coordinate system, not " +
                                 std::to_string(number));
            analysis::CoordinateTransform transform(context.requireModel("find the coordinate system of the axis"));
            const analysis::CoordinateFrame& frame = transform.frame(static_cast<base::EntityId>(number));
            const base::Vec3 z = frame.axes.column(2);
            axis = {frame.origin.x, frame.origin.y, frame.origin.z, z.x, z.y, z.z};
        }
        if (axis.size() != 6)
            throw QueryError(
                "axis is (origin, direction): six numbers in the basic system, or a coordinate system "
                "ID, not " +
                std::to_string(axis.size()) + " numbers");
        const double length = std::sqrt(axis[3] * axis[3] + axis[4] * axis[4] + axis[5] * axis[5]);
        if (!(length > 0.0))
            throw QueryError("the direction of the axis is the zero vector");
        const std::array<double, 3> direction = {axis[3] / length, axis[4] / length, axis[5] / length};
        const std::vector<double> stations = arguments.reals("stations");
        std::vector<std::array<double, 3>> points;
        for (const double station : stations)
            points.push_back(
                {axis[0] + station * direction[0], axis[1] + station * direction[1], axis[2] + station * direction[2]});
        // the side is the part beyond the station along the axis: the plane normal points from it to the rest; the
        // planes are found in one sweep along the axis
        std::vector<CutDefinition> cuts =
            geometry.byParallelPlanes(points, {-direction[0], -direction[1], -direction[2]});
        // the moment point of the stations: the point of the axis at each, or `momentPoint` (one for all, or one
        // per station)
        std::vector<double> momentPoints;
        if (arguments.has("momentPoint"))
        {
            momentPoints = arguments.reals("momentPoint");
            if (momentPoints.size() != 3 && momentPoints.size() != 3 * stations.size())
                throw QueryError("momentPoint is one point (three numbers, basic system) or one per station (" +
                                 std::to_string(3 * stations.size()) + " numbers), not " +
                                 std::to_string(momentPoints.size()) + " numbers");
        }
        for (std::size_t index = 0; index < stations.size(); ++index)
        {
            SeriesCut item;
            item.name = stationText(stations[index]);
            item.station = stations[index];
            item.point = points[index];
            if (!momentPoints.empty())
            {
                const std::size_t at = momentPoints.size() == 3 ? 0 : 3 * index;
                item.point = {momentPoints[at], momentPoints[at + 1], momentPoints[at + 2]};
            }
            item.cut = std::make_shared<CutDefinition>(std::move(cuts[index]));
            item.axisNormal = direction;
            series.push_back(std::move(item));
        }
        return series;
    }

    // The envelope form: per cut and component, the largest and the smallest value over the rows (a complex value by
    // its magnitude), with the row that gives each.
    [[nodiscard]] std::shared_ptr<const QueryTable> envelopeOf(
        const ResultQueryContext& context,
        const std::vector<std::pair<const SeriesCut*, std::vector<ResultantRow>>>& results,
        const std::vector<ProviderInfo>& providers) const
    {
        static constexpr std::array<const char*, 6> COMPONENTS = {"fx", "fy", "fz", "mx", "my", "mz"};
        std::vector<std::string> cut;
        std::vector<double> station;
        std::vector<std::string> component;
        std::array<std::vector<double>, 2> value;
        std::array<std::vector<std::string>, 2> combination;
        std::array<std::vector<std::uint8_t>, 2> combinationValid;
        std::array<std::vector<std::int32_t>, 2> subcase;
        std::array<std::vector<std::uint8_t>, 2> subcaseValid;
        std::array<std::vector<double>, 2> step;
        std::array<std::vector<std::string>, 2> analysis;
        std::vector<std::int64_t> providerRows(providers.size(), 0);
        for (const auto& [item, rows] : results)
        {
            for (const ResultantRow& row : rows)
                ++providerRows[static_cast<std::size_t>(row.provider)];
            for (std::size_t number = 0; number < 6; ++number)
            {
                std::array<Extreme, 2> extreme; // the largest, the smallest
                for (const ResultantRow& row : rows)
                {
                    const double current =
                        row.isComplex ? std::hypot(row.value[number], row.imaginary[number]) : row.value[number];
                    if (std::isnan(current))
                        continue;
                    if (extreme[0].row == nullptr || current > extreme[0].value)
                        extreme[0] = Extreme{current, &row};
                    if (extreme[1].row == nullptr || current < extreme[1].value)
                        extreme[1] = Extreme{current, &row};
                }
                cut.push_back(item->name);
                station.push_back(item->station);
                component.emplace_back(COMPONENTS[number]);
                for (std::size_t side = 0; side < 2; ++side)
                {
                    const ResultantRow* row = extreme[side].row;
                    value[side].push_back(extreme[side].value);
                    const bool isCombination = row != nullptr && !row->combination.empty();
                    combination[side].push_back(row != nullptr ? row->combination : std::string());
                    combinationValid[side].push_back(isCombination ? 1 : 0);
                    subcase[side].push_back(row != nullptr ? row->subcase : 0);
                    subcaseValid[side].push_back(row != nullptr && !isCombination ? 1 : 0);
                    step[side].push_back(row != nullptr ? row->step : std::nan(""));
                    analysis[side].push_back(row != nullptr ? row->analysis : std::string());
                }
            }
        }
        TableBuilder builder;
        builder.text("cut", cut).real("station", std::move(station)).dictionary("component", component);
        static constexpr std::array<const char*, 2> PREFIX = {"max", "min"};
        for (std::size_t side = 0; side < 2; ++side)
        {
            const std::string prefix = PREFIX[side];
            builder.real(prefix, std::move(value[side]));
            builder.text(prefix + "Combination", combination[side], std::move(combinationValid[side]));
            builder.int32(prefix + "Subcase", std::move(subcase[side]), std::move(subcaseValid[side]));
            builder.real(prefix + "Step", std::move(step[side]));
            builder.dictionary(prefix + "Analysis", analysis[side]);
        }
        QueryTable table = *builder.finish();
        std::vector<SourceNote> notes;
        for (std::size_t index = 0; index < providers.size(); ++index)
        {
            if (providerRows[index] == 0)
                continue;
            SourceNote note;
            note.provider = providers[index].name;
            note.approximate = providers[index].approximate;
            note.columns = {"max", "min"};
            note.rows = providerRows[index];
            note.status = context.readStatus();
            notes.push_back(std::move(note));
        }
        table.setSources(std::move(notes));
        return std::make_shared<const QueryTable>(std::move(table));
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "sectionCuts";
        info.family = "R4";
        info.resultQuery = true;
        info.needsModel = true;
        info.detailParameter = "detail";
        info.description =
            "A series of cuts in one call, each as `sectionCut` gives it, the grid point forces decoded once for the "
            "whole series: named cuts `cuts={name: {\"plane\": (point, normal)}}` (the side behind the plane, against "
            "the normal: the station at that point of an axis whose direction is minus the normal, with the same "
            "resultant; with the normal along the axis the side is the other part and the resultant the opposite) "
            "or `{name: {\"nodes\": IDs, "
            "\"sideElements\": IDs}}` (the moment about the centroid of the cut grid points, or about its own "
            "`\"point\"`), and stations along an "
            "axis `axis=(origin, direction)` or `axis=frameId` (the origin and the Z axis of a coordinate system) with "
            "`stations` (the distances along the axis): a plane normal to the "
            "axis at each station, the side being the elements whose centre is strictly beyond the station along the "
            "direction (one centred in the plane is not on the side; the resultant "
            "is what the outer part applies to the rest) and the moment taken about the point of the axis at the "
            "station, or about `momentPoint` (one point, or one per station): the shear, bending and torsion "
            "diagrams of a wing or a fuselage. `elements` restricts every cut "
            "to a group; `frame=\"cut\"` gives the axes of each cut: its own `\"frame\"` (a (3, 3) array whose "
            "columns are its axes in the basic system, or (x, z), x projected on the plane normal to z; a line of "
            "grid points too), else z along the normal of a named plane, along the "
            "axis for a station, x the projection of `xAxis`; a cut that gives its frame needs frame=\"cut\". "
            "`detail=\"envelope\"` gives, per cut and component, "
            "the largest and the smallest value over the subcases, steps, combinations and analyses of the call (a "
            "complex value by its magnitude), with the combination, subcase, step and analysis that give each.";
        info.parameters = {
            parameter("cuts", ParameterKind::Cuts, "None",
                      "named cuts: {name: {\"plane\": (point, normal)}} or {name: {\"nodes\": IDs, \"sideElements\": "
                      "IDs}}, each with an optional \"point\" (the moment point) and \"frame\" (its axes)"),
            parameter("axis", ParameterKind::Reals, "None",
                      "the axis of the stations: (origin, direction), or a coordinate system ID (its Z axis)"),
            parameter("momentPoint", ParameterKind::Reals, "None",
                      "the moment point of the stations: one point, or one per station (basic system)"),
            parameter("stations", ParameterKind::Reals, "None", "the distances along the axis of the stations"),
            parameter("elements", ParameterKind::IdList, "None", "restrict every cut to a group of elements"),
            parameter("method", ParameterKind::Text, "\"auto\"", "\"auto\", \"nodePointForces\" or \"integration\""),
            parameter("xAxis", ParameterKind::Reals, "None",
                      "the x axis of frame=\"cut\", projected on each plane (a vector in the basic system)"),
            parameter("detail", ParameterKind::Text, "\"resultant\"", "\"resultant\" or \"envelope\""),
            parameter("combinations", ParameterKind::Combinations, "None",
                      "linear combinations of static subcases: {name: {subcase: factor}}")};
        info.defaultSort = "cut (in the order given), combination, subcase, [step]";
        ColumnListBuilder resultant;
        resultant.text("cut").real("station").text("combination").int32("subcase").step();
        resultant.results("fx fy fz mx my mz").reals("pointX pointY pointZ");
        resultant.int64("cutElementCount").int64("nodeCount").flag("isComplete");
        resultant.textList("uncountedCards").intList("uncountedIds").dictionary("source");
        info.columnSets.push_back(resultant.build("resultant"));
        ColumnListBuilder envelope;
        envelope.text("cut").real("station").dictionary("component");
        envelope.real("max").text("maxCombination").int32("maxSubcase").real("maxStep").dictionary("maxAnalysis");
        envelope.real("min").text("minCombination").int32("minSubcase").real("minStep").dictionary("minAnalysis");
        info.columnSets.push_back(envelope.build("envelope"));
        return info;
    }
};

} // namespace

void addSectionCutsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<SectionCutsQuery>());
}

} // namespace oor::query
