#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/AeroBoxRanges.hpp"
#include "oor/model/cards/aero/aeroBoxList/AeroBoxListTable.hpp"
#include "oor/model/cards/aero/aeroFiniteSurfaceSpline6/AeroFiniteSurfaceSpline6Table.hpp"
#include "oor/model/cards/aero/aeroNode/AeroNodeTable.hpp"
#include "oor/model/cards/aero/aeroQuad4/AeroQuad4Table.hpp"
#include "oor/model/cards/aero/aeroTria3/AeroTria3Table.hpp"
#include "oor/model/cards/aero/aeroUserSpline/AeroUserSplineTable.hpp"

#include <string>
#include <unordered_set>

namespace oor::analysis
{

namespace
{

// The items of an AELIST are the boxes of the panels (a control surface, a spline, a component), the (x, y) points of a
// Mach box surface, or the points and elements of an aerodynamic mesh (AEGRID, AEQUAD4, AETRIA3). An item that none of
// them has moves nothing: the format reference does not call it an error, so this is a warning. A model with no panel
// and no aerodynamic mesh has nothing to compare with (a deck split in several files) and is not judged. Two AELISTs
// hold other numbers and are not judged either: the VSLIST of a SPLINE6 whose VSTYPE is STRUC lists structural shell
// elements, and the AELIST of a SPLINEX is data for the spline server, which has no
// meaning to the solver.
class ListBoxCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "aero.list-box", base::Severity::Warning, "aero",
        "An aerodynamic box list names a box that no aerodynamic panel owns and a number that no aerodynamic mesh "
        "has."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const model::AeroBoxRanges boxes(model);
        std::unordered_set<base::EntityId> meshIds;
        forEachCard<model::AeroNodeTable>(
            model, [&](const model::AeroNodeCard& card, model::CardLocation) { meshIds.insert(card.id); });
        forEachCard<model::AeroQuad4Table>(
            model, [&](const model::AeroQuad4Card& card, model::CardLocation) { meshIds.insert(card.id); });
        forEachCard<model::AeroTria3Table>(
            model, [&](const model::AeroTria3Card& card, model::CardLocation) { meshIds.insert(card.id); });
        if (boxes.ranges().empty() && meshIds.empty())
            return;
        std::unordered_set<base::EntityId> otherLists;
        forEachCard<model::AeroFiniteSurfaceSpline6Table>(
            model,
            [&](const model::AeroFiniteSurfaceSpline6Card& card, model::CardLocation)
            {
                if (card.surfaceKind == model::AeroFiniteSurfaceSpline6SurfaceType::Struc)
                    otherLists.insert(card.surfaceElemListId);
            });
        forEachCard<model::AeroUserSplineTable>(model, [&](const model::AeroUserSplineCard& card, model::CardLocation)
                                                { otherLists.insert(card.aeroBoxListId); });
        forEachCard<model::AeroBoxListTable>(
            model,
            [&](const model::AeroBoxListCard& card, model::CardLocation where)
            {
                if (otherLists.contains(card.id))
                    return;
                for (const model::IdRun& run : card.ids)
                {
                    for (const base::EntityId id : {run.first, run.last})
                    {
                        if (boxes.owner(id) == nullptr && !meshIds.contains(id))
                        {
                            sink.reportCard(
                                where, "entries",
                                "the item " + std::to_string(id) +
                                    " is a box of no aerodynamic panel and a point or element of no aerodynamic mesh",
                                "name a box of an aerodynamic panel, or define the mesh");
                            return;
                        }
                    }
                }
            });
    }
};

} // namespace

void addListBoxCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ListBoxCheck>());
}

} // namespace oor::analysis
