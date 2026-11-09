#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/solution/SolutionChecks.hpp"
#include "oor/model/cards/solution_control/rotorAxisym/RotorAxisymTable.hpp"

#include <map>

namespace oor::analysis
{

namespace
{

// An axisymmetric rotor that has no ROTORAX list of grid points: the format reference asks every rotor defined by
// ROTORAX cards to have at least one with LTYPE GRID, which gives the axis of symmetry. The RSPINR,
// RSPINT and UNBALNC cards may only name those grid points.
class RotorAxisymAxisCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"solution.rotor-axisym-axis", base::Severity::Warning, "solution",
                                    "A rotor of axisymmetric rotor cards has no card that lists the grid points of its "
                                    "axis."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        struct Rotor
        {
            model::CardLocation first; // the first card of the rotor, where the finding goes
            bool hasAxis = false;
        };
        std::map<base::EntityId, Rotor> rotors;
        forEachCard<model::RotorAxisymTable>(
            context.model(),
            [&](const model::RotorAxisymCard& card, model::CardLocation where)
            {
                Rotor& rotor = rotors.try_emplace(card.id, Rotor{where, false}).first->second;
                rotor.hasAxis = rotor.hasAxis || card.listType == model::RotorAxisymListType::Grid;
            });
        // GRID is a value of the field, written as the deck writes it.
        const std::string listType = std::string("listType") + " GRID";
        for (const auto& [rotorid, rotor] : rotors)
        {
            if (!rotor.hasAxis)
                sink.reportCard(rotor.first, "listType",
                                "no " + std::string("RotorAxisym") + " card of the rotor " + std::to_string(rotorid) +
                                    " lists the grid points of its axis (" + listType + ")",
                                "add a " + std::string("RotorAxisym") + " with " + listType +
                                    " and at least two collinear grid points");
        }
    }
};

} // namespace

void addRotorAxisymAxisCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<RotorAxisymAxisCheck>());
}

} // namespace oor::analysis
