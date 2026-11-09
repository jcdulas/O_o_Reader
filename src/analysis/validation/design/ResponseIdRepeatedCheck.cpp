#include "oor/analysis/validation/design/DesignChecks.hpp"
#include "oor/analysis/validation/engine/SharedIds.hpp"

namespace oor::analysis
{

namespace
{

// The DRESP1, DRESP2 and DRESP3 responses share one number space: a DCONSTR, a DESOBJ and the DRESP1 and DRESP2 groups
// of a DRESP2 name a response by its ID alone. Two cards of one type with the same
// ID are the reader's business (`ref.duplicate-id`); this check is for the ID that two kinds of card have.
class ResponseIdRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"design.response-id-repeated", base::Severity::Error, "design",
                                    "A design response has the ID of a design response of another kind."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportIdsSharedByTypes(context, sink, model::CardCategory::DesignResponse, "response");
    }
};

} // namespace

void addResponseIdRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ResponseIdRepeatedCheck>());
}

} // namespace oor::analysis
