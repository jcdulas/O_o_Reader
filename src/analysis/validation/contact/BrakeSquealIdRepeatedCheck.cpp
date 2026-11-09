#include "oor/analysis/validation/contact/ContactChecks.hpp"
#include "oor/analysis/validation/engine/SharedIds.hpp"
#include "oor/model/cards/nodes/moduleBrakeSystem/ModuleBrakeSystemTable.hpp"

namespace oor::analysis
{

namespace
{

// A brake squeal analysis number that a BSQUEAL and a BRKSYS both have: the Case Control command BSQUEAL names the
// analysis by this number and cannot tell the two cards apart (the format reference gives both cards the ID "of a
// corresponding BSQUEAL Case Control command"). Two cards of one type with the same ID are the reader's business. An
// MDBKSYS is selected by the same command and may share its ID with BRKSYS cards (the
// command selects them all, the first line of the MDBKSYS winning), so it is left out.
class BrakeSquealIdRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "contact.brake-squeal-id-repeated", base::Severity::Error, "contact",
        "A brake squeal card and a brake system have the same ID, which the brake squeal Case Control command cannot "
        "tell apart."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportIdsSharedByTypes(context, sink, model::CardCategory::BrakeSqueal, "brake squeal analysis",
                               [](const model::Model&, model::CardLocation where)
                               { return where.table == model::typeIndexOf<model::ModuleBrakeSystemTable>(); });
    }
};

} // namespace

void addBrakeSquealIdRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<BrakeSquealIdRepeatedCheck>());
}

} // namespace oor::analysis
