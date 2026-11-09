// A model as the source of a query: the stored columns of the cards, the card type, the computed columns (only when
// used), joins that follow IDs to properties and to grid points, and a category made of several card types.

#include "QueryTestSupport.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/elements/shellTria3/ShellTria3Table.hpp"
#include "oor/model/cards/materials/isoMat/IsoMatTable.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/properties/shellProp/ShellPropTable.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/providers/ModelSource.hpp"
#include "support/ModelRecord.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using oor::testing::modelRecord;

using namespace oor;
namespace q = oor::query;
using oor::testing::readModelText;
using q::testing::runOn;

namespace
{

template <typename T>
q::StoredColumnCatalogue::Fetcher scalar(model::Column<T>& (*column)(model::Model&))
{
    return [column](model::Model& model) { return q::QueryColumn::primitiveShared<T>(column(model).share()); };
}

// The few stored columns these tests need, registered again: the catalogue already has every column of every card
// (src/query/providers/columns/), and adding a column replaces its getter.
void registerColumns()
{
    static const bool DONE = []
    {
        q::StoredColumnCatalogue& catalogue = q::StoredColumnCatalogue::instance();
        const auto node = model::typeIndexOf<model::NodeTable>();
        const auto quad = model::typeIndexOf<model::ShellQuad4Table>();
        const auto tria = model::typeIndexOf<model::ShellTria3Table>();
        const auto shell = model::typeIndexOf<model::ShellPropTable>();
        const auto mat1 = model::typeIndexOf<model::IsoMatTable>();
        catalogue.add(node, "id",
                      scalar<std::int32_t>([](model::Model& m) -> model::Column<std::int32_t>&
                                           { return m.table<model::NodeTable>().idColumn(); }));
        catalogue.add(node, "positionFrameId",
                      scalar<std::int32_t>([](model::Model& m) -> model::Column<std::int32_t>&
                                           { return m.table<model::NodeTable>().positionFrameIdColumn(); }));
        catalogue.add(
            node, "xyz", [](model::Model& m)
            { return q::QueryColumn::fixedListShared<double, 3>(m.table<model::NodeTable>().xyzColumn().share()); });
        catalogue.add(quad, "id",
                      scalar<std::int32_t>([](model::Model& m) -> model::Column<std::int32_t>&
                                           { return m.table<model::ShellQuad4Table>().idColumn(); }));
        catalogue.add(quad, "propId",
                      scalar<std::int32_t>([](model::Model& m) -> model::Column<std::int32_t>&
                                           { return m.table<model::ShellQuad4Table>().propIdColumn(); }));
        catalogue.add(quad, "nodes",
                      [](model::Model& m)
                      {
                          return q::QueryColumn::fixedListShared<std::int32_t, 4>(
                              m.table<model::ShellQuad4Table>().nodesColumn().share());
                      });
        catalogue.add(tria, "id",
                      scalar<std::int32_t>([](model::Model& m) -> model::Column<std::int32_t>&
                                           { return m.table<model::ShellTria3Table>().idColumn(); }));
        catalogue.add(tria, "propId",
                      scalar<std::int32_t>([](model::Model& m) -> model::Column<std::int32_t>&
                                           { return m.table<model::ShellTria3Table>().propIdColumn(); }));
        catalogue.add(tria, "nodes",
                      [](model::Model& m)
                      {
                          return q::QueryColumn::fixedListShared<std::int32_t, 3>(
                              m.table<model::ShellTria3Table>().nodesColumn().share());
                      });
        catalogue.add(shell, "id",
                      scalar<std::int32_t>([](model::Model& m) -> model::Column<std::int32_t>&
                                           { return m.table<model::ShellPropTable>().idColumn(); }));
        catalogue.add(shell, "membraneMatId",
                      scalar<std::int32_t>([](model::Model& m) -> model::Column<std::int32_t>&
                                           { return m.table<model::ShellPropTable>().membraneMatIdColumn(); }));
        catalogue.add(shell, "thickness",
                      scalar<double>([](model::Model& m) -> model::Column<double>&
                                     { return m.table<model::ShellPropTable>().thicknessColumn(); }));
        catalogue.add(mat1, "id",
                      scalar<std::int32_t>([](model::Model& m) -> model::Column<std::int32_t>&
                                           { return m.table<model::IsoMatTable>().idColumn(); }));
        catalogue.add(mat1, "density",
                      scalar<double>([](model::Model& m) -> model::Column<double>&
                                     { return m.table<model::IsoMatTable>().densityColumn(); }));
        return true;
    }();
    (void)DONE;
}

// A plate of two by two quadrilaterals (elements 1 to 4, property 1 of thickness 0.5, density 100) and two triangles
// (elements 10 and 11, property 2 of thickness 2).
std::shared_ptr<model::Model> plate()
{
    std::string deck = "IsoMat;7;2.1E+5;;0.3;100.\nShellProp;1;7;0.5\nShellProp;2;7;2.\n";
    for (int j = 0; j < 3; ++j)
    {
        for (int i = 0; i < 3; ++i)
            deck += modelRecord("Node", 1 + i + 3 * j, "", static_cast<double>(i), static_cast<double>(j), "0.");
    }
    deck += "Node;20;;5.;0.;0.\nNode;21;;5.;1.;0.\nNode;22;;6.;0.;0.\n";
    int eid = 1;
    for (int j = 0; j < 2; ++j)
    {
        for (int i = 0; i < 2; ++i)
        {
            const int n = 1 + i + 3 * j;
            deck += modelRecord("ShellQuad4", eid++, 1, n, n + 1, n + 4, n + 3);
        }
    }
    deck += "ShellTria3;10;2;20;22;21\nShellTria3;11;2;20;21;22\n";
    return std::make_shared<model::Model>(readModelText(deck));
}

q::Query on(const std::shared_ptr<model::Model>& model, std::uint16_t type, std::string name)
{
    return q::Query(std::make_shared<q::ModelSource>(model, std::move(name), std::vector<std::uint16_t>{type}));
}

std::vector<std::int32_t> ints(const q::QueryColumn& column)
{
    const auto values = column.as<std::int32_t>();
    return {values.begin(), values.end()};
}

} // namespace

