#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/aeroGust/AeroGustTable.hpp"
#include "oor/model/cards/aero/aeroUnsteadyParams/AeroUnsteadyParamsTable.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>

namespace oor::analysis
{

namespace
{

// The velocity of a GUST must be the VELOCITY of the AERO card: the
// gust is a function of the position along the vehicle and of the speed, and the aerodynamic data are made for one
// speed. A deck with no AERO velocity (it is optional) is not judged. A warning, not an error: the solver does not
// refuse two speeds apart (the response is the engineer's to judge).
class GustVelocityCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.gust-velocity", base::Severity::Warning, "aero",
                                    "The velocity of a gust is not the velocity of the unsteady aerodynamic parameters "
                                    "card."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        std::optional<double> velocity;
        forEachCard<model::AeroUnsteadyParamsTable>(context.model(),
                                                    [&](const model::AeroUnsteadyParamsCard& card, model::CardLocation)
                                                    {
                                                        if (!velocity && card.velocity)
                                                            velocity = card.velocity;
                                                    });
        if (!velocity)
            return;
        forEachCard<model::AeroGustTable>(
            context.model(),
            [&](const model::AeroGustCard& card, model::CardLocation where)
            {
                if (std::abs(card.velocity - *velocity) >
                    1.0e-9 * std::max(std::abs(card.velocity), std::abs(*velocity)))
                {
                    sink.reportCard(where, "velocity",
                                    std::string("velocity") + " is " + std::to_string(card.velocity) + " and the " +
                                        std::string("velocity") + " of the " + std::string("AeroUnsteadyParams") +
                                        " card is " + std::to_string(*velocity),
                                    "give the same speed to both");
                }
            });
    }
};

} // namespace

void addGustVelocityCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<GustVelocityCheck>());
}

} // namespace oor::analysis
