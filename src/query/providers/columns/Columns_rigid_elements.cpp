// The stored columns of the cards of the family "rigid_elements", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/base/text/DigitSets.hpp"
#include "oor/model/cards/rigid_elements/rigidAxisymConnection/RigidAxisymConnectionTable.hpp"
#include "oor/model/cards/rigid_elements/rigidBar/RigidBarTable.hpp"
#include "oor/model/cards/rigid_elements/rigidBar1/RigidBar1Table.hpp"
#include "oor/model/cards/rigid_elements/rigidBodyMulti/RigidBodyMultiTable.hpp"
#include "oor/model/cards/rigid_elements/rigidBodySearched/RigidBodySearchedTable.hpp"
#include "oor/model/cards/rigid_elements/rigidBodyStar/RigidBodyStarTable.hpp"
#include "oor/model/cards/rigid_elements/rigidInterpolation/RigidInterpolationTable.hpp"
#include "oor/model/cards/rigid_elements/rigidJoint/RigidJointTable.hpp"
#include "oor/model/cards/rigid_elements/rigidMeshConnection/RigidMeshConnectionTable.hpp"
#include "oor/model/cards/rigid_elements/rigidRod/RigidRodTable.hpp"
#include "oor/model/cards/rigid_elements/rigidShellSolidConnection/RigidShellSolidConnectionTable.hpp"
#include "oor/model/cards/rigid_elements/rigidSpline/RigidSplineTable.hpp"
#include "oor/model/cards/rigid_elements/rigidTriaPlate/RigidTriaPlateTable.hpp"
#include "oor/model/cards/rigid_elements/rigidTriaPlate1/RigidTriaPlate1Table.hpp"
#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

void registerRigidElementsColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::RigidBarTable;
        const std::uint16_t type = model::typeIndexOf<RigidBarTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RigidBarTable>().idColumn()); });
        registerColumn(catalogue, type, "nodeA", [](const auto& model)
                       { return exportColumn(model->template table<RigidBarTable>().nodeAColumn()); });
        registerColumn(catalogue, type, "nodeB", [](const auto& model)
                       { return exportColumn(model->template table<RigidBarTable>().nodeBColumn()); });
        registerColumn(catalogue, type, "independentComponentsA", [](const auto& model)
                       { return exportColumn(model->template table<RigidBarTable>().independentComponentsAColumn()); });
        registerColumn(catalogue, type, "independentComponentsB", [](const auto& model)
                       { return exportColumn(model->template table<RigidBarTable>().independentComponentsBColumn()); });
        registerColumn(catalogue, type, "componentsAtA", [](const auto& model)
                       { return exportColumn(model->template table<RigidBarTable>().componentsAtAColumn()); });
        registerColumn(catalogue, type, "componentsAtB", [](const auto& model)
                       { return exportColumn(model->template table<RigidBarTable>().componentsAtBColumn()); });
        registerColumn(catalogue, type, "alphaCoefficient", [](const auto& model)
                       { return exportColumn(model->template table<RigidBarTable>().alphaCoefficientColumn()); });
        registerColumn(catalogue, type, "referenceTemperature", [](const auto& model)
                       { return exportColumn(model->template table<RigidBarTable>().referenceTemperatureColumn()); });
    }
    {
        using model::RigidBar1Table;
        const std::uint16_t type = model::typeIndexOf<RigidBar1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RigidBar1Table>().idColumn()); });
        registerColumn(catalogue, type, "nodeA", [](const auto& model)
                       { return exportColumn(model->template table<RigidBar1Table>().nodeAColumn()); });
        registerColumn(catalogue, type, "nodeB", [](const auto& model)
                       { return exportColumn(model->template table<RigidBar1Table>().nodeBColumn()); });
        registerColumn(catalogue, type, "componentsB", [](const auto& model)
                       { return exportColumn(model->template table<RigidBar1Table>().componentsBColumn()); });
        registerColumn(catalogue, type, "alphaCoefficient", [](const auto& model)
                       { return exportColumn(model->template table<RigidBar1Table>().alphaCoefficientColumn()); });
        registerColumn(catalogue, type, "referenceTemperature", [](const auto& model)
                       { return exportColumn(model->template table<RigidBar1Table>().referenceTemperatureColumn()); });
    }
    {
        using model::RigidAxisymConnectionTable;
        const std::uint16_t type = model::typeIndexOf<RigidAxisymConnectionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RigidAxisymConnectionTable>().idColumn()); });
        registerColumn(catalogue, type, "node3D", [](const auto& model)
                       { return exportColumn(model->template table<RigidAxisymConnectionTable>().node3DColumn()); });
        registerColumn(
            catalogue, type, "axisymNode", [](const auto& model)
            { return exportColumn(model->template table<RigidAxisymConnectionTable>().axisymNodeColumn()); });
    }
    {
        using model::RigidBodyMultiTable;
        const std::uint16_t type = model::typeIndexOf<RigidBodyMultiTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RigidBodyMultiTable>().idColumn()); });
        registerColumn(
            catalogue, type, "independentNode", [](const auto& model)
            { return exportVariableColumn(model->template table<RigidBodyMultiTable>().independentNodeColumn()); });
        registerColumn(
            catalogue, type, "independentComponents",
            [](const auto& model)
            {
                return exportVariableColumn(model->template table<RigidBodyMultiTable>().independentComponentsColumn());
            });
        registerColumn(
            catalogue, type, "dependentNode", [](const auto& model)
            { return exportVariableColumn(model->template table<RigidBodyMultiTable>().dependentNodeColumn()); });
        registerColumn(
            catalogue, type, "dependentComponents", [](const auto& model)
            { return exportVariableColumn(model->template table<RigidBodyMultiTable>().dependentComponentsColumn()); });
        registerColumn(catalogue, type, "alphaCoefficient", [](const auto& model)
                       { return exportColumn(model->template table<RigidBodyMultiTable>().alphaCoefficientColumn()); });
        registerColumn(
            catalogue, type, "referenceTemperature", [](const auto& model)
            { return exportColumn(model->template table<RigidBodyMultiTable>().referenceTemperatureColumn()); });
    }
    {
        using model::RigidBodyStarTable;
        const std::uint16_t type = model::typeIndexOf<RigidBodyStarTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RigidBodyStarTable>().idColumn()); });
        registerColumn(catalogue, type, "independentNode", [](const auto& model)
                       { return exportColumn(model->template table<RigidBodyStarTable>().independentColumn()); });
        registerColumn(catalogue, type, "dependentComponents", [](const auto& model)
                       { return exportColumn(model->template table<RigidBodyStarTable>().componentsColumn()); });
        registerColumn(catalogue, type, "dependentNode", [](const auto& model)
                       { return exportVariableColumn(model->template table<RigidBodyStarTable>().dependentColumn()); });
        registerColumn(catalogue, type, "alphaCoefficient", [](const auto& model)
                       { return exportColumn(model->template table<RigidBodyStarTable>().alphaCoefficientColumn()); });
        registerColumn(
            catalogue, type, "referenceTemperature", [](const auto& model)
            { return exportColumn(model->template table<RigidBodyStarTable>().referenceTemperatureColumn()); });
    }
    {
        using model::RigidBodySearchedTable;
        const std::uint16_t type = model::typeIndexOf<RigidBodySearchedTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RigidBodySearchedTable>().idColumn()); });
        registerColumn(catalogue, type, "spotNode", [](const auto& model)
                       { return exportColumn(model->template table<RigidBodySearchedTable>().spotNodeColumn()); });
    }
    {
        using model::RigidInterpolationTable;
        const std::uint16_t type = model::typeIndexOf<RigidInterpolationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RigidInterpolationTable>().idColumn()); });
        registerColumn(
            catalogue, type, "referenceNode", [](const auto& model)
            { return exportColumn(model->template table<RigidInterpolationTable>().referenceNodeColumn()); });
        registerColumn(
            catalogue, type, "referenceComponents", [](const auto& model)
            { return exportColumn(model->template table<RigidInterpolationTable>().refComponentsColumn()); });
        registerColumn(
            catalogue, type, "weight", [](const auto& model)
            { return exportVariableColumn(model->template table<RigidInterpolationTable>().groupWeightColumn()); });
        registerColumn(
            catalogue, type, "components", [](const auto& model)
            { return exportVariableColumn(model->template table<RigidInterpolationTable>().groupComponentsColumn()); });
        registerColumn(
            catalogue, type, "groupSize", [](const auto& model)
            { return exportVariableColumn(model->template table<RigidInterpolationTable>().groupSizeColumn()); });
        registerColumn(
            catalogue, type, "node", [](const auto& model)
            { return exportVariableColumn(model->template table<RigidInterpolationTable>().groupNodeColumn()); });
        registerColumn(
            catalogue, type, "dependentNode", [](const auto& model)
            { return exportVariableColumn(model->template table<RigidInterpolationTable>().dependentNodeColumn()); });
        registerColumn(catalogue, type, "dependentComponents",
                       [](const auto& model)
                       {
                           return exportVariableColumn(
                               model->template table<RigidInterpolationTable>().dependentComponentsColumn());
                       });
        registerColumn(
            catalogue, type, "alphaCoefficient", [](const auto& model)
            { return exportColumn(model->template table<RigidInterpolationTable>().alphaCoefficientColumn()); });
        registerColumn(
            catalogue, type, "referenceTemperature", [](const auto& model)
            { return exportColumn(model->template table<RigidInterpolationTable>().referenceTemperatureColumn()); });
    }
    {
        using model::RigidMeshConnectionTable;
        const std::uint16_t type = model::typeIndexOf<RigidMeshConnectionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RigidMeshConnectionTable>().idColumn()); });
    }
    {
        using model::RigidJointTable;
        const std::uint16_t type = model::typeIndexOf<RigidJointTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RigidJointTable>().idColumn()); });
        registerColumn(catalogue, type, "nodeA", [](const auto& model)
                       { return exportColumn(model->template table<RigidJointTable>().nodeAColumn()); });
        registerColumn(catalogue, type, "nodeB", [](const auto& model)
                       { return exportColumn(model->template table<RigidJointTable>().nodeBColumn()); });
        registerColumn(catalogue, type, "componentsB", [](const auto& model)
                       { return exportColumn(model->template table<RigidJointTable>().componentsBColumn()); });
    }
    {
        using model::RigidRodTable;
        const std::uint16_t type = model::typeIndexOf<RigidRodTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RigidRodTable>().idColumn()); });
        registerColumn(catalogue, type, "nodeA", [](const auto& model)
                       { return exportColumn(model->template table<RigidRodTable>().nodeAColumn()); });
        registerColumn(catalogue, type, "nodeB", [](const auto& model)
                       { return exportColumn(model->template table<RigidRodTable>().nodeBColumn()); });
        registerColumn(catalogue, type, "componentsAtA", [](const auto& model)
                       { return exportColumn(model->template table<RigidRodTable>().componentsAtAColumn()); });
        registerColumn(catalogue, type, "componentsAtB", [](const auto& model)
                       { return exportColumn(model->template table<RigidRodTable>().componentsAtBColumn()); });
        registerColumn(catalogue, type, "alphaCoefficient", [](const auto& model)
                       { return exportColumn(model->template table<RigidRodTable>().alphaCoefficientColumn()); });
        registerColumn(catalogue, type, "referenceTemperature", [](const auto& model)
                       { return exportColumn(model->template table<RigidRodTable>().referenceTemperatureColumn()); });
    }
    {
        using model::RigidSplineTable;
        const std::uint16_t type = model::typeIndexOf<RigidSplineTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RigidSplineTable>().idColumn()); });
        registerColumn(catalogue, type, "diameterRatio", [](const auto& model)
                       { return exportColumn(model->template table<RigidSplineTable>().diameterRatioColumn()); });
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportVariableColumn(model->template table<RigidSplineTable>().nodeColumn()); });
        registerColumn(catalogue, type, "component", [](const auto& model)
                       { return exportVariableColumn(model->template table<RigidSplineTable>().componentColumn()); });
    }
    {
        using model::RigidShellSolidConnectionTable;
        const std::uint16_t type = model::typeIndexOf<RigidShellSolidConnectionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RigidShellSolidConnectionTable>().idColumn()); });
        registerColumn(catalogue, type, "kind", [](const auto& model)
                       { return exportColumn(model->template table<RigidShellSolidConnectionTable>().typeColumn()); });
        registerColumn(catalogue, type, "ids", [](const auto& model)
                       { return exportColumn(model->template table<RigidShellSolidConnectionTable>().idsColumn()); });
    }
    {
        using model::RigidTriaPlateTable;
        const std::uint16_t type = model::typeIndexOf<RigidTriaPlateTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RigidTriaPlateTable>().idColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<RigidTriaPlateTable>().nodesColumn()); });
        registerColumn(
            catalogue, type, "independentComponents", [](const auto& model)
            { return exportColumn(model->template table<RigidTriaPlateTable>().independentComponentsColumn()); });
        registerColumn(
            catalogue, type, "dependentComponents", [](const auto& model)
            { return exportColumn(model->template table<RigidTriaPlateTable>().dependentComponentsColumn()); });
        registerColumn(catalogue, type, "alphaCoefficient", [](const auto& model)
                       { return exportColumn(model->template table<RigidTriaPlateTable>().alphaCoefficientColumn()); });
        registerColumn(
            catalogue, type, "referenceTemperature", [](const auto& model)
            { return exportColumn(model->template table<RigidTriaPlateTable>().referenceTemperatureColumn()); });
    }
    {
        using model::RigidTriaPlate1Table;
        const std::uint16_t type = model::typeIndexOf<RigidTriaPlate1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RigidTriaPlate1Table>().idColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<RigidTriaPlate1Table>().nodesColumn()); });
        registerColumn(
            catalogue, type, "dependentComponents", [](const auto& model)
            { return exportColumn(model->template table<RigidTriaPlate1Table>().dependentComponentsColumn()); });
        registerColumn(
            catalogue, type, "alphaCoefficient", [](const auto& model)
            { return exportColumn(model->template table<RigidTriaPlate1Table>().alphaCoefficientColumn()); });
        registerColumn(
            catalogue, type, "referenceTemperature", [](const auto& model)
            { return exportColumn(model->template table<RigidTriaPlate1Table>().referenceTemperatureColumn()); });
    }
}

} // namespace oor::query::column_export
