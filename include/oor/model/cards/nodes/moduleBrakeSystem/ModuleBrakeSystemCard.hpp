#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// The BSONLY flag of an MDBKSYS: whether the nonlinear iterations stop once the brake squeal analysis is done. The
// enumerators are numbered in the order of MODULE_BRAKE_SQUEAL_ONLY_WORDS.
enum class ModuleBrakeSystemSquealOnly : std::uint8_t
{
    Yes, // YES (the default): the brake squeal analysis only, then exit the nonlinear iteration
    No   // NO: go on with the nonlinear analysis
};

inline constexpr std::array<std::string_view, 2> MODULE_BRAKE_SQUEAL_ONLY_WORDS{"Yes", "No"};

// One brake system (a disk) of an MDBKSYS: the two contact bodies that rub (a body of a module and its BCBODY1 number
// there), the motion defined for each, and the BRKSYS entry that goes with them.
struct ModuleBrakeSystemDisk
{
    std::string name;                 // DISKi: the name of the brake system, one to eight characters
    base::EntityId moduleId1 = 0;     // MODID1i: the module of the first body; 0 is the main Bulk Data
    base::EntityId body1Id = 0;       // BODY1IDi: its BCBODY1
    base::EntityId moduleId2 = 0;     // MODID2i: the module of the second body
    base::EntityId body2Id = 0;       // BODY2IDi: its BCBODY1
    base::EntityId motion1Id = 0;     // MOTN1IDi: the MOTION of the first body, 0 when blank
    base::EntityId motion2Id = 0;     // MOTN2IDi: the MOTION of the second body, 0 when blank
    base::EntityId brakeSystemId = 0; // BRKPIDi: the BRKSYS entry of the system, 0 when blank

    [[nodiscard]] bool operator==(const ModuleBrakeSystemDisk&) const noexcept = default;
};

// One MDBKSYS card: the data of a brake system for brake squeal calculations with one wheel or several, on one axis or
// several, in different modules (SOL 400).
//
// Card layout: MDBKSYS  ID  OMETH  IVEC  BSONLY  ISLIDEBS         fields 1 to 5 (the grid of the format spells the last
//                                                                 one ISLIDER3, its description ISLIDEBS)
//                      DISK1 MODID11 BODY1ID1 MODID21 BODY2ID1 MOTN1ID1 MOTN2ID1 BRKPID1  fields 9 to 16
//                      DISK2 ...                                                         one line for each disk
// ID is the number that a BSQUEAL command of the Case Control selects (positive, unique among the MDBKSYS). OMETH is
// the load factor (or the time) of the brake squeal analysis (real, default 0.0); IVEC is 0 (default: the friction
// vector of the primary body is in the direction of the nodes of the secondary body) or 1 (tangential); BSONLY is YES
// (default) or NO; ISLIDEBS is 0 (default: sliding contact of the disk and pad pairs) or 1 (general static contact).
// Each disk line has its eight fields as the format draws them: the descriptions list BRKPID1i and BRKPID2i (the BRKSYS
// of the first and the second body) while the grid has one field, BRKPIDi, in the last place; the card follows the
// layout (one BRKPID on each line). The card may have no disk lines (the format reference does
// not require one: the format says the first lines of the entries that a BSQUEAL selects can differ). The card belongs
// in module 0 and modules must be present.
//
// The modules are plain integers. A body is a BCBODY1 of its module: it is looked up in this model (`ContactBody`) only
// when its module is 0, the main Bulk Data. The MOTION and the BRKSYS entries are cards of the later part of the
// closure that are not typed yet: their IDs are plain, nothing is looked up. The ID is named by the Case Control
// BSQUEAL, which is not a reference kind.
struct ModuleBrakeSystemCard
{
    base::EntityId id = 0;
    double squealLoadFactor = 0.0;
    std::int32_t frictionVectorMethod = 0;
    ModuleBrakeSystemSquealOnly squealOnly = ModuleBrakeSystemSquealOnly::Yes;
    std::int32_t contactMethod = 0;
    std::vector<ModuleBrakeSystemDisk> disks;

    [[nodiscard]] bool operator==(const ModuleBrakeSystemCard&) const noexcept = default;
};

} // namespace oor::model
