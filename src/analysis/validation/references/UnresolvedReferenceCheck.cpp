#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/references/ReferenceChecks.hpp"

namespace oor::analysis
{

namespace
{

// Every reference field of a card that points at a card that does not exist. This is the one check that reports a
// missing target, whatever the card: a FORCE on a grid point that is not there, an element with a missing property, a
// grid point in an undefined coordinate system. The checks of the other families assume the references resolve.
class UnresolvedReferenceCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"ref.unresolved", base::Severity::Error, "references",
                                    "A field of a card refers to an ID that no card defines."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        for (const UnresolvedUse& use : context.references().unresolved)
        {
            if (use.inCaseControl)
                continue; // the Case Control family reports its own references
            std::string message = "refers to " + std::string(describeTarget(use.target)) + " " +
                                  std::to_string(use.id) + ", which does not exist";
            sink.reportCard(use.location, use.field, std::move(message),
                            "define the " + std::string(describeTarget(use.target)) + " or correct the ID");
        }
    }
};

} // namespace

void addUnresolvedReferenceCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<UnresolvedReferenceCheck>());
}

} // namespace oor::analysis
