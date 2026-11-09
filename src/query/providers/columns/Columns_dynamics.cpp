// The stored columns of the cards of the family "dynamics", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/dynamics/acousticSourceLoad/AcousticSourceLoadTable.hpp"
#include "oor/model/cards/dynamics/bucklingEigenMethod/BucklingEigenMethodTable.hpp"
#include "oor/model/cards/dynamics/complexEigenMethod/ComplexEigenMethodTable.hpp"
#include "oor/model/cards/dynamics/complexEigenPoles/ComplexEigenPolesTable.hpp"
#include "oor/model/cards/dynamics/dynamicLoadCombination/DynamicLoadCombinationTable.hpp"
#include "oor/model/cards/dynamics/dynamicLoadDelay/DynamicLoadDelayTable.hpp"
#include "oor/model/cards/dynamics/dynamicLoadPhase/DynamicLoadPhaseTable.hpp"
#include "oor/model/cards/dynamics/dynamicLoadScale/DynamicLoadScaleTable.hpp"
#include "oor/model/cards/dynamics/freqBetweenModes3/FreqBetweenModes3Table.hpp"
#include "oor/model/cards/dynamics/freqLinearSteps1/FreqLinearSteps1Table.hpp"
#include "oor/model/cards/dynamics/freqList/FreqListTable.hpp"
#include "oor/model/cards/dynamics/freqLogSteps2/FreqLogSteps2Table.hpp"
#include "oor/model/cards/dynamics/freqModeFractions5/FreqModeFractions5Table.hpp"
#include "oor/model/cards/dynamics/freqResponseLoad1/FreqResponseLoad1Table.hpp"
#include "oor/model/cards/dynamics/freqResponseLoad2/FreqResponseLoad2Table.hpp"
#include "oor/model/cards/dynamics/freqSpreadAroundModes4/FreqSpreadAroundModes4Table.hpp"
#include "oor/model/cards/dynamics/frfAssemblyLoadDelay/FrfAssemblyLoadDelayTable.hpp"
#include "oor/model/cards/dynamics/frfAssemblyLoadPhase/FrfAssemblyLoadPhaseTable.hpp"
#include "oor/model/cards/dynamics/frfAssemblyLoadScale/FrfAssemblyLoadScaleTable.hpp"
#include "oor/model/cards/dynamics/frfComponent/FrfComponentTable.hpp"
#include "oor/model/cards/dynamics/frfConnection/FrfConnectionTable.hpp"
#include "oor/model/cards/dynamics/frfExcitationDof/FrfExcitationDofTable.hpp"
#include "oor/model/cards/dynamics/frfExcitationDofList/FrfExcitationDofListTable.hpp"
#include "oor/model/cards/dynamics/frfFlexibleConnection/FrfFlexibleConnectionTable.hpp"
#include "oor/model/cards/dynamics/frfReleasedDofs/FrfReleasedDofsTable.hpp"
#include "oor/model/cards/dynamics/frfSinglePointConstr/FrfSinglePointConstrTable.hpp"
#include "oor/model/cards/dynamics/heatShellNodalLoad/HeatShellNodalLoadTable.hpp"
#include "oor/model/cards/dynamics/hybridDamping/HybridDampingTable.hpp"
#include "oor/model/cards/dynamics/hybridDampingRotorOptions/HybridDampingRotorOptionsTable.hpp"
#include "oor/model/cards/dynamics/initialElemValues/InitialElemValuesTable.hpp"
#include "oor/model/cards/dynamics/initialValuesUserRoutine/InitialValuesUserRoutineTable.hpp"
#include "oor/model/cards/dynamics/initialVelocityField/InitialVelocityFieldTable.hpp"
#include "oor/model/cards/dynamics/lanczosEigenMethod/LanczosEigenMethodTable.hpp"
#include "oor/model/cards/dynamics/nonlinearNegativePowerLoad4/NonlinearNegativePowerLoad4Table.hpp"
#include "oor/model/cards/dynamics/nonlinearPositivePowerLoad3/NonlinearPositivePowerLoad3Table.hpp"
#include "oor/model/cards/dynamics/nonlinearProductLoad2/NonlinearProductLoad2Table.hpp"
#include "oor/model/cards/dynamics/nonlinearTableLoad1/NonlinearTableLoad1Table.hpp"
#include "oor/model/cards/dynamics/randomSpectralDensity/RandomSpectralDensityTable.hpp"
#include "oor/model/cards/dynamics/randomTimeLags/RandomTimeLagsTable.hpp"
#include "oor/model/cards/dynamics/realEigenMethod/RealEigenMethodTable.hpp"
#include "oor/model/cards/dynamics/rotorHybridDamping/RotorHybridDampingTable.hpp"
#include "oor/model/cards/dynamics/rotorUnbalanceLoad/RotorUnbalanceLoadTable.hpp"
#include "oor/model/cards/dynamics/transferFunction/TransferFunctionTable.hpp"
#include "oor/model/cards/dynamics/transientFormulaLoad2/TransientFormulaLoad2Table.hpp"
#include "oor/model/cards/dynamics/transientInitialCondition/TransientInitialConditionTable.hpp"
#include "oor/model/cards/dynamics/transientTableLoad1/TransientTableLoad1Table.hpp"
#include "oor/model/cards/loads/LoadClosureSupport.hpp"
#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

void registerDynamicsColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::AcousticSourceLoadTable;
        const std::uint16_t type = model::typeIndexOf<AcousticSourceLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSourceLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "excitationId", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSourceLoadTable>().excitationIdColumn()); });
        registerColumn(catalogue, type, "delayId", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSourceLoadTable>().delayIdColumn()); });
        registerColumn(catalogue, type, "delay", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSourceLoadTable>().delayColumn()); });
        registerColumn(catalogue, type, "phaseId", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSourceLoadTable>().phaseIdColumn()); });
        registerColumn(catalogue, type, "phase", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSourceLoadTable>().phaseColumn()); });
        registerColumn(catalogue, type, "powerTableId", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSourceLoadTable>().powerTableIdColumn()); });
        registerColumn(catalogue, type, "power", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSourceLoadTable>().powerColumn()); });
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSourceLoadTable>().densityColumn()); });
        registerColumn(catalogue, type, "bulkModulus", [](const auto& model)
                       { return exportColumn(model->template table<AcousticSourceLoadTable>().bulkModulusColumn()); });
    }
    {
        using model::HybridDampingRotorOptionsTable;
        const std::uint16_t type = model::typeIndexOf<HybridDampingRotorOptionsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HybridDampingRotorOptionsTable>().idColumn()); });
    }
    {
        using model::DynamicLoadScaleTable;
        const std::uint16_t type = model::typeIndexOf<DynamicLoadScaleTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DynamicLoadScaleTable>().idColumn()); });
        registerColumn(catalogue, type, "points", [](const auto& model)
                       { return exportColumn(model->template table<DynamicLoadScaleTable>().pointsColumn()); });
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<DynamicLoadScaleTable>().componentsColumn()); });
        registerColumn(catalogue, type, "scaleFactors", [](const auto& model)
                       { return exportColumn(model->template table<DynamicLoadScaleTable>().scaleFactorsColumn()); });
    }
    {
        using model::DynamicLoadDelayTable;
        const std::uint16_t type = model::typeIndexOf<DynamicLoadDelayTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DynamicLoadDelayTable>().idColumn()); });
        registerColumn(catalogue, type, "points", [](const auto& model)
                       { return exportColumn(model->template table<DynamicLoadDelayTable>().pointsColumn()); });
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<DynamicLoadDelayTable>().componentsColumn()); });
        registerColumn(catalogue, type, "delays", [](const auto& model)
                       { return exportColumn(model->template table<DynamicLoadDelayTable>().delaysColumn()); });
    }
    {
        using model::DynamicLoadCombinationTable;
        const std::uint16_t type = model::typeIndexOf<DynamicLoadCombinationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DynamicLoadCombinationTable>().idColumn()); });
        registerColumn(
            catalogue, type, "scaleFactor", [](const auto& model)
            { return exportColumn(model->template table<DynamicLoadCombinationTable>().scaleFactorColumn()); });
        registerColumn(catalogue, type, "termScaleFactors",
                       [](const auto& model)
                       {
                           return exportVariableColumn(
                               model->template table<DynamicLoadCombinationTable>().termScaleFactorsColumn());
                       });
        registerColumn(catalogue, type, "termLoadSetIds",
                       [](const auto& model)
                       {
                           return exportVariableColumn(
                               model->template table<DynamicLoadCombinationTable>().termLoadSetIdsColumn());
                       });
    }
    {
        using model::DynamicLoadPhaseTable;
        const std::uint16_t type = model::typeIndexOf<DynamicLoadPhaseTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DynamicLoadPhaseTable>().idColumn()); });
        registerColumn(catalogue, type, "points", [](const auto& model)
                       { return exportColumn(model->template table<DynamicLoadPhaseTable>().pointsColumn()); });
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<DynamicLoadPhaseTable>().componentsColumn()); });
        registerColumn(catalogue, type, "phaseAngles", [](const auto& model)
                       { return exportColumn(model->template table<DynamicLoadPhaseTable>().phaseAnglesColumn()); });
    }
    {
        using model::BucklingEigenMethodTable;
        const std::uint16_t type = model::typeIndexOf<BucklingEigenMethodTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BucklingEigenMethodTable>().idColumn()); });
        registerColumn(catalogue, type, "method", [](const auto& model)
                       { return exportColumn(model->template table<BucklingEigenMethodTable>().methodColumn()); });
        registerColumn(
            catalogue, type, "lowerEigenvalue", [](const auto& model)
            { return exportColumn(model->template table<BucklingEigenMethodTable>().lowerEigenvalueColumn()); });
        registerColumn(
            catalogue, type, "upperEigenvalue", [](const auto& model)
            { return exportColumn(model->template table<BucklingEigenMethodTable>().upperEigenvalueColumn()); });
        registerColumn(
            catalogue, type, "estimatedModeCount", [](const auto& model)
            { return exportColumn(model->template table<BucklingEigenMethodTable>().estimatedModeCountColumn()); });
        registerColumn(
            catalogue, type, "desiredPositiveModeCount",
            [](const auto& model)
            {
                return exportColumn(model->template table<BucklingEigenMethodTable>().desiredPositiveModeCountColumn());
            });
        registerColumn(
            catalogue, type, "desiredNegativeModeCount",
            [](const auto& model)
            {
                return exportColumn(model->template table<BucklingEigenMethodTable>().desiredNegativeModeCountColumn());
            });
        registerColumn(catalogue, type, "norm", [](const auto& model)
                       { return exportColumn(model->template table<BucklingEigenMethodTable>().normColumn()); });
        registerColumn(catalogue, type, "normNode", [](const auto& model)
                       { return exportColumn(model->template table<BucklingEigenMethodTable>().normNodeColumn()); });
        registerColumn(
            catalogue, type, "normComponent", [](const auto& model)
            { return exportColumn(model->template table<BucklingEigenMethodTable>().normComponentColumn()); });
    }
    {
        using model::ComplexEigenMethodTable;
        const std::uint16_t type = model::typeIndexOf<ComplexEigenMethodTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ComplexEigenMethodTable>().idColumn()); });
        registerColumn(catalogue, type, "methodName", [](const auto& model)
                       { return exportColumn(model->template table<ComplexEigenMethodTable>().methodNameColumn()); });
        registerColumn(catalogue, type, "norm", [](const auto& model)
                       { return exportColumn(model->template table<ComplexEigenMethodTable>().normColumn()); });
        registerColumn(catalogue, type, "normNode", [](const auto& model)
                       { return exportColumn(model->template table<ComplexEigenMethodTable>().normNodeColumn()); });
        registerColumn(
            catalogue, type, "normComponent", [](const auto& model)
            { return exportColumn(model->template table<ComplexEigenMethodTable>().normComponentColumn()); });
        registerColumn(
            catalogue, type, "convergenceTolerance", [](const auto& model)
            { return exportColumn(model->template table<ComplexEigenMethodTable>().convergenceToleranceColumn()); });
        registerColumn(
            catalogue, type, "desiredModeCount0", [](const auto& model)
            { return exportColumn(model->template table<ComplexEigenMethodTable>().desiredModeCount0Column()); });
        registerColumn(
            catalogue, type, "alphaA", [](const auto& model)
            { return exportVariableColumn(model->template table<ComplexEigenMethodTable>().alphaAColumn()); });
        registerColumn(
            catalogue, type, "omegaA", [](const auto& model)
            { return exportVariableColumn(model->template table<ComplexEigenMethodTable>().omegaAColumn()); });
        registerColumn(
            catalogue, type, "alphaB", [](const auto& model)
            { return exportVariableColumn(model->template table<ComplexEigenMethodTable>().alphaBColumn()); });
        registerColumn(
            catalogue, type, "omegaB", [](const auto& model)
            { return exportVariableColumn(model->template table<ComplexEigenMethodTable>().omegaBColumn()); });
        registerColumn(
            catalogue, type, "regionWidth", [](const auto& model)
            { return exportVariableColumn(model->template table<ComplexEigenMethodTable>().regionWidthColumn()); });
        registerColumn(catalogue, type, "estimatedModeCount",
                       [](const auto& model)
                       {
                           return exportVariableColumn(
                               model->template table<ComplexEigenMethodTable>().estimatedModeCountColumn());
                       });
        registerColumn(
            catalogue, type, "maximumBlockSize",
            [](const auto& model)
            {
                return exportVariableColumn(model->template table<ComplexEigenMethodTable>().maximumBlockSizeColumn());
            });
        registerColumn(
            catalogue, type, "initialBlockSize",
            [](const auto& model)
            {
                return exportVariableColumn(model->template table<ComplexEigenMethodTable>().initialBlockSizeColumn());
            });
        registerColumn(
            catalogue, type, "solveInterval", [](const auto& model)
            { return exportVariableColumn(model->template table<ComplexEigenMethodTable>().solveIntervalColumn()); });
        registerColumn(
            catalogue, type, "desiredModeCount",
            [](const auto& model)
            {
                return exportVariableColumn(model->template table<ComplexEigenMethodTable>().desiredModeCountColumn());
            });
    }
    {
        using model::ComplexEigenPolesTable;
        const std::uint16_t type = model::typeIndexOf<ComplexEigenPolesTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ComplexEigenPolesTable>().idColumn()); });
        registerColumn(catalogue, type, "realPart1", [](const auto& model)
                       { return exportColumn(model->template table<ComplexEigenPolesTable>().realPart1Column()); });
        registerColumn(
            catalogue, type, "imaginaryPart1", [](const auto& model)
            { return exportColumn(model->template table<ComplexEigenPolesTable>().imaginaryPart1Column()); });
        registerColumn(catalogue, type, "multiplicity1", [](const auto& model)
                       { return exportColumn(model->template table<ComplexEigenPolesTable>().multiplicity1Column()); });
        registerColumn(catalogue, type, "realPart2", [](const auto& model)
                       { return exportColumn(model->template table<ComplexEigenPolesTable>().realPart2Column()); });
        registerColumn(
            catalogue, type, "imaginaryPart2", [](const auto& model)
            { return exportColumn(model->template table<ComplexEigenPolesTable>().imaginaryPart2Column()); });
        registerColumn(catalogue, type, "multiplicity2", [](const auto& model)
                       { return exportColumn(model->template table<ComplexEigenPolesTable>().multiplicity2Column()); });
    }
    {
        using model::RealEigenMethodTable;
        const std::uint16_t type = model::typeIndexOf<RealEigenMethodTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RealEigenMethodTable>().idColumn()); });
        registerColumn(catalogue, type, "method", [](const auto& model)
                       { return exportColumn(model->template table<RealEigenMethodTable>().methodColumn()); });
        registerColumn(catalogue, type, "frequency1", [](const auto& model)
                       { return exportColumn(model->template table<RealEigenMethodTable>().frequency1Column()); });
        registerColumn(catalogue, type, "frequency2", [](const auto& model)
                       { return exportColumn(model->template table<RealEigenMethodTable>().frequency2Column()); });
        registerColumn(
            catalogue, type, "estimatedModeCount", [](const auto& model)
            { return exportColumn(model->template table<RealEigenMethodTable>().estimatedModeCountColumn()); });
        registerColumn(
            catalogue, type, "desiredModeCount", [](const auto& model)
            { return exportColumn(model->template table<RealEigenMethodTable>().desiredModeCountColumn()); });
        registerColumn(catalogue, type, "normalization", [](const auto& model)
                       { return exportColumn(model->template table<RealEigenMethodTable>().normalizationColumn()); });
        registerColumn(catalogue, type, "normNode", [](const auto& model)
                       { return exportColumn(model->template table<RealEigenMethodTable>().normNodeColumn()); });
        registerColumn(catalogue, type, "normComponent", [](const auto& model)
                       { return exportColumn(model->template table<RealEigenMethodTable>().normComponentColumn()); });
    }
    {
        using model::LanczosEigenMethodTable;
        const std::uint16_t type = model::typeIndexOf<LanczosEigenMethodTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<LanczosEigenMethodTable>().idColumn()); });
        registerColumn(
            catalogue, type, "lowerFrequency", [](const auto& model)
            { return exportColumn(model->template table<LanczosEigenMethodTable>().lowerFrequencyColumn()); });
        registerColumn(
            catalogue, type, "upperFrequency", [](const auto& model)
            { return exportColumn(model->template table<LanczosEigenMethodTable>().upperFrequencyColumn()); });
        registerColumn(
            catalogue, type, "desiredModeCount", [](const auto& model)
            { return exportColumn(model->template table<LanczosEigenMethodTable>().desiredModeCountColumn()); });
        registerColumn(catalogue, type, "messageLevel", [](const auto& model)
                       { return exportColumn(model->template table<LanczosEigenMethodTable>().messageLevelColumn()); });
        registerColumn(catalogue, type, "maximumSet", [](const auto& model)
                       { return exportColumn(model->template table<LanczosEigenMethodTable>().maximumSetColumn()); });
        registerColumn(catalogue, type, "shiftScale", [](const auto& model)
                       { return exportColumn(model->template table<LanczosEigenMethodTable>().shiftScaleColumn()); });
        registerColumn(catalogue, type, "norm", [](const auto& model)
                       { return exportColumn(model->template table<LanczosEigenMethodTable>().normColumn()); });
        registerColumn(catalogue, type, "alpha", [](const auto& model)
                       { return exportColumn(model->template table<LanczosEigenMethodTable>().alphaColumn()); });
        registerColumn(
            catalogue, type, "frequencySegmentCount", [](const auto& model)
            { return exportColumn(model->template table<LanczosEigenMethodTable>().frequencySegmentCountColumn()); });
        registerColumn(catalogue, type, "segmentFrequencies",
                       [](const auto& model)
                       {
                           return exportVariableColumn(
                               model->template table<LanczosEigenMethodTable>().segmentFrequenciesColumn());
                       });
    }
    {
        using model::FrfAssemblyLoadDelayTable;
        const std::uint16_t type = model::typeIndexOf<FrfAssemblyLoadDelayTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FrfAssemblyLoadDelayTable>().idColumn()); });
        registerColumn(
            catalogue, type, "frfComponentId", [](const auto& model)
            { return exportColumn(model->template table<FrfAssemblyLoadDelayTable>().frfComponentIdColumn()); });
        registerColumn(
            catalogue, type, "frfComponentName", [](const auto& model)
            { return exportTextColumn(model->template table<FrfAssemblyLoadDelayTable>().frfComponentNameColumn()); });
        registerColumn(catalogue, type, "pointId", [](const auto& model)
                       { return exportColumn(model->template table<FrfAssemblyLoadDelayTable>().pointIdColumn()); });
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<FrfAssemblyLoadDelayTable>().componentsColumn()); });
        registerColumn(catalogue, type, "delay", [](const auto& model)
                       { return exportColumn(model->template table<FrfAssemblyLoadDelayTable>().delayColumn()); });
    }
    {
        using model::FrfAssemblyLoadScaleTable;
        const std::uint16_t type = model::typeIndexOf<FrfAssemblyLoadScaleTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FrfAssemblyLoadScaleTable>().idColumn()); });
        registerColumn(
            catalogue, type, "frfComponentId", [](const auto& model)
            { return exportColumn(model->template table<FrfAssemblyLoadScaleTable>().frfComponentIdColumn()); });
        registerColumn(
            catalogue, type, "frfComponentName", [](const auto& model)
            { return exportTextColumn(model->template table<FrfAssemblyLoadScaleTable>().frfComponentNameColumn()); });
        registerColumn(catalogue, type, "pointId", [](const auto& model)
                       { return exportColumn(model->template table<FrfAssemblyLoadScaleTable>().pointIdColumn()); });
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<FrfAssemblyLoadScaleTable>().componentsColumn()); });
        registerColumn(
            catalogue, type, "scaleFactor", [](const auto& model)
            { return exportColumn(model->template table<FrfAssemblyLoadScaleTable>().scaleFactorColumn()); });
    }
    {
        using model::FrfAssemblyLoadPhaseTable;
        const std::uint16_t type = model::typeIndexOf<FrfAssemblyLoadPhaseTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FrfAssemblyLoadPhaseTable>().idColumn()); });
        registerColumn(
            catalogue, type, "frfComponentId", [](const auto& model)
            { return exportColumn(model->template table<FrfAssemblyLoadPhaseTable>().frfComponentIdColumn()); });
        registerColumn(
            catalogue, type, "frfComponentName", [](const auto& model)
            { return exportTextColumn(model->template table<FrfAssemblyLoadPhaseTable>().frfComponentNameColumn()); });
        registerColumn(catalogue, type, "pointId", [](const auto& model)
                       { return exportColumn(model->template table<FrfAssemblyLoadPhaseTable>().pointIdColumn()); });
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<FrfAssemblyLoadPhaseTable>().componentsColumn()); });
        registerColumn(catalogue, type, "phase", [](const auto& model)
                       { return exportColumn(model->template table<FrfAssemblyLoadPhaseTable>().phaseColumn()); });
    }
    {
        using model::FreqListTable;
        const std::uint16_t type = model::typeIndexOf<FreqListTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FreqListTable>().idColumn()); });
        registerColumn(catalogue, type, "frequencies", [](const auto& model)
                       { return exportVariableColumn(model->template table<FreqListTable>().frequenciesColumn()); });
    }
    {
        using model::FreqLinearSteps1Table;
        const std::uint16_t type = model::typeIndexOf<FreqLinearSteps1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FreqLinearSteps1Table>().idColumn()); });
        registerColumn(catalogue, type, "frequency1", [](const auto& model)
                       { return exportColumn(model->template table<FreqLinearSteps1Table>().frequency1Column()); });
        registerColumn(
            catalogue, type, "frequencyIncrement", [](const auto& model)
            { return exportColumn(model->template table<FreqLinearSteps1Table>().frequencyIncrementColumn()); });
        registerColumn(catalogue, type, "incrementCount", [](const auto& model)
                       { return exportColumn(model->template table<FreqLinearSteps1Table>().incrementCountColumn()); });
    }
    {
        using model::FreqLogSteps2Table;
        const std::uint16_t type = model::typeIndexOf<FreqLogSteps2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FreqLogSteps2Table>().idColumn()); });
        registerColumn(catalogue, type, "frequency1", [](const auto& model)
                       { return exportColumn(model->template table<FreqLogSteps2Table>().frequency1Column()); });
        registerColumn(catalogue, type, "frequency2", [](const auto& model)
                       { return exportColumn(model->template table<FreqLogSteps2Table>().frequency2Column()); });
        registerColumn(catalogue, type, "intervalCount", [](const auto& model)
                       { return exportColumn(model->template table<FreqLogSteps2Table>().intervalCountColumn()); });
    }
    {
        using model::FreqBetweenModes3Table;
        const std::uint16_t type = model::typeIndexOf<FreqBetweenModes3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FreqBetweenModes3Table>().idColumn()); });
        registerColumn(catalogue, type, "frequency1", [](const auto& model)
                       { return exportColumn(model->template table<FreqBetweenModes3Table>().frequency1Column()); });
        registerColumn(catalogue, type, "frequency2", [](const auto& model)
                       { return exportColumn(model->template table<FreqBetweenModes3Table>().frequency2Column()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportColumn(model->template table<FreqBetweenModes3Table>().kindColumn()); });
        registerColumn(
            catalogue, type, "frequencyCount", [](const auto& model)
            { return exportColumn(model->template table<FreqBetweenModes3Table>().frequencyCountColumn()); });
        registerColumn(catalogue, type, "clustering", [](const auto& model)
                       { return exportColumn(model->template table<FreqBetweenModes3Table>().clusteringColumn()); });
    }
    {
        using model::FreqSpreadAroundModes4Table;
        const std::uint16_t type = model::typeIndexOf<FreqSpreadAroundModes4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FreqSpreadAroundModes4Table>().idColumn()); });
        registerColumn(
            catalogue, type, "frequency1", [](const auto& model)
            { return exportColumn(model->template table<FreqSpreadAroundModes4Table>().frequency1Column()); });
        registerColumn(
            catalogue, type, "frequency2", [](const auto& model)
            { return exportColumn(model->template table<FreqSpreadAroundModes4Table>().frequency2Column()); });
        registerColumn(
            catalogue, type, "frequencySpread", [](const auto& model)
            { return exportColumn(model->template table<FreqSpreadAroundModes4Table>().frequencySpreadColumn()); });
        registerColumn(
            catalogue, type, "frequencyCount", [](const auto& model)
            { return exportColumn(model->template table<FreqSpreadAroundModes4Table>().frequencyCountColumn()); });
    }
    {
        using model::FreqModeFractions5Table;
        const std::uint16_t type = model::typeIndexOf<FreqModeFractions5Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FreqModeFractions5Table>().idColumn()); });
        registerColumn(catalogue, type, "frequency1", [](const auto& model)
                       { return exportColumn(model->template table<FreqModeFractions5Table>().frequency1Column()); });
        registerColumn(catalogue, type, "frequency2", [](const auto& model)
                       { return exportColumn(model->template table<FreqModeFractions5Table>().frequency2Column()); });
        registerColumn(
            catalogue, type, "fractions", [](const auto& model)
            { return exportVariableColumn(model->template table<FreqModeFractions5Table>().fractionsColumn()); });
    }
    {
        using model::FrfComponentTable;
        const std::uint16_t type = model::typeIndexOf<FrfComponentTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FrfComponentTable>().idColumn()); });
        registerColumn(catalogue, type, "componentName", [](const auto& model)
                       { return exportTextColumn(model->template table<FrfComponentTable>().nameColumn()); });
        registerColumn(catalogue, type, "medium", [](const auto& model)
                       { return exportColumn(model->template table<FrfComponentTable>().mediumColumn()); });
        registerColumn(catalogue, type, "unitNumber", [](const auto& model)
                       { return exportColumn(model->template table<FrfComponentTable>().unitNumberColumn()); });
        registerColumn(catalogue, type, "lengthScaleFactor", [](const auto& model)
                       { return exportColumn(model->template table<FrfComponentTable>().lengthScaleFactorColumn()); });
        registerColumn(catalogue, type, "forceScaleFactor", [](const auto& model)
                       { return exportColumn(model->template table<FrfComponentTable>().forceScaleFactorColumn()); });
    }
    {
        using model::FrfConnectionTable;
        const std::uint16_t type = model::typeIndexOf<FrfConnectionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FrfConnectionTable>().idColumn()); });
    }
    {
        using model::FrfFlexibleConnectionTable;
        const std::uint16_t type = model::typeIndexOf<FrfFlexibleConnectionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FrfFlexibleConnectionTable>().idColumn()); });
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<FrfFlexibleConnectionTable>().componentColumn()); });
    }
    {
        using model::FrfReleasedDofsTable;
        const std::uint16_t type = model::typeIndexOf<FrfReleasedDofsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FrfReleasedDofsTable>().idColumn()); });
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<FrfReleasedDofsTable>().componentsColumn()); });
    }
    {
        using model::FrfSinglePointConstrTable;
        const std::uint16_t type = model::typeIndexOf<FrfSinglePointConstrTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FrfSinglePointConstrTable>().idColumn()); });
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<FrfSinglePointConstrTable>().componentsColumn()); });
    }
    {
        using model::FrfExcitationDofTable;
        const std::uint16_t type = model::typeIndexOf<FrfExcitationDofTable>();
        registerColumn(catalogue, type, "pointId", [](const auto& model)
                       { return exportColumn(model->template table<FrfExcitationDofTable>().pointIdColumn()); });
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<FrfExcitationDofTable>().componentColumn()); });
        registerColumn(catalogue, type, "entryLabel", [](const auto& model)
                       { return exportTextColumn(model->template table<FrfExcitationDofTable>().entryLabelColumn()); });
    }
    {
        using model::FrfExcitationDofListTable;
        const std::uint16_t type = model::typeIndexOf<FrfExcitationDofListTable>();
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<FrfExcitationDofListTable>().componentColumn()); });
        registerColumn(
            catalogue, type, "runs", [](const auto& model)
            { return exportVariableColumn(model->template table<FrfExcitationDofListTable>().runsColumn()); });
    }
    {
        using model::HybridDampingTable;
        const std::uint16_t type = model::typeIndexOf<HybridDampingTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HybridDampingTable>().idColumn()); });
    }
    {
        using model::NonlinearTableLoad1Table;
        const std::uint16_t type = model::typeIndexOf<NonlinearTableLoad1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearTableLoad1Table>().idColumn()); });
        registerColumn(catalogue, type, "nodeI", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearTableLoad1Table>().nodeIColumn()); });
        registerColumn(catalogue, type, "componentsI", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearTableLoad1Table>().componentsIColumn()); });
        registerColumn(catalogue, type, "scaleFactor", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearTableLoad1Table>().scaleFactorColumn()); });
        registerColumn(catalogue, type, "nodeJ", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearTableLoad1Table>().nodeJColumn()); });
        registerColumn(catalogue, type, "componentsJ", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearTableLoad1Table>().componentsJColumn()); });
        registerColumn(catalogue, type, "tableId", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearTableLoad1Table>().tableIdColumn()); });
    }
    {
        using model::NonlinearProductLoad2Table;
        const std::uint16_t type = model::typeIndexOf<NonlinearProductLoad2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearProductLoad2Table>().idColumn()); });
        registerColumn(catalogue, type, "nodeI", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearProductLoad2Table>().nodeIColumn()); });
        registerColumn(
            catalogue, type, "componentsI", [](const auto& model)
            { return exportColumn(model->template table<NonlinearProductLoad2Table>().componentsIColumn()); });
        registerColumn(
            catalogue, type, "scaleFactor", [](const auto& model)
            { return exportColumn(model->template table<NonlinearProductLoad2Table>().scaleFactorColumn()); });
        registerColumn(catalogue, type, "nodeJ", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearProductLoad2Table>().nodeJColumn()); });
        registerColumn(
            catalogue, type, "componentsJ", [](const auto& model)
            { return exportColumn(model->template table<NonlinearProductLoad2Table>().componentsJColumn()); });
        registerColumn(catalogue, type, "nodeK", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearProductLoad2Table>().nodeKColumn()); });
        registerColumn(
            catalogue, type, "componentsK", [](const auto& model)
            { return exportColumn(model->template table<NonlinearProductLoad2Table>().componentsKColumn()); });
    }
    {
        using model::NonlinearPositivePowerLoad3Table;
        const std::uint16_t type = model::typeIndexOf<NonlinearPositivePowerLoad3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearPositivePowerLoad3Table>().idColumn()); });
        registerColumn(
            catalogue, type, "nodeI", [](const auto& model)
            { return exportColumn(model->template table<NonlinearPositivePowerLoad3Table>().nodeIColumn()); });
        registerColumn(
            catalogue, type, "componentsI", [](const auto& model)
            { return exportColumn(model->template table<NonlinearPositivePowerLoad3Table>().componentsIColumn()); });
        registerColumn(
            catalogue, type, "scaleFactor", [](const auto& model)
            { return exportColumn(model->template table<NonlinearPositivePowerLoad3Table>().scaleFactorColumn()); });
        registerColumn(
            catalogue, type, "nodeJ", [](const auto& model)
            { return exportColumn(model->template table<NonlinearPositivePowerLoad3Table>().nodeJColumn()); });
        registerColumn(
            catalogue, type, "componentsJ", [](const auto& model)
            { return exportColumn(model->template table<NonlinearPositivePowerLoad3Table>().componentsJColumn()); });
        registerColumn(
            catalogue, type, "exponent", [](const auto& model)
            { return exportColumn(model->template table<NonlinearPositivePowerLoad3Table>().exponentColumn()); });
    }
    {
        using model::NonlinearNegativePowerLoad4Table;
        const std::uint16_t type = model::typeIndexOf<NonlinearNegativePowerLoad4Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearNegativePowerLoad4Table>().idColumn()); });
        registerColumn(
            catalogue, type, "nodeI", [](const auto& model)
            { return exportColumn(model->template table<NonlinearNegativePowerLoad4Table>().nodeIColumn()); });
        registerColumn(
            catalogue, type, "componentsI", [](const auto& model)
            { return exportColumn(model->template table<NonlinearNegativePowerLoad4Table>().componentsIColumn()); });
        registerColumn(
            catalogue, type, "scaleFactor", [](const auto& model)
            { return exportColumn(model->template table<NonlinearNegativePowerLoad4Table>().scaleFactorColumn()); });
        registerColumn(
            catalogue, type, "nodeJ", [](const auto& model)
            { return exportColumn(model->template table<NonlinearNegativePowerLoad4Table>().nodeJColumn()); });
        registerColumn(
            catalogue, type, "componentsJ", [](const auto& model)
            { return exportColumn(model->template table<NonlinearNegativePowerLoad4Table>().componentsJColumn()); });
        registerColumn(
            catalogue, type, "exponent", [](const auto& model)
            { return exportColumn(model->template table<NonlinearNegativePowerLoad4Table>().exponentColumn()); });
    }
    {
        using model::RandomSpectralDensityTable;
        const std::uint16_t type = model::typeIndexOf<RandomSpectralDensityTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RandomSpectralDensityTable>().idColumn()); });
        registerColumn(catalogue, type, "subcaseJ", [](const auto& model)
                       { return exportColumn(model->template table<RandomSpectralDensityTable>().subcaseJColumn()); });
        registerColumn(catalogue, type, "subcaseK", [](const auto& model)
                       { return exportColumn(model->template table<RandomSpectralDensityTable>().subcaseKColumn()); });
        registerColumn(catalogue, type, "realPart", [](const auto& model)
                       { return exportColumn(model->template table<RandomSpectralDensityTable>().realPartColumn()); });
        registerColumn(
            catalogue, type, "imaginaryPart", [](const auto& model)
            { return exportColumn(model->template table<RandomSpectralDensityTable>().imaginaryPartColumn()); });
        registerColumn(catalogue, type, "tableId", [](const auto& model)
                       { return exportColumn(model->template table<RandomSpectralDensityTable>().tableIdColumn()); });
    }
    {
        using model::RandomTimeLagsTable;
        const std::uint16_t type = model::typeIndexOf<RandomTimeLagsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RandomTimeLagsTable>().idColumn()); });
        registerColumn(catalogue, type, "count", [](const auto& model)
                       { return exportColumn(model->template table<RandomTimeLagsTable>().countColumn()); });
        registerColumn(catalogue, type, "startTime", [](const auto& model)
                       { return exportColumn(model->template table<RandomTimeLagsTable>().startTimeColumn()); });
        registerColumn(catalogue, type, "maximumTime", [](const auto& model)
                       { return exportColumn(model->template table<RandomTimeLagsTable>().maximumTimeColumn()); });
    }
    {
        using model::FreqResponseLoad1Table;
        const std::uint16_t type = model::typeIndexOf<FreqResponseLoad1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FreqResponseLoad1Table>().idColumn()); });
        registerColumn(catalogue, type, "excitationId", [](const auto& model)
                       { return exportColumn(model->template table<FreqResponseLoad1Table>().excitationIdColumn()); });
        registerColumn(catalogue, type, "delayId", [](const auto& model)
                       { return exportColumn(model->template table<FreqResponseLoad1Table>().delayIdColumn()); });
        registerColumn(catalogue, type, "delay", [](const auto& model)
                       { return exportColumn(model->template table<FreqResponseLoad1Table>().delayColumn()); });
        registerColumn(catalogue, type, "phaseId", [](const auto& model)
                       { return exportColumn(model->template table<FreqResponseLoad1Table>().phaseIdColumn()); });
        registerColumn(catalogue, type, "phase", [](const auto& model)
                       { return exportColumn(model->template table<FreqResponseLoad1Table>().phaseColumn()); });
        registerColumn(catalogue, type, "realTableId", [](const auto& model)
                       { return exportColumn(model->template table<FreqResponseLoad1Table>().realTableIdColumn()); });
        registerColumn(catalogue, type, "realValue", [](const auto& model)
                       { return exportColumn(model->template table<FreqResponseLoad1Table>().realValueColumn()); });
        registerColumn(
            catalogue, type, "imaginaryTableId", [](const auto& model)
            { return exportColumn(model->template table<FreqResponseLoad1Table>().imaginaryTableIdColumn()); });
        registerColumn(
            catalogue, type, "imaginaryValue", [](const auto& model)
            { return exportColumn(model->template table<FreqResponseLoad1Table>().imaginaryValueColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportColumn(model->template table<FreqResponseLoad1Table>().kindColumn()); });
    }
    {
        using model::FreqResponseLoad2Table;
        const std::uint16_t type = model::typeIndexOf<FreqResponseLoad2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FreqResponseLoad2Table>().idColumn()); });
        registerColumn(catalogue, type, "excitationId", [](const auto& model)
                       { return exportColumn(model->template table<FreqResponseLoad2Table>().excitationIdColumn()); });
        registerColumn(catalogue, type, "delayId", [](const auto& model)
                       { return exportColumn(model->template table<FreqResponseLoad2Table>().delayIdColumn()); });
        registerColumn(catalogue, type, "delay", [](const auto& model)
                       { return exportColumn(model->template table<FreqResponseLoad2Table>().delayColumn()); });
        registerColumn(catalogue, type, "phaseId", [](const auto& model)
                       { return exportColumn(model->template table<FreqResponseLoad2Table>().phaseIdColumn()); });
        registerColumn(catalogue, type, "phase", [](const auto& model)
                       { return exportColumn(model->template table<FreqResponseLoad2Table>().phaseColumn()); });
        registerColumn(
            catalogue, type, "amplitudeTableId", [](const auto& model)
            { return exportColumn(model->template table<FreqResponseLoad2Table>().amplitudeTableIdColumn()); });
        registerColumn(catalogue, type, "amplitude", [](const auto& model)
                       { return exportColumn(model->template table<FreqResponseLoad2Table>().amplitudeColumn()); });
        registerColumn(catalogue, type, "phaseTableId", [](const auto& model)
                       { return exportColumn(model->template table<FreqResponseLoad2Table>().phaseTableIdColumn()); });
        registerColumn(catalogue, type, "phaseAngle", [](const auto& model)
                       { return exportColumn(model->template table<FreqResponseLoad2Table>().phaseAngleColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportColumn(model->template table<FreqResponseLoad2Table>().kindColumn()); });
    }
    {
        using model::RotorHybridDampingTable;
        const std::uint16_t type = model::typeIndexOf<RotorHybridDampingTable>();
        registerColumn(catalogue, type, "pairCount", [](const auto& model)
                       { return exportColumn(model->template table<RotorHybridDampingTable>().pairCountColumn()); });
    }
    {
        using model::HeatShellNodalLoadTable;
        const std::uint16_t type = model::typeIndexOf<HeatShellNodalLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HeatShellNodalLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportVariableColumn(model->template table<HeatShellNodalLoadTable>().nodeColumn()); });
        registerColumn(
            catalogue, type, "components", [](const auto& model)
            { return exportVariableColumn(model->template table<HeatShellNodalLoadTable>().componentsColumn()); });
        registerColumn(
            catalogue, type, "power", [](const auto& model)
            { return exportVariableColumn(model->template table<HeatShellNodalLoadTable>().powerColumn()); });
    }
    {
        using model::TransferFunctionTable;
        const std::uint16_t type = model::typeIndexOf<TransferFunctionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TransferFunctionTable>().idColumn()); });
        registerColumn(catalogue, type, "nodeD", [](const auto& model)
                       { return exportColumn(model->template table<TransferFunctionTable>().nodeDColumn()); });
        registerColumn(catalogue, type, "componentsD", [](const auto& model)
                       { return exportColumn(model->template table<TransferFunctionTable>().componentsDColumn()); });
        registerColumn(catalogue, type, "coefficientB0", [](const auto& model)
                       { return exportColumn(model->template table<TransferFunctionTable>().coefficientB0Column()); });
        registerColumn(catalogue, type, "coefficientB1", [](const auto& model)
                       { return exportColumn(model->template table<TransferFunctionTable>().coefficientB1Column()); });
        registerColumn(catalogue, type, "coefficientB2", [](const auto& model)
                       { return exportColumn(model->template table<TransferFunctionTable>().coefficientB2Column()); });
        registerColumn(
            catalogue, type, "termNodes", [](const auto& model)
            { return exportVariableColumn(model->template table<TransferFunctionTable>().termNodesColumn()); });
        registerColumn(
            catalogue, type, "termComponents", [](const auto& model)
            { return exportVariableColumn(model->template table<TransferFunctionTable>().termComponentsColumn()); });
        registerColumn(
            catalogue, type, "coefficientsA0", [](const auto& model)
            { return exportVariableColumn(model->template table<TransferFunctionTable>().coefficientsA0Column()); });
        registerColumn(
            catalogue, type, "coefficientsA1", [](const auto& model)
            { return exportVariableColumn(model->template table<TransferFunctionTable>().coefficientsA1Column()); });
        registerColumn(
            catalogue, type, "coefficientsA2", [](const auto& model)
            { return exportVariableColumn(model->template table<TransferFunctionTable>().coefficientsA2Column()); });
    }
    {
        using model::TransientInitialConditionTable;
        const std::uint16_t type = model::typeIndexOf<TransientInitialConditionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TransientInitialConditionTable>().idColumn()); });
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportColumn(model->template table<TransientInitialConditionTable>().nodeColumn()); });
        registerColumn(
            catalogue, type, "components", [](const auto& model)
            { return exportColumn(model->template table<TransientInitialConditionTable>().componentsColumn()); });
        registerColumn(catalogue, type, "initialDisplacement",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<TransientInitialConditionTable>().initialDisplacementColumn());
                       });
        registerColumn(
            catalogue, type, "initialVelocity", [](const auto& model)
            { return exportColumn(model->template table<TransientInitialConditionTable>().initialVelocityColumn()); });
    }
    {
        using model::InitialVelocityFieldTable;
        const std::uint16_t type = model::typeIndexOf<InitialVelocityFieldTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<InitialVelocityFieldTable>().idColumn()); });
    }
    {
        using model::InitialElemValuesTable;
        const std::uint16_t type = model::typeIndexOf<InitialElemValuesTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<InitialElemValuesTable>().idColumn()); });
    }
    {
        using model::InitialValuesUserRoutineTable;
        const std::uint16_t type = model::typeIndexOf<InitialValuesUserRoutineTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<InitialValuesUserRoutineTable>().idColumn()); });
    }
    {
        using model::TransientTableLoad1Table;
        const std::uint16_t type = model::typeIndexOf<TransientTableLoad1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TransientTableLoad1Table>().idColumn()); });
        registerColumn(
            catalogue, type, "excitationId", [](const auto& model)
            { return exportColumn(model->template table<TransientTableLoad1Table>().excitationIdColumn()); });
        registerColumn(catalogue, type, "delayId", [](const auto& model)
                       { return exportColumn(model->template table<TransientTableLoad1Table>().delayIdColumn()); });
        registerColumn(catalogue, type, "delay", [](const auto& model)
                       { return exportColumn(model->template table<TransientTableLoad1Table>().delayColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportColumn(model->template table<TransientTableLoad1Table>().kindColumn()); });
        registerColumn(catalogue, type, "tableId", [](const auto& model)
                       { return exportColumn(model->template table<TransientTableLoad1Table>().tableIdColumn()); });
        registerColumn(catalogue, type, "value", [](const auto& model)
                       { return exportColumn(model->template table<TransientTableLoad1Table>().valueColumn()); });
        registerColumn(catalogue, type, "initialDisplacementFactor",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<TransientTableLoad1Table>().initialDisplacementFactorColumn());
                       });
        registerColumn(
            catalogue, type, "initialVelocityFactor", [](const auto& model)
            { return exportColumn(model->template table<TransientTableLoad1Table>().initialVelocityFactorColumn()); });
    }
    {
        using model::TransientFormulaLoad2Table;
        const std::uint16_t type = model::typeIndexOf<TransientFormulaLoad2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TransientFormulaLoad2Table>().idColumn()); });
        registerColumn(
            catalogue, type, "excitationId", [](const auto& model)
            { return exportColumn(model->template table<TransientFormulaLoad2Table>().excitationIdColumn()); });
        registerColumn(catalogue, type, "delayId", [](const auto& model)
                       { return exportColumn(model->template table<TransientFormulaLoad2Table>().delayIdColumn()); });
        registerColumn(catalogue, type, "delay", [](const auto& model)
                       { return exportColumn(model->template table<TransientFormulaLoad2Table>().delayColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportColumn(model->template table<TransientFormulaLoad2Table>().kindColumn()); });
        registerColumn(catalogue, type, "time1", [](const auto& model)
                       { return exportColumn(model->template table<TransientFormulaLoad2Table>().time1Column()); });
        registerColumn(catalogue, type, "time2", [](const auto& model)
                       { return exportColumn(model->template table<TransientFormulaLoad2Table>().time2Column()); });
        registerColumn(catalogue, type, "frequency", [](const auto& model)
                       { return exportColumn(model->template table<TransientFormulaLoad2Table>().frequencyColumn()); });
        registerColumn(catalogue, type, "phase", [](const auto& model)
                       { return exportColumn(model->template table<TransientFormulaLoad2Table>().phaseColumn()); });
        registerColumn(
            catalogue, type, "coefficientC", [](const auto& model)
            { return exportColumn(model->template table<TransientFormulaLoad2Table>().coefficientCColumn()); });
        registerColumn(catalogue, type, "exponentB", [](const auto& model)
                       { return exportColumn(model->template table<TransientFormulaLoad2Table>().exponentBColumn()); });
        registerColumn(catalogue, type, "initialDisplacementFactor",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<TransientFormulaLoad2Table>().initialDisplacementFactorColumn());
                       });
        registerColumn(
            catalogue, type, "initialVelocityFactor",
            [](const auto& model)
            {
                return exportColumn(model->template table<TransientFormulaLoad2Table>().initialVelocityFactorColumn());
            });
    }
    {
        using model::RotorUnbalanceLoadTable;
        const std::uint16_t type = model::typeIndexOf<RotorUnbalanceLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RotorUnbalanceLoadTable>().idColumn()); });
        registerColumn(catalogue, type, "massTableId", [](const auto& model)
                       { return exportColumn(model->template table<RotorUnbalanceLoadTable>().massTableIdColumn()); });
        registerColumn(catalogue, type, "mass", [](const auto& model)
                       { return exportColumn(model->template table<RotorUnbalanceLoadTable>().massColumn()); });
        registerColumn(catalogue, type, "nodeId", [](const auto& model)
                       { return exportColumn(model->template table<RotorUnbalanceLoadTable>().nodeIdColumn()); });
        registerColumn(catalogue, type, "direction", [](const auto& model)
                       { return exportColumn(model->template table<RotorUnbalanceLoadTable>().directionColumn()); });
        registerColumn(
            catalogue, type, "radialOffsetTableId", [](const auto& model)
            { return exportColumn(model->template table<RotorUnbalanceLoadTable>().radialOffsetTableIdColumn()); });
        registerColumn(catalogue, type, "radialOffset", [](const auto& model)
                       { return exportColumn(model->template table<RotorUnbalanceLoadTable>().radialOffsetColumn()); });
        registerColumn(
            catalogue, type, "angularPosition", [](const auto& model)
            { return exportColumn(model->template table<RotorUnbalanceLoadTable>().angularPositionColumn()); });
        registerColumn(
            catalogue, type, "axialOffsetTableId", [](const auto& model)
            { return exportColumn(model->template table<RotorUnbalanceLoadTable>().axialOffsetTableIdColumn()); });
        registerColumn(catalogue, type, "axialOffset", [](const auto& model)
                       { return exportColumn(model->template table<RotorUnbalanceLoadTable>().axialOffsetColumn()); });
        registerColumn(
            catalogue, type, "startTimeOrFrequency", [](const auto& model)
            { return exportColumn(model->template table<RotorUnbalanceLoadTable>().startTimeOrFrequencyColumn()); });
        registerColumn(
            catalogue, type, "stopTimeOrFrequency", [](const auto& model)
            { return exportColumn(model->template table<RotorUnbalanceLoadTable>().stopTimeOrFrequencyColumn()); });
        registerColumn(
            catalogue, type, "correctionFlag", [](const auto& model)
            { return exportColumn(model->template table<RotorUnbalanceLoadTable>().correctionFlagColumn()); });
    }
}

} // namespace oor::query::column_export
