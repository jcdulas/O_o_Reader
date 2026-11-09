#include "oor/io/zip/Zip.hpp"
#include "oor/results/core/MemoryResults.hpp"
#include "oor/results/xlsx/XlsxWriter.hpp"
#include "support/TestFiles.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <span>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::results;

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// Keeps the values of the tables of the tests alive (an input borrows them).
template <typename T>
std::span<const std::byte> kept(std::vector<T> values)
{
    static std::vector<std::shared_ptr<void>> store;
    auto held = std::make_shared<std::vector<T>>(std::move(values));
    store.push_back(held);
    return std::as_bytes(std::span<const T>(*held));
}

ResultTables sampleTables()
{
    ResultTables tables;

    MemoryTableInput transient;
    transient.kind = ResultKind::Displacements;
    transient.subcase = 2;
    transient.stepKind = StepKind::Time;
    transient.steps = {0.5, 1.0};
    transient.ids = {1, 2};
    transient.type = ValueType::Float32;
    std::vector<float> ramp(2 * 2 * 6);
    for (std::size_t index = 0; index < ramp.size(); ++index)
        ramp[index] = 0.25F * static_cast<float>(index);
    transient.values = kept(ramp);
    transient.title = "PLATE & STIFFENER";
    addTableToResults(tables, makeMemoryTable(std::move(transient)));

    MemoryTableInput stress;
    stress.kind = ResultKind::Stress;
    stress.elementName = "ShellQuad4";
    stress.steps = {NOT_A_NUMBER};
    stress.ids = {10, 10};
    stress.layers = {1, 2};
    std::vector<double> stresses(2 * 8, 12.5);
    stresses[15] = NOT_A_NUMBER; // an empty cell
    stress.values = kept(stresses);
    addTableToResults(tables, makeMemoryTable(std::move(stress)));

    MemoryTableInput complex;
    complex.kind = ResultKind::Displacements;
    complex.subcase = 3;
    complex.stepKind = StepKind::Frequency;
    complex.steps = {20.0};
    complex.ids = {7};
    complex.type = ValueType::Complex128;
    complex.values = kept(std::vector<double>{1, -1, 2, -2, 3, -3, 4, -4, 5, -5, 6, -6});
    addTableToResults(tables, makeMemoryTable(std::move(complex)));

    MemoryTableInput forces;
    forces.kind = ResultKind::NodePointForces;
    forces.steps = {NOT_A_NUMBER};
    forces.ids = {5, 5};
    forces.elementIds = {0, 10};
    forces.texts.emplace_back("elementName", std::vector<std::string>{"appliedLoad", "ShellQuad4"});
    forces.values = kept(std::vector<double>(2 * 6, 1.0));
    addTableToResults(tables, makeMemoryTable(std::move(forces)));

    auto data = std::make_shared<NamedResultData>();
    ResultArray matrix;
    matrix.name = "terms";
    matrix.type = ArrayType::Float64;
    matrix.shape = {2, 2};
    matrix.allocate();
    const auto terms = matrix.mutableValues<double>();
    terms[0] = 1.0;
    terms[1] = 2.0;
    terms[2] = 3.0;
    terms[3] = 4.0;
    data->arrays.push_back(std::move(matrix));
    NamedResultMeta meta;
    meta.kind = "matrix";
    meta.name = "KAA";
    meta.attributes = {{"form", "symmetric"}};
    tables.addNamed(std::make_shared<NamedResult>(std::move(meta), std::shared_ptr<const NamedResultData>(data)));
    return tables;
}

// The files of a workbook, by name.
std::map<std::string, std::string> partsOf(const std::string& bytes)
{
    std::map<std::string, std::string> parts;
    for (io::ZipFile& file : io::readZip(bytes))
        parts[file.name] = std::move(file.content);
    return parts;
}

bool has(const std::string& text, const std::string& part)
{
    return text.find(part) != std::string::npos;
}

} // namespace

TEST_CASE("a workbook has an index, a sheet per table and the parts Excel needs", "[results][xlsx]")
{
    XlsxWriteOptions options;
    options.title = "Wing <static>";
    ResultWriteReport report;
    const std::string bytes = formatXlsx(sampleTables(), options, &report);
    CHECK(report.finished);
    CHECK(report.skipped.empty());
    CHECK(report.written.size() == 5);

    const auto parts = partsOf(bytes);
    for (const char* part :
         {"[Content_Types].xml", "_rels/.rels", "docProps/core.xml", "docProps/app.xml", "xl/workbook.xml",
          "xl/_rels/workbook.xml.rels", "xl/styles.xml", "xl/worksheets/sheet1.xml", "xl/worksheets/sheet6.xml"})
    {
        INFO(part);
        CHECK(parts.count(part) == 1);
    }
    CHECK(parts.count("xl/worksheets/sheet7.xml") == 0); // the index and six sheets

    const std::string& workbook = parts.at("xl/workbook.xml");
    CHECK(has(workbook, "<sheet name=\"Index\" sheetId=\"1\""));
    CHECK(has(workbook, "name=\"SC1 stress ShellQuad4\""));
    CHECK(has(workbook, "name=\"matrix KAA\""));
    CHECK(has(workbook, "_xlnm._FilterDatabase"));

    // the index: the title escaped, a link to each sheet
    const std::string& index = parts.at("xl/worksheets/sheet1.xml");
    CHECK(has(index, "Wing &lt;static&gt;"));
    CHECK(has(index, "location=\"&apos;SC1 stress ShellQuad4&apos;!A1\""));
}

