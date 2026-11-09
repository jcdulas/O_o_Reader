#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/aeroSplineEquation3/AeroSplineEquation3Table.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <string>

namespace oor::analysis
{

namespace
{

// A SPLINE3 sets one component of motion of a box (COMP, 1 to 6) from structural degrees of freedom, and the components
// a box has depend on the kind of the CAERO that owns it: a Doublet-Lattice box moves in the normal direction and in
// rotation about the y axis (3 and 5), a body in its two lateral translations and rotations (2, 3, 5 and 6), a Mach box
// only in the normal direction (3), a strip of strip or piston theory in the normal direction and the two rotations (3,
// 5 and 6). A CAERO that does not exist is `ref.unresolved`.
class AeroSplineEquation3ComponentCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.aero-spline-equation3-component", base::Severity::Error, "aero",
                                    "The component of an equation spline is not one that the kind of its aerodynamic "
                                    "panel allows."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::AeroSplineEquation3Table>(
            model,
            [&](const model::AeroSplineEquation3Card& card, model::CardLocation where)
            {
                const std::optional<model::CardLocation> panel =
                    model.locate(base::IdSpace::AeroElement, card.aeroPanelId);
                if (!panel)
                    return;
                const std::string_view kind = cardNameAt(model, *panel);
                const std::span<const std::int32_t> allowed = allowedComponents(kind);
                if (allowed.size() == 0 || std::find(allowed.begin(), allowed.end(), card.component) != allowed.end())
                    return;
                std::string list;
                for (const std::int32_t component : allowed)
                    list += (list.empty() ? "" : ", ") + std::to_string(component);
                sink.reportCard(where, "component",
                                "the component " + std::to_string(card.component) + " is not one of a " +
                                    std::string(kind) + " box (" + list + ")",
                                "give a component the box can move in");
            });
    }

private:
    // An empty list stands for a card the table does not know.
    static std::span<const std::int32_t> allowedComponents(std::string_view kind)
    {
        static constexpr std::array<std::int32_t, 2> PANEL{3, 5};
        static constexpr std::array<std::int32_t, 4> BODY{2, 3, 5, 6};
        static constexpr std::array<std::int32_t, 1> MACH_BOXES{3};
        static constexpr std::array<std::int32_t, 3> STRIPS{3, 5, 6};
        if (kind == "AeroLiftingPanel1")
            return PANEL;
        if (kind == "AeroBody2")
            return BODY;
        if (kind == "AeroMachBoxPanel3")
            return MACH_BOXES;
        if (kind == "AeroStripPanel4" || kind == "AeroPistonPanel5")
            return STRIPS;
        return {};
    }
};

} // namespace

void addAeroSplineEquation3ComponentCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<AeroSplineEquation3ComponentCheck>());
}

} // namespace oor::analysis
