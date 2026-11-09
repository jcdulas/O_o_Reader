#include "oor/analysis/quality/MeshQuality.hpp"

namespace oor::analysis
{

QualityArrays MeshQuality::compute(const model::Model& model, std::uint16_t typeIndex,
                                   std::span<const std::uint32_t> rows, base::ThreadPool* pool)
{
    if (ShellQuality::supports(typeIndex))
        return ShellQuality::compute(model, typeIndex, rows, pool);
    return SolidQuality::compute(model, typeIndex, rows, pool);
}

QualityValues MeshQuality::of(const model::Model& model, model::CardLocation location)
{
    if (ShellQuality::supports(location.table))
        return ShellQuality::of(model, location);
    return SolidQuality::of(model, location);
}

} // namespace oor::analysis