TEST_CASE("a card type is a table of its stored columns, shared with the model", "[query][model]")
{
    registerColumns();
    const auto model = plate();
    const auto type = model::typeIndexOf<model::ShellQuad4Table>();
    q::ExecutionContext context;
    const auto table = on(model, type, "CQUAD4").run(context);
    REQUIRE(table->rowCount() == 4);
    // every stored column of the CQUAD4 (src/query/providers/columns/), then the type
    CHECK(table->columnNames() == std::vector<std::string>{"id", "propId", "nodes", "materialAngle", "matFrameId",
                                                           "offset", "thicknessFlag", "cornerThickness", "type"});
    CHECK(ints(table->column("id")) == std::vector<std::int32_t>{1, 2, 3, 4});
    CHECK(table->column("nodes").type() == q::ColumnType::FixedList);
    CHECK(table->column("nodes").width() == 4);
    CHECK(table->column("type").textAt(2) == "ShellQuad4");
    // no copy: the column of the table is the buffer of the column of the card table
    const auto buffer = model->table<model::ShellQuad4Table>().idColumn().share();
    CHECK(table->column("id").values().data() == static_cast<const void*>(buffer->data()));
    // the rows know their cards
    REQUIRE(table->origin().isPresent());
    CHECK((*table->origin().types)[0] == type);
    CHECK((*table->origin().keys).size() == 4);
    // a later edit does not change the table: the model clones the buffer
    model->table<model::ShellQuad4Table>().setPropId(0, 99);
    CHECK(ints(table->column("propId")) == std::vector<std::int32_t>{1, 1, 1, 1});
}

