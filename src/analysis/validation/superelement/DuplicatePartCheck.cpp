#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/superelement/SuperelementChecks.hpp"

#include <set>
#include <tuple>

namespace oor::analysis
{

namespace
{

// Two parts of the deck of the same kind and number (two BEGIN SUPER=1): the solver reads the first and the second
// would replace or clash with it. A mass increment of a superelement (BEGIN SUPER=n MASSID=m) is a part of its own,
// told apart by its MASSID, and a module may be opened again with APPEND.
class DuplicatePartCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"superelement.duplicate-part",
                                    base::Severity::Error,
                                    "superelement",
                                    "Two parts of the deck have the same BEGIN kind and number.",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        std::set<std::tuple<std::string, std::int64_t, std::int64_t>> seen;
        for (const std::unique_ptr<model::Model>& part : context.root().parts())
        {
            const std::optional<model::PartInfo>& info = part->partInfo();
            if (info && !info->appends && !seen.emplace(info->kind, info->id, info->massId).second)
                sink.reportStatement("SectionBegin", info->id, part->partLabel() + " is opened twice",
                                     "number the parts differently");
        }
    }
};

} // namespace

void addDuplicatePartCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<DuplicatePartCheck>());
}

} // namespace oor::analysis
