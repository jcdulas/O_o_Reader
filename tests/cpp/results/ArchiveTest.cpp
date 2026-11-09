#include "oor/base/errors/Errors.hpp"
#include "oor/io/hdf5/Hdf5Tree.hpp"
#include "oor/io/hdf5/Hdf5Writer.hpp"
#include "oor/results/archive/ArchiveFormat.hpp"
#include "oor/results/archive/ArchiveResultFile.hpp"
#include "oor/results/archive/ArchiveWriter.hpp"
#include "oor/results/core/MemoryResults.hpp"
#include "oor/results/core/ReadResults.hpp"
#include "support/TestFiles.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstring>
#include <limits>
#include <span>
#include <vector>

using namespace oor;
using namespace oor::results;

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

template <typename T>
std::span<const std::byte> bytesOf(const std::vector<T>& values)
{
    return std::as_bytes(std::span<const T>(values));
}

// Displacements of three grid points over two times of a transient analysis, in single precision.
std::shared_ptr<ResultTable> transientDisplacements()
{
    static const std::vector<float> values = []
    {
        std::vector<float> ramp(2 * 3 * 6);
        for (std::size_t index = 0; index < ramp.size(); ++index)
            ramp[index] = static_cast<float>(index) * 0.5F;
        return ramp;
    }();
    MemoryTableInput input;
    input.kind = ResultKind::Displacements;
    input.subcase = 3;
    input.stepKind = StepKind::Time;
    input.steps = {0.1, 0.2};
    input.ids = {1, 2, 3};
    input.type = ValueType::Float32;
    input.values = bytesOf(values);
    input.title = "A TRANSIENT RUN";
    return makeMemoryTable(std::move(input));
}

// The stresses of two CQUAD4 at their centre, two fibres each.
std::shared_ptr<ResultTable> shellStress()
{
    static const std::vector<double> values = []
    {
        std::vector<double> ramp(4 * 8);
        for (std::size_t index = 0; index < ramp.size(); ++index)
            ramp[index] = 10.0 + static_cast<double>(index);
        return ramp;
    }();
    MemoryTableInput input;
    input.kind = ResultKind::Stress;
    input.elementName = "ShellQuad4";
    input.subcase = 1;
    input.steps = {NOT_A_NUMBER};
    input.ids = {10, 10, 20, 20};
    input.layers = {1, 2, 1, 2};
    input.values = bytesOf(values);
    return makeMemoryTable(std::move(input));
}

// Complex displacements of two grid points at one frequency.
std::shared_ptr<ResultTable> complexDisplacements()
{
    static const std::vector<double> values = []
    {
        std::vector<double> ramp(2 * 6 * 2);
        for (std::size_t index = 0; index < ramp.size(); ++index)
            ramp[index] = static_cast<double>(index) - 5.0;
        return ramp;
    }();
    MemoryTableInput input;
    input.kind = ResultKind::Displacements;
    input.subcase = 2;
    input.stepKind = StepKind::Frequency;
    input.steps = {12.5};
    input.ids = {7, 8};
    input.type = ValueType::Complex128;
    input.values = bytesOf(values);
    return makeMemoryTable(std::move(input));
}

// Grid point forces: a text label a row (the source of the force).
std::shared_ptr<ResultTable> nodePointForces()
{
    static const std::vector<double> values(3 * 6, 1.5);
    MemoryTableInput input;
    input.kind = ResultKind::NodePointForces;
    input.subcase = 1;
    input.steps = {NOT_A_NUMBER};
    input.ids = {5, 5, 5};
    input.elementIds = {0, 10, 0};
    input.texts.emplace_back("elementName", std::vector<std::string>{"appliedLoad", "ShellQuad4", "total"});
    input.values = bytesOf(values);
    return makeMemoryTable(std::move(input));
}

// A named result: a 2 x 3 matrix and a single number.
std::shared_ptr<NamedResult> namedMatrix()
{
    auto data = std::make_shared<NamedResultData>();
    ResultArray matrix;
    matrix.name = "terms";
    matrix.description = "the terms of the matrix";
    matrix.type = ArrayType::Float64;
    matrix.shape = {2, 3};
    matrix.allocate();
    const auto terms = matrix.mutableValues<double>();
    for (std::size_t index = 0; index < terms.size(); ++index)
        terms[index] = static_cast<double>(index) * 1.25;
    data->arrays.push_back(std::move(matrix));
    ResultArray count;
    count.name = "rank";
    count.type = ArrayType::Int64;
    count.allocate();
    count.mutableValues<std::int64_t>()[0] = 2;
    data->arrays.push_back(std::move(count));
    NamedResultMeta meta;
    meta.kind = "matrix";
    meta.name = "KAA";
    meta.subcase = 1;
    meta.attributes = {{"form", "general"}, {"symmetric", "no"}};
    return std::make_shared<NamedResult>(std::move(meta), std::shared_ptr<const NamedResultData>(std::move(data)));
}

