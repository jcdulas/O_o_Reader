#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/properties/solidProp/SolidPropTable.hpp"

namespace oor::analysis
{

namespace
{

// A PSOLID that makes fluid elements (FCTN = PFLUID or FFLUID) must use a MAT10.
class FluidSolidMaterialCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.fluid-solid-material", base::Severity::Error, "compatibility",
                                    "A solid property with a fluid formulation uses a material that is not a fluid "
                                    "material."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::SolidPropTable>(
            model,
            [&](const model::SolidPropCard& card, model::CardLocation where)
            {
                if (card.elementFunction != "PressureFluid" && card.elementFunction != "FreeFluid")
                    return;
                const std::optional<model::CardLocation> material = model.locate(base::IdSpace::Material, card.matId);
                if (!material || cardNameAt(model, *material) == "FluidMat")
                    return;
                sink.reportCard(where, "matId",
                                std::string("elementFunction") + " is " + card.elementFunction + " but material " +
                                    std::to_string(card.matId) + " is a " + std::string(cardNameAt(model, *material)) +
                                    ", not a " + std::string("FluidMat"),
                                "use a " + std::string("FluidMat") + " for fluid elements");
            });
    }
};

} // namespace

void addFluidSolidMaterialCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<FluidSolidMaterialCheck>());
}

} // namespace oor::analysis
