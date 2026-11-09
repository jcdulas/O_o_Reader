#include "oor/results/xlsx/XlsxWriter.hpp"

#include "oor/base/Version.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/Json.hpp"
#include "oor/io/zip/Zip.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <fstream>
#include <set>
#include <system_error>

// An .xlsx file is a ZIP of XML files (Office Open XML, SpreadsheetML). The parts written here, the smallest set Excel,
// LibreOffice and the common libraries open:
//
//   [Content_Types].xml          the type of every part
//   _rels/.rels                  where the workbook and the document properties are
//   docProps/core.xml, app.xml   title, author program, date
//   xl/workbook.xml              the sheets, in order, and the ranges of their filters
//   xl/_rels/workbook.xml.rels   where each sheet and the styles are
//   xl/styles.xml                four cell styles: plain, header (bold), title (bold, larger), link (blue, underlined)
//   xl/worksheets/sheetN.xml     the cells of sheet N; texts are written in the cells ("inline strings")

namespace oor::results
{

namespace
{

constexpr int STYLE_PLAIN = 0;
constexpr int STYLE_HEADER = 1;
constexpr int STYLE_TITLE = 2;
constexpr int STYLE_LINK = 3;
constexpr std::size_t SHEET_NAME_LIMIT = 31; // Excel refuses longer sheet names

constexpr const char* XML_DECLARATION = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
constexpr const char* MAIN_NAMESPACE = "http://schemas.openxmlformats.org/spreadsheetml/2006/main";
constexpr const char* RELATIONSHIP_NAMESPACE = "http://schemas.openxmlformats.org/officeDocument/2006/relationships";

// A text for XML content or attributes: the five characters XML gives a meaning to are escaped, and the control
// characters XML 1.0 does not allow are dropped.
std::string xmlEscaped(std::string_view text)
{
    std::string escaped;
    escaped.reserve(text.size());
    for (const char character : text)
    {
        switch (character)
        {
        case '&':
            escaped += "&amp;";
            break;
        case '<':
            escaped += "&lt;";
            break;
        case '>':
            escaped += "&gt;";
            break;
        case '"':
            escaped += "&quot;";
            break;
        case '\'':
            escaped += "&apos;";
            break;
        default:
            if (static_cast<unsigned char>(character) >= 0x20 || character == '\t' || character == '\n')
                escaped += character;
        }
    }
    return escaped;
}

// The letters of a column, from 0: A, B, ..., Z, AA, AB, ...
std::string columnLetters(std::size_t column)
{
    std::string letters;
    ++column;
    while (column > 0)
    {
        const std::size_t remainder = (column - 1) % 26;
        letters.insert(letters.begin(), static_cast<char>('A' + remainder));
        column = (column - 1) / 26;
    }
    return letters;
}

std::string cellReference(std::size_t column, std::size_t row)
{
    return columnLetters(column) + std::to_string(row);
}

std::string currentTimeUtc()
{
    const std::time_t now = std::time(nullptr);
    std::tm parts{};
#if defined(_WIN32)
    gmtime_s(&parts, &now);
#else
    gmtime_r(&now, &parts);
#endif
    char text[32];
    std::strftime(text, sizeof(text), "%Y-%m-%dT%H:%M:%SZ", &parts);
    return text;
}

// The cells of one sheet, line by line. Rows are numbered from 1, columns from 0, as Excel shows them.
class SheetBuilder
{
public:
    explicit SheetBuilder(std::string name) : m_name(std::move(name)) {}

    [[nodiscard]] const std::string& name() const noexcept { return m_name; }
    [[nodiscard]] std::size_t rowCount() const noexcept { return m_row; }

    // Starts the next row; the cells added go to it.
    void newRow()
    {
        closeRow();
        ++m_row;
        m_column = 0;
        m_rows += "<row r=\"" + std::to_string(m_row) + "\">";
        m_rowOpen = true;
    }

    void text(std::string_view value, int style = STYLE_PLAIN)
    {
        if (!value.empty())
        {
            m_rows += "<c r=\"" + cellReference(m_column, m_row) + "\" t=\"inlineStr\"" + styleAttribute(style) +
                      "><is><t xml:space=\"preserve\">" + xmlEscaped(value) + "</t></is></c>";
            noteWidth(value.size());
        }
        ++m_column;
    }

