#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/analysis/mass/MassCalculator.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/loads/facePressureLoad/FacePressureLoadTable.hpp"
#include "oor/model/cards/loads/pointsPressureLoad/PointsPressureLoadTable.hpp"
#include "oor/model/cards/loads/shellPressureLoad/ShellPressureLoadTable.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ElementData.hpp"
#include "oor/query/named/common/LoadVectors.hpp"
#include "oor/query/named/common/SetSelection.hpp"
#include "oor/query/named/common/Topology.hpp"
#include "oor/query/named/topology/t8_loads_constraints/LoadConstraintQueries.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <optional>

namespace oor::query
{

namespace
{

// The resultant being added up: the force and the moment about the origin of the basic system.
struct Sum
{
    base::Vec3 force;
    base::Vec3 moment; // about the origin
    void add(const base::Vec3& point, const base::Vec3& pushed, const base::Vec3& twist = {})
    {
        force += pushed;
        moment += base::cross(point, pushed) + twist;
    }
};

// The vector area of a face (half the cross product of its diagonals) and its centre, from its corners.
void faceOf(const NodePositions& positions, const std::array<base::EntityId, 4>& corners, base::Vec3& area,
            base::Vec3& centre)
{
    const std::size_t count = corners[3] == 0 ? 3 : 4;
    const base::Vec3 a = positions.position(corners[0]);
    const base::Vec3 b = positions.position(corners[1]);
    const base::Vec3 c = positions.position(corners[2]);
    if (count == 3)
    {
        area = 0.5 * base::cross(b - a, c - a);
        centre = (a + b + c) / 3.0;
    }
    else
    {
        const base::Vec3 d = positions.position(corners[3]);
        area = 0.5 * base::cross(c - a, d - b);
        centre = (a + b + c + d) / 4.0;
    }
}

// A pressure on a face of three or four corners (`corners` in the order of the card, which gives the side the normal
// points to; `pressures` at the corners), added to `sum` as the format reference says: the
// pressure interpolated over the face by its shape functions, linear on a triangle and bilinear on a quadrilateral, and
// integrated along the normal of the surface at each point. The quadrilateral takes the 2 x 2 Gauss rule, exact for a
// flat face under a bilinear pressure; the triangle the rule of its three mid-sides, exact for its linear pressure. The
// force and the moment are those of the equivalent grid point loads: the resultant of a uniform pressure on a flat
// quadrilateral acts at the centroid of its area, not at the mean of its corners (a trapezoid differs). With `along` (a
// unit vector: the direction CID, N1 to N3 of a PLOAD4) the pressure acts along it instead of the normal, the force at
// each point the pressure times the area there. With `plane` (the unit normal of the mean plane of a warped
// four-noded quadrilateral shell, warpedMeanPlane) the area of each point is that of its projection on the plane and
// the pressure acts along the plane's normal (or `along`): this library's rule (the solver's unverified; a companion
// package's D-039 applies the same), the same total force.
void addFacePressure(Sum& sum, const std::array<base::Vec3, 4>& corners, std::size_t count,
                     const std::array<double, 4>& pressures, double scale, const base::Vec3* along = nullptr,
                     const base::Vec3* plane = nullptr)
{
    const auto loadOf = [along](const base::Vec3& areaVector)
    { return along != nullptr ? base::norm(areaVector) * *along : areaVector; };
    if (count == 3)
    {
        // the mid-sides (1/2, 1/2, 0), (0, 1/2, 1/2), (1/2, 0, 1/2) in area coordinates, a third of the area each
        const base::Vec3 area = 0.5 * base::cross(corners[1] - corners[0], corners[2] - corners[0]);
        for (std::size_t side = 0; side < 3; ++side)
        {
            const std::size_t next = (side + 1) % 3;
            const base::Vec3 point = 0.5 * (corners[side] + corners[next]);
            const double pressure = 0.5 * (pressures[side] + pressures[next]);
            sum.add(point, (scale * pressure / 3.0) * loadOf(area));
        }
        return;
    }
    // the corners at (xi, eta) = (-1, -1), (1, -1), (1, 1), (-1, 1); the Gauss points at +-1/sqrt(3), weight 1 each
    constexpr std::array<double, 4> XI{-1.0, 1.0, 1.0, -1.0};
    constexpr std::array<double, 4> ETA{-1.0, -1.0, 1.0, 1.0};
    const double gauss = 1.0 / std::sqrt(3.0);
    for (const double xi : {-gauss, gauss})
    {
        for (const double eta : {-gauss, gauss})
        {
            base::Vec3 point;
            base::Vec3 alongXi;
            base::Vec3 alongEta;
            double pressure = 0.0;
            for (std::size_t corner = 0; corner < 4; ++corner)
            {
                const double shape = 0.25 * (1.0 + XI[corner] * xi) * (1.0 + ETA[corner] * eta);
                point += shape * corners[corner];
                pressure += shape * pressures[corner];
                alongXi += (0.25 * XI[corner] * (1.0 + ETA[corner] * eta)) * corners[corner];
                alongEta += (0.25 * ETA[corner] * (1.0 + XI[corner] * xi)) * corners[corner];
            }
            const base::Vec3 area = base::cross(alongXi, alongEta); // dA of the point
            sum.add(point, (scale * pressure) * loadOf(plane != nullptr ? base::dot(area, *plane) * *plane : area));
        }
    }
}

// loadResultant: the force and moment of the loads of each set (or subcase) about a point.
class LoadResultantQuery final : public QueryBase
{
public:
    LoadResultantQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        // the cards of every load set, read once for all the sets
        const analysis::LoadSetIndex loadSets(model);
        const auto point = run.arguments.point("point").value_or(std::array<double, 3>{0.0, 0.0, 0.0});
        std::int64_t frameId = 0;
        if (run.arguments.isText("frame"))
        {
            if (run.arguments.textOr("frame", "basic") != "basic")
                throw QueryError("loadResultant: frame is \"basic\" or the ID of a coordinate system");
        }
        else if (const auto ids = run.arguments.ids("frame"))
        {
            frameId = ids->empty() ? 0 : ids->front();
        }
        // The rows: a set each, or a subcase each.
        struct Item
        {
            std::int64_t key;
            base::EntityId sid;
        };
        std::vector<Item> items;
        const bool bySubcase = run.arguments.has("subcases");
        if (bySubcase)
        {
            const model::CaseControl& caseControl = model.control().caseControl;
            const model::CaseLineages lineages(caseControl);
            const auto numbers = run.arguments.ids("subcases");
            for (const model::Subcase* subcase : analysis::analysisSubcases(caseControl))
            {
                if (std::find(numbers->begin(), numbers->end(), subcase->id) == numbers->end())
                    continue;
                const auto sid = analysis::effectiveInteger(lineages, *subcase, "StaticLoadSelection");
                items.push_back(Item{subcase->id, sid ? static_cast<base::EntityId>(*sid) : 0});
            }
            if (const auto sids = run.arguments.ids("setIds"))
            {
                for (const std::int64_t sid : *sids)
                    items.push_back(Item{sid, static_cast<base::EntityId>(sid)});
            }
        }
        else
        {
            const std::vector<base::EntityId> sets =
                askedSets(model, run.arguments, model::LOAD_SELECTION).value_or(allLoadSets(model));
            for (const base::EntityId sid : sets)
                items.push_back(Item{sid, sid});
        }

