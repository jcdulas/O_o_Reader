// The stored columns of the cards of the family "materials", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/materials/anisoMat2D/AnisoMat2DTable.hpp"
#include "oor/model/cards/materials/anisoMat2DFreq/AnisoMat2DFreqTable.hpp"
#include "oor/model/cards/materials/anisoMat2DTemperature/AnisoMat2DTemperatureTable.hpp"
#include "oor/model/cards/materials/anisoMat3D/AnisoMat3DTable.hpp"
#include "oor/model/cards/materials/anisoMat3DFreq/AnisoMat3DFreqTable.hpp"
#include "oor/model/cards/materials/anisoMat3DTemperature/AnisoMat3DTemperatureTable.hpp"
#include "oor/model/cards/materials/axisymOrthoMat/AxisymOrthoMatTable.hpp"
#include "oor/model/cards/materials/axisymOrthoMatStressDependent/AxisymOrthoMatStressDependentTable.hpp"
#include "oor/model/cards/materials/axisymOrthoMatTemperature/AxisymOrthoMatTemperatureTable.hpp"
#include "oor/model/cards/materials/bulkViscosityMat/BulkViscosityMatTable.hpp"
#include "oor/model/cards/materials/creepMat/CreepMatTable.hpp"
#include "oor/model/cards/materials/elastoPlasticMat/ElastoPlasticMatTable.hpp"
#include "oor/model/cards/materials/elastoPlasticMatTemperature/ElastoPlasticMatTemperatureTable.hpp"
#include "oor/model/cards/materials/eulerMat/EulerMatTable.hpp"
#include "oor/model/cards/materials/fabricMat/FabricMatTable.hpp"
#include "oor/model/cards/materials/failureMat/FailureMatTable.hpp"
#include "oor/model/cards/materials/failureMatTemperature/FailureMatTemperatureTable.hpp"
#include "oor/model/cards/materials/fatigueMat/FatigueMatTable.hpp"
#include "oor/model/cards/materials/fluidMat/FluidMatTable.hpp"
#include "oor/model/cards/materials/fluidMatFreq/FluidMatFreqTable.hpp"
#include "oor/model/cards/materials/gasketMat/GasketMatTable.hpp"
#include "oor/model/cards/materials/gasketMatTemperature/GasketMatTemperatureTable.hpp"
#include "oor/model/cards/materials/heatAnisoMatTemperature/HeatAnisoMatTemperatureTable.hpp"
#include "oor/model/cards/materials/heatIsoMatTemperature/HeatIsoMatTemperatureTable.hpp"
#include "oor/model/cards/materials/hyperelasticMat/HyperelasticMatTable.hpp"
#include "oor/model/cards/materials/hyperelasticMatTemperature/HyperelasticMatTemperatureTable.hpp"
#include "oor/model/cards/materials/hyperelasticPolynomialMat/HyperelasticPolynomialMatTable.hpp"
#include "oor/model/cards/materials/isoMat/IsoMatTable.hpp"
#include "oor/model/cards/materials/isoMatExplicitExt/IsoMatExplicitExtTable.hpp"
#include "oor/model/cards/materials/isoMatFreq/IsoMatFreqTable.hpp"
#include "oor/model/cards/materials/isoMatStressDependent/IsoMatStressDependentTable.hpp"
#include "oor/model/cards/materials/isoMatTemperature/IsoMatTemperatureTable.hpp"
#include "oor/model/cards/materials/micromechanicsCompositeMat/MicromechanicsCompositeMatTable.hpp"
#include "oor/model/cards/materials/orthoMat2D/OrthoMat2DTable.hpp"
#include "oor/model/cards/materials/orthoMat2DFailure/OrthoMat2DFailureTable.hpp"
#include "oor/model/cards/materials/orthoMat2DFreq/OrthoMat2DFreqTable.hpp"
#include "oor/model/cards/materials/orthoMat2DStressDependent/OrthoMat2DStressDependentTable.hpp"
#include "oor/model/cards/materials/orthoMat2DTemperature/OrthoMat2DTemperatureTable.hpp"
#include "oor/model/cards/materials/orthoMat3D/OrthoMat3DTable.hpp"
#include "oor/model/cards/materials/orthoMat3DNonlinear/OrthoMat3DNonlinearTable.hpp"
#include "oor/model/cards/materials/orthoMat3DNonlinearStressDependent/OrthoMat3DNonlinearStressDependentTable.hpp"
#include "oor/model/cards/materials/orthoMat3DNonlinearTemperature/OrthoMat3DNonlinearTemperatureTable.hpp"
#include "oor/model/cards/materials/orthoMat3DTemperature/OrthoMat3DTemperatureTable.hpp"
#include "oor/model/cards/materials/poroElasticMat/PoroElasticMatTable.hpp"
#include "oor/model/cards/materials/poroElasticMatFreq/PoroElasticMatFreqTable.hpp"
#include "oor/model/cards/materials/progressiveFailureMat/ProgressiveFailureMatTable.hpp"
#include "oor/model/cards/materials/rigidMat/RigidMatTable.hpp"
#include "oor/model/cards/materials/shapeMemoryMat/ShapeMemoryMatTable.hpp"
#include "oor/model/cards/materials/userCompositeFailureMat/UserCompositeFailureMatTable.hpp"
#include "oor/model/cards/materials/userMat/UserMatTable.hpp"
#include "oor/model/cards/materials/userMatService/UserMatServiceTable.hpp"
#include "oor/model/cards/materials/userMatTable/UserMatTableTable.hpp"
#include "oor/model/cards/materials/viscoElasticMat/ViscoElasticMatTable.hpp"
#include "oor/model/cards/materials/viscoElasticMatTemperature/ViscoElasticMatTemperatureTable.hpp"
#include "oor/model/cards/materials/viscoPlasticMat/ViscoPlasticMatTable.hpp"
#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

void registerMaterialsColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::UserCompositeFailureMatTable;
        const std::uint16_t type = model::typeIndexOf<UserCompositeFailureMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserCompositeFailureMatTable>().idColumn()); });
    }
    {
        using model::CreepMatTable;
        const std::uint16_t type = model::typeIndexOf<CreepMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CreepMatTable>().idColumn()); });
        registerColumn(catalogue, type, "referenceTemperature", [](const auto& model)
                       { return exportColumn(model->template table<CreepMatTable>().referenceTemperatureColumn()); });
        registerColumn(catalogue, type, "exponent", [](const auto& model)
                       { return exportColumn(model->template table<CreepMatTable>().exponentColumn()); });
        registerColumn(catalogue, type, "form", [](const auto& model)
                       { return exportColumn(model->template table<CreepMatTable>().formColumn()); });
        registerColumn(
            catalogue, type, "primaryStiffnessTableId", [](const auto& model)
            { return exportColumn(model->template table<CreepMatTable>().primaryStiffnessTableIdColumn()); });
        registerColumn(catalogue, type, "primaryDampingTableId", [](const auto& model)
                       { return exportColumn(model->template table<CreepMatTable>().primaryDampingTableIdColumn()); });
        registerColumn(
            catalogue, type, "secondaryDampingTableId", [](const auto& model)
            { return exportColumn(model->template table<CreepMatTable>().secondaryDampingTableIdColumn()); });
        registerColumn(catalogue, type, "threshold", [](const auto& model)
                       { return exportColumn(model->template table<CreepMatTable>().thresholdColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportColumn(model->template table<CreepMatTable>().kindColumn()); });
        registerColumn(catalogue, type, "coefficients", [](const auto& model)
                       { return exportColumn(model->template table<CreepMatTable>().coefficientsColumn()); });
    }
    {
        using model::IsoMatTable;
        const std::uint16_t type = model::typeIndexOf<IsoMatTable>();
        registerColumn(catalogue, type, "sourceBlankFields", [](const auto& model)
        {
            return exportCardValues(model->template table<IsoMatTable>(),
                                    [](const model::IsoMatCard& card) { return card.blankFields; });
        });
        registerColumn(catalogue, type, "id",
                       [](const auto& model) { return exportColumn(model->template table<IsoMatTable>().idColumn()); });
        registerColumn(catalogue, type, "youngsModulus", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatTable>().youngsModulusColumn()); });
        registerColumn(catalogue, type, "shearModulus", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatTable>().shearModulusColumn()); });
        registerColumn(catalogue, type, "poissonRatio", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatTable>().poissonRatioColumn()); });
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatTable>().densityColumn()); });
        registerColumn(catalogue, type, "thermalExpansion", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatTable>().thermalExpansionColumn()); });
        registerColumn(catalogue, type, "referenceTemperature", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatTable>().referenceTemperatureColumn()); });
        registerColumn(catalogue, type, "structuralDamping", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatTable>().structuralDampingColumn()); });
        registerColumn(catalogue, type, "tensionAllowable", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatTable>().tensionAllowableColumn()); });
        registerColumn(catalogue, type, "compressionAllowable", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatTable>().compressionAllowableColumn()); });
        registerColumn(catalogue, type, "shearAllowable", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatTable>().shearAllowableColumn()); });
        registerColumn(catalogue, type, "stressFrameId", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatTable>().stressFrameIdColumn()); });
    }
    {
        using model::FluidMatTable;
        const std::uint16_t type = model::typeIndexOf<FluidMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FluidMatTable>().idColumn()); });
        registerColumn(catalogue, type, "bulkModulus", [](const auto& model)
                       { return exportColumn(model->template table<FluidMatTable>().bulkModulusColumn()); });
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<FluidMatTable>().densityColumn()); });
        registerColumn(catalogue, type, "soundSpeed", [](const auto& model)
                       { return exportColumn(model->template table<FluidMatTable>().soundSpeedColumn()); });
        registerColumn(catalogue, type, "structuralDamping", [](const auto& model)
                       { return exportColumn(model->template table<FluidMatTable>().structuralDampingColumn()); });
        registerColumn(catalogue, type, "normalizedAdmittance", [](const auto& model)
                       { return exportColumn(model->template table<FluidMatTable>().normalizedAdmittanceColumn()); });
    }
    {
        using model::FluidMatFreqTable;
        const std::uint16_t type = model::typeIndexOf<FluidMatFreqTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FluidMatFreqTable>().idColumn()); });
    }
    {
        using model::OrthoMat3DTable;
        const std::uint16_t type = model::typeIndexOf<OrthoMat3DTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat3DTable>().idColumn()); });
        registerColumn(catalogue, type, "youngsModulus1", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat3DTable>().youngsModulus1Column()); });
        registerColumn(catalogue, type, "youngsModulus2", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat3DTable>().youngsModulus2Column()); });
        registerColumn(catalogue, type, "youngsModulus3", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat3DTable>().youngsModulus3Column()); });
        registerColumn(catalogue, type, "poissonRatio12", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat3DTable>().poissonRatio12Column()); });
        registerColumn(catalogue, type, "poissonRatio13", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat3DTable>().poissonRatio13Column()); });
        registerColumn(catalogue, type, "poissonRatio23", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat3DTable>().poissonRatio23Column()); });
        registerColumn(catalogue, type, "shearModulus12", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat3DTable>().shearModulus12Column()); });
        registerColumn(catalogue, type, "shearModulus13", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat3DTable>().shearModulus13Column()); });
        registerColumn(catalogue, type, "shearModulus23", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat3DTable>().shearModulus23Column()); });
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat3DTable>().densityColumn()); });
        registerColumn(catalogue, type, "thermalExpansion1", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat3DTable>().thermalExpansion1Column()); });
        registerColumn(catalogue, type, "thermalExpansion2", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat3DTable>().thermalExpansion2Column()); });
        registerColumn(catalogue, type, "thermalExpansion3", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat3DTable>().thermalExpansion3Column()); });
        registerColumn(catalogue, type, "referenceTemperature", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat3DTable>().referenceTemperatureColumn()); });
        registerColumn(catalogue, type, "structuralDamping", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat3DTable>().structuralDampingColumn()); });
    }
    {
        using model::IsoMatExplicitExtTable;
        const std::uint16_t type = model::typeIndexOf<IsoMatExplicitExtTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatExplicitExtTable>().idColumn()); });
        registerColumn(
            catalogue, type, "equationOfStateId", [](const auto& model)
            { return exportColumn(model->template table<IsoMatExplicitExtTable>().equationOfStateIdColumn()); });
    }
    {
        using model::IsoMatFreqTable;
        const std::uint16_t type = model::typeIndexOf<IsoMatFreqTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatFreqTable>().idColumn()); });
        registerColumn(catalogue, type, "youngsModulusTableId", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatFreqTable>().youngsModulusTableIdColumn()); });
        registerColumn(catalogue, type, "shearModulusTableId", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatFreqTable>().shearModulusTableIdColumn()); });
        registerColumn(catalogue, type, "poissonRatioTableId", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatFreqTable>().poissonRatioTableIdColumn()); });
        registerColumn(
            catalogue, type, "structuralDampingTableId", [](const auto& model)
            { return exportColumn(model->template table<IsoMatFreqTable>().structuralDampingTableIdColumn()); });
    }
    {
        using model::AnisoMat2DTable;
        const std::uint16_t type = model::typeIndexOf<AnisoMat2DTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat2DTable>().idColumn()); });
        registerColumn(catalogue, type, "stiffness", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat2DTable>().stiffnessColumn()); });
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat2DTable>().densityColumn()); });
        registerColumn(catalogue, type, "thermalExpansion", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat2DTable>().thermalExpansionColumn()); });
        registerColumn(catalogue, type, "referenceTemperature", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat2DTable>().referenceTemperatureColumn()); });
        registerColumn(catalogue, type, "structuralDamping", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat2DTable>().structuralDampingColumn()); });
        registerColumn(catalogue, type, "tensionAllowable", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat2DTable>().tensionAllowableColumn()); });
        registerColumn(catalogue, type, "compressionAllowable", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat2DTable>().compressionAllowableColumn()); });
        registerColumn(catalogue, type, "shearAllowable", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat2DTable>().shearAllowableColumn()); });
        registerColumn(catalogue, type, "stressFrameId", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat2DTable>().stressFrameIdColumn()); });
        registerColumn(
            catalogue, type, "structuralDampingTerms", [](const auto& model)
            { return exportColumn(model->template table<AnisoMat2DTable>().structuralDampingTermsColumn()); });
    }
    {
        using model::AnisoMat2DFreqTable;
        const std::uint16_t type = model::typeIndexOf<AnisoMat2DFreqTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat2DFreqTable>().idColumn()); });
    }
    {
        using model::AxisymOrthoMatTable;
        const std::uint16_t type = model::typeIndexOf<AxisymOrthoMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymOrthoMatTable>().idColumn()); });
        registerColumn(catalogue, type, "youngsModulusX", [](const auto& model)
                       { return exportColumn(model->template table<AxisymOrthoMatTable>().youngsModulusXColumn()); });
        registerColumn(
            catalogue, type, "youngsModulusTheta", [](const auto& model)
            { return exportColumn(model->template table<AxisymOrthoMatTable>().youngsModulusThetaColumn()); });
        registerColumn(catalogue, type, "youngsModulusZ", [](const auto& model)
                       { return exportColumn(model->template table<AxisymOrthoMatTable>().youngsModulusZColumn()); });
        registerColumn(catalogue, type, "poissonRatioXth", [](const auto& model)
                       { return exportColumn(model->template table<AxisymOrthoMatTable>().poissonRatioXthColumn()); });
        registerColumn(catalogue, type, "poissonRatioThz", [](const auto& model)
                       { return exportColumn(model->template table<AxisymOrthoMatTable>().poissonRatioThzColumn()); });
        registerColumn(catalogue, type, "poissonRatioZx", [](const auto& model)
                       { return exportColumn(model->template table<AxisymOrthoMatTable>().poissonRatioZxColumn()); });
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<AxisymOrthoMatTable>().densityColumn()); });
        registerColumn(catalogue, type, "shearModulusZx", [](const auto& model)
                       { return exportColumn(model->template table<AxisymOrthoMatTable>().shearModulusZxColumn()); });
        registerColumn(
            catalogue, type, "thermalExpansionX", [](const auto& model)
            { return exportColumn(model->template table<AxisymOrthoMatTable>().thermalExpansionXColumn()); });
        registerColumn(
            catalogue, type, "thermalExpansionTheta", [](const auto& model)
            { return exportColumn(model->template table<AxisymOrthoMatTable>().thermalExpansionThetaColumn()); });
        registerColumn(
            catalogue, type, "thermalExpansionZ", [](const auto& model)
            { return exportColumn(model->template table<AxisymOrthoMatTable>().thermalExpansionZColumn()); });
        registerColumn(
            catalogue, type, "referenceTemperature", [](const auto& model)
            { return exportColumn(model->template table<AxisymOrthoMatTable>().referenceTemperatureColumn()); });
        registerColumn(
            catalogue, type, "structuralDamping", [](const auto& model)
            { return exportColumn(model->template table<AxisymOrthoMatTable>().structuralDampingColumn()); });
    }
    {
        using model::OrthoMat2DTable;
        const std::uint16_t type = model::typeIndexOf<OrthoMat2DTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().idColumn()); });
        registerColumn(catalogue, type, "youngsModulus1", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().youngsModulus1Column()); });
        registerColumn(catalogue, type, "youngsModulus2", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().youngsModulus2Column()); });
        registerColumn(catalogue, type, "poissonRatio12", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().poissonRatio12Column()); });
        registerColumn(catalogue, type, "shearModulus12", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().shearModulus12Column()); });
        registerColumn(catalogue, type, "shearModulus1Z", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().shearModulus1ZColumn()); });
        registerColumn(catalogue, type, "shearModulus2Z", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().shearModulus2ZColumn()); });
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().densityColumn()); });
        registerColumn(catalogue, type, "thermalExpansion1", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().thermalExpansion1Column()); });
        registerColumn(catalogue, type, "thermalExpansion2", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().thermalExpansion2Column()); });
        registerColumn(catalogue, type, "referenceTemperature", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().referenceTemperatureColumn()); });
        registerColumn(catalogue, type, "tensionStrength1", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().tensionStrength1Column()); });
        registerColumn(catalogue, type, "compressionStrength1", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().compressionStrength1Column()); });
        registerColumn(catalogue, type, "tensionStrength2", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().tensionStrength2Column()); });
        registerColumn(catalogue, type, "compressionStrength2", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().compressionStrength2Column()); });
        registerColumn(catalogue, type, "shearStrength", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().shearStrengthColumn()); });
        registerColumn(catalogue, type, "structuralDamping", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().structuralDampingColumn()); });
        registerColumn(catalogue, type, "interactionTerm12", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().interactionTerm12Column()); });
        registerColumn(catalogue, type, "strainAllowables", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().strainAllowablesColumn()); });
        registerColumn(catalogue, type, "hashin", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().hashinColumn()); });
        registerColumn(catalogue, type, "hashinUnidirectional", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().hashinUnidirectionalColumn()); });
        registerColumn(catalogue, type, "hashinTape", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().hashinTapeColumn()); });
        registerColumn(catalogue, type, "hashinFabric", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTable>().hashinFabricColumn()); });
    }
    {
        using model::OrthoMat2DFailureTable;
        const std::uint16_t type = model::typeIndexOf<OrthoMat2DFailureTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DFailureTable>().idColumn()); });
    }
    {
        using model::OrthoMat2DFreqTable;
        const std::uint16_t type = model::typeIndexOf<OrthoMat2DFreqTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DFreqTable>().idColumn()); });
    }
    {
        using model::AnisoMat3DTable;
        const std::uint16_t type = model::typeIndexOf<AnisoMat3DTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat3DTable>().idColumn()); });
        registerColumn(catalogue, type, "stiffness", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat3DTable>().stiffnessColumn()); });
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat3DTable>().densityColumn()); });
        registerColumn(catalogue, type, "thermalExpansion", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat3DTable>().thermalExpansionColumn()); });
        registerColumn(catalogue, type, "referenceTemperature", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat3DTable>().referenceTemperatureColumn()); });
        registerColumn(catalogue, type, "structuralDamping", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat3DTable>().structuralDampingColumn()); });
        registerColumn(
            catalogue, type, "hasStructuralDampingTerms", [](const auto& model)
            { return exportColumn(model->template table<AnisoMat3DTable>().hasStructuralDampingTermsColumn()); });
        registerColumn(
            catalogue, type, "structuralDampingTerms", [](const auto& model)
            { return exportColumn(model->template table<AnisoMat3DTable>().structuralDampingTermsColumn()); });
    }
    {
        using model::AnisoMat3DFreqTable;
        const std::uint16_t type = model::typeIndexOf<AnisoMat3DFreqTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat3DFreqTable>().idColumn()); });
    }
    {
        using model::BulkViscosityMatTable;
        const std::uint16_t type = model::typeIndexOf<BulkViscosityMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<BulkViscosityMatTable>().idColumn()); });
        registerColumn(
            catalogue, type, "bulkViscosityKind", [](const auto& model)
            { return exportColumn(model->template table<BulkViscosityMatTable>().bulkViscosityKindColumn()); });
        registerColumn(
            catalogue, type, "quadraticBulkViscosity", [](const auto& model)
            { return exportColumn(model->template table<BulkViscosityMatTable>().quadraticBulkViscosityColumn()); });
        registerColumn(
            catalogue, type, "linearBulkViscosity", [](const auto& model)
            { return exportColumn(model->template table<BulkViscosityMatTable>().linearBulkViscosityColumn()); });
        registerColumn(
            catalogue, type, "compressionScale", [](const auto& model)
            { return exportColumn(model->template table<BulkViscosityMatTable>().compressionScaleColumn()); });
        registerColumn(
            catalogue, type, "membraneDamping", [](const auto& model)
            { return exportColumn(model->template table<BulkViscosityMatTable>().membraneDampingColumn()); });
        registerColumn(
            catalogue, type, "initialMetricRelaxation", [](const auto& model)
            { return exportColumn(model->template table<BulkViscosityMatTable>().initialMetricRelaxationColumn()); });
    }
    {
        using model::ProgressiveFailureMatTable;
        const std::uint16_t type = model::typeIndexOf<ProgressiveFailureMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ProgressiveFailureMatTable>().idColumn()); });
    }
    {
        using model::EulerMatTable;
        const std::uint16_t type = model::typeIndexOf<EulerMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<EulerMatTable>().idColumn()); });
    }
    {
        using model::MicromechanicsCompositeMatTable;
        const std::uint16_t type = model::typeIndexOf<MicromechanicsCompositeMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MicromechanicsCompositeMatTable>().idColumn()); });
        registerColumn(
            catalogue, type, "userDefinedNameId",
            [](const auto& model)
            {
                return exportColumn(model->template table<MicromechanicsCompositeMatTable>().userDefinedNameIdColumn());
            });
        registerColumn(
            catalogue, type, "density", [](const auto& model)
            { return exportColumn(model->template table<MicromechanicsCompositeMatTable>().densityColumn()); });
    }
    {
        using model::ElastoPlasticMatTable;
        const std::uint16_t type = model::typeIndexOf<ElastoPlasticMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ElastoPlasticMatTable>().idColumn()); });
        registerColumn(catalogue, type, "form", [](const auto& model)
                       { return exportColumn(model->template table<ElastoPlasticMatTable>().formColumn()); });
        registerColumn(catalogue, type, "yieldStress", [](const auto& model)
                       { return exportColumn(model->template table<ElastoPlasticMatTable>().yieldStressColumn()); });
        registerColumn(
            catalogue, type, "stressStrainTableId", [](const auto& model)
            { return exportColumn(model->template table<ElastoPlasticMatTable>().stressStrainTableIdColumn()); });
        registerColumn(catalogue, type, "yieldCriterion", [](const auto& model)
                       { return exportColumn(model->template table<ElastoPlasticMatTable>().yieldCriterionColumn()); });
        registerColumn(catalogue, type, "workHardening", [](const auto& model)
                       { return exportColumn(model->template table<ElastoPlasticMatTable>().workHardeningColumn()); });
        registerColumn(catalogue, type, "hardeningSlope", [](const auto& model)
                       { return exportColumn(model->template table<ElastoPlasticMatTable>().hardeningSlopeColumn()); });
        registerColumn(catalogue, type, "groups", [](const auto& model)
                       { return exportColumn(model->template table<ElastoPlasticMatTable>().groupsColumn()); });
        registerColumn(catalogue, type, "rateOption", [](const auto& model)
                       { return exportColumn(model->template table<ElastoPlasticMatTable>().rateOptionColumn()); });
        registerColumn(
            catalogue, type, "strainRateTableId", [](const auto& model)
            { return exportColumn(model->template table<ElastoPlasticMatTable>().strainRateTableIdColumn()); });
        registerColumn(
            catalogue, type, "cowperCoefficient", [](const auto& model)
            { return exportColumn(model->template table<ElastoPlasticMatTable>().cowperCoefficientColumn()); });
        registerColumn(catalogue, type, "cowperExponent", [](const auto& model)
                       { return exportColumn(model->template table<ElastoPlasticMatTable>().cowperExponentColumn()); });
        registerColumn(catalogue, type, "anisotropy", [](const auto& model)
                       { return exportColumn(model->template table<ElastoPlasticMatTable>().anisotropyColumn()); });
        registerColumn(catalogue, type, "pressureOption", [](const auto& model)
                       { return exportColumn(model->template table<ElastoPlasticMatTable>().pressureOptionColumn()); });
        registerColumn(
            catalogue, type, "mohrCoulombAlpha", [](const auto& model)
            { return exportColumn(model->template table<ElastoPlasticMatTable>().mohrCoulombAlphaColumn()); });
        registerColumn(
            catalogue, type, "mohrCoulombBeta", [](const auto& model)
            { return exportColumn(model->template table<ElastoPlasticMatTable>().mohrCoulombBetaColumn()); });
        registerColumn(
            catalogue, type, "chabocheHardening", [](const auto& model)
            { return exportColumn(model->template table<ElastoPlasticMatTable>().chabocheHardeningColumn()); });
        registerColumn(
            catalogue, type, "johnsonCookHardening", [](const auto& model)
            { return exportColumn(model->template table<ElastoPlasticMatTable>().johnsonCookHardeningColumn()); });
        registerColumn(
            catalogue, type, "creepEquivalentStress", [](const auto& model)
            { return exportColumn(model->template table<ElastoPlasticMatTable>().creepEquivalentStressColumn()); });
    }
    {
        using model::FailureMatTable;
        const std::uint16_t type = model::typeIndexOf<FailureMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FailureMatTable>().idColumn()); });
    }
    {
        using model::PoroElasticMatFreqTable;
        const std::uint16_t type = model::typeIndexOf<PoroElasticMatFreqTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PoroElasticMatFreqTable>().idColumn()); });
    }
    {
        using model::FabricMatTable;
        const std::uint16_t type = model::typeIndexOf<FabricMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FabricMatTable>().idColumn()); });
    }
    {
        using model::FatigueMatTable;
        const std::uint16_t type = model::typeIndexOf<FatigueMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FatigueMatTable>().idColumn()); });
    }
    {
        using model::GasketMatTable;
        const std::uint16_t type = model::typeIndexOf<GasketMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<GasketMatTable>().idColumn()); });
        registerColumn(catalogue, type, "membraneMatId", [](const auto& model)
                       { return exportColumn(model->template table<GasketMatTable>().membraneMatIdColumn()); });
        registerColumn(catalogue, type, "behaviorKind", [](const auto& model)
                       { return exportColumn(model->template table<GasketMatTable>().behaviorKindColumn()); });
        registerColumn(catalogue, type, "loadingTableId", [](const auto& model)
                       { return exportColumn(model->template table<GasketMatTable>().loadingTableIdColumn()); });
        registerColumn(catalogue, type, "unloadingTableIds", [](const auto& model)
                       { return exportColumn(model->template table<GasketMatTable>().unloadingTableIdsColumn()); });
        registerColumn(catalogue, type, "yieldPressure", [](const auto& model)
                       { return exportColumn(model->template table<GasketMatTable>().yieldPressureColumn()); });
        registerColumn(catalogue, type, "tensileModulus", [](const auto& model)
                       { return exportColumn(model->template table<GasketMatTable>().tensileModulusColumn()); });
        registerColumn(
            catalogue, type, "transverseShearModulus", [](const auto& model)
            { return exportColumn(model->template table<GasketMatTable>().transverseShearModulusColumn()); });
        registerColumn(catalogue, type, "initialGap", [](const auto& model)
                       { return exportColumn(model->template table<GasketMatTable>().initialGapColumn()); });
    }
    {
        using model::HyperelasticMatTable;
        const std::uint16_t type = model::typeIndexOf<HyperelasticMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HyperelasticMatTable>().idColumn()); });
        registerColumn(catalogue, type, "model", [](const auto& model)
                       { return exportColumn(model->template table<HyperelasticMatTable>().modelColumn()); });
        registerColumn(catalogue, type, "bulkModulus", [](const auto& model)
                       { return exportColumn(model->template table<HyperelasticMatTable>().bulkModulusColumn()); });
        registerColumn(catalogue, type, "density", [](const auto& model)
                       { return exportColumn(model->template table<HyperelasticMatTable>().densityColumn()); });
        registerColumn(
            catalogue, type, "thermalExpansion", [](const auto& model)
            { return exportColumn(model->template table<HyperelasticMatTable>().thermalExpansionColumn()); });
        registerColumn(
            catalogue, type, "referenceTemperature", [](const auto& model)
            { return exportColumn(model->template table<HyperelasticMatTable>().referenceTemperatureColumn()); });
        registerColumn(
            catalogue, type, "structuralDamping", [](const auto& model)
            { return exportColumn(model->template table<HyperelasticMatTable>().structuralDampingColumn()); });
        registerColumn(catalogue, type, "coefficientsC", [](const auto& model)
                       { return exportColumn(model->template table<HyperelasticMatTable>().coefficientsCColumn()); });
        registerColumn(catalogue, type, "tensionTableId", [](const auto& model)
                       { return exportColumn(model->template table<HyperelasticMatTable>().tensionTableIdColumn()); });
        registerColumn(
            catalogue, type, "equibiaxialTableId", [](const auto& model)
            { return exportColumn(model->template table<HyperelasticMatTable>().equibiaxialTableIdColumn()); });
        registerColumn(
            catalogue, type, "simpleShearTableId", [](const auto& model)
            { return exportColumn(model->template table<HyperelasticMatTable>().simpleShearTableIdColumn()); });
        registerColumn(
            catalogue, type, "pureShearTableId", [](const auto& model)
            { return exportColumn(model->template table<HyperelasticMatTable>().pureShearTableIdColumn()); });
        registerColumn(
            catalogue, type, "volumetricTableId", [](const auto& model)
            { return exportColumn(model->template table<HyperelasticMatTable>().volumetricTableIdColumn()); });
        registerColumn(
            catalogue, type, "distortionalOrder", [](const auto& model)
            { return exportColumn(model->template table<HyperelasticMatTable>().distortionalOrderColumn()); });
        registerColumn(catalogue, type, "volumetricOrder", [](const auto& model)
                       { return exportColumn(model->template table<HyperelasticMatTable>().volumetricOrderColumn()); });
        registerColumn(catalogue, type, "termCount", [](const auto& model)
                       { return exportColumn(model->template table<HyperelasticMatTable>().termCountColumn()); });
        registerColumn(catalogue, type, "ogdenModuli", [](const auto& model)
                       { return exportColumn(model->template table<HyperelasticMatTable>().ogdenModuliColumn()); });
        registerColumn(catalogue, type, "alpha", [](const auto& model)
                       { return exportColumn(model->template table<HyperelasticMatTable>().alphaColumn()); });
        registerColumn(catalogue, type, "beta", [](const auto& model)
                       { return exportColumn(model->template table<HyperelasticMatTable>().betaColumn()); });
        registerColumn(catalogue, type, "rubberModulus", [](const auto& model)
                       { return exportColumn(model->template table<HyperelasticMatTable>().rubberModulusColumn()); });
        registerColumn(
            catalogue, type, "exponentOrModulus", [](const auto& model)
            { return exportColumn(model->template table<HyperelasticMatTable>().exponentOrModulusColumn()); });
        registerColumn(catalogue, type, "invariantLimit", [](const auto& model)
                       { return exportColumn(model->template table<HyperelasticMatTable>().invariantLimitColumn()); });
        registerColumn(catalogue, type, "coefficientsD", [](const auto& model)
                       { return exportColumn(model->template table<HyperelasticMatTable>().coefficientsDColumn()); });
    }
    {
        using model::HyperelasticPolynomialMatTable;
        const std::uint16_t type = model::typeIndexOf<HyperelasticPolynomialMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HyperelasticPolynomialMatTable>().idColumn()); });
        registerColumn(
            catalogue, type, "coefficientA10", [](const auto& model)
            { return exportColumn(model->template table<HyperelasticPolynomialMatTable>().coefficientA10Column()); });
        registerColumn(
            catalogue, type, "coefficientA01", [](const auto& model)
            { return exportColumn(model->template table<HyperelasticPolynomialMatTable>().coefficientA01Column()); });
        registerColumn(
            catalogue, type, "coefficientD1", [](const auto& model)
            { return exportColumn(model->template table<HyperelasticPolynomialMatTable>().coefficientD1Column()); });
        registerColumn(
            catalogue, type, "density", [](const auto& model)
            { return exportColumn(model->template table<HyperelasticPolynomialMatTable>().densityColumn()); });
        registerColumn(
            catalogue, type, "thermalExpansion", [](const auto& model)
            { return exportColumn(model->template table<HyperelasticPolynomialMatTable>().thermalExpansionColumn()); });
        registerColumn(catalogue, type, "referenceTemperature",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<HyperelasticPolynomialMatTable>().referenceTemperatureColumn());
                       });
        registerColumn(
            catalogue, type, "structuralDamping",
            [](const auto& model)
            {
                return exportColumn(model->template table<HyperelasticPolynomialMatTable>().structuralDampingColumn());
            });
        registerColumn(
            catalogue, type, "distortionalOrder",
            [](const auto& model)
            {
                return exportColumn(model->template table<HyperelasticPolynomialMatTable>().distortionalOrderColumn());
            });
        registerColumn(
            catalogue, type, "volumetricOrder", [](const auto& model)
            { return exportColumn(model->template table<HyperelasticPolynomialMatTable>().volumetricOrderColumn()); });
        registerColumn(
            catalogue, type, "coefficientsA", [](const auto& model)
            { return exportColumn(model->template table<HyperelasticPolynomialMatTable>().coefficientsAColumn()); });
        registerColumn(
            catalogue, type, "coefficientsD", [](const auto& model)
            { return exportColumn(model->template table<HyperelasticPolynomialMatTable>().coefficientsDColumn()); });
        registerColumn(
            catalogue, type, "tensionTableId", [](const auto& model)
            { return exportColumn(model->template table<HyperelasticPolynomialMatTable>().tensionTableIdColumn()); });
        registerColumn(
            catalogue, type, "equibiaxialTableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<HyperelasticPolynomialMatTable>().equibiaxialTableIdColumn());
            });
        registerColumn(
            catalogue, type, "simpleShearTableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<HyperelasticPolynomialMatTable>().simpleShearTableIdColumn());
            });
        registerColumn(
            catalogue, type, "pureShearTableId", [](const auto& model)
            { return exportColumn(model->template table<HyperelasticPolynomialMatTable>().pureShearTableIdColumn()); });
        registerColumn(
            catalogue, type, "volumetricTableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<HyperelasticPolynomialMatTable>().volumetricTableIdColumn());
            });
    }
    {
        using model::OrthoMat3DNonlinearTable;
        const std::uint16_t type = model::typeIndexOf<OrthoMat3DNonlinearTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat3DNonlinearTable>().idColumn()); });
    }
    {
        using model::PoroElasticMatTable;
        const std::uint16_t type = model::typeIndexOf<PoroElasticMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<PoroElasticMatTable>().idColumn()); });
        registerColumn(catalogue, type, "solidMatId", [](const auto& model)
                       { return exportColumn(model->template table<PoroElasticMatTable>().solidMatIdColumn()); });
        registerColumn(catalogue, type, "fluidMatId", [](const auto& model)
                       { return exportColumn(model->template table<PoroElasticMatTable>().fluidMatIdColumn()); });
        registerColumn(catalogue, type, "biotFactor", [](const auto& model)
                       { return exportColumn(model->template table<PoroElasticMatTable>().biotFactorColumn()); });
        registerColumn(
            catalogue, type, "simplifiedModel", [](const auto& model)
            { return exportTextColumn(model->template table<PoroElasticMatTable>().simplifiedModelColumn()); });
        registerColumn(catalogue, type, "solidDensity", [](const auto& model)
                       { return exportColumn(model->template table<PoroElasticMatTable>().solidDensityColumn()); });
        registerColumn(catalogue, type, "viscosity", [](const auto& model)
                       { return exportColumn(model->template table<PoroElasticMatTable>().viscosityColumn()); });
        registerColumn(
            catalogue, type, "specificHeatRatio", [](const auto& model)
            { return exportColumn(model->template table<PoroElasticMatTable>().specificHeatRatioColumn()); });
        registerColumn(catalogue, type, "prandtlNumber", [](const auto& model)
                       { return exportColumn(model->template table<PoroElasticMatTable>().prandtlNumberColumn()); });
        registerColumn(catalogue, type, "porosity", [](const auto& model)
                       { return exportColumn(model->template table<PoroElasticMatTable>().porosityColumn()); });
        registerColumn(catalogue, type, "tortuosity", [](const auto& model)
                       { return exportColumn(model->template table<PoroElasticMatTable>().tortuosityColumn()); });
        registerColumn(catalogue, type, "flowResistivity", [](const auto& model)
                       { return exportColumn(model->template table<PoroElasticMatTable>().flowResistivityColumn()); });
        registerColumn(catalogue, type, "viscousLength", [](const auto& model)
                       { return exportColumn(model->template table<PoroElasticMatTable>().viscousLengthColumn()); });
        registerColumn(catalogue, type, "thermalLength", [](const auto& model)
                       { return exportColumn(model->template table<PoroElasticMatTable>().thermalLengthColumn()); });
    }
    {
        using model::RigidMatTable;
        const std::uint16_t type = model::typeIndexOf<RigidMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RigidMatTable>().idColumn()); });
    }
    {
        using model::IsoMatStressDependentTable;
        const std::uint16_t type = model::typeIndexOf<IsoMatStressDependentTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatStressDependentTable>().idColumn()); });
        registerColumn(catalogue, type, "tableId", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatStressDependentTable>().tableIdColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatStressDependentTable>().typeColumn()); });
        registerColumn(catalogue, type, "hardeningSlope", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatStressDependentTable>().hColumn()); });
        registerColumn(
            catalogue, type, "yieldFunction", [](const auto& model)
            { return exportColumn(model->template table<IsoMatStressDependentTable>().yieldFunctionColumn()); });
        registerColumn(
            catalogue, type, "hardeningRule", [](const auto& model)
            { return exportColumn(model->template table<IsoMatStressDependentTable>().hardeningRuleColumn()); });
        registerColumn(
            catalogue, type, "initialYieldStress", [](const auto& model)
            { return exportColumn(model->template table<IsoMatStressDependentTable>().initialYieldStressColumn()); });
        registerColumn(
            catalogue, type, "frictionAngle", [](const auto& model)
            { return exportColumn(model->template table<IsoMatStressDependentTable>().frictionAngleColumn()); });
    }
    {
        using model::AxisymOrthoMatStressDependentTable;
        const std::uint16_t type = model::typeIndexOf<AxisymOrthoMatStressDependentTable>();
        registerColumn(
            catalogue, type, "id", [](const auto& model)
            { return exportColumn(model->template table<AxisymOrthoMatStressDependentTable>().idColumn()); });
        registerColumn(
            catalogue, type, "youngsModulusXTableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<AxisymOrthoMatStressDependentTable>().youngsModulusXTableIdColumn());
            });
        registerColumn(
            catalogue, type, "youngsModulusThetaTableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<AxisymOrthoMatStressDependentTable>().youngsModulusThetaTableIdColumn());
            });
        registerColumn(
            catalogue, type, "youngsModulusZTableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<AxisymOrthoMatStressDependentTable>().youngsModulusZTableIdColumn());
            });
        registerColumn(
            catalogue, type, "poissonRatioXthTableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<AxisymOrthoMatStressDependentTable>().poissonRatioXthTableIdColumn());
            });
        registerColumn(
            catalogue, type, "poissonRatioThzTableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<AxisymOrthoMatStressDependentTable>().poissonRatioThzTableIdColumn());
            });
        registerColumn(
            catalogue, type, "poissonRatioZxTableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<AxisymOrthoMatStressDependentTable>().poissonRatioZxTableIdColumn());
            });
        registerColumn(
            catalogue, type, "densityTableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<AxisymOrthoMatStressDependentTable>().densityTableIdColumn());
            });
        registerColumn(
            catalogue, type, "shearModulusZxTableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<AxisymOrthoMatStressDependentTable>().shearModulusZxTableIdColumn());
            });
        registerColumn(
            catalogue, type, "thermalExpansionXTableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<AxisymOrthoMatStressDependentTable>().thermalExpansionXTableIdColumn());
            });
        registerColumn(
            catalogue, type, "thermalExpansionThetaTableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<AxisymOrthoMatStressDependentTable>().thermalExpansionThetaTableIdColumn());
            });
        registerColumn(
            catalogue, type, "thermalExpansionZTableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<AxisymOrthoMatStressDependentTable>().thermalExpansionZTableIdColumn());
            });
    }
    {
        using model::OrthoMat2DStressDependentTable;
        const std::uint16_t type = model::typeIndexOf<OrthoMat2DStressDependentTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DStressDependentTable>().idColumn()); });
        registerColumn(catalogue, type, "youngsModulus1TableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<OrthoMat2DStressDependentTable>().youngsModulus1TableIdColumn());
                       });
        registerColumn(catalogue, type, "youngsModulus2TableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<OrthoMat2DStressDependentTable>().youngsModulus2TableIdColumn());
                       });
        registerColumn(catalogue, type, "poissonRatio12TableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<OrthoMat2DStressDependentTable>().poissonRatio12TableIdColumn());
                       });
        registerColumn(catalogue, type, "shearModulus12TableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<OrthoMat2DStressDependentTable>().shearModulus12TableIdColumn());
                       });
        registerColumn(catalogue, type, "shearModulus1ZTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<OrthoMat2DStressDependentTable>().shearModulus1ZTableIdColumn());
                       });
        registerColumn(catalogue, type, "shearModulus2ZTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<OrthoMat2DStressDependentTable>().shearModulus2ZTableIdColumn());
                       });
        registerColumn(
            catalogue, type, "densityTableId", [](const auto& model)
            { return exportColumn(model->template table<OrthoMat2DStressDependentTable>().densityTableIdColumn()); });
        registerColumn(
            catalogue, type, "thermalExpansion1TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat2DStressDependentTable>().thermalExpansion1TableIdColumn());
            });
        registerColumn(
            catalogue, type, "thermalExpansion2TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat2DStressDependentTable>().thermalExpansion2TableIdColumn());
            });
    }
    {
        using model::ShapeMemoryMatTable;
        const std::uint16_t type = model::typeIndexOf<ShapeMemoryMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ShapeMemoryMatTable>().idColumn()); });
    }
    {
        using model::OrthoMat3DNonlinearStressDependentTable;
        const std::uint16_t type = model::typeIndexOf<OrthoMat3DNonlinearStressDependentTable>();
        registerColumn(
            catalogue, type, "id", [](const auto& model)
            { return exportColumn(model->template table<OrthoMat3DNonlinearStressDependentTable>().idColumn()); });
        registerColumn(
            catalogue, type, "youngsModulus1TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearStressDependentTable>().youngsModulus1TableIdColumn());
            });
        registerColumn(
            catalogue, type, "youngsModulus2TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearStressDependentTable>().youngsModulus2TableIdColumn());
            });
        registerColumn(
            catalogue, type, "youngsModulus3TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearStressDependentTable>().youngsModulus3TableIdColumn());
            });
        registerColumn(
            catalogue, type, "poissonRatio12TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearStressDependentTable>().poissonRatio12TableIdColumn());
            });
        registerColumn(
            catalogue, type, "poissonRatio23TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearStressDependentTable>().poissonRatio23TableIdColumn());
            });
        registerColumn(
            catalogue, type, "poissonRatio31TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearStressDependentTable>().poissonRatio31TableIdColumn());
            });
        registerColumn(catalogue, type, "densityTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<OrthoMat3DNonlinearStressDependentTable>().densityTableIdColumn());
                       });
        registerColumn(
            catalogue, type, "shearModulus12TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearStressDependentTable>().shearModulus12TableIdColumn());
            });
        registerColumn(
            catalogue, type, "shearModulus23TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearStressDependentTable>().shearModulus23TableIdColumn());
            });
        registerColumn(
            catalogue, type, "shearModulus31TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearStressDependentTable>().shearModulus31TableIdColumn());
            });
        registerColumn(
            catalogue, type, "thermalExpansion1TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearStressDependentTable>().thermalExpansion1TableIdColumn());
            });
        registerColumn(
            catalogue, type, "thermalExpansion2TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearStressDependentTable>().thermalExpansion2TableIdColumn());
            });
        registerColumn(
            catalogue, type, "thermalExpansion3TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearStressDependentTable>().thermalExpansion3TableIdColumn());
            });
    }
    {
        using model::IsoMatTemperatureTable;
        const std::uint16_t type = model::typeIndexOf<IsoMatTemperatureTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<IsoMatTemperatureTable>().idColumn()); });
        registerColumn(
            catalogue, type, "youngsModulusTableId", [](const auto& model)
            { return exportColumn(model->template table<IsoMatTemperatureTable>().youngsModulusTableIdColumn()); });
        registerColumn(
            catalogue, type, "shearModulusTableId", [](const auto& model)
            { return exportColumn(model->template table<IsoMatTemperatureTable>().shearModulusTableIdColumn()); });
        registerColumn(
            catalogue, type, "poissonRatioTableId", [](const auto& model)
            { return exportColumn(model->template table<IsoMatTemperatureTable>().poissonRatioTableIdColumn()); });
        registerColumn(
            catalogue, type, "densityTableId", [](const auto& model)
            { return exportColumn(model->template table<IsoMatTemperatureTable>().densityTableIdColumn()); });
        registerColumn(
            catalogue, type, "thermalExpansionTableId", [](const auto& model)
            { return exportColumn(model->template table<IsoMatTemperatureTable>().thermalExpansionTableIdColumn()); });
        registerColumn(
            catalogue, type, "structuralDampingTableId", [](const auto& model)
            { return exportColumn(model->template table<IsoMatTemperatureTable>().structuralDampingTableIdColumn()); });
        registerColumn(
            catalogue, type, "tensionAllowableTableId", [](const auto& model)
            { return exportColumn(model->template table<IsoMatTemperatureTable>().tensionAllowableTableIdColumn()); });
        registerColumn(catalogue, type, "compressionAllowableTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<IsoMatTemperatureTable>().compressionAllowableTableIdColumn());
                       });
        registerColumn(
            catalogue, type, "shearAllowableTableId", [](const auto& model)
            { return exportColumn(model->template table<IsoMatTemperatureTable>().shearAllowableTableIdColumn()); });
    }
    {
        using model::OrthoMat3DTemperatureTable;
        const std::uint16_t type = model::typeIndexOf<OrthoMat3DTemperatureTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat3DTemperatureTable>().idColumn()); });
        registerColumn(
            catalogue, type, "youngsModulus1TableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<OrthoMat3DTemperatureTable>().youngsModulus1TableIdColumn());
            });
        registerColumn(
            catalogue, type, "youngsModulus2TableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<OrthoMat3DTemperatureTable>().youngsModulus2TableIdColumn());
            });
        registerColumn(
            catalogue, type, "youngsModulus3TableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<OrthoMat3DTemperatureTable>().youngsModulus3TableIdColumn());
            });
        registerColumn(
            catalogue, type, "poissonRatio12TableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<OrthoMat3DTemperatureTable>().poissonRatio12TableIdColumn());
            });
        registerColumn(
            catalogue, type, "poissonRatio13TableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<OrthoMat3DTemperatureTable>().poissonRatio13TableIdColumn());
            });
        registerColumn(
            catalogue, type, "poissonRatio23TableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<OrthoMat3DTemperatureTable>().poissonRatio23TableIdColumn());
            });
        registerColumn(
            catalogue, type, "shearModulus12TableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<OrthoMat3DTemperatureTable>().shearModulus12TableIdColumn());
            });
        registerColumn(
            catalogue, type, "shearModulus13TableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<OrthoMat3DTemperatureTable>().shearModulus13TableIdColumn());
            });
        registerColumn(
            catalogue, type, "shearModulus23TableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<OrthoMat3DTemperatureTable>().shearModulus23TableIdColumn());
            });
        registerColumn(
            catalogue, type, "densityTableId", [](const auto& model)
            { return exportColumn(model->template table<OrthoMat3DTemperatureTable>().densityTableIdColumn()); });
        registerColumn(catalogue, type, "thermalExpansion1TableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<OrthoMat3DTemperatureTable>().thermalExpansion1TableIdColumn());
                       });
        registerColumn(catalogue, type, "thermalExpansion2TableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<OrthoMat3DTemperatureTable>().thermalExpansion2TableIdColumn());
                       });
        registerColumn(catalogue, type, "thermalExpansion3TableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<OrthoMat3DTemperatureTable>().thermalExpansion3TableIdColumn());
                       });
        registerColumn(catalogue, type, "structuralDampingTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<OrthoMat3DTemperatureTable>().structuralDampingTableIdColumn());
                       });
    }
    {
        using model::AnisoMat2DTemperatureTable;
        const std::uint16_t type = model::typeIndexOf<AnisoMat2DTemperatureTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat2DTemperatureTable>().idColumn()); });
        registerColumn(
            catalogue, type, "stiffnessTableIds", [](const auto& model)
            { return exportColumn(model->template table<AnisoMat2DTemperatureTable>().stiffnessTableIdsColumn()); });
        registerColumn(
            catalogue, type, "densityTableId", [](const auto& model)
            { return exportColumn(model->template table<AnisoMat2DTemperatureTable>().densityTableIdColumn()); });
        registerColumn(catalogue, type, "thermalExpansionTableIds",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<AnisoMat2DTemperatureTable>().thermalExpansionTableIdsColumn());
                       });
        registerColumn(catalogue, type, "structuralDampingTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<AnisoMat2DTemperatureTable>().structuralDampingTableIdColumn());
                       });
        registerColumn(catalogue, type, "tensionAllowableTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<AnisoMat2DTemperatureTable>().tensionAllowableTableIdColumn());
                       });
        registerColumn(catalogue, type, "compressionAllowableTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<AnisoMat2DTemperatureTable>().compressionAllowableTableIdColumn());
                       });
        registerColumn(
            catalogue, type, "shearAllowableTableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<AnisoMat2DTemperatureTable>().shearAllowableTableIdColumn());
            });
        registerColumn(
            catalogue, type, "structuralDampingTermTableIds",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<AnisoMat2DTemperatureTable>().structuralDampingTermTableIdsColumn());
            });
    }
    {
        using model::AxisymOrthoMatTemperatureTable;
        const std::uint16_t type = model::typeIndexOf<AxisymOrthoMatTemperatureTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymOrthoMatTemperatureTable>().idColumn()); });
        registerColumn(catalogue, type, "youngsModulusXTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<AxisymOrthoMatTemperatureTable>().youngsModulusXTableIdColumn());
                       });
        registerColumn(
            catalogue, type, "youngsModulusThetaTableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<AxisymOrthoMatTemperatureTable>().youngsModulusThetaTableIdColumn());
            });
        registerColumn(catalogue, type, "youngsModulusZTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<AxisymOrthoMatTemperatureTable>().youngsModulusZTableIdColumn());
                       });
        registerColumn(catalogue, type, "poissonRatioXthTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<AxisymOrthoMatTemperatureTable>().poissonRatioXthTableIdColumn());
                       });
        registerColumn(catalogue, type, "poissonRatioThzTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<AxisymOrthoMatTemperatureTable>().poissonRatioThzTableIdColumn());
                       });
        registerColumn(catalogue, type, "poissonRatioZxTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<AxisymOrthoMatTemperatureTable>().poissonRatioZxTableIdColumn());
                       });
        registerColumn(
            catalogue, type, "densityTableId", [](const auto& model)
            { return exportColumn(model->template table<AxisymOrthoMatTemperatureTable>().densityTableIdColumn()); });
        registerColumn(catalogue, type, "shearModulusZxTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<AxisymOrthoMatTemperatureTable>().shearModulusZxTableIdColumn());
                       });
        registerColumn(
            catalogue, type, "thermalExpansionXTableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<AxisymOrthoMatTemperatureTable>().thermalExpansionXTableIdColumn());
            });
        registerColumn(
            catalogue, type, "thermalExpansionThetaTableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<AxisymOrthoMatTemperatureTable>().thermalExpansionThetaTableIdColumn());
            });
        registerColumn(
            catalogue, type, "thermalExpansionZTableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<AxisymOrthoMatTemperatureTable>().thermalExpansionZTableIdColumn());
            });
        registerColumn(
            catalogue, type, "structuralDampingTableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<AxisymOrthoMatTemperatureTable>().structuralDampingTableIdColumn());
            });
    }
    {
        using model::HeatIsoMatTemperatureTable;
        const std::uint16_t type = model::typeIndexOf<HeatIsoMatTemperatureTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HeatIsoMatTemperatureTable>().idColumn()); });
        registerColumn(
            catalogue, type, "conductivityTableId", [](const auto& model)
            { return exportColumn(model->template table<HeatIsoMatTemperatureTable>().conductivityTableIdColumn()); });
        registerColumn(
            catalogue, type, "specificHeatTableId", [](const auto& model)
            { return exportColumn(model->template table<HeatIsoMatTemperatureTable>().specificHeatTableIdColumn()); });
        registerColumn(
            catalogue, type, "convectionCoefficientTableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<HeatIsoMatTemperatureTable>().convectionCoefficientTableIdColumn());
            });
        registerColumn(
            catalogue, type, "viscosityTableId", [](const auto& model)
            { return exportColumn(model->template table<HeatIsoMatTemperatureTable>().viscosityTableIdColumn()); });
        registerColumn(catalogue, type, "heatGenerationFactorTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<HeatIsoMatTemperatureTable>().heatGenerationFactorTableIdColumn());
                       });
    }
    {
        using model::HeatAnisoMatTemperatureTable;
        const std::uint16_t type = model::typeIndexOf<HeatAnisoMatTemperatureTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HeatAnisoMatTemperatureTable>().idColumn()); });
        registerColumn(catalogue, type, "conductivityXxTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<HeatAnisoMatTemperatureTable>().conductivityXxTableIdColumn());
                       });
        registerColumn(catalogue, type, "conductivityXyTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<HeatAnisoMatTemperatureTable>().conductivityXyTableIdColumn());
                       });
        registerColumn(catalogue, type, "conductivityXzTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<HeatAnisoMatTemperatureTable>().conductivityXzTableIdColumn());
                       });
        registerColumn(catalogue, type, "conductivityYyTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<HeatAnisoMatTemperatureTable>().conductivityYyTableIdColumn());
                       });
        registerColumn(catalogue, type, "conductivityYzTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<HeatAnisoMatTemperatureTable>().conductivityYzTableIdColumn());
                       });
        registerColumn(catalogue, type, "conductivityZzTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<HeatAnisoMatTemperatureTable>().conductivityZzTableIdColumn());
                       });
        registerColumn(
            catalogue, type, "specificHeatTableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<HeatAnisoMatTemperatureTable>().specificHeatTableIdColumn());
            });
        registerColumn(
            catalogue, type, "heatGenerationFactorTableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<HeatAnisoMatTemperatureTable>().heatGenerationFactorTableIdColumn());
            });
    }
    {
        using model::OrthoMat2DTemperatureTable;
        const std::uint16_t type = model::typeIndexOf<OrthoMat2DTemperatureTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<OrthoMat2DTemperatureTable>().idColumn()); });
        registerColumn(
            catalogue, type, "youngsModulus1TableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<OrthoMat2DTemperatureTable>().youngsModulus1TableIdColumn());
            });
        registerColumn(
            catalogue, type, "youngsModulus2TableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<OrthoMat2DTemperatureTable>().youngsModulus2TableIdColumn());
            });
        registerColumn(
            catalogue, type, "poissonRatio12TableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<OrthoMat2DTemperatureTable>().poissonRatio12TableIdColumn());
            });
        registerColumn(
            catalogue, type, "shearModulus12TableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<OrthoMat2DTemperatureTable>().shearModulus12TableIdColumn());
            });
        registerColumn(
            catalogue, type, "shearModulus1ZTableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<OrthoMat2DTemperatureTable>().shearModulus1ZTableIdColumn());
            });
        registerColumn(
            catalogue, type, "shearModulus2ZTableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<OrthoMat2DTemperatureTable>().shearModulus2ZTableIdColumn());
            });
        registerColumn(
            catalogue, type, "densityTableId", [](const auto& model)
            { return exportColumn(model->template table<OrthoMat2DTemperatureTable>().densityTableIdColumn()); });
        registerColumn(catalogue, type, "thermalExpansion1TableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<OrthoMat2DTemperatureTable>().thermalExpansion1TableIdColumn());
                       });
        registerColumn(catalogue, type, "thermalExpansion2TableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<OrthoMat2DTemperatureTable>().thermalExpansion2TableIdColumn());
                       });
        registerColumn(catalogue, type, "tensionStrength1TableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<OrthoMat2DTemperatureTable>().tensionStrength1TableIdColumn());
                       });
        registerColumn(catalogue, type, "compressionStrength1TableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<OrthoMat2DTemperatureTable>().compressionStrength1TableIdColumn());
                       });
        registerColumn(catalogue, type, "tensionStrength2TableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<OrthoMat2DTemperatureTable>().tensionStrength2TableIdColumn());
                       });
        registerColumn(catalogue, type, "compressionStrength2TableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<OrthoMat2DTemperatureTable>().compressionStrength2TableIdColumn());
                       });
        registerColumn(
            catalogue, type, "shearStrengthTableId", [](const auto& model)
            { return exportColumn(model->template table<OrthoMat2DTemperatureTable>().shearStrengthTableIdColumn()); });
        registerColumn(catalogue, type, "structuralDampingTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<OrthoMat2DTemperatureTable>().structuralDampingTableIdColumn());
                       });
        registerColumn(catalogue, type, "interactionTerm12TableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<OrthoMat2DTemperatureTable>().interactionTerm12TableIdColumn());
                       });
    }
    {
        using model::AnisoMat3DTemperatureTable;
        const std::uint16_t type = model::typeIndexOf<AnisoMat3DTemperatureTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AnisoMat3DTemperatureTable>().idColumn()); });
        registerColumn(
            catalogue, type, "stiffnessTableIds", [](const auto& model)
            { return exportColumn(model->template table<AnisoMat3DTemperatureTable>().stiffnessTableIdsColumn()); });
        registerColumn(
            catalogue, type, "densityTableId", [](const auto& model)
            { return exportColumn(model->template table<AnisoMat3DTemperatureTable>().densityTableIdColumn()); });
        registerColumn(catalogue, type, "thermalExpansionTableIds",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<AnisoMat3DTemperatureTable>().thermalExpansionTableIdsColumn());
                       });
        registerColumn(catalogue, type, "structuralDampingTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<AnisoMat3DTemperatureTable>().structuralDampingTableIdColumn());
                       });
        registerColumn(
            catalogue, type, "structuralDampingTermTableIds",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<AnisoMat3DTemperatureTable>().structuralDampingTermTableIdsColumn());
            });
    }
    {
        using model::ElastoPlasticMatTemperatureTable;
        const std::uint16_t type = model::typeIndexOf<ElastoPlasticMatTemperatureTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ElastoPlasticMatTemperatureTable>().idColumn()); });
    }
    {
        using model::FailureMatTemperatureTable;
        const std::uint16_t type = model::typeIndexOf<FailureMatTemperatureTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FailureMatTemperatureTable>().idColumn()); });
    }
    {
        using model::GasketMatTemperatureTable;
        const std::uint16_t type = model::typeIndexOf<GasketMatTemperatureTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<GasketMatTemperatureTable>().idColumn()); });
        registerColumn(catalogue, type, "membraneYoungsModulusTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<GasketMatTemperatureTable>().membraneYoungsModulusTableIdColumn());
                       });
        registerColumn(catalogue, type, "membranePoissonRatioTableId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<GasketMatTemperatureTable>().membranePoissonRatioTableIdColumn());
                       });
        registerColumn(
            catalogue, type, "membraneDensityTableId",
            [](const auto& model)
            {
                return exportColumn(model->template table<GasketMatTemperatureTable>().membraneDensityTableIdColumn());
            });
        registerColumn(
            catalogue, type, "loadingTableId", [](const auto& model)
            { return exportColumn(model->template table<GasketMatTemperatureTable>().loadingTableIdColumn()); });
        registerColumn(
            catalogue, type, "unloadingTableIds", [](const auto& model)
            { return exportColumn(model->template table<GasketMatTemperatureTable>().unloadingTableIdsColumn()); });
        registerColumn(
            catalogue, type, "yieldPressureTableId", [](const auto& model)
            { return exportColumn(model->template table<GasketMatTemperatureTable>().yieldPressureTableIdColumn()); });
        registerColumn(
            catalogue, type, "tensileModulusTableId", [](const auto& model)
            { return exportColumn(model->template table<GasketMatTemperatureTable>().tensileModulusTableIdColumn()); });
        registerColumn(
            catalogue, type, "transverseShearModulusTableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<GasketMatTemperatureTable>().transverseShearModulusTableIdColumn());
            });
        registerColumn(
            catalogue, type, "initialGapTableId", [](const auto& model)
            { return exportColumn(model->template table<GasketMatTemperatureTable>().initialGapTableIdColumn()); });
    }
    {
        using model::HyperelasticMatTemperatureTable;
        const std::uint16_t type = model::typeIndexOf<HyperelasticMatTemperatureTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<HyperelasticMatTemperatureTable>().idColumn()); });
    }
    {
        using model::OrthoMat3DNonlinearTemperatureTable;
        const std::uint16_t type = model::typeIndexOf<OrthoMat3DNonlinearTemperatureTable>();
        registerColumn(
            catalogue, type, "id", [](const auto& model)
            { return exportColumn(model->template table<OrthoMat3DNonlinearTemperatureTable>().idColumn()); });
        registerColumn(
            catalogue, type, "youngsModulus1TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearTemperatureTable>().youngsModulus1TableIdColumn());
            });
        registerColumn(
            catalogue, type, "youngsModulus2TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearTemperatureTable>().youngsModulus2TableIdColumn());
            });
        registerColumn(
            catalogue, type, "youngsModulus3TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearTemperatureTable>().youngsModulus3TableIdColumn());
            });
        registerColumn(
            catalogue, type, "poissonRatio12TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearTemperatureTable>().poissonRatio12TableIdColumn());
            });
        registerColumn(
            catalogue, type, "poissonRatio23TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearTemperatureTable>().poissonRatio23TableIdColumn());
            });
        registerColumn(
            catalogue, type, "poissonRatio31TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearTemperatureTable>().poissonRatio31TableIdColumn());
            });
        registerColumn(
            catalogue, type, "shearModulus12TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearTemperatureTable>().shearModulus12TableIdColumn());
            });
        registerColumn(
            catalogue, type, "shearModulus23TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearTemperatureTable>().shearModulus23TableIdColumn());
            });
        registerColumn(
            catalogue, type, "shearModulus31TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearTemperatureTable>().shearModulus31TableIdColumn());
            });
        registerColumn(
            catalogue, type, "thermalExpansion1TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearTemperatureTable>().thermalExpansion1TableIdColumn());
            });
        registerColumn(
            catalogue, type, "thermalExpansion2TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearTemperatureTable>().thermalExpansion2TableIdColumn());
            });
        registerColumn(
            catalogue, type, "thermalExpansion3TableId",
            [](const auto& model)
            {
                return exportColumn(
                    model->template table<OrthoMat3DNonlinearTemperatureTable>().thermalExpansion3TableIdColumn());
            });
    }
    {
        using model::UserMatTableTable;
        const std::uint16_t type = model::typeIndexOf<UserMatTableTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserMatTableTable>().idColumn()); });
    }
    {
        using model::ViscoElasticMatTemperatureTable;
        const std::uint16_t type = model::typeIndexOf<ViscoElasticMatTemperatureTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ViscoElasticMatTemperatureTable>().idColumn()); });
    }
    {
        using model::UserMatServiceTable;
        const std::uint16_t type = model::typeIndexOf<UserMatServiceTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserMatServiceTable>().idColumn()); });
    }
    {
        using model::UserMatTable;
        const std::uint16_t type = model::typeIndexOf<UserMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<UserMatTable>().idColumn()); });
    }
    {
        using model::ViscoElasticMatTable;
        const std::uint16_t type = model::typeIndexOf<ViscoElasticMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ViscoElasticMatTable>().idColumn()); });
        registerColumn(catalogue, type, "model", [](const auto& model)
                       { return exportColumn(model->template table<ViscoElasticMatTable>().modelColumn()); });
        registerColumn(
            catalogue, type, "solidThermalExpansion", [](const auto& model)
            { return exportColumn(model->template table<ViscoElasticMatTable>().solidThermalExpansionColumn()); });
        registerColumn(
            catalogue, type, "liquidThermalExpansion", [](const auto& model)
            { return exportColumn(model->template table<ViscoElasticMatTable>().liquidThermalExpansionColumn()); });
        registerColumn(catalogue, type, "terms", [](const auto& model)
                       { return exportVariableColumn(model->template table<ViscoElasticMatTable>().termsColumn()); });
        registerColumn(
            catalogue, type, "orthoTerms", [](const auto& model)
            { return exportVariableColumn(model->template table<ViscoElasticMatTable>().orthoTermsColumn()); });
    }
    {
        using model::ViscoPlasticMatTable;
        const std::uint16_t type = model::typeIndexOf<ViscoPlasticMatTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<ViscoPlasticMatTable>().idColumn()); });
    }
}

} // namespace oor::query::column_export
