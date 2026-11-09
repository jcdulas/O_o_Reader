#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// The SFORM field of a CONTRLT: what the sensor measures. The format reference allows the temperature only, T (the
// default).
enum class SensorForm : std::uint8_t
{
    Temperature
};

inline constexpr std::array<std::string_view, 1> HEAT_CONTROLLER_SENSOR_FORM_WORDS{"Temperature"};

// The CTYPE field of a CONTRLT: the kind of controller. The format reference allows the thermostat only, TSTAT (the
// default).
enum class ControlType : std::uint8_t
{
    Thermostat
};

inline constexpr std::array<std::string_view, 1> HEAT_CONTROLLER_TYPE_WORDS{"Thermostat"};

// The PTYPE numbers of a CONTRLT, 1 to 11: 1 to 6 are the thermostat curves (heating, cooling and the other
// on/off patterns of the dead band between PL and PH), 7 is bang-bang, 8 proportional, 9 steady state, 10 and 11 the
// two forms of a PID controller.
inline constexpr std::int32_t HEAT_CONTROLLER_MIN_CONTROL_LAW = 1;
inline constexpr std::int32_t HEAT_CONTROLLER_MAX_CONTROL_LAW = 11;

// One CONTRLT card: the controller that the CNTRLND field of a QVECT, QVOL, QBDY3, RADBC, CONV or CONVM names in a heat
// transfer analysis.
//
// Card layout: CONTRLT  ID  SENSOR  SFORM  CTYPE  PL  PH  PTYPE  PZERO          fields 1 to 8
//                      DT  DELAY  TAUC                                          fields 9 to 11
// The ID is the controller node: a heat card names it with CNTRLND, which is a plain number there (a grid or scalar
// point of the same number combines its logic with the controller, so the number is not an ID the model can look up).
// SENSOR is the grid point or scalar point that measures the temperature. PL and PH are required: the limits of the
// dead band of a thermostat (for PTYPE 9 to 11 PL is the set temperature and PH is not used, but the format reference
// still marks both as required). PTYPE is required and has no default. PZERO, the initial value of the controller, is
// 0.0 or 1.0: the controller starts either off or on, and no value between the two has a meaning. DT, DELAY and TAUC
// are times that default to 0.0 although they are meant to be positive, so the card accepts zero and refuses a
// negative time.
struct HeatControllerCard
{
    base::EntityId id = 0;         // unique among the CONTRLT cards
    base::EntityId sensorNode = 0; // a grid point or a scalar point
    SensorForm sensorForm = SensorForm::Temperature;
    ControlType controllerKind = ControlType::Thermostat;
    double lowerLimit = 0.0;
    double upperLimit = 0.0;
    std::int32_t controlLaw = 0;
    double initialState = 0.0;
    double timeStep = 0.0;
    double delayTime = 0.0;
    double timeConstant = 0.0;
};

} // namespace oor::model
