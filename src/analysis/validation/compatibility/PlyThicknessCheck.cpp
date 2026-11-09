#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/properties/compProp/CompPropTable.hpp"
#include "oor/model/cards/properties/compPropGlobal/CompPropGlobalTable.hpp"

namespace oor::analysis
{

namespace
{

template <typename PlyList>
std::string zeroPlies(const PlyList& plies)
{
    std::string numbers;
    std::size_t zeros = 0;
    for (std::size_t index = 0; index < plies.size(); ++index)
    {
        if (plies[index].thickness == 0.0)
        {
            numbers += (numbers.empty() ? "" : ", ") + std::to_string(index + 1);
            ++zeros;
        }
    }
    return zeros == plies.size() ? std::string() : numbers; // every ply zero is the laminate check's business
}

// A ply with no thickness in a laminate whose other plies have some: it adds nothing, and it is usually a blank T.
class PlyThicknessCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.ply-thickness", base::Severity::Warning, "compatibility",
                                    "A ply of a composite property has zero thickness while other plies do not."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachCard<model::CompPropTable>(context.model(),
                                          [&](const model::CompPropCard& card, model::CardLocation where)
                                          {
                                              if (const std::string numbers = zeroPlies(card.plies); !numbers.empty())
                                                  sink.reportCard(where, "plyThickness",
                                                                  "ply " + numbers + " has zero thickness",
                                                                  "give the ply a thickness or remove it");
                                          });
        forEachCard<model::CompPropGlobalTable>(
            context.model(),
            [&](const model::CompPropGlobalCard& card, model::CardLocation where)
            {
                if (const std::string numbers = zeroPlies(card.plies); !numbers.empty())
                    sink.reportCard(where, "plyThickness", "ply " + numbers + " has zero thickness",
                                    "give the ply a thickness or remove it");
            });
    }
};

} // namespace

void addPlyThicknessCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<PlyThicknessCheck>());
}

} // namespace oor::analysis