TEST_CASE("a computed column is evaluated only when it is used, for the rows left", "[query][model][lazy]")
{
    registerColumns();
    const auto model = plate();
    const auto type = model::typeIndexOf<model::ShellQuad4Table>();
    q::ComputationCounters& counters = q::computationCounters();
    const auto geometryBefore = counters.geometryCalls.load();
    const auto qualityBefore = counters.qualityCalls.load();
    const auto massBefore = counters.massCalls.load();
    const auto rowsBefore = counters.rowsComputed.load();

    // stored columns only: nothing is computed
    const q::Query base = on(model, type, "CQUAD4");
    q::SelectItem eid;
    eid.name = "id";
    q::SelectItem pid;
    pid.name = "propId";
    CHECK(runOn(base.select({eid, pid}), 1)->report().computedColumns.empty());
    CHECK(counters.geometryCalls.load() == geometryBefore);

    // the area, for the elements left by a filter on a stored column
    q::SelectItem area;
    area.name = "area";
    const auto table = runOn(base.where(q::Expression::parse("id >= 3")).select({eid, area}), 1);
    CHECK(table->report().computedColumns == std::vector<std::string>{"area"});
    CHECK(counters.geometryCalls.load() == geometryBefore + 1);
    CHECK(counters.rowsComputed.load() == rowsBefore + 2);
    CHECK(counters.qualityCalls.load() == qualityBefore);
    CHECK(counters.massCalls.load() == massBefore);
    CHECK(table->column("area").as<double>()[0] == Catch::Approx(1.0));

    // the three components of the centre come from one evaluation
    q::SelectItem x;
    x.name = "centerX";
    q::SelectItem y;
    y.name = "centerY";
    (void)runOn(base.select({eid, x, y}), 1);
    CHECK(counters.geometryCalls.load() == geometryBefore + 2);

    // the same column used twice (a filter and the output) is computed once, for all the rows the filter saw
    (void)runOn(base.where(q::Expression::parse("area > 0.5")).select({eid, area}), 1);
    CHECK(counters.geometryCalls.load() == geometryBefore + 3);
}

TEST_CASE("computed columns equal the analysis functions", "[query][model]")
{
    registerColumns();
    const auto model = plate();
    const auto type = model::typeIndexOf<model::ShellQuad4Table>();
    std::vector<q::SelectItem> items;
    for (const char* name :
         {"id", "area", "volume", "thickness", "centerX", "centerY", "centerZ", "normalZ", "mass", "aspectRatio",
          "skew", "edgeNeighbourCount", "nodeNeighbourCount", "propIdRow", "nodesRows"})
    {
        q::SelectItem item;
        item.name = name;
        items.push_back(item);
    }
    const auto table = runOn(on(model, type, "CQUAD4").select(items), 1);
    const std::vector<std::uint32_t> rows = {0, 1, 2, 3};
    const analysis::ElementArrays arrays =
        analysis::ElementGeometry::compute(*model, type, rows, analysis::ALL_QUANTITIES);
    for (std::size_t row = 0; row < 4; ++row)
    {
        CHECK(table->column("area").as<double>()[row] == arrays.areas[row]);
        CHECK(table->column("volume").as<double>()[row] == arrays.volumes[row]);
        CHECK(table->column("thickness").as<double>()[row] == arrays.thicknesses[row]);
        CHECK(table->column("centerX").as<double>()[row] == arrays.centers[row].x);
        CHECK(table->column("centerZ").as<double>()[row] == arrays.centers[row].z);
        CHECK(table->column("normalZ").as<double>()[row] == arrays.normals[row].z);
        CHECK(table->column("mass").as<double>()[row] == Catch::Approx(100.0 * 0.5 * 1.0));
        CHECK(table->column("aspectRatio").as<double>()[row] == Catch::Approx(1.0));
    }
    // each quadrilateral touches the three others by a corner, and two of them by an edge
    CHECK(table->column("nodeNeighbourCount").as<std::int64_t>()[0] == 3);
    CHECK(table->column("edgeNeighbourCount").as<std::int64_t>()[0] == 2);
    // the resolved rows: the first element has the grid points 1, 2, 5, 4 which are the rows 0, 1, 4, 3 of the table
    CHECK(table->column("nodesRows").type() == q::ColumnType::FixedList);
    const auto resolved = table->column("nodesRows").child()->as<std::int32_t>();
    CHECK(std::vector<std::int32_t>(resolved.begin(), resolved.begin() + 4) == std::vector<std::int32_t>{0, 1, 4, 3});
    CHECK(table->column("propIdRow").as<std::int32_t>()[0] == 0);
}

