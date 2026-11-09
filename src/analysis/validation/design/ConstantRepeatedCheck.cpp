#include "oor/analysis/validation/design/DesignChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/optimisation/designConstants/DesignConstantsTable.hpp"
#include "oor/model/cards/optimisation/designFieldConstants/DesignFieldConstantsTable.hpp"

#include <set>
#include <string>

namespace oor::analysis
{

namespace
{

// A constant is found by its label, and the label must be unique over all the DTABLE and DTABLE2 entries (format
// reference). The reader cannot see it: the labels of the entries are not an ID,
// and each card type has its own list. The first definition stands; each later one is reported, at its card, whatever
// the case of the letters (a label is matched without regard to case).
class ConstantRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"design.constant-repeated", base::Severity::Error, "design",
                                    "A design constant label is defined twice."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        std::set<std::string> seen;
        const auto note = [&](const std::string& label, model::CardLocation where)
        {
            if (!seen.insert(std::string(label)).second)
                sink.reportCard(where, "label", "the constant " + label + " is defined more than once",
                                "give each constant one label, over all the " +
                                    cardLabels(context.model(), {"DesignConstants", "DesignFieldConstants"}, "and") +
                                    " cards");
        };
        forEachCard<model::DesignConstantsTable>(context.model(),
                                                 [&](const model::DesignConstantsCard& card, model::CardLocation where)
                                                 {
                                                     for (const model::DesignConstantsEntry& entry : card.entries)
                                                         note(entry.label, where);
                                                 });
        forEachCard<model::DesignFieldConstantsTable>(
            context.model(),
            [&](const model::DesignFieldConstantsCard& card, model::CardLocation where)
            {
                for (const model::DesignFieldConstantsEntry& entry : card.entries)
                    note(entry.label, where);
            });
    }
};

} // namespace

void addConstantRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ConstantRepeatedCheck>());
}

} // namespace oor::analysis
