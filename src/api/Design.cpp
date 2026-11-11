#include "oor/api/Design.hpp"
#include "oor/model/design/DesignableCard.hpp"

#include "oor/analysis/design/DesignApplication.hpp"

#include <cmath>
#include <set>
#include <stdexcept>

namespace oor::api
{
std::vector<std::string> designableCards()
{
    return model::designableCardNames();
}
model::DesignModel designModel(const Deck& deck)
{
    return model::DesignModel::read(*deck.model());
}

model::DesignUpdate applyDesignPoint(Deck& deck, const std::vector<std::int32_t>& ids,
                                     const std::vector<double>& values, model::DesignLimitMode limits,
                                     bool setInitialValues)
{
    if (ids.size() != values.size())
        throw std::invalid_argument("design variable IDs and values must have the same length");
    const auto design = designModel(deck);
    auto point = design.initialPoint();
    std::set<std::int32_t> seen;
    for (std::size_t i = 0; i < ids.size(); ++i)
    {
        const auto index = design.findVariable(ids[i]);
        if (!index || !seen.insert(ids[i]).second)
            throw std::invalid_argument("unknown or repeated design variable ID: " + std::to_string(ids[i]));
        if (design.variables()[*index].link != 0)
            throw std::invalid_argument("a dependent design variable is computed by its link: " +
                                        std::to_string(ids[i]));
        if (!std::isfinite(values[i]))
            throw std::invalid_argument("design variable values must be finite");
        point.values[*index] = values[i];
    }
    if (!design.notApplied().empty())
        throw std::invalid_argument("the model contains design declarations that cannot be applied: " +
                                    design.notApplied().front().card);
    const auto evaluation = design.evaluate(*deck.model(), point, false);
    for (const auto bound : evaluation.variableBounds)
        if (bound != 0)
            throw std::invalid_argument("a design variable is outside its bounds; nothing was written");
    model::DesignApplyOptions options;
    options.limits = limits;
    options.setInitialValues = setInitialValues;
    return analysis::DesignApplication::apply(*deck.model(), design, point, options);
}
} // namespace oor::api
