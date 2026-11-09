#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/SharedIds.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"

namespace oor::analysis
{

namespace
{

// A move number that two kinds of card have: MDTRAN, MDROT1, MDROT2, MDMIR1 and MDMIR2 each define a move that an
// MDMOVE names by its MVID, and the number must be unique among the five (the format reference: the move number is what
// an MDMOVE names). Two cards of one type are the reader's business (`ref.duplicate-id`).
class ModuleMoveIdRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "compat.module-move-id-repeated", base::Severity::Error, "compatibility",
        "A module translation, rotation or mirror card has the move number of a move of another card."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportIdsSharedByTypes(context, sink, model::CardCategory::ModuleMove, "module move");
    }
};

} // namespace

void addModuleMoveIdRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ModuleMoveIdRepeatedCheck>());
}

} // namespace oor::analysis
