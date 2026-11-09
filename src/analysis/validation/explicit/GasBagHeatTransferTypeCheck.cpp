#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/explicit/ExplicitChecks.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/explicit/gasBagHeatTransfer/GasBagHeatTransferTable.hpp"
#include "oor/model/references/ReferenceChecker.hpp"

#include <string>
#include <vector>

namespace oor::analysis
{

namespace
{

// A HEATLOS whose HTRTYPE word does not match the heat transfer card that has the number HTRTYPID. The HTRCONV and the
// HTRRAD cards share one category (each has a number space of its own), so the model resolves the number to whichever
// has it; the word is what says which one was meant. The words are the names of the cards. An HTRCONV 2 and an HTRRAD 2
// may both exist: it is enough that one of them is the type named.
class GasBagHeatTransferTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "explicit.gas-bag-heat-transfer-type", base::Severity::Error, "explicit",
        "The heat transfer type of a gas bag heat transfer card does not name the kind of the heat transfer card it "
        "points to."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::GasBagHeatTransferTable>(
            model,
            [&](const model::GasBagHeatTransferCard& card, model::CardLocation where)
            {
                if (card.modelId == 0)
                    return;
                const std::vector<model::CardLocation> found =
                    model::cardsOfSet(model, model::CardCategory::HeatTransferModel, card.modelId);
                if (found.empty()) // ref.unresolved
                    return;
                // The kind is the neutral name of a card type, compared with the type of each card found.
                const std::string_view type = model::keywordText(model::HEAT_TRANSFER_WORDS, card.modelKind);
                for (const model::CardLocation& location : found)
                {
                    if (cardNameAt(model, location) == type)
                        return;
                }
                sink.reportCard(
                    where, "modelKind",
                    std::string("modelKind") + " is " + std::string(type) + " but the heat transfer card " +
                        std::to_string(card.modelId) + " is an " + std::string(cardNameAt(model, found.front())),
                    "give the " + std::string("modelKind") + " of the card that " + std::string("modelId") + " names");
            });
    }
};

} // namespace

void addGasBagHeatTransferTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<GasBagHeatTransferTypeCheck>());
}

} // namespace oor::analysis
