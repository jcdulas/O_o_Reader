#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/NamedQueryRegistry.hpp"
#include "oor/query/named/common/ColumnHelpers.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/providers/quantities/ElementModelFacts.hpp"
#include "oor/query/providers/quantities/ElementQuantities.hpp"
#include "oor/query/providers/quantities/Resultants.hpp"
#include "oor/query/results/ResultModelIndex.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <set>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

enum class ForceShape : std::uint8_t
{
    Rod,     // axial, torsion
    Spring,  // one force
    Bush,    // fx fy fz mx my mz in the element system
    Bar2,    // the two ends of a bar: moments at A and B, one shear, axial and torque
    Stations // stations along the element
};

std::optional<ForceShape> shapeOf(const std::string& name, bool fastener)
{
    if (!fastener)
    {
        if (name == "Rod2" || name == "Rod2InlineSection" || name == "Tube2" || name == "ViscousDamper2")
            return ForceShape::Rod;
        if (name == "Bar2")
            return ForceShape::Bar2;
        if (name == "Bar2LoadStations" || name == "Beam2" || name == "CurvedBeam2")
            return ForceShape::Stations;
        return std::nullopt;
    }
    if (name == "Bushing" || name == "Fastener")
        return ForceShape::Bush;
    if (name == "Weld")
        return ForceShape::Bar2;
    if (name == "SpringDofProperty" || name == "SpringDofInline" || name == "SpringScalarProperty" || name == "SpringScalarInline")
        return ForceShape::Spring;
    return std::nullopt;
}

const std::vector<std::string> ONE_DIMENSIONAL = {"axial", "shear1", "shear2", "torque", "moment1", "moment2"};
const std::vector<std::string> FASTENER = {"axial", "shear1", "shear2", "shear", "torque", "moment1", "moment2"};

// The component of a table by the first name it has (-1 when none).
int firstOf(const TableValues& values, std::initializer_list<const char*> names)
{
    for (const char* name : names)
    {
        const int found = values.component(name);
        if (found >= 0)
            return found;
    }
    return -1;
}

