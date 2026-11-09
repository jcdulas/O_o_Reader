#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/explicit/ExplicitChecks.hpp"
#include "oor/model/cards/explicit/gasBagLeakage/GasBagLeakageTable.hpp"
#include "oor/model/references/ReferenceChecker.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// A LEAKAGE whose PORTYPE word does not match the porosity card that has the number PORTYPID. The nine porosity cards
// (PORFLOW, PORFLWT, PORHOLE, PERMEAB, PORFGBG, PERMGBG, PORFCPL, PORHYDS, PORUDS) share one category, so the model
// resolves the number to whichever has it; the word is what says which one was meant. The words are the names of the
// cards. Two porosity cards of different types may share a number: it is enough that one of them is the type named.
class GasBagLeakagePorosityTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"explicit.leakage-porosity-type", base::Severity::Error, "explicit",
                                    "The porosity type of a gas bag leakage card does not name the kind of the "
                                    "porosity card it points to."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::GasBagLeakageTable>(
            model,
            [&](const model::GasBagLeakageCard& card, model::CardLocation where)
            {
                if (card.porosityModelId == 0)
                    return;
                const std::vector<model::CardLocation> found =
                    model::cardsOfSet(model, model::CardCategory::Porosity, card.porosityModelId);
                if (found.empty()) // ref.unresolved
                    return;
                // The kind is the neutral name of a card type, compared with the type of each card found.
                const std::string_view type = model::keywordText(model::POROSITY_TYPE_WORDS, card.porosityKind);
                for (const model::CardLocation& location : found)
                {
                    if (cardNameAt(model, location) == type)
                        return;
                }
                sink.reportCard(where, "porosityKind",
                                std::string("porosityKind") + " is " + std::string(type) + " but the porosity card " +
                                    std::to_string(card.porosityModelId) + " is a " +
                                    std::string(cardNameAt(model, found.front())),
                                "give the " + std::string("porosityKind") + " of the card that " +
                                    std::string("porosityModelId") + " names");
            });
    }
};

} // namespace

void addGasBagLeakagePorosityTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<GasBagLeakagePorosityTypeCheck>());
}

} // namespace oor::analysis
