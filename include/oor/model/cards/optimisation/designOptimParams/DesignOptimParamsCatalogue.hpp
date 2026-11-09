#pragma once

#include "oor/model/cards/optimisation/DesignValue.hpp"

#include <cstdint>
#include <span>
#include <string_view>

namespace oor::model
{

// The kind of value a design optimisation parameter takes.
enum class DesignOptimParamsKind : std::uint8_t
{
    Integer,
    Real,
    Word,
};

// One parameter that a DOPTPRM card sets. `defaultValue` is the value
// the optimisation uses when no DOPTPRM gives the parameter; where the format reference gives one value for sizing and
// shape optimisation and another for topology optimisation (DELX, DESMAX, DXMIN, CONVDV, DELB for fatigue) it is the
// first, the one of a model with no TOPVAR.
struct DesignOptimParamsInfo
{
    std::string_view name; // the neutral name (MaxDesignCycles for DESMAX)
    DesignOptimParamsKind kind;
    bool hasDefault;
    double defaultValue; // a number for an Integer or a Real parameter
    std::string_view note;
};

// The parameters of the format reference in the order of its table (by their neutral names).
[[nodiscard]] std::span<const DesignOptimParamsInfo> designOptimParamsCatalogue() noexcept;

// The entry of a neutral name; null for a name the format reference does not list (a deck may use such a name, as
// ADSCOD, which the table does not list).
[[nodiscard]] const DesignOptimParamsInfo* findDesignOptimParam(std::string_view name) noexcept;

// The name of a kind: "integer", "real" or "text".
[[nodiscard]] std::string_view designOptimParamsKindName(DesignOptimParamsKind kind) noexcept;

// True when a value suits the kind of the parameter: an integer for an Integer one, an integer or a real for a Real one
// (an integer is a real that is whole: DELX may be written 1), a word for a Word one.
[[nodiscard]] bool designOptimParamsValueFits(const DesignOptimParamsInfo& info, const DesignValue& value) noexcept;

} // namespace oor::model
