#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/fields/FieldChecks.hpp"
#include "oor/model/cards/elements/pointMassInertia/PointMassInertiaTable.hpp"

#include <algorithm>

namespace oor::analysis
{

namespace
{

// A concentrated mass with no mass and no inertia: it adds nothing to the model.
class PointMassInertiaNoMassCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"field.point-mass2-no-mass", base::Severity::Warning, "fields",
                                    "A concentrated point mass has zero mass and zero inertia."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachCard<model::PointMassInertiaTable>(
            context.model(),
            [&](const model::PointMassInertiaCard& card, model::CardLocation where)
            {
                const bool noInertia = std::ranges::all_of(card.inertia, [](double value) { return value == 0.0; });
                if (card.mass == 0.0 && noInertia)
                    sink.reportCard(where, "mass", std::string("mass") + " and the inertias are all zero",
                                    "give the mass, or remove the card");
            });
    }
};

} // namespace

void addPointMassInertiaNoMassCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<PointMassInertiaNoMassCheck>());
}

} // namespace oor::analysis
