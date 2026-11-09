#include "oor/base/errors/Errors.hpp"
#include "oor/results/attach/ResultAttachment.hpp"
#include "oor/results/core/MatrixResults.hpp"
#include "oor/results/core/MemoryResults.hpp"
#include "oor/results/core/NamedResultBuilder.hpp"
#include "oor/results/core/ResultDecodeStats.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <numeric>
#include <span>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::results;
using oor::testing::readModelText;

namespace
{

std::span<const std::byte> bytesOf(const std::vector<double>& values)
{
    return std::as_bytes(std::span<const double>(values));
}

MemoryTableInput displacementsOf(const std::vector<double>& values, std::vector<std::int32_t> ids)
{
    MemoryTableInput input;
    input.kind = ResultKind::Displacements;
    input.ids = std::move(ids);
    input.steps = {std::nan("")};
    input.values = bytesOf(values);
    return input;
}

bool mentions(const std::vector<std::string>& problems, const std::string& text)
{
    return std::any_of(problems.begin(), problems.end(),
                       [&](const std::string& problem) { return problem.find(text) != std::string::npos; });
}

} // namespace

TEST_CASE("a table made from arrays holds its values and says where it comes from", "[results][memory]")
{
    std::vector<double> values(3 * 6);
    for (std::size_t index = 0; index < values.size(); ++index)
        values[index] = static_cast<double>(index) * 0.5;
    const MemoryTableInput input = displacementsOf(values, {1, 2, 3});
    REQUIRE(checkMemoryTable(input).empty());
    const std::shared_ptr<ResultTable> table = makeMemoryTable(input);
    const ResultTableMeta& meta = table->meta();
    CHECK(meta.origin == "memory");
    CHECK(meta.status == ResultStatus::NotValidated);
    CHECK(meta.kind == ResultKind::Displacements);
    CHECK(meta.analysisCode == 1);
    CHECK(meta.stepKind == StepKind::LoadCombinationSet);
    CHECK(meta.valueType == ValueType::Float64);
    CHECK(meta.wordSize == 8);
    CHECK(meta.storedFrame == StoredFrame::NodeOutputSystem);
    CHECK(meta.components == std::vector<std::string>{"t1", "t2", "t3", "r1", "r2", "r3"});
    CHECK(table->isDecoded());
    const auto data = table->data();
    CHECK(data->ids == std::vector<std::int32_t>{1, 2, 3});
    CHECK(data->pointTypes == std::vector<std::int32_t>{1, 1, 1});
    CHECK(std::equal(values.begin(), values.end(), data->values<double>().begin()));
    ResultTables tables;
    addTableToResults(tables, table);
    CHECK(tables.container(ResultKind::Displacements).tableCount() == 1);
    CHECK(tables.container(ResultKind::Displacements).tablesOf(1).front() == table);
}

TEST_CASE("the arrays of a table are checked and every problem is named", "[results][memory]")
{
    std::vector<double> values(2 * 6, 1.0);
    SECTION("an ID twice")
    {
        MemoryTableInput input = displacementsOf(values, {4, 4});
        CHECK(mentions(checkMemoryTable(input), "given more than once: 4"));
        CHECK_THROWS_AS(makeMemoryTable(input), base::UsageError);
    }
    SECTION("a component the kind does not have")
    {
        // the input holds a view of the values: they outlive it (AddressSanitizer found a temporary)
        const std::vector<double> values(2, 1.0);
        MemoryTableInput input = displacementsOf(values, {1, 2});
        input.components = {"t1"};
        CHECK(checkMemoryTable(input).empty());
        input.components = {"sxx"};
        CHECK(mentions(checkMemoryTable(input), "'sxx' is not a component of displacements"));
    }
    SECTION("values that are not steps x rows x components")
    {
        // the input holds a view of the values: they outlive it (AddressSanitizer found a temporary)
        const std::vector<double> values(7, 1.0);
        const MemoryTableInput input = displacementsOf(values, {1, 2});
        CHECK(mentions(checkMemoryTable(input), "the values hold 56 bytes where 1 steps x 2 rows x 6 components"));
    }
    SECTION("a static table has one step")
    {
        // the input holds a view of the values: they outlive it (AddressSanitizer found a temporary)
        const std::vector<double> values(2 * 6 * 2, 1.0);
        MemoryTableInput input = displacementsOf(values, {1, 2});
        input.steps = {std::nan(""), std::nan("")};
        CHECK(mentions(checkMemoryTable(input), "has one step, not 2"));
    }
    SECTION("a transient table has steps with values, once each")
    {
        // the input holds a view of the values: they outlive it (AddressSanitizer found a temporary)
        const std::vector<double> values(2 * 6 * 2, 1.0);
        MemoryTableInput input = displacementsOf(values, {1, 2});
        input.analysisCode = 6;
        input.steps = {0.1, 0.1};
        CHECK(mentions(checkMemoryTable(input), "given twice"));
        input.steps = {0.1, 0.2};
        CHECK(checkMemoryTable(input).empty());
        CHECK(makeMemoryTable(input)->meta().stepKind == StepKind::Time);
    }
    SECTION("a step kind that the analysis does not have")
    {
        MemoryTableInput input = displacementsOf(values, {1, 2});
        input.analysisCode = 2;
        input.stepKind = StepKind::Time;
        input.steps = {1.0};
        CHECK(mentions(checkMemoryTable(input), "has steps of kind mode, not time"));
    }
    SECTION("a family kind needs a known element type")
    {
        MemoryTableInput input;
        input.kind = ResultKind::Stress;
        input.ids = {1};
        input.steps = {std::nan("")};
        const std::vector<double> stress(8, 0.0);
        input.values = bytesOf(stress);
        CHECK(mentions(checkMemoryTable(input), "give elementType"));
        input.elementName = "cquad99";
        CHECK(mentions(checkMemoryTable(input), "'cquad99' is not an element type"));
        input.elementName = "ShellQuad4";
        CHECK(checkMemoryTable(input).empty());
        const auto table = makeMemoryTable(input);
        CHECK(table->data()->layers == std::vector<std::int32_t>{1});
        CHECK(table->meta().layout == "centre");
        ResultTables tables;
        addTableToResults(tables, table);
        CHECK(tables.elementContainer(ResultKind::Stress, "ShellQuad4").tableCount() == 1);
    }
}

