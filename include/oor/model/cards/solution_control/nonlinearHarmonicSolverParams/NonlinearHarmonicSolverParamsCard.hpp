#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace oor::model
{

// The value of a parameter: an integer, a real or a word (DECOMP), as `NonlinearHarmonicSolverParamsCatalogue.hpp` says
// each parameter takes it. A word is kept in upper case, and a real parameter written as an integer holds the real that
// is whole.
using NonlinearHarmonicSolverParamsValue = std::variant<std::int32_t, double, std::string>;

// A parameter and its value.
struct NonlinearHarmonicSolverParamsParameter
{
    std::string name; // upper case
    NonlinearHarmonicSolverParamsValue value;

    [[nodiscard]] bool operator==(const NonlinearHarmonicSolverParamsParameter&) const noexcept = default;
};

// One NHRMPRM card: values for the parameters of the nonlinear frequency response analysis (SOL 128). The twelve
// parameters of the format are in `NonlinearHarmonicSolverParamsCatalogue.hpp` with their kind, default and the values
// they may have.
//
// Card layout: NHRMPRM  PARAM1  VAL1  PARAM2  VAL2     fields 1 to 4 (the layout has two pairs)
// In this library's numbering the pairs sit side by side from field 1, four to a line, and go on over the
// continuation lines: a name and its value in the next field. The format draws two pairs and no continuation, but it
// lists twelve names, so the card keeps any number of pairs and reads a pair whose two fields are blank as padding. A
// name the catalogue does not list is kept with its value as written, an integer, a real or a word (a later release may
// add parameters); a listed name takes the values the catalogue gives it. The format says some of them can also be
// given by PARAM, which then wins: a rule between cards. It does not limit the number of NHRMPRM cards, and DECOMP = QR
// needs CONTP = 2, which the card does not check since CONTP may be on another card.
struct NonlinearHarmonicSolverParamsCard
{
    std::vector<NonlinearHarmonicSolverParamsParameter> parameters;

    [[nodiscard]] bool operator==(const NonlinearHarmonicSolverParamsCard&) const noexcept = default;
};

} // namespace oor::model
