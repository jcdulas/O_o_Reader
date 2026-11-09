// The stored columns of the cards of the family "coordinate_systems", for the query engine (ColumnExport.hpp).
// One block a card type: its type index, then a getter a column.

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/coordinate_systems/cylindricalFrame1/CylindricalFrame1Table.hpp"
#include "oor/model/cards/coordinate_systems/cylindricalFrame2/CylindricalFrame2Table.hpp"
#include "oor/model/cards/coordinate_systems/generalFrame3/GeneralFrame3Table.hpp"
#include "oor/model/cards/coordinate_systems/rectFrame1/RectFrame1Table.hpp"
#include "oor/model/cards/coordinate_systems/rectFrame2/RectFrame2Table.hpp"
#include "oor/model/cards/coordinate_systems/rectFrame3/RectFrame3Table.hpp"
#include "oor/model/cards/coordinate_systems/sphericalFrame1/SphericalFrame1Table.hpp"
#include "oor/model/cards/coordinate_systems/sphericalFrame2/SphericalFrame2Table.hpp"
#include "oor/query/providers/ColumnExport.hpp"

namespace oor::query::column_export
{

void registerCoordinateSystemsColumns(StoredColumnCatalogue& catalogue)
{
    {
        using model::CylindricalFrame1Table;
        const std::uint16_t type = model::typeIndexOf<CylindricalFrame1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CylindricalFrame1Table>().idColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<CylindricalFrame1Table>().nodesColumn()); });
    }
    {
        using model::RectFrame1Table;
        const std::uint16_t type = model::typeIndexOf<RectFrame1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RectFrame1Table>().idColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<RectFrame1Table>().nodesColumn()); });
    }
    {
        using model::SphericalFrame1Table;
        const std::uint16_t type = model::typeIndexOf<SphericalFrame1Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SphericalFrame1Table>().idColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<SphericalFrame1Table>().nodesColumn()); });
    }
    {
        using model::CylindricalFrame2Table;
        const std::uint16_t type = model::typeIndexOf<CylindricalFrame2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<CylindricalFrame2Table>().idColumn()); });
        registerColumn(
            catalogue, type, "referenceFrameId", [](const auto& model)
            { return exportColumn(model->template table<CylindricalFrame2Table>().referenceFrameIdColumn()); });
        registerColumn(catalogue, type, "pointA", [](const auto& model)
                       { return exportColumn(model->template table<CylindricalFrame2Table>().pointAColumn()); });
        registerColumn(catalogue, type, "pointB", [](const auto& model)
                       { return exportColumn(model->template table<CylindricalFrame2Table>().pointBColumn()); });
        registerColumn(catalogue, type, "pointC", [](const auto& model)
                       { return exportColumn(model->template table<CylindricalFrame2Table>().pointCColumn()); });
    }
    {
        using model::RectFrame2Table;
        const std::uint16_t type = model::typeIndexOf<RectFrame2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RectFrame2Table>().idColumn()); });
        registerColumn(catalogue, type, "referenceFrameId", [](const auto& model)
                       { return exportColumn(model->template table<RectFrame2Table>().referenceFrameIdColumn()); });
        registerColumn(catalogue, type, "pointA", [](const auto& model)
                       { return exportColumn(model->template table<RectFrame2Table>().pointAColumn()); });
        registerColumn(catalogue, type, "pointB", [](const auto& model)
                       { return exportColumn(model->template table<RectFrame2Table>().pointBColumn()); });
        registerColumn(catalogue, type, "pointC", [](const auto& model)
                       { return exportColumn(model->template table<RectFrame2Table>().pointCColumn()); });
    }
    {
        using model::SphericalFrame2Table;
        const std::uint16_t type = model::typeIndexOf<SphericalFrame2Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<SphericalFrame2Table>().idColumn()); });
        registerColumn(
            catalogue, type, "referenceFrameId", [](const auto& model)
            { return exportColumn(model->template table<SphericalFrame2Table>().referenceFrameIdColumn()); });
        registerColumn(catalogue, type, "pointA", [](const auto& model)
                       { return exportColumn(model->template table<SphericalFrame2Table>().pointAColumn()); });
        registerColumn(catalogue, type, "pointB", [](const auto& model)
                       { return exportColumn(model->template table<SphericalFrame2Table>().pointBColumn()); });
        registerColumn(catalogue, type, "pointC", [](const auto& model)
                       { return exportColumn(model->template table<SphericalFrame2Table>().pointCColumn()); });
    }
    {
        using model::GeneralFrame3Table;
        const std::uint16_t type = model::typeIndexOf<GeneralFrame3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<GeneralFrame3Table>().idColumn()); });
        registerColumn(catalogue, type, "methodName", [](const auto& model)
                       { return exportTextColumn(model->template table<GeneralFrame3Table>().methodNameColumn()); });
        registerColumn(catalogue, type, "matrixForm", [](const auto& model)
                       { return exportTextColumn(model->template table<GeneralFrame3Table>().matrixFormColumn()); });
        registerColumn(catalogue, type, "angleFunctionIds", [](const auto& model)
                       { return exportColumn(model->template table<GeneralFrame3Table>().angleFunctionIdsColumn()); });
        registerColumn(catalogue, type, "referenceFrameId", [](const auto& model)
                       { return exportColumn(model->template table<GeneralFrame3Table>().referenceFrameIdColumn()); });
    }
    {
        using model::RectFrame3Table;
        const std::uint16_t type = model::typeIndexOf<RectFrame3Table>();
        registerColumn(catalogue, type, "id", [](const auto& model)
                       { return exportColumn(model->template table<RectFrame3Table>().idColumn()); });
        registerColumn(catalogue, type, "nodes", [](const auto& model)
                       { return exportColumn(model->template table<RectFrame3Table>().nodesColumn()); });
    }
}

} // namespace oor::query::column_export
