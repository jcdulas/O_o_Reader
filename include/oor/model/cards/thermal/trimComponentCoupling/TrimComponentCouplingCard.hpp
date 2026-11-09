#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// The default of the two compression ratios of the reduced impedance matrices: no compression.
inline constexpr double TRIM_COUPLING_DEFAULT_COMPRESSION = 1.0;

// One ACPEMCP card: how a (3D, finite element) trim component, a layer of porous or elastic material of a car interior,
// is coupled to the structure and to the acoustic cavities, and which of its degrees of freedom are constrained. The
// reduced boundary impedance matrices of the component are computed from it. Case Control TRIMGRP names the component
// by its TID; the card goes in the main Bulk Data (`BEGIN BULK`), and the SET1 and SET3 cards it names are in the Bulk
// Data of the trim component (`BEGIN BULK TRMC`).
//
// Card layout: ACPEMCP  TID  SGLUED  SSLIDE  SOPEN  SIMPER  OOC  SPM  SAIRGAP        fields 1 to 8
//                      SCUX  SCUY  SCUZ  SCRX  SCRY  SCRZ  SCFP  RID               fields 9 to 16
//                      CRIMSR  CRIMFR                                              fields 17 and 18
//
// TID is the number of the trim component, a plain integer (nothing in the Bulk Data defines a component by it). A trim
// component may have several ACPEMCP cards, told apart by RID, the region of the component, which is 0 when blank; the
// one with a blank RID comes first, so the cards of a component share its TID (IdSpace::Group) and no ID is
// unique. SGLUED, SSLIDE, SOPEN, SIMPER and SAIRGAP name SET1 or SET3 cards of grids, one for each kind of interface
// (glued to the structure, sliding on it, open to the cavities, impervious to the cavities, coupled through an air
// gap), and SCUX to SCFP the sets of grids that are held at zero in each translation, in each rotation and in the fluid
// pressure; a blank (0) set is a kind of interface or constraint the component does not have. The rotations SCRX, SCRY
// and SCRZ are kept although the 3D elements of the current implementation do not process them. OOC is the number of
// blocks of the out-of-core solver (1 or more, 1 when blank); SPM is a flag, 0 or more, that selects the single
// precision MUMPS solver when it is greater than 0, 0 when blank. CRIMSR and CRIMFR are the compression ratios of the
// reduced impedance matrices for the coupling with the structure and with the cavity, each in the range 0 (excluded) to
// 1, 1.0 when blank.
struct TrimComponentCouplingCard
{
    base::EntityId id = 0; // the trim component, shared by its cards
    base::EntityId gluedSetId = 0;
    base::EntityId slidingSetId = 0;
    base::EntityId openSetId = 0;
    base::EntityId imperviousSetId = 0;
    std::int32_t outOfCoreBlockCount = 1;
    std::int32_t singlePrecisionFlag = 0;
    base::EntityId airGapSetId = 0;
    base::EntityId fixedTranslationXSetId = 0;
    base::EntityId fixedTranslationYSetId = 0;
    base::EntityId fixedTranslationZSetId = 0;
    base::EntityId fixedRotationXSetId = 0;
    base::EntityId fixedRotationYSetId = 0;
    base::EntityId fixedRotationZSetId = 0;
    base::EntityId fixedPressureSetId = 0;
    base::EntityId regionId = 0; // the region of the component, 0 when blank
    double structureCompressionRatio = TRIM_COUPLING_DEFAULT_COMPRESSION;
    double cavityCompressionRatio = TRIM_COUPLING_DEFAULT_COMPRESSION;

    [[nodiscard]] bool operator==(const TrimComponentCouplingCard&) const noexcept = default;
};

} // namespace oor::model
