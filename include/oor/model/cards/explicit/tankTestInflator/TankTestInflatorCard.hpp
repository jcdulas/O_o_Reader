#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// METHOD (field 2): how the mass flow rate of the inflator is calculated from the tank test: from the average
// temperature of the gas (AVTEMP) or from the pressure measured in the inflator (INFPRES).
enum class TankTestInflatorMethod : std::uint8_t
{
    AverageTemperature,
    InflatorPressure
};
inline constexpr std::array<std::string_view, 2> TANK_TEST_INFLATOR_METHOD_WORDS{"AverageTemperatureMethod",
                                                                                 "InfinitePressure"};

// One INFLTNK card (SOL 700): an inflator described by what a tank test measured: the pressure in the tank against time
// gives the mass flow rate and the temperature of the gas that the inflator blows into a GBAG or an Euler coupling
// surface (COUPLE) through the ABINFL that names the card. With an Euler domain it works only with the single material
// hydrodynamic solver and an EOSGAM equation of state.
//
// Card layout, three lines:
//   INFLTNK  INFLID  METHOD  TPTABLE  TANKVOL  INFLMAS  INITPRES  ENDPRES  INITTEMP   fields 1 to 8
//   ENDTEMP  GAMMA  CV  R  CP  IPTABLE  INFLPRES  INFLTEMP                             fields 9 to 16
//   INFLAREA  SFTP  SFIP                                                              fields 17 to 19
// (the format reference draws the ratio of the specific heats and the two specific heats as symbols; GAMMA, CV and CP
// are the names the other gas cards give them; the grid also spells ENDPRES as ENDTPRES.) INFLID numbers the card
// (unique among the inflators of every kind, which the ABINFL INFTYPID names). TPTABLE is the number of a TABLED1, the
// tank pressure against time; TANKVOL is the volume of the tank and INFLMAS the total mass of gas that the inflator
// generates.
//
// The two methods use different fields (the fields of the pressure method are given only
// with that method). The common decks, an AVTEMP card, leaves them blank, and the card
// does the same: with METHOD = AVTEMP, ENDPRES, INITTEMP, ENDTEMP, IPTABLE, INFLPRES, INFLTEMP, INFLAREA, SFTP and SFIP
// have no value (SFTP and SFIP are 1.0); with METHOD = INFPRES the card needs ENDPRES, INITTEMP, ENDTEMP, IPTABLE,
// INFLPRES and INFLAREA, the format reference marking all six required. IPTABLE is the number of a TABLED1, the
// inflator pressure against time. INFLTEMP is the temperature of the inflowing gas, a real, or the word ATM (and blank)
// for the average temperature of the AVTEMP method: the card keeps no value for ATM. SFTP and SFIP scale the two
// pressure tables.
//
// INITPRES is meant to be positive, but a tank that starts at zero gauge pressure is a real case: zero is accepted.
// The gas: R is required. The format reference asks for two of the four gas constants (GAMMA, CV, R, CP), so one of the
// other three comes with it; the solver derives the rest, and the card does not count them, as the EOSGAM does not.
struct TankTestInflatorCard
{
    base::EntityId id = 0; // unique number of the inflator
    TankTestInflatorMethod method = TankTestInflatorMethod::AverageTemperature;
    base::EntityId tankPressureTableId = 0;     // a TABLED1: the tank pressure against time
    double tankVolume = 0.0;                    // the volume of the tank, greater than zero
    double inflatorMass = 0.0;                  // the mass of gas the inflator generates, greater than zero
    double initialPressure = 0.0;               // the initial pressure in the tank, not negative
    std::optional<double> endPressure;          // INFPRES only: the pressure in the tank at the end of the table
    std::optional<double> initialTemperature;   // INFPRES only: the initial temperature in the tank
    std::optional<double> endTemperature;       // INFPRES only: the temperature in the tank at the end
    std::optional<double> gammaCoefficient;     // the ratio of the specific heats
    std::optional<double> specificHeatVolume;   // specific heat at constant volume
    double gasConstant = 0.0;                   // gas constant, greater than zero
    std::optional<double> specificHeatPressure; // specific heat at constant pressure
    base::EntityId inflatorPressureTableId = 0; // INFPRES only: a TABLED1, the inflator pressure against time
    std::optional<double> inflatorPressure;     // INFPRES only: the initial pressure in the inflator
    std::optional<double> inflatorTemperature;  // INFPRES only: the temperature of the gas, nothing for ATM
    std::optional<double> inflatorArea;         // INFPRES only: the area of the holes of the inflator
    double tankPressureScale = 1.0;             // INFPRES only: the scale factor of the tank pressure
    double inflatorPressureScale = 1.0;         // INFPRES only: the scale factor of the inflator pressure
};

} // namespace oor::model
