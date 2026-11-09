#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/loads/LoadChecks.hpp"
#include "oor/model/cards/loads/gravityLoad/GravityLoadTable.hpp"

namespace oor::analysis
{

namespace
{

// A GRAV card with no acceleration: the scale A is zero (the direction vector is refused when it is zero).
class ZeroGravityCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"load.zero-gravity", base::Severity::Warning, "loads",
                                    "A gravity load has an acceleration of zero."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::GravityLoadTable>(
            model,
            [&](const model::GravityLoadCard& card, model::CardLocation where)
            {
                if (card.magnitude == 0.0)
                    sink.reportCard(where, "magnitude", "the acceleration " + std::string("magnitude") + " is zero",
                                    "give the acceleration, or remove the card");
            });
    }
};

} // namespace

void addZeroGravityCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ZeroGravityCheck>());
}

} // namespace oor::analysis
