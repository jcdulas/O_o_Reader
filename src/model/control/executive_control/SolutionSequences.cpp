#include "oor/model/control/executive_control/SolutionSequences.hpp"

namespace oor::model
{

const std::vector<SolutionSequence>& SolutionSequences::all() noexcept
{
    static const std::vector<SolutionSequence> SEQUENCES = {
        {101, "LinearStatics", "LinearStaticsNumber"},
        {103, "NormalModes", "NormalModesNumber"},
        {105, "LinearBuckling", "LinearBucklingNumber"},
        {106, "NonlinearStatics", "NonlinearStaticsNumber"},
        {107, "DirectComplexEigen", "DirectComplexEigenNumber"},
        {108, "DirectFreqResponse", "DirectFreqResponseNumber"},
        {109, "DirectTransient", "DirectTransientNumber"},
        {110, "ModalComplexEigen", "ModalComplexEigenNumber"},
        {111, "ModalFreqResponse", "ModalFreqResponseNumber"},
        {112, "ModalTransient", "ModalTransientNumber"},
        {114, "CyclicStatics", "CyclicStaticsNumber"},
        {115, "CyclicModes", "CyclicModesNumber"},
        {116, "CyclicBuckling", "CyclicBucklingNumber"},
        {118, "CyclicFreqResponse", "CyclicFreqResponseNumber"},
        {128, "NonlinearHarmonic", "NonlinearHarmonicNumber"},
        {129, "NonlinearTransient", "NonlinearTransientNumber"},
        {144, "StaticAeroelasticity", "StaticAeroelasticityNumber"},
        {145, "FlutterAnalysis", "FlutterAnalysisNumber"},
        {146, "AeroelasticResponse", "AeroelasticResponseNumber"},
        {153, "NonlinearSteadyHeat", "NonlinearSteadyHeatNumber"},
        {159, "NonlinearTransientHeat", "NonlinearTransientHeatNumber"},
        {200, "DesignOptimization", "DesignOptimizationNumber"},
        {400, "GeneralNonlinear", "GeneralNonlinearNumber"},
        {700, "NonlinearTransient", "ExplicitNonlinearNumber"}};
    return SEQUENCES;
}

std::optional<std::int64_t> SolutionSequences::numberOf(std::string_view name) noexcept
{
    for (const SolutionSequence& sequence : all())
    {
        if (sequence.name == name)
            return sequence.number;
    }
    return std::nullopt;
}

std::string_view SolutionSequences::nameOf(std::int64_t number) noexcept
{
    for (const SolutionSequence& sequence : all())
    {
        if (sequence.number == number)
            return sequence.name;
    }
    return {};
}

} // namespace oor::model
