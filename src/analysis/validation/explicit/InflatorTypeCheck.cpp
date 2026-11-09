#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/explicit/ExplicitChecks.hpp"
#include "oor/model/cards/loads/inflatorInlet/InflatorInletTable.hpp"
#include "oor/model/references/ReferenceChecker.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// An ABINFL whose INFTYPE word does not match the inflator card that has the number INFTYPID. INFLTR, INFLHB, INFLCG
// and INFLTNK share one category, so the model resolves the number to whichever has it; the word (the name of the card)
// says which one was meant. It is enough that one of the cards with the number is the type named.
class InflatorTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"explicit.inflator-type", base::Severity::Error, "explicit",
                                    "The inflator type of an inflator inlet does not name the kind of the inflator "
                                    "card it points to."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::InflatorInletTable>(
            model,
            [&](const model::InflatorInletCard& card, model::CardLocation where)
            {
                if (card.inflatorKindId == 0)
                    return;
                const std::vector<model::CardLocation> found =
                    model::cardsOfSet(model, model::CardCategory::Inflator, card.inflatorKindId);
                if (found.empty()) // ref.unresolved
                    return;
                // The word table holds the neutral card names: the card type is compared by its name.
                const std::string_view kind =
                    model::keywordText(model::INFLATOR_INLET_INFLATOR_WORDS, card.inflatorKind);
                for (const model::CardLocation& location : found)
                {
                    if (cardNameAt(model, location) == kind)
                        return;
                }
                sink.reportCard(where, "inflatorKind",
                                std::string("inflatorKind") + " is " + std::string(kind) + " but the inflator card " +
                                    std::to_string(card.inflatorKindId) + " is a " +
                                    std::string(cardNameAt(model, found.front())),
                                "give the " + std::string("inflatorKind") + " of the card that " +
                                    std::string("inflatorKindId") + " names");
            });
    }
};

} // namespace

void addInflatorTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<InflatorTypeCheck>());
}

} // namespace oor::analysis
