#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/explicit/ExplicitChecks.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/explicit/initialConditionRegion/InitialConditionRegionTable.hpp"
#include "oor/model/references/ReferenceChecker.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace oor::analysis
{

namespace
{

// The card type a TICREG of this TYPE names in its VID: the closed surface of an InitialSurfaceVolume, the sphere of a
// SphereShape, the cylinder of a CylinderShape (the format's table "Type: Region"). The other two types name a
// ContactBoxRegion and a SET1, which have categories of their own, so the reference already tells a wrong card.
std::string_view shapeCardOf(model::EulerRegionType type) noexcept
{
    switch (type)
    {
    case model::EulerRegionType::Surface:
        return "InitialSurfaceVolume";
    case model::EulerRegionType::Sphere:
        return "SphereShape";
    case model::EulerRegionType::Cylinder:
        return "CylinderShape";
    case model::EulerRegionType::Box:
    case model::EulerRegionType::Element:
        break;
    }
    return {};
}

// A TICREG whose TYPE does not match the shape card that has the number VID. The SURFINI, SPHERE and CYLINDR cards
// share one category (each has a number space of its own), so the model resolves the number to whichever has it; the
// TYPE is what says which one was meant. It is enough that one of the cards that have the number is of the type named.
class InitialConditionRegionShapeTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"explicit.initial-condition-region-shape-type", base::Severity::Error, "explicit",
                                    "The type of an initial condition region does not name the kind of the shape card "
                                    "it points to."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::InitialConditionRegionTable>(
            model,
            [&](const model::InitialConditionRegionCard& card, model::CardLocation where)
            {
                const std::string_view expected = shapeCardOf(card.type);
                if (expected.empty())
                    return;
                const std::vector<model::CardLocation> found =
                    model::cardsOfSet(model, model::CardCategory::EulerRegionShape, card.shapeId);
                if (found.empty()) // ref.unresolved
                    return;
                for (const model::CardLocation& location : found)
                {
                    if (cardNameAt(model, location) == expected)
                        return;
                }
                const std::string typeField = std::string("shapeType");
                const std::string shapeField = std::string("shapeId");
                sink.reportCard(where, "shapeType",
                                typeField + " is " +
                                    std::string(model::keywordText(model::EULER_REGION_WORDS, card.type)) +
                                    " but the shape card " + std::to_string(card.shapeId) + " is a " +
                                    std::string(cardNameAt(model, found.front())),
                                "give the " + typeField + " of the card that " + shapeField + " names, or the " +
                                    shapeField + " of a " + std::string(expected));
            });
    }
};

} // namespace

void addInitialConditionRegionShapeTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<InitialConditionRegionShapeTypeCheck>());
}

} // namespace oor::analysis
