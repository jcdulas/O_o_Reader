#pragma once

#include "oor/analysis/mass/MassCalculator.hpp"
#include "oor/analysis/mass/NonStructuralMass.hpp"
#include "oor/query/named/common/CardFacts.hpp"
#include "oor/query/named/common/CardList.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace oor::query
{

// The mass of every element of a list, with where the mass is and what it is made of, one value per element in the
// order of the list. An element whose mass cannot be computed has NaN (the totals leave it out); an element with no
// mass (a spring, a rigid element) has 0.
struct ElementMassData
{
    std::vector<double> mass;
    std::vector<double> structural;
    std::vector<double> nonStructural;
    std::vector<double> cx;
    std::vector<double> cy;
    std::vector<double> cz;
    std::vector<std::int32_t> material; // the material of the structural mass, 0 for none or several
    // The inertia about its own centre of the elements that have one (concentrated masses, beams), by position in the
    // list.
    std::unordered_map<std::size_t, base::Mat3> inertia;
};

// The options the named queries use for the mass: the NSM set the Case Control selects, no WTMASS (the masses of the
// deck's own unit system), as `element.mass()` does.
class MassSetting
{
public:
    explicit MassSetting(const model::Model& model, base::ThreadPool* pool);
    [[nodiscard]] analysis::MassOptions options() const noexcept
    {
        analysis::MassOptions options;
        options.nonStructural = m_nonStructural ? &*m_nonStructural : nullptr;
        return options;
    }

private:
    std::optional<analysis::NonStructuralMass> m_nonStructural;
};

// The main material of each element of a list (`propIds` its properties, 0 for none): the one the card gives (a
// CONROD), else the first one of its property (MID1 of a PSHELL, else MID2; the first ply of a laminate); 0 for none.
[[nodiscard]] std::vector<std::int32_t> mainMaterials(model::Model& model, CardFacts& facts, const CardList& list,
                                                      const std::vector<std::int32_t>& pids);

// The card types of the fasteners: CBUSH, CFAST, CWELD, the scalar springs CELAS1 to CELAS4 and CBUSH1D (the elements
// of `fastenersTable`, `fastenerForces` and the fastener margins).
[[nodiscard]] std::vector<std::uint16_t> fastenerElementTypes();

// The numbers about the elements of a list that several queries share, each made the first time it is asked for. The
// list and the model must outlive the object.
class ElementData
{
public:
    ElementData(std::shared_ptr<model::Model> model, const CardList& list, ExecutionContext& context);

    [[nodiscard]] const CardList& list() const noexcept { return m_list; }
    // The property ID of each element (0 for an element with none).
    [[nodiscard]] const std::vector<std::int32_t>& properties();
    [[nodiscard]] const std::vector<double>& lengths();
    [[nodiscard]] const std::vector<double>& areas();
    [[nodiscard]] const std::vector<double>& volumes();
    [[nodiscard]] const ElementMassData& masses();
    // For a property that is a laminate: the share of the structural mass of its elements that each material carries
    // (the plies' density times thickness, summed per material; the shares add to 1). Null for any other property.
    [[nodiscard]] const std::vector<std::pair<std::int32_t, double>>* laminateShares(std::int32_t pid);

private:
    std::shared_ptr<model::Model> m_model;
    const CardList& m_list;
    ExecutionContext& m_context;
    ListColumns m_columns;
    std::optional<std::vector<std::int32_t>> m_properties;
    std::optional<std::vector<double>> m_lengths;
    std::optional<std::vector<double>> m_areas;
    std::optional<std::vector<double>> m_volumes;
    std::optional<ElementMassData> m_masses;
    std::map<std::int32_t, std::optional<std::vector<std::pair<std::int32_t, double>>>> m_shares;
};

} // namespace oor::query