    // A number; NaN leaves the cell empty. `singlePrecision`: the value was a float, written in its shortest form.
    void number(double value, bool singlePrecision = false)
    {
        if (std::isfinite(value))
        {
            std::string written;
            base::appendJsonNumber(written, value, singlePrecision);
            m_rows += "<c r=\"" + cellReference(m_column, m_row) + "\"><v>" + written + "</v></c>";
            noteWidth(written.size());
        }
        ++m_column;
    }

    // A text that is a link to cell A1 of another sheet of the workbook.
    void link(std::string_view value, const std::string& targetSheet)
    {
        m_links.push_back("<hyperlink ref=\"" + cellReference(m_column, m_row) + "\" location=\"" +
                          xmlEscaped(quotedSheet(targetSheet) + "!A1") + "\" display=\"" + xmlEscaped(value) + "\"/>");
        text(value, STYLE_LINK);
    }

    void skipCell() { ++m_column; }

    // The current row is the header of a table: frozen above the data, with filters over `columns` columns down to the
    // last row written.
    void markHeader(std::size_t columns)
    {
        m_headerRow = m_row;
        m_headerColumns = columns;
    }

    // The range of the filters ("'Name'!$A$5:$H$100"), empty when the sheet has none.
    [[nodiscard]] std::string filterRange() const
    {
        if (m_headerRow == 0 || m_headerColumns == 0)
            return {};
        return quotedSheet(m_name) + "!$A$" + std::to_string(m_headerRow) + ":$" + columnLetters(m_headerColumns - 1) +
               "$" + std::to_string(std::max(m_row, m_headerRow));
    }

    [[nodiscard]] std::string xml()
    {
        closeRow();
        std::string out = XML_DECLARATION;
        out += std::string("<worksheet xmlns=\"") + MAIN_NAMESPACE + "\" xmlns:r=\"" + RELATIONSHIP_NAMESPACE + "\">";
        if (m_headerRow > 0)
            out += "<sheetViews><sheetView workbookViewId=\"0\"><pane ySplit=\"" + std::to_string(m_headerRow) +
                   "\" topLeftCell=\"A" + std::to_string(m_headerRow + 1) +
                   "\" activePane=\"bottomLeft\" state=\"frozen\"/></sheetView></sheetViews>";
        if (!m_widths.empty())
        {
            out += "<cols>";
            for (std::size_t column = 0; column < m_widths.size(); ++column)
            {
                const double width = std::min(60.0, std::max(9.0, static_cast<double>(m_widths[column]) + 2.0));
                out += "<col min=\"" + std::to_string(column + 1) + "\" max=\"" + std::to_string(column + 1) +
                       "\" width=\"" + std::to_string(width) + "\" customWidth=\"1\"/>";
            }
            out += "</cols>";
        }
        out += "<sheetData>" + m_rows + "</sheetData>";
        if (m_headerRow > 0 && m_headerColumns > 0)
            out += "<autoFilter ref=\"A" + std::to_string(m_headerRow) + ":" + columnLetters(m_headerColumns - 1) +
                   std::to_string(std::max(m_row, m_headerRow)) + "\"/>";
        if (!m_links.empty())
        {
            out += "<hyperlinks>";
            for (const std::string& link : m_links)
                out += link;
            out += "</hyperlinks>";
        }
        out += "</worksheet>";
        return out;
    }

    // A sheet name as a formula writes it: between quotes, a quote doubled.
    static std::string quotedSheet(const std::string& name)
    {
        std::string quoted = "'";
        for (const char character : name)
            quoted += character == '\'' ? std::string("''") : std::string(1, character);
        return quoted + "'";
    }

private:
    void closeRow()
    {
        if (m_rowOpen)
            m_rows += "</row>";
        m_rowOpen = false;
    }

    static std::string styleAttribute(int style)
    {
        return style == STYLE_PLAIN ? std::string() : " s=\"" + std::to_string(style) + "\"";
    }

    // The widest content of each column (in characters), for the column widths; the first rows are enough.
    void noteWidth(std::size_t characters)
    {
        if (m_row > 200)
            return;
        if (m_widths.size() <= m_column)
            m_widths.resize(m_column + 1, 0);
        m_widths[m_column] = std::max(m_widths[m_column], characters);
    }

