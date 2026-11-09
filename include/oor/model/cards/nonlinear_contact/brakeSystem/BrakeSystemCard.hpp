#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/BrakeKeywords.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace oor::model
{

// One brake system (a disk) of a BRKSYS: the two contact bodies that rub, the motion of each, and the property of the
// system.
struct BrakeSystemDisk
{
    std::string name;                  // Disk_i: the name of the brake system, one to eight characters
    base::EntityId firstBodyId = 0;    // BD1_ID: the BCBODY1 of the first body
    base::EntityId secondBodyId = 0;   // BD2_ID: the BCBODY1 of the second body
    base::EntityId firstMotionId = 0;  // MT1_ID: the MOTION defined for the first body
    base::EntityId secondMotionId = 0; // MT2_ID: the MOTION of the second body; stored resolved (MT1_ID when blank)
    base::EntityId brakePropId = 0;    // BProp_ID: the BRKPROP that defines the property of the system

    [[nodiscard]] bool operator==(const BrakeSystemDisk&) const noexcept = default;
};

// One BRKSYS card (SOL 400): the data of a brake system for brake squeal calculations with one wheel or several, on
// one axis or several.
//
// Card layout: BRKSYS  ID  OMETH  IVEC  BSONLY  ISLIDEBS                                         fields 1 to 5
//                     Disk1  BD1_ID  BD2_ID  MT1_ID  MT2_ID  BProp_ID                           fields 9 to 14
//                     Disk2  ...                                                              one line for each disk
// ID is the number that a Case Control BSQUEAL selects (it is the ID of a BSQUEAL card too: the two cards share the
// number space of that command, so a BSQUEAL and a BRKSYS must not have the same ID:
// `contact.brake-squeal-id-repeated`). OMETH is the load factor (or time) of the brake squeal analysis (real, default
// 0.0); BSONLY is YES (default) or NO; IVEC is 0 (default: the friction vector of the primary body has the direction of
// that of the nodes of the secondary body) or 1 (tangential); ISLIDEBS is 0 (default: sliding contact for the disk and
// pad pairs) or 1 (general static contact). A disk line has the name and the five IDs; BD1_ID, BD2_ID, MT1_ID and
// BProp_ID are required, and a blank MT2_ID is MT1_ID (the format reference), so it is stored resolved and always
// written. The format does not require a disk line (the first line may stand alone: the entry of a module can leave
// them to the MDBKSYS), and the sample of the format reference has two.
//
// A BD1_ID and a BD2_ID are bodies of the model (`ContactBody`; the format says BCBODY1, which the check
// `contact.brake-body-kind` verifies), the MT ids name MOTION cards (`BrakeMotion`) and BProp_ID a BRKPROP
// (`BrakeProperty`).
struct BrakeSystemCard
{
    base::EntityId id = 0;
    double loadFactor = 0.0;
    std::int32_t frictionVectorMode = 0;
    BrakeSquealOnly squealOnly = BrakeSquealOnly::Yes;
    std::int32_t slidingContactOption = 0;
    std::vector<BrakeSystemDisk> disks;

    [[nodiscard]] bool operator==(const BrakeSystemCard&) const noexcept = default;
};

} // namespace oor::model
