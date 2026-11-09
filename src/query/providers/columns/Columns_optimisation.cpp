// The stored columns of the cards of the family "optimisation", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/base/errors/Errors.hpp"
#include "oor/model/cards/optimisation/DesignTargets.hpp"
#include "oor/model/cards/optimisation/ResponseArguments.hpp"
#include "oor/model/cards/optimisation/designBeadRegion/DesignBeadRegionTable.hpp"
#include "oor/model/cards/optimisation/designConstr/DesignConstrTable.hpp"
#include "oor/model/cards/optimisation/designConstrScreen/DesignConstrScreenTable.hpp"
#include "oor/model/cards/optimisation/designConstrUnion/DesignConstrUnionTable.hpp"
#include "oor/model/cards/optimisation/designControlSurface/DesignControlSurfaceTable.hpp"
#include "oor/model/cards/optimisation/designDiscreteValues/DesignDiscreteValuesTable.hpp"
#include "oor/model/cards/optimisation/designElemRelation1/DesignElemRelation1Table.hpp"
#include "oor/model/cards/optimisation/designElemRelation2/DesignElemRelation2Table.hpp"
#include "oor/model/cards/optimisation/designEquation/DesignEquationTable.hpp"
#include "oor/model/cards/optimisation/designLoadRelation1/DesignLoadRelation1Table.hpp"
#include "oor/model/cards/optimisation/designMatRelation1/DesignMatRelation1Table.hpp"
#include "oor/model/cards/optimisation/designMatRelation2/DesignMatRelation2Table.hpp"
#include "oor/model/cards/optimisation/designNodeMove/DesignNodeMoveTable.hpp"
#include "oor/model/cards/optimisation/designPropRelation1/DesignPropRelation1Table.hpp"
#include "oor/model/cards/optimisation/designPropRelation2/DesignPropRelation2Table.hpp"
#include "oor/model/cards/optimisation/designResp1/DesignResp1References.hpp"
#include "oor/model/cards/optimisation/designResp1/DesignResp1Table.hpp"
#include "oor/model/cards/optimisation/designResp2/DesignResp2Table.hpp"
#include "oor/model/cards/optimisation/designResp3/DesignResp3Table.hpp"
#include "oor/model/cards/optimisation/designTopologyRegion/DesignTopologyRegionTable.hpp"
#include "oor/model/cards/optimisation/designTopometryRegion/DesignTopometryRegionTable.hpp"
#include "oor/model/cards/optimisation/designVar/DesignVarTable.hpp"
#include "oor/model/cards/optimisation/designVarLink/DesignVarLinkTable.hpp"
#include "oor/model/expressions/EquationEvaluator.hpp"
#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

void registerOptimisationColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::DesignBeadRegionTable;
        const std::uint16_t type = model::typeIndexOf<DesignBeadRegionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DesignBeadRegionTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<DesignBeadRegionTable>().propIdColumn()); });
    }
    {
        using model::DesignConstrUnionTable;
        const std::uint16_t type = model::typeIndexOf<DesignConstrUnionTable>();
        registerColumn(
            catalogue, type, "constraintSetId", [](const auto& model)
            { return exportColumn(model->template table<DesignConstrUnionTable>().constraintSetIdColumn()); });
    }
    {
        using model::DesignConstrTable;
        const std::uint16_t type = model::typeIndexOf<DesignConstrTable>();
        registerColumn(catalogue, type, "constraintSetId", [](const auto& model)
                       { return exportColumn(model->template table<DesignConstrTable>().constraintSetIdColumn()); });
        registerColumn(catalogue, type, "responseId", [](const auto& model)
                       { return exportColumn(model->template table<DesignConstrTable>().responseIdColumn()); });
        registerColumn(catalogue, type, "lowerAllowable", [](const auto& model)
                       { return exportColumn(model->template table<DesignConstrTable>().lowerAllowableColumn()); });
        registerColumn(
            catalogue, type, "lowerAllowableTableId", [](const auto& model)
            { return exportColumn(model->template table<DesignConstrTable>().lowerAllowableTableIdColumn()); });
        registerColumn(catalogue, type, "upperAllowable", [](const auto& model)
                       { return exportColumn(model->template table<DesignConstrTable>().upperAllowableColumn()); });
        registerColumn(
            catalogue, type, "upperAllowableTableId", [](const auto& model)
            { return exportColumn(model->template table<DesignConstrTable>().upperAllowableTableIdColumn()); });
        registerColumn(catalogue, type, "lowFrequency", [](const auto& model)
                       { return exportColumn(model->template table<DesignConstrTable>().lowFrequencyColumn()); });
        registerColumn(catalogue, type, "highFrequency", [](const auto& model)
                       { return exportColumn(model->template table<DesignConstrTable>().highFrequencyColumn()); });
    }
    {
        using model::DesignDiscreteValuesTable;
        const std::uint16_t type = model::typeIndexOf<DesignDiscreteValuesTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DesignDiscreteValuesTable>().idColumn()); });
    }
    {
        using model::DesignEquationTable;
        const std::uint16_t type = model::typeIndexOf<DesignEquationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DesignEquationTable>().idColumn()); });
    }
    {
        using model::DesignVarTable;
        const std::uint16_t type = model::typeIndexOf<DesignVarTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DesignVarTable>().idColumn()); });
        registerColumn(catalogue, type, "entryLabel", [](const auto& model)
                       { return exportTextColumn(model->template table<DesignVarTable>().entryLabelColumn()); });
        registerColumn(catalogue, type, "initialValue", [](const auto& model)
                       { return exportColumn(model->template table<DesignVarTable>().initialValueColumn()); });
        registerColumn(catalogue, type, "lowerBound", [](const auto& model)
                       { return exportColumn(model->template table<DesignVarTable>().lowerBoundColumn()); });
        registerColumn(catalogue, type, "upperBound", [](const auto& model)
                       { return exportColumn(model->template table<DesignVarTable>().upperBoundColumn()); });
        registerColumn(catalogue, type, "designVarMoveLimit", [](const auto& model)
                       { return exportColumn(model->template table<DesignVarTable>().designVarMoveLimitColumn()); });
        registerColumn(catalogue, type, "discreteValuesId", [](const auto& model)
                       { return exportColumn(model->template table<DesignVarTable>().discreteValuesIdColumn()); });
    }
    {
        using model::DesignVarLinkTable;
        const std::uint16_t type = model::typeIndexOf<DesignVarLinkTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DesignVarLinkTable>().idColumn()); });
    }
    {
        using model::DesignResp1Table;
        const std::uint16_t type = model::typeIndexOf<DesignResp1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DesignResp1Table>().idColumn()); });
    }
    {
        using model::DesignResp2Table;
        const std::uint16_t type = model::typeIndexOf<DesignResp2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DesignResp2Table>().idColumn()); });
    }
    {
        using model::DesignResp3Table;
        const std::uint16_t type = model::typeIndexOf<DesignResp3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DesignResp3Table>().idColumn()); });
    }
    {
        using model::DesignConstrScreenTable;
        const std::uint16_t type = model::typeIndexOf<DesignConstrScreenTable>();
        registerColumn(
            catalogue, type, "responseKind", [](const auto& model)
            { return exportTextColumn(model->template table<DesignConstrScreenTable>().responseKindColumn()); });
        registerColumn(catalogue, type, "threshold", [](const auto& model)
                       { return exportColumn(model->template table<DesignConstrScreenTable>().thresholdColumn()); });
        registerColumn(catalogue, type, "maxRetained", [](const auto& model)
                       { return exportColumn(model->template table<DesignConstrScreenTable>().maxRetainedColumn()); });
    }
    {
        using model::DesignElemRelation1Table;
        const std::uint16_t type = model::typeIndexOf<DesignElemRelation1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DesignElemRelation1Table>().idColumn()); });
    }
    {
        using model::DesignElemRelation2Table;
        const std::uint16_t type = model::typeIndexOf<DesignElemRelation2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DesignElemRelation2Table>().idColumn()); });
    }
    {
        using model::DesignNodeMoveTable;
        const std::uint16_t type = model::typeIndexOf<DesignNodeMoveTable>();
        registerColumn(catalogue, type, "designVarId", [](const auto& model)
                       { return exportColumn(model->template table<DesignNodeMoveTable>().designVarIdColumn()); });
        registerColumn(catalogue, type, "nodeId", [](const auto& model)
                       { return exportColumn(model->template table<DesignNodeMoveTable>().nodeIdColumn()); });
        registerColumn(catalogue, type, "frameId", [](const auto& model)
                       { return exportColumn(model->template table<DesignNodeMoveTable>().frameIdColumn()); });
        registerColumn(catalogue, type, "coefficient", [](const auto& model)
                       { return exportColumn(model->template table<DesignNodeMoveTable>().coefficientColumn()); });
        registerColumn(catalogue, type, "direction", [](const auto& model)
                       { return exportColumn(model->template table<DesignNodeMoveTable>().directionColumn()); });
    }
    {
        using model::DesignLoadRelation1Table;
        const std::uint16_t type = model::typeIndexOf<DesignLoadRelation1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DesignLoadRelation1Table>().idColumn()); });
        registerColumn(catalogue, type, "setId", [](const auto& model)
                       { return exportColumn(model->template table<DesignLoadRelation1Table>().setIdColumn()); });
    }
    {
        using model::DesignMatRelation1Table;
        const std::uint16_t type = model::typeIndexOf<DesignMatRelation1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DesignMatRelation1Table>().idColumn()); });
    }
    {
        using model::DesignMatRelation2Table;
        const std::uint16_t type = model::typeIndexOf<DesignMatRelation2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DesignMatRelation2Table>().idColumn()); });
    }
    {
        using model::DesignPropRelation1Table;
        const std::uint16_t type = model::typeIndexOf<DesignPropRelation1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DesignPropRelation1Table>().idColumn()); });
    }
    {
        using model::DesignPropRelation2Table;
        const std::uint16_t type = model::typeIndexOf<DesignPropRelation2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DesignPropRelation2Table>().idColumn()); });
    }
    {
        using model::DesignControlSurfaceTable;
        const std::uint16_t type = model::typeIndexOf<DesignControlSurfaceTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DesignControlSurfaceTable>().idColumn()); });
        registerColumn(
            catalogue, type, "controlSurfaceLabel",
            [](const auto& model)
            {
                return exportTextColumn(model->template table<DesignControlSurfaceTable>().controlSurfaceLabelColumn());
            });
        registerColumn(catalogue, type, "trimId", [](const auto& model)
                       { return exportColumn(model->template table<DesignControlSurfaceTable>().trimIdColumn()); });
        registerColumn(
            catalogue, type, "designVarId", [](const auto& model)
            { return exportColumn(model->template table<DesignControlSurfaceTable>().designVarIdColumn()); });
        registerColumn(
            catalogue, type, "coefficient", [](const auto& model)
            { return exportColumn(model->template table<DesignControlSurfaceTable>().coefficientColumn()); });
    }
    {
        using model::DesignTopometryRegionTable;
        const std::uint16_t type = model::typeIndexOf<DesignTopometryRegionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DesignTopometryRegionTable>().idColumn()); });
        registerColumn(catalogue, type, "propId", [](const auto& model)
                       { return exportColumn(model->template table<DesignTopometryRegionTable>().propIdColumn()); });
    }
    {
        using model::DesignTopologyRegionTable;
        const std::uint16_t type = model::typeIndexOf<DesignTopologyRegionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<DesignTopologyRegionTable>().idColumn()); });
    }
}

} // namespace oor::query::column_export