    std::string m_name;
    std::string m_rows;
    std::vector<std::string> m_links;
    std::vector<std::size_t> m_widths;
    std::size_t m_row = 0;
    std::size_t m_column = 0;
    std::size_t m_headerRow = 0;
    std::size_t m_headerColumns = 0;
    bool m_rowOpen = false;
};

// The names of the sheets: Excel wants them unique (ignoring case), at most 31 characters, without : \ / ? * [ ].
class SheetNames
{
public:
    std::string make(const std::string& wanted)
    {
        std::string base;
        for (const char character : wanted)
            base += std::string_view(":\\/?*[]").find(character) == std::string_view::npos ? character : '_';
        if (base.empty())
            base = "Sheet";
        for (int number = 1;; ++number)
        {
            const std::string suffix = number == 1 ? std::string() : " (" + std::to_string(number) + ")";
            const std::string name = base.substr(0, SHEET_NAME_LIMIT - suffix.size()) + suffix;
            std::string key;
            for (const char character : name)
                key += static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
            if (m_taken.insert(key).second)
                return name;
        }
    }

private:
    std::set<std::string> m_taken;
};

// What the index lists about a sheet.
struct IndexLine
{
    std::string sheet;
    std::string kind;
    std::string elementType;
    std::string variant;
    std::int32_t subcase = 0;
    std::string analysis;
    std::string stepKind;
    std::size_t steps = 0;
    std::size_t rows = 0;
    std::size_t components = 0;
    std::string valueType;
    std::string tableName;
};

// ---------------------------------------------------------------------------------------------------------------
// The sheets of a result table.

// The columns that label a row of a table, in the order they are written.
struct RowLabels
{
    bool step = false;
    bool eigenvalue = false;
    bool cycles = false;
    std::string idTitle;
    bool pointType = false;
    bool nodeId = false;
    bool elementId = false;
    bool layer = false;
    bool position = false;
    std::vector<const ResultText*> texts;

