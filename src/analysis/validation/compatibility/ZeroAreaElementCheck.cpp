#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"

#include <cmath>

namespace oor::analysis
{

namespace
{

// A shell whose corners are on a line or coincide: no area, no normal, no stiffness.
class ZeroAreaElementCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.zero-area-element", base::Severity::Error, "compatibility",
                                    "A shell element has zero area (collapsed or collinear nodes)."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const ElementMeasures& measures = context.measures();
        const double limit = 1.0e-12 * measures.scale * measures.scale;
        for (const ElementMeasure& measure : measures.elements)
        {
            if (std::isfinite(measure.area) && measure.area <= limit)
                sink.reportCard(measure.location, "", "the nodes give an area of " + std::to_string(measure.area),
                                "check the grid points of the element");
        }
    }
};

} // namespace

void addZeroAreaElementCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ZeroAreaElementCheck>());
}

} // namespace oor::analysis
