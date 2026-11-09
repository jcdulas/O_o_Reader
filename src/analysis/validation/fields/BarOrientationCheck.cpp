#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/fields/FieldChecks.hpp"

namespace oor::analysis
{

namespace
{

// The orientation vector of a CBAR or CBEAM (X1, X2, X3, or the line from GA to G0) that is zero or parallel to the
// axis GA-GB: the element system, which the section axes follow, is then undefined.
class BarOrientationCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"field.bar-orientation-degenerate", base::Severity::Error, "fields",
                                    "The orientation vector of a bar or beam element is zero or parallel to its axis."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        for (const ElementMeasure& measure : context.measures().elements)
        {
            if (measure.orientationUndefined)
                sink.reportCard(measure.location, "orientation1",
                                "the orientation vector is zero or parallel to the axis of the element",
                                "give an orientation vector (or a " + std::string("orientationNode") +
                                    " node) that is not along the axis");
        }
    }
};

} // namespace

void addBarOrientationCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<BarOrientationCheck>());
}

} // namespace oor::analysis