    [[nodiscard]] std::size_t count() const
    {
        return (step ? 1 : 0) + (eigenvalue ? 1 : 0) + (cycles ? 1 : 0) + 1 + (pointType ? 1 : 0) + (nodeId ? 1 : 0) +
               (elementId ? 1 : 0) + (layer ? 1 : 0) + (position ? 1 : 0) + texts.size();
    }
};

RowLabels rowLabelsOf(const ResultTableMeta& meta, const ResultData& data)
{
    RowLabels labels;
    const bool valuedSteps = meta.stepKind != StepKind::None && meta.stepKind != StepKind::LoadCombinationSet;
    labels.step = valuedSteps || data.stepCount > 1;
    labels.eigenvalue = !data.eigenvalues.empty();
    labels.cycles = !data.cycles.empty();
    switch (resultKindInfo(meta.kind).rows)
    {
    case RowKind::GridPoint:
        labels.idTitle = "Grid";
        break;
    case RowKind::Element:
        labels.idTitle = "Element";
        break;
    case RowKind::GridPointElement:
        labels.idTitle = "Grid";
        break;
    case RowKind::Mode:
        labels.idTitle = "Mode";
        break;
    }
    labels.pointType = !data.pointTypes.empty();
    labels.nodeId = !data.nodeIds.empty();
    labels.elementId = !data.elementIds.empty();
    labels.layer = data.isElementData() &&
                   std::any_of(data.layers.begin(), data.layers.end(), [](std::int32_t layer) { return layer != 1; });
    labels.position = !data.positions.empty();
    for (const ResultText& text : data.texts)
        labels.texts.push_back(&text);
    return labels;
}

std::string stepTitle(StepKind kind)
{
    switch (kind)
    {
    case StepKind::Mode:
        return "Mode";
    case StepKind::Frequency:
        return "Frequency";
    case StepKind::Time:
        return "Time";
    case StepKind::LoadFactor:
        return "Load factor";
    default:
        return "Step";
    }
}

std::string pointTypeText(std::int32_t type)
{
    switch (type)
    {
    case 1:
        return "G";
    case 2:
        return "S";
    case 3:
        return "E";
    default:
        return std::to_string(type);
    }
}

std::string sheetTitleOf(const ResultTableMeta& meta)
{
    std::string title = "SC" + std::to_string(meta.subcase) + " ";
    if (meta.variant != ResultVariant::Plain)
        title += std::string(resultVariantName(meta.variant)) + " ";
    title += std::string(resultKindName(meta.kind));
    const std::string container = meta.elementName.empty() ? meta.subName : meta.elementName;
    if (!container.empty())
        title += " " + container;
    return title;
}

// The lines at the top of a table sheet: what the table is, a link back to the index, then the header line.
void writeTableHeader(SheetBuilder& sheet, const ResultTableMeta& meta, const ResultData& data, const RowLabels& labels,
                      std::size_t part, std::size_t parts)
{
    sheet.newRow();
    std::string title = std::string(resultKindName(meta.kind));
    const std::string container = meta.elementName.empty() ? meta.subName : meta.elementName;
    if (!container.empty())
        title += " of " + container;
    title += ", subcase " + std::to_string(meta.subcase);
    if (parts > 1)
        title += " (part " + std::to_string(part) + " of " + std::to_string(parts) + ")";
    sheet.text(title, STYLE_TITLE);

    sheet.newRow();
    std::string facts = "Analysis: " + std::string(analysisTypeName(meta.analysisCode)) +
                        "   Steps: " + std::to_string(data.stepCount) + "   Rows: " + std::to_string(data.rowCount) +
                        "   Values: " + std::string(valueTypeName(data.type));
    if (meta.variant != ResultVariant::Plain)
        facts += "   Variant: " + std::string(resultVariantName(meta.variant));
    if (!meta.layout.empty())
        facts += "   Layout: " + meta.layout;
    if (meta.units != "unknown" && !meta.units.empty())
        facts += "   Units: " + meta.units;
    sheet.text(facts);

    if (!meta.title.empty() || !meta.subtitle.empty() || !meta.label.empty())
    {
        sheet.newRow();
        sheet.text(meta.title);
        sheet.text(meta.subtitle);
        sheet.text(meta.label);
    }
    sheet.newRow();
    sheet.link("Back to the index", "Index");

    sheet.newRow(); // the header line
    if (labels.step)
        sheet.text(stepTitle(meta.stepKind), STYLE_HEADER);
    if (labels.eigenvalue)
        sheet.text("Eigenvalue", STYLE_HEADER);
    if (labels.cycles)
        sheet.text("Cycles", STYLE_HEADER);
    sheet.text(labels.idTitle, STYLE_HEADER);
    if (labels.pointType)
        sheet.text("Type", STYLE_HEADER);
    if (labels.nodeId)
        sheet.text("Grid", STYLE_HEADER);
    if (labels.elementId)
        sheet.text("Element", STYLE_HEADER);
    if (labels.layer)
        sheet.text("Layer", STYLE_HEADER);
    if (labels.position)
        sheet.text("Position", STYLE_HEADER);
    for (const ResultText* text : labels.texts)
        sheet.text(text->name, STYLE_HEADER);
    const bool complex = isComplexType(data.type);
    for (const std::string& component : data.components)
    {
        if (complex)
        {
            sheet.text(component + " (real)", STYLE_HEADER);
            sheet.text(component + " (imag)", STYLE_HEADER);
        }
        else
        {
            sheet.text(component, STYLE_HEADER);
        }
    }
    sheet.markHeader(labels.count() + data.componentCount * (complex ? 2 : 1));
}

// One data line: the labels of the row, then its values.
void writeTableRow(SheetBuilder& sheet, const ResultData& data, const RowLabels& labels, std::size_t step,
                   std::size_t row)
{
    sheet.newRow();
    if (labels.step)
        sheet.number(data.steps[step]);
    if (labels.eigenvalue)
        sheet.number(data.eigenvalues[step]);
    if (labels.cycles)
        sheet.number(data.cycles[step]);
    sheet.number(data.ids[row]);
    if (labels.pointType)
        sheet.text(pointTypeText(data.pointTypes[row]));
    if (labels.nodeId)
        sheet.number(data.nodeIds[row]);
    if (labels.elementId)
        sheet.number(data.elementIds[row]);
    if (labels.layer)
        sheet.number(data.layers[row]);
    if (labels.position)
        sheet.number(data.positions[row]);
    for (const ResultText* text : labels.texts)
        sheet.text(text->at(row));

    const bool complex = isComplexType(data.type);
    const bool single = data.type == ValueType::Float32 || data.type == ValueType::Complex64;
    const std::size_t width = data.componentCount * (complex ? 2 : 1);
    const std::size_t first = (step * data.rowCount + row) * width;
    if (single)
    {
        const auto values = data.values<float>();
        for (std::size_t index = 0; index < width; ++index)
            sheet.number(values[first + index], true);
    }
    else
    {
        const auto values = data.values<double>();
        for (std::size_t index = 0; index < width; ++index)
            sheet.number(values[first + index]);
    }
}

// ---------------------------------------------------------------------------------------------------------------
// The workbook.

class WorkbookBuilder
{
public:
    explicit WorkbookBuilder(const XlsxWriteOptions& options, ResultWriteReport& report)
        : m_options(options), m_report(report)
    {
        if (options.maxRowsPerSheet < 20)
            throw base::UsageError("xlsx: maxRowsPerSheet is " + std::to_string(options.maxRowsPerSheet) +
                                   "; a sheet needs room for its header and some rows (at least 20)");
        m_names.make("Index"); // reserved for the index sheet
    }

