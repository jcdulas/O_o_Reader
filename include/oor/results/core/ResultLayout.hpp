#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace oor::results
{

// The vocabulary of a result table's layout, shared by every file format that holds results.

// How the values of a table are ordered: SORT1 holds one record per step (every node or element in it), SORT2 one
// record per node or element (every step in it).
enum class SortOrder : std::uint8_t
{
    Sort1 = 1,
    Sort2 = 2
};

// What a table holds: real values, complex values, or random-analysis values (PSD, RMS, ...; real).
enum class ValueKind : std::uint8_t
{
    Real,
    Complex,
    Random
};

// How the values are written: real, real and imaginary parts, or magnitude and phase (the last two are complex).
//
// `Random` and `ModalContribution` are not ways to write values: they are the "form" under which the status of the
// tables of a result variant (below) is kept, because a random table (PSD, RMS, ...) and a table of modal contributions
// are read by other layouts than the table of the same element type of a plain analysis and are validated apart.
enum class ResultFormat : std::uint8_t
{
    Real,
    RealImaginary,
    MagnitudePhase,
    Random,
    ModalContribution
};

// What kind of result set a table belongs to. `Plain` is the result of an analysis; the others hold the same quantities
// (displacements, element forces, stresses, ...) processed or written differently: the modal contributions to the
// stresses of a frequency response, and the random analysis results (the power spectral density, the
// autocorrelation, the cumulative root mean square, the root mean square, the number of positive crossings, the peak to
// peak value). Each variant has its own containers (`results.psd.displacements`).
enum class ResultVariant : std::uint8_t
{
    Plain,
    ModalContribution,
    Psd,               // power spectral density function
    AutoCorrelation,   // autocorrelation function ("ato")
    CumulativeRms,     // cumulative root mean square ("crm")
    Rms,               // root mean square
    NumberOfCrossings, // number of positive crossings ("no")
    PeakToPeak         // peak to peak value ("pk")
};

// What the step of a result set is (the "time" of its values).
enum class StepKind : std::uint8_t
{
    None,               // the steps are inside the data (SORT2), or the analysis has none
    LoadCombinationSet, // one step: a load set number, which differs from one table to another and does not identify a
                        // step
    Mode,               // a mode number
    Frequency,          // a frequency
    Time,               // a time
    LoadFactor          // a load factor of a non-linear static analysis
};

[[nodiscard]] std::string_view sortOrderName(SortOrder order) noexcept;
// "psd", "ato", "crm", "rms", "no", "pk", "modalContribution"; empty for a plain result.
[[nodiscard]] std::string_view resultVariantName(ResultVariant variant) noexcept;
// The variant of a name ("psd", in any case); Plain for an empty name; nothing for another.
[[nodiscard]] std::optional<ResultVariant> resultVariantFromName(std::string_view name);
// True for the five random analysis variants (not the modal contributions).
[[nodiscard]] bool isRandomVariant(ResultVariant variant) noexcept;
// The variants other than Plain, in the order above.
[[nodiscard]] std::span<const ResultVariant> allResultVariants() noexcept;
[[nodiscard]] std::string_view valueKindName(ValueKind kind) noexcept;
[[nodiscard]] std::string_view resultFormatName(ResultFormat format) noexcept;
[[nodiscard]] std::string_view stepKindName(StepKind kind) noexcept;

// The name of an analysis code, "unknown" for a code this library does not know. The analysis codes are small numbers
// this library uses to say what analysis a table comes from: 1 static, 2 real modes, 3 differential stiffness, 4
// buckling (differential stiffness), 5 frequency response, 6 transient, 7 pre-buckling, 8 buckling modes, 9 complex
// modes, 10 non-linear static, 11 geometric non-linear static, 12 contact; 0 for a table that belongs to no analysis
// (the grid point weight).
[[nodiscard]] std::string_view analysisTypeName(int analysisCode) noexcept;

// What the step of a table is, for an analysis code.
[[nodiscard]] StepKind stepKindOf(int analysisCode) noexcept;

} // namespace oor::results
