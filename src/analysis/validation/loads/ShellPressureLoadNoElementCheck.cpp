#include "ElementsInRange.hpp"
#include "oor/analysis/validation/loads/LoadChecks.hpp"
#include "oor/model/cards/loads/shellPressureLoad/ShellPressureLoadTable.hpp"

namespace oor::analysis
{

namespace
{

// A PLOAD2 whose element list matches no element: the elements of a THRU range that do not exist are skipped, so a list
// of which none exists loads nothing (and nothing says so). The elements of a part that is the image of another (a
// REPEAT or MIRROR superelement) are those of the part it copies.
class ShellPressureLoadNoElementCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"load.shell-pressure-load-no-element", base::Severity::Warning, "loads",
                                    "No element of a shell pressure load list exists: the card loads nothing."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const model::Model* image = imageOfPart(model);
        forEachCard<model::ShellPressureLoadTable>(
            model,
            [&](const model::ShellPressureLoadCard& card, model::CardLocation where)
            {
                bool any = false;
                for (const model::ElementRun& run : card.elements)
                {
                    forEachElementInRange(model, run.first, run.last, [&](model::CardLocation) { any = true; });
                    if (!any && image != nullptr)
                        forEachElementInRange(*image, run.first, run.last, [&](model::CardLocation) { any = true; });
                    if (any)
                        return;
                }
                sink.reportCard(where, "elemId1", "none of the elements of the list exists", "check the element IDs");
            });
    }
};

} // namespace

void addShellPressureLoadNoElementCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ShellPressureLoadNoElementCheck>());
}

} // namespace oor::analysis
