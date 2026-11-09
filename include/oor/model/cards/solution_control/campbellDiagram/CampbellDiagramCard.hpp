#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/solution_control/rotordynamicParams/RotordynamicParamsCard.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace oor::model
{

// One CAMPBLL card: the parameters of the generation of a Campbell diagram and of the tracking of the modes along it.
//
// Card layout: CAMPBLL  CID  VPARM  DDVALID  TYPE
//                      MODTRK  CORU  SWITR  NUMMOD  PRTCOR            fields 9 to 13 (the second line)
// VPARM is the parameter that varies and SPEED is the only one the format allows (the speed of the reference rotor),
// TYPE the unit of the speeds (RPM or FREQ, the unit RGYRO already has) that the DDVAL gives; the format gives neither
// a default, so a blank is kept blank (`std::string` empty and `std::nullopt`) and written as a blank, and the checks
// of the solution judge it. DDVALID is the DDVAL that lists the values of the varying parameter. MODTRK chooses the
// mode tracking: 0 numerical (the default), 1 both the numerical and the eigenvector based, 2 eigenvector based only.
// CORU, the threshold of the mode correlation, is 0.7 when blank, SWITR is 0 (use the modes of the last
// rotor speed whose tracking succeeded) or 1 (use the modes of the speed whose tracking failed), NUMMOD the
// number of modes to track, which the format gives no default (the solver takes the first modes with a positive
// imaginary part), and PRTCOR 1 prints the correlation matrix at each rotor speed (0 by default). The defaults of
// MODTRK, CORU, SWITR and PRTCOR do not depend on another field, so they are stored as they are and written as blanks.
// The CID has its own space (IdSpace::PerCardType).
struct CampbellDiagramCard
{
    static constexpr std::int32_t DEFAULT_MODE_TRACKING_METHOD = 0;
    static constexpr double DEFAULT_CORRELATION_THRESHOLD = 0.7;
    static constexpr std::int32_t DEFAULT_TRACKING_SWITCH = 0;
    static constexpr std::int32_t DEFAULT_PRINT_CORRELATION = 0;

    base::EntityId id = 0;
    std::string varyingParameter; // "Speed" or empty when blank
    base::EntityId discreteValuesId = 0;
    std::optional<RotorSpeedUnit> type;
    std::int32_t modeTrackingMethod = DEFAULT_MODE_TRACKING_METHOD;
    double correlationThreshold = DEFAULT_CORRELATION_THRESHOLD;
    std::int32_t trackingSwitch = DEFAULT_TRACKING_SWITCH;
    std::optional<std::int32_t> trackedModeCount;
    std::int32_t printCorrelation = DEFAULT_PRINT_CORRELATION;

    [[nodiscard]] bool operator==(const CampbellDiagramCard&) const noexcept = default;
};

} // namespace oor::model
