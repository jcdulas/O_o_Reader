#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/loads/LoadChecks.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/loads/freeBodyLoad/FreeBodyLoadTable.hpp"
#include "oor/model/cards/loads/freeBodyLoadSubcase/FreeBodyLoadSubcaseTable.hpp"

#include <set>
#include <string>

namespace oor::analysis
{

namespace
{

// An FBODYLD names the subsystem of its load by the NAMES of an FBODYSB (format reference, FBODYLD: "name of a FBODYSB
// Bulk Data entry that defines the subsystem for this load"). A name is not an ID, so no reference resolves it and
// `ref.unresolved` does not see it: this check matches the names, without regard to the case, as every name a card
// gives another card is.
class FreeBodySubsystemCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"load.free-body-subsystem", base::Severity::Error, "loads",
                                    "A free body load names a subsystem that no free body subsystem card defines."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        std::set<std::string> defined;
        forEachCard<model::FreeBodyLoadSubcaseTable>(
            model, [&](const model::FreeBodyLoadSubcaseCard& card, model::CardLocation)
            { defined.insert(std::string(card.names)); });
        forEachCard<model::FreeBodyLoadTable>(
            model,
            [&](const model::FreeBodyLoadCard& card, model::CardLocation where)
            {
                if (defined.contains(std::string(card.subsystemName)))
                    return;
                sink.reportCard(
                    where, "subsystemName",
                    "the load case " + card.name + " names the subsystem " + card.subsystemName + ", which no " +
                        std::string("FreeBodyLoadSubcase") + " defines",
                    "add the " + std::string("FreeBodyLoadSubcase") + " card, or name the subsystem of another one");
            });
    }
};

} // namespace

void addFreeBodySubsystemCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<FreeBodySubsystemCheck>());
}

} // namespace oor::analysis