        LoadVectors vectors(model, run.context.pool());
        const NodePositions positions(model, run.context.pool());
        const CardList elementList = listCards(model, typesOfCategory(model::CardCategory::Element));
        std::optional<analysis::MassSums> modelMass; // for the inertial loads, made when a set has one
        std::vector<Sum> sums(items.size());
        std::vector<std::string> sources(items.size());
        for (std::size_t index = 0; index < items.size(); ++index)
        {
            run.context.checkCancelled();
            bool skipped = false;
            for (const analysis::ElementaryLoad& load : analysis::LoadSets::expand(model, loadSets, items[index].sid))
            {
                Sum& sum = sums[index];
                if (load.isConcentrated() && load.kind != analysis::LoadKind::EnforcedMotion)
                {
                    for (const NodalLoad& nodal : vectors.nodal(load))
                        sum.add(positions.position(nodal.node), nodal.force, nodal.moment);
                }
                else if (load.kind == analysis::LoadKind::Gravity)
                {
                    if (!modelMass)
                    {
                        const MassSetting setting(model, run.context.pool());
                        modelMass = analysis::MassCalculator::total(model, analysis::MassCalculator::allElements(model),
                                                                    setting.options(), run.context.pool())
                                        .sums;
                    }
                    if (const auto acceleration = vectors.acceleration(load); acceleration && modelMass->mass() != 0.0)
                        sum.add(modelMass->centerOfGravity(), modelMass->mass() * *acceleration);
                }
                else if (load.kind == analysis::LoadKind::Pressure)
                {
                    const std::uint16_t type = load.card.table;
                    if (type == model::typeIndexOf<model::PointsPressureLoadTable>())
                    {
                        const model::PointsPressureLoadCard data =
                            static_cast<const model::PointsPressureLoadTable&>(*model.findTable(type))
                                .card(load.card.row);
                        base::Vec3 area;
                        base::Vec3 centre;
                        faceOf(positions, {data.nodes[0], data.nodes[1], data.nodes[2], data.nodes[3]}, area, centre);
                        sum.add(centre, load.scale * data.pressure * area);
                        continue;
                    }
                    double pressure = 0.0; // the mean pressure, on the face of a solid
                    std::array<double, 4> cornerPressures{};
                    base::EntityId node1 = 0;
                    base::EntityId node3Or4 = 0;
                    std::int32_t directionSystem = 0;
                    std::optional<base::Vec3> direction; // CID, N1 to N3 of a PLOAD4, as given
                    if (type == model::typeIndexOf<model::ShellPressureLoadTable>())
                    {
                        pressure = static_cast<const model::ShellPressureLoadTable&>(*model.findTable(type))
                                       .card(load.card.row)
                                       .pressure;
                        cornerPressures.fill(pressure);
                    }
                    else
                    {
                        const model::FacePressureLoadCard data =
                            static_cast<const model::FacePressureLoadTable&>(*model.findTable(type))
                                .card(load.card.row);
                        pressure =
                            (data.pressures[0] + data.pressures[1] + data.pressures[2] + data.pressures[3]) / 4.0;
                        cornerPressures = {data.pressures[0], data.pressures[1], data.pressures[2], data.pressures[3]};
                        node1 = data.node1;
                        node3Or4 = data.node3Or4;
                        if (data.kind == model::PressureKind::Line)
                        {
                            skipped = true; // a load along an edge (SORL = LINE): not a pressure on the face
                            continue;
                        }
                        const base::Vec3 given{data.direction[0], data.direction[1], data.direction[2]};
                        if (base::norm(given) > 0.0)
                        {
                            directionSystem = data.frameId;
                            direction = given;
                        }
                    }
                    // The direction in the basic system at a point of the face (it turns with a cylindrical system).
                    const auto directionAt = [&](const std::array<base::Vec3, 4>& corners, std::size_t count)
                    {
                        base::Vec3 centre;
                        for (std::size_t corner = 0; corner < count; ++corner)
                            centre += corners[corner] / static_cast<double>(count);
                        return base::normalized(vectors.inBasic(directionSystem, centre, *direction));
                    };
                    std::vector<base::EntityId> slots;
                    for (const analysis::IdRun& range : load.elements)
                    {
                        for (const std::int32_t eid : idsInRun(elementList.ids, range.first, range.last))
                        {
                            const auto location = model.locate(base::IdSpace::Element, eid);
                            if (!location)
                                continue;
                            const analysis::ElementShape shape = analysis::ElementTopology::shapeOf(location->table);
                            const std::size_t cornerCount = shape == analysis::ElementShape::Triangle        ? 3
                                                            : shape == analysis::ElementShape::Quadrilateral ? 4
                                                                                                             : 0;
                            if (cornerCount != 0 && !analysis::ElementTopology::alternatingCorners(location->table))
                            {
                                // the corners of the shell in the order of its card (the mid-side points left out)
                                slots.clear();
                                analysis::ElementTopology::slotsOf(*model.findTable(location->table), location->table,
                                                                   location->row, slots);
                                std::array<base::Vec3, 4> corners{};
                                bool placed = slots.size() >= cornerCount;
                                for (std::size_t corner = 0; placed && corner < cornerCount; ++corner)
                                {
                                    placed = slots[corner] != 0 && positions.has(slots[corner]);
                                    if (placed)
                                        corners[corner] = positions.position(slots[corner]);
                                }
                                // a warped four-noded quadrilateral (not a CQUAD8, whose surface is its own): its
                                // mean plane
                                const std::optional<base::Vec3> plane = placed && cornerCount == 4 && slots.size() == 4
                                                                            ? warpedMeanPlane(corners)
                                                                            : std::nullopt;
                                if (placed)
                                {
                                    // a PLOAD4 on a triangle gives P1 to P3 (P4 is not used); a PLOAD4 that gives P1
                                    // alone has P2 to P4 equal to it (the card's defaults)
                                    if (direction)
                                    {
                                        const base::Vec3 along = directionAt(corners, cornerCount);
                                        addFacePressure(sum, corners, cornerCount, cornerPressures, load.scale, &along,
                                                        plane ? &*plane : nullptr);
                                    }
                                    else
                                        addFacePressure(sum, corners, cornerCount, cornerPressures, load.scale, nullptr,
                                                        plane ? &*plane : nullptr);
                                    continue;
                                }
                            }
                            if (analysis::dimensionOf(shape) == 2)
                            {
                                const analysis::ElementValues values =
                                    analysis::ElementGeometry::of(model, *location,
                                                                  analysis::bit(analysis::Quantity::Area) |
                                                                      analysis::bit(analysis::Quantity::Center) |
                                                                      analysis::bit(analysis::Quantity::Normal));
                                if (analysis::isDefined(values.center) && analysis::isDefined(values.normal))
                                    sum.add(values.center, load.scale * pressure * values.area * values.normal);
                            }
                            else if (analysis::dimensionOf(shape) == 3)
                            {
                                if (const auto face = pressureFace(model, *location, node1, node3Or4))
                                {
                                    const std::size_t faceCorners = (*face)[3] == 0 ? 3 : 4;
                                    std::array<base::Vec3, 4> corners{};
                                    for (std::size_t corner = 0; corner < faceCorners; ++corner)
                                        corners[corner] = positions.position((*face)[corner]);
                                    std::array<double, 4> uniform{};
                                    uniform.fill(pressure);
                                    if (direction)
                                    {
                                        const base::Vec3 along = directionAt(corners, faceCorners);
                                        addFacePressure(sum, corners, faceCorners, uniform, load.scale, &along);
                                    }
                                    else // on a solid positive pressure is inward: against the outward normal
                                        addFacePressure(sum, corners, faceCorners, uniform, -load.scale);
                                }
                            }
                        }
                    }
                }
                else if (load.kind == analysis::LoadKind::Rotation || load.kind == analysis::LoadKind::LineLoad ||
                         load.kind == analysis::LoadKind::Acceleration ||
                         load.kind == analysis::LoadKind::PointAcceleration)
                {
                    skipped = true;
                }
            }
            sources[index] = skipped
                                 ? "load cards (RFORCE, ACCEL, ACCEL1, PLOAD1 and PLOAD4 along an edge not included)"
                                 : "load cards";
        }

