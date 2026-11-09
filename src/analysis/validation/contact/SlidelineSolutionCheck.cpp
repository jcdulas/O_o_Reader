#include "oor/analysis/validation/contact/ContactChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/nonlinear_contact/contactSlideline/ContactSlidelineTable.hpp"
#include "oor/model/cards/nonlinear_contact/slidelineFriction/SlidelineFrictionTable.hpp"

namespace oor::analysis
{

namespace
{

// A slideline contact region (BCONP) or its friction (BFRIC) in a deck that does not run the slideline contact: the
// format reference gives both for SOL 106 and SOL 129 only (SOL 400 has the contact tables and BCONECT instead). A deck
// with no SOL statement is not judged.
class SlidelineSolutionCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"contact.slideline-solution", base::Severity::Warning, "contact",
                                    "A slideline contact or slideline friction card is in a deck whose solution is not "
                                    "SOL 106 or SOL 129."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const std::optional<std::int64_t> solution = context.solution();
        if (!solution || *solution == 106 || *solution == 129)
            return;
        const std::string message =
            "slideline contact is for SOL 106 and SOL 129, and the deck is SOL " + std::to_string(*solution);
        const std::string suggestion =
            "describe the contact with " +
            cardLabels(context.model(), {"ContactTable", "ContactBody", "ContactPair"}, "and") +
            " (SOL 101 or SOL 400), or use SOL 106";
        forEachCard<model::ContactSlidelineTable>(context.model(),
                                                  [&](const model::ContactSlidelineCard&, model::CardLocation where)
                                                  { sink.reportCard(where, "", message, suggestion); });
        forEachCard<model::SlidelineFrictionTable>(context.model(),
                                                   [&](const model::SlidelineFrictionCard&, model::CardLocation where)
                                                   { sink.reportCard(where, "", message, suggestion); });
    }
};

} // namespace

void addSlidelineSolutionCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SlidelineSolutionCheck>());
}

} // namespace oor::analysis