TEST_CASE("a join to the property adds its columns, and a missing property gives nulls", "[query][model][join]")
{
    registerColumns();
    const auto model = plate();
    model->table<model::ShellQuad4Table>().setPropId(3, 42); // no property 42
    const auto type = model::typeIndexOf<model::ShellQuad4Table>();
    q::JoinStep step;
    step.target = "property";
    const auto table = runOn(on(model, type, "CQUAD4").join(step), 1);
    REQUIRE(table->rowCount() == 4);
    // id is taken by the element's own column, thickness by the computed thickness of the element
    CHECK(table->has("propertyThickness"));
    CHECK(table->has("propertyId"));
    CHECK(table->column("propertyThickness").as<double>()[0] == 0.5);
    CHECK(table->column("propertyThickness").isNull(3));
    CHECK_FALSE(table->column("propertyThickness").isNull(2));
    // the material through the property
    q::JoinStep material;
    material.target = "material";
    const auto both = runOn(on(model, type, "CQUAD4").join(step).join(material), 1);
    CHECK(both->has("density"));
    CHECK(both->column("density").as<double>()[1] == 100.0);
    // an inner join drops the element whose property is missing
    step.how = q::JoinKind::Inner;
    CHECK(runOn(on(model, type, "CQUAD4").join(step), 1)->rowCount() == 3);
    // a filter on a joined column
    step.how = q::JoinKind::Left;
    const auto thick =
        runOn(on(model, type, "CQUAD4").join(step).where(q::Expression::parse("propertyThickness > 0.1")), 1);
    CHECK(thick->rowCount() == 3);
}

TEST_CASE("a join to the nodes gives a row for each node of each element, with their positions", "[query][model][join]")
{
    registerColumns();
    const auto model = plate();
    const auto type = model::typeIndexOf<model::ShellQuad4Table>();
    q::JoinStep step;
    step.target = "node";
    q::SelectItem eid;
    eid.name = "id";
    q::SelectItem position;
    position.name = "nodePosition";
    q::SelectItem nid;
    nid.name = "nodeId";
    q::SelectItem x;
    x.name = "nodeX";
    q::SelectItem y;
    y.name = "nodeY";
    const auto table = runOn(on(model, type, "CQUAD4").join(step).select({eid, position, nid, x, y}), 1);
    REQUIRE(table->rowCount() == 16);
    // the second element: grid points 2, 3, 6, 5
    CHECK(ints(table->column("nodeId")).at(4) == 2);
    CHECK(ints(table->column("nodePosition")).at(7) == 4);
    CHECK(table->column("nodeX").as<double>()[5] == 2.0);
    CHECK(table->column("nodeY").as<double>()[6] == 1.0);
    // the mean of the positions of the nodes of each element is its centre
    const auto centres = runOn(on(model, type, "CQUAD4")
                                   .join(step)
                                   .groupBy({"id"}, {{"cx", q::AggregateFunction::Mean, "nodeX", ""},
                                                     {"cy", q::AggregateFunction::Mean, "nodeY", ""}}),
                               1);
    const auto direct = runOn(on(model, type, "CQUAD4")
                                  .select({eid,
                                           []
                                           {
                                               q::SelectItem item;
                                               item.name = "centerX";
                                               return item;
                                           }()}),
                              1);
    for (std::size_t row = 0; row < 4; ++row)
        CHECK(centres->column("cx").as<double>()[row] == Catch::Approx(direct->column("centerX").as<double>()[row]));
}

