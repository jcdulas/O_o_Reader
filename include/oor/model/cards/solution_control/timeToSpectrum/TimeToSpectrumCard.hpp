#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace oor::model
{

// The window function applied to each block of the time signal (the WINDOW field).
enum class TimeToSpectrumWindow : std::uint8_t
{
    Hanning,
    None
};
inline constexpr std::array<std::string_view, 2> TIME_TO_SPECTRUM_WINDOW_WORDS{"HanningWindow", "None"};

enum class TimeToSpectrumFileFormat : std::uint8_t
{
    Csv,
    LoadHistoryFile
};
inline constexpr std::array<std::string_view, 2> TIME_TO_SPECTRUM_FILE_FORMAT_WORDS{"CommaSeparatedFile",
                                                                                    "LoadHistoryFile"};

// One record of the EVENT lines: an event, the length of its window and the overlap of two windows.
struct TimeToSpectrumEvent
{
    base::EntityId eventId = 0; // the number of the event: the TID of an FTGLOAD of TYPE PSD (not an FTGEVNT number)
    double length = 0.0;        // Ti: the length of the window in time
    double overlap = 0.0;       // the overlap (positive) or gap (negative) of two windows in time, 0.0 when blank

    [[nodiscard]] bool operator==(const TimeToSpectrumEvent&) const noexcept = default;
};

// One record of the DELETE lines: the portions of the time signal of an event that are deleted before the transform, up
// to three (start, end) pairs of times.
struct TimeToSpectrumDeletion
{
    base::EntityId eventId = 0; // an event of the EVENT lines
    std::vector<std::pair<double, double>> intervals;

    [[nodiscard]] bool operator==(const TimeToSpectrumDeletion&) const noexcept = default;
};

// One pair of the MAP lines: the subcase of a transfer function and the channel of the file of its load.
struct TimeToSpectrumChannel
{
    int loadCaseId = 0;
    int channel = 0;

    [[nodiscard]] bool operator==(const TimeToSpectrumChannel&) const noexcept = default;
};

// One TIM2PSD card (SOL 108 and 111): the conversion of time histories to power spectral densities, the Fast Fourier
// Transformation that a random vibration fatigue analysis uses when its loads are given as time signals.
//
// Card layout:
//   TIM2PSD  ID  SRATE  WINDOW  FORMAT  MEANS  NSKIP                                    fields 1 to 6
//            EVENT   EVNTID1 (4 blank fields) T1  DELTA1                                one event on each line
//            DELETE  EVNTID1  Ti1_1 Tf1_1  Ti2_1 Tf2_1  Ti3_1 Tf3_1                      one event on each line
//            MAP     LCID1 CHAN1  LCID2 CHAN2  LCID3 CHAN3                               three pairs on each line
// Each section starts with its keyword in field 2 of a line and goes on over the lines below it that have a blank field
// 2 (a free format deck keeps the commas of the empty fields). EVENT is required and
// appears once; DELETE and MAP are optional and appear once. The sections are written in that order.
//
// Defaults (static, stored resolved, left blank by the writer): WINDOW HANNING, FORMAT CSV, MEANS NO, NSKIP 0, and a
// blank overlap is 0.0. SRATE has no default. An event number of a DELETE line must be one of the EVENT lines, the
// pairs of a DELETE record are filled in order and in increasing time (format reference: t1 <= t2 <= t3 < t4 ...), and
// the subcases and the channels of the MAP lines increase.
//
// The card names no other card: EVNTIDi is the number of the event, which is the TID of an FTGLOAD (and so the number
// of a UDNAME), and LCIDi a subcase, so none is a reference. The ID is unique among the TIM2PSD cards
// (IdSpace::PerCardType, category Other: a FATIGUE command of the Case Control selects it).
struct TimeToSpectrumCard
{
    base::EntityId id = 0;
    double samplingRate = 0.0; // samples per second
    TimeToSpectrumWindow window = TimeToSpectrumWindow::Hanning;
    TimeToSpectrumFileFormat format = TimeToSpectrumFileFormat::Csv;
    bool meanStress = false; // MEANS YES: the mean of each signal is calculated (and not used)
    int skipCount = 0;       // the header lines of a CSV file to skip
    std::vector<TimeToSpectrumEvent> events;
    std::vector<TimeToSpectrumDeletion> deletions;
    std::vector<TimeToSpectrumChannel> channels;
};

} // namespace oor::model
