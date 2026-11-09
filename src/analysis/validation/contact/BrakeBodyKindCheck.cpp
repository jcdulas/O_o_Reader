#include "oor/analysis/validation/contact/ContactChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/nonlinear_contact/brakeSystem/BrakeSystemTable.hpp"
#include "oor/model/references/ReferenceChecker.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// A body of a brake system that is not a BCBODY1. The format reference takes BD1_ID and BD2_ID as the BCBODY1 IDs
// of the two bodies of the system. The reference resolves to any contact body (BCBODY, BCBODY1, BCSURF, BCGRID), so the
// kind of the card it finds is looked at here; a body that no card defines is `ref.unresolved`.
class BrakeBodyKindCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"contact.brake-body-kind", base::Severity::Warning, "contact",
                                    "A brake system names a contact body that is not a contact body of the second "
                                    "kind."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachCard<model::BrakeSystemTable>(
            context.model(),
            [&](const model::BrakeSystemCard& card, model::CardLocation where)
            {
                for (const model::BrakeSystemDisk& disk : card.disks)
                {
                    check(context, sink, where, "firstBodyId", disk.name, disk.firstBodyId);
                    check(context, sink, where, "secondBodyId", disk.name, disk.secondBodyId);
                }
            });
    }

private:
    static void check(const ValidationContext& context, FindingSink& sink, model::CardLocation where, const char* field,
                      const std::string& disk, base::EntityId body)
    {
        const auto found = model::resolveReference(context.model(), model::ReferenceTarget::ContactBody, body);
        if (!found)
            return;
        const std::string_view name = cardNameAt(context.model(), *found);
        if (name != "ContactBody1")
            sink.reportCard(where, field,
                            "the disk " + disk + " names the body " + std::to_string(body) + ", a " +
                                std::string(name) + ", where a brake system needs a " + std::string("ContactBody1"),
                            "name a " + std::string("ContactBody1") +
                                " of the brake system, or define the body with a " + std::string("ContactBody1"));
    }
};

} // namespace

void addBrakeBodyKindCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<BrakeBodyKindCheck>());
}

} // namespace oor::analysis
