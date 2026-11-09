#include "oor/analysis/derived/DerivedQuantities.hpp"

#include "oor/analysis/frames/FrameConventions.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/base/threading/ParallelFor.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <numbers>
#include <optional>
#include <unordered_map>

namespace oor::analysis
{

namespace
{

constexpr double NAN_VALUE = std::numeric_limits<double>::quiet_NaN();
constexpr std::size_t ROWS_PER_CHUNK = 8192;

enum Entry : int
{
    XX = 0,
    YY = 1,
    ZZ = 2,
    XY = 3,
    YZ = 4,
    XZ = 5
};

// What a stress or strain table holds, for the arithmetic.
struct TensorTable
{
    bool strain = false;
    bool solid = false;                               // six components, else the in-plane three
    std::array<int, 6> in = {-1, -1, -1, -1, -1, -1}; // the component of each entry (-1: not in the table)
};

int positionOf(const std::vector<std::string>& names, std::string_view name) noexcept
{
    const auto found = std::find(names.begin(), names.end(), name);
    return found == names.end() ? -1 : static_cast<int>(found - names.begin());
}

bool tensorFamily(FrameFamily family) noexcept
{
    return family == FrameFamily::ShellStress || family == FrameFamily::ShellStrain ||
           family == FrameFamily::PlyStress || family == FrameFamily::PlyStrain || family == FrameFamily::SolidStress ||
           family == FrameFamily::SolidStrain;
}

TensorTable tensorTableOf(const results::ResultTableMeta& meta)
{
    const FrameFamily family = FrameConventionTable::familyOf(meta);
    if (!tensorFamily(family))
        throw base::UsageError(
            "derived quantities are made from the components of a stress or strain table of a shell, a "
            "laminate or a solid: this table holds " +
            std::string(results::resultKindName(meta.kind)) +
            (meta.elementName.empty() ? "" : " of " + meta.elementName));
    TensorTable table;
    table.strain =
        family == FrameFamily::ShellStrain || family == FrameFamily::PlyStrain || family == FrameFamily::SolidStrain;
    // the names of the entries in the tables of shells, plies and solids (and in the tables a frame change made)
    static constexpr std::array<std::array<std::string_view, 2>, 6> STRESS_NAMES = {
        {{"oxx", "o11"}, {"oyy", "o22"}, {"ozz", ""}, {"txy", "t12"}, {"tyz", "t2z"}, {"txz", "t1z"}}};
    static constexpr std::array<std::array<std::string_view, 2>, 6> STRAIN_NAMES = {
        {{"exx", "e11"}, {"eyy", "e22"}, {"ezz", ""}, {"exy", "e12"}, {"eyz", "e2z"}, {"exz", "e1z"}}};
    const auto& names = table.strain ? STRAIN_NAMES : STRESS_NAMES;
    for (int entry = XX; entry <= XZ; ++entry)
        for (const std::string_view candidate : names[static_cast<std::size_t>(entry)])
            if (!candidate.empty() && table.in[static_cast<std::size_t>(entry)] < 0)
                table.in[static_cast<std::size_t>(entry)] = positionOf(meta.components, candidate);
    const bool solidFamily = family == FrameFamily::SolidStress || family == FrameFamily::SolidStrain;
    for (const int entry : {XX, YY, XY})
        if (table.in[static_cast<std::size_t>(entry)] < 0)
            throw base::UsageError("the table has no component '" +
                                   std::string(names[static_cast<std::size_t>(entry)][0]) +
                                   "', which the derived quantities are made from");
    // the six components make a solid: a table of a shell turned out of its plane has them too
    const bool sixComponents = table.in[ZZ] >= 0 && table.in[YZ] >= 0 && table.in[XZ] >= 0;
    if (solidFamily && !sixComponents)
        throw base::UsageError("the table has not the six components of a solid stress or strain");
    table.solid = sixComponents;
    return table;
}

std::vector<std::string> namesOf(const TensorTable& table, DerivedKind kind)
{
    switch (kind)
    {
    case DerivedKind::VonMises:
        return {"vonMises"};
    case DerivedKind::Principal:
        if (table.solid)
            return table.strain ? std::vector<std::string>{"e1", "e2", "e3"}
                                : std::vector<std::string>{"o1", "o2", "o3"};
        return {"major", "minor", "angle"};
    case DerivedKind::MaxShear:
        return {"maxShear"};
    case DerivedKind::OctahedralShear:
        return {"octahedralShear"};
    }
    return {};
}

// The eigenvalues of a symmetric 3 x 3 matrix, decreasing (the closed form by the trigonometric solution).
std::array<double, 3> eigenvalues(const double s[6]) noexcept
{
    const double a00 = s[XX];
    const double a11 = s[YY];
    const double a22 = s[ZZ];
    const double a01 = s[XY];
    const double a12 = s[YZ];
    const double a02 = s[XZ];
    const double offDiagonal = a01 * a01 + a12 * a12 + a02 * a02;
    if (offDiagonal == 0.0)
    {
        std::array<double, 3> values = {a00, a11, a22};
        std::sort(values.begin(), values.end(), std::greater<>());
        return values;
    }
    const double q = (a00 + a11 + a22) / 3.0;
    const double p2 = (a00 - q) * (a00 - q) + (a11 - q) * (a11 - q) + (a22 - q) * (a22 - q) + 2.0 * offDiagonal;
    const double p = std::sqrt(p2 / 6.0);
    const double b00 = (a00 - q) / p;
    const double b11 = (a11 - q) / p;
    const double b22 = (a22 - q) / p;
    const double b01 = a01 / p;
    const double b12 = a12 / p;
    const double b02 = a02 / p;
    const double determinant =
        b00 * (b11 * b22 - b12 * b12) - b01 * (b01 * b22 - b12 * b02) + b02 * (b01 * b12 - b11 * b02);
    const double r = std::clamp(determinant / 2.0, -1.0, 1.0);
    const double phi = std::acos(r) / 3.0;
    const double largest = q + 2.0 * p * std::cos(phi);
    const double smallest = q + 2.0 * p * std::cos(phi + 2.0 * std::numbers::pi / 3.0);
    return {largest, 3.0 * q - largest - smallest, smallest};
}

// One row of one step: the derived values of the tensor `s` (xx yy zz xy yz xz, the shears as tensor shears).
void deriveRow(const TensorTable& layout, DerivedKind kind, const double s[6], double* out)
{
    const double factor = layout.strain ? 2.0 / 3.0 : 1.0; // the equivalent strain is 2/3 of the stress form
    switch (kind)
    {
    case DerivedKind::VonMises:
        if (layout.solid)
            out[0] = factor * std::sqrt(0.5 * ((s[XX] - s[YY]) * (s[XX] - s[YY]) + (s[YY] - s[ZZ]) * (s[YY] - s[ZZ]) +
                                               (s[ZZ] - s[XX]) * (s[ZZ] - s[XX])) +
                                        3.0 * (s[XY] * s[XY] + s[YZ] * s[YZ] + s[XZ] * s[XZ]));
        else
            out[0] = factor * std::sqrt(s[XX] * s[XX] - s[XX] * s[YY] + s[YY] * s[YY] + 3.0 * s[XY] * s[XY]);
        return;
    case DerivedKind::Principal:
    case DerivedKind::MaxShear:
        if (layout.solid)
        {
            const std::array<double, 3> values = eigenvalues(s);
            if (kind == DerivedKind::Principal)
            {
                out[0] = values[0];
                out[1] = values[1];
                out[2] = values[2];
            }
            else
            {
                out[0] = (layout.strain ? 1.0 : 0.5) * (values[0] - values[2]);
            }
            return;
        }
        else
        {
            const double centre = 0.5 * (s[XX] + s[YY]);
            const double radius = std::sqrt(0.25 * (s[XX] - s[YY]) * (s[XX] - s[YY]) + s[XY] * s[XY]);
            if (kind == DerivedKind::Principal)
            {
                out[0] = centre + radius;
                out[1] = centre - radius;
                out[2] = 0.5 * std::atan2(2.0 * s[XY], s[XX] - s[YY]) * (180.0 / std::numbers::pi);
            }
            else
            {
                out[0] = (layout.strain ? 2.0 : 1.0) * radius;
            }
            return;
        }
    case DerivedKind::OctahedralShear:
    {
        const double sum = (s[XX] - s[YY]) * (s[XX] - s[YY]) + (s[YY] - s[ZZ]) * (s[YY] - s[ZZ]) +
                           (s[ZZ] - s[XX]) * (s[ZZ] - s[XX]) + 6.0 * (s[XY] * s[XY] + s[YZ] * s[YZ] + s[XZ] * s[XZ]);
        out[0] = std::sqrt(sum) / 3.0; // the same for a strain (its shears as tensor shears): the solver's tables
        return;
    }
    }
}

// An empty block with the labels of `data` and these components.
std::shared_ptr<results::ResultData> sameLabels(const results::ResultData& data, std::vector<std::string> components,
                                                results::ValueType type)
{
    auto out = std::make_shared<results::ResultData>();
    out->type = type;
    out->stepCount = data.stepCount;
    out->rowCount = data.rowCount;
    out->componentCount = components.size();
    out->steps = data.steps;
    out->eigenvalues = data.eigenvalues;
    out->eigenvaluesImag = data.eigenvaluesImag;
    out->cycles = data.cycles;
    out->ids = data.ids;
    out->pointTypes = data.pointTypes;
    out->nodeIds = data.nodeIds;
    out->elementIds = data.elementIds;
    out->layers = data.layers;
    out->positions = data.positions;
    out->texts = data.texts;
    out->components = std::move(components);
    out->allocate();
    return out;
}

bool isSingle(results::ValueType type) noexcept
{
    return type == results::ValueType::Float32 || type == results::ValueType::Complex64;
}

std::shared_ptr<results::ResultTable> derivedTable(const results::ResultTable& source, std::string derivation,
                                                   std::shared_ptr<const results::ResultData> data)
{
    results::ResultTableMeta meta = source.meta();
    meta.derivation = std::move(derivation);
    auto table = std::make_shared<results::ResultTable>(std::move(meta), std::move(data));
    if (const std::shared_ptr<results::ResultAttachment> owner = source.attachment())
        table->setAttachment(owner);
    return table;
}

void requireReal(const results::ResultTableMeta& meta, std::string_view what)
{
    if (results::isComplexType(meta.valueType))
        throw base::UsageError(std::string(what) +
                               " needs real values: a complex table has no such quantity; take `atPhase(degrees)` or "
                               "`magnitude()` first");
}

template <typename Real>
void deriveAll(const TensorTable& layout, DerivedKind kind, const results::ResultData& in, results::ResultData& out,
               base::ThreadPool* pool)
{
    const Real* source = in.values<Real>().data();
    Real* target = out.mutableValues<Real>().data();
    const std::size_t outCount = out.componentCount;
    const std::size_t total = in.stepCount * in.rowCount;
    const std::vector<base::IndexRange> chunks = base::splitIntoChunks(total, ROWS_PER_CHUNK);
    const auto body = [&](std::size_t chunkIndex)
    {
        double s[6];
        double result[3];
        for (std::size_t item = chunks[chunkIndex].begin; item < chunks[chunkIndex].end; ++item)
        {
            const Real* row = source + item * in.componentCount;
            for (int entry = XX; entry <= XZ; ++entry)
            {
                const int component = layout.in[static_cast<std::size_t>(entry)];
                double value = component >= 0 ? static_cast<double>(row[component]) : 0.0;
                if (layout.strain && entry >= XY)
                    value *= 0.5; // the tensor shear of an engineering shear
                s[entry] = value;
            }
            deriveRow(layout, kind, s, result);
            for (std::size_t component = 0; component < outCount; ++component)
                target[item * outCount + component] = static_cast<Real>(result[component]);
        }
    };
    if (pool != nullptr && chunks.size() > 1)
        base::parallelFor(*pool, chunks.size(), body);
    else
        for (std::size_t chunk = 0; chunk < chunks.size(); ++chunk)
            body(chunk);
}

} // namespace

TensorScalars DerivedQuantities::scalarsOf(bool strain, bool solid, const double components[6])
{
    TensorTable layout;
    layout.strain = strain;
    layout.solid = solid;
    double s[6];
    for (int entry = XX; entry <= XZ; ++entry)
        s[entry] = strain && entry >= XY ? 0.5 * components[entry] : components[entry]; // a tensor shear
    if (!solid)
        s[ZZ] = s[YZ] = s[XZ] = 0.0;
    TensorScalars out;
    double value[3] = {0.0, 0.0, 0.0};
    deriveRow(layout, DerivedKind::VonMises, s, value);
    out.vonMises = value[0];
    deriveRow(layout, DerivedKind::Principal, s, value);
    out.first = value[0];
    out.second = value[1];
    out.third = solid ? value[2] : NAN_VALUE;
    out.angle = solid ? NAN_VALUE : value[2];
    deriveRow(layout, DerivedKind::MaxShear, s, value);
    out.maxShear = value[0];
    if (solid)
    {
        deriveRow(layout, DerivedKind::OctahedralShear, s, value);
        out.octahedralShear = value[0];
    }
    else
    {
        out.octahedralShear = NAN_VALUE;
    }
    return out;
}

void DerivedQuantities::vonMisesOf(bool strain, bool solid, const TensorColumns& columns, std::size_t begin,
                                   std::size_t end, double* out) noexcept
{
    TensorTable layout;
    layout.strain = strain;
    layout.solid = solid;
    const auto at = [&columns](int entry, std::size_t row)
    {
        const auto index = static_cast<std::size_t>(entry);
        return columns.data[index][static_cast<std::ptrdiff_t>(row) * columns.stride[index]];
    };
    // as `scalarsOf`: the tensor shears of a strain, the out-of-plane entries of a plane tensor 0
    for (std::size_t row = begin; row < end; ++row)
    {
        double s[6] = {at(XX, row), at(YY, row), 0.0, 0.0, 0.0, 0.0};
        s[XY] = strain ? 0.5 * at(XY, row) : at(XY, row);
        if (solid)
        {
            s[ZZ] = at(ZZ, row);
            s[YZ] = strain ? 0.5 * at(YZ, row) : at(YZ, row);
            s[XZ] = strain ? 0.5 * at(XZ, row) : at(XZ, row);
        }
        double value[3] = {0.0, 0.0, 0.0};
        deriveRow(layout, DerivedKind::VonMises, s, value);
        out[row - begin] = value[0];
    }
}

std::string_view derivedKindName(DerivedKind kind) noexcept
{
    switch (kind)
    {
    case DerivedKind::VonMises:
        return "vonMises";
    case DerivedKind::Principal:
        return "principal";
    case DerivedKind::MaxShear:
        return "maxShear";
    case DerivedKind::OctahedralShear:
        return "octahedralShear";
    }
    return "vonMises";
}

bool DerivedQuantities::isTensorTable(const results::ResultTableMeta& meta) noexcept
{
    try
    {
        (void)tensorTableOf(meta);
        return true;
    }
    catch (const base::OorError&)
    {
        return false;
    }
}

std::shared_ptr<results::ResultTable> DerivedQuantities::compute(const results::ResultTable& table, DerivedKind kind,
                                                                 base::ThreadPool* pool,
                                                                 const results::DecodeContext& context)
{
    const results::ResultTableMeta& meta = table.meta();
    requireReal(meta, derivedKindName(kind));
    const TensorTable layout = tensorTableOf(meta);
    if (kind == DerivedKind::OctahedralShear && !layout.solid)
        throw base::UsageError("the octahedral shear is a quantity of solids: a shell or a ply has maxShear");
    const std::shared_ptr<const results::ResultData> data = table.data(context);
    std::shared_ptr<results::ResultData> out = sameLabels(*data, namesOf(layout, kind), data->type);
    if (isSingle(data->type))
        deriveAll<float>(layout, kind, *data, *out, pool);
    else
        deriveAll<double>(layout, kind, *data, *out, pool);
    return derivedTable(table, std::string(derivedKindName(kind)), out);
}

// ---------------------------------------------------------------------------------------------- the surface and layers

std::vector<std::int32_t> DerivedQuantities::layersOf(const results::ResultTable& table,
                                                      const results::DecodeContext& context)
{
    std::vector<std::int32_t> layers = table.data(context)->layers.toVector();
    std::sort(layers.begin(), layers.end());
    layers.erase(std::unique(layers.begin(), layers.end()), layers.end());
    return layers;
}

std::vector<std::pair<std::int32_t, std::shared_ptr<results::ResultTable>>> DerivedQuantities::byLayer(
    const results::ResultTable& table, const results::DecodeContext& context)
{
    std::vector<std::pair<std::int32_t, std::shared_ptr<results::ResultTable>>> result;
    for (const std::int32_t layer : layersOf(table, context))
    {
        results::ResultFilter filter;
        filter.layers = std::vector<std::int32_t>{layer};
        result.emplace_back(layer, table.select(filter));
    }
    return result;
}

std::shared_ptr<results::ResultTable> DerivedQuantities::surface(const results::ResultTable& table, Surface which,
                                                                 const results::DecodeContext& context)
{
    const results::ResultTableMeta& meta = table.meta();
    const FrameFamily family = FrameConventionTable::familyOf(meta);
    if (family != FrameFamily::ShellStress && family != FrameFamily::ShellStrain)
        throw base::UsageError("surface selects the fibres of a shell (Z1 and Z2): this table holds " +
                               std::string(results::resultKindName(meta.kind)) +
                               (meta.elementName.empty() ? "" : " of " + meta.elementName) +
                               "; the plies of a laminate are selected with by_layer");
    if (which != Surface::Middle)
    {
        results::ResultFilter filter;
        filter.layers = std::vector<std::int32_t>{which == Surface::Z1 ? 1 : 2};
        return table.select(filter);
    }
    requireReal(meta, "the middle surface");
    const TensorTable layout = tensorTableOf(meta);
    const std::shared_ptr<const results::ResultData> data = table.data(context);
    // the rows of each place (element, grid point): its two fibres
    std::unordered_map<std::uint64_t, std::array<std::int64_t, 2>> places;
    std::vector<std::uint64_t> order;
    for (std::size_t row = 0; row < data->rowCount; ++row)
    {
        const std::uint32_t node = data->nodeIds.empty() ? 0U : static_cast<std::uint32_t>(data->nodeIds[row]);
        const std::uint64_t key =
            (static_cast<std::uint64_t>(static_cast<std::uint32_t>(data->ids[row])) << 32U) | node;
        auto found = places.find(key);
        if (found == places.end())
        {
            found = places.emplace(key, std::array<std::int64_t, 2>{-1, -1}).first;
            order.push_back(key);
        }
        const std::int32_t layer = data->layers[row];
        if (layer == 1 || layer == 2)
            found->second[static_cast<std::size_t>(layer) - 1] = static_cast<std::int64_t>(row);
    }
    std::vector<std::uint64_t> kept;
    for (const std::uint64_t key : order)
        if (places[key][0] >= 0 && places[key][1] >= 0)
            kept.push_back(key);
    if (kept.empty())
        throw base::EmptySelectionError("the table has no place with both fibres: there is no middle surface");
    const std::vector<std::string> names = {layout.strain ? "exx" : "oxx", layout.strain ? "eyy" : "oyy",
                                            layout.strain ? "exy" : "txy"};
    auto out = std::make_shared<results::ResultData>();
    out->type = data->type;
    out->stepCount = data->stepCount;
    out->rowCount = kept.size();
    out->componentCount = 3;
    out->steps = data->steps;
    out->eigenvalues = data->eigenvalues;
    out->eigenvaluesImag = data->eigenvaluesImag;
    out->cycles = data->cycles;
    out->components = names;
    for (const std::uint64_t key : kept)
    {
        const auto rows = places[key];
        const auto first = static_cast<std::size_t>(rows[0]);
        out->ids.push_back(data->ids[first]);
        if (!data->nodeIds.empty())
            out->nodeIds.push_back(data->nodeIds[first]);
        out->layers.push_back(0);
    }
    out->allocate();
    const auto average = [&](auto sample)
    {
        using Real = decltype(sample);
        const Real* source = data->values<Real>().data();
        Real* target = out->mutableValues<Real>().data();
        for (std::size_t step = 0; step < data->stepCount; ++step)
            for (std::size_t index = 0; index < kept.size(); ++index)
            {
                const auto rows = places[kept[index]];
                for (std::size_t component = 0; component < 3; ++component)
                {
                    const std::size_t source0 = layout.in[component == 2 ? static_cast<std::size_t>(XY) : component];
                    const Real first =
                        source[(step * data->rowCount + static_cast<std::size_t>(rows[0])) * data->componentCount +
                               source0];
                    const Real second =
                        source[(step * data->rowCount + static_cast<std::size_t>(rows[1])) * data->componentCount +
                               source0];
                    target[(step * out->rowCount + index) * 3 + component] =
                        static_cast<Real>(0.5 * (static_cast<double>(first) + static_cast<double>(second)));
                }
            }
    };
    if (isSingle(data->type))
        average(float{});
    else
        average(double{});
    return derivedTable(table, "surface(middle)", out);
}

// ------------------------------------------------------------------------------------------ extremes over layers,
// steps

std::shared_ptr<results::ResultTable> DerivedQuantities::maxOverLayers(const results::ResultTable& table,
                                                                       const std::string& quantity,
                                                                       base::ThreadPool* pool,
                                                                       const results::DecodeContext& context)
{
    const results::ResultTableMeta& meta = table.meta();
    requireReal(meta, "maxOverLayers");
    // the column of values: a component of the table, else a derived quantity
    std::shared_ptr<const results::ResultData> values = table.data(context);
    std::size_t column = 0;
    const int stored = positionOf(meta.components, quantity);
    if (stored >= 0)
    {
        column = static_cast<std::size_t>(stored);
    }
    else
    {
        const TensorTable layout = tensorTableOf(meta);
        struct Choice
        {
            DerivedKind kind;
            std::size_t position;
        };
        std::optional<Choice> choice;
        if (quantity == "vonMises")
            choice = Choice{DerivedKind::VonMises, 0};
        else if (quantity == "maxShear")
            choice = Choice{DerivedKind::MaxShear, 0};
        else if (quantity == "octahedralShear")
            choice = Choice{DerivedKind::OctahedralShear, 0};
        else
            for (const DerivedKind kind : {DerivedKind::Principal})
            {
                const std::vector<std::string> names = namesOf(layout, kind);
                const auto found = std::find(names.begin(), names.end(), quantity);
                if (found != names.end())
                    choice = Choice{kind, static_cast<std::size_t>(found - names.begin())};
            }
        if (!choice)
            throw base::UsageError("'" + quantity +
                                   "' is neither a component of the table nor a derived quantity (vonMises, maxShear, "
                                   "octahedralShear, major, minor, angle, o1, o2, o3)");
        values = compute(table, choice->kind, pool, context)->data(context);
        column = choice->position;
    }
    const results::ResultData& data = *values;
    if (data.layers.empty())
        throw base::UsageError("the table has no layers: its rows are not fibres or plies");
    std::unordered_map<std::uint64_t, std::size_t> places;
    std::vector<std::size_t> placeOfRow(data.rowCount);
    std::vector<std::size_t> firstRowOfPlace;
    for (std::size_t row = 0; row < data.rowCount; ++row)
    {
        const std::uint32_t node = data.nodeIds.empty() ? 0U : static_cast<std::uint32_t>(data.nodeIds[row]);
        const std::uint64_t key = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(data.ids[row])) << 32U) | node;
        const auto [found, inserted] = places.emplace(key, firstRowOfPlace.size());
        if (inserted)
            firstRowOfPlace.push_back(row);
        placeOfRow[row] = found->second;
    }
    const std::size_t placeCount = firstRowOfPlace.size();
    auto out = std::make_shared<results::ResultData>();
    out->type = data.type;
    out->stepCount = data.stepCount;
    out->rowCount = placeCount;
    out->componentCount = 2;
    out->steps = data.steps;
    out->eigenvalues = data.eigenvalues;
    out->eigenvaluesImag = data.eigenvaluesImag;
    out->cycles = data.cycles;
    out->components = {quantity, "layer"};
    for (const std::size_t row : firstRowOfPlace)
    {
        out->ids.push_back(data.ids[row]);
        if (!data.nodeIds.empty())
            out->nodeIds.push_back(data.nodeIds[row]);
        if (!data.positions.empty())
            out->positions.push_back(data.positions[row]);
        out->layers.push_back(0);
    }
    out->allocate();
    const auto reduce = [&](auto sample)
    {
        using Real = decltype(sample);
        const Real* source = data.values<Real>().data();
        Real* target = out->mutableValues<Real>().data();
        for (std::size_t step = 0; step < data.stepCount; ++step)
        {
            std::vector<double> best(placeCount, -std::numeric_limits<double>::infinity());
            std::vector<std::int32_t> layer(placeCount, 0);
            std::vector<std::uint8_t> any(placeCount, 0);
            for (std::size_t row = 0; row < data.rowCount; ++row)
            {
                const double value =
                    static_cast<double>(source[(step * data.rowCount + row) * data.componentCount + column]);
                const std::size_t place = placeOfRow[row];
                if (std::isnan(value))
                    continue;
                if (any[place] == 0 || value > best[place])
                {
                    best[place] = value;
                    layer[place] = data.layers[row];
                    any[place] = 1;
                }
            }
            for (std::size_t place = 0; place < placeCount; ++place)
            {
                target[(step * placeCount + place) * 2] = static_cast<Real>(any[place] != 0 ? best[place] : NAN_VALUE);
                target[(step * placeCount + place) * 2 + 1] =
                    static_cast<Real>(any[place] != 0 ? static_cast<double>(layer[place]) : NAN_VALUE);
            }
        }
    };
    if (isSingle(data.type))
        reduce(float{});
    else
        reduce(double{});
    return derivedTable(table, "maxOverLayers(" + quantity + ")", out);
}

