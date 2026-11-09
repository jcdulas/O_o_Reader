#include "oor/analysis/validation/contact/ContactChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/nonlinear_contact/contactBody/ContactBodyTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBody1/ContactBody1Table.hpp"
#include "oor/model/references/ReferenceChecker.hpp"

namespace oor::analysis
{

namespace
{

// A deformable body whose BSID names a rigid surface. The BSID of a deformable body is a BSURF or a BCPROP (and, in SOL
// 700, a BCBOX, BCMATL, BCSEG, BCGRID or BCELIPS); a BCPATCH, BCBZIER, BCNURB2 or BCNURBS, or the BCRGSRF that lists
// them, describes a rigid surface and a rigid body (BEHAV RIGID) names it. A rigid body may be made of elements, so the
// opposite is not wrong.
class BodySurfaceKindCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"contact.body-surface-kind", base::Severity::Error, "contact",
                                    "A deformable contact body has a surface number that names a rigid surface."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachCard<model::ContactBodyTable>(context.model(),
                                             [&](const model::ContactBodyCard& card, model::CardLocation where)
                                             { check(context, sink, where, card.behavior, card.surfaceId); });
        forEachCard<model::ContactBody1Table>(context.model(),
                                              [&](const model::ContactBody1Card& card, model::CardLocation where)
                                              { check(context, sink, where, card.behavior, card.surfaceId); });
    }

private:
    static void check(const ValidationContext& context, FindingSink& sink, model::CardLocation where,
                      const std::string& behavior, base::EntityId surface)
    {
        if (behavior != "Deformable" || surface == 0)
            return;
        const auto found = model::resolveReference(context.model(), model::ReferenceTarget::ContactSurface, surface);
        if (!found)
            return; // an unresolved reference is `ref.unresolved`
        const std::string_view name = cardNameAt(context.model(), *found);
        if (isOneOf(name, {"ContactPatchSurface", "ContactBezierSurface", "ContactNurbsCurve", "ContactNurbsSurface",
                           "ContactRigidSurfaceList"}))
            sink.reportCard(where, "surfaceId",
                            "the " + std::string("surfaceId") + " " + std::to_string(surface) + " names a " +
                                std::string(name) + ", a rigid surface, for a deformable body",
                            "make the body rigid (" + std::string("behavior") + " RIGID) or name a " +
                                cardLabels(context.model(), {"ContactElemSurface", "ContactPropertyRegion"}));
    }
};

} // namespace

void addBodySurfaceKindCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<BodySurfaceKindCheck>());
}

} // namespace oor::analysis
