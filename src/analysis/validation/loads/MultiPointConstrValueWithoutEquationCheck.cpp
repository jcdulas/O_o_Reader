#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/loads/LoadChecks.hpp"
#include "oor/model/cards/constraints/multiPointConstrValue/MultiPointConstrValueTable.hpp"
#include "oor/model/cards/constraints/multiPointConstrWithValue/MultiPointConstrWithValueTable.hpp"

#include <set>
#include <string>
#include <utility>

namespace oor::analysis
{

namespace
{

// The component of a degree of freedom as a number that is comparable: a scalar point has its component blank, 0 or 1.
std::int32_t comparable(std::int32_t component) noexcept
{
    return component == 0 ? 1 : component;
}

// An MPCD equation whose degree of freedom is the dependent one of no MPCY: the format reference says
// the point and the component of an MPCD need an MPCY that names them as well (the value of the MPCD replaces the
// right-hand side of that equation).
class MultiPointConstrValueWithoutEquationCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "load.multi-point-constr-value-without-equation", base::Severity::Error, "loads",
        "A multipoint constraint value card gives a value to a degree of freedom that no multipoint constraint with "
        "value has as its dependent one."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        std::set<std::pair<base::EntityId, std::int32_t>> dependent;
        forEachCard<model::MultiPointConstrWithValueTable>(
            model, [&](const model::MultiPointConstrWithValueCard& card, model::CardLocation)
            { dependent.emplace(card.node, comparable(card.component)); });
        forEachCard<model::MultiPointConstrValueTable>(
            model,
            [&](const model::MultiPointConstrValueCard& card, model::CardLocation where)
            {
                for (std::size_t index = 0; index < card.points.size(); ++index)
                {
                    if (card.points[index] == 0 ||
                        dependent.contains({card.points[index], comparable(card.components[index])}))
                        continue;
                    sink.reportCard(
                        where, "dependentNode" + std::to_string(index + 1),
                        "the point " + std::to_string(card.points[index]) + ", component " +
                            std::to_string(card.components[index]) + " is the dependent one of no " +
                            std::string("MultiPointConstrWithValue"),
                        "add the " + std::string("MultiPointConstrWithValue") + " equation of that degree of freedom");
                }
            });
    }
};

} // namespace

void addMultiPointConstrValueWithoutEquationCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<MultiPointConstrValueWithoutEquationCheck>());
}

} // namespace oor::analysis
