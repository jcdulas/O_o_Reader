#include "oor/results/core/ResultLayout.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <array>

namespace oor::results
{

std::string_view sortOrderName(SortOrder order) noexcept
{
    return order == SortOrder::Sort1 ? "sort1" : "sort2";
}

std::string_view resultVariantName(ResultVariant variant) noexcept
{
    switch (variant)
    {
    case ResultVariant::Plain:
        return "";
    case ResultVariant::ModalContribution:
        return "modalContribution";
    case ResultVariant::Psd:
        return "psd";
    case ResultVariant::AutoCorrelation:
        return "ato";
    case ResultVariant::CumulativeRms:
        return "crm";
    case ResultVariant::Rms:
        return "rms";
    case ResultVariant::NumberOfCrossings:
        return "no";
    case ResultVariant::PeakToPeak:
        return "pk";
    }
    return "";
}

namespace
{
constexpr std::array<ResultVariant, 7> VARIANTS = {
    ResultVariant::ModalContribution, ResultVariant::Psd, ResultVariant::AutoCorrelation,
    ResultVariant::CumulativeRms,     ResultVariant::Rms, ResultVariant::NumberOfCrossings,
    ResultVariant::PeakToPeak};
} // namespace

std::span<const ResultVariant> allResultVariants() noexcept
{
    return VARIANTS;
}

std::optional<ResultVariant> resultVariantFromName(std::string_view name)
{
    if (name.empty())
        return ResultVariant::Plain;
    for (const ResultVariant variant : VARIANTS)
        if (base::equalsIgnoreCase(resultVariantName(variant), name))
            return variant;
    return std::nullopt;
}

bool isRandomVariant(ResultVariant variant) noexcept
{
    return variant != ResultVariant::Plain && variant != ResultVariant::ModalContribution;
}

std::string_view valueKindName(ValueKind kind) noexcept
{
    switch (kind)
    {
    case ValueKind::Real:
        return "real";
    case ValueKind::Complex:
        return "complex";
    case ValueKind::Random:
        return "random";
    }
    return "real";
}

std::string_view resultFormatName(ResultFormat format) noexcept
{
    switch (format)
    {
    case ResultFormat::Real:
        return "real";
    case ResultFormat::RealImaginary:
        return "realImaginary";
    case ResultFormat::MagnitudePhase:
        return "magnitudePhase";
    case ResultFormat::Random:
        return "random";
    case ResultFormat::ModalContribution:
        return "modalContribution";
    }
    return "real";
}

std::string_view stepKindName(StepKind kind) noexcept
{
    switch (kind)
    {
    case StepKind::None:
        return "none";
    case StepKind::LoadCombinationSet:
        return "loadSet";
    case StepKind::Mode:
        return "mode";
    case StepKind::Frequency:
        return "frequency";
    case StepKind::Time:
        return "time";
    case StepKind::LoadFactor:
        return "loadFactor";
    }
    return "none";
}

std::string_view analysisTypeName(int analysisCode) noexcept
{
    switch (analysisCode)
    {
    case 0:
        return "none";
    case 1:
    case 101:
        return "static";
    case 2:
    case 103:
        return "modes";
    case 105:
        return "buckling";
    case 144:
        return "aeroelastic";
    case 3:
        return "differentialStiffness0";
    case 4:
        return "differentialStiffness1";
    case 5:
        return "frequency";
    case 6:
        return "transient";
    case 7:
        return "preBuckling";
    case 8:
        return "postBuckling";
    case 9:
        return "complexModes";
    case 10:
        return "nonlinearStatic";
    case 11:
        return "geometricNonlinearStatic";
    case 12:
        return "contran";
    default:
        return "unknown";
    }
}

StepKind stepKindOf(int analysisCode) noexcept
{
    switch (analysisCode)
    {
    case 1:
    case 101:
    case 144:
    case 3:
    case 4:
    case 7:
    case 8:
    case 11:
        return StepKind::LoadCombinationSet;
    case 2:
    case 103:
    case 105:
    case 9:
        return StepKind::Mode;
    case 5:
        return StepKind::Frequency;
    case 6:
    case 12:
        return StepKind::Time;
    case 10:
        return StepKind::LoadFactor;
    default:
        return StepKind::None;
    }
}

} // namespace oor::results
