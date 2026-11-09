#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/aero/aeroTrimLink/AeroTrimLinkTable.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace oor::analysis
{

namespace
{

// An AELINK makes one trim variable (LABLD) a linear combination of others, and a variable can be constrained only
// once: not twice in the cards of one ID, and not in a card of the ID and in one that applies to every subcase (the ID
// 0 or the word ALWAYS).
class LinkDependentRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "aero.link-dependent-repeated", base::Severity::Error, "aero",
        "A trim link makes a trim variable depend on others when another trim link that applies to the same subcases "
        "already does."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        struct Link
        {
            base::EntityId id;
            model::CardLocation where;
        };
        std::unordered_map<std::string, std::vector<Link>> byVariable; // the dependent variable, upper case
        forEachCard<model::AeroTrimLinkTable>(
            context.model(),
            [&](const model::AeroTrimLinkCard& card, model::CardLocation where)
            {
                std::vector<Link>& earlier = byVariable[std::string(card.dependentLabel)];
                for (const Link& other : earlier)
                {
                    // Two cards apply to the same subcase when their IDs are equal or one
                    // of them is 0 (every subcase).
                    if (other.id == card.id || other.id == 0 || card.id == 0)
                    {
                        sink.reportCard(where, "dependentLabel",
                                        "the variable " + card.dependentLabel +
                                            " is already the dependent variable of another " +
                                            std::string("AeroTrimLink") + " that applies to the same subcases",
                                        "constrain a variable with one " + std::string("AeroTrimLink") + " only");
                        break;
                    }
                }
                earlier.push_back(Link{card.id, where});
            });
    }
};

} // namespace

void addLinkDependentRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<LinkDependentRepeatedCheck>());
}

} // namespace oor::analysis
