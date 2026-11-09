#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/fields/FieldChecks.hpp"
#include "oor/model/cards/parameters/parameter/ParameterTable.hpp"
#include "oor/model/parameters/ParameterCatalogue.hpp"

namespace oor::analysis
{

namespace
{

// A parameter whose format reference entry lists its options as the complete set, given a value of the right kind that
// is not one of them (PARAM,ACOUT,LOUD). Information only: the catalogue lists an option set when the entry gives one,
// and the text of an entry can leave a value out.
class ParameterValueCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"field.parameter-value", base::Severity::Info, "fields",
                                    "A parameter has a value that its format reference entry does not list."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::ParameterCatalogue& catalogue = model::ParameterCatalogue::instance();
        forEachCard<model::ParameterTable>(
            context.model(),
            [&](const model::ParameterCard& card, model::CardLocation where)
            {
                const auto problem = catalogue.problemWith(card);
                if (problem && problem->fit == model::ParameterFit::NotAllowed)
                    sink.reportCard(where, "value1", problem->message,
                                    "use one of the values the format reference lists for the parameter");
            });
    }
};

} // namespace

void addParameterValueCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ParameterValueCheck>());
}

} // namespace oor::analysis
