#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/superelement/SuperelementChecks.hpp"

namespace oor::analysis
{

namespace
{

// A BEGIN SUPER (or AUXMODEL, ...) with no card in it: the part does nothing.
class EmptyPartCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"superelement.empty-part",
                                    base::Severity::Warning,
                                    "superelement",
                                    "A part of the deck (BEGIN SUPER, ...) has no card.",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        for (const std::unique_ptr<model::Model>& part : context.root().parts())
        {
            const std::optional<model::PartInfo>& info = part->partInfo();
            if (info && part->cardCount() == 0 && part->parts().empty())
                sink.reportStatement("SectionBegin", info->id, part->partLabel() + " has no card",
                                     "remove the part, or put its cards in it");
        }
    }
};

} // namespace

void addEmptyPartCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<EmptyPartCheck>());
}

} // namespace oor::analysis
