#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/SharedIds.hpp"

namespace oor::analysis
{

namespace
{

// The splines of every kind share one number space: the format reference says the EID of each SPLINEi must be unique
// with respect to all the SPLINEi (...). Two cards of one type with the same ID are the reader's business
// (`ref.duplicate-id`); this check is for the ID that two kinds of spline have.
class SplineIdRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.spline-id-repeated", base::Severity::Error, "aero",
                                    "A spline of one kind has the ID of a spline of another kind."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportIdsSharedByTypes(context, sink, model::CardCategory::AeroSpline, "spline");
    }
};

} // namespace

void addSplineIdRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SplineIdRepeatedCheck>());
}

} // namespace oor::analysis