        // The resultant about the point, along the axes of the frame.
        base::Mat3 axes = base::Mat3::identity();
        if (frameId != 0)
        {
            analysis::CoordinateTransform transform(model);
            axes = transform.frame(static_cast<base::EntityId>(frameId)).axes;
        }
        const base::Vec3 about{point[0], point[1], point[2]};
        std::vector<std::int32_t> key(items.size());
        std::array<std::vector<double>, 6> values;
        for (auto& column : values)
            column.resize(items.size());
        std::array<std::vector<double>, 3> pointColumns;
        for (auto& column : pointColumns)
            column.assign(items.size(), 0.0);
        for (std::size_t index = 0; index < items.size(); ++index)
        {
            key[index] = static_cast<std::int32_t>(items[index].key);
            const base::Vec3 moment = sums[index].moment - base::cross(about, sums[index].force);
            const base::Vec3 force = axes.transposed() * sums[index].force;
            const base::Vec3 twist = axes.transposed() * moment;
            values[0][index] = force.x;
            values[1][index] = force.y;
            values[2][index] = force.z;
            values[3][index] = twist.x;
            values[4][index] = twist.y;
            values[5][index] = twist.z;
            pointColumns[0][index] = about.x;
            pointColumns[1][index] = about.y;
            pointColumns[2][index] = about.z;
        }
        std::vector<std::size_t> order(items.size());
        for (std::size_t index = 0; index < order.size(); ++index)
            order[index] = index;
        std::stable_sort(order.begin(), order.end(),
                         [&](std::size_t left, std::size_t right) { return key[left] < key[right]; });
        TableBuilder builder;
        std::vector<std::int32_t> orderedKey;
        std::vector<std::string> orderedSource;
        for (const std::size_t index : order)
        {
            orderedKey.push_back(key[index]);
            orderedSource.push_back(sources[index]);
        }
        builder.int32(bySubcase ? "subcase" : "setId", std::move(orderedKey));
        static const char* const NAMES[6] = {"fx", "fy", "fz", "mx", "my", "mz"};
        for (std::size_t item = 0; item < 6; ++item)
        {
            std::vector<double> ordered;
            for (const std::size_t index : order)
                ordered.push_back(values[item][index]);
            builder.real(NAMES[item], std::move(ordered));
        }
        static const char* const POINT[3] = {"pointX", "pointY", "pointZ"};
        for (std::size_t item = 0; item < 3; ++item)
        {
            std::vector<double> ordered;
            for (const std::size_t index : order)
                ordered.push_back(pointColumns[item][index]);
            builder.real(POINT[item], std::move(ordered));
        }
        builder.text("source", orderedSource);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "loadResultant";
        info.family = "T8";
        info.description =
            "The resultant force and moment of the loads of each set (or, with `subcases`, of each subcase: the set "
            "its LOAD "
            "command selects) about `point` (basic system, the origin by default), in the basic system or along the "
            "axes of "
            "the coordinate system `frame`. It adds the concentrated forces and moments, the pressures (PLOAD, PLOAD2 "
            "and "
            "PLOAD4 on shells, along the normal or the direction CID, N1 to N3 of a PLOAD4; PLOAD4 on the face of a "
            "solid, "
            "inward; each integrated over the face, a PLOAD4 on a warped CQUAD4 or CQUADR over "
            "its mean plane, on its area projected there (this library's own rule; the solver's unverified): the loads "
            "at its grid points p n int N_i (xXi x xEta) . n, n "
            "the unit normal of its diagonals' cross product) and the inertial load of GRAV "
            "through the "
            "mass of the model (its mass at its centre of gravity). RFORCE, PLOAD1 and a PLOAD4 along an edge (SORL = "
            "LINE) are not included, and `source` "
            "says so "
            "when a set has one. With `subcases` the first column is `subcase` instead of `setId`.";
        info.parameters = {
            parameter("setIds", ParameterKind::IdList, "None", "the load sets"),
            parameter("subcases", ParameterKind::IdList, "None", "the subcases (their LOAD sets)"),
            parameter("point", ParameterKind::Point, "(0, 0, 0)", "the point the moment is taken about (basic system)"),
            parameter("frame", ParameterKind::IdListOrText, "\"basic\"", "\"basic\" or a coordinate system ID")};
        info.defaultSort = "setId (or subcase)";
        ColumnListBuilder columns;
        columns.int32("setId").reals("fx fy fz mx my mz pointX pointY pointZ").text("source");
        info.columnSets.push_back(columns.build());
        ColumnListBuilder bySubcase;
        bySubcase.int32("subcase").reals("fx fy fz mx my mz pointX pointY pointZ").text("source");
        info.columnSets.push_back(bySubcase.build("subcases")); // the form when `subcases` is given
        return info;
    }
};

} // namespace

void addLoadResultantQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<LoadResultantQuery>());
}

} // namespace oor::query
