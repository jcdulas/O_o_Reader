#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"

#include <cmath>

namespace oor::analysis
{

namespace
{

// A rod, tube, bar or beam whose two end points are the same point: no length, no axis, no stiffness.
class ZeroLengthElementCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.zero-length-element", base::Severity::Error, "compatibility",
                                    "A rod, tube, bar or beam element has zero length."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const ElementMeasures& measures = context.measures();
        const double limit = 1.0e-9 * measures.scale;
        for (const ElementMeasure& measure : measures.elements)
        {
            if (std::isfinite(measure.length) && measure.length <= limit)
                sink.reportCard(measure.location, "",
                                "the two end points coincide (length " + std::to_string(measure.length) + ")",
                                "move one of the grid points, or remove the element");
        }
    }
};

} // namespace

void addZeroLengthElementCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ZeroLengthElementCheck>());
}

} // namespace oor::analysis
