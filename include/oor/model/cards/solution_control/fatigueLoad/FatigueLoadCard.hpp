#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// What kind of load an FTGLOAD defines (its TYPE field). A blank TYPE (no value here) is the table of SOL 101 and 103:
// the TID names a TABLFTG or a TABLED1.
enum class FatigueLoadType : std::uint8_t
{
    Db,                  // SOL 101, 103: the load history is in a data acquisition file, TID names a UDNAME
    DataAcquisitionFile, // the same
    LoadHistoryFile,     // the load history is a channel of a load history file, TID names a UDNAME
    Const,               // a constant amplitude cycle between two peaks (SCALE is the first, OFFSET the second)
    Psd,                 // SOL 108, 111: random loading, LCID names RANDPS cards
    Sine,                // SOL 108, 111: sine waves or a sine sweep, from the DETLOAD and SWEEP lines
    Narrow,              // SOL 108, 111: narrow band blocks, from the DETLOAD line
    Static               // a static offset, from a subcase (SOL 101, 103) or an binary result file (SOL 108, 111)
};
inline constexpr std::array<std::string_view, 8> FATIGUE_LOAD_TYPE_WORDS{
    "Decibel",  "DataAcquisitionFile", "LoadHistoryFile", "ConstantLoad", "SpectralDensityLoad",
    "SineLoad", "NarrowBand",          "Static"};

// The longest name of an equivalent unit: fields 4 to 9 of the line, 48 characters.

// The "UNITS" line of the format of SOL 101 and 103: the equivalent unit the life is reported in.
struct FatigueLoadUnits
{
    double equivalentUnits = 1.0; // the number of equivalent units of one repeat
    std::string name;             // EQNAME, empty when blank (the solver then writes "Repeats")

    [[nodiscard]] bool operator==(const FatigueLoadUnits&) const noexcept = default;
};

// One sine wave (TYPE SINE) or narrow band block (TYPE NARROW) of the "DETLOAD" line.
struct FatigueLoadTone
{
    double frequency = 0.0; // Fi: the frequency of the wave, or the centre frequency of the band
    double height = 0.0; // Gi: the peak amplitude of the wave, or Hi: the height of the band in stress squared per Hz
    std::optional<double> width; // Bi: the width of a narrow band, blank for a sine wave

    [[nodiscard]] bool operator==(const FatigueLoadTone&) const noexcept = default;
};

// The type of a sine sweep: linear in Hz (the default), in decibels or in octaves.
enum class FatigueLoadSweepType : std::uint8_t
{
    Hz,
    Db,
    Oct
};
inline constexpr std::array<std::string_view, 3> FATIGUE_LOAD_SWEEP_TYPE_WORDS{"FreqInHertz", "Decibel", "Octave"};

// What a sweep or a frequency response is expressed in (FRFUNIT and LDUNIT): acceleration, velocity, displacement or
// force.
enum class FatigueLoadQuantity : std::uint8_t
{
    Acceleration,
    Velocity,
    Displacement,
    Force
};
inline constexpr std::array<std::string_view, 4> FATIGUE_LOAD_QUANTITY_WORDS{"Acceleration", "Velocity", "Displacement",
                                                                             "Force"};

// The "SWEEP" line and the line after it (SOL 108 and 111, TYPE SINE).
struct FatigueLoadSweep
{
    double firstFrequency = 0.0;                                      // F1
    double firstHeight = 0.0;                                         // G1
    double lastFrequency = 0.0;                                       // Fn
    double lastHeight = 0.0;                                          // Gn
    FatigueLoadSweepType type = FatigueLoadSweepType::Hz;             // SWTYPE
    int count = 50;                                                   // SWNUM: the number of sine waves is one more
    double rate = 0.0;                                                // SWRATE: per second, in the unit of the type
    int passes = 0;                                                   // NSWEEP
    FatigueLoadQuantity frfUnit = FatigueLoadQuantity::Acceleration;  // FRFUNIT
    FatigueLoadQuantity loadUnit = FatigueLoadQuantity::Acceleration; // LDUNIT

    [[nodiscard]] bool operator==(const FatigueLoadSweep&) const noexcept = default;
};

// One FTGLOAD card: a cyclic loading variation of a fatigue analysis (a load history of a pseudo-static analysis, SOL
// 101, of a modal one, SOL 103, or a random, sine, narrow band or static load of a random vibration analysis, SOL 108
// and 111). An FTGEVNT groups the FTGLOADs of an event.
//
// Card layout, SOL 101 and 103:  FTGLOAD  ID  TID  LCID  LDM  SCALE/MAX  OFFSET/MIN  TYPE  CHNL         fields 1 to 8
//                                        UNITS  EQUIV  EQNAME                                      a line, optional
// Card layout, SOL 108 and 111:  FTGLOAD  ID  TID  LCID  (blank)  SCALE  OFFSET  TYPE                  fields 1 to 7
//                                        DETLOAD  F1  G1/H1  B1  F2  G2/H2  B2                       lines, two bands
//                                        each SWEEP  F1  G1/H1  (blank)  Fn  Gn/Hn                         a line,
//                                               (blank)  SWTYPE  SWNUM  SWRATE  NSWEEP  FRFUNIT  LDUNIT   and the line
//                                               after it
// The two formats have the same first fields at the same places (SOL 108 leaves LDM and CHNL blank and has no CHNL) and
// are told apart by what follows: a UNITS line is the format of SOL 101 and 103, a DETLOAD or SWEEP line the format of
// SOL 108 and 111, and a card with both kinds is refused. The card holds the union, so the blank LDM of a card of SOL
// 108 reads as the default 1.0 that the format gives it.
//
// Defaults. LDM 1.0 and SCALE/MAX 1.0 are static. The default of OFFSET/MIN is 0.0, and -1.0 when TYPE is CONST (the
// second peak of a constant amplitude cycle): it depends on TYPE, so it is stored resolved and always written (the
// format is not consistent about the offset of SOL 108, 1.0 or 0.0; the SOL 101 value is kept). TYPE is absent when
// blank. TID is blank (0) when the type needs none; the format reference asks for it when the TYPE is blank, DB, a data
// acquisition file or the load history file word. LCID is any integer: a subcase or a mode number, 0 (a constant
// offset), -1 or -2 (the correlation of several PSD inputs).
//
// What TID names depends on TYPE (the reference follows it): a blank TYPE names a TABLFTG (a TABLED1 is allowed by the
// format too, which the FatigueTable target does not find: the card reads and writes it all the same), DB, a data
// acquisition file, the load history file word, STATIC and PSD a UDNAME; for the other types the field is blank or
// ignored and is not a reference. LCID names the RANDPS cards (the RandomSet target) when TYPE is PSD and it is
// positive.
//
// The ID is unique among the FTGLOAD cards (IdSpace::PerCardType, the category FatigueLoad that an FTGEVNT names).
struct FatigueLoadCard
{
    base::EntityId id = 0;
    base::EntityId tableId = 0; // 0 when blank
    int loadCaseId = 0;
    double referenceLoadMagnitude = 1.0;
    double scaleFactor = 1.0; // SCALE, or MAX of a constant amplitude cycle
    double offset = 0.0;      // OFFSET, or MIN of a constant amplitude cycle (resolved: -1.0 for TYPE CONST when blank)
    std::optional<FatigueLoadType> type;
    base::EntityId channel = 0; // the channel of a load history file, 0 when blank
    std::optional<FatigueLoadUnits> units;
    std::vector<FatigueLoadTone> tones;
    std::optional<FatigueLoadSweep> sweep;
};

} // namespace oor::model
