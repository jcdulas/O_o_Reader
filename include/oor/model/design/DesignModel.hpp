#pragma once

#include "oor/base/ids/CardKey.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/optimisation/DesignField.hpp"
#include "oor/model/cards/optimisation/DesignTerms.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/model/design/DesignableCard.hpp"
#include "oor/model/expressions/EquationSet.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

// The design model of a deck (SOL 200): the design variables, the DLINK cards that make some of them depend on others,
// the constants of the DTABLE and DTABLE2 cards, the equations, and the relations (DVPREL1/2, DVMREL1/2, DVCREL1/2)
// that turn the values of the variables into fields of properties, materials and elements. `DesignModel::read` takes
// all of that from a model; `evaluate` computes the fields a design point gives; `applyDesign` (DesignUpdate.hpp)
// writes them in one atomic operation. The DVGRID cards (`shapeTerms`) move grid points: their moves need the
// coordinate systems, so `analysis::DesignApplication` computes them and hands them to `applyDesign`.
//
// What is not applied: DVSHAP and DVBSHAP (they move grid points along basis vectors that are not in the deck), TOPVAR,
// TOMVAR and BEADVAR (the regions of a topology, topometry and topography optimisation: the solver makes their
// variables), DVLREL1 (a designed load) and DVPSURF (a designed control surface setting of a trim), and a DVPREL1 on a
// ply of the laminates by its global ply ID (TYPE GPLY). They are listed in `notApplied()` and left alone. A DVLREL1
// and a DVPSURF name design variables, which count as used (`isUsed`); a TOMVAR and a BEADVAR name none.

namespace oor::model
{

// A DESVAR. `link` is the ID of the DLINK that makes the variable dependent (0 for an independent one).
struct DesignVariable
{
    base::EntityId id = 0;
    std::string label;
    double initial = 0.0; // XINIT
    double lower = 0.0;   // XLB
    double upper = 0.0;   // XUB
    // The fraction the variable may change in one cycle: DELXV of the card, else DELX of the DOPTPRM (0.5 without one).
    double moveLimit = 0.5;
    base::EntityId link = 0;
    base::EntityId discreteValues = 0; // DDVAL: the DDVAL card of the values the variable may take (0: continuous)
    base::CardKey key;
    std::uint32_t row = 0;
};

// A DLINK: `dependent` = `constant` + `multiplier` * (the sum of the coefficient times the independent variable).
struct DesignLink
{
    base::EntityId id = 0;
    base::EntityId dependent = 0;
    double constant = 0.0;
    double multiplier = 1.0;
    std::vector<DesignTerm> terms;
};

enum class DesignTargetKind : std::uint8_t
{
    Property,
    Material,
    Element,
};

enum class DesignRelationShape : std::uint8_t
{
    Linear,   // DVxREL1: C0 + the sum of the coefficient times the variable
    Equation, // DVxREL2: a DEQATN of the variables and the constants
};

// One relation card. The members before `applicable` say what the card says; the members after it are what the model
// makes of it: the card it drives and the field, the limits in force, and the reason when the relation cannot be
// applied.
struct DesignRelation
{
    std::string card; // the card as the messages name it, the keyword of its type: "DVPREL1", "DVMREL2", ...
    std::uint16_t cardType = 0;
    base::CardKey key;
    std::uint32_t row = 0;
    base::EntityId id = 0;
    DesignTargetKind targetKind = DesignTargetKind::Property;
    DesignRelationShape shape = DesignRelationShape::Linear;
    std::string targetCard;  // TYPE: the neutral card name ("ShellProp", "IsoMat", "PointMassInertia")
    std::string targetLabel; // the keyword of targetCard that the messages show ("PSHELL")
    base::EntityId targetId = 0;
    DesignField field;                // as the deck wrote it
    std::optional<double> lowerLimit; // PMIN, MPMIN, CPMIN as written
    double upperLimit = 1.0e20;       // PMAX, MPMAX, CPMAX
    // Linear: the constant, the terms and PVAL (the first coefficient is the value the field has in the model).
    double constant = 0.0;
    std::vector<DesignTerm> terms;
    bool fromProperty = false;
    // Equation: the DEQATN, the variables and the labels of the constants it receives, in this order.
    base::EntityId equationId = 0;
    std::vector<base::EntityId> variables;
    std::vector<std::string> labels;

    bool applicable = false;
    std::string problem; // why the relation cannot be applied (empty when it can)
    // True when the card the relation names is there and has the type TYPE says, and the card has no such field (or not
    // one a relation can drive): the case of the check `design.relation-target`.
    bool badField = false;
    std::optional<CardLocation> target;
    const DesignableCard* fields = nullptr; // the functions of the card type, when it has some
    DesignFieldFacts facts;                 // the field, when it was resolved
    double lowest = -1.0e35;                // the limits in force: PMIN of the card or its default, and PMAX
    double highest = 1.0e20;
    bool defaultsApplied =
        true; // false when the format reference does not apply the default limits (one variable and C0 = 0)

