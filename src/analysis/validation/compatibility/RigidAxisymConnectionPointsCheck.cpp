#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/elements/axisymQuad/AxisymQuadTable.hpp"
#include "oor/model/cards/elements/axisymTria/AxisymTriaTable.hpp"
#include "oor/model/cards/properties/axisymHarmonicProp/AxisymHarmonicPropTable.hpp"
#include "oor/model/cards/rigid_elements/rigidAxisymConnection/RigidAxisymConnectionTable.hpp"

#include <string>
#include <unordered_map>

namespace oor::analysis
{

namespace
{

// What the axisymmetric harmonic elements (CQUADX and CTRIAX) do with a point that an RBAX3D names.
struct PointUse
{
    base::EntityId element = 0;       // an element that has the point, 0 when none
    base::EntityId wrongElement = 0;  // an element of a PAXSYMH with a harmonic index other than 1, 0 when none
    base::EntityId wrongProperty = 0; // the PAXSYMH of that element
    base::EntityId wrongHarmonic = 0; // its NHARM
};

// The harmonic index NHARM of the PAXSYMH with this ID, or -1 when the property is not a PAXSYMH (it is then another
// check's business: an unresolved reference, or a property of the wrong kind).
base::EntityId harmonicIndexOf(const model::Model& model, base::EntityId pid)
{
    const std::optional<model::CardLocation> property = model.locate(base::IdSpace::Property, pid);
    if (!property || !isType<model::AxisymHarmonicPropTable>(*property))
        return -1;
    return cardAt<model::AxisymHarmonicPropTable>(model, *property).harmonicIndex;
}

// Notes the elements of a table that have one of the points that the RBAX3D cards name.
template <typename TableT>
void scanElements(const model::Model& model, std::unordered_map<base::EntityId, PointUse>& uses)
{
    forEachCard<TableT>(
        model,
        [&](const typename TableT::CardType& card, model::CardLocation)
        {
            std::optional<base::EntityId> harmonic; // looked up when the element has a point of interest
            for (const base::EntityId node : card.nodes)
            {
                const auto found = uses.find(node);
                if (node == 0 || found == uses.end())
                    continue;
                PointUse& use = found->second;
                if (use.element == 0)
                    use.element = card.id;
                if (!harmonic)
                    harmonic = harmonicIndexOf(model, card.propId);
                if (*harmonic >= 0 && *harmonic != 1 && use.wrongElement == 0)
                {
                    use.wrongElement = card.id;
                    use.wrongProperty = card.propId;
                    use.wrongHarmonic = *harmonic;
                }
            }
        });
}

// the point G3D, on the axis of symmetry, may not belong to an axisymmetric
// harmonic element (a CQUADX or a CTRIAX), and the point GAX must belong to one, whose PAXSYMH has harmonic index 1.
// The solver stops with a fatal message when it does not. A point that no card defines is reported by `ref.unresolved`,
// and a property that is not a PAXSYMH is not judged here.
class RigidAxisymConnectionPointsCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "compat.rigid-axisym-connection-points", base::Severity::Error, "compatibility",
        "A rigid axisymmetric connection whose 3D point is a point of an axisymmetric harmonic element, or whose "
        "axisymmetric point is not a point of one, or only of one with a harmonic index other than 1."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        std::unordered_map<base::EntityId, PointUse> uses;
        forEachCard<model::RigidAxisymConnectionTable>(
            model,
            [&](const model::RigidAxisymConnectionCard& card, model::CardLocation)
            {
                uses.try_emplace(card.node3D);
                uses.try_emplace(card.axisymNode);
            });
        if (uses.empty())
            return;
        scanElements<model::AxisymQuadTable>(model, uses);
        scanElements<model::AxisymTriaTable>(model, uses);

        forEachCard<model::RigidAxisymConnectionTable>(
            model,
            [&](const model::RigidAxisymConnectionCard& card, model::CardLocation where)
            {
                const std::string g3d = std::string("node3D");
                const std::string gax = std::string("axisymNode");
                const PointUse& independent = uses.at(card.node3D);
                if (independent.element != 0 && model.locate(base::IdSpace::Node, card.node3D))
                    sink.reportCard(where, "node3D",
                                    "the point " + g3d + " " + std::to_string(card.node3D) +
                                        " is a point of the axisymmetric harmonic element " +
                                        std::to_string(independent.element) + ": " + g3d +
                                        " must be a point of the 3-D model on the axis of symmetry",
                                    "name a point on the axis that no " + std::string("AxisymQuad") + " or " +
                                        std::string("AxisymTria") + " uses");
                const PointUse& dependent = uses.at(card.axisymNode);
                if (!model.locate(base::IdSpace::Node, card.axisymNode))
                    return; // ref.unresolved reports it
                if (dependent.element == 0)
                    sink.reportCard(where, "axisymNode",
                                    "the point " + gax + " " + std::to_string(card.axisymNode) + " is a point of no " +
                                        std::string("AxisymQuad") + " or " + std::string("AxisymTria") + ": " +
                                        gax +
                                        " must belong to an axisymmetric "
                                        "harmonic element",
                                    "name a point of a " + std::string("AxisymQuad") + " or a " +
                                        std::string("AxisymTria"));
                else if (dependent.wrongElement != 0)
                    sink.reportCard(where, "axisymNode",
                                    "the element " + std::to_string(dependent.wrongElement) + " that has the point " +
                                        gax + " " + std::to_string(card.axisymNode) + " has the harmonic index " +
                                        std::to_string(dependent.wrongHarmonic) + " on its " +
                                        std::string("AxisymHarmonicProp") + " " +
                                        std::to_string(dependent.wrongProperty) + ": the harmonic index must be 1",
                                    "give the " + std::string("AxisymHarmonicProp") + " the harmonic index 1 (" +
                                        std::string("harmonicIndex") + ")");
            });
    }
};

} // namespace

void addRigidAxisymConnectionPointsCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<RigidAxisymConnectionPointsCheck>());
}

} // namespace oor::analysis
