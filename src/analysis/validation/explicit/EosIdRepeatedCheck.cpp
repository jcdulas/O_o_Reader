#include "oor/analysis/validation/engine/SharedIds.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/explicit/ExplicitChecks.hpp"

namespace oor::analysis
{

namespace
{

// An equation of state number that two kinds of card have: EOSDEF, EOSGAM, EOSIG, EOSJWL, EOSMG, EOSNA, EOSPOL and
// EOSUDS are all named by the EID of a MATDEUL (or MAT1A), so the EIDs must be unique among them (the format
// reference). Two cards of one type with the same EID are the reader's business (`ref.duplicate-id`).
class EosIdRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"explicit.eos-id-repeated", base::Severity::Error, "explicit",
                                    "An equation of state card has the number of an equation of state of another "
                                    "card."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportIdsSharedByTypes(context, sink, model::CardCategory::EquationOfState, "equation of state");
    }
};

} // namespace

void addEosIdRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<EosIdRepeatedCheck>());
}

} // namespace oor::analysis
