#include "oor/model/control/statement/StatementDefinition.hpp"

namespace oor::model
{

namespace
{

// One statement of the Executive Control section. `shape` says how its text is read.
StatementDefinition& define(std::vector<StatementDefinition>& list, std::string name, ControlKind shape)
{
    StatementDefinition definition;
    definition.name = std::move(name);
    definition.section = SectionKind::Executive;
    definition.shape = shape;
    list.push_back(std::move(definition));
    return list.back();
}

std::vector<StatementDefinition> makeStatements()
{
    std::vector<StatementDefinition> list;
    list.reserve(24);
    // ID i1, i2: two names, unused by the program.
    define(list, "JobIdentifier", ControlKind::JobIdentifier);
    // SOL n, SOL name, SOL n,m (SOL 700,129), with SOLIN=dbset and NOEXE, and the SOL 700 keywords.
    define(list, "SolutionSequence", ControlKind::SolutionSequence)
        .describers({"SolutionInput", "NoExecution", "ExecutablePath", "StopPoint", "ProcessorCount",
                     "ExplicitDistributedParallel", "FluidStructureParallel", "MessagePassingLibrary",
                     "VersionSelection"});
    // TIME t1, t2: the CPU and I/O limits in minutes.
    define(list, "TimeLimit", ControlKind::TimeLimit);
    // DIAG k1, k2, ...: the diagnostics requested; several statements add up.
    define(list, "DiagnosticSelection", ControlKind::DiagnosticSelection);
    define(list, "ApplicationKind", ControlKind::Keyword).describers({"HeatTransferApplication", "CoupledApplication"});
    define(list, "DomainDecomposition", ControlKind::Keyword)
        .describers({"StatisticsOption",
                     "ModesDomain",
                     "FrequencyDomain",
                     "ComponentModeSynthesis",
                     "SensitivityDomain",
                     "NonlinearSolverDomain",
                     "StaticReduction",
                     "PartitionOption",
                     "DomainCount",
                     "UpdateFactor",
                     "PrintLevel",
                     "ClusterCount",
                     "DomainClusterSize",
                     "CompressionMethod",
                     "ModelDomain",
                     "RunOption",
                     "ParallelChildProcesses",
                     "MultipleParts",
                     "PartitionMethod",
                     "TipSizeLimit",
                     "VersionSelection",
                     "ScheduleOption",
                     "GroupSize",
                     "ReductionOption",
                     "MinDofs",
                     "ExpansionFactor",
                     "ExpansionFactorX"});
    // The format's ECHO statement has two formats, ECHOOFF and ECHOON.
    define(list, "ExecutiveEchoOff", ControlKind::Keyword);
    define(list, "ExecutiveEchoOn", ControlKind::Keyword);
    define(list, "GeometryCheck", ControlKind::Keyword)
        .describers({"MessageLimit", "MessageKind", "FatalLevel", "InformLevel", "WarningLevel", "SummaryOutput",
                     "AdvancedNonlinearElems", "NoMessages"});
    define(list, "ModelCheckOptions", ControlKind::Keyword)
        .describers({"MaterialDensityCheck", "MaterialExpansionCheck", "MaterialAnisotropicExpansionCheck",
                     "MaterialDampingCheck", "ModelCheckOnly", "PrintLevel"});
    define(list, "SparseSolverOptions", ControlKind::Keyword)
        .describers({"CompressionMethod", "OrderingMethod", "FactorMethod", "ModuleTimeRatio", "ModuleTimeStatistics",
                     "LowRankTolerance", "RefinementCount", "MaxResidualRatio", "MemoryIncrement",
                     "DirectSolverExpert"});
    return list;
}

} // namespace

const StatementCatalogue& executiveStatements()
{
    static const StatementCatalogue CATALOGUE(makeStatements());
    return CATALOGUE;
}

} // namespace oor::model
