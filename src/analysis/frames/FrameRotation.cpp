#include "oor/analysis/frames/FrameRotation.hpp"

#include "EntityFrames.hpp"
#include "oor/analysis/frames/FrameMath.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/base/threading/ParallelFor.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <utility>

namespace oor::analysis
{

using detail::EntityFrames;
using detail::FrameRole;
using results::StoredFrame;

namespace
{

constexpr double NAN_VALUE = std::numeric_limits<double>::quiet_NaN();
constexpr std::size_t ROWS_PER_CHUNK = 4096;

// The entries of a symmetric tensor, in the order of the plans: xx yy zz xy yz xz.
enum Entry : int
{
    XX = 0,
    YY = 1,
    ZZ = 2,
    XY = 3,
    YZ = 4,
    XZ = 5
};

enum class Kernel : std::uint8_t
{
    VectorPair, // two vectors of three components
    Tensor,     // a symmetric tensor, some entries of it
    ShellForce  // the in-plane resultants of a shell
};

// What a frame change does to the components of a table: which components it reads, and which it writes.
struct Plan
{
    Kernel kernel = Kernel::Tensor;
    std::array<int, 6> tensorIn = {-1, -1, -1, -1, -1, -1}; // the component each entry is read from (-1: zero)
    std::vector<int> tensorOut;                             // the entry written to each output component
    bool engineering = false;                               // the shears of the components are engineering shears
    std::array<int, 8> force = {-1, -1, -1, -1, -1, -1, -1, -1};
    std::array<int, 6> vector = {-1, -1, -1, -1, -1, -1};
    int keep = -1; // a component copied to the first output component (the fibre distance of a shell)
    std::vector<std::string> outNames;
    bool direction = false;
};

int positionOf(const std::vector<std::string>& names, std::string_view name) noexcept
{
    const auto found = std::find(names.begin(), names.end(), name);
    return found == names.end() ? -1 : static_cast<int>(found - names.begin());
}

[[noreturn]] void lacks(std::string_view name, const results::ResultTableMeta& meta)
{
    throw base::UsageError("the table of " + std::string(results::resultKindName(meta.kind)) + " has no component '" +
                           std::string(name) +
                           "', which the frame change needs (was it read with a components filter?)");
}

int require(const std::vector<std::string>& names, std::string_view name, const results::ResultTableMeta& meta)
{
    const int position = positionOf(names, name);
    if (position < 0)
        lacks(name, meta);
    return position;
}

std::string_view tensorEntryName(int entry, bool strain) noexcept
{
    static constexpr std::array<std::string_view, 6> STRESS = {"oxx", "oyy", "ozz", "txy", "tyz", "txz"};
    static constexpr std::array<std::string_view, 6> STRAIN = {"exx", "eyy", "ezz", "exy", "eyz", "exz"};
    return (strain ? STRAIN : STRESS)[static_cast<std::size_t>(entry)];
}

bool strainFamily(FrameFamily family) noexcept
{
    return family == FrameFamily::ShellStrain || family == FrameFamily::PlyStrain || family == FrameFamily::SolidStrain;
}

Plan makePlan(FrameFamily family, const results::ResultTableMeta& meta, FrameTargetKind target)
{
    Plan plan;
    plan.direction = target == FrameTargetKind::Direction;
    const std::vector<std::string>& names = meta.components;
    switch (family)
    {
    case FrameFamily::NodalVector:
    case FrameFamily::NodeForceBalance:
    {
        static constexpr std::array<std::string_view, 6> NODAL = {"t1", "t2", "t3", "r1", "r2", "r3"};
        static constexpr std::array<std::string_view, 6> FORCES = {"f1", "f2", "f3", "m1", "m2", "m3"};
        const auto& expected = family == FrameFamily::NodeForceBalance ? FORCES : NODAL;
        plan.kernel = Kernel::VectorPair;
        for (std::size_t index = 0; index < 6; ++index)
        {
            plan.vector[index] = require(names, expected[index], meta);
            plan.outNames.emplace_back(expected[index]);
        }
        if (plan.direction)
        {
            const bool motion =
                meta.kind == results::ResultKind::Displacements || meta.kind == results::ResultKind::Velocities ||
                meta.kind == results::ResultKind::Accelerations || meta.kind == results::ResultKind::Eigenvectors;
            plan.outNames = motion ? std::vector<std::string>{"translation", "rotation"}
                                   : std::vector<std::string>{"force", "moment"};
        }
        return plan;
    }
    case FrameFamily::ShellForce:
    {
        static constexpr std::array<std::string_view, 8> FORCES = {"nx", "ny", "nxy", "mx", "my", "mxy", "qx", "qy"};
        plan.kernel = Kernel::ShellForce;
        for (std::size_t index = 0; index < 8; ++index)
        {
            plan.force[index] = require(names, FORCES[index], meta);
            plan.outNames.emplace_back(FORCES[index]);
        }
        return plan;
    }
    case FrameFamily::ShellStress:
    case FrameFamily::ShellStrain:
    case FrameFamily::PlyStress:
    case FrameFamily::PlyStrain:
    case FrameFamily::SolidStress:
    case FrameFamily::SolidStrain:
    {
        const bool strain = strainFamily(family);
        const bool shell = family == FrameFamily::ShellStress || family == FrameFamily::ShellStrain;
        const bool ply = family == FrameFamily::PlyStress || family == FrameFamily::PlyStrain;
        plan.kernel = Kernel::Tensor;
        plan.engineering = strain;
        // the entries of the tensor and the names they have in the tables: those of the decoded tables of shells (oxx
        // oyy txy), plies (o11 o22 t12 t1z t2z) and solids, and those of the tables a frame change made
        static constexpr std::array<std::array<std::string_view, 2>, 6> STRESS_NAMES = {
            {{"oxx", "o11"}, {"oyy", "o22"}, {"ozz", ""}, {"txy", "t12"}, {"tyz", "t2z"}, {"txz", "t1z"}}};
        static constexpr std::array<std::array<std::string_view, 2>, 6> STRAIN_NAMES = {
            {{"exx", "e11"}, {"eyy", "e22"}, {"ezz", ""}, {"exy", "e12"}, {"eyz", "e2z"}, {"exz", "e1z"}}};
        const auto& entryNames = strain ? STRAIN_NAMES : STRESS_NAMES;
        for (int entry = XX; entry <= XZ; ++entry)
        {
            int position = -1;
            for (const std::string_view candidate : entryNames[static_cast<std::size_t>(entry)])
                if (!candidate.empty() && position < 0)
                    position = positionOf(names, candidate);
            const bool required = entry == XX || entry == YY || entry == XY || family == FrameFamily::SolidStress ||
                                  family == FrameFamily::SolidStrain;
            if (position < 0 && required)
                lacks(entryNames[static_cast<std::size_t>(entry)][0], meta);
            plan.tensorIn[static_cast<std::size_t>(entry)] = position;
        }
        if (plan.direction)
        {
            plan.outNames = {"normal"};
            return plan;
        }
        const bool inPlane = target == FrameTargetKind::Element || target == FrameTargetKind::Material;
        if (shell && inPlane)
        {
            plan.tensorOut = {XX, YY, XY};
        }
        else if (ply && (inPlane || target == FrameTargetKind::Ply))
        {
            plan.tensorOut = {XX, YY, XY, XZ, YZ};
        }
        else
        {
            plan.tensorOut = {XX, YY, ZZ, XY, YZ, XZ};
        }
        if (shell)
            plan.keep = positionOf(names, "fiberDistance") >= 0 ? positionOf(names, "fiberDistance")
                                                                : positionOf(names, "fiberCurvature");
        if (plan.keep >= 0)
            plan.outNames.push_back(names[static_cast<std::size_t>(plan.keep)]);
        if (ply && target == FrameTargetKind::Ply)
        {
            for (const std::string_view name : strain
                                                   ? std::array<std::string_view, 5>{"e11", "e22", "e12", "e1z", "e2z"}
                                                   : std::array<std::string_view, 5>{"o11", "o22", "t12", "t1z", "t2z"})
                plan.outNames.emplace_back(name);
        }
        else
        {
            for (const int entry : plan.tensorOut)
                plan.outNames.emplace_back(tensorEntryName(entry, strain));
        }
        return plan;
    }
    default:
        break;
    }
    throw base::UnsupportedFrameError("no frame change is defined for this table");
}

// ---- the arithmetic ------------------------------------------------------------------------------------------------

struct Sym3
{
    double v[6] = {0, 0, 0, 0, 0, 0}; // xx yy zz xy yz xz
};

// M S M^T for the symmetric S.
Sym3 turn(const base::Mat3& m, const Sym3& s) noexcept
{
    const double a[3][3] = {{s.v[XX], s.v[XY], s.v[XZ]}, {s.v[XY], s.v[YY], s.v[YZ]}, {s.v[XZ], s.v[YZ], s.v[ZZ]}};
    double t[3][3];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            t[i][j] = m(static_cast<std::size_t>(i), 0) * a[0][j] + m(static_cast<std::size_t>(i), 1) * a[1][j] +
                      m(static_cast<std::size_t>(i), 2) * a[2][j];
    const auto entry = [&](int i, int j)
    {
        return t[i][0] * m(static_cast<std::size_t>(j), 0) + t[i][1] * m(static_cast<std::size_t>(j), 1) +
               t[i][2] * m(static_cast<std::size_t>(j), 2);
    };
    Sym3 r;
    r.v[XX] = entry(0, 0);
    r.v[YY] = entry(1, 1);
    r.v[ZZ] = entry(2, 2);
    r.v[XY] = entry(0, 1);
    r.v[YZ] = entry(1, 2);
    r.v[XZ] = entry(0, 2);
    return r;
}

bool isFinite(const base::Mat3& m) noexcept
{
    return std::all_of(m.values.begin(), m.values.end(), [](double value) { return std::isfinite(value); });
}

struct Run
{
    const Plan* plan = nullptr;
    const EntityFrames* frames = nullptr;
    bool hasTarget = false;
    const std::vector<base::Vec3>* directions = nullptr; // Direction targets
    std::size_t steps = 0;
    std::size_t rows = 0;
    std::size_t inComps = 0;
    std::size_t outComps = 0;
    int mult = 1;
};

// One row of one step, one part (real or imaginary) of its values. `m` is the matrix of the change (or, for a
// direction, the rotation of the stored frame to the basic system); `ok` says the row has a frame.
template <typename Real>
void applyRow(const Run& run, const Real* in, Real* out, const base::Mat3& m, bool ok, const base::Vec3& d, int part)
{
    const Plan& plan = *run.plan;
    const std::size_t mult = static_cast<std::size_t>(run.mult);
    const auto load = [&](int component)
    { return static_cast<double>(in[static_cast<std::size_t>(component) * mult + static_cast<std::size_t>(part)]); };
    const auto store = [&](std::size_t component, double value)
    { out[component * mult + static_cast<std::size_t>(part)] = static_cast<Real>(value); };
    const std::size_t outCount = plan.outNames.size();
    if (!ok)
    {
        for (std::size_t component = 0; component < outCount; ++component)
            store(component, NAN_VALUE);
        return;
    }
    switch (plan.kernel)
    {
    case Kernel::VectorPair:
    {
        const base::Vec3 a{load(plan.vector[0]), load(plan.vector[1]), load(plan.vector[2])};
        const base::Vec3 b{load(plan.vector[3]), load(plan.vector[4]), load(plan.vector[5])};
        if (plan.direction)
        {
            store(0, dot(m * a, d));
            store(1, dot(m * b, d));
            return;
        }
        const base::Vec3 ta = m * a;
        const base::Vec3 tb = m * b;
        store(0, ta.x);
        store(1, ta.y);
        store(2, ta.z);
        store(3, tb.x);
        store(4, tb.y);
        store(5, tb.z);
        return;
    }
    case Kernel::ShellForce:
    {
        // the in-plane turn: the top left 2 x 2 block of the matrix
        const double c00 = m(0, 0);
        const double c01 = m(0, 1);
        const double c10 = m(1, 0);
        const double c11 = m(1, 1);
        const auto at = [&](std::size_t index) { return load(plan.force[index]); };
        const auto tensor = [&](std::size_t first)
        {
            const double xx = at(first);
            const double yy = at(first + 1);
            const double xy = at(first + 2);
            const double nxx = c00 * c00 * xx + 2.0 * c00 * c01 * xy + c01 * c01 * yy;
            const double nyy = c10 * c10 * xx + 2.0 * c10 * c11 * xy + c11 * c11 * yy;
            const double nxy = c00 * c10 * xx + (c00 * c11 + c01 * c10) * xy + c01 * c11 * yy;
            store(first, nxx);
            store(first + 1, nyy);
            store(first + 2, nxy);
        };
        tensor(0);
        tensor(3);
        const double qx = at(6);
        const double qy = at(7);
        store(6, c00 * qx + c01 * qy);
        store(7, c10 * qx + c11 * qy);
        return;
    }
    case Kernel::Tensor:
    {
        Sym3 s;
        for (int entry = XX; entry <= XZ; ++entry)
        {
            const int component = plan.tensorIn[static_cast<std::size_t>(entry)];
            double value = component >= 0 ? load(component) : 0.0;
            if (plan.engineering && entry >= XY)
                value *= 0.5;
            s.v[entry] = value;
        }
        if (plan.direction)
        {
            // d^T (R S R^T) d with v = R^T d: v^T S v
            const base::Vec3 v = m.transposed() * d;
            const double normal = v.x * v.x * s.v[XX] + v.y * v.y * s.v[YY] + v.z * v.z * s.v[ZZ] +
                                  2.0 * (v.x * v.y * s.v[XY] + v.y * v.z * s.v[YZ] + v.x * v.z * s.v[XZ]);
            store(0, normal);
            return;
        }
        const Sym3 t = turn(m, s);
        std::size_t component = 0;
        if (plan.keep >= 0)
            store(component++, load(plan.keep));
        for (const int entry : plan.tensorOut)
        {
            double value = t.v[entry];
            if (plan.engineering && entry >= XY)
                value *= 2.0;
            store(component++, value);
        }
        return;
    }
    }
}

template <typename Real>
void runKernel(const Run& run, const results::ResultData& in, results::ResultData& out, base::ThreadPool* pool,
               const base::CancellationToken* token)
{
    const Real* source = reinterpret_cast<const Real*>(in.storage.get());
    Real* target = reinterpret_cast<Real*>(out.storage.get());
    const std::size_t mult = static_cast<std::size_t>(run.mult);
    const std::vector<base::IndexRange> chunks = base::splitIntoChunks(run.rows, ROWS_PER_CHUNK);
    const bool direction = run.plan->direction;
    const auto body = [&](std::size_t chunkIndex)
    {
        const base::IndexRange chunk = chunks[chunkIndex];
        const std::size_t count = chunk.end - chunk.begin;
        std::vector<base::Mat3> matrices(count);
        std::vector<base::Vec3> directions(count);
        std::vector<std::uint8_t> ok(count, 0);
        for (std::size_t local = 0; local < count; ++local)
        {
            const std::size_t row = chunk.begin + local;
            const std::uint32_t entity = run.frames->entityOfRow[row];
            // a scalar point, or a target that is the stored frame: the identity, exactly
            const bool same = run.hasTarget && !run.frames->sameFrame.empty() && run.frames->sameFrame[entity] != 0;
            if ((run.frames->passThrough[entity] != 0 || same) && !direction)
            {
                matrices[local] = base::Mat3::identity();
                ok[local] = 1;
                continue;
            }
            const base::Mat3 stored = run.frames->storedOfRow(row);
            base::Mat3 matrix = stored;
            if (!direction && run.hasTarget)
                matrix = run.frames->targetOfRow(row).transposed() * stored;
            ok[local] = isFinite(matrix) ? 1 : 0;
            if (direction)
            {
                const base::Vec3 raw = run.directions->size() == 1 ? (*run.directions)[0] : (*run.directions)[row];
                const double length = base::norm(raw);
                directions[local] =
                    length > 0.0 && std::isfinite(length) ? raw / length : base::Vec3{NAN_VALUE, NAN_VALUE, NAN_VALUE};
                if (run.frames->passThrough[entity] != 0 || !std::isfinite(length) || !(length > 0.0))
                    ok[local] = 0;
            }
            matrices[local] = matrix;
        }
        for (std::size_t step = 0; step < run.steps; ++step)
        {
            for (std::size_t local = 0; local < count; ++local)
            {
                const std::size_t row = chunk.begin + local;
                const Real* rowIn = source + (step * run.rows + row) * run.inComps * mult;
                Real* rowOut = target + (step * run.rows + row) * run.outComps * mult;
                for (int part = 0; part < run.mult; ++part)
                    applyRow<Real>(run, rowIn, rowOut, matrices[local], ok[local] != 0, directions[local], part);
            }
        }
    };
    if (pool != nullptr && chunks.size() > 1)
    {
        base::parallelFor(*pool, chunks.size(), body, token);
    }
    else
    {
        for (std::size_t chunk = 0; chunk < chunks.size(); ++chunk)
            body(chunk);
    }
}

std::string describeTarget(const TargetFrame& target)
{
    switch (target.kind)
    {
    case FrameTargetKind::Coordinate:
        return "inFrame(coordinateSystem " + std::to_string(target.system) + ")";
    case FrameTargetKind::Cylindrical:
        return "inFrame(cylindrical about " + std::to_string(target.system) + ")";
    case FrameTargetKind::Spherical:
        return "inFrame(spherical about " + std::to_string(target.system) + ")";
    case FrameTargetKind::Direction:
        return "along(direction)";
    default:
        return "inFrame(" + std::string(frameTargetName(target.kind)) + ")";
    }
}

std::optional<FrameRole> roleOf(const TargetFrame& target)
{
    switch (target.kind)
    {
    case FrameTargetKind::Basic:
        return FrameRole{StoredFrame::Basic, 0};
    case FrameTargetKind::NodeOutput:
        return FrameRole{StoredFrame::NodeOutputSystem, 0};
    case FrameTargetKind::Element:
        return FrameRole{StoredFrame::Element, 0};
    case FrameTargetKind::Material:
        return FrameRole{StoredFrame::Material, 0};
    case FrameTargetKind::Ply:
        return FrameRole{StoredFrame::Ply, 0};
    case FrameTargetKind::Coordinate:
        return FrameRole{StoredFrame::Coordinate, target.system};
    case FrameTargetKind::Cylindrical:
        return FrameRole{StoredFrame::Cylindrical, target.system};
    case FrameTargetKind::Spherical:
        return FrameRole{StoredFrame::Spherical, target.system};
    case FrameTargetKind::Direction:
    case FrameTargetKind::Any:
        break;
    }
    return std::nullopt;
}

StoredFrame storedFrameOfTarget(FrameTargetKind kind) noexcept
{
    switch (kind)
    {
    case FrameTargetKind::NodeOutput:
        return StoredFrame::NodeOutputSystem;
    case FrameTargetKind::Element:
        return StoredFrame::Element;
    case FrameTargetKind::Material:
        return StoredFrame::Material;
    case FrameTargetKind::Ply:
        return StoredFrame::Ply;
    case FrameTargetKind::Coordinate:
        return StoredFrame::Coordinate;
    case FrameTargetKind::Cylindrical:
        return StoredFrame::Cylindrical;
    case FrameTargetKind::Spherical:
        return StoredFrame::Spherical;
    default:
        return StoredFrame::Basic;
    }
}

// Refuses a table that cannot take a frame change: the stored frame is not known, or the table was itself made by an
// operation, or the pair is not validated.
void requireOffered(const results::ResultTable& table, FrameFamily family, const StoredFrameInfo& stored,
                    FrameTargetKind target)
{
    const results::ResultTableMeta& meta = table.meta();
    const std::string what = std::string(frameFamilyName(family)) + " (" +
                             std::string(results::resultKindName(meta.kind)) +
                             (meta.elementName.empty() ? "" : "." + meta.elementName) + ")";
    if (!FrameConventionTable::hasFrameChange(family))
        throw base::UnsupportedFrameError(
            "cannot change the frame of " + what + ": " +
            FrameConventionTable::find(family, StoredFrame::Unknown, FrameTargetKind::Any).reason);
    if (stored.kind == StoredFrame::Unknown)
        throw base::UnsupportedFrameError("cannot change the frame of " + what +
                                          ": the frame the values are stored in is not known: " + stored.reason);
    // the pairs of a family do not depend on which frame the values are in, only on the kind of values and the target;
    // a table that a frame change made can be in a frame that no table of the file is in
    const std::vector<StoredFrame> offered = FrameConventionTable::storedFramesOf(family);
    const StoredFrame lookup =
        std::find(offered.begin(), offered.end(), stored.kind) != offered.end() ? stored.kind : offered.front();
    const ConventionRow row = FrameConventionTable::find(family, lookup, target);
    if (row.status == ConventionStatus::Refused)
        throw base::UnsupportedFrameError("cannot change the frame of " + what + " from " +
                                          std::string(results::storedFrameName(stored.kind)) + " to " +
                                          std::string(frameTargetName(target)) + ": " + row.reason);
}

// The source of a table turned when it is read (FrameRotation::rotateWhenRead): the rows a read asks for are selected
// in the table first, then turned. The steps are selected the same way (`stepsInData`: the selection reaches `load`).
class TurnedWhenRead final : public results::ResultTableSource
{
public:
    TurnedWhenRead(std::shared_ptr<const model::Model> model,
                   std::shared_ptr<const results::ResultAttachment> attachment,
                   std::shared_ptr<results::ResultTable> table, TargetFrame target, std::vector<std::string> names,
                   std::shared_ptr<FrameCache> cache)
        : m_model(std::move(model)),
          m_attachment(std::move(attachment)),
          m_table(std::move(table)),
          m_target(std::move(target)),
          m_names(std::move(names)),
          m_cache(std::move(cache))
    {
    }

