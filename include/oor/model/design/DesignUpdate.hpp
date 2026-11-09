#pragma once

#include "oor/base/math/Vec3.hpp"
#include "oor/model/design/DesignModel.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace oor::model
{

// What `applyDesign` does with the value a relation gives when it is outside the limits of the relation (PMIN and PMAX
// of the card, or the default PMIN of its field): the optimiser keeps the field within them, so the default clips;
// `Error` refuses the whole design point and names the relation; `Ignore` writes the value as the relation gives it.
enum class DesignLimitMode : std::uint8_t
{
    Clip,
    Error,
    Ignore,
};

struct DesignApplyOptions
{
    DesignLimitMode limits = DesignLimitMode::Clip;
    // A relation that cannot be applied (a card type or a field the library cannot write, a missing target) is left out
    // and reported, instead of refusing the whole design point.
    bool skipUnapplicable = false;
    // Also gives the DESVAR cards the values as their initial value (XINIT), the dependent ones included, so that the
    // deck written afterwards starts from the design point.
    bool setInitialValues = false;
};

// What happened to one field a relation drives.
struct DesignFieldChange
{
    std::size_t relation = 0; // the index in DesignModel::relations()
    double before = 0.0;      // the field in the model
    double requested = 0.0;   // what the relation gave
    double after = 0.0;       // what the model holds now (the limit when the value was clipped)
    DesignLimitHit limit = DesignLimitHit::None;
    bool changed = false; // false when the field holds the value to rounding (it was not written)
};

// A grid point that the DVGRID cards move.
struct DesignGridMove
{
    base::EntityId grid = 0;
    std::uint32_t row = 0; // in the GRID table
    base::Vec3 before;     // X1 X2 X3 in the CP system of the grid point
    base::Vec3 after;
    base::Vec3 displacement; // the move, in the basic system
};

// What the DVGRID cards do at a design point, worked out by analysis::DesignShapeChange: the grid points that move, the
// design variables whose initial value (XINIT) becomes the value of the point (the DVGRID cards move a grid point by
// the change of its variable from XINIT, so the position of a grid point and the XINIT of the variables that move it
// stay a pair: applying a point twice moves the points once), and the DVGRID cards left out (`skipUnapplicable`).
struct DesignShapeUpdate
{
    std::vector<DesignGridMove> moves;
    std::vector<std::size_t> variables; // indexes into DesignModel::variables()
    std::vector<std::size_t> skipped;   // indexes into DesignModel::shapeTerms()
    std::vector<std::string> problems;  // why each skipped card cannot be applied, in the same order
};

struct DesignUpdate
{
    std::vector<double> variables; // the values of the variables the point gave, the dependent ones from DLINK
    std::vector<std::int8_t> variableBounds; // -1 under the lower bound, 0 within, 1 over the upper bound
    std::vector<DesignFieldChange> fields;   // every relation applied, in the order of the relations
    std::vector<std::size_t> skipped;        // the relations left out (skipUnapplicable): their index
    std::size_t changedFields = 0;
    std::vector<DesignGridMove> grids;      // the grid points moved, in the order of the GRID table
    std::vector<std::size_t> skippedShapes; // the DVGRID cards left out (skipUnapplicable): their index
    std::vector<std::string> shapeProblems; // why, aligned with skippedShapes
};

// Applies a design point to the model: evaluates DLINK first (the dependent variables), then every relation, and writes
// the property, material and element fields they give, and the grid point moves of `shape`, all in one operation. The
// design was read from this model and the model has not changed since (DesignError otherwise). Everything that can fail
// is decided before the first write, and a failure while writing puts the model back as it was. A field whose new value
// equals the old one to rounding is not written, so a design point that reproduces the model leaves it, and its content
// version, as they were.
//
// Two relations that drive the same field of the same card are an error. DVSHAP, DVBSHAP and TOPVAR are not applied
// (DesignModel::notApplied); the DVGRID cards are applied when `shape` gives their moves (analysis::DesignApplication).
[[nodiscard]] DesignUpdate applyDesign(Model& model, const DesignModel& design, const DesignPoint& point,
                                       const DesignApplyOptions& options = {}, const DesignShapeUpdate& shape = {});

} // namespace oor::model
