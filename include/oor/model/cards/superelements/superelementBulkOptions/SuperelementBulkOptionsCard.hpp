#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

enum class SuperelementBulkOptionsType : std::uint8_t
{
    Primary,               // a part superelement (BEGIN SUPER) or a list superelement
    Collector,             // a collector: it holds no grid or scalar point
    External,              // an external superelement made by EXTSEOUT in an earlier job
    ExternalBinaryResults, // external, from a binary result file of an earlier run (UNITNO is its FORTRAN unit)
    ExternalOutput4,       // external, from an OUTPUT4 file of an earlier run (UNITNO is its FORTRAN unit)
    ExternalPunch,         // external, from a PUNCH file with DMIG entries of an earlier run
    Repeat,                // an identical copy of the superelement RSEID (a secondary superelement)
    Mirror,                // a mirror image of the superelement RSEID (a secondary superelement; it needs an SEMPLN)
    ExternalReducedModel   // a smart superelement with a FMU on the FORTRAN unit UNITNO
};

inline constexpr std::array<std::string_view, 9> SUPERELEMENT_BULK_TYPE_WORDS{"Primary",
                                                                              "CollectorSuperelement",
                                                                              "External",
                                                                              "ExternalBinaryResults",
                                                                              "ExternalMatrixFile",
                                                                              "ExternalPunchFile",
                                                                              "Repeat",
                                                                              "Mirror",
                                                                              "ExternalReducedModel"};

// METHOD: how the boundary points are searched. MANUAL asks for SECONCT cards.
enum class SuperelementBulkOptionsMethod : std::uint8_t
{
    Auto,
    Manual
};

inline constexpr std::array<std::string_view, 2> SUPERELEMENT_BULK_METHOD_WORDS{"Automatic", "Manual"};

// LOC: whether the points of a manual connection are checked to be at the same place (YES, the default) or connected
// wherever they are (NO).
enum class SuperelementBulkOptionsLocation : std::uint8_t
{
    Yes,
    No
};

inline constexpr std::array<std::string_view, 2> SUPERELEMENT_BULK_LOCATION_WORDS{"Yes", "No"};

// One SEBULK card: the boundary search options of a superelement, and what a repeated, mirrored or collector
// superelement is.
//
// Card layout: SEBULK  SEID  TYPE  RSEID  METHOD  TOL  LOC  UNITNO            fields 1 to 7
// SEID is the number of the superelement (0 is valid for the residual structure only to give it a TOL). TYPE
// is required. RSEID names the primary superelement of a REPEAT or a MIRROR and is 0 otherwise. A blank METHOD is AUTO
// and a blank LOC is YES. A TOL of 0.0 is the default and means that the tolerance is taken from the user parameter
// CONFAC (1.0E-5 unless the deck says another), which is also the default of the TOL of every SECONCT of the
// superelement. UNITNO is the unit of the binary result file or the OUTPUT4 file of an external superelement of one of
// those two types, and 0 when blank. The card must be in the main Bulk Data. A superelement has one SEBULK, the number
// is the ID the card is filed under (IdSpace::Group, category Superelement), and the card defines the superelement.
struct SuperelementBulkOptionsCard
{
    base::EntityId superelementId = 0;
    SuperelementBulkOptionsType partType = SuperelementBulkOptionsType::Primary;
    base::EntityId referenceSuperelementId = 0;
    SuperelementBulkOptionsMethod method = SuperelementBulkOptionsMethod::Auto;
    double tolerance = 0.0;
    SuperelementBulkOptionsLocation location = SuperelementBulkOptionsLocation::Yes;
    base::EntityId unitNumber = 0;

    [[nodiscard]] bool operator==(const SuperelementBulkOptionsCard&) const noexcept = default;
};

} // namespace oor::model