    [[nodiscard]] std::shared_ptr<const results::ResultData> load(const results::LoadRequest& request,
                                                                  const results::DecodeContext& context) const override
    {
        results::ResultFilter rows = request.filter;
        rows.components.reset(); // the frame change needs the components it reads; the others are dropped after
        const std::shared_ptr<results::ResultTable> source =
            rows.hasRowSelection() || rows.hasStepSelection() ? m_table->select(rows) : m_table;
        std::shared_ptr<const results::ResultData> turned =
            FrameRotation::rotate(*m_model, *m_attachment, *source, m_target, context.pool, context, m_cache.get())
                ->data(context);
        if (request.components != m_names)
        {
            results::ResultFilter kept;
            kept.components = request.components;
            turned = results::subsetResultData(*turned, kept, m_table->meta().stepKind);
        }
        return turned;
    }
    [[nodiscard]] bool stepsInData() const noexcept override { return true; }
    [[nodiscard]] std::shared_ptr<const results::ResultTableSource> withSteps(
        std::span<const std::uint32_t>) const override
    {
        return nullptr;
    }

private:
    std::shared_ptr<const model::Model> m_model;
    std::shared_ptr<const results::ResultAttachment> m_attachment;
    std::shared_ptr<results::ResultTable> m_table;
    TargetFrame m_target;
    std::vector<std::string> m_names;
    std::shared_ptr<FrameCache> m_cache;
};

} // namespace

std::vector<base::Mat3> FrameRotation::storedMatrices(const model::Model& model,
                                                      const results::ResultAttachment& attachment,
                                                      const results::ResultTable& table, base::ThreadPool* pool,
                                                      const results::DecodeContext& context)
{
    StoredFrames::requireCurrent(model, attachment, pool);
    const FrameFamily family = FrameConventionTable::familyOf(table.meta());
    const StoredFrameInfo stored = StoredFrames::describe(model, attachment, table, context);
    if (!FrameConventionTable::hasFrameChange(family) || stored.kind == StoredFrame::Unknown)
        throw base::UnsupportedFrameError("the frame of the values of this table is not known" +
                                          (stored.reason.empty() ? std::string() : ": " + stored.reason));
    const std::shared_ptr<const results::ResultData> data = table.data(context);
    const EntityFrames frames =
        detail::buildEntityFrames(model, attachment, table, *data, family, stored, std::nullopt, pool);
    std::vector<base::Mat3> result(data->rowCount);
    for (std::size_t row = 0; row < result.size(); ++row)
        result[row] =
            frames.passThrough[frames.entityOfRow[row]] != 0 ? base::Mat3::identity() : frames.storedOfRow(row);
    return result;
}

std::shared_ptr<results::ResultTable> FrameRotation::rotate(const model::Model& model,
                                                            const results::ResultAttachment& attachment,
                                                            const results::ResultTable& table,
                                                            const TargetFrame& target, base::ThreadPool* pool,
                                                            const results::DecodeContext& context, FrameCache* cache)
{
    StoredFrames::requireCurrent(model, attachment, pool);
    const results::ResultTableMeta& meta = table.meta();
    const FrameFamily family = FrameConventionTable::familyOf(meta);
    const StoredFrameInfo stored = StoredFrames::describe(model, attachment, table, context);
    requireOffered(table, family, stored, target.kind);

    const std::shared_ptr<const results::ResultData> data = table.data(context);
    const Plan plan = [&]
    {
        try
        {
            return makePlan(family, meta, target.kind);
        }
        catch (const base::UsageError& error)
        {
            throw base::UnsupportedFrameError(error.what());
        }
    }();
    if (plan.direction)
    {
        if (target.directions.empty() || (target.directions.size() != 1 && target.directions.size() != data->rowCount))
            throw base::UsageError("along takes one direction, or one for each of the " +
                                   std::to_string(data->rowCount) + " rows of the table (got " +
                                   std::to_string(target.directions.size()) + ")");
    }
    const std::optional<FrameRole> role = roleOf(target);
    const bool turns = true;
    // the change gives the components of the table: an identity change keeps its values (below), the rows not mapped
    const bool sameComponents = plan.outNames == data->components;
    const EntityFrames frames = detail::buildEntityFrames(model, attachment, table, *data, family, stored,
                                                          turns && !plan.direction ? role : std::nullopt, pool,
                                                          plan.direction ? nullptr : cache, !sameComponents);

    results::ResultTableMeta derived = meta;
    derived.derivation = describeTarget(target);
    derived.storedFrame = storedFrameOfTarget(target.kind);
    derived.frameSystem = target.kind == FrameTargetKind::Coordinate || target.kind == FrameTargetKind::Cylindrical ||
                                  target.kind == FrameTargetKind::Spherical
                              ? target.system
                              : 0;
    derived.frameReason.clear();
    // the target is the stored frame of every row (a material frame that is the element frame, a companion package),
    // and the change gives the components of the table: its values as they are, not turned by the identity
    if (turns && !plan.direction && frames.targetIsStored && sameComponents)
    {
        auto result = std::make_shared<results::ResultTable>(std::move(derived), data);
        if (const std::shared_ptr<results::ResultAttachment> owner = table.attachment())
            result->setAttachment(owner);
        return result;
    }

    auto out = std::make_shared<results::ResultData>();
    out->type = data->type;
    out->stepCount = data->stepCount;
    out->rowCount = data->rowCount;
    out->componentCount = plan.outNames.size();
    out->steps = data->steps;
    out->eigenvalues = data->eigenvalues;
    out->eigenvaluesImag = data->eigenvaluesImag;
    out->cycles = data->cycles;
    out->ids = data->ids;
    out->pointTypes = data->pointTypes;
    out->nodeIds = data->nodeIds;
    out->elementIds = data->elementIds;
    out->layers = data->layers;
    out->positions = data->positions;
    out->texts = data->texts;
    out->components = plan.outNames;
    out->allocate();

    Run run;
    run.plan = &plan;
    run.frames = &frames;
    run.hasTarget = turns && !plan.direction;
    run.directions = &target.directions;
    run.steps = data->stepCount;
    run.rows = data->rowCount;
    run.inComps = data->componentCount;
    run.outComps = plan.outNames.size();
    run.mult = results::isComplexType(data->type) ? 2 : 1;
    const bool single = data->type == results::ValueType::Float32 || data->type == results::ValueType::Complex64;
    if (single)
        runKernel<float>(run, *data, *out, pool, context.token);
    else
        runKernel<double>(run, *data, *out, pool, context.token);

    auto result =
        std::make_shared<results::ResultTable>(std::move(derived), std::shared_ptr<const results::ResultData>(out));
    if (const std::shared_ptr<results::ResultAttachment> owner = table.attachment())
        result->setAttachment(owner);
    return result;
}

std::shared_ptr<results::ResultTable> FrameRotation::rotateWhenRead(
    std::shared_ptr<const model::Model> model, std::shared_ptr<const results::ResultAttachment> attachment,
    std::shared_ptr<results::ResultTable> table, const TargetFrame& target, const results::DecodeContext& context,
    std::shared_ptr<FrameCache> cache)
{
    StoredFrames::requireCurrent(*model, *attachment, context.pool);
    const results::ResultTableMeta& meta = table->meta();
    const FrameFamily family = FrameConventionTable::familyOf(meta);
    const StoredFrameInfo stored = StoredFrames::describe(*model, *attachment, *table, context);
    requireOffered(*table, family, stored, target.kind);
    if (target.kind == FrameTargetKind::Direction) // a direction for each row needs the rows as they are
        return rotate(*model, *attachment, *table, target, context.pool, context);
    const Plan plan = [&]
    {
        try
        {
            return makePlan(family, meta, target.kind);
        }
        catch (const base::UsageError& error)
        {
            throw base::UnsupportedFrameError(error.what());
        }
    }();
    // the frame the values are stored in, and the components a change would give are those of the table (the forces of
    // a shell): nothing to turn. A stress gets its components of the frame (its tensor and invariants) even so.
    if (const std::optional<FrameRole> role = roleOf(target);
        role && stored.kind == role->kind && stored.system == role->system && plan.outNames == meta.components)
        return table;
    results::ResultTableMeta derived = meta;
    derived.components = plan.outNames;
    derived.derivation = describeTarget(target);
    derived.storedFrame = storedFrameOfTarget(target.kind);
    derived.frameSystem = target.kind == FrameTargetKind::Coordinate || target.kind == FrameTargetKind::Cylindrical ||
                                  target.kind == FrameTargetKind::Spherical
                              ? target.system
                              : 0;
    derived.frameReason.clear();
    const std::shared_ptr<results::ResultAttachment> owner = table->attachment();
    auto result = std::make_shared<results::ResultTable>(
        std::move(derived), std::make_shared<TurnedWhenRead>(std::move(model), std::move(attachment), std::move(table),
                                                             target, plan.outNames, std::move(cache)));
    if (owner)
        result->setAttachment(owner);
    return result;
}

} // namespace oor::analysis