std::shared_ptr<results::ResultTable> DerivedQuantities::maxMin(const results::ResultTable& table,
                                                                const std::vector<std::string>& components,
                                                                const results::DecodeContext& context)
{
    const results::ResultTableMeta& meta = table.meta();
    requireReal(meta, "maxMin");
    const std::shared_ptr<const results::ResultData> data = table.data(context);
    std::vector<std::string> wanted = components;
    if (wanted.empty())
        wanted = data->components;
    std::vector<std::size_t> columns;
    for (const std::string& name : wanted)
    {
        const int position = positionOf(data->components, name);
        if (position < 0)
            throw base::UsageError("the table has no component '" + name + "'");
        columns.push_back(static_cast<std::size_t>(position));
    }
    auto out = std::make_shared<results::ResultData>();
    out->type = data->type;
    out->stepCount = 1;
    out->rowCount = data->rowCount;
    out->componentCount = wanted.size() * 4;
    out->steps = {NAN_VALUE};
    out->ids = data->ids;
    out->pointTypes = data->pointTypes;
    out->nodeIds = data->nodeIds;
    out->elementIds = data->elementIds;
    out->layers = data->layers;
    out->positions = data->positions;
    out->texts = data->texts;
    for (const std::string& name : wanted)
        for (const char* suffix : {"Max", "MaxStep", "Min", "MinStep"})
            out->components.push_back(name + suffix);
    out->allocate();
    const auto reduce = [&](auto sample)
    {
        using Real = decltype(sample);
        const Real* source = data->values<Real>().data();
        Real* target = out->mutableValues<Real>().data();
        for (std::size_t row = 0; row < data->rowCount; ++row)
            for (std::size_t index = 0; index < columns.size(); ++index)
            {
                double maximum = NAN_VALUE;
                double minimum = NAN_VALUE;
                double maximumStep = NAN_VALUE;
                double minimumStep = NAN_VALUE;
                for (std::size_t step = 0; step < data->stepCount; ++step)
                {
                    const double value = static_cast<double>(
                        source[(step * data->rowCount + row) * data->componentCount + columns[index]]);
                    if (std::isnan(value))
                        continue;
                    if (std::isnan(maximum) || value > maximum)
                    {
                        maximum = value;
                        maximumStep = data->steps[step];
                    }
                    if (std::isnan(minimum) || value < minimum)
                    {
                        minimum = value;
                        minimumStep = data->steps[step];
                    }
                }
                Real* slot = target + (row * columns.size() + index) * 4;
                slot[0] = static_cast<Real>(maximum);
                slot[1] = static_cast<Real>(maximumStep);
                slot[2] = static_cast<Real>(minimum);
                slot[3] = static_cast<Real>(minimumStep);
            }
    };
    if (isSingle(data->type))
        reduce(float{});
    else
        reduce(double{});
    results::ResultTableMeta derived = meta;
    derived.stepKind = results::StepKind::None;
    derived.derivation = "maxMin";
    auto result =
        std::make_shared<results::ResultTable>(std::move(derived), std::shared_ptr<const results::ResultData>(out));
    if (const std::shared_ptr<results::ResultAttachment> owner = table.attachment())
        result->setAttachment(owner);
    return result;
}

