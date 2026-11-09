#include "oor/analysis/validation/engine/CheckRegistry.hpp"

#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/case_control/CaseControlChecks.hpp"
#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/constraints/ConstraintChecks.hpp"
#include "oor/analysis/validation/contact/ContactChecks.hpp"
#include "oor/analysis/validation/coordinates/CoordinateChecks.hpp"
#include "oor/analysis/validation/design/DesignChecks.hpp"
#include "oor/analysis/validation/explicit/ExplicitChecks.hpp"
#include "oor/analysis/validation/fields/FieldChecks.hpp"
#include "oor/analysis/validation/geometry/GeometryChecks.hpp"
#include "oor/analysis/validation/loads/LoadChecks.hpp"
#include "oor/analysis/validation/parts/PartChecks.hpp"
#include "oor/analysis/validation/references/ReferenceChecks.hpp"
#include "oor/analysis/validation/solution/SolutionChecks.hpp"
#include "oor/analysis/validation/superelement/SuperelementChecks.hpp"

#include <algorithm>
#include <stdexcept>

namespace oor::analysis
{

// Builds the registry: the families in order, each registering its checks in the order they run.
struct CheckRegistryBuilder
{
    static void fill(CheckRegistry& registry)
    {
        registerReferenceChecks(registry);
        registerFieldChecks(registry);
        registerCompatibilityChecks(registry);
        registerConstraintChecks(registry);
        registerLoadChecks(registry);
        registerCoordinateChecks(registry);
        registerSolutionChecks(registry);
        registerCaseControlChecks(registry);
        registerAeroChecks(registry);
        registerContactChecks(registry);
        registerExplicitChecks(registry);
        registerSuperelementChecks(registry);
        registerDesignChecks(registry);
        registerPartChecks(registry);
        registerGeometryChecks(registry);
    }
};

const std::vector<CheckFamily>& CheckRegistry::families()
{
    static const std::vector<CheckFamily> FAMILIES = {
        {"references", "ref", "References and duplicate IDs"},
        {"fields", "field", "Required fields and value rules"},
        {"compatibility", "compat", "Element, property and material compatibility"},
        {"constraints", "constraint", "Constraints and rigid elements"},
        {"loads", "load", "Loads"},
        {"coordinates", "coord", "Coordinate systems"},
        {"solution", "solution", "Solution control"},
        {"case_control", "case", "Case Control"},
        {"aero", "aero", "Aeroelasticity"},
        {"contact", "contact", "Contact"},
        {"explicit", "explicit", "Explicit solver (SOL 700)"},
        {"superelement", "superelement", "Superelements"},
        {"design", "design", "Design optimisation"},
        {"parts", "part", "Parts"},
        {"geometry", "geometry", "Geometry (opt-in)"},
    };
    return FAMILIES;
}

const CheckRegistry& CheckRegistry::instance()
{
    static const CheckRegistry REGISTRY = []
    {
        CheckRegistry registry;
        CheckRegistryBuilder::fill(registry);
        return registry;
    }();
    return REGISTRY;
}

void CheckRegistry::add(std::unique_ptr<ValidationCheck> check)
{
    const CheckInfo& info = check->info();
    if (familyIndex(info.family) == families().size())
        throw std::logic_error("check " + std::string(info.code) + " is in the unknown family " +
                               std::string(info.family));
    if (find(info.code) != nullptr)
        throw std::logic_error("two checks have the code " + std::string(info.code));
    // The checks of a family are consecutive and the families come in order: a check is added after the ones of its
    // family and before the ones of the next.
    if (!m_checks.empty() && familyIndex(m_checks.back()->info().family) > familyIndex(info.family))
        throw std::logic_error("check " + std::string(info.code) + " is registered out of the order of the families");
    m_checks.push_back(std::move(check));
}

const ValidationCheck* CheckRegistry::find(std::string_view code) const noexcept
{
    const auto found = std::ranges::find_if(m_checks, [code](const auto& check) { return check->info().code == code; });
    return found == m_checks.end() ? nullptr : found->get();
}

std::size_t CheckRegistry::familyIndex(std::string_view family) noexcept
{
    const std::vector<CheckFamily>& list = families();
    const auto found = std::ranges::find_if(list, [family](const CheckFamily& entry) { return entry.name == family; });
    return static_cast<std::size_t>(found - list.begin());
}

bool CheckRegistry::matches(std::string_view name, const CheckInfo& info) noexcept
{
    if (name == info.code || name == info.family || name == "all")
        return true;
    const std::size_t dot = info.code.find('.');
    return dot != std::string_view::npos && info.code.substr(0, dot) == name;
}

bool CheckRegistry::isKnownName(std::string_view name) const noexcept
{
    return std::ranges::any_of(m_checks, [name](const auto& check) { return matches(name, check->info()); }) ||
           std::ranges::any_of(
               families(), [name](const CheckFamily& family) { return family.name == name || family.prefix == name; });
}

} // namespace oor::analysis
