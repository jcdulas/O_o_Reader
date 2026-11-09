#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nodes/ModuleBoundarySupport.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

enum class ModuleBulkOptionsType : std::uint8_t
{
    Primary,               // PRIMARY (the default): a module with grid points of its own
    Repeat,                // REPEAT: a copy of a primary module, moved or mirrored
    Mirror,                // MIRROR: a copy of a primary module, mirrored
    Move,                  // MOVE: a primary or copied module placed by an MDMOVE entry
    External,              // EXTERNAL: a module made by an earlier run (EXTMDOUT)
    ExternalBinaryResults, // the same, from a binary results file
    ExternalMatrixFile     // the same, from a matrix file
};

// How the boundary points between modules are searched for (the METHOD field). Numbered in the order of
// MODULE_BULK_METHOD_WORDS.
enum class ModuleBulkOptionsMethod : std::uint8_t
{
    Auto,  // AUTO: by position, within the tolerance
    Manual // MANUAL (the default): only the points that MDCONCT entries connect
};

inline constexpr std::array<std::string_view, 2> MODULE_BULK_METHOD_WORDS{"Automatic", "Manual"};

// One MDBULK card: the type of a module and how the boundary between modules is searched (SOLs 101, 103 to 112).
//
// Card layout: MDBULK  MODID  TYPE  RMODID  METHOD  TOL  .  UNITNO                fields 1 to 7 (field 6 is blank)
// MODID is a module number (a positive integer) or the word ALL, which gives METHOD and TOL to every module that has no
// MDBULK of its own: with ALL, TYPE and RMODID are blank (the card keeps no difference between a blank TYPE
// and PRIMARY, so an ALL card with TYPE PRIMARY is accepted). RMODID is the module a copy is made from (TYPE REPEAT,
// MIRROR or MOVE with RMODID above 0 and not MODID; the card does not refuse an RMODID the format says is unused). A
// TOL of 0.0 is the default: the value of the PARAM CONFAC (1.0E-5), taken when the boundary is searched; the format
// states no range for it. UNITNO is the Fortran unit of the binary results file or the matrix file of an external
// module of either file type (0 when blank).
//
// The module numbers (MODID, RMODID) are plain integers: a module is a BEGIN MODULE section, which the library keeps as
// text. The card belongs in the main Bulk Data and is meaningful only when the deck has modules; the reader
// keeps it wherever it stands and a check can tell where.
struct ModuleBulkOptionsCard
{
    ModuleOrAll module;                                          // MODID: the module, or ALL
    ModuleBulkOptionsType kind = ModuleBulkOptionsType::Primary; // TYPE
    base::EntityId sourceModuleId = 0; // RMODID: the module of which this one is a copy, 0 when blank
    ModuleBulkOptionsMethod method = ModuleBulkOptionsMethod::Manual;
    double tolerance = 0.0;        // TOL: 0.0 takes the value of PARAM,CONFAC
    base::EntityId unitNumber = 0; // UNITNO: 0 when blank

    [[nodiscard]] bool operator==(const ModuleBulkOptionsCard&) const noexcept = default;
};

} // namespace oor::model