// The axes of the element in the basic system (the columns x, y, z), or nothing.
std::optional<base::Mat3> elementAxes(const ResultQueryContext& context, ResultModelIndex& index, std::size_t set,
                                      std::int32_t eid)
{
    const auto location = index.element(set, eid);
    const model::Model* model = context.modelOf(set);
    if (!location || model == nullptr)
        return std::nullopt;
    try
    {
        const base::Mat3 frame =
            analysis::ElementGeometry::of(*model, *location, analysis::bit(analysis::Quantity::Frame)).frame;
        if (!std::isfinite(frame.values[0]))
            return std::nullopt;
        return frame;
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

// ---- the force table of the result file
// --------------------------------------------------------------------------------

class StoredForceTableProvider final : public ElementProvider
{
public:
    explicit StoredForceTableProvider(bool fastener) : m_fastener(fastener) {}
    [[nodiscard]] std::string name() const override { return "element force table of the result file"; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ElementRequest& request,
                                  ElementFlatSet& sink) const override
    {
        Outcome outcome;
        std::string problem;
        ResultModelIndex index(context);
        std::vector<std::pair<TableRef, ForceShape>> tables;
        for (const std::string& name : context.elementNames(results::ResultKind::ElementForces))
        {
            const auto shape = shapeOf(name, m_fastener);
            if (!shape || !request.wantsType(cardOfContainer(name)))
                continue;
            for (const TableRef& ref : context.elementTables(results::ResultKind::ElementForces, name, false))
                tables.emplace_back(ref, *shape);
        }
        const bool anyTable = !tables.empty();
        // the axis given as a vector needs the element systems, which the index finds and keeps: one table at a time
        using Made = std::pair<std::optional<ElementFlat>, std::string>;
        makeInOrder<Made>(
            context, tables.size(), [&](std::size_t at) { return indexedRows(tables[at].first); },
            [&](std::size_t at)
            {
                Made made;
                made.first = flatten(context, tables[at].first, tables[at].second, request, index, made.second);
                return made;
            },
            [&](std::size_t, Made&& made)
            {
                if (!made.second.empty())
                    problem = std::move(made.second);
                if (made.first)
                    outcome.rows += sink.add(std::move(*made.first));
            },
            !request.axisVector);
        if (outcome.rows == 0)
            outcome.reason = !problem.empty() ? problem
                             : anyTable ? "the table has no row of the elements, subcases and steps asked for"
                                        : "the result file has no force table of these elements (output of FORCE was "
                                          "not requested)";
        return outcome;
    }

private:
    [[nodiscard]] std::optional<ElementFlat> flatten(const ResultQueryContext& context, const TableRef& ref,
                                                     ForceShape shape, const ElementRequest& request,
                                                     ResultModelIndex& index, std::string& problem) const
    {
        const results::ResultTableMeta& meta = ref.table->meta();
        const std::shared_ptr<const results::ResultData> decoded = context.decodeRows(ref, request.elementIds);
        if (!decoded)
            return std::nullopt;
        const TableValues values(*decoded);
        const std::vector<std::string>& fields = m_fastener ? FASTENER : ONE_DIMENSIONAL;
        const auto fieldIndex = [&](const char* field)
        { return static_cast<std::size_t>(std::find(fields.begin(), fields.end(), field) - fields.begin()); };
        // the component of the table that feeds each field (one list for a bar's end A and one for end B)
        std::vector<int> source[2];
        for (auto& list : source)
            list.assign(fields.size(), -1);
        switch (shape)
        {
        case ForceShape::Rod:
            source[0][fieldIndex("axial")] = values.component("axial");
            source[0][fieldIndex("torque")] = values.component("torsion");
            break;
        case ForceShape::Spring:
            source[0][fieldIndex("axial")] = firstOf(values, {"springForce", "damperForce"});
            break;
        case ForceShape::Bush:
        {
            const char* const names[6] = {"fx", "fy", "fz", "mx", "my", "mz"};
            int component[6];
            for (int number = 0; number < 6; ++number)
                component[number] = values.component(names[number]);
            // the axis the forces are given along: x as stored, or another axis of the element system
            static constexpr int AXIS_X[6] = {0, 1, 2, 3, 4, 5};
            static constexpr int AXIS_Y[6] = {1, 2, 0, 4, 5, 3};
            static constexpr int AXIS_Z[6] = {2, 0, 1, 5, 3, 4};
            const int* cyclic = AXIS_X;
            if (request.axisName == "y")
                cyclic = AXIS_Y;
            else if (request.axisName == "z")
                cyclic = AXIS_Z;
            else if (!request.axisName.empty() && request.axisName != "x")
                throw QueryError("axis is \"x\", \"y\", \"z\" (of the element system) or a vector, not '" +
                                 request.axisName + "'");
            source[0][fieldIndex("axial")] = component[cyclic[0]];
            source[0][fieldIndex("shear1")] = component[cyclic[1]];
            source[0][fieldIndex("shear2")] = component[cyclic[2]];
            source[0][fieldIndex("torque")] = component[cyclic[3]];
            source[0][fieldIndex("moment1")] = component[cyclic[4]];
            source[0][fieldIndex("moment2")] = component[cyclic[5]];
            break;
        }
        case ForceShape::Bar2:
            for (int end = 0; end < 2; ++end)
            {
                source[end][fieldIndex("shear1")] = values.component("shear1");
                source[end][fieldIndex("shear2")] = values.component("shear2");
                source[end][fieldIndex("axial")] = values.component("axial");
                source[end][fieldIndex("torque")] = values.component("torque");
                source[end][fieldIndex("moment1")] = values.component(end == 0 ? "bendingMomentA1" : "bendingMomentB1");
                source[end][fieldIndex("moment2")] = values.component(end == 0 ? "bendingMomentA2" : "bendingMomentB2");
            }
            break;
        case ForceShape::Stations:
            source[0][fieldIndex("moment1")] = values.component("bendingMoment1");
            source[0][fieldIndex("moment2")] = values.component("bendingMoment2");
            source[0][fieldIndex("shear1")] = values.component("shear1");
            source[0][fieldIndex("shear2")] = values.component("shear2");
            source[0][fieldIndex("axial")] = firstOf(values, {"axial", "axialForce"});
            source[0][fieldIndex("torque")] = firstOf(values, {"torque", "totalTorque"});
            break;
        }
        const bool anySource = std::any_of(source[0].begin(), source[0].end(), [](int c) { return c >= 0; });
        if (!anySource)
            return std::nullopt;
        const bool vectorAxis = shape == ForceShape::Bush && request.axisVector.has_value();
        // the stations of each beam: its first and last station, and its last position (the length its stations are
        // divided by). PERFORMANCE: the rows of a table come element by element, increasing: each
        // element's are then one run, summed in a pass (two maps looked up for every row before, kept for a table
        // whose rows are not in order).
        struct Beam
        {
            std::pair<std::int32_t, std::int32_t> stations;
            double lastPosition = 0.0;
            bool positioned = false;
        };
        std::vector<Beam> beams;              // per run of rows of one element
        std::vector<std::uint32_t> beamOfRow; // the run of each row
        std::map<std::int32_t, double> lastPosition;
        std::map<std::int32_t, std::pair<std::int32_t, std::int32_t>> stations;
        const bool byRuns =
            shape == ForceShape::Stations && std::is_sorted(decoded->ids.begin(), decoded->ids.begin() + values.rows());
        if (byRuns)
        {
            beamOfRow.resize(values.rows());
            for (std::size_t row = 0; row < values.rows(); ++row)
            {
                if (row == 0 || decoded->ids[row] != decoded->ids[row - 1])
                    beams.push_back(Beam{std::make_pair(decoded->layers[row], decoded->layers[row]), 0.0, false});
                Beam& beam = beams.back();
                beamOfRow[row] = static_cast<std::uint32_t>(beams.size() - 1);
                beam.stations.first = std::min(beam.stations.first, decoded->layers[row]);
                beam.stations.second = std::max(beam.stations.second, decoded->layers[row]);
                if (row < decoded->positions.size())
                {
                    beam.lastPosition = std::max(beam.lastPosition, decoded->positions[row]);
                    beam.positioned = true;
                }
            }
        }
        else if (shape == ForceShape::Stations)
        {
            for (std::size_t row = 0; row < values.rows(); ++row)
                if (row < decoded->positions.size())
                    lastPosition[decoded->ids[row]] =
                        std::max(lastPosition[decoded->ids[row]], decoded->positions[row]);
            for (std::size_t row = 0; row < values.rows(); ++row)
            {
                auto [position, inserted] =
                    stations.emplace(decoded->ids[row], std::make_pair(decoded->layers[row], decoded->layers[row]));
                if (!inserted)
                {
                    position->second.first = std::min(position->second.first, decoded->layers[row]);
                    position->second.second = std::max(position->second.second, decoded->layers[row]);
                }
            }
        }

        ElementFlat flat;
        const bool stepped = meta.stepKind == results::StepKind::Time ||
                             meta.stepKind == results::StepKind::Frequency ||
                             meta.stepKind == results::StepKind::Mode || meta.stepKind == results::StepKind::LoadFactor;
        std::vector<std::vector<double>> real(fields.size());
        std::vector<std::vector<double>> imaginary(fields.size());
        const bool complex = values.isComplex();
        const std::size_t shearField = m_fastener ? fieldIndex("shear") : fields.size();
        // PERFORMANCE: the rows are many (a bar has two, every subcase a table): the columns are reserved, the values
        // of a row go through two buffers made once (a companion package, 2026-10-13: readBlock of 2505 CBAR over 2000
        // subcases spent most of its time allocating them)
        const int endCount = shape == ForceShape::Bar2 ? 2 : 1;
        const std::size_t expected = values.steps() * values.rows() * static_cast<std::size_t>(endCount);
        flat.eid.reserve(expected);
        flat.node.reserve(expected);
        flat.layer.reserve(expected);
        flat.step.reserve(expected);
        flat.position.reserve(expected);
        flat.location.reserve(expected);
        for (std::size_t field = 0; field < fields.size(); ++field)
        {
            real[field].reserve(expected);
            if (complex)
                imaginary[field].reserve(expected);
        }
        std::vector<double> re(fields.size(), NOT_A_NUMBER);
        std::vector<double> im(fields.size(), NOT_A_NUMBER);
        static const std::string CENTER = "center";
        static const std::string END_A = "endA";
        static const std::string END_B = "endB";
        static const std::string STATION = "station";
        for (std::size_t stepIndex = 0; stepIndex < values.steps(); ++stepIndex)
            for (std::size_t row = 0; row < values.rows(); ++row)
            {
                const std::int32_t element = decoded->ids[row];
                // the rows of the elements asked for were selected when the table was read
                if (!request.elementIds && !request.wantsElement(element))
                    continue;
                if (request.propertyIds && !request.wantsProperty(context.elementCard(ref.set, element).pid))
                    continue;
                for (int end = 0; end < endCount; ++end)
                {
                    std::fill(re.begin(), re.end(), NOT_A_NUMBER);
                    std::fill(im.begin(), im.end(), NOT_A_NUMBER);
                    for (std::size_t field = 0; field < fields.size(); ++field)
                        if (source[end][field] >= 0)
                        {
                            re[field] = values.real(stepIndex, row, static_cast<std::size_t>(source[end][field]));
                            if (complex)
                                im[field] = values.imag(stepIndex, row, static_cast<std::size_t>(source[end][field]));
                        }
                    if (vectorAxis)
                    {
                        // the forces and moments in the element system turned to the basic system, then along the axis
                        const auto axes = elementAxes(context, index, ref.set, element);
                        if (!axes)
                        {
                            problem =
                                "the element system of the fasteners is not known: the axis given as a vector cannot "
                                "be used";
                            return std::nullopt;
                        }
                        const base::Vec3 axis = base::normalized(
                            base::Vec3{(*request.axisVector)[0], (*request.axisVector)[1], (*request.axisVector)[2]});
                        const base::Vec3 force = *axes * base::Vec3{re[fieldIndex("axial")], re[fieldIndex("shear1")],
                                                                    re[fieldIndex("shear2")]};
                        const base::Vec3 moment =
                            *axes *
                            base::Vec3{re[fieldIndex("torque")], re[fieldIndex("moment1")], re[fieldIndex("moment2")]};
                        const double along = base::dot(force, axis);
                        const base::Vec3 across = force - along * axis;
                        std::fill(re.begin(), re.end(), NOT_A_NUMBER);
                        re[fieldIndex("axial")] = along;
                        re[shearField] = base::norm(across);
                        re[fieldIndex("torque")] = base::dot(moment, axis);
                    }
                    else if (shearField < fields.size() && !std::isnan(re[fieldIndex("shear1")]) &&
                             !std::isnan(re[fieldIndex("shear2")]))
                    {
                        re[shearField] = std::hypot(re[fieldIndex("shear1")], re[fieldIndex("shear2")]);
                    }
                    flat.eid.push_back(element);
                    flat.node.push_back(0);
                    flat.layer.push_back(0);
                    flat.step.push_back(stepped ? decoded->steps[stepIndex] : NOT_A_NUMBER);
                    if (!decoded->eigenvalues.empty())
                        flat.eigenvalue.push_back(decoded->eigenvalues[stepIndex]);
                    double station = NOT_A_NUMBER;
                    const std::string* location = &CENTER;
                    if (shape == ForceShape::Bar2)
                    {
                        location = end == 0 ? &END_A : &END_B;
                        station = end == 0 ? 0.0 : 1.0;
                    }
                    else if (shape == ForceShape::Stations)
                    {
                        const auto range = byRuns ? beams[beamOfRow[row]].stations : stations.at(element);
                        location = decoded->layers[row] == range.first    ? &END_A
                                   : decoded->layers[row] == range.second ? &END_B
                                                                          : &STATION;
                        const double length =
                            byRuns ? (beams[beamOfRow[row]].positioned ? beams[beamOfRow[row]].lastPosition : 0.0)
                                   : (lastPosition.contains(element) ? lastPosition.at(element) : 0.0);
                        if (row < decoded->positions.size() && length > 0.0)
                            station = decoded->positions[row] / length;
                    }
                    flat.position.push_back(station);
                    flat.location.push_back(*location);
                    for (std::size_t field = 0; field < fields.size(); ++field)
                    {
                        real[field].push_back(re[field]);
                        if (complex)
                            imaginary[field].push_back(im[field]);
                    }
                }
            }
        if (flat.eid.empty())
            return std::nullopt;
        flat.set = ref.set;
        flat.analysis = context.sets()[ref.set].analysis;
        flat.status = std::string(results::resultStatusName(meta.status));
        flat.subcase = meta.subcase;
        flat.designCycle = meta.designCycle;
        flat.stepKind = stepped ? meta.stepKind : results::StepKind::None;
        flat.elementName = meta.elementName;
        flat.card = cardOfContainer(meta.elementName);
        flat.frame = vectorAxis ? "basic" : "element";
        flat.precision = precisionOf(*decoded);
        flat.magnitudePhase = meta.storedFormat == results::ResultFormat::MagnitudePhase;
        for (std::size_t field = 0; field < fields.size(); ++field)
        {
            flat.values[fields[field]] = std::move(real[field]);
            if (complex)
                flat.imaginary[fields[field]] = std::move(imaginary[field]);
        }
        return flat;
    }

    bool m_fastener;
};

// ---- the force of a rod from its stress
// ------------------------------------------------------------------------------------

class RodStressTimesAreaProvider final : public ElementProvider
{
public:
    [[nodiscard]] std::string name() const override { return "stress table and the area of the property (rods)"; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ElementRequest& request,
                                  ElementFlatSet& sink) const override
    {
        if (!context.hasModel())
            return {0, "no model is attached to the results (the area is in the model)"};
        Outcome outcome;
        std::map<std::string, std::int64_t> reasons;
        bool anyTable = false;
        std::map<std::size_t, std::unique_ptr<ElementModelFacts>> facts;
        for (const std::string& name : context.elementNames(results::ResultKind::Stress))
        {
            if (familyOfContainer(name) != ElementFamily::Rod || !request.wantsType(cardOfContainer(name)))
                continue;
            for (const TableRef& ref : context.elementTables(results::ResultKind::Stress, name, false))
            {
                anyTable = true;
                auto stress = flattenStressTable(context, ref, false, request);
                if (!stress || stress->isComplex())
                    continue;
                if (!facts.contains(ref.set))
                    facts[ref.set] = std::make_unique<ElementModelFacts>(context, ref.set);
                ElementFlat flat;
                const std::size_t rows = stress->rows();
                std::vector<double> axial(rows, NOT_A_NUMBER);
                std::vector<double> torque(rows, NOT_A_NUMBER);
                std::vector<std::size_t> keep;
                for (std::size_t row = 0; row < rows; ++row)
                {
                    const RodFacts& rod = facts[ref.set]->rod(stress->eid[row]);
                    if (!rod.ok)
                    {
                        ++reasons[rod.why];
                        continue;
                    }
                    axial[row] = stress->values.at("axial")[row] * rod.area;
                    if (rod.torsion > 0.0 && rod.coefficient > 0.0)
                        torque[row] = stress->values.at("torsion")[row] * rod.torsion / rod.coefficient;
                    keep.push_back(row);
                }
                if (keep.empty())
                    continue;
                flat = *stress;
                flat.values.clear();
                for (const std::string& field : ONE_DIMENSIONAL)
                    flat.values[field] = std::vector<double>(rows, NOT_A_NUMBER);
                flat.values["axial"] = std::move(axial);
                flat.values["torque"] = std::move(torque);
                flat.frame = "element";
                flat.keepRows(keep);
                outcome.rows += sink.add(std::move(flat));
            }
        }
        if (outcome.rows == 0)
            outcome.reason = !reasons.empty() ? joinReasons(reasons)
                             : anyTable       ? "no stress row of the rods asked for"
                                              : "the result file has no stress table of rods";
        return outcome;
    }
};

// ---- the grid point forces of a fastener
// ------------------------------------------------------------------------------------

// The force a fastener carries from the grid point forces its element puts on its end: along the fastener axis (the
// line between its ends, or the element x axis), the tension, the resultant of the transverse force and the torque
// about the axis. The end B is used (the force of a fastener in tension on its end B points back along the axis), else
// the end A.
class NodePointForcesOfElementProvider final : public ElementProvider
{
public:
    [[nodiscard]] std::string name() const override { return "grid point forces of the element"; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ElementRequest& request,
                                  ElementFlatSet& sink) const override
    {
        if (!context.hasModel())
            return {0, "no model is attached to the results (the ends and the axis of the fastener are in the model)"};
        std::vector<TableRef> tables = context.resultantTables(results::ResultKind::NodePointForces);
        if (tables.empty())
            return {0, "the result file has no grid point force table (output of GPFORCE was not requested)"};
        // the ends and the axis of every fastener asked for
        QueryArguments arguments;
        if (request.elementIds)
            arguments.set("elementIds", *request.elementIds);
        if (!request.types.empty())
            arguments.set("types", request.types);
        const QueryRun run{context.modelPointer(), arguments, context.exec(), nullptr, {}};
        const std::shared_ptr<const QueryTable> fasteners = NamedQueryRegistry::instance().run("fastenersTable", run);
        struct Ends
        {
            std::int32_t n1 = 0;
            std::int32_t n2 = 0;
            std::array<double, 3> axis = {NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER};
            std::string card;
        };
        std::map<std::int32_t, Ends> ends;
        {
            const auto eid = asInt32(fasteners->column("elementId"));
            const auto n1 = asInt32(fasteners->column("n1"));
            const auto n2 = asInt32(fasteners->column("n2"));
            const auto ax = asDoubles(fasteners->column("axisX"));
            const auto ay = asDoubles(fasteners->column("axisY"));
            const auto az = asDoubles(fasteners->column("axisZ"));
            const auto type = asTexts(fasteners->column("type"));
            for (std::size_t row = 0; row < eid.size(); ++row)
                ends[eid[row]] = Ends{n1[row], n2[row], {ax[row], ay[row], az[row]}, type[row]};
        }
        if (ends.empty())
            return {0, "the model has no fastener of the kind asked for"};
        ResultModelIndex index(context);
        Outcome outcome;
        for (const TableRef& ref : tables)
        {
            const results::ResultTableMeta& meta = ref.table->meta();
            const std::shared_ptr<const results::ResultData> decoded = context.decodeRows(ref, request.elementIds);
            if (!decoded)
                continue;
            const TableValues values(*decoded);
            if (values.isComplex() || decoded->elementIds.empty())
                continue;
            int component[6];
            const char* const names[6] = {"f1", "f2", "f3", "m1", "m2", "m3"};
            bool complete = true;
            for (int number = 0; number < 6; ++number)
            {
                component[number] = values.component(names[number]);
                complete = complete && component[number] >= 0;
            }
            if (!complete)
                continue;
            const bool stepped =
                meta.stepKind == results::StepKind::Time || meta.stepKind == results::StepKind::Frequency ||
                meta.stepKind == results::StepKind::Mode || meta.stepKind == results::StepKind::LoadFactor;
            std::map<std::pair<std::int32_t, std::int32_t>, std::size_t> rowOfPair;
            for (std::size_t row = 0; row < values.rows(); ++row)
                rowOfPair[{decoded->elementIds[row], decoded->ids[row]}] = row;
            for (std::size_t stepIndex = 0; stepIndex < values.steps(); ++stepIndex)
            {
                std::map<std::string, ElementFlat> flats;
                std::map<std::string, std::vector<std::vector<double>>> reals;
                for (const auto& [element, fastener] : ends)
                {
                    if (!request.wantsElement(element) || std::isnan(fastener.axis[0]))
                        continue;
                    auto found = rowOfPair.find({element, fastener.n2});
                    double sign = -1.0;
                    if (found == rowOfPair.end())
                    {
                        found = rowOfPair.find({element, fastener.n1});
                        sign = 1.0;
                    }
                    if (found == rowOfPair.end())
                        continue;
                    const base::Vec3 axis{fastener.axis[0], fastener.axis[1], fastener.axis[2]};
                    const std::size_t row = found->second;
                    const base::Vec3 force{values.real(stepIndex, row, static_cast<std::size_t>(component[0])),
                                           values.real(stepIndex, row, static_cast<std::size_t>(component[1])),
                                           values.real(stepIndex, row, static_cast<std::size_t>(component[2]))};
                    const base::Vec3 moment{values.real(stepIndex, row, static_cast<std::size_t>(component[3])),
                                            values.real(stepIndex, row, static_cast<std::size_t>(component[4])),
                                            values.real(stepIndex, row, static_cast<std::size_t>(component[5]))};
                    const double along = sign * base::dot(force, axis);
                    const base::Vec3 across = force - base::dot(force, axis) * axis;
                    ElementFlat& flat = flats[fastener.card];
                    std::vector<std::vector<double>>& real = reals[fastener.card];
                    if (real.empty())
                        real.resize(FASTENER.size());
                    flat.eid.push_back(element);
                    flat.node.push_back(0);
                    flat.layer.push_back(0);
                    flat.step.push_back(stepped ? decoded->steps[stepIndex] : NOT_A_NUMBER);
                    if (!decoded->eigenvalues.empty())
                        flat.eigenvalue.push_back(decoded->eigenvalues[stepIndex]);
                    flat.position.push_back(NOT_A_NUMBER);
                    flat.location.push_back("center");
                    const double row7[7] = {
                        along,        NOT_A_NUMBER, NOT_A_NUMBER, base::norm(across), sign * base::dot(moment, axis),
                        NOT_A_NUMBER, NOT_A_NUMBER};
                    for (std::size_t field = 0; field < FASTENER.size(); ++field)
                        real[field].push_back(row7[field]);
                }
                for (auto& [card, flat] : flats)
                {
                    flat.set = ref.set;
                    flat.analysis = context.sets()[ref.set].analysis;
                    flat.status = std::string(results::resultStatusName(meta.status));
                    flat.subcase = meta.subcase;
                    flat.designCycle = meta.designCycle;
                    flat.stepKind = stepped ? meta.stepKind : results::StepKind::None;
                    flat.card = card;
                    flat.elementName = containerOfCard(card);
                    flat.frame = "basic";
                    flat.precision = precisionOf(*decoded);
                    for (std::size_t field = 0; field < FASTENER.size(); ++field)
                        flat.values[FASTENER[field]] = std::move(reals[card][field]);
                    outcome.rows += sink.add(std::move(flat));
                }
            }
        }
        if (outcome.rows == 0)
            outcome.reason =
                "no grid point force row of a fastener asked for (its element or its ends are not in the table)";
        return outcome;
    }
};

// The station and place of each row a table of bars or beams gives (StationForceTable), as the stored-table provider
// finds them.
void stationLabels(const results::ResultData& decoded, bool bar, std::vector<double>& station,
                   std::vector<std::uint8_t>& location)
{
    const std::size_t rows = decoded.rowCount;
    if (bar)
    {
        station.resize(2 * rows);
        location.resize(2 * rows);
        for (std::size_t row = 0; row < rows; ++row)
        {
            station[2 * row] = 0.0;
            station[2 * row + 1] = 1.0;
            location[2 * row] = 0;
            location[2 * row + 1] = 1;
        }
        return;
    }
    // a beam's first and last station and its last position, over the run of its rows (or over all its rows when the
    // table is not by increasing element), as the provider finds them
    station.assign(rows, NOT_A_NUMBER);
    location.assign(rows, 2);
    const auto& ids = decoded.ids;
    const auto& layers = decoded.layers;
    const auto& positions = decoded.positions;
    const auto stationsOf =
        [&](std::size_t first, std::size_t last, std::pair<std::int32_t, std::int32_t> range, double length)
    {
        for (std::size_t row = first; row < last; ++row)
        {
            location[row] = layers[row] == range.first ? 0 : (layers[row] == range.second ? 1 : 2);
            if (row < positions.size() && length > 0.0)
                station[row] = positions[row] / length;
        }
    };
    if (std::is_sorted(ids.begin(), ids.begin() + static_cast<std::ptrdiff_t>(rows)))
    {
        for (std::size_t first = 0; first < rows;)
        {
            std::size_t last = first;
            std::pair<std::int32_t, std::int32_t> range{layers[first], layers[first]};
            double length = 0.0;
            bool positioned = false;
            while (last < rows && ids[last] == ids[first])
            {
                range.first = std::min(range.first, layers[last]);
                range.second = std::max(range.second, layers[last]);
                if (last < positions.size())
                {
                    length = std::max(length, positions[last]);
                    positioned = true;
                }
                ++last;
            }
            stationsOf(first, last, range, positioned ? length : 0.0);
            first = last;
        }
        return;
    }
    std::map<std::int32_t, double> lastPosition;
    std::map<std::int32_t, std::pair<std::int32_t, std::int32_t>> ranges;
    for (std::size_t row = 0; row < rows; ++row)
    {
        if (row < positions.size())
            lastPosition[ids[row]] = std::max(lastPosition[ids[row]], positions[row]);
        auto [place, inserted] = ranges.emplace(ids[row], std::make_pair(layers[row], layers[row]));
        if (!inserted)
        {
            place->second.first = std::min(place->second.first, layers[row]);
            place->second.second = std::max(place->second.second, layers[row]);
        }
    }
    for (std::size_t row = 0; row < rows; ++row)
    {
        const auto found = lastPosition.find(ids[row]);
        stationsOf(row, row + 1, ranges.at(ids[row]), found != lastPosition.end() ? found->second : 0.0);
    }
}

// True when two decoded tables have the same row labels: the same elements, layers and positions in each row.
bool sameRowLabels(const results::ResultData& one, const results::ResultData& other)
{
    return one.ids.size() == other.ids.size() && std::equal(one.ids.begin(), one.ids.end(), other.ids.begin()) &&
           one.layers.size() == other.layers.size() &&
           std::equal(one.layers.begin(), one.layers.end(), other.layers.begin()) &&
           one.positions.size() == other.positions.size() &&
           std::memcmp(one.positions.data(), other.positions.data(), one.positions.size() * sizeof(double)) == 0;
}

} // namespace

const std::vector<std::string>& oneDimensionalFields()
{
    return ONE_DIMENSIONAL;
}

const std::vector<std::string>& fastenerFields()
{
    return FASTENER;
}

const ElementChain& oneDimensionalForceChain()
{
    static const ElementChain CHAIN = []
    {
        ElementChain chain("bar and rod forces");
        chain.add(std::make_unique<StoredForceTableProvider>(false));
        chain.add(std::make_unique<RodStressTimesAreaProvider>());
        return chain;
    }();
    return CHAIN;
}

const std::vector<std::string>& stationPlaceNames()
{
    static const std::vector<std::string> NAMES = {"endA", "endB", "station"};
    return NAMES;
}

std::optional<std::vector<StationForceTable>> stationForceTables(const ResultQueryContext& context,
                                                                 const ElementRequest& request, std::string& source)
{
    const ElementChain& chain = oneDimensionalForceChain();
    const QuantityInfo info = chain.info();
    if (ProviderSwitches::isDisabled(STATION_FORCE_SWITCH))
        return std::nullopt;
    for (const ProviderInfo& provider : info.providers)
        if (ProviderSwitches::isDisabled(provider.name))
            return std::nullopt;
    if (request.propertyIds || !request.location.empty() || request.layers || !request.axisName.empty() ||
        request.axisVector || !request.tableFrame.empty())
        return std::nullopt;
    // every element: none may be a rod whose force comes from its stress
    if (!request.elementIds)
        for (const std::string& name : context.elementNames(results::ResultKind::Stress))
            if (familyOfContainer(name) == ElementFamily::Rod && request.wantsType(cardOfContainer(name)))
                return std::nullopt;
    source = sourceText(info.providers.front().name, info.providers.front().approximate);
    // the tables the stored-table provider reads: every one a bar's or a beam's (a rod may come from its stress)
    std::vector<std::pair<TableRef, ForceShape>> tables;
    for (const std::string& name : context.elementNames(results::ResultKind::ElementForces))
    {
        const auto shape = shapeOf(name, false);
        if (!shape || !request.wantsType(cardOfContainer(name)))
            continue;
        if (*shape != ForceShape::Bar2 && *shape != ForceShape::Stations)
            return std::nullopt;
        for (const TableRef& ref : context.elementTables(results::ResultKind::ElementForces, name, false))
            tables.emplace_back(ref, *shape);
    }
    if (tables.empty())
        return std::nullopt;
    // the elements asked for, sorted, each once
    std::vector<std::int32_t> wanted;
    if (request.elementIds)
    {
        wanted.assign(request.elementIds->begin(), request.elementIds->end());
        if (!std::is_sorted(wanted.begin(), wanted.end()))
            std::sort(wanted.begin(), wanted.end());
        wanted.erase(std::unique(wanted.begin(), wanted.end()), wanted.end());
    }
    std::vector<StationForceTable> made(tables.size());
    std::atomic<bool> other{false};
    const auto make = [&](std::size_t at)
    {
        const TableRef& ref = tables[at].first;
        const ForceShape shape = tables[at].second;
        const results::ResultTableMeta& meta = ref.table->meta();
        StationForceTable& table = made[at];
        // a table in memory with no more rows than twice the elements asked for is taken whole (shared, not copied:
        // its rows of other elements have no place in the block) instead of selected (a copy of the IDs asked for
        // and a search of them for every table: what grew fastest with the elements)
        const bool whole =
            !request.elementIds || (ref.table->isDecoded() && meta.rowCountKnown && meta.rowCount <= 2 * wanted.size());
        table.decoded = context.decodeRows(ref, whole ? std::nullopt : request.elementIds);
        if (!table.decoded)
            return;
        const results::ResultData& decoded = *table.decoded;
        const TableValues values(decoded);
        if (values.isComplex())
        {
            other.store(true);
            return;
        }
        table.set = ref.set;
        table.analysis = context.sets()[ref.set].analysis;
        table.subcase = meta.subcase;
        table.designCycle = meta.designCycle;
        table.elementName = meta.elementName;
        table.stepped = meta.stepKind == results::StepKind::Time || meta.stepKind == results::StepKind::Frequency ||
                        meta.stepKind == results::StepKind::Mode || meta.stepKind == results::StepKind::LoadFactor;
        table.bar = shape == ForceShape::Bar2;
        // the components of the fields, as the provider takes them
        for (auto& end : table.source)
            end.fill(-1);
        const auto field = [](const char* name)
        {
            return static_cast<std::size_t>(std::find(ONE_DIMENSIONAL.begin(), ONE_DIMENSIONAL.end(), name) -
                                            ONE_DIMENSIONAL.begin());
        };
        if (table.bar)
            for (int end = 0; end < 2; ++end)
            {
                auto& to = table.source[static_cast<std::size_t>(end)];
                to[field("shear1")] = values.component("shear1");
                to[field("shear2")] = values.component("shear2");
                to[field("axial")] = values.component("axial");
                to[field("torque")] = values.component("torque");
                to[field("moment1")] = values.component(end == 0 ? "bendingMomentA1" : "bendingMomentB1");
                to[field("moment2")] = values.component(end == 0 ? "bendingMomentA2" : "bendingMomentB2");
            }
        else
        {
            auto& to = table.source[0];
            to[field("moment1")] = values.component("bendingMoment1");
            to[field("moment2")] = values.component("bendingMoment2");
            to[field("shear1")] = values.component("shear1");
            to[field("shear2")] = values.component("shear2");
            to[field("axial")] = firstOf(values, {"axial", "axialForce"});
            to[field("torque")] = firstOf(values, {"torque", "totalTorque"});
        }
        if (std::none_of(table.source[0].begin(), table.source[0].end(), [](int component) { return component >= 0; }))
        {
            table.decoded.reset(); // no row, as the provider gives none
            return;
        }
    };
    const results::DecodeContext decode = context.decode();
    if (decode.pool != nullptr && tables.size() > 1)
        base::parallelFor(*decode.pool, tables.size(), make, decode.token);
    else
        for (std::size_t at = 0; at < tables.size(); ++at)
            make(at);
    if (other.load())
        return std::nullopt;
    std::vector<StationForceTable> kept;
    kept.reserve(made.size());
    for (StationForceTable& table : made)
        if (table.decoded && table.decoded->rowCount > 0 && table.decoded->stepCount > 0)
            kept.push_back(std::move(table));
    // the labels of the rows, shared by a table with those of the one before (the subcases of one element type: a
    // station and a place a row, made once and not for every subcase)
    for (std::size_t index = 0; index < kept.size(); ++index)
    {
        StationForceTable& table = kept[index];
        if (index > 0 && kept[index - 1].bar == table.bar && sameRowLabels(*kept[index - 1].decoded, *table.decoded))
        {
            table.station = kept[index - 1].station;
            table.location = kept[index - 1].location;
            continue;
        }
        auto station = std::make_shared<std::vector<double>>();
        auto location = std::make_shared<std::vector<std::uint8_t>>();
        stationLabels(*table.decoded, table.bar, *station, *location);
        table.station = std::move(station);
        table.location = std::move(location);
    }
    if (kept.empty())
        return std::nullopt;
    if (!request.elementIds)
        return kept;
    // every element asked for has a row: else the chain's other ways and onMissing decide
    std::vector<std::uint8_t> present(wanted.size(), 0);
    std::size_t found = 0;
    const results::ResultData* before = nullptr;
    for (const StationForceTable& table : kept)
    {
        if (found == wanted.size())
            break;
        const auto& ids = table.decoded->ids;
        // the tables of the subcases of one element type mostly have the same elements: looked at once
        if (before != nullptr && before->ids.size() == ids.size() &&
            std::equal(ids.begin(), ids.end(), before->ids.begin()))
            continue;
        before = table.decoded.get();
        auto from = wanted.begin(); // the rows by increasing element: the search goes on from the last one
        for (std::size_t row = 0; row < ids.size(); ++row)
        {
            if (row > 0 && ids[row] == ids[row - 1])
                continue;
            if (row > 0 && ids[row] < ids[row - 1])
                from = wanted.begin();
            auto at = from;
            while (at != wanted.end() && *at < ids[row] && at - from < 8)
                ++at;
            if (at != wanted.end() && *at < ids[row])
                at = std::lower_bound(at, wanted.end(), ids[row]);
            from = at;
            if (at == wanted.end() || *at != ids[row])
                continue;
            std::uint8_t& seen = present[static_cast<std::size_t>(at - wanted.begin())];
            if (seen == 0)
            {
                seen = 1;
                ++found;
            }
        }
    }
    if (found != wanted.size())
        return std::nullopt;
    return kept;
}

const ElementChain& fastenerForceChain()
{
    static const ElementChain CHAIN = []
    {
        ElementChain chain("fastener forces");
        chain.add(std::make_unique<StoredForceTableProvider>(true));
        chain.add(std::make_unique<NodePointForcesOfElementProvider>());
        return chain;
    }();
    return CHAIN;
}

} // namespace oor::query
