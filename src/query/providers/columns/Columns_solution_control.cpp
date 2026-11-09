// The stored columns of the cards of the family "solution_control", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/solution_control/RotorSupport.hpp"
#include "oor/model/cards/solution_control/arcLengthParams/ArcLengthParamsTable.hpp"
#include "oor/model/cards/solution_control/campbellDiagram/CampbellDiagramTable.hpp"
#include "oor/model/cards/solution_control/cosimulationArea/CosimulationAreaTable.hpp"
#include "oor/model/cards/solution_control/cosimulationInputs/CosimulationInputsTable.hpp"
#include "oor/model/cards/solution_control/cosimulationOutputs/CosimulationOutputsTable.hpp"
#include "oor/model/cards/solution_control/cosimulationSelection/CosimulationSelectionTable.hpp"
#include "oor/model/cards/solution_control/cosimulationService/CosimulationServiceTable.hpp"
#include "oor/model/cards/solution_control/crossSpectrumPair/CrossSpectrumPairTable.hpp"
#include "oor/model/cards/solution_control/cyclicSymmetry/CyclicSymmetryTable.hpp"
#include "oor/model/cards/solution_control/deactivatedElems/DeactivatedElemsTable.hpp"
#include "oor/model/cards/solution_control/fatigueDefinition/FatigueDefinitionTable.hpp"
#include "oor/model/cards/solution_control/fatigueEvent/FatigueEventTable.hpp"
#include "oor/model/cards/solution_control/fatigueLoad/FatigueLoadTable.hpp"
#include "oor/model/cards/solution_control/fatigueParams/FatigueParamsTable.hpp"
#include "oor/model/cards/solution_control/fatigueProp/FatiguePropTable.hpp"
#include "oor/model/cards/solution_control/fatigueSequence/FatigueSequenceTable.hpp"
#include "oor/model/cards/solution_control/fluidStructureControl/FluidStructureControlTable.hpp"
#include "oor/model/cards/solution_control/iterativeSolverOptions/IterativeSolverOptionsCatalogue.hpp"
#include "oor/model/cards/solution_control/iterativeSolverOptions/IterativeSolverOptionsTable.hpp"
#include "oor/model/cards/solution_control/modeTracking/ModeTrackingTable.hpp"
#include "oor/model/cards/solution_control/modelParams/ModelParamsTable.hpp"
#include "oor/model/cards/solution_control/nonlinearStaticParams/NonlinearStaticParamsTable.hpp"
#include "oor/model/cards/solution_control/nonlinearTimeSteps/NonlinearTimeStepsTable.hpp"
#include "oor/model/cards/solution_control/referenceRotor/ReferenceRotorTable.hpp"
#include "oor/model/cards/solution_control/residualVectorDofs/ResidualVectorDofsTable.hpp"
#include "oor/model/cards/solution_control/residualVectorDofs1/ResidualVectorDofs1Table.hpp"
#include "oor/model/cards/solution_control/resultFileModules/ResultFileModulesTable.hpp"
#include "oor/model/cards/solution_control/resultMetadata/ResultMetadataTable.hpp"
#include "oor/model/cards/solution_control/rotorAxisym/RotorAxisymTable.hpp"
#include "oor/model/cards/solution_control/rotorInitialBend/RotorInitialBendTable.hpp"
#include "oor/model/cards/solution_control/rotorLineNodes/RotorLineNodesTable.hpp"
#include "oor/model/cards/solution_control/rotorModel3D/RotorModel3DTable.hpp"
#include "oor/model/cards/solution_control/rotorRelativeSpin/RotorRelativeSpinTable.hpp"
#include "oor/model/cards/solution_control/rotorSuperelement/RotorSuperelementTable.hpp"
#include "oor/model/cards/solution_control/rotorTransientSpin/RotorTransientSpinTable.hpp"
#include "oor/model/cards/solution_control/rotordynamicParams/RotordynamicParamsTable.hpp"
#include "oor/model/cards/solution_control/stochasticParams/StochasticParamsTable.hpp"
#include "oor/model/cards/solution_control/superelementReferenceRotor/SuperelementReferenceRotorTable.hpp"
#include "oor/model/cards/solution_control/timeToSpectrum/TimeToSpectrumTable.hpp"
#include "oor/model/cards/solution_control/tireSelection/TireSelectionTable.hpp"
#include "oor/model/cards/solution_control/transferPathSetup/TransferPathSetupTable.hpp"
#include "oor/model/cards/solution_control/transientTimeSteps/TransientTimeStepsTable.hpp"
#include "oor/model/cards/solution_control/userFileName/UserFileNameTable.hpp"
#include "oor/model/cards/solution_control/wettedElemNodes/WettedElemNodesTable.hpp"
#include "oor/model/cards/solution_control/wettedElemSide/WettedElemSideTable.hpp"
#include "oor/model/cards/solution_control/wettedSurface/WettedSurfaceTable.hpp"
#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

void registerSolutionControlColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::CampbellDiagramTable;
        const std::uint16_t type = model::typeIndexOf<CampbellDiagramTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CampbellDiagramTable>().idColumn()); });
        registerColumn(
            catalogue, type, "discreteValuesId", [](const auto& model)
            { return exportColumn(model->template table<CampbellDiagramTable>().discreteValuesIdColumn()); });
    }
    {
        using model::CosimulationAreaTable;
        const std::uint16_t type = model::typeIndexOf<CosimulationAreaTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CosimulationAreaTable>().idColumn()); });
    }
    {
        using model::CosimulationInputsTable;
        const std::uint16_t type = model::typeIndexOf<CosimulationInputsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CosimulationInputsTable>().idColumn()); });
    }
    {
        using model::CosimulationOutputsTable;
        const std::uint16_t type = model::typeIndexOf<CosimulationOutputsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CosimulationOutputsTable>().idColumn()); });
    }
    {
        using model::CosimulationSelectionTable;
        const std::uint16_t type = model::typeIndexOf<CosimulationSelectionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CosimulationSelectionTable>().idColumn()); });
    }
    {
        using model::CosimulationServiceTable;
        const std::uint16_t type = model::typeIndexOf<CosimulationServiceTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CosimulationServiceTable>().idColumn()); });
    }
    {
        using model::CyclicSymmetryTable;
        const std::uint16_t type = model::typeIndexOf<CyclicSymmetryTable>();
        registerColumn(catalogue, type, "segmentCount", [](const auto& model)
                       { return exportColumn(model->template table<CyclicSymmetryTable>().segmentsColumn()); });
    }
    {
        using model::DeactivatedElemsTable;
        const std::uint16_t type = model::typeIndexOf<DeactivatedElemsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DeactivatedElemsTable>().idColumn()); });
    }
    {
        using model::FluidStructureControlTable;
        const std::uint16_t type = model::typeIndexOf<FluidStructureControlTable>();
        registerColumn(
            catalogue, type, "serviceName", [](const auto& model)
            { return exportTextColumn(model->template table<FluidStructureControlTable>().serviceNameColumn()); });
    }
    {
        using model::FatigueDefinitionTable;
        const std::uint16_t type = model::typeIndexOf<FatigueDefinitionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FatigueDefinitionTable>().idColumn()); });
    }
    {
        using model::FatigueEventTable;
        const std::uint16_t type = model::typeIndexOf<FatigueEventTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FatigueEventTable>().idColumn()); });
    }
    {
        using model::FatigueLoadTable;
        const std::uint16_t type = model::typeIndexOf<FatigueLoadTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FatigueLoadTable>().idColumn()); });
    }
    {
        using model::FatigueParamsTable;
        const std::uint16_t type = model::typeIndexOf<FatigueParamsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FatigueParamsTable>().idColumn()); });
    }
    {
        using model::FatigueSequenceTable;
        const std::uint16_t type = model::typeIndexOf<FatigueSequenceTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FatigueSequenceTable>().idColumn()); });
    }
    {
        using model::ResultFileModulesTable;
        const std::uint16_t type = model::typeIndexOf<ResultFileModulesTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ResultFileModulesTable>().idColumn()); });
    }
    {
        using model::IterativeSolverOptionsTable;
        const std::uint16_t type = model::typeIndexOf<IterativeSolverOptionsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<IterativeSolverOptionsTable>().idColumn()); });
    }
    {
        using model::ModelParamsTable;
        const std::uint16_t type = model::typeIndexOf<ModelParamsTable>();
        registerColumn(catalogue, type, "name", [](const auto& model)
                       { return exportVariableTextColumn(model->template table<ModelParamsTable>().nameColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportVariableColumn(model->template table<ModelParamsTable>().kindColumn()); });
        registerColumn(catalogue, type, "integer", [](const auto& model)
                       { return exportVariableColumn(model->template table<ModelParamsTable>().integerColumn()); });
        registerColumn(catalogue, type, "real", [](const auto& model)
                       { return exportVariableColumn(model->template table<ModelParamsTable>().realColumn()); });
        registerColumn(catalogue, type, "text", [](const auto& model)
                       { return exportVariableTextColumn(model->template table<ModelParamsTable>().textColumn()); });
    }
    {
        using model::ResultMetadataTable;
        const std::uint16_t type = model::typeIndexOf<ResultMetadataTable>();
        registerColumn(catalogue, type, "metadataName", [](const auto& model)
                       { return exportTextColumn(model->template table<ResultMetadataTable>().handleColumn()); });
    }
    {
        using model::ModeTrackingTable;
        const std::uint16_t type = model::typeIndexOf<ModeTrackingTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ModeTrackingTable>().idColumn()); });
        registerColumn(catalogue, type, "lowRange", [](const auto& model)
                       { return exportColumn(model->template table<ModeTrackingTable>().lowRangeColumn()); });
        registerColumn(catalogue, type, "highRange", [](const auto& model)
                       { return exportColumn(model->template table<ModeTrackingTable>().highRangeColumn()); });
        registerColumn(catalogue, type, "trackingFilter", [](const auto& model)
                       { return exportColumn(model->template table<ModeTrackingTable>().trackingFilterColumn()); });
    }
    {
        using model::NonlinearStaticParamsTable;
        const std::uint16_t type = model::typeIndexOf<NonlinearStaticParamsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearStaticParamsTable>().idColumn()); });
        registerColumn(
            catalogue, type, "incrementCount", [](const auto& model)
            { return exportColumn(model->template table<NonlinearStaticParamsTable>().incrementCountColumn()); });
        registerColumn(catalogue, type, "timeStep", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearStaticParamsTable>().timeStepColumn()); });
        registerColumn(
            catalogue, type, "stiffnessMethod", [](const auto& model)
            { return exportColumn(model->template table<NonlinearStaticParamsTable>().stiffnessMethodColumn()); });
        registerColumn(
            catalogue, type, "stiffnessUpdateStep", [](const auto& model)
            { return exportColumn(model->template table<NonlinearStaticParamsTable>().stiffnessUpdateStepColumn()); });
        registerColumn(
            catalogue, type, "maxIterations", [](const auto& model)
            { return exportColumn(model->template table<NonlinearStaticParamsTable>().maxIterationsColumn()); });
        registerColumn(
            catalogue, type, "convergenceFlags", [](const auto& model)
            { return exportTextColumn(model->template table<NonlinearStaticParamsTable>().convergenceFlagsColumn()); });
        registerColumn(
            catalogue, type, "intermediateOutput",
            [](const auto& model)
            {
                return exportTextColumn(model->template table<NonlinearStaticParamsTable>().intermediateOutputColumn());
            });
        registerColumn(catalogue, type, "epsilonU", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearStaticParamsTable>().epsilonUColumn()); });
        registerColumn(catalogue, type, "epsilonP", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearStaticParamsTable>().epsilonPColumn()); });
        registerColumn(catalogue, type, "epsilonW", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearStaticParamsTable>().epsilonWColumn()); });
        registerColumn(
            catalogue, type, "maxDivergences", [](const auto& model)
            { return exportColumn(model->template table<NonlinearStaticParamsTable>().maxDivergencesColumn()); });
        registerColumn(
            catalogue, type, "maxQuasiNewtonVectors",
            [](const auto& model)
            {
                return exportColumn(model->template table<NonlinearStaticParamsTable>().maxQuasiNewtonVectorsColumn());
            });
        registerColumn(
            catalogue, type, "maxLineSearches", [](const auto& model)
            { return exportColumn(model->template table<NonlinearStaticParamsTable>().maxLineSearchesColumn()); });
        registerColumn(catalogue, type, "stressSubincrementFraction",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<NonlinearStaticParamsTable>().stressSubincrementFractionColumn());
                       });
        registerColumn(
            catalogue, type, "lineSearchTolerance", [](const auto& model)
            { return exportColumn(model->template table<NonlinearStaticParamsTable>().lineSearchToleranceColumn()); });
        registerColumn(
            catalogue, type, "maxBisections", [](const auto& model)
            { return exportColumn(model->template table<NonlinearStaticParamsTable>().maxBisectionsColumn()); });
        registerColumn(
            catalogue, type, "maxRotationRatio", [](const auto& model)
            { return exportColumn(model->template table<NonlinearStaticParamsTable>().maxRotationRatioColumn()); });
        registerColumn(
            catalogue, type, "rotationTolerance", [](const auto& model)
            { return exportColumn(model->template table<NonlinearStaticParamsTable>().rotationToleranceColumn()); });
        registerColumn(
            catalogue, type, "minIterations", [](const auto& model)
            { return exportColumn(model->template table<NonlinearStaticParamsTable>().minIterationsColumn()); });
    }
    {
        using model::ArcLengthParamsTable;
        const std::uint16_t type = model::typeIndexOf<ArcLengthParamsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ArcLengthParamsTable>().idColumn()); });
        registerColumn(catalogue, type, "type", [](const auto& model)
                       { return exportColumn(model->template table<ArcLengthParamsTable>().typeColumn()); });
        registerColumn(
            catalogue, type, "minArcLengthRatio", [](const auto& model)
            { return exportColumn(model->template table<ArcLengthParamsTable>().minArcLengthRatioColumn()); });
        registerColumn(
            catalogue, type, "maxArcLengthRatio", [](const auto& model)
            { return exportColumn(model->template table<ArcLengthParamsTable>().maxArcLengthRatioColumn()); });
        registerColumn(catalogue, type, "scaleFactor", [](const auto& model)
                       { return exportColumn(model->template table<ArcLengthParamsTable>().scaleFactorColumn()); });
        registerColumn(catalogue, type, "designIteration", [](const auto& model)
                       { return exportColumn(model->template table<ArcLengthParamsTable>().designIterationColumn()); });
        registerColumn(catalogue, type, "maxIncrements", [](const auto& model)
                       { return exportColumn(model->template table<ArcLengthParamsTable>().maxIncrementsColumn()); });
    }
    {
        using model::FatiguePropTable;
        const std::uint16_t type = model::typeIndexOf<FatiguePropTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FatiguePropTable>().idColumn()); });
    }
    {
        using model::CrossSpectrumPairTable;
        const std::uint16_t type = model::typeIndexOf<CrossSpectrumPairTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CrossSpectrumPairTable>().idColumn()); });
        registerColumn(
            catalogue, type, "responseKind1", [](const auto& model)
            { return exportTextColumn(model->template table<CrossSpectrumPairTable>().responseKind1Column()); });
        registerColumn(
            catalogue, type, "responseKind2", [](const auto& model)
            { return exportTextColumn(model->template table<CrossSpectrumPairTable>().responseKind2Column()); });
        registerColumn(catalogue, type, "targetIds", [](const auto& model)
                       { return exportColumn(model->template table<CrossSpectrumPairTable>().targetIdsColumn()); });
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<CrossSpectrumPairTable>().componentsColumn()); });
        registerColumn(catalogue, type, "curveId", [](const auto& model)
                       { return exportColumn(model->template table<CrossSpectrumPairTable>().curveIdColumn()); });
    }
    {
        using model::ReferenceRotorTable;
        const std::uint16_t type = model::typeIndexOf<ReferenceRotorTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ReferenceRotorTable>().idColumn()); });
        registerColumn(catalogue, type, "referenceRotorId", [](const auto& model)
                       { return exportColumn(model->template table<ReferenceRotorTable>().referenceRotorIdColumn()); });
    }
    {
        using model::SuperelementReferenceRotorTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementReferenceRotorTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SuperelementReferenceRotorTable>().idColumn()); });
        registerColumn(
            catalogue, type, "referenceRotorId",
            [](const auto& model)
            {
                return exportColumn(model->template table<SuperelementReferenceRotorTable>().referenceRotorIdColumn());
            });
        registerColumn(
            catalogue, type, "referenceSuperelementId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<SuperelementReferenceRotorTable>().referenceSuperelementIdColumn());
            });
    }
    {
        using model::RotordynamicParamsTable;
        const std::uint16_t type = model::typeIndexOf<RotordynamicParamsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RotordynamicParamsTable>().idColumn()); });
        registerColumn(catalogue, type, "synchronous", [](const auto& model)
                       { return exportColumn(model->template table<RotordynamicParamsTable>().synchronousColumn()); });
        registerColumn(
            catalogue, type, "referenceRotorId", [](const auto& model)
            { return exportColumn(model->template table<RotordynamicParamsTable>().referenceRotorIdColumn()); });
        registerColumn(catalogue, type, "speedUnit", [](const auto& model)
                       { return exportColumn(model->template table<RotordynamicParamsTable>().speedUnitColumn()); });
        registerColumn(catalogue, type, "speedLow", [](const auto& model)
                       { return exportColumn(model->template table<RotordynamicParamsTable>().speedLowColumn()); });
        registerColumn(catalogue, type, "speedHigh", [](const auto& model)
                       { return exportColumn(model->template table<RotordynamicParamsTable>().speedHighColumn()); });
        registerColumn(catalogue, type, "speed", [](const auto& model)
                       { return exportColumn(model->template table<RotordynamicParamsTable>().speedColumn()); });
        registerColumn(
            catalogue, type, "rotorSuperelementId", [](const auto& model)
            { return exportColumn(model->template table<RotordynamicParamsTable>().rotorSuperelementIdColumn()); });
        registerColumn(
            catalogue, type, "viscousDampingFrequency", [](const auto& model)
            { return exportColumn(model->template table<RotordynamicParamsTable>().viscousDampingFrequencyColumn()); });
        registerColumn(catalogue, type, "structuralDampingFrequency",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<RotordynamicParamsTable>().structuralDampingFrequencyColumn());
                       });
        registerColumn(
            catalogue, type, "hybridDampingFrequency", [](const auto& model)
            { return exportColumn(model->template table<RotordynamicParamsTable>().hybridDampingFrequencyColumn()); });
    }
    {
        using model::RotorInitialBendTable;
        const std::uint16_t type = model::typeIndexOf<RotorInitialBendTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RotorInitialBendTable>().idColumn()); });
        registerColumn(catalogue, type, "rotorId", [](const auto& model)
                       { return exportColumn(model->template table<RotorInitialBendTable>().rotorIdColumn()); });
        registerColumn(
            catalogue, type, "loadId", [](const auto& model)
            { return exportColumn(model->template table<RotorInitialBendTable>().loadCombinationIdColumn()); });
    }
    {
        using model::RotorModel3DTable;
        const std::uint16_t type = model::typeIndexOf<RotorModel3DTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RotorModel3DTable>().idColumn()); });
        registerColumn(catalogue, type, "frameType", [](const auto& model)
                       { return exportColumn(model->template table<RotorModel3DTable>().frameColumn()); });
    }
    {
        using model::RotorAxisymTable;
        const std::uint16_t type = model::typeIndexOf<RotorAxisymTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RotorAxisymTable>().idColumn()); });
        registerColumn(catalogue, type, "listType", [](const auto& model)
                       { return exportColumn(model->template table<RotorAxisymTable>().listTypeColumn()); });
    }
    {
        using model::RotorLineNodesTable;
        const std::uint16_t type = model::typeIndexOf<RotorLineNodesTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RotorLineNodesTable>().idColumn()); });
        registerColumn(catalogue, type, "first", [](const auto& model)
                       { return exportVariableColumn(model->template table<RotorLineNodesTable>().firstColumn()); });
        registerColumn(catalogue, type, "last", [](const auto& model)
                       { return exportVariableColumn(model->template table<RotorLineNodesTable>().lastColumn()); });
        registerColumn(
            catalogue, type, "increment", [](const auto& model)
            { return exportVariableColumn(model->template table<RotorLineNodesTable>().incrementColumn()); });
    }
    {
        using model::RotorSuperelementTable;
        const std::uint16_t type = model::typeIndexOf<RotorSuperelementTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RotorSuperelementTable>().idColumn()); });
        registerColumn(
            catalogue, type, "superelementId", [](const auto& model)
            { return exportColumn(model->template table<RotorSuperelementTable>().superelementIdColumn()); });
        registerColumn(
            catalogue, type, "superelementOption", [](const auto& model)
            { return exportColumn(model->template table<RotorSuperelementTable>().superelementOptionColumn()); });
    }
    {
        using model::RotorRelativeSpinTable;
        const std::uint16_t type = model::typeIndexOf<RotorRelativeSpinTable>();
        registerColumn(catalogue, type, "rotorId", [](const auto& model)
                       { return exportColumn(model->template table<RotorRelativeSpinTable>().rotorIdColumn()); });
        registerColumn(
            catalogue, type, "rotorSuperelementId", [](const auto& model)
            { return exportColumn(model->template table<RotorRelativeSpinTable>().rotorSuperelementIdColumn()); });
    }
    {
        using model::RotorTransientSpinTable;
        const std::uint16_t type = model::typeIndexOf<RotorTransientSpinTable>();
        registerColumn(catalogue, type, "rotorId", [](const auto& model)
                       { return exportColumn(model->template table<RotorTransientSpinTable>().rotorIdColumn()); });
        registerColumn(
            catalogue, type, "rotorSuperelementId", [](const auto& model)
            { return exportColumn(model->template table<RotorTransientSpinTable>().rotorSuperelementIdColumn()); });
    }
    {
        using model::ResidualVectorDofsTable;
        const std::uint16_t type = model::typeIndexOf<ResidualVectorDofsTable>();
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportVariableColumn(model->template table<ResidualVectorDofsTable>().nodeColumn()); });
        registerColumn(
            catalogue, type, "component", [](const auto& model)
            { return exportVariableColumn(model->template table<ResidualVectorDofsTable>().componentColumn()); });
    }
    {
        using model::ResidualVectorDofs1Table;
        const std::uint16_t type = model::typeIndexOf<ResidualVectorDofs1Table>();
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<ResidualVectorDofs1Table>().componentsColumn()); });
        registerColumn(
            catalogue, type, "runs", [](const auto& model)
            { return exportVariableColumn(model->template table<ResidualVectorDofs1Table>().runsColumn()); });
    }
    {
        using model::StochasticParamsTable;
        const std::uint16_t type = model::typeIndexOf<StochasticParamsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<StochasticParamsTable>().idColumn()); });
    }
    {
        using model::TimeToSpectrumTable;
        const std::uint16_t type = model::typeIndexOf<TimeToSpectrumTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TimeToSpectrumTable>().idColumn()); });
    }
    {
        using model::TireSelectionTable;
        const std::uint16_t type = model::typeIndexOf<TireSelectionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TireSelectionTable>().idColumn()); });
    }
    {
        using model::TransferPathSetupTable;
        const std::uint16_t type = model::typeIndexOf<TransferPathSetupTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TransferPathSetupTable>().idColumn()); });
    }
    {
        using model::TransientTimeStepsTable;
        const std::uint16_t type = model::typeIndexOf<TransientTimeStepsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<TransientTimeStepsTable>().idColumn()); });
        registerColumn(
            catalogue, type, "stepCounts", [](const auto& model)
            { return exportVariableColumn(model->template table<TransientTimeStepsTable>().stepCountsColumn()); });
        registerColumn(
            catalogue, type, "timeSteps", [](const auto& model)
            { return exportVariableColumn(model->template table<TransientTimeStepsTable>().timeStepsColumn()); });
        registerColumn(
            catalogue, type, "outputIntervals", [](const auto& model)
            { return exportVariableColumn(model->template table<TransientTimeStepsTable>().outputIntervalsColumn()); });
    }
    {
        using model::NonlinearTimeStepsTable;
        const std::uint16_t type = model::typeIndexOf<NonlinearTimeStepsTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearTimeStepsTable>().idColumn()); });
        registerColumn(catalogue, type, "stepCount", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearTimeStepsTable>().stepCountColumn()); });
        registerColumn(catalogue, type, "timeStep", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearTimeStepsTable>().timeStepColumn()); });
        registerColumn(
            catalogue, type, "outputInterval", [](const auto& model)
            { return exportColumn(model->template table<NonlinearTimeStepsTable>().outputIntervalColumn()); });
        registerColumn(catalogue, type, "method", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearTimeStepsTable>().methodColumn()); });
        registerColumn(
            catalogue, type, "stiffnessUpdateStep", [](const auto& model)
            { return exportColumn(model->template table<NonlinearTimeStepsTable>().stiffnessUpdateStepColumn()); });
        registerColumn(
            catalogue, type, "maxIterations", [](const auto& model)
            { return exportColumn(model->template table<NonlinearTimeStepsTable>().maxIterationsColumn()); });
        registerColumn(
            catalogue, type, "convergenceFlags", [](const auto& model)
            { return exportTextColumn(model->template table<NonlinearTimeStepsTable>().convergenceFlagsColumn()); });
        registerColumn(catalogue, type, "epsilonU", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearTimeStepsTable>().epsilonUColumn()); });
        registerColumn(catalogue, type, "epsilonP", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearTimeStepsTable>().epsilonPColumn()); });
        registerColumn(catalogue, type, "epsilonW", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearTimeStepsTable>().epsilonWColumn()); });
        registerColumn(
            catalogue, type, "maxDivergences", [](const auto& model)
            { return exportColumn(model->template table<NonlinearTimeStepsTable>().maxDivergencesColumn()); });
        registerColumn(
            catalogue, type, "maxQuasiNewtonVectors", [](const auto& model)
            { return exportColumn(model->template table<NonlinearTimeStepsTable>().maxQuasiNewtonVectorsColumn()); });
        registerColumn(
            catalogue, type, "maxLineSearches", [](const auto& model)
            { return exportColumn(model->template table<NonlinearTimeStepsTable>().maxLineSearchesColumn()); });
        registerColumn(catalogue, type, "stressSubincrementFraction",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<NonlinearTimeStepsTable>().stressSubincrementFractionColumn());
                       });
        registerColumn(
            catalogue, type, "maxBisections", [](const auto& model)
            { return exportColumn(model->template table<NonlinearTimeStepsTable>().maxBisectionsColumn()); });
        registerColumn(
            catalogue, type, "adjustInterval", [](const auto& model)
            { return exportColumn(model->template table<NonlinearTimeStepsTable>().adjustIntervalColumn()); });
        registerColumn(
            catalogue, type, "minStepsPerCycle", [](const auto& model)
            { return exportColumn(model->template table<NonlinearTimeStepsTable>().minStepsPerCycleColumn()); });
        registerColumn(catalogue, type, "stepBounds", [](const auto& model)
                       { return exportColumn(model->template table<NonlinearTimeStepsTable>().stepBoundsColumn()); });
        registerColumn(
            catalogue, type, "maxRotationRatio", [](const auto& model)
            { return exportColumn(model->template table<NonlinearTimeStepsTable>().maxRotationRatioColumn()); });
        registerColumn(
            catalogue, type, "displacementTolerance", [](const auto& model)
            { return exportColumn(model->template table<NonlinearTimeStepsTable>().displacementToleranceColumn()); });
        registerColumn(
            catalogue, type, "rotationTolerance", [](const auto& model)
            { return exportColumn(model->template table<NonlinearTimeStepsTable>().rotationToleranceColumn()); });
        registerColumn(
            catalogue, type, "minIterations", [](const auto& model)
            { return exportColumn(model->template table<NonlinearTimeStepsTable>().minIterationsColumn()); });
    }
    {
        using model::UserFileNameTable;
        const std::uint16_t type = model::typeIndexOf<UserFileNameTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserFileNameTable>().idColumn()); });
    }
    {
        using model::WettedElemSideTable;
        const std::uint16_t type = model::typeIndexOf<WettedElemSideTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<WettedElemSideTable>().idColumn()); });
    }
    {
        using model::WettedElemNodesTable;
        const std::uint16_t type = model::typeIndexOf<WettedElemNodesTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<WettedElemNodesTable>().idColumn()); });
    }
    {
        using model::WettedSurfaceTable;
        const std::uint16_t type = model::typeIndexOf<WettedSurfaceTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<WettedSurfaceTable>().idColumn()); });
    }
}

} // namespace oor::query::column_export
