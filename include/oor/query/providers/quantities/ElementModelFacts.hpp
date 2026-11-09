#pragma once

#include "oor/query/results/ResultSets.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>

namespace oor::query
{

// What the model says of the elements of a result set that the recomputations of a stress, a strain or a force need:
// the thickness and the isotropic material of a shell, the area of a rod, the material of a solid. Looked up by the
// element ID the file uses, through the attachment (so the answer follows a renumbering of the model). An element the
// model cannot describe for the purpose says why (`why`).
struct ShellFacts
{
    bool ok = false;
    std::string why;
    double thickness = 0.0;
    double z1 = 0.0; // the fibre distances of the property
    double z2 = 0.0;
    double e = 0.0;
    double nu = 0.0;
    double g = 0.0;
    std::int32_t pid = 0;
};

struct RodFacts
{
    bool ok = false;
    std::string why;
    double area = 0.0;
    double torsion = 0.0;     // J
    double coefficient = 0.0; // C
    double e = 0.0;
    double g = 0.0;
    std::int32_t pid = 0;
};

struct SolidFacts
{
    bool ok = false;
    std::string why;
    double e = 0.0;
    double nu = 0.0;
    double g = 0.0;
    std::int32_t pid = 0;
};

class ElementModelFacts
{
public:
    ElementModelFacts(const ResultQueryContext& context, std::size_t set);

    // True when the result set has a model at all.
    [[nodiscard]] bool hasModel() const noexcept { return m_model != nullptr; }
    // Plain shells whose property is a PSHELL with one isotropic MAT1 for membrane and bending (and no coupling).
    [[nodiscard]] const ShellFacts& shell(std::int32_t resultEid);
    // Rods (CROD, CONROD) with a PROD and a MAT1.
    [[nodiscard]] const RodFacts& rod(std::int32_t resultEid);
    // Solids with a PSOLID and a MAT1.
    [[nodiscard]] const SolidFacts& solid(std::int32_t resultEid);

private:
    const ResultQueryContext& m_context;
    std::size_t m_set;
    const model::Model* m_model;
    std::map<std::int32_t, ShellFacts> m_shells;
    std::map<std::int32_t, RodFacts> m_rods;
    std::map<std::int32_t, SolidFacts> m_solids;
};

} // namespace oor::query
