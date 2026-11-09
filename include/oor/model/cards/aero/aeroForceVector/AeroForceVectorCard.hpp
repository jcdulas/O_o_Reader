#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/aero/AeroSupport.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace oor::model
{

// The mesh an AEFORCE is given on: the aerodynamic mesh (the ks-set, a matrix of forces named by DMIK) or the
// structural mesh (the g-set, a load set named by LSET). The enumerators are numbered in the order of
// AERO_FORCE_VECTOR_MESH_WORDS.
enum class AeroForceVectorMesh : std::uint8_t
{
    Aero,
    Struct
};

inline constexpr std::array<std::string_view, 2> AERO_FORCE_VECTOR_MESH_WORDS{"AeroMesh", "Structure"};

// The word of the PERQ field.
inline constexpr std::string_view AERO_FORCE_VECTOR_PER_PRESSURE_WORD = "PerDynamicPressure";

// One AEFORCE card: a vector of forces, absolute or per unit dynamic pressure, that goes with one control vector of the
// aerodynamic configuration and is used by the static aeroelastic trim. The AECONFIG the card belongs to is chosen by
// the Case Control, and the program expects a card for each Mach number and symmetry the analysis uses.
//
// Card layout: AEFORCE  MACH  SYMXZ  SYMXY  UXID  MESH  LSET  DMIK  PERQ           fields 1 to 8
// MACH is zero or more and not 1.0 (`checkForceMach`). SYMXZ and SYMXY are one of SYMM, ASYMM and ANTI, the symmetry of
// the force vector about the xz and the xy planes (`FORCE_SYMMETRY_WORDS`); the format reference gives no default or
// blank for them, so both are required, as is UXID, the number of the UXVEC with the control parameters. MESH is AERO
// or STRUCT and, with no default in the format reference, is required. It selects which of the next fields is used.
//
// With AERO the forces are the matrix of the DMIK card that DMIK names (required), and LSET is not used: it
// must be blank. The PERQ field is not used either (the forces of a DMIK are always multiplied by the dynamic
// pressure); the format reference only says that it is ignored, so the card keeps a PERQ that is written there.
//
// With STRUCT the forces are a load set, LSET, a positive number (required: any static load a deck can
// select, such as FORCE1, PLOAD4 or LOAD), and DMIK is not used: it must be blank. PERQ is the word PERQ or blank; with
// it the loads are multiplied by the dynamic pressure, without it they are absolute.
//
// A field that the MESH does not select is refused when it has a value (PERQ excepted, see above).
//
// DMIK is a name, not a reference: it is kept as written and a check of the model matches it with the names of the DMIK
// cards without regard to the case. LSET is the ID of a load set (ReferenceTarget::LoadCombinationSet), blank (0) for
// AERO.
struct AeroForceVectorCard
{
    double machNumber = 0.0;
    ForceSymmetry symmetricXz = ForceSymmetry::Symmetric;
    ForceSymmetry symmetricXy = ForceSymmetry::Symmetric;
    base::EntityId controlVectorId = 0; // a UXVEC
    AeroForceVectorMesh mesh = AeroForceVectorMesh::Aero;
    base::EntityId loadSetId = 0;          // a load set, STRUCT only
    std::optional<std::string> matrixName; // the name of a DMIK matrix, AERO only
    bool perUnitPressure = false;          // PERQ written: the loads of a STRUCT mesh are per unit dynamic pressure
};

} // namespace oor::model
