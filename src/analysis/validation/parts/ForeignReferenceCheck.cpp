#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/parts/PartChecks.hpp"
#include "oor/model/references/ReferenceChecker.hpp"

#include <memory>
#include <unordered_map>

namespace oor::analysis
{

namespace
{

// A card that refers to an ID its own part does not define and another part (or the residual structure) does: an
// element of a superelement on a grid point of the residual structure. Each part is a model of its own; the ID has to
// be defined in it. The same reference is also reported, as unresolved, by ref.unresolved.
class ForeignReferenceCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"part.foreign-reference", base::Severity::Error, "parts",
                                    "A card refers to an ID that exists only in another part of the deck."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        if (context.allModels().size() < 2)
            return;
        // a resolver a part, made when a reference first looks in it
        std::unordered_map<const model::Model*, std::unique_ptr<model::ReferenceResolver>> resolvers;
        const auto resolverOf = [&](const model::Model* part) -> model::ReferenceResolver&
        {
            std::unique_ptr<model::ReferenceResolver>& resolver = resolvers[part];
            if (!resolver)
                resolver = std::make_unique<model::ReferenceResolver>(*part);
            return *resolver;
        };
        for (const UnresolvedUse& use : context.references().unresolved)
        {
            if (use.inCaseControl)
                continue;
            for (const model::Model* other : context.allModels())
            {
                if (other == &context.model() || !resolverOf(other).resolve(use.target, use.id))
                    continue;
                const std::string where = other->partInfo() ? other->partLabel() : "the residual structure";
                sink.reportCard(use.location, use.field,
                                "refers to " + std::string(describeTarget(use.target)) + " " + std::to_string(use.id) +
                                    ", which exists only in " + where,
                                "define it in this part: a part has its own IDs");
                break;
            }
        }
    }
};

} // namespace

void addForeignReferenceCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ForeignReferenceCheck>());
}

} // namespace oor::analysis