TEST_CASE("the layouts of the tables a program may hand over are listed", "[results][memory]")
{
    const std::vector<TableLayout> layouts = memoryLayouts();
    const auto found = std::find_if(layouts.begin(), layouts.end(), [](const TableLayout& layout)
                                    { return layout.kind == ResultKind::Displacements && !layout.complex; });
    REQUIRE(found != layouts.end());
    CHECK(found->components == std::vector<std::string>{"t1", "t2", "t3", "r1", "r2", "r3"});
    CHECK(std::any_of(layouts.begin(), layouts.end(), [](const TableLayout& layout)
                      { return layout.kind == ResultKind::Displacements && layout.complex; }));
    const std::vector<std::string> names = memoryElementNames(ResultKind::Stress);
    CHECK(std::find(names.begin(), names.end(), "ShellQuad4") != names.end());
    CHECK(memoryComponents(ResultKind::Stress, "ShellQuad4", false).size() > 3);
}

TEST_CASE("a matrix result carries the labels of its degrees of freedom and is stored as DMIG", "[results][memory]")
{
    model::LabelledTerms terms;
    terms.form = 1;
    terms.columnId = {1, 1, 2};
    terms.columnComponent = {1, 1, 3};
    terms.rowId = {1, 2, 2};
    terms.rowComponent = {1, 3, 3};
    terms.real = {4.0, -1.0, 2.5};
    model::Model model;
    (void)model::addDirectMatrixNodal(model, "K", terms);
    const model::CooMatrix coo = model::directMatrixNodalCoo("K", *model::directMatrixNodalTerms(model, "K"));
    const std::shared_ptr<NamedResult> result = matrixResultOf(coo);
    CHECK(result->meta().kind == "matrix");
    CHECK(*result->meta().attribute("origin") == "model");
    CHECK(*result->meta().attribute("form") == "square");
    const auto data = result->data();
    REQUIRE(data->find("rowIds") != nullptr);
    CHECK(data->find("rowIds")->values<std::int32_t>()[1] == 2);
    CHECK(data->find("shape")->values<std::int64_t>()[0] == 2);
    // stored again: the same terms
    const model::LabelledTerms back = matrixTermsOf(*result);
    CHECK(back.form == 1);
    CHECK(back.rowId == terms.rowId);
    CHECK(back.columnId == terms.columnId);
    CHECK(back.columnComponent == terms.columnComponent);
    CHECK(back.real == terms.real);
}