    void addTable(const ResultTable& table)
    {
        const ResultTableMeta& meta = table.meta();
        const std::shared_ptr<const ResultData> data = table.data();
        const std::string container = meta.elementName.empty() ? meta.subName : meta.elementName;
        if (data->stepCount == 0 || data->rowCount == 0 || data->componentCount == 0)
        {
            m_report.skipped.push_back(SkippedTable{std::string(resultKindName(meta.kind)), container, meta.subcase,
                                                    std::string(write_reasons::NO_FILE_LAYOUT),
                                                    "an empty table (no step, row or component) has nothing to write"});
            return;
        }
        const RowLabels labels = rowLabelsOf(meta, *data);
        const std::size_t headerRows = 8; // the most rows the header block takes (writeTableHeader)
        const std::size_t perSheet = m_options.maxRowsPerSheet - headerRows;
        const std::size_t total = data->stepCount * data->rowCount;
        const std::size_t parts = (total + perSheet - 1) / perSheet;
        const std::string title = sheetTitleOf(meta);
        std::string firstSheet;
        for (std::size_t part = 0; part < parts; ++part)
        {
            SheetBuilder sheet(m_names.make(parts == 1 ? title : title + " " + std::to_string(part + 1)));
            if (part == 0)
                firstSheet = sheet.name();
            writeTableHeader(sheet, meta, *data, labels, part + 1, parts);
            const std::size_t end = std::min(total, (part + 1) * perSheet);
            for (std::size_t line = part * perSheet; line < end; ++line)
                writeTableRow(sheet, *data, labels, line / data->rowCount, line % data->rowCount);
            IndexLine index;
            index.sheet = sheet.name();
            index.kind = std::string(resultKindName(meta.kind));
            index.elementType = container;
            index.variant = std::string(resultVariantName(meta.variant));
            index.subcase = meta.subcase;
            index.analysis = std::string(analysisTypeName(meta.analysisCode));
            index.stepKind = std::string(stepKindName(meta.stepKind));
            index.steps = data->stepCount;
            index.rows = data->rowCount;
            index.components = data->componentCount;
            index.valueType = std::string(valueTypeName(data->type));
            index.tableName = meta.tableName;
            m_index.push_back(std::move(index));
            m_sheets.push_back(std::move(sheet));
        }
        WrittenTable written;
        written.kind = std::string(resultKindName(meta.kind));
        written.container = container;
        written.subcase = meta.subcase;
        written.fileTable = firstSheet;
        written.steps = data->stepCount;
        written.rows = data->rowCount;
        m_report.written.push_back(std::move(written));
    }

    void addNamed(const NamedResult& named)
    {
        const NamedResultMeta& meta = named.meta();
        const std::shared_ptr<const NamedResultData> data = named.data();
        SheetBuilder sheet(m_names.make(meta.kind + " " + meta.name));
        sheet.newRow();
        sheet.text(meta.kind + " " + meta.name + ", subcase " + std::to_string(meta.subcase), STYLE_TITLE);
        sheet.newRow();
        sheet.link("Back to the index", "Index");
        for (const auto& [key, value] : meta.attributes)
        {
            sheet.newRow();
            sheet.text(key, STYLE_HEADER);
            sheet.text(value);
        }
        for (const ResultArray& array : data->arrays)
            writeArray(sheet, array);
        for (const ResultText& text : data->texts)
        {
            sheet.newRow();
            sheet.newRow();
            sheet.text(text.name, STYLE_HEADER);
            for (std::size_t row = 0; row < text.size(); ++row)
            {
                sheet.newRow();
                sheet.text(text.at(row));
            }
        }
        IndexLine index;
        index.sheet = sheet.name();
        index.kind = meta.kind;
        index.elementType = meta.name;
        index.subcase = meta.subcase;
        index.analysis = std::string(analysisTypeName(meta.analysisCode));
        index.tableName = meta.tableName;
        m_index.push_back(std::move(index));

        WrittenTable written;
        written.kind = meta.kind;
        written.container = meta.name;
        written.subcase = meta.subcase;
        written.fileTable = sheet.name();
        m_report.written.push_back(std::move(written));
        m_sheets.push_back(std::move(sheet));
    }