TEST_CASE("a table sheet has its labels, its values and a frozen header with filters", "[results][xlsx]")
{
    const auto parts = partsOf(formatXlsx(sampleTables()));
    // the sheets follow the containers: find the sheet of each table by its title
    std::map<std::string, std::string> sheets;
    for (const auto& [name, content] : parts)
        if (name.rfind("xl/worksheets/", 0) == 0 && name != "xl/worksheets/sheet1.xml") // not the index
            sheets[name] = content;
    const auto sheetWith = [&](const std::string& text) -> std::string
    {
        for (const auto& [name, content] : sheets)
            if (has(content, text))
                return content;
        return {};
    };

    const std::string transient = sheetWith("displacements, subcase 2");
    REQUIRE_FALSE(transient.empty());
    CHECK(has(transient, "PLATE &amp; STIFFENER"));
    CHECK(has(transient, ">Time<"));
    CHECK(has(transient, "state=\"frozen\""));
    CHECK(has(transient, "<autoFilter ref="));
    CHECK(has(transient, "<v>0.5</v>"));  // the first step
    CHECK(has(transient, "<v>5.75</v>")); // the last value: 23 * 0.25, a float written in its shortest form

    const std::string stress = sheetWith("stress of ShellQuad4");
    REQUIRE_FALSE(stress.empty());
    CHECK(has(stress, ">vonMises<"));
    CHECK(has(stress, ">Layer<"));
    CHECK_FALSE(has(stress, ">Time<")); // a static table has no step column

    const std::string complex = sheetWith("displacements, subcase 3");
    CHECK(has(complex, ">t1 (real)<"));
    CHECK(has(complex, ">t1 (imag)<"));
    CHECK(has(complex, "<v>-6</v>"));

    const std::string forces = sheetWith("nodePointForces");
    CHECK(has(forces, ">elementName<"));
    CHECK(has(forces, ">appliedLoad<"));

    const std::string matrix = sheetWith("matrix KAA");
    CHECK(has(matrix, ">symmetric<"));
    CHECK(has(matrix, "<v>4</v>"));
}

TEST_CASE("a table longer than a sheet goes on in the next sheets", "[results][xlsx]")
{
    MemoryTableInput input;
    input.kind = ResultKind::Displacements;
    input.steps = {NOT_A_NUMBER};
    std::vector<std::int32_t> ids(50);
    for (std::size_t index = 0; index < ids.size(); ++index)
        ids[index] = static_cast<std::int32_t>(index + 1);
    input.ids = ids;
    input.values = kept(std::vector<double>(50 * 6, 1.0));
    ResultTables tables;
    addTableToResults(tables, makeMemoryTable(std::move(input)));

    XlsxWriteOptions options;
    options.maxRowsPerSheet = 28; // 20 data rows a sheet
    const auto parts = partsOf(formatXlsx(tables, options));
    const std::string& workbook = parts.at("xl/workbook.xml");
    CHECK(has(workbook, "name=\"SC1 displacements 1\""));
    CHECK(has(workbook, "name=\"SC1 displacements 3\""));
    CHECK_FALSE(has(workbook, "name=\"SC1 displacements 4\""));
    CHECK(has(parts.at("xl/worksheets/sheet4.xml"), "(part 3 of 3)"));
    CHECK_THROWS(formatXlsx(tables, XlsxWriteOptions{{}, {}, {}, 6, 5}));
}

TEST_CASE("writeXlsx writes the file in place", "[results][xlsx]")
{
    const testing::TestFiles files;
    const auto path = files.root() / "results.xlsx";
    const ResultWriteReport report = writeXlsx(path, sampleTables());
    CHECK(report.finished);
    CHECK(report.path == path.string());
    std::ifstream stream(path, std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    CHECK(bytes.size() == report.bytes);
    CHECK(partsOf(bytes).size() == 13);
    CHECK_FALSE(std::filesystem::exists(path.string() + ".oor_tmp"));
}
