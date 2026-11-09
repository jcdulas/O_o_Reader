#include "oor/analysis/design/DesignApplication.hpp"

#include "oor/analysis/design/DesignShapeChange.hpp"
#include "oor/model/design/DesignErrors.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

constexpr std::size_t PROBLEMS_SHOWN = 8;

} // namespace

model::DesignUpdate DesignApplication::apply(model::Model& model, const model::DesignModel& design,
                                             const model::DesignPoint& point, const model::DesignApplyOptions& options)
{
    model::DesignShapeUpdate shape;
    if (!design.shapeTerms().empty())
    {
        const model::DesignEvaluation evaluation =
            design.evaluate(model, point, options.limits == model::DesignLimitMode::Clip);
        shape = DesignShapeChange::compute(model, design, evaluation.variables);
        if (!shape.skipped.empty() && !options.skipUnapplicable)
        {
            std::string message = "apply: " + std::to_string(shape.skipped.size()) + " " +
                                  std::string("DesignNodeMove") + " card(s) cannot be applied; nothing was written";
            for (std::size_t index = 0; index < shape.problems.size() && index < PROBLEMS_SHOWN; ++index)
                message += "\n  " + shape.problems[index];
            if (shape.problems.size() > PROBLEMS_SHOWN)
                message += "\n  ... and " + std::to_string(shape.problems.size() - PROBLEMS_SHOWN) + " more";
            throw model::DesignError(message);
        }
    }
    return model::applyDesign(model, design, point, options, shape);
}

} // namespace oor::analysis
