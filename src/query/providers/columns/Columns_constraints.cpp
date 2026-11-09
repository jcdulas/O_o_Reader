// The stored columns of the cards of the family "constraints", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/constraints/autoConstrExclusion/AutoConstrExclusionTable.hpp"
#include "oor/model/cards/constraints/autoConstrExclusionList/AutoConstrExclusionListTable.hpp"
#include "oor/model/cards/constraints/cyclicAxisNodes/CyclicAxisNodesTable.hpp"
#include "oor/model/cards/constraints/cyclicBoundaryJoin/CyclicBoundaryJoinTable.hpp"
#include "oor/model/cards/constraints/cyclicSupport/CyclicSupportTable.hpp"
#include "oor/model/cards/constraints/enforcedMotionConstr/EnforcedMotionConstrTable.hpp"
#include "oor/model/cards/constraints/freeBodySupport/FreeBodySupportTable.hpp"
#include "oor/model/cards/constraints/freeBodySupportSet/FreeBodySupportSetTable.hpp"
#include "oor/model/cards/constraints/geomBoundaryCurve/GeomBoundaryCurveTable.hpp"
#include "oor/model/cards/constraints/geomSinglePointConstr/GeomSinglePointConstrTable.hpp"
#include "oor/model/cards/constraints/multiPointConstr/MultiPointConstrTable.hpp"
#include "oor/model/cards/constraints/multiPointConstrAxisym/MultiPointConstrAxisymTable.hpp"
#include "oor/model/cards/constraints/multiPointConstrUnion/MultiPointConstrUnionTable.hpp"
#include "oor/model/cards/constraints/multiPointConstrValue/MultiPointConstrValueTable.hpp"
#include "oor/model/cards/constraints/multiPointConstrWithValue/MultiPointConstrWithValueTable.hpp"
#include "oor/model/cards/constraints/relativeEnforcedMotionConstr/RelativeEnforcedMotionConstrTable.hpp"
#include "oor/model/cards/constraints/singlePointConstr/SinglePointConstrTable.hpp"
#include "oor/model/cards/constraints/singlePointConstrAxisym/SinglePointConstrAxisymTable.hpp"
#include "oor/model/cards/constraints/singlePointConstrList/SinglePointConstrListTable.hpp"
#include "oor/model/cards/constraints/singlePointConstrUnion/SinglePointConstrUnionTable.hpp"
#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

void registerConstraintsColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::CyclicAxisNodesTable;
        const std::uint16_t type = model::typeIndexOf<CyclicAxisNodesTable>();
        registerColumn(catalogue, type, "itemCount", [](const auto& model)
                       { return exportColumn(model->template table<CyclicAxisNodesTable>().itemCountColumn()); });
    }
    {
        using model::CyclicBoundaryJoinTable;
        const std::uint16_t type = model::typeIndexOf<CyclicBoundaryJoinTable>();
        registerColumn(catalogue, type, "sideCode", [](const auto& model)
                       { return exportColumn(model->template table<CyclicBoundaryJoinTable>().sideCodeColumn()); });
    }
    {
        using model::CyclicSupportTable;
        const std::uint16_t type = model::typeIndexOf<CyclicSupportTable>();
        registerColumn(catalogue, type, "nodeId", [](const auto& model)
                       { return exportColumn(model->template table<CyclicSupportTable>().nodeIdColumn()); });
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<CyclicSupportTable>().componentsColumn()); });
    }
    {
        using model::GeomBoundaryCurveTable;
        const std::uint16_t type = model::typeIndexOf<GeomBoundaryCurveTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<GeomBoundaryCurveTable>().idColumn()); });
    }
    {
        using model::GeomSinglePointConstrTable;
        const std::uint16_t type = model::typeIndexOf<GeomSinglePointConstrTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<GeomSinglePointConstrTable>().idColumn()); });
        registerColumn(
            catalogue, type, "components", [](const auto& model)
            { return exportColumn(model->template table<GeomSinglePointConstrTable>().componentsColumn()); });
        registerColumn(
            catalogue, type, "entityType", [](const auto& model)
            { return exportTextColumn(model->template table<GeomSinglePointConstrTable>().entityTypeColumn()); });
        registerColumn(catalogue, type, "entityId", [](const auto& model)
                       { return exportColumn(model->template table<GeomSinglePointConstrTable>().entityIdColumn()); });
    }
    {
        using model::MultiPointConstrTable;
        const std::uint16_t type = model::typeIndexOf<MultiPointConstrTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MultiPointConstrTable>().idColumn()); });
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportVariableColumn(model->template table<MultiPointConstrTable>().nodeColumn()); });
        registerColumn(
            catalogue, type, "component", [](const auto& model)
            { return exportVariableColumn(model->template table<MultiPointConstrTable>().componentColumn()); });
        registerColumn(
            catalogue, type, "coefficient", [](const auto& model)
            { return exportVariableColumn(model->template table<MultiPointConstrTable>().coefficientColumn()); });
    }
    {
        using model::MultiPointConstrUnionTable;
        const std::uint16_t type = model::typeIndexOf<MultiPointConstrUnionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MultiPointConstrUnionTable>().idColumn()); });
        registerColumn(
            catalogue, type, "sets", [](const auto& model)
            { return exportVariableColumn(model->template table<MultiPointConstrUnionTable>().setsColumn()); });
    }
    {
        using model::MultiPointConstrAxisymTable;
        const std::uint16_t type = model::typeIndexOf<MultiPointConstrAxisymTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MultiPointConstrAxisymTable>().idColumn()); });
        registerColumn(
            catalogue, type, "ringId", [](const auto& model)
            { return exportVariableColumn(model->template table<MultiPointConstrAxisymTable>().ringIdColumn()); });
        registerColumn(
            catalogue, type, "harmonic", [](const auto& model)
            { return exportVariableColumn(model->template table<MultiPointConstrAxisymTable>().harmonicColumn()); });
        registerColumn(
            catalogue, type, "component", [](const auto& model)
            { return exportVariableColumn(model->template table<MultiPointConstrAxisymTable>().componentColumn()); });
        registerColumn(
            catalogue, type, "coefficient", [](const auto& model)
            { return exportVariableColumn(model->template table<MultiPointConstrAxisymTable>().coefficientColumn()); });
    }
    {
        using model::MultiPointConstrValueTable;
        const std::uint16_t type = model::typeIndexOf<MultiPointConstrValueTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MultiPointConstrValueTable>().idColumn()); });
        registerColumn(catalogue, type, "dependentNodes", [](const auto& model)
                       { return exportColumn(model->template table<MultiPointConstrValueTable>().pointsColumn()); });
        registerColumn(
            catalogue, type, "dependentComponents", [](const auto& model)
            { return exportColumn(model->template table<MultiPointConstrValueTable>().componentsColumn()); });
        registerColumn(catalogue, type, "rightHandSides", [](const auto& model)
                       { return exportColumn(model->template table<MultiPointConstrValueTable>().valuesColumn()); });
    }
    {
        using model::MultiPointConstrWithValueTable;
        const std::uint16_t type = model::typeIndexOf<MultiPointConstrWithValueTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<MultiPointConstrWithValueTable>().idColumn()); });
        registerColumn(
            catalogue, type, "dependentNode", [](const auto& model)
            { return exportColumn(model->template table<MultiPointConstrWithValueTable>().dependentNodeColumn()); });
        registerColumn(catalogue, type, "dependentComponents",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<MultiPointConstrWithValueTable>().dependentComponentsColumn());
                       });
        registerColumn(catalogue, type, "dependentCoefficient",
                       [](const auto& model)
                       {
                           return exportColumn(
                               model->template table<MultiPointConstrWithValueTable>().dependentCoefficientColumn());
                       });
        registerColumn(
            catalogue, type, "rightHandSide", [](const auto& model)
            { return exportColumn(model->template table<MultiPointConstrWithValueTable>().rightHandSideColumn()); });
        registerColumn(
            catalogue, type, "node", [](const auto& model)
            { return exportVariableColumn(model->template table<MultiPointConstrWithValueTable>().nodeColumn()); });
        registerColumn(
            catalogue, type, "component",
            [](const auto& model)
            {
                return exportVariableColumn(model->template table<MultiPointConstrWithValueTable>().componentColumn());
            });
        registerColumn(catalogue, type, "coefficient",
                       [](const auto& model)
                       {
                           return exportVariableColumn(
                               model->template table<MultiPointConstrWithValueTable>().coefficientColumn());
                       });
    }
    {
        using model::SinglePointConstrTable;
        const std::uint16_t type = model::typeIndexOf<SinglePointConstrTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SinglePointConstrTable>().idColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<SinglePointConstrTable>().nodesColumn()); });
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<SinglePointConstrTable>().componentsColumn()); });
        registerColumn(catalogue, type, "enforced", [](const auto& model)
                       { return exportColumn(model->template table<SinglePointConstrTable>().enforcedColumn()); });
    }
    {
        using model::SinglePointConstrListTable;
        const std::uint16_t type = model::typeIndexOf<SinglePointConstrListTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SinglePointConstrListTable>().idColumn()); });
        registerColumn(
            catalogue, type, "components", [](const auto& model)
            { return exportColumn(model->template table<SinglePointConstrListTable>().componentsColumn()); });
        registerColumn(
            catalogue, type, "runs", [](const auto& model)
            { return exportVariableColumn(model->template table<SinglePointConstrListTable>().runsColumn()); });
    }
    {
        using model::SinglePointConstrUnionTable;
        const std::uint16_t type = model::typeIndexOf<SinglePointConstrUnionTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SinglePointConstrUnionTable>().idColumn()); });
        registerColumn(
            catalogue, type, "sets", [](const auto& model)
            { return exportVariableColumn(model->template table<SinglePointConstrUnionTable>().setsColumn()); });
    }
    {
        using model::SinglePointConstrAxisymTable;
        const std::uint16_t type = model::typeIndexOf<SinglePointConstrAxisymTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SinglePointConstrAxisymTable>().idColumn()); });
        registerColumn(catalogue, type, "ringId", [](const auto& model)
                       { return exportColumn(model->template table<SinglePointConstrAxisymTable>().ringIdColumn()); });
        registerColumn(
            catalogue, type, "harmonicId", [](const auto& model)
            { return exportColumn(model->template table<SinglePointConstrAxisymTable>().harmonicColumn()); });
        registerColumn(
            catalogue, type, "components", [](const auto& model)
            { return exportColumn(model->template table<SinglePointConstrAxisymTable>().componentsColumn()); });
        registerColumn(
            catalogue, type, "enforcedValue", [](const auto& model)
            { return exportColumn(model->template table<SinglePointConstrAxisymTable>().enforcedColumn()); });
    }
    {
        using model::EnforcedMotionConstrTable;
        const std::uint16_t type = model::typeIndexOf<EnforcedMotionConstrTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<EnforcedMotionConstrTable>().idColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<EnforcedMotionConstrTable>().nodesColumn()); });
        registerColumn(catalogue, type, "components", [](const auto& model)
                       { return exportColumn(model->template table<EnforcedMotionConstrTable>().componentsColumn()); });
        registerColumn(catalogue, type, "enforced", [](const auto& model)
                       { return exportColumn(model->template table<EnforcedMotionConstrTable>().enforcedColumn()); });
    }
    {
        using model::AutoConstrExclusionTable;
        const std::uint16_t type = model::typeIndexOf<AutoConstrExclusionTable>();
        registerColumn(
            catalogue, type, "node", [](const auto& model)
            { return exportVariableColumn(model->template table<AutoConstrExclusionTable>().nodeColumn()); });
        registerColumn(
            catalogue, type, "component", [](const auto& model)
            { return exportVariableColumn(model->template table<AutoConstrExclusionTable>().componentColumn()); });
    }
    {
        using model::AutoConstrExclusionListTable;
        const std::uint16_t type = model::typeIndexOf<AutoConstrExclusionListTable>();
        registerColumn(
            catalogue, type, "components", [](const auto& model)
            { return exportColumn(model->template table<AutoConstrExclusionListTable>().componentsColumn()); });
        registerColumn(
            catalogue, type, "runs", [](const auto& model)
            { return exportVariableColumn(model->template table<AutoConstrExclusionListTable>().runsColumn()); });
    }
    {
        using model::RelativeEnforcedMotionConstrTable;
        const std::uint16_t type = model::typeIndexOf<RelativeEnforcedMotionConstrTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RelativeEnforcedMotionConstrTable>().idColumn()); });
        registerColumn(
            catalogue, type, "nodes", [](const auto& model)
            { return exportColumn(model->template table<RelativeEnforcedMotionConstrTable>().nodesColumn()); });
        registerColumn(
            catalogue, type, "components", [](const auto& model)
            { return exportColumn(model->template table<RelativeEnforcedMotionConstrTable>().componentsColumn()); });
        registerColumn(
            catalogue, type, "enforced", [](const auto& model)
            { return exportColumn(model->template table<RelativeEnforcedMotionConstrTable>().enforcedColumn()); });
    }
    {
        using model::FreeBodySupportTable;
        const std::uint16_t type = model::typeIndexOf<FreeBodySupportTable>();
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportVariableColumn(model->template table<FreeBodySupportTable>().nodeColumn()); });
        registerColumn(
            catalogue, type, "component", [](const auto& model)
            { return exportVariableColumn(model->template table<FreeBodySupportTable>().componentColumn()); });
    }
    {
        using model::FreeBodySupportSetTable;
        const std::uint16_t type = model::typeIndexOf<FreeBodySupportSetTable>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<FreeBodySupportSetTable>().idColumn()); });
        registerColumn(catalogue, type, "node", [](const auto& model)
                       { return exportVariableColumn(model->template table<FreeBodySupportSetTable>().nodeColumn()); });
        registerColumn(
            catalogue, type, "component", [](const auto& model)
            { return exportVariableColumn(model->template table<FreeBodySupportSetTable>().componentColumn()); });
    }
}

} // namespace oor::query::column_export
