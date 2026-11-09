#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/superelement/SuperelementChecks.hpp"
#include "oor/model/cards/superelements/superelementBulkOptions/SuperelementBulkOptionsTable.hpp"
#include "oor/model/cards/superelements/superelementMirrorPlane/SuperelementMirrorPlaneTable.hpp"

#include <set>

namespace oor::analysis
{

namespace
{

// A MIRROR superelement with no mirror plane: the SEBULK of type MIRROR names the superelement it mirrors, and an
// SEMPLN with the number of the mirror gives the plane (format reference, SEBULK and SEMPLN). Without the plane the
// solver cannot place the image.
class MirrorPlaneCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"superelement.mirror-plane", base::Severity::Error, "superelement",
                                    "A superelement of the mirror type has no card that gives its mirror plane."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        if (model.partInfo())
            return;
        std::set<base::EntityId> planes;
        forEachCard<model::SuperelementMirrorPlaneTable>(
            model, [&](const model::SuperelementMirrorPlaneCard& card, model::CardLocation)
            { planes.insert(card.superelementId); });
        forEachCard<model::SuperelementBulkOptionsTable>(
            model,
            [&](const model::SuperelementBulkOptionsCard& card, model::CardLocation where)
            {
                if (card.partType == model::SuperelementBulkOptionsType::Mirror &&
                    !planes.contains(card.superelementId))
                    sink.reportCard(where, "partType",
                                    "superelement " + std::to_string(card.superelementId) + " is a mirror image of " +
                                        std::to_string(card.referenceSuperelementId) + " and has no " +
                                        std::string("SuperelementMirrorPlane"),
                                    "add an " + std::string("SuperelementMirrorPlane") + " with " +
                                        std::string("superelementId") + " " + std::to_string(card.superelementId) +
                                        " and the three points of the plane");
            });
    }
};

} // namespace

void addMirrorPlaneCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<MirrorPlaneCheck>());
}

} // namespace oor::analysis
