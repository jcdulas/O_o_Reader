#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/SharedIds.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"

namespace oor::analysis
{

namespace
{

// An event number that an FTGEVNT and an FTGSEQ both have: the two cards are the loading events of a fatigue analysis
// and the sequences of them, a FATIGUE command of the Case Control and an FIDi of an FTGSEQ name either by one number,
// and the format reference asks for it to be unique among the two. Two cards
// of one type are the reader's business (`ref.duplicate-id`).
class FatigueEventIdRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.fatigue-event-id-repeated", base::Severity::Error, "compatibility",
                                    "A fatigue event and a fatigue sequence have the same number."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportIdsSharedByTypes(context, sink, model::CardCategory::FatigueEvent, "fatigue event");
    }
};

} // namespace

void addFatigueEventIdRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<FatigueEventIdRepeatedCheck>());
}

} // namespace oor::analysis
