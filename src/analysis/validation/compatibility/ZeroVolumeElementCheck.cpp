#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"

#include <cmath>

namespace oor::analysis
{

namespace
{

// A solid whose corners are in a plane or coincide: no volume.
class ZeroVolumeElementCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.zero-volume-element", base::Severity::Error, "compatibility",
                                    "A solid element has zero volume (flat or collapsed)."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const ElementMeasures& measures = context.measures();
        const double limit = 1.0e-15 * measures.scale * measures.scale * measures.scale;
        for (const ElementMeasure& measure : measures.elements)
        {
            if (std::isfinite(measure.volume) && measure.volume <= limit)
                sink.reportCard(measure.location, "", "the nodes give a volume of " + std::to_string(measure.volume),
                                "check the grid points of the element");
        }
    }
};

} // namespace

void addZeroVolumeElementCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ZeroVolumeElementCheck>());
}

} // namespace oor::analysis
