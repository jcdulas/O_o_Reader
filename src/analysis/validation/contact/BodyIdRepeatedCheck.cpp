#include "oor/analysis/validation/contact/ContactChecks.hpp"
#include "oor/analysis/validation/engine/SharedIds.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/nonlinear_contact/contactNodeRegion/ContactNodeRegionTable.hpp"

namespace oor::analysis
{

namespace
{

// A body ID that two kinds of card have: BCBODY, BCBODY1, BCSURF and BCGRID all define a body that a BCONECT, a
// BCTABLE, a BCHANGE or a BCMOVE names by its BID, and the BIDs must be unique among them (the format reference). The
// SOL 700 form of BCGRID is not a body: its ID names a cloud of grid points (a surface), so it is left out.
class BodyIdRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "contact.body-id-repeated", base::Severity::Error, "contact",
        "A contact body or a face or node contact region has the body number of a contact body of another card."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportIdsSharedByTypes(context, sink, model::CardCategory::ContactBody, "contact body",
                               [](const model::Model& model, model::CardLocation where)
                               {
                                   return isType<model::ContactNodeRegionTable>(where) &&
                                          cardAt<model::ContactNodeRegionTable>(model, where).sol700;
                               });
    }
};

} // namespace

void addBodyIdRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<BodyIdRepeatedCheck>());
}

} // namespace oor::analysis
