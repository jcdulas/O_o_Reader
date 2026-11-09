#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/loads/LoadChecks.hpp"
#include "oor/model/cards/loads/facePressureLoad/FacePressureLoadTable.hpp"
#include "oor/model/cards/loads/pointForceLoad/PointForceLoadTable.hpp"
#include "oor/model/cards/loads/pointForceLoad1/PointForceLoad1Table.hpp"
#include "oor/model/cards/loads/pointForceLoad2/PointForceLoad2Table.hpp"
#include "oor/model/cards/loads/pointMomentLoad/PointMomentLoadTable.hpp"
#include "oor/model/cards/loads/pointMomentLoad1/PointMomentLoad1Table.hpp"
#include "oor/model/cards/loads/pointMomentLoad2/PointMomentLoad2Table.hpp"
#include "oor/model/cards/loads/pointsPressureLoad/PointsPressureLoadTable.hpp"
#include "oor/model/cards/loads/scalarPointLoad/ScalarPointLoadTable.hpp"
#include "oor/model/cards/loads/shellPressureLoad/ShellPressureLoadTable.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

namespace oor::analysis
{

namespace
{

bool allZero(const std::array<double, 3>& vector)
{
    return vector[0] == 0.0 && vector[1] == 0.0 && vector[2] == 0.0;
}

// "the magnitude F is zero", with the keyword of the magnitude field of the card type in the dictionary of the model.
std::string zeroMagnitude(const model::Model&, std::string_view)
{
    return "the magnitude " + std::string("magnitude") + " is zero";
}

// "the direction (N1, N2, N3) is the zero vector", with the keywords of the direction fields of the card type.
std::string zeroDirection(const model::Model&, std::string_view)
{
    return "the direction (" + std::string("directionX") + ", " + std::string("directionY") + ", " +
           std::string("directionZ") + ") is the zero vector";
}

// A load whose value is zero (or whose direction is the zero vector): it is in the deck and does nothing, which is
// usually a blank field that was meant to be filled.
class ZeroMagnitudeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"load.zero-magnitude", base::Severity::Warning, "loads",
                                    "A point force, point moment, pressure or scalar point load has a magnitude of "
                                    "zero."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const auto zero = [&](model::CardLocation where, const char* field, std::string why)
        { sink.reportCard(where, field, std::move(why), "give the load a value, or remove the card"); };
        forEachCard<model::PointForceLoadTable>(
            model,
            [&](const model::PointForceLoadCard& card, model::CardLocation where)
            {
                if (card.magnitude == 0.0)
                    zero(where, "magnitude", zeroMagnitude(model, "PointForceLoad"));
                else if (allZero(card.direction))
                    zero(where, "directionX", zeroDirection(model, "PointForceLoad"));
            });
        forEachCard<model::PointMomentLoadTable>(
            model,
            [&](const model::PointMomentLoadCard& card, model::CardLocation where)
            {
                if (card.magnitude == 0.0)
                    zero(where, "magnitude", zeroMagnitude(model, "PointMomentLoad"));
                else if (allZero(card.direction))
                    zero(where, "directionX", zeroDirection(model, "PointMomentLoad"));
            });
        forEachCard<model::PointForceLoad1Table>(model,
                                                 [&](const model::PointForceLoad1Card& card, model::CardLocation where)
                                                 {
                                                     if (card.magnitude == 0.0)
                                                         zero(where, "magnitude",
                                                              zeroMagnitude(model, "PointForceLoad1"));
                                                 });
        forEachCard<model::PointForceLoad2Table>(model,
                                                 [&](const model::PointForceLoad2Card& card, model::CardLocation where)
                                                 {
                                                     if (card.magnitude == 0.0)
                                                         zero(where, "magnitude",
                                                              zeroMagnitude(model, "PointForceLoad2"));
                                                 });
        forEachCard<model::PointMomentLoad1Table>(
            model,
            [&](const model::PointMomentLoad1Card& card, model::CardLocation where)
            {
                if (card.magnitude == 0.0)
                    zero(where, "magnitude", zeroMagnitude(model, "PointMomentLoad1"));
            });
        forEachCard<model::PointMomentLoad2Table>(
            model,
            [&](const model::PointMomentLoad2Card& card, model::CardLocation where)
            {
                if (card.magnitude == 0.0)
                    zero(where, "magnitude", zeroMagnitude(model, "PointMomentLoad2"));
            });
        forEachCard<model::PointsPressureLoadTable>(
            model,
            [&](const model::PointsPressureLoadCard& card, model::CardLocation where)
            {
                if (card.pressure == 0.0)
                    zero(where, "pressure", "the pressure is zero");
            });
        forEachCard<model::ShellPressureLoadTable>(
            model,
            [&](const model::ShellPressureLoadCard& card, model::CardLocation where)
            {
                if (card.pressure == 0.0)
                    zero(where, "pressure", "the pressure is zero");
            });
        forEachCard<model::FacePressureLoadTable>(
            model,
            [&](const model::FacePressureLoadCard& card, model::CardLocation where)
            {
                // A blank P2, P3 or P4 is NaN: the first pressure stands for them.
                const bool none =
                    std::ranges::all_of(card.pressures, [](double p) { return std::isnan(p) || p == 0.0; });
                if (none)
                    zero(where, "pressure1", "every pressure is zero");
            });
        forEachCard<model::ScalarPointLoadTable>(
            model,
            [&](const model::ScalarPointLoadCard& card, model::CardLocation where)
            {
                const bool none =
                    std::ranges::all_of(card.loads, [](const model::PointLoad& load) { return load.magnitude == 0.0; });
                if (none)
                    zero(where, "magnitude", "every magnitude is zero");
            });
    }
};

} // namespace

void addZeroMagnitudeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ZeroMagnitudeCheck>());
}

} // namespace oor::analysis