TEST_CASE("a matrix result without labels cannot be a DMIG matrix", "[results][memory]")
{
    const std::vector<std::int64_t> shape = {1, 1};
    const std::vector<std::int32_t> row = {0};
    const std::vector<double> value = {1.0};
    std::vector<NamedArrayInput> arrays;
    arrays.push_back({"shape", ArrayType::Int64, {2}, std::as_bytes(std::span<const std::int64_t>(shape))});
    arrays.push_back({"row", ArrayType::Int32, {1}, std::as_bytes(std::span<const std::int32_t>(row))});
    arrays.push_back({"column", ArrayType::Int32, {1}, std::as_bytes(std::span<const std::int32_t>(row))});
    arrays.push_back({"value", ArrayType::Float64, {1}, bytesOf(value)});
    const std::vector<NamedTextInput> noTexts;
    const auto bare = makeNamedResult("matrix", NamedResultOrigin{}, arrays, noTexts);
    CHECK_THROWS_AS(matrixTermsOf(*bare), base::UsageError);
    // the labels are all four or none, as long as the rows and the columns
    const std::vector<std::int32_t> ids = {7, 8};
    arrays.push_back({"rowIds", ArrayType::Int32, {2}, std::as_bytes(std::span<const std::int32_t>(ids))});
    CHECK_THROWS_AS(makeNamedResult("matrix", NamedResultOrigin{}, arrays, noTexts), base::UsageError);
}

TEST_CASE("the sets attached to a model have labels that differ", "[results][memory]")
{
    auto model = std::make_shared<model::Model>(readModelText("Node;1;;0.;0.;0.\nNode;2;;1.;0.;0.\n"));
    const std::vector<double> values(2 * 6, 1.0);
    const auto makeSet = [&](const std::string& label)
    {
        auto tables = std::make_shared<ResultTables>();
        addTableToResults(*tables, makeMemoryTable(displacementsOf(values, {1, 2})));
        return std::pair{std::make_shared<MemorySetFile>(label), tables};
    };
    const auto [firstFile, firstTables] = makeSet("memory");
    const auto first = ResultAttachment::attach(model, firstFile, firstTables);
    CHECK(first->analysis() == "memory");
    // the default label, taken: a number is added; an explicit one that is taken is refused
    const auto [secondFile, secondTables] = makeSet("memory");
    const auto second = ResultAttachment::attach(model, secondFile, secondTables);
    CHECK(second->analysis() == "memory#2");
    const auto [thirdFile, thirdTables] = makeSet("other");
    AttachOptions explicitLabel;
    explicitLabel.analysis = "memory";
    CHECK_THROWS_AS(ResultAttachment::attach(model, thirdFile, thirdTables, explicitLabel), base::UsageError);
    explicitLabel.analysis = "third";
    const auto third = ResultAttachment::attach(model, thirdFile, thirdTables, explicitLabel);
    CHECK(third->analysis() == "third");
    CHECK(ResultAttachment::labelInUse(*model, "third"));
    CHECK_FALSE(ResultAttachment::labelInUse(*model, "third", third.get()));
    // a label is changed to a free one only
    CHECK_THROWS_AS(third->rename(*model, "memory"), base::UsageError);
    CHECK_THROWS_AS(third->rename(*model, ""), base::UsageError);
    third->rename(*model, "renamed");
    CHECK(third->analysis() == "renamed");
    // attaching the same tables again is a refresh, not a second set
    const auto again = ResultAttachment::attach(model, firstFile, firstTables);
    CHECK(again->analysis() == "memory");
    CHECK(ResultAttachment::of(*model).size() == 3);
}

TEST_CASE("the centre and the corner layouts of a CQUAD4 are told apart by the rows", "[results][memory]")
{
    const std::vector<double> centreValues(2 * 8, 1.0);
    MemoryTableInput centre;
    centre.kind = ResultKind::Stress;
    centre.elementName = "ShellQuad4";
    centre.ids = {1, 1};
    centre.layers = {1, 2};
    centre.steps = {std::nan("")};
    centre.values = bytesOf(centreValues);
    const std::shared_ptr<ResultTable> centreTable = makeMemoryTable(centre);
    CHECK(centreTable->meta().layout == "centre");

    // the centre then a corner (grid point 11) of the element, two fibres each
    const std::vector<double> cornerValues(4 * 8, 2.0);
    MemoryTableInput corner = centre;
    corner.ids = {1, 1, 1, 1};
    corner.layers = {1, 2, 1, 2};
    corner.nodeIds = {0, 0, 11, 11};
    corner.values = bytesOf(cornerValues);
    const std::shared_ptr<ResultTable> cornerTable = makeMemoryTable(corner);
    CHECK(cornerTable->meta().layout == "corner");
    CHECK(cornerTable->meta().layout != centreTable->meta().layout);

    ResultTables tables;
    MemoryTableIndex index;
    const std::vector<std::shared_ptr<ResultTable>> batch = {centreTable, cornerTable};
    addMemoryTables(tables, index, batch);
    CHECK(index.size() == 2);
    CHECK(tables.elementContainer(ResultKind::Stress, "ShellQuad4").tablesOf(1).size() == 2);
    // the same centre table again: refused by its key, nothing added
    const std::vector<std::shared_ptr<ResultTable>> again = {makeMemoryTable(centre)};
    CHECK_THROWS_AS(addMemoryTables(tables, index, again), base::UsageError);
    CHECK(tables.elementContainer(ResultKind::Stress, "ShellQuad4").tablesOf(1).size() == 2);
}