    // The bytes of the .xlsx file.
    [[nodiscard]] std::string finish()
    {
        SheetBuilder index = indexSheet();
        std::vector<SheetBuilder*> ordered;
        ordered.push_back(&index);
        for (SheetBuilder& sheet : m_sheets)
            ordered.push_back(&sheet);

        io::ZipWriter zip(m_options.compressionLevel);
        zip.addFile("[Content_Types].xml", contentTypes(ordered.size()));
        zip.addFile("_rels/.rels", packageRelationships());
        zip.addFile("docProps/core.xml", coreProperties());
        zip.addFile("docProps/app.xml", appProperties());
        zip.addFile("xl/workbook.xml", workbook(ordered));
        zip.addFile("xl/_rels/workbook.xml.rels", workbookRelationships(ordered.size()));
        zip.addFile("xl/styles.xml", styles());
        for (std::size_t number = 0; number < ordered.size(); ++number)
            zip.addFile("xl/worksheets/sheet" + std::to_string(number + 1) + ".xml", ordered[number]->xml());
        return zip.finish();
    }

private:
    // An array of a named result: a grid for a two-dimensional one, a column for the others.
    static void writeArray(SheetBuilder& sheet, const ResultArray& array)
    {
        sheet.newRow();
        sheet.newRow();
        std::string shape;
        for (const std::size_t extent : array.shape)
            shape += (shape.empty() ? "" : " x ") + std::to_string(extent);
        sheet.text(array.name, STYLE_HEADER);
        sheet.text(shape.empty() ? std::string("a number") : shape);
        sheet.text(array.description);

        const bool complex = array.type == ArrayType::Complex64 || array.type == ArrayType::Complex128;
        const std::size_t parts = complex ? 2 : 1;
        const std::size_t count = array.valueCount();
        const auto valueAt = [&](std::size_t index) -> double
        {
            switch (array.type)
            {
            case ArrayType::Int32:
                return array.values<std::int32_t>()[index];
            case ArrayType::Int64:
                return static_cast<double>(array.values<std::int64_t>()[index]);
            case ArrayType::Float32:
            case ArrayType::Complex64:
                return array.values<float>()[index];
            case ArrayType::Float64:
            case ArrayType::Complex128:
                return array.values<double>()[index];
            }
            return 0.0;
        };
        const std::size_t columns = array.shape.size() == 2 ? array.shape[1] : 1;
        const std::size_t rows = columns == 0 ? 0 : count / columns;
        for (std::size_t row = 0; row < rows; ++row)
        {
            sheet.newRow();
            for (std::size_t column = 0; column < columns; ++column)
                for (std::size_t part = 0; part < parts; ++part)
                    sheet.number(valueAt((row * columns + column) * parts + part));
        }
    }

    SheetBuilder indexSheet()
    {
        SheetBuilder sheet("Index");
        sheet.newRow();
        sheet.text(m_options.title.empty() ? std::string("Results") : m_options.title, STYLE_TITLE);
        if (!m_options.subtitle.empty())
        {
            sheet.newRow();
            sheet.text(m_options.subtitle);
        }
        if (!m_options.label.empty())
        {
            sheet.newRow();
            sheet.text(m_options.label);
        }
        sheet.newRow();
        sheet.text("Written by " + std::string(base::LIBRARY_NAME) + " " + std::string(base::LIBRARY_VERSION) + " on " +
                   currentTimeUtc());
        sheet.newRow();
        sheet.newRow();
        for (const char* title : {"Sheet", "Kind", "Element type", "Variant", "Subcase", "Analysis", "Step kind",
                                  "Steps", "Rows", "Components", "Values", "Table name"})
            sheet.text(title, STYLE_HEADER);
        sheet.markHeader(12);
        for (const IndexLine& line : m_index)
        {
            sheet.newRow();
            sheet.link(line.sheet, line.sheet);
            sheet.text(line.kind);
            sheet.text(line.elementType);
            sheet.text(line.variant);
            sheet.number(line.subcase);
            sheet.text(line.analysis);
            sheet.text(line.stepKind);
            if (line.steps > 0)
            {
                sheet.number(static_cast<double>(line.steps));
                sheet.number(static_cast<double>(line.rows));
                sheet.number(static_cast<double>(line.components));
            }
            else
            {
                sheet.skipCell();
                sheet.skipCell();
                sheet.skipCell();
            }
            sheet.text(line.valueType);
            sheet.text(line.tableName);
        }
        return sheet;
    }