TEST_CASE("a category of several card types is one table", "[query][model][category]")
{
    registerColumns();
    const auto model = plate();
    const std::vector<std::uint16_t> types = {model::typeIndexOf<model::ShellQuad4Table>(),
                                              model::typeIndexOf<model::ShellTria3Table>()};
    auto source = std::make_shared<q::ModelSource>(model, "elements", types);
    const q::Query query(source);
    const auto table = runOn(query, 1);
    REQUIRE(table->rowCount() == 6);
    CHECK(ints(table->column("id")) == std::vector<std::int32_t>{1, 2, 3, 4, 10, 11});
    // the node lists have two widths: they are lists
    CHECK(table->column("nodes").type() == q::ColumnType::List);
    CHECK(table->column("nodes").listLength(0) == 4);
    CHECK(table->column("nodes").listLength(4) == 3);
    CHECK(table->column("type").textAt(0) == "ShellQuad4");
    CHECK(table->column("type").textAt(5) == "ShellTria3");
    // computed columns are evaluated type by type and put in the order of the table
    q::SelectItem eid;
    eid.name = "id";
    q::SelectItem area;
    area.name = "area";
    q::SelectItem mass;
    mass.name = "mass";
    const auto areas = runOn(query.select({eid, area, mass}), 1);
    CHECK(areas->column("area").as<double>()[0] == Catch::Approx(1.0));
    CHECK(areas->column("area").as<double>()[4] == Catch::Approx(0.5));
    CHECK(areas->column("mass").as<double>()[4] == Catch::Approx(0.5 * 2.0 * 100.0));
    // a filter on the card type, and a sort
    const auto triangles = runOn(query.where(q::Expression::parse("type == 'ShellTria3'")).sortBy({{"id", true}}), 1);
    CHECK(ints(triangles->column("id")) == std::vector<std::int32_t>{11, 10});
    // the join to the property serves both types
    q::JoinStep step;
    step.target = "property";
    const auto joined = runOn(query.join(step), 1);
    CHECK(joined->column("propertyThickness").as<double>()[0] == 0.5);
    CHECK(joined->column("propertyThickness").as<double>()[5] == 2.0);
}

TEST_CASE("a model query gives the same table on one thread and on twelve", "[query][model][threads]")
{
    registerColumns();
    std::string deck = "IsoMat;7;2.1E+5;;0.3;100.\nShellProp;1;7;0.5\nShellProp;2;7;2.\n";
    constexpr int NX = 120;
    constexpr int NY = 90;
    for (int j = 0; j <= NY; ++j)
    {
        for (int i = 0; i <= NX; ++i)
            deck += modelRecord("Node", 1 + i + (NX + 1) * j, "", i * 0.5, j * 0.25, 0.01 * ((i * 7 + j * 3) % 5));
    }
    int eid = 1;
    for (int j = 0; j < NY; ++j)
    {
        for (int i = 0; i < NX; ++i)
        {
            const int n = 1 + i + (NX + 1) * j;
            deck += modelRecord("ShellQuad4", eid, 1 + (i + j) % 2, n, n + 1, n + NX + 2, n + NX + 1);
            ++eid;
        }
    }
    const auto model = std::make_shared<model::Model>(readModelText(deck));
    const auto type = model::typeIndexOf<model::ShellQuad4Table>();
    q::JoinStep property;
    property.target = "property";
    q::SelectItem eid1;
    eid1.name = "id";
    q::SelectItem area;
    area.name = "area";
    q::SelectItem skew;
    skew.name = "skew";
    q::SelectItem mass;
    mass.name = "mass";
    q::SelectItem edges;
    edges.name = "edgeNeighbourCount";
    q::SelectItem thickness;
    thickness.name = "thickness";
    const q::Query query = on(model, type, "CQUAD4")
                               .join(property)
                               .where(q::Expression::parse("propertyThickness > 0.4 and edgeNeighbourCount >= 3"))
                               .select({eid1, area, skew, mass, edges, thickness})
                               .sortBy({{"skew", true}, {"id", false}});
    const auto one = runOn(query, 1);
    const auto twelve = runOn(query, 12);
    CHECK(one->rowCount() > 1000);
    CHECK(q::testing::sameTable(*one, *twelve));
}