TEST_CASE("the repeated places of an element table are found in the rows of each element", "[results][memory]")
{
    // three CQUAD4 at their centre and corners, two fibres each (ten rows an element), the rows of an element together
    MemoryTableInput input;
    input.kind = ResultKind::Stress;
    input.elementName = "ShellQuad4";
    input.steps = {std::nan("")};
    for (const std::int32_t element : {7, 8, 9})
        for (const std::int32_t node : {0, 3 * element + 1, 3 * element + 2, 3 * element + 3, 3 * element + 4})
            for (const std::int32_t layer : {1, 2})
            {
                input.ids.push_back(element);
                input.nodeIds.push_back(node);
                input.layers.push_back(layer);
            }
    const std::vector<double> values(input.ids.size() * 8, 1.0);
    input.values = bytesOf(values);
    CHECK(checkMemoryTable(input).empty());

    SECTION("a place given twice in an element, and twice more in the next")
    {
        input.nodeIds[13] = 0;                 // element 8: the second fibre of its first corner is that of its centre
        input.nodeIds[16] = input.nodeIds[14]; // its third corner is its second
        input.nodeIds[17] = input.nodeIds[15];
        input.layers[25] = 1; // element 9: the two fibres of its second corner are one
        CHECK(mentions(checkMemoryTable(input), "a place of an element is given more than once: 8 (4 repeated rows)"));
        CHECK_THROWS_AS(makeMemoryTable(input), base::UsageError);
    }
    SECTION("the IDs out of order: every row sorted, the same answer")
    {
        std::swap(input.ids[0], input.ids[29]); // element 9's last row first, element 7's last
        std::swap(input.nodeIds[0], input.nodeIds[29]);
        std::swap(input.layers[0], input.layers[29]);
        CHECK(checkMemoryTable(input).empty());
        input.layers[9] = input.layers[8]; // element 7: the two fibres of its last corner are one
        CHECK(mentions(checkMemoryTable(input), "a place of an element is given more than once: 7 (1 repeated row)"));
    }
    SECTION("the stations of a beam: a run of one element compared by its positions")
    {
        input.nodeIds.assign(input.ids.size(), 0);
        input.layers.assign(input.ids.size(), 1);
        for (std::size_t row = 0; row < input.ids.size(); ++row)
            input.positions.push_back(0.1 * static_cast<double>(row % 10));
        CHECK(checkMemoryTable(input).empty());
        input.positions[12] = input.positions[11];
        CHECK(mentions(checkMemoryTable(input), "given more than once: 8 (1 repeated row)"));
    }
    SECTION("rows of one place told apart by a text")
    {
        input.nodeIds.assign(input.ids.size(), 0);
        input.layers.assign(input.ids.size(), 1);
        std::vector<std::string> fibres;
        for (std::size_t row = 0; row < input.ids.size(); ++row)
            fibres.push_back("F" + std::to_string(row % 10));
        input.texts.emplace_back("fibre", fibres);
        CHECK(checkMemoryTable(input).empty());
        fibres[21] = fibres[20];
        input.texts.front() = ResultText("fibre", fibres);
        CHECK(mentions(checkMemoryTable(input), "given more than once: 9 (1 repeated row)"));
    }
    SECTION("a long run of one element: its pairs sorted, not inserted")
    {
        MemoryTableInput plies = input;
        plies.ids.assign(60, 5);
        plies.nodeIds.assign(60, 0);
        plies.layers.resize(60);
        std::iota(plies.layers.begin(), plies.layers.end(), 1);
        const std::vector<double> plyValues(60 * 8, 1.0);
        plies.values = bytesOf(plyValues);
        CHECK(checkMemoryTable(plies).empty());
        plies.layers[59] = 3;
        CHECK(mentions(checkMemoryTable(plies), "given more than once: 5 (1 repeated row)"));
    }
}

