#pragma once

#include "oor/base/errors/Errors.hpp"
#include "oor/results/core/ResultData.hpp"
#include "oor/results/core/ResultLayout.hpp"
#include "oor/results/core/ResultTable.hpp"

#include <initializer_list>
#include <string>
#include <string_view>

// The archive stores enumerations by their text names ("float32", "frequency", "basic"), never by numbers, so that a
// person reading the file with any HDF5 tool understands it. These helpers turn a name back into its value.
namespace oor::results::archive
{

// The value of `values` whose name (given by `nameOf`) is `text`; throws IoError naming `what` when none is.
template <typename Enum, typename NameOf>
Enum enumFromName(std::string_view text, std::initializer_list<Enum> values, NameOf nameOf, std::string_view what)
{
    for (const Enum value : values)
        if (nameOf(value) == text)
            return value;
    throw base::IoError("archive: unknown " + std::string(what) + " '" + std::string(text) + "'");
}

inline ValueType valueTypeFromName(std::string_view text)
{
    return enumFromName(text, {ValueType::Float32, ValueType::Float64, ValueType::Complex64, ValueType::Complex128},
                        valueTypeName, "value type");
}

inline StepKind stepKindFromName(std::string_view text)
{
    return enumFromName(text,
                        {StepKind::None, StepKind::LoadCombinationSet, StepKind::Mode, StepKind::Frequency,
                         StepKind::Time, StepKind::LoadFactor},
                        stepKindName, "step kind");
}

inline ResultFormat resultFormatFromName(std::string_view text)
{
    return enumFromName(text,
                        {ResultFormat::Real, ResultFormat::RealImaginary, ResultFormat::MagnitudePhase,
                         ResultFormat::Random, ResultFormat::ModalContribution},
                        resultFormatName, "stored format");
}

inline ResultStatus resultStatusFromName(std::string_view text)
{
    return enumFromName(text, {ResultStatus::Validated, ResultStatus::NotValidated, ResultStatus::Experimental},
                        resultStatusName, "status");
}

inline StoredFrame storedFrameFromName(std::string_view text)
{
    return enumFromName(text,
                        {StoredFrame::Unknown, StoredFrame::NodeOutputSystem, StoredFrame::Basic, StoredFrame::Element,
                         StoredFrame::Material, StoredFrame::Ply, StoredFrame::Coordinate, StoredFrame::Cylindrical,
                         StoredFrame::Spherical, StoredFrame::Mixed},
                        storedFrameName, "stored frame");
}

} // namespace oor::results::archive
