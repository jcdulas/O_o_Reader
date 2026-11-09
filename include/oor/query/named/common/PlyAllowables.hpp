#pragma once

#include "oor/model/core/Model.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace oor::query
{

// The allowables of the plies of the slots of a block (COMPOSITE-API-AUDIT P2): for the element of each row and the
// ply number of each slot, the ply of its laminate (PCOMP, PCOMPG), its material and what the failure theories read of
// it, as the margins and the composite queries resolve them. The plies are listed once each (row r: a ply of a
// property, whatever the elements of the property), and `slots` gives the row of each slot.
struct PlyAllowables
{
    std::vector<std::int32_t> pid;
    std::vector<std::int32_t> ply;   // the number of the ply in its stack (1 the first)
    std::vector<std::int32_t> plyId; // the global ID of a PCOMPG ply, 0 for a PCOMP
    std::vector<std::int32_t> mid;   // 0 for none
    std::vector<double> theta;
    std::vector<double> thickness;
    // The theory of the ply by its code (the position in plyTheoryNames: the theory asked for, else the FT of the
    // property, HFAIL as Hashin), -1 for none.
    std::vector<std::int8_t> theory;
    // What the theory of the ply reads, as the MAT8 holds it (a blank as the card reads it): the strengths of
    // its HFAIL group for Hashin when the MAT8 has one (st its HF11; NaN otherwise, Hashin then taking S), the F12 of
    // Tsai-Wu, the moduli of the maximum strain theory and STRN; NaN (and false) without a MAT8.
    std::vector<double> xt;
    std::vector<double> xc;
    std::vector<double> yt;
    std::vector<double> yc;
    std::vector<double> s;
    std::vector<double> f12;
    std::vector<double> st;
    std::vector<double> e1;
    std::vector<double> e2;
    std::vector<double> nu12;
    std::vector<double> g12;
    std::vector<std::uint8_t> strainAllowables;
    std::vector<double> sb; // the SB of the laminate, the allowable interlaminar shear (NaN when blank)
    // the row of the ply of each slot (element * locations + slot), -1 for none
    std::vector<std::int32_t> slots;

    [[nodiscard]] std::size_t rows() const noexcept { return pid.size(); }
};

// The names of the theories by their code: "maxStress", "maxStrain", "hill", "hoffman", "tsaiWu", "hashin",
// "puck", "larc03" (the codes of nr.kernels.ply_theories).
[[nodiscard]] const std::vector<std::string>& plyTheoryNames();

// The plies of `elementIds` at the ply numbers `layers` (elements * locations, row by row; a number below 1 or past
// the stack: no ply), by `theory` ("" the property's). QueryError for a theory that is not one of plyTheoryNames.
[[nodiscard]] PlyAllowables plyAllowables(const model::Model& model, const std::vector<std::int32_t>& elementIds,
                                          const std::vector<std::int32_t>& layers, std::size_t locations,
                                          const std::string& theory);

} // namespace oor::query