// ------------------------------------------------------------------------------------------------------ complex values

namespace
{

std::shared_ptr<results::ResultTable> realFromComplex(const results::ResultTable& table,
                                                      const results::DecodeContext& context, std::string derivation,
                                                      double cosine, double sine, bool magnitude)
{
    const results::ResultTableMeta& meta = table.meta();
    if (!results::isComplexType(meta.valueType))
        throw base::UsageError("the table holds real values already");
    const std::shared_ptr<const results::ResultData> data = table.data(context);
    const bool single = data->type == results::ValueType::Complex64;
    std::shared_ptr<results::ResultData> out =
        sameLabels(*data, data->components, single ? results::ValueType::Float32 : results::ValueType::Float64);
    const auto convert = [&](auto sample)
    {
        using Real = decltype(sample);
        const Real* source = data->values<Real>().data();
        Real* target = out->mutableValues<Real>().data();
        const std::size_t count = data->valueCount();
        for (std::size_t index = 0; index < count; ++index)
        {
            const double re = static_cast<double>(source[2 * index]);
            const double im = static_cast<double>(source[2 * index + 1]);
            target[index] = static_cast<Real>(magnitude ? std::hypot(re, im) : re * cosine - im * sine);
        }
    };
    if (single)
        convert(float{});
    else
        convert(double{});
    results::ResultTableMeta derived = meta;
    derived.valueKind = results::ValueKind::Real;
    derived.valueType = out->type;
    derived.storedFormat = results::ResultFormat::Real;
    derived.derivation = std::move(derivation);
    auto result =
        std::make_shared<results::ResultTable>(std::move(derived), std::shared_ptr<const results::ResultData>(out));
    if (const std::shared_ptr<results::ResultAttachment> owner = table.attachment())
        result->setAttachment(owner);
    return result;
}

} // namespace

std::shared_ptr<results::ResultTable> DerivedQuantities::atPhase(const results::ResultTable& table,
                                                                 double phaseInDegrees,
                                                                 const results::DecodeContext& context)
{
    const double radians = phaseInDegrees * (std::numbers::pi / 180.0);
    return realFromComplex(table, context, "atPhase(" + std::to_string(phaseInDegrees) + ")", std::cos(radians),
                           std::sin(radians), false);
}

std::shared_ptr<results::ResultTable> DerivedQuantities::magnitude(const results::ResultTable& table,
                                                                   const results::DecodeContext& context)
{
    return realFromComplex(table, context, "magnitude", 1.0, 0.0, true);
}

} // namespace oor::analysis
