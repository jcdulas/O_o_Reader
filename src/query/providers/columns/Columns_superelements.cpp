// The stored columns of the cards of the family "superelements", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/superelements/axisymConeSector/AxisymConeSectorTable.hpp"
#include "oor/model/cards/superelements/axisymConeSupport/AxisymConeSupportTable.hpp"
#include "oor/model/cards/superelements/fixedBoundaryDofs/FixedBoundaryDofsTable.hpp"
#include "oor/model/cards/superelements/fixedBoundaryDofs1/FixedBoundaryDofs1Table.hpp"
#include "oor/model/cards/superelements/freeBoundaryDofs/FreeBoundaryDofsTable.hpp"
#include "oor/model/cards/superelements/freeBoundaryDofs1/FreeBoundaryDofs1Table.hpp"
#include "oor/model/cards/superelements/massCaseCombination/MassCaseCombinationTable.hpp"
#include "oor/model/cards/superelements/superelementAccessPoints/SuperelementAccessPointsTable.hpp"
#include "oor/model/cards/superelements/superelementBoundarySearch/SuperelementBoundarySearchTable.hpp"
#include "oor/model/cards/superelements/superelementBulkOptions/SuperelementBulkOptionsTable.hpp"
#include "oor/model/cards/superelements/superelementConnection/SuperelementConnectionTable.hpp"
#include "oor/model/cards/superelements/superelementCopyPoints/SuperelementCopyPointsTable.hpp"
#include "oor/model/cards/superelements/superelementCopySequence/SuperelementCopySequenceTable.hpp"
#include "oor/model/cards/superelements/superelementDesignLink/SuperelementDesignLinkTable.hpp"
#include "oor/model/cards/superelements/superelementDesignResp2/SuperelementDesignResp2Table.hpp"
#include "oor/model/cards/superelements/superelementDesignResp3/SuperelementDesignResp3Table.hpp"
#include "oor/model/cards/superelements/superelementElemTransfer/SuperelementElemTransferTable.hpp"
#include "oor/model/cards/superelements/superelementExcludedNodes/SuperelementExcludedNodesTable.hpp"
#include "oor/model/cards/superelements/superelementExteriorPoints/SuperelementExteriorPointsTable.hpp"
#include "oor/model/cards/superelements/superelementExternalBoundary/SuperelementExternalBoundaryTable.hpp"
#include "oor/model/cards/superelements/superelementFixedDofSet/SuperelementFixedDofSetTable.hpp"
#include "oor/model/cards/superelements/superelementFixedDofSet1/SuperelementFixedDofSet1Table.hpp"
#include "oor/model/cards/superelements/superelementFreeDofSet/SuperelementFreeDofSetTable.hpp"
#include "oor/model/cards/superelements/superelementFreeDofSet1/SuperelementFreeDofSet1Table.hpp"
#include "oor/model/cards/superelements/superelementGeneralizedDofSet/SuperelementGeneralizedDofSetTable.hpp"
#include "oor/model/cards/superelements/superelementGeneralizedDofSet1/SuperelementGeneralizedDofSet1Table.hpp"
#include "oor/model/cards/superelements/superelementGeneralizedPointCount/SuperelementGeneralizedPointCountTable.hpp"
#include "oor/model/cards/superelements/superelementInteriorNodes/SuperelementInteriorNodesTable.hpp"
#include "oor/model/cards/superelements/superelementLabel/SuperelementLabelTable.hpp"
#include "oor/model/cards/superelements/superelementLoadScale/SuperelementLoadScaleTable.hpp"
#include "oor/model/cards/superelements/superelementLocation/SuperelementLocationTable.hpp"
#include "oor/model/cards/superelements/superelementMirrorPlane/SuperelementMirrorPlaneTable.hpp"
#include "oor/model/cards/superelements/superelementReleasedDofs/SuperelementReleasedDofsTable.hpp"
#include "oor/model/cards/superelements/superelementSupport/SuperelementSupportTable.hpp"
#include "oor/model/cards/superelements/superelementTree/SuperelementTreeTable.hpp"
#include "oor/model/cards/superelements/superelementUserDofSet/SuperelementUserDofSetTable.hpp"
#include "oor/model/cards/superelements/superelementUserDofSet1/SuperelementUserDofSet1Table.hpp"
#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

void registerSuperelementsColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::SuperelementAccessPointsTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementAccessPointsTable>();
        registerColumn(
            catalogue, type, "nodeIds", [](const auto& model)
            { return exportVariableColumn(model->template table<SuperelementAccessPointsTable>().nodeIdsColumn()); });
    }
    {
        using model::FixedBoundaryDofsTable;
        const std::uint16_t type = model::typeIndexOf<FixedBoundaryDofsTable>();
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportVariableColumn(model->template table<FixedBoundaryDofsTable>().nodeColumn()); });
        registerColumn(
            catalogue, type, "component", [](const auto& model)
            { return exportVariableColumn(model->template table<FixedBoundaryDofsTable>().componentColumn()); });
    }
    {
        using model::FixedBoundaryDofs1Table;
        const std::uint16_t type = model::typeIndexOf<FixedBoundaryDofs1Table>();
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<FixedBoundaryDofs1Table>().componentsColumn()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<FixedBoundaryDofs1Table>().runsColumn()); });
    }
    {
        using model::FreeBoundaryDofs1Table;
        const std::uint16_t type = model::typeIndexOf<FreeBoundaryDofs1Table>();
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<FreeBoundaryDofs1Table>().componentsColumn()); });
        registerColumn(catalogue, type, "runs", [](const auto& model)
                       { return exportVariableColumn(model->template table<FreeBoundaryDofs1Table>().runsColumn()); });
        registerColumn(catalogue, type, "all", [](const auto& model)
                       { return exportColumn(model->template table<FreeBoundaryDofs1Table>().allColumn()); });
    }
    {
        using model::FreeBoundaryDofsTable;
        const std::uint16_t type = model::typeIndexOf<FreeBoundaryDofsTable>();
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportVariableColumn(model->template table<FreeBoundaryDofsTable>().nodeColumn()); });
        registerColumn(
            catalogue, type, "component", [](const auto& model)
            { return exportVariableColumn(model->template table<FreeBoundaryDofsTable>().componentColumn()); });
    }
    {
        using model::SuperelementCopyPointsTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementCopyPointsTable>();
        registerColumn(catalogue, type, "number", [](const auto& model)
                       { return exportColumn(model->template table<SuperelementCopyPointsTable>().numberColumn()); });
    }
    {
        using model::SuperelementExteriorPointsTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementExteriorPointsTable>();
        registerColumn(
            catalogue, type, "superelementId", [](const auto& model)
            { return exportColumn(model->template table<SuperelementExteriorPointsTable>().superelementIdColumn()); });
    }
    {
        using model::SuperelementExternalBoundaryTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementExternalBoundaryTable>();
        registerColumn(
            catalogue, type, "count", [](const auto& model)
            { return exportColumn(model->template table<SuperelementExternalBoundaryTable>().countColumn()); });
    }
    {
        using model::MassCaseCombinationTable;
        const std::uint16_t type = model::typeIndexOf<MassCaseCombinationTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MassCaseCombinationTable>().idColumn()); });
        registerColumn(
            catalogue, type, "overallScale", [](const auto& model)
            { return exportColumn(model->template table<MassCaseCombinationTable>().overallScaleColumn()); });
    }
    {
        using model::SuperelementReleasedDofsTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementReleasedDofsTable>();
        registerColumn(
            catalogue, type, "superelementId", [](const auto& model)
            { return exportColumn(model->template table<SuperelementReleasedDofsTable>().superelementIdColumn()); });
    }
    {
        using model::SuperelementBoundarySearchTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementBoundarySearchTable>();
        registerColumn(
            catalogue, type, "superelementIdA", [](const auto& model)
            { return exportColumn(model->template table<SuperelementBoundarySearchTable>().superelementIdAColumn()); });
    }
    {
        using model::SuperelementFixedDofSetTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementFixedDofSetTable>();
        registerColumn(
            catalogue, type, "superelementId", [](const auto& model)
            { return exportColumn(model->template table<SuperelementFixedDofSetTable>().superelementIdColumn()); });
    }
    {
        using model::SuperelementFixedDofSet1Table;
        const std::uint16_t type = model::typeIndexOf<SuperelementFixedDofSet1Table>();
        registerColumn(
            catalogue, type, "superelementId", [](const auto& model)
            { return exportColumn(model->template table<SuperelementFixedDofSet1Table>().superelementIdColumn()); });
    }
    {
        using model::SuperelementBulkOptionsTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementBulkOptionsTable>();
        registerColumn(
            catalogue, type, "superelementId", [](const auto& model)
            { return exportColumn(model->template table<SuperelementBulkOptionsTable>().superelementIdColumn()); });
        registerColumn(
            catalogue, type, "partType", [](const auto& model)
            { return exportColumn(model->template table<SuperelementBulkOptionsTable>().partTypeColumn()); });
        registerColumn(catalogue, type, "referenceSuperelementId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<SuperelementBulkOptionsTable>().referenceSuperelementIdColumn());
                       });
        registerColumn(catalogue, type, "method", [](const auto& model)
                       { return exportColumn(model->template table<SuperelementBulkOptionsTable>().methodColumn()); });
        registerColumn(
            catalogue, type, "tolerance", [](const auto& model)
            { return exportColumn(model->template table<SuperelementBulkOptionsTable>().toleranceColumn()); });
        registerColumn(
            catalogue, type, "location", [](const auto& model)
            { return exportColumn(model->template table<SuperelementBulkOptionsTable>().locationColumn()); });
        registerColumn(
            catalogue, type, "unitNumber", [](const auto& model)
            { return exportColumn(model->template table<SuperelementBulkOptionsTable>().unitNumberColumn()); });
    }
    {
        using model::SuperelementConnectionTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementConnectionTable>();
        registerColumn(
            catalogue, type, "superelementIdA", [](const auto& model)
            { return exportColumn(model->template table<SuperelementConnectionTable>().superelementIdAColumn()); });
        registerColumn(
            catalogue, type, "superelementIdB", [](const auto& model)
            { return exportColumn(model->template table<SuperelementConnectionTable>().superelementIdBColumn()); });
    }
    {
        using model::SuperelementFreeDofSetTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementFreeDofSetTable>();
        registerColumn(
            catalogue, type, "superelementId", [](const auto& model)
            { return exportColumn(model->template table<SuperelementFreeDofSetTable>().superelementIdColumn()); });
    }
    {
        using model::SuperelementFreeDofSet1Table;
        const std::uint16_t type = model::typeIndexOf<SuperelementFreeDofSet1Table>();
        registerColumn(
            catalogue, type, "superelementId", [](const auto& model)
            { return exportColumn(model->template table<SuperelementFreeDofSet1Table>().superelementIdColumn()); });
    }
    {
        using model::AxisymConeSectorTable;
        const std::uint16_t type = model::typeIndexOf<AxisymConeSectorTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<AxisymConeSectorTable>().idColumn()); });
        registerColumn(catalogue, type, "ringId", [](const auto& model)
                       { return exportColumn(model->template table<AxisymConeSectorTable>().ringIdColumn()); });
        registerColumn(catalogue, type, "radius", [](const auto& model)
                       { return exportColumn(model->template table<AxisymConeSectorTable>().radiusColumn()); });
        registerColumn(catalogue, type, "phiAngle1", [](const auto& model)
                       { return exportColumn(model->template table<AxisymConeSectorTable>().phiAngle1Column()); });
        registerColumn(catalogue, type, "phiAngle2", [](const auto& model)
                       { return exportColumn(model->template table<AxisymConeSectorTable>().phiAngle2Column()); });
    }
    {
        using model::SuperelementDesignLinkTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementDesignLinkTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SuperelementDesignLinkTable>().idColumn()); });
    }
    {
        using model::SuperelementDesignResp2Table;
        const std::uint16_t type = model::typeIndexOf<SuperelementDesignResp2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SuperelementDesignResp2Table>().idColumn()); });
    }
    {
        using model::SuperelementDesignResp3Table;
        const std::uint16_t type = model::typeIndexOf<SuperelementDesignResp3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SuperelementDesignResp3Table>().idColumn()); });
    }
    {
        using model::SuperelementElemTransferTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementElemTransferTable>();
        registerColumn(
            catalogue, type, "superelementId", [](const auto& model)
            { return exportColumn(model->template table<SuperelementElemTransferTable>().superelementIdColumn()); });
    }
    {
        using model::SuperelementExcludedNodesTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementExcludedNodesTable>();
        registerColumn(
            catalogue, type, "superelementIdA", [](const auto& model)
            { return exportColumn(model->template table<SuperelementExcludedNodesTable>().superelementIdAColumn()); });
    }
    {
        using model::SuperelementLabelTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementLabelTable>();
        registerColumn(
            catalogue, type, "superelementId", [](const auto& model)
            { return exportColumn(model->template table<SuperelementLabelTable>().superelementIdColumn()); });
        registerColumn(
            catalogue, type, "entryLabel", [](const auto& model)
            { return exportTextColumn(model->template table<SuperelementLabelTable>().entryLabelColumn()); });
    }
    {
        using model::SuperelementLoadScaleTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementLoadScaleTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SuperelementLoadScaleTable>().idColumn()); });
    }
    {
        using model::SuperelementLocationTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementLocationTable>();
        registerColumn(
            catalogue, type, "superelementId", [](const auto& model)
            { return exportColumn(model->template table<SuperelementLocationTable>().superelementIdColumn()); });
        registerColumn(catalogue, type, "pointsA", [](const auto& model)
                       { return exportColumn(model->template table<SuperelementLocationTable>().pointsAColumn()); });
        registerColumn(catalogue, type, "pointsB", [](const auto& model)
                       { return exportColumn(model->template table<SuperelementLocationTable>().pointsBColumn()); });
    }
    {
        using model::SuperelementMirrorPlaneTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementMirrorPlaneTable>();
        registerColumn(
            catalogue, type, "superelementId", [](const auto& model)
            { return exportColumn(model->template table<SuperelementMirrorPlaneTable>().superelementIdColumn()); });
        registerColumn(catalogue, type, "points", [](const auto& model)
                       { return exportColumn(model->template table<SuperelementMirrorPlaneTable>().pointsColumn()); });
    }
    {
        using model::SuperelementGeneralizedPointCountTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementGeneralizedPointCountTable>();
        registerColumn(catalogue, type, "superelementId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<SuperelementGeneralizedPointCountTable>().superelementIdColumn());
                       });
        registerColumn(
            catalogue, type, "count", [](const auto& model)
            { return exportColumn(model->template table<SuperelementGeneralizedPointCountTable>().countColumn()); });
    }
    {
        using model::SuperelementCopySequenceTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementCopySequenceTable>();
        registerColumn(
            catalogue, type, "superelementId", [](const auto& model)
            { return exportColumn(model->template table<SuperelementCopySequenceTable>().superelementIdColumn()); });
    }
    {
        using model::SuperelementGeneralizedDofSetTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementGeneralizedDofSetTable>();
        registerColumn(
            catalogue, type, "superelementId",
            [](const auto& model)
            {
                return exportColumn(model->template table<SuperelementGeneralizedDofSetTable>().superelementIdColumn());
            });
    }
    {
        using model::SuperelementGeneralizedDofSet1Table;
        const std::uint16_t type = model::typeIndexOf<SuperelementGeneralizedDofSet1Table>();
        registerColumn(catalogue, type, "superelementId",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<SuperelementGeneralizedDofSet1Table>().superelementIdColumn());
                       });
    }
    {
        using model::SuperelementInteriorNodesTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementInteriorNodesTable>();
        registerColumn(
            catalogue, type, "superelementId", [](const auto& model)
            { return exportColumn(model->template table<SuperelementInteriorNodesTable>().superelementIdColumn()); });
    }
    {
        using model::SuperelementSupportTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementSupportTable>();
        registerColumn(
            catalogue, type, "superelementId", [](const auto& model)
            { return exportColumn(model->template table<SuperelementSupportTable>().superelementIdColumn()); });
    }
    {
        using model::SuperelementTreeTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementTreeTable>();
        registerColumn(catalogue, type, "superelementId", [](const auto& model)
                       { return exportColumn(model->template table<SuperelementTreeTable>().superelementIdColumn()); });
    }
    {
        using model::SuperelementUserDofSetTable;
        const std::uint16_t type = model::typeIndexOf<SuperelementUserDofSetTable>();
        registerColumn(
            catalogue, type, "superelementId", [](const auto& model)
            { return exportColumn(model->template table<SuperelementUserDofSetTable>().superelementIdColumn()); });
        registerColumn(
            catalogue, type, "setName", [](const auto& model)
            { return exportTextColumn(model->template table<SuperelementUserDofSetTable>().setNameColumn()); });
    }
    {
        using model::SuperelementUserDofSet1Table;
        const std::uint16_t type = model::typeIndexOf<SuperelementUserDofSet1Table>();
        registerColumn(
            catalogue, type, "superelementId", [](const auto& model)
            { return exportColumn(model->template table<SuperelementUserDofSet1Table>().superelementIdColumn()); });
        registerColumn(
            catalogue, type, "setName", [](const auto& model)
            { return exportTextColumn(model->template table<SuperelementUserDofSet1Table>().setNameColumn()); });
    }
    {
        using model::AxisymConeSupportTable;
        const std::uint16_t type = model::typeIndexOf<AxisymConeSupportTable>();
        registerColumn(catalogue, type, "count", [](const auto& model)
                       { return exportColumn(model->template table<AxisymConeSupportTable>().countColumn()); });
    }
}

} // namespace oor::query::column_export