TEST_CASE("a table keeps the block of values it is given without a copy", "[results][memory]")
{
    constexpr std::size_t COUNT = 2 * 6;
    auto released = std::make_shared<bool>(false);
    std::shared_ptr<double[]> block(new double[COUNT],
                                    [released](const double* values)
                                    {
                                        *released = true;
                                        delete[] values;
                                    });
    for (std::size_t index = 0; index < COUNT; ++index)
        block[index] = 0.5 * static_cast<double>(index);
    MemoryTableInput input;
    input.kind = ResultKind::Displacements;
    input.ids = {1, 2};
    input.steps = {std::nan("")};
    input.values = std::as_bytes(std::span<const double>(block.get(), COUNT));
    input.sharedValues = block;
    CHECK(checkMemoryTable(input).empty());

    SECTION("the table holds the block until it goes")
    {
        const std::uint64_t allocated = decodeStats().bytesAllocated.load();
        std::shared_ptr<ResultTable> table = makeMemoryTable(std::move(input));
        CHECK(table->data()->bytes() == reinterpret_cast<const std::byte*>(block.get()));
        CHECK(table->data()->values<double>()[3] == 1.5);
        CHECK(decodeStats().bytesAllocated.load() == allocated);
        block.reset();
        CHECK_FALSE(*released);
        table.reset();
        CHECK(*released);
    }
    SECTION("a block that does not start at the values is refused")
    {
        input.values = std::as_bytes(std::span<const double>(block.get() + 1, COUNT - 1));
        CHECK(mentions(checkMemoryTable(input), "does not start at the values"));
        CHECK_THROWS_AS(makeMemoryTable(input), base::UsageError);
    }
}

TEST_CASE("the key of a table holds the load set and the table name", "[results][memory]")
{
    const std::vector<double> values(6, 0.0);
    MemoryTableInput first = displacementsOf(values, {1});
    MemoryTableInput shape = first;
    shape.analysisCode = 8; // a shape of a buckling analysis: its mode number is the load set
    shape.loadSet = 2;
    MemoryTableInput basic = first;
    basic.tableName = "displacementsBasic";
    const MemoryTableKey firstKey = MemoryTableIndex::keyOf(makeMemoryTable(first)->meta());
    const MemoryTableKey shapeKey = MemoryTableIndex::keyOf(makeMemoryTable(shape)->meta());
    const MemoryTableKey basicKey = MemoryTableIndex::keyOf(makeMemoryTable(basic)->meta());
    CHECK(firstKey != shapeKey);
    CHECK(firstKey != basicKey);
    CHECK(shapeKey.loadSet == 2);
    CHECK(MemoryTableIndex::describe(shapeKey).find("load set 2") != std::string::npos);
    MemoryTableIndex index;
    CHECK(index.insert(firstKey));
    CHECK_FALSE(index.insert(firstKey));
    CHECK(index.contains(firstKey));
    CHECK_FALSE(index.contains(basicKey));
}

TEST_CASE("the components of every layout and stress code are accepted, not mixed", "[results][memory]")
{
    const std::vector<std::vector<std::string>> lists = memoryComponentLists(ResultKind::Stress, "ShellQuad4", false);
    REQUIRE(lists.size() >= 2);
    CHECK(lists.front() == memoryComponents(ResultKind::Stress, "ShellQuad4", false));
    const auto has = [&](const std::string& name)
    {
        return std::any_of(lists.begin() + 1, lists.end(), [&](const std::vector<std::string>& list)
                           { return std::find(list.begin(), list.end(), name) != list.end(); });
    };
    CHECK(has("maxShear"));
    const std::vector<double> values(2, 1.0);
    MemoryTableInput mixed;
    mixed.kind = ResultKind::Stress;
    mixed.elementName = "ShellQuad4";
    mixed.ids = {1};
    mixed.steps = {std::nan("")};
    mixed.components = {"vonMises", "maxShear"};
    mixed.values = bytesOf(values);
    CHECK(mentions(checkMemoryTable(mixed), "not those of one layout"));
    mixed.components = {"oxx", "maxShear"};
    CHECK(checkMemoryTable(mixed).empty());
    // an eigenvalue table of complex modes holds real values with its own components
    const std::vector<std::vector<std::string>> modes = memoryComponentLists(ResultKind::Eigenvalues, "", false);
    CHECK(std::any_of(modes.begin(), modes.end(), [](const std::vector<std::string>& list)
                      { return std::find(list.begin(), list.end(), "eigenvalueImag") != list.end(); }));
    // the mechanism shapes have a layout of their own, the components of a nodal kind
    const std::vector<TableLayout> layouts = memoryLayouts();
    const auto mechanism = std::find_if(layouts.begin(), layouts.end(), [](const TableLayout& layout)
                                        { return layout.kind == ResultKind::MechanismShapes; });
    REQUIRE(mechanism != layouts.end());
    CHECK(mechanism->components == std::vector<std::string>{"t1", "t2", "t3", "r1", "r2", "r3"});
}