ResultTables sampleTables()
{
    ResultTables tables;
    addTableToResults(tables, transientDisplacements());
    addTableToResults(tables, shellStress());
    addTableToResults(tables, complexDisplacements());
    addTableToResults(tables, nodePointForces());
    tables.addNamed(namedMatrix());
    return tables;
}

} // namespace

TEST_CASE("an archive gives back every table, label and value it was given", "[results][archive]")
{
    const testing::TestFiles files;
    const auto path = files.root() / "run.oorh5";
    ArchiveWriteOptions options;
    options.title = "SAMPLE";
    const ResultWriteReport report = writeArchive(path, sampleTables(), options);
    CHECK(report.finished);
    CHECK(report.skipped.empty());
    CHECK(report.written.size() == 5);
    CHECK_FALSE(std::filesystem::exists(path.string() + ".oor_tmp"));

    const auto archive = ArchiveResultFile::open(path);
    CHECK(archive->format() == "archive");
    CHECK(archive->formatVersion() == "3");
    CHECK(archive->metadata().title == "SAMPLE");
    CHECK(archive->metadata().producer.rfind("oor_reader", 0) == 0);
    CHECK(archive->metadata().tableCount == 4);
    CHECK(archive->metadata().namedCount == 1);

    const ResultTables tables = archive->tables({});
    // single precision stays single precision, and the steps come from the index
    const auto& displacements = tables.container(ResultKind::Displacements).tablesOf(3);
    REQUIRE(displacements.size() == 1);
    const ResultTableMeta& meta = displacements.front()->meta();
    CHECK(meta.valueType == ValueType::Float32);
    CHECK(meta.stepKind == StepKind::Time);
    CHECK(meta.steps == std::vector<double>{0.1, 0.2});
    CHECK(meta.title == "A TRANSIENT RUN");
    CHECK(meta.origin == "archive");
    CHECK_FALSE(displacements.front()->isDecoded());
    const auto data = displacements.front()->data();
    CHECK(data->ids.toVector() == std::vector<std::int32_t>{1, 2, 3});
    CHECK(data->values<float>()[35] == 17.5F);

    const auto& stress = tables.elementContainer(ResultKind::Stress, "ShellQuad4").tablesOf(1);
    REQUIRE(stress.size() == 1);
    const auto stressData = stress.front()->data();
    CHECK(stressData->layers.toVector() == std::vector<std::int32_t>{1, 2, 1, 2});
    CHECK(stressData->components == memoryComponents(ResultKind::Stress, "ShellQuad4", false));
    CHECK(stressData->values<double>()[31] == 41.0);
    CHECK(std::isnan(stressData->steps.front()));

    const auto& complex = tables.container(ResultKind::Displacements).tablesOf(2);
    REQUIRE(complex.size() == 1);
    const auto complexData = complex.front()->data();
    CHECK(complexData->type == ValueType::Complex128);
    CHECK(complexData->values<double>()[23] == 18.0);

    const auto& forces = tables.container(ResultKind::NodePointForces).tablesOf(1);
    REQUIRE(forces.size() == 1);
    const auto forceData = forces.front()->data();
    REQUIRE(forceData->texts.size() == 1);
    CHECK(forceData->texts.front().values() == std::vector<std::string>{"appliedLoad", "ShellQuad4", "total"});
    CHECK(forceData->elementIds.toVector() == std::vector<std::int32_t>{0, 10, 0});

    REQUIRE(tables.namedCount() == 1);
    const auto& named = *tables.namedResults().front();
    CHECK(named.meta().name == "KAA");
    REQUIRE(named.meta().attribute("form") != nullptr);
    CHECK(*named.meta().attribute("form") == "general");
    const auto namedData = named.data();
    const ResultArray* terms = namedData->find("terms");
    REQUIRE(terms != nullptr);
    CHECK(terms->shape == std::vector<std::size_t>{2, 3});
    CHECK(terms->values<double>()[5] == 6.25);
    const ResultArray* rank = namedData->find("rank");
    REQUIRE(rank != nullptr);
    CHECK(rank->shape.empty());
    CHECK(rank->values<std::int64_t>()[0] == 2);
}

