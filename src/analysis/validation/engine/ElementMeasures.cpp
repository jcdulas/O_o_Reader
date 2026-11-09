#include "oor/analysis/validation/engine/ElementMeasures.hpp"

#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/base/errors/Errors.hpp"

#include <algorithm>
#include <cmath>

namespace oor::analysis
{

namespace
{

constexpr std::size_t CHUNK = 4096;

enum class Kind
{
    None,
    Line, // rods, tubes, bars and beams
    Bar,  // bars and beams, which have an orientation
    Shell,
    Solid
};

// The kinds that are told by the card name: the rods, tubes, bars and beams.
Kind kindOf(const model::CardTypeInfo& info)
{
    const std::string_view name = info.name;
    if (name == "Bar2" || name == "Beam2")
        return Kind::Bar;
    if (name == "Rod2" || name == "Rod2InlineSection" || name == "Tube2")
        return Kind::Line;
    return Kind::None;
}

Kind kindOfType(std::uint16_t type)
{
    const Kind named = kindOf(model::CardRegistry::instance().entry(type).info);
    if (named != Kind::None)
        return named;
    if (model::CardRegistry::instance().entry(type).info.category != model::CardCategory::Element)
        return Kind::None;
    switch (dimensionOf(ElementTopology::shapeOf(type)))
    {
    case 2:
        return Kind::Shell;
    case 3:
        return Kind::Solid;
    default:
        return Kind::None;
    }
}

std::uint8_t quantitiesOf(Kind kind)
{
    switch (kind)
    {
    case Kind::Line:
        return bit(Quantity::Length);
    case Kind::Bar:
        return static_cast<std::uint8_t>(bit(Quantity::Length) | bit(Quantity::Frame));
    case Kind::Shell:
        return bit(Quantity::Area);
    case Kind::Solid:
        return bit(Quantity::Volume);
    case Kind::None:
        break;
    }
    return 0;
}

double nan()
{
    return std::numeric_limits<double>::quiet_NaN();
}

bool frameUndefined(const base::Mat3& frame)
{
    return !std::isfinite(frame.values[0]);
}

// Measures the rows [begin, end) of a table; the rows are live.
void measureChunk(const model::Model& model, std::uint16_t type, Kind kind, const std::vector<std::uint32_t>& rows,
                  std::vector<ElementMeasure>& out)
{
    const std::uint8_t quantities = quantitiesOf(kind);
    const auto store = [&](std::uint32_t row, double length, double area, double volume, bool undefinedOrientation)
    {
        ElementMeasure measure;
        measure.location = model::CardLocation{type, row};
        measure.length = kind == Kind::Line || kind == Kind::Bar ? length : nan();
        measure.area = kind == Kind::Shell ? area : nan();
        measure.volume = kind == Kind::Solid ? volume : nan();
        measure.orientationUndefined = undefinedOrientation;
        out.push_back(measure);
    };
    try
    {
        const ElementArrays arrays = ElementGeometry::compute(model, type, rows, quantities);
        for (std::size_t index = 0; index < rows.size(); ++index)
        {
            const double length = arrays.lengths.empty() ? nan() : arrays.lengths[index];
            const double area = arrays.areas.empty() ? nan() : arrays.areas[index];
            const double volume = arrays.volumes.empty() ? nan() : arrays.volumes[index];
            const bool undefinedOrientation = kind == Kind::Bar && std::isfinite(length) && length > 0.0 &&
                                              !arrays.frames.empty() && frameUndefined(arrays.frames[index]);
            store(rows[index], length, area, volume, undefinedOrientation);
        }
        return;
    }
    catch (const base::OorError&)
    {
        // A node that does not exist: measure the elements one by one and leave out the ones that cannot be.
    }
    for (const std::uint32_t row : rows)
    {
        try
        {
            const ElementValues values = ElementGeometry::of(model, model::CardLocation{type, row}, quantities);
            const bool undefinedOrientation = kind == Kind::Bar && std::isfinite(values.length) &&
                                              values.length > 0.0 && frameUndefined(values.frame);
            store(row, values.length, values.area, values.volume, undefinedOrientation);
        }
        catch (const base::OorError&)
        {
        }
    }
}

double modelScale(const model::Model& model)
{
    const BasicPositionCache& cache = BasicPositionCache::of(model);
    if (cache.positions().empty())
        return 0.0;
    std::array<double, 3> low = cache.positions().front();
    std::array<double, 3> high = low;
    for (const std::array<double, 3>& point : cache.positions())
    {
        for (std::size_t axis = 0; axis < 3; ++axis)
        {
            low[axis] = std::min(low[axis], point[axis]);
            high[axis] = std::max(high[axis], point[axis]);
        }
    }
    return std::sqrt((high[0] - low[0]) * (high[0] - low[0]) + (high[1] - low[1]) * (high[1] - low[1]) +
                     (high[2] - low[2]) * (high[2] - low[2]));
}

} // namespace

ElementMeasures ElementMeasures::build(const model::Model& model)
{
    ElementMeasures result;
    try
    {
        result.scale = modelScale(model);
    }
    catch (const base::OorError&)
    {
        return result; // the positions cannot be computed: the coordinate checks say why
    }
    const model::CardRegistry& registry = model::CardRegistry::instance();
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        const model::CardTable* table = model.findTable(type);
        if (table == nullptr || !ElementGeometry::supports(type))
            continue;
        const Kind kind = kindOfType(type);
        if (kind == Kind::None)
            continue;
        std::vector<std::uint32_t> rows;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            rows.push_back(row);
            if (rows.size() == CHUNK)
            {
                measureChunk(model, type, kind, rows, result.elements);
                rows.clear();
            }
        }
        if (!rows.empty())
            measureChunk(model, type, kind, rows, result.elements);
    }
    return result;
}

} // namespace oor::analysis
