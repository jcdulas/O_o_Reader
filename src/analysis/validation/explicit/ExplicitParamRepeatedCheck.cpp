#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/explicit/ExplicitChecks.hpp"
#include "oor/model/cards/explicit/explicitParam/ExplicitParamCatalogue.hpp"
#include "oor/model/cards/explicit/explicitParam/ExplicitParamTable.hpp"

#include <string>
#include <unordered_map>

namespace oor::analysis
{

namespace
{

// A setting of DYPARAM given twice with different values: the solver takes the last card, so the first is dead text. A
// setting is the name, or the name and its option for CONTACT, or the name, the entity and the keyword for SMP
// (`explicitParamIdentity`); the entries that may be repeated on purpose (MATRMERG, one group of rigid bodies per card)
// and the names the catalogue does not know are not judged. Each model on its own, as PARAM is.
class ExplicitParamRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"explicit.explicit-param-repeated", base::Severity::Warning, "explicit",
                                    "An explicit parameter is given twice with different values."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        std::unordered_map<std::string, model::ExplicitParamCard> before;
        forEachCard<model::ExplicitParamTable>(
            context.model(),
            [&](const model::ExplicitParamCard& card, model::CardLocation where)
            {
                const std::optional<std::string> setting = model::explicitParamIdentity(card);
                if (!setting)
                    return;
                const auto previous = before.find(*setting);
                if (previous != before.end() && !(previous->second == card))
                    sink.reportCard(
                        where, "fieldValue",
                        *setting + " is given again with other values: the last card is the one that counts",
                        "keep one card, or the values that are meant");
                before[*setting] = card;
            });
    }
};

} // namespace

void addExplicitParamRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ExplicitParamRepeatedCheck>());
}

} // namespace oor::analysis