TEST_CASE("an archive reads only the steps, rows and components a filter keeps", "[results][archive]")
{
    const testing::TestFiles files;
    const auto path = files.root() / "run.oorh5";
    (void)writeArchive(path, sampleTables());
    const auto archive = readResults(path);
    CHECK(archive->format() == "archive");

    ResultFilter filter;
    filter.subcases = std::vector<std::int32_t>{3};
    filter.steps = std::vector<double>{0.2};
    filter.ids = std::vector<std::int32_t>{2};
    filter.components = std::vector<std::string>{"t3"};
    const ResultTables tables = ResultTables::build(archive, filter);
    CHECK(tables.tableCount() == 1);
    CHECK(tables.namedCount() == 0);
    const auto table = tables.container(ResultKind::Displacements).tablesOf(3).front();
    const auto data = table->data();
    CHECK(data->stepCount == 1);
    CHECK(data->rowCount == 1);
    CHECK(data->components == std::vector<std::string>{"t3"});
    // step 1, row 1, component 2 of the ramp: (1 * 18 + 1 * 6 + 2) * 0.5
    CHECK(data->values<float>()[0] == 13.0F);

    ResultFilter none;
    none.subcases = std::vector<std::int32_t>{99};
    CHECK_THROWS_AS(ResultTables::build(archive, none), base::EmptySelectionError);
}

TEST_CASE("an archive is plain HDF5: its index and datasets are readable with any HDF5 tool", "[results][archive]")
{
    const testing::TestFiles files;
    const auto path = files.root() / "run.oorh5";
    ArchiveWriteOptions options;
    options.compressionLevel = 6;
    (void)writeArchive(path, sampleTables(), options);
    const io::Hdf5Tree tree = io::Hdf5Tree::read(path);
    CHECK(tree.find(std::string(archive::TABLE_INDEX)) != nullptr);
    CHECK(tree.find(std::string(archive::NAMED_INDEX)) != nullptr);
    CHECK(tree.find(std::string(archive::INDEX_STEPS)) != nullptr);
    // a complex table has a last dimension of 2 (real, imaginary)
    bool complexFound = false;
    for (int number = 1; number <= 4; ++number)
    {
        const io::Hdf5Node* node = tree.find("/data/tables/00000" + std::to_string(number) + "/values");
        REQUIRE(node != nullptr);
        complexFound = complexFound || node->shape == std::vector<std::uint64_t>{1, 2, 6, 2};
    }
    CHECK(complexFound);
}

TEST_CASE("large values are chunked by step and compressed", "[results][archive]")
{
    const testing::TestFiles files;
    const auto path = files.root() / "large.oorh5";
    std::vector<double> values(4 * 500 * 6, 1.0);
    MemoryTableInput input;
    input.kind = ResultKind::Displacements;
    input.stepKind = StepKind::Time;
    input.steps = {1.0, 2.0, 3.0, 4.0};
    std::vector<std::int32_t> ids(500);
    for (std::size_t index = 0; index < ids.size(); ++index)
        ids[index] = static_cast<std::int32_t>(index + 1);
    input.ids = ids;
    input.values = bytesOf(values);
    ResultTables tables;
    addTableToResults(tables, makeMemoryTable(std::move(input)));
    (void)writeArchive(path, tables);
    const io::Hdf5Tree tree = io::Hdf5Tree::read(path);
    const io::Hdf5Node* node = tree.find("/data/tables/000001/values");
    REQUIRE(node != nullptr);
    CHECK(node->layout == "chunked");
    CHECK(node->storageBytes < values.size() * sizeof(double) / 10); // constant values compress very well
}

TEST_CASE("an HDF5 file that is not an archive is refused", "[results][archive]")
{
    const testing::TestFiles files;
    const auto path = files.root() / "other.h5";
    io::Hdf5WriteRequest request;
    std::vector<double> numbers = {1.0, 2.0};
    io::Hdf5WriteDataset dataset;
    dataset.path = "/numbers";
    dataset.array.kind = io::Hdf5ValueKind::Float64;
    dataset.array.shape = {2};
    dataset.array.data = numbers.data();
    request.datasets.push_back(dataset);
    io::writeHdf5(path, request);
    CHECK_FALSE(ArchiveResultFile::isArchive(path));
    CHECK_THROWS_AS(ArchiveResultFile::open(path), base::IoError);
    CHECK_THROWS_AS(readResults(path), base::IoError);
}

TEST_CASE("an aborted archive leaves nothing behind", "[results][archive]")
{
    const testing::TestFiles files;
    const auto path = files.root() / "aborted.oorh5";
    {
        const auto session = beginArchive(path);
        session->push(transientDisplacements());
        CHECK(std::filesystem::exists(path.string() + ".oor_tmp"));
    } // destroyed without finish
    CHECK_FALSE(std::filesystem::exists(path.string() + ".oor_tmp"));
    CHECK_FALSE(std::filesystem::exists(path));
    CHECK_THROWS_AS(beginArchive(path, ArchiveWriteOptions{10, true, false, {}, {}, {}, {}}), base::UsageError);
}