    // True when the relation is a function of the variable, whatever the coefficient.
    [[nodiscard]] bool usesVariable(base::EntityId variable) const;
};

// A DVGRID: the design variable moves the grid point by `coefficient` * (X - X0) * `vector`, X its value and X0 its
// initial value, the vector given in the coordinate system `system` (0: basic) at the grid point (
// the vector is not made a unit vector). Several cards on one grid point add.
struct DesignShapeTerm
{
    base::EntityId variable = 0;
    base::EntityId grid = 0;
    base::EntityId system = 0;
    double coefficient = 0.0;
    std::array<double, 3> vector = {};
};

// A card of the design model that `apply` does not carry out, with the reason.
struct DesignNotApplied
{
    std::string card; // the card type: "DesignBeadRegion", "DesignLoadRelation1", ...
    base::EntityId id = 0;
    std::string reason;
};

// A set of values of the design variables, in the order of `DesignModel::variables()`.
struct DesignPoint
{
    std::vector<double> values;
};

enum class DesignLimitHit : std::int8_t
{
    Minimum = -1,
    None = 0,
    Maximum = 1,
};

// What a design point gives a relation. NaN where the relation could not be evaluated (`problem` says why).
struct DesignRelationValue
{
    double current = 0.0;   // the field in the model
    double requested = 0.0; // what the relation gives
    double value = 0.0; // the value to write: `requested` within the limits, or as it is when the limits are ignored
    DesignLimitHit limit = DesignLimitHit::None; // which limit `requested` crossed
    std::string problem;
};

struct DesignEvaluation
{
    std::vector<double> variables;              // the values of the variables, the dependent ones from their DLINK
    std::vector<std::int8_t> variableBounds;    // -1 under the lower bound, 0 within, 1 over the upper bound
    std::vector<DesignRelationValue> relations; // aligned with `DesignModel::relations()`
};

class DesignModel
{
public:
    // Reads the design model of `model`: nothing is changed and nothing is evaluated.
    [[nodiscard]] static DesignModel read(const Model& model);

    [[nodiscard]] const std::vector<DesignVariable>& variables() const noexcept { return m_variables; }
    [[nodiscard]] const std::vector<DesignLink>& links() const noexcept { return m_links; }
    [[nodiscard]] const std::vector<DesignRelation>& relations() const noexcept { return m_relations; }
    [[nodiscard]] const std::vector<DesignNotApplied>& notApplied() const noexcept { return m_notApplied; }
    // The DVGRID cards, in the order of the deck.
    [[nodiscard]] const std::vector<DesignShapeTerm>& shapeTerms() const noexcept { return m_shapeTerms; }
    // The problems of the cards that make the variables undefined (a DLINK that names a variable no DESVAR defines, two
    // DLINK cards for one variable): a point cannot be evaluated while there is one.
    [[nodiscard]] const std::vector<std::string>& problems() const noexcept { return m_problems; }

    // True when some card uses the design variable: a relation, a DVGRID, DVSHAP or DVBSHAP, a DVLREL1 or a DVPSURF,
    // the DESVAR group of a DRESP2 or DRESP3, or a DLINK whose dependent variable is used (its independent variables
    // are then).
    [[nodiscard]] bool isUsed(base::EntityId variable) const { return m_used.contains(variable); }

    // The design variable that drives a property, a material or an element: the first variable of the first relation
    // (in the order of `relations()`) on that card that can be applied and drives a field the mass depends on (a
    // thickness, an area, a density, a non-structural mass), the first term of a linear relation, the first variable of
    // an equation; none when no relation drives it.
    [[nodiscard]] std::optional<base::EntityId> variableDriving(DesignTargetKind kind, base::EntityId id) const;

    // The index in variables() of a variable by ID or by label (the label is compared without regard to case; a label
    // that two variables share is an error, DesignError).
    [[nodiscard]] std::optional<std::size_t> findVariable(base::EntityId id) const;
    [[nodiscard]] std::optional<std::size_t> findVariable(const std::string& label) const;

    // The initial values: the XINIT of every variable.
    [[nodiscard]] DesignPoint initialPoint() const;

    // The constant a label names (a DTABLE value, or the field of the card a DTABLE2 entry names in `model`, as the
    // model holds it now); nullopt when the label is unknown or has no value, and `reason` then says why (a table of a
    // frequency or a time, a DTABLE2 on a field the library cannot read).
    [[nodiscard]] std::optional<double> constant(const Model& model, const std::string& label,
                                                 std::string* reason = nullptr) const;

    // The value every relation gives at a point, and the values of the variables (the dependent ones from their DLINK:
    // a value given for them is ignored). `model` is the model this design was read from, unchanged since. A relation
    // that cannot be applied or evaluated has NaN values and its problem in the evaluation; with `applyLimits` a value
    // outside the limits of its relation is brought to the limit (`limit` says which was crossed either way). Throws
    // DesignError when the model changed since the design was read, the point has the wrong number of values, or a card
    // of the design model has a problem that leaves the variables undefined (`problems()`).
    [[nodiscard]] DesignEvaluation evaluate(const Model& model, const DesignPoint& point, bool applyLimits) const;

    // The number of the model's content this was read from.
    [[nodiscard]] std::uint64_t contentVersion() const noexcept { return m_contentVersion; }

    // The equations of the model that parse, by DEQATN ID.
    [[nodiscard]] const EquationSet* equation(base::EntityId id) const;

private:
    friend class DesignReader;

    // A DTABLE2 entry: the field of a card of the model that a label stands for.
    struct FieldConstant
    {
        std::string cardName;
        base::EntityId id = 0;
        std::string field;
    };

    std::vector<DesignVariable> m_variables;
    std::vector<DesignLink> m_links;
    std::vector<DesignRelation> m_relations;
    std::vector<DesignNotApplied> m_notApplied;
    std::vector<DesignShapeTerm> m_shapeTerms;
    std::vector<std::string> m_problems;
    std::set<base::EntityId> m_used;
    std::map<base::EntityId, EquationSet> m_equations;
    std::map<std::string, double> m_values;                // the DTABLE constants, by upper case label
    std::map<std::string, std::string> m_unavailable;      // the labels that have no value, and why
    std::map<std::string, FieldConstant> m_fieldConstants; // the DTABLE2 entries
    std::uint64_t m_contentVersion = 0;
};

} // namespace oor::model
