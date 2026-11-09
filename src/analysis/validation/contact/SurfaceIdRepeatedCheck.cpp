#include "oor/analysis/validation/contact/ContactChecks.hpp"
#include "oor/analysis/validation/engine/SharedIds.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/nonlinear_contact/contactSegment/ContactSegmentTable.hpp"

namespace oor::analysis
{

namespace
{

// A surface ID that two kinds of card have: the BSID of a body names a BSURF, BCPROP, BCMATL or BCBOX (and, in SOL 700,
// a BCELIPS), or a rigid surface (BCPATCH, BCBZIER, BCNURB2, BCNURBS or the BCRGSRF that lists them), and the IDs of
// these cards must be different (the format reference). A BCSEG is a segment of a surface, numbered on its own, so it
// is left out.
class SurfaceIdRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "contact.surface-id-repeated", base::Severity::Error, "contact",
        "Two cards that define a contact surface (element, property, material, box or patch region, ...) have the same "
        "ID."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportIdsSharedByTypes(context, sink, model::CardCategory::ContactSurface, "contact surface",
                               [](const model::Model&, model::CardLocation where)
                               { return isType<model::ContactSegmentTable>(where); });
    }
};

} // namespace

void addSurfaceIdRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SurfaceIdRepeatedCheck>());
}

} // namespace oor::analysis