    static std::string contentTypes(std::size_t sheets)
    {
        std::string xml = XML_DECLARATION;
        xml +=
            "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
            "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
            "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
            "<Override PartName=\"/xl/workbook.xml\" "
            "ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
            "<Override PartName=\"/xl/styles.xml\" "
            "ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>"
            "<Override PartName=\"/docProps/core.xml\" "
            "ContentType=\"application/vnd.openxmlformats-package.core-properties+xml\"/>"
            "<Override PartName=\"/docProps/app.xml\" "
            "ContentType=\"application/vnd.openxmlformats-officedocument.extended-properties+xml\"/>";
        for (std::size_t number = 1; number <= sheets; ++number)
            xml += "<Override PartName=\"/xl/worksheets/sheet" + std::to_string(number) +
                   ".xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>";
        return xml + "</Types>";
    }

    static std::string packageRelationships()
    {
        return std::string(XML_DECLARATION) +
               "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
               "<Relationship Id=\"rId1\" Type=\"" +
               RELATIONSHIP_NAMESPACE +
               "/officeDocument\" Target=\"xl/workbook.xml\"/>"
               "<Relationship Id=\"rId2\" "
               "Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" "
               "Target=\"docProps/core.xml\"/>"
               "<Relationship Id=\"rId3\" Type=\"" +
               RELATIONSHIP_NAMESPACE +
               "/extended-properties\" Target=\"docProps/app.xml\"/>"
               "</Relationships>";
    }

    std::string coreProperties() const
    {
        const std::string producer = std::string(base::LIBRARY_NAME) + " " + std::string(base::LIBRARY_VERSION);
        return std::string(XML_DECLARATION) +
               "<cp:coreProperties "
               "xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" "
               "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:dcterms=\"http://purl.org/dc/terms/\" "
               "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">"
               "<dc:title>" +
               xmlEscaped(m_options.title.empty() ? std::string("Results") : m_options.title) +
               "</dc:title><dc:creator>" + xmlEscaped(producer) +
               "</dc:creator>"
               "<dcterms:created xsi:type=\"dcterms:W3CDTF\">" +
               currentTimeUtc() +
               "</dcterms:created>"
               "</cp:coreProperties>";
    }

    static std::string appProperties()
    {
        return std::string(XML_DECLARATION) +
               "<Properties xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\">"
               "<Application>" +
               std::string(base::LIBRARY_NAME) + "</Application></Properties>";
    }

    static std::string workbook(const std::vector<SheetBuilder*>& sheets)
    {
        std::string xml = XML_DECLARATION;
        xml += std::string("<workbook xmlns=\"") + MAIN_NAMESPACE + "\" xmlns:r=\"" + RELATIONSHIP_NAMESPACE +
               "\"><sheets>";
        for (std::size_t number = 0; number < sheets.size(); ++number)
            xml += "<sheet name=\"" + xmlEscaped(sheets[number]->name()) + "\" sheetId=\"" +
                   std::to_string(number + 1) + "\" r:id=\"rId" + std::to_string(number + 1) + "\"/>";
        xml += "</sheets>";
        // the range of the filters of each sheet, as Excel records it
        std::string names;
        for (std::size_t number = 0; number < sheets.size(); ++number)
        {
            const std::string range = sheets[number]->filterRange();
            if (!range.empty())
                names += "<definedName name=\"_xlnm._FilterDatabase\" localSheetId=\"" + std::to_string(number) +
                         "\" hidden=\"1\">" + xmlEscaped(range) + "</definedName>";
        }
        if (!names.empty())
            xml += "<definedNames>" + names + "</definedNames>";
        return xml + "</workbook>";
    }

