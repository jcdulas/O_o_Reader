#include "oor/analysis/connections/ContactSearch.hpp"
#include "oor/analysis/validation/contact/ContactChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"

#include <cstdio>

namespace oor::analysis
{

namespace
{

// A pair of contact bodies that a contact table names and whose surfaces do not meet: no grid point of the secondary
// surface lies within the distance of the pair from the primary surface, and none of the primary surface from the
// secondary one when the pair searches both ways. The components do not merge such a pair.
class PairApartCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"contact.pair-apart", base::Severity::Warning, "contact",
                                    "A contact table pairs two bodies whose surfaces do not meet: no grid point of "
                                    "one lies within the contact distance of the other."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        // No pool: a check does not start parallel work of its own (the engine runs the families in parallel).
        const std::shared_ptr<const ContactSearch> search = ContactSearch::of(context.model(), std::nullopt);
        if (!search->fromTables())
            return;
        for (std::size_t index = 0; index < search->pairs().size(); ++index)
        {
            const ContactPair& pair = search->pairs()[index];
            const ContactPairResult& result = search->results()[index];
            if (result.meets || !pair.table)
                continue;
            char distance[32];
            std::snprintf(distance, sizeof(distance), "%.6g", result.tolerance);
            sink.reportCard(*pair.table, "",
                            "the secondary body " + std::to_string(pair.secondary->id) + " (" +
                                std::string(contactBodySourceName(pair.secondary->source)) + ") and the primary body " +
                                std::to_string(pair.primary->id) + " (" +
                                std::string(contactBodySourceName(pair.primary->source)) +
                                ") do not meet: no grid point of one is within " + distance + " of the other",
                            "move the bodies into contact, give the pair a larger " +
                                std::string("ContactDistanceTolerance") + ", or remove the pair");
        }
    }
};

} // namespace

void addPairApartCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<PairApartCheck>());
}

} // namespace oor::analysis
