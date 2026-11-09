#pragma once

#include "oor/model/core/Model.hpp"
#include "oor/query/providers/quantities/CompositeQuantities.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace oor::query
{

// The allowables of the margins, as the margin queries and `elementAllowables` both resolve them: the caller's, else
// the material card.

// The allowables a caller gave: by element, property, material, or for all, with the optional second number of a pair
// (the shear allowable of a fastener). The keys are "element:eid", "property:pid", "material:mid", "all", with ":1" for
// the second number of a pair.
class CallerAllowables
{
public:
    explicit CallerAllowables(const std::vector<std::pair<std::string, double>>& entries);
    [[nodiscard]] bool any() const noexcept { return !m_values.empty(); }
    // The allowable for an element (element first, then property, then material, then all): the first number, or the
    // second of a pair; nothing when the caller gave none.
    [[nodiscard]] std::optional<double> find(std::int32_t eid, std::int32_t pid, std::int32_t mid, int position) const;

private:
    std::map<std::pair<std::string, std::int64_t>, std::array<std::optional<double>, 2>> m_values;
};

// The allowable of an IsoMat (MAT1): ST for tension, SC for compression (format reference MAT1). NaN when the material
// is not an IsoMat or leaves the field blank; `source` names the field ("MAT1.ST") or says why there is none.
[[nodiscard]] double isoMatAllowable(const model::Model& model, std::int32_t mid, bool compression,
                                     std::string& source);

// The stress limits ST, SC, SS of an IsoMat (MAT1) as the card holds them (0 for a blank field); nothing when the
// material is not an IsoMat.
[[nodiscard]] std::optional<std::array<double, 3>> isoMatStressLimits(const model::Model& model, std::int32_t mid);

// The allowables of an OrthoMat2D (MAT8: Xt, Xc, Yt, Yc, S, F12, the moduli for the maximum strain theory, STRN);
// nothing when the material is not an OrthoMat2D.
[[nodiscard]] std::optional<PlyMaterial> orthoMat2DAllowables(const model::Model& model, std::int32_t mid);

} // namespace oor::query
