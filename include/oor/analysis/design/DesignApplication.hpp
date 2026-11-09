#pragma once

#include "oor/model/core/Model.hpp"
#include "oor/model/design/DesignModel.hpp"
#include "oor/model/design/DesignUpdate.hpp"

namespace oor::analysis
{

// Gives a model a design point, shape included (`model.design.apply`): the fields the relations
// drive (model::applyDesign) and the grid points the DVGRID cards move (DesignShapeChange), in one operation that
// either completes or leaves the model as it was.
class DesignApplication
{
public:
    // A DVGRID that cannot be applied refuses the whole point (model::DesignError, naming the card), unless
    // `options.skipUnapplicable` leaves it out: it is then in `skippedShapes`, with its reason in `shapeProblems`.
    [[nodiscard]] static model::DesignUpdate apply(model::Model& model, const model::DesignModel& design,
                                                   const model::DesignPoint& point,
                                                   const model::DesignApplyOptions& options = {});
};

} // namespace oor::analysis
