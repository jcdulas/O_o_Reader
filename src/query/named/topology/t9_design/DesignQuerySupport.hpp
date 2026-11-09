#pragma once

#include "oor/analysis/mass/MassCalculator.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/model/design/DesignModel.hpp"
#include "oor/model/design/DesignUpdate.hpp"
#include "oor/query/named/common/CardFacts.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/common/ElementData.hpp"

#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// What the queries of the design model (family T9) share: the Case Control commands that pick the objective and the
// constraints, which cards a design variable drives, and the mass of the elements it drives.

namespace oor::query
{

// What the Case Control says about the design: DESOBJ names the response that is the objective (DESOBJ(MIN) and
// DESOBJ(MAX); MIN when neither is written), DESSUB and DESGLB the sets of constraints a subcase has (a DCONADD set
// stands for the DCONSTR sets it adds; DESGLB applies to every subcase).
struct DesignCase
{
    std::optional<base::EntityId> objective;
    bool maximise = false;
    std::map<base::EntityId, std::vector<std::int32_t>> subcasesOfSet; // DCONSTR set -> subcase numbers, ascending
};

[[nodiscard]] DesignCase readDesignCase(const model::Model& model);

// The DCONSTR sets a DESSUB or DESGLB value selects, ascending: the members of the DCONADD of that number, and the
// number itself when a DCONSTR set has it or no DCONADD does.
[[nodiscard]] std::vector<base::EntityId> selectedConstraintSets(const model::Model& model, base::EntityId selected);

// For each design variable, the relations it moves: the ones that name it, and (through a DLINK) the ones of the
// variables that depend on it. Indexes into DesignModel::relations(), ascending.
[[nodiscard]] std::vector<std::vector<std::size_t>> relationsMovedBy(const model::DesignModel& design);

// The elements and properties a relation drives, in the elements of one list.
struct RelationDrive
{
    std::vector<base::EntityId>
        properties;                    // distinct, ascending: the property it names, or the ones that use its material
    std::vector<std::size_t> elements; // positions in DrivenCards::elements(), ascending
};

// Which elements each relation drives: a DVPREL the elements of the property, a DVMREL the elements of the properties
// that name the material (a laminate ply, a plate), a DVCREL the element it names. The element list is every element
// of the model.
class DrivenCards
{
public:
    DrivenCards(std::shared_ptr<model::Model> model, const model::DesignModel& design, ExecutionContext& context);

    [[nodiscard]] const CardList& elements() const noexcept { return m_elements; }
    [[nodiscard]] ElementData& data() noexcept { return m_data; }
    // What a relation drives (nothing for a relation whose target does not exist).
    [[nodiscard]] const RelationDrive& of(std::size_t relation);
    // What a set of relations drives, together.
    [[nodiscard]] RelationDrive of(const std::vector<std::size_t>& relations);
    // True when some relation drives the property (names it, or names a material it uses) or the element (names it, or
    // drives its property), whether or not the relation can be applied.
    [[nodiscard]] bool isPropertyDriven(base::EntityId property) const
    {
        return m_drivenProperties.contains(property);
    }
    [[nodiscard]] bool isElementDriven(std::size_t position);

private:
    std::shared_ptr<model::Model> m_model;
    const model::DesignModel& m_design;
    CardList m_elements;
    ElementData m_data;
    std::unordered_map<base::EntityId, std::vector<std::size_t>> m_byProperty; // elements of each property
    std::unordered_map<base::EntityId, std::size_t> m_byId;                    // position of each element
    std::unordered_map<base::EntityId, std::vector<base::EntityId>> m_propertiesOfMaterial;
    std::map<std::size_t, RelationDrive> m_cache;
    std::unordered_set<base::EntityId> m_drivenProperties;
    std::unordered_set<base::EntityId> m_drivenElements;
};

// A copy of the model with a design point applied through `model.design.apply`, DVGRID included (a relation or a DVGRID
// that cannot be applied is left out). `values` gives the design variables by ID; those it leaves out keep their
// initial value, and a value for a variable that no DESVAR defines is ignored. `design` is the design model of `model`.
// The limits of the relations are not applied unless `limits` says so: the values of an optimisation are the ones that
// kept the fields within them already.
[[nodiscard]] model::Model modelAt(const model::Model& model, const model::DesignModel& design,
                                   const std::map<std::int32_t, double>& values,
                                   model::DesignLimitMode limits = model::DesignLimitMode::Ignore);

// The mass of each element of a list, with its property and the material of its structural mass, in the order of the
// list: the mass is NaN for an element whose mass cannot be computed, the property and material are 0 for an element
// that has none (a laminate has no one material). Made on a model that has the elements of the list (a copy that a
// design point was applied to, for instance).
struct ElementMassList
{
    std::vector<double> mass;
    std::vector<std::int32_t> property;
    std::vector<std::int32_t> material;
};
[[nodiscard]] ElementMassList massOfEach(const model::Model& model, const CardList& elements, base::ThreadPool* pool);

// The mass of the elements of a list in a model, the structural and non-structural mass of their cards and of the NSM
// set the Case Control selects, as `element.mass()` gives it; an element whose mass cannot be computed counts nothing.
[[nodiscard]] double massOfElements(const model::Model& model, const std::vector<model::CardLocation>& elements,
                                    base::ThreadPool* pool);

} // namespace oor::query
