#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/fields/FieldChecks.hpp"
#include "oor/model/cards/parameters/parameter/ParameterTable.hpp"
#include "oor/model/parameters/ParameterCatalogue.hpp"

namespace oor::analysis
{

namespace
{

// A parameter of the format reference given a value of another kind: a word for a real (PARAM,WTMASS,YES), a real for
// an integer (PARAM,POST,-1.5). The reader keeps the card and warns; this finds it again in a model that was edited,
// and in the parts. An integer where a real is expected is accepted, as the solver does.
class ParameterKindCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "field.parameter-kind", base::Severity::Warning, "fields",
        "A parameter of the format reference has a value of another kind than the one it takes."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::ParameterCatalogue& catalogue = model::ParameterCatalogue::instance();
        forEachCard<model::ParameterTable>(
            context.model(),
            [&](const model::ParameterCard& card, model::CardLocation where)
            {
                const auto problem = catalogue.problemWith(card);
                if (problem && problem->fit == model::ParameterFit::WrongKind)
                    sink.reportCard(where, "value1", problem->message,
                                    "give the parameter a value of the kind the format reference lists");
            });
    }
};

} // namespace

void addParameterKindCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ParameterKindCheck>());
}

} // namespace oor::analysis