    static std::string workbookRelationships(std::size_t sheets)
    {
        std::string xml = XML_DECLARATION;
        xml += "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">";
        for (std::size_t number = 1; number <= sheets; ++number)
            xml += "<Relationship Id=\"rId" + std::to_string(number) + "\" Type=\"" + RELATIONSHIP_NAMESPACE +
                   "/worksheet\" Target=\"worksheets/sheet" + std::to_string(number) + ".xml\"/>";
        xml += "<Relationship Id=\"rId" + std::to_string(sheets + 1) + "\" Type=\"" + RELATIONSHIP_NAMESPACE +
               "/styles\" Target=\"styles.xml\"/>";
        return xml + "</Relationships>";
    }

    // Four styles, in the order of the STYLE_ constants: plain, header, title, link.
    static std::string styles()
    {
        return std::string(XML_DECLARATION) + "<styleSheet xmlns=\"" + MAIN_NAMESPACE + "\">" +
               "<fonts count=\"4\">"
               "<font><sz val=\"11\"/><name val=\"Calibri\"/></font>"
               "<font><b/><sz val=\"11\"/><name val=\"Calibri\"/></font>"
               "<font><b/><sz val=\"14\"/><name val=\"Calibri\"/></font>"
               "<font><u/><sz val=\"11\"/><color rgb=\"FF0563C1\"/><name val=\"Calibri\"/></font>"
               "</fonts>"
               "<fills count=\"3\"><fill><patternFill patternType=\"none\"/></fill>"
               "<fill><patternFill patternType=\"gray125\"/></fill>"
               "<fill><patternFill patternType=\"solid\"><fgColor rgb=\"FFDDEBF7\"/><bgColor indexed=\"64\"/>"
               "</patternFill></fill></fills>"
               "<borders count=\"1\"><border><left/><right/><top/><bottom/><diagonal/></border></borders>"
               "<cellStyleXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\"/></cellStyleXfs>"
               "<cellXfs count=\"4\">"
               "<xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\"/>"
               "<xf numFmtId=\"0\" fontId=\"1\" fillId=\"2\" borderId=\"0\" xfId=\"0\" applyFont=\"1\" "
               "applyFill=\"1\"/>"
               "<xf numFmtId=\"0\" fontId=\"2\" fillId=\"0\" borderId=\"0\" xfId=\"0\" applyFont=\"1\"/>"
               "<xf numFmtId=\"0\" fontId=\"3\" fillId=\"0\" borderId=\"0\" xfId=\"0\" applyFont=\"1\"/>"
               "</cellXfs>"
               "<cellStyles count=\"1\"><cellStyle name=\"Normal\" xfId=\"0\" builtinId=\"0\"/></cellStyles>"
               "</styleSheet>";
    }

    const XlsxWriteOptions& m_options;
    ResultWriteReport& m_report;
    SheetNames m_names;
    std::vector<SheetBuilder> m_sheets;
    std::vector<IndexLine> m_index;
};

} // namespace

std::string formatXlsx(const ResultTables& tables, const XlsxWriteOptions& options, ResultWriteReport* report)
{
    ResultWriteReport local;
    ResultWriteReport& target = report != nullptr ? *report : local;
    target.format = "xlsx";
    WorkbookBuilder workbook(options, target);
    for (const std::shared_ptr<ResultTable>& table : tables.all())
        workbook.addTable(*table);
    for (const std::shared_ptr<NamedResult>& named : tables.namedResults())
        workbook.addNamed(*named);
    std::string bytes = workbook.finish();
    target.finished = true;
    target.bytes = bytes.size();
    return bytes;
}

ResultWriteReport writeXlsx(const std::filesystem::path& path, const ResultTables& tables,
                            const XlsxWriteOptions& options)
{
    ResultWriteReport report;
    const std::string bytes = formatXlsx(tables, options, &report);
    if (path.has_parent_path())
        std::filesystem::create_directories(path.parent_path()); // the folder of the file, when it is missing
    std::filesystem::path temporary = path;
    temporary += ".oor_tmp";
    {
        std::ofstream stream(temporary, std::ios::binary);
        if (!stream)
            throw base::IoError("cannot write " + temporary.string());
        stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        if (!stream)
            throw base::IoError("cannot write " + temporary.string());
    }
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    std::filesystem::rename(temporary, path);
    report.path = path.string();
    return report;
}

} // namespace oor::results
