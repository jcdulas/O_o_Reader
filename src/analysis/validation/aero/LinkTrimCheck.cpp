#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/aeroTrim/AeroTrimTable.hpp"
#include "oor/model/cards/aero/aeroTrim2/AeroTrim2Table.hpp"
#include "oor/model/cards/aero/aeroTrimLink/AeroTrimLinkTable.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// A positive ID on an AELINK is the set of the TRIM (or TRIM2) that the Case Control command TRIM selects, and the link
// holds for that subcase only; 0 and ALWAYS hold for every one. An ID that no TRIM
// or TRIM2 has is a link no subcase can select: a warning, since the deck still runs, without the link.
class LinkTrimCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.link-trim", base::Severity::Warning, "aero",
                                    "The positive ID of a trim link is the set number of no trim card."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::AeroTrimLinkTable>(
            model,
            [&](const model::AeroTrimLinkCard& card, model::CardLocation where)
            {
                // the TRIM and TRIM2 cards each have an ID space of their own (their SIDs are one set, checked apart)
                if (card.id <= 0 || model.locateInType(model::typeIndexOf<model::AeroTrimTable>(), card.id) ||
                    model.locateInType(model::typeIndexOf<model::AeroTrim2Table>(), card.id))
                    return;
                sink.reportCard(where, "id",
                                "no " + cardLabels(model, {"AeroTrim", "AeroTrim2"}) + " card has the " +
                                    std::string("id") + " " + std::to_string(card.id) +
                                    ", so no subcase selects the link",
                                "give the " + std::string("id") + " of a " + std::string("AeroTrim") +
                                    " card, or 0 for every subcase");
            });
    }
};

} // namespace

void addLinkTrimCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<LinkTrimCheck>());
}

} // namespace oor::analysis
