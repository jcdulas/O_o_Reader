#include "AeroCheckSupport.hpp"
#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/aeroBody2/AeroBody2Table.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// A body needs interference elements and slender body elements to be a body: at least one interference element (format
// reference) and at least two slender body elements. Each is a number (NSB, NINT) or the points of an
// AEFACT, one more than the elements. A list that does not exist is `ref.unresolved`, one that is too short is reported
// here (it makes no element).
class BodyElementsCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "aero.body-elements", base::Severity::Error, "aero",
        "A slender body has fewer than two slender body elements or no interference element."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::AeroBody2Table>(
            model,
            [&](const model::AeroBody2Card& card, model::CardLocation where)
            {
                const std::optional<std::int64_t> slender =
                    divisionElements(model, card.slenderElemCount, card.slenderListId);
                if (slender && *slender < MIN_SLENDER_ELEMENTS)
                {
                    sink.reportCard(
                        where, card.slenderElemCount > 0 ? "slenderElemCount" : "slenderListId",
                        "the body has " + std::to_string(*slender) + " slender body element" +
                            (*slender == 1 ? "" : "s") + ", and it needs at least two",
                        "give " + std::string("slenderElemCount") + " at least 2 or a list of at least three points");
                }
                const std::optional<std::int64_t> interference =
                    divisionElements(model, card.interferenceElemCount, card.interferenceListId);
                if (interference && *interference < MIN_INTERFERENCE_ELEMENTS)
                {
                    sink.reportCard(where,
                                    card.interferenceElemCount > 0 ? "interferenceElemCount" : "interferenceListId",
                                    "the body has no interference element",
                                    "give " + std::string("interferenceElemCount") +
                                        " at least 1 or a list of at least two points");
                }
            });
    }

private:
    static constexpr std::int64_t MIN_SLENDER_ELEMENTS = 2;
    static constexpr std::int64_t MIN_INTERFERENCE_ELEMENTS = 1;
};

} // namespace

void addBodyElementsCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<BodyElementsCheck>());
}

} // namespace oor::analysis
