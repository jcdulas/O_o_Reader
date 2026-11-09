#include "oor/query/results/RowAccumulator.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/threading/ParallelFor.hpp"
#include "oor/model/DataContract.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/operators/ColumnOps.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <utility>

namespace oor::query
{

std::string sourceText(const std::string& name, bool approximate)
{
    return approximate ? name + " (approximate)" : name;
}

void RowBlock::check(const std::string& name, std::size_t size) const
{
    if (size != m_rows)
        throw base::UsageError("a block of " + std::to_string(m_rows) + " rows was given the column '" + name +
                               "' of " + std::to_string(size) + " values");
}

void RowBlock::ints(const std::string& name, std::vector<std::int32_t> values, std::vector<std::uint8_t> valid)
{
    check(name, values.size());
    Column column;
    column.kind = Column::Kind::Integer;
    column.integers = std::move(values);
    column.valid = std::move(valid);
    m_columns[name] = std::move(column);
}

void RowBlock::reals(const std::string& name, std::vector<double> values)
{
    check(name, values.size());
    Column column;
    column.kind = Column::Kind::Real;
    column.reals = std::move(values);
    m_columns[name] = std::move(column);
}

void RowBlock::complex(const std::string& name, std::vector<double> real, std::vector<double> imaginary)
{
    check(name, real.size());
    check(name, imaginary.size());
    Column column;
    column.kind = Column::Kind::Complex;
    column.reals = std::move(real);
    column.imaginary = std::move(imaginary);
    m_columns[name] = std::move(column);
}

void RowBlock::texts(const std::string& name, std::vector<std::string> values, std::vector<std::uint8_t> valid)
{
    check(name, values.size());
    Column column;
    column.kind = Column::Kind::Text;
    column.texts = std::move(values);
    column.valid = std::move(valid);
    m_columns[name] = std::move(column);
}

void RowBlock::codedTexts(const std::string& name, std::vector<std::string> dictionary, std::vector<std::int32_t> codes)
{
    check(name, codes.size());
    Column column;
    column.kind = Column::Kind::CodedText;
    column.texts = std::move(dictionary);
    column.codes = std::move(codes);
    m_columns[name] = std::move(column);
}

void RowBlock::flags(const std::string& name, std::vector<std::uint8_t> values, std::vector<std::uint8_t> valid)
{
    check(name, values.size());
    Column column;
    column.kind = Column::Kind::Flag;
    column.flags = std::move(values);
    column.valid = std::move(valid);
    m_columns[name] = std::move(column);
}

void RowBlock::intLists(const std::string& name, std::vector<std::vector<std::int32_t>> values)
{
    check(name, values.size());
    Column column;
    column.kind = Column::Kind::IntList;
    column.intLists = std::move(values);
    m_columns[name] = std::move(column);
}

void RowBlock::textLists(const std::string& name, std::vector<std::vector<std::string>> values)
{
    check(name, values.size());
    Column column;
    column.kind = Column::Kind::TextList;
    column.textLists = std::move(values);
    m_columns[name] = std::move(column);
}

void RowBlock::rename(const std::string& from, const std::string& to)
{
    const auto found = m_columns.find(from);
    if (found == m_columns.end())
        throw base::UsageError("a block has no column '" + from + "' to rename");
    Column column = std::move(found->second);
    m_columns.erase(found);
    m_columns[to] = std::move(column);
}

void RowBlock::steps(results::StepKind kind, std::vector<double> values, std::vector<double> eigenvalues)
{
    check("[step]", values.size());
    m_stepKind = kind;
    m_steps = std::move(values);
    m_eigenvalues = std::move(eigenvalues);
}

void RowBlock::origin(std::size_t set, std::string analysis, std::int32_t designCycle)
{
    m_set = set;
    m_analysis = std::move(analysis);
    m_designCycle = designCycle;
}

std::int16_t RowAccumulator::addProvider(std::string name, bool approximate)
{
    m_providers.push_back(Provider{std::move(name), approximate});
    return static_cast<std::int16_t>(m_providers.size() - 1);
}

void RowAccumulator::note(std::int16_t provider, const std::vector<std::string>& columns, std::int64_t rows)
{
    if (rows <= 0)
        return;
    m_notes.push_back(Note{provider, columns, rows});
}

void RowAccumulator::add(RowBlock&& block)
{
    if (block.rows() == 0)
        return;
    m_rows += block.rows();
    m_blocks.push_back(std::move(block));
}

namespace
{

QueryColumn withValidity(QueryColumn column, const std::vector<std::uint8_t>& valid)
{
    std::int64_t nullCount = 0;
    ColumnBuffer bits = validityFromFlags(valid, nullCount);
    if (nullCount == 0)
        return column;
    return column.withValidity(std::move(bits), nullCount);
}

std::vector<std::uint8_t> validOfReals(const std::vector<double>& values)
{
    std::vector<std::uint8_t> valid(values.size());
    for (std::size_t row = 0; row < values.size(); ++row)
        valid[row] = std::isnan(values[row]) ? 0 : 1;
    return valid;
}

QueryColumn realColumn(const std::vector<double>& values, bool single)
{
    const std::vector<std::uint8_t> valid = validOfReals(values);
    if (single)
    {
        std::vector<float> narrowed(values.size());
        for (std::size_t row = 0; row < values.size(); ++row)
            narrowed[row] = static_cast<float>(values[row]);
        return withValidity(QueryColumn::primitive(std::move(narrowed)), valid);
    }
    return withValidity(QueryColumn::primitive(std::vector<double>(values)), valid);
}

} // namespace

std::shared_ptr<const QueryTable> RowAccumulator::finish(const ResultQueryContext& context, const QueryInfo& info,
                                                         const std::string& detail, bool forceSource)
{
    const ColumnSet& set = info.columnsFor(detail);
    std::set<std::string> listed;
    for (const ColumnSpec& spec : set.columns)
        listed.insert(spec.name);
    listed.insert("source");
    for (const RowBlock& block : m_blocks)
        for (const auto& entry : block.columns())
            if (!listed.contains(entry.first))
                throw base::UsageError("the query '" + info.name + "' made a column '" + entry.first +
                                       "' that its catalogue does not list");

    bool single = !m_blocks.empty();
    bool anyDesign = false;
    bool anyMagnitudePhase = false;
    bool kinds[6] = {false, false, false, false, false, false};
    std::set<std::string> complexNames;
    for (const RowBlock& block : m_blocks)
    {
        single = single && block.precision() == Precision::Single;
        anyDesign = anyDesign || block.designCycle() >= 0;
        anyMagnitudePhase = anyMagnitudePhase || block.magnitudePhase();
        kinds[static_cast<std::size_t>(block.stepKind())] = true;
        for (const auto& entry : block.columns())
            if (entry.second.kind == RowBlock::Column::Kind::Complex)
                complexNames.insert(entry.first);
    }
    const std::string form = !context.selection().complexForm.empty() ? context.selection().complexForm
                                                                      : (anyMagnitudePhase ? "magPhase" : "realImag");
    const std::size_t rows = m_rows;

    QueryTable table;
    // The columns are made on the threads of the call, each by a job of its own, and added in their order.
    using Columns = std::vector<std::pair<std::string, QueryColumn>>;
    std::vector<std::function<void(Columns&)>> jobs;

    const auto addInteger = [&](Columns& out, const std::string& name, bool wide)
    {
        std::vector<std::int32_t> values;
        std::vector<std::uint8_t> valid;
        values.reserve(rows);
        valid.reserve(rows);
        for (const RowBlock& block : m_blocks)
        {
            const auto found = block.columns().find(name);
            if (found == block.columns().end())
            {
                values.insert(values.end(), block.rows(), 0);
                valid.insert(valid.end(), block.rows(), 0);
                continue;
            }
            const RowBlock::Column& column = found->second;
            if (column.kind != RowBlock::Column::Kind::Integer)
                throw base::UsageError("the column '" + name + "' of '" + info.name + "' is an integer column");
            values.insert(values.end(), column.integers.begin(), column.integers.end());
            if (column.valid.empty())
                valid.insert(valid.end(), block.rows(), 1);
            else
                valid.insert(valid.end(), column.valid.begin(), column.valid.end());
        }
        if (wide)
        {
            std::vector<std::int64_t> widened(values.begin(), values.end());
            out.emplace_back(name, withValidity(QueryColumn::primitive(std::move(widened)), valid));
        }
        else
        {
            out.emplace_back(name, withValidity(QueryColumn::primitive(std::move(values)), valid));
        }
    };

    const auto gatherReals = [&](const std::string& name, bool imaginary)
    {
        std::vector<double> values;
        values.reserve(rows);
        for (const RowBlock& block : m_blocks)
        {
            const auto found = block.columns().find(name);
            if (found == block.columns().end())
            {
                values.insert(values.end(), block.rows(), std::nan(""));
                continue;
            }
            const RowBlock::Column& column = found->second;
            if (column.kind == RowBlock::Column::Kind::Real)
            {
                if (imaginary)
                    values.insert(values.end(), block.rows(),
                                  0.0); // a real value is a complex one with no imaginary part
                else
                    values.insert(values.end(), column.reals.begin(), column.reals.end());
            }
            else if (column.kind == RowBlock::Column::Kind::Complex)
            {
                const std::vector<double>& part = imaginary ? column.imaginary : column.reals;
                values.insert(values.end(), part.begin(), part.end());
            }
            else
            {
                throw base::UsageError("the column '" + name + "' of '" + info.name + "' is a column of numbers");
            }
        }
        return values;
    };

    const auto addReal = [&](Columns& out, const std::string& name, bool result)
    {
        if (result && complexNames.contains(name))
        {
            std::vector<double> real = gatherReals(name, false);
            std::vector<double> imaginary = gatherReals(name, true);
            if (form == "magPhase")
            {
                std::vector<double> magnitude(rows);
                std::vector<double> phase(rows);
                for (std::size_t row = 0; row < rows; ++row)
                {
                    magnitude[row] = std::hypot(real[row], imaginary[row]);
                    phase[row] = std::isnan(real[row])
                                     ? std::nan("")
                                     : std::atan2(imaginary[row], real[row]) * 180.0 / 3.14159265358979323846;
                }
                out.emplace_back(name + "Mag", realColumn(magnitude, single));
                out.emplace_back(name + "Phase", realColumn(phase, single));
            }
            else
            {
                out.emplace_back(name + "Re", realColumn(real, single));
                out.emplace_back(name + "Im", realColumn(imaginary, single));
            }
            return;
        }
        out.emplace_back(name, realColumn(gatherReals(name, false), result && single));
    };

    const auto addText = [&](Columns& out, const std::string& name, bool dictionary)
    {
        // a column of codes in some block: one dictionary for every block, the texts of the others coded into it
        const bool coded = std::any_of(m_blocks.begin(), m_blocks.end(),
                                       [&](const RowBlock& block)
                                       {
                                           const auto found = block.columns().find(name);
                                           return found != block.columns().end() &&
                                                  found->second.kind == RowBlock::Column::Kind::CodedText;
                                       });
        if (coded)
        {
            std::vector<std::string> texts;
            std::map<std::string, std::int32_t> codeOf;
            const auto code = [&](const std::string& text)
            {
                const auto [at, added] = codeOf.try_emplace(text, static_cast<std::int32_t>(texts.size()));
                if (added)
                    texts.push_back(text);
                return at->second;
            };
            std::vector<std::int32_t> codes;
            codes.reserve(rows);
            for (const RowBlock& block : m_blocks)
            {
                const auto found = block.columns().find(name);
                if (found == block.columns().end())
                {
                    codes.insert(codes.end(), block.rows(), -1);
                    continue;
                }
                const RowBlock::Column& column = found->second;
                if (column.kind == RowBlock::Column::Kind::CodedText)
                {
                    std::vector<std::int32_t> entry(column.texts.size());
                    for (std::size_t index = 0; index < column.texts.size(); ++index)
                        entry[index] = code(column.texts[index]);
                    for (const std::int32_t value : column.codes)
                        codes.push_back(value < 0 ? -1 : entry[static_cast<std::size_t>(value)]);
                    continue;
                }
                if (column.kind != RowBlock::Column::Kind::Text)
                    throw base::UsageError("the column '" + name + "' of '" + info.name + "' is a text column");
                for (std::size_t row = 0; row < column.texts.size(); ++row)
                    codes.push_back(!column.valid.empty() && column.valid[row] == 0 ? -1 : code(column.texts[row]));
            }
            if (dictionary)
            {
                out.emplace_back(name, codedDictionaryColumn(texts, codes));
                return;
            }
            std::vector<std::string> values(codes.size());
            std::vector<std::uint8_t> valid(codes.size(), 1);
            for (std::size_t row = 0; row < codes.size(); ++row)
            {
                if (codes[row] < 0)
                    valid[row] = 0;
                else
                    values[row] = texts[static_cast<std::size_t>(codes[row])];
            }
            out.emplace_back(name, withValidity(QueryColumn::utf8(values), valid));
            return;
        }
        std::vector<std::string> values;
        std::vector<std::uint8_t> valid;
        values.reserve(rows);
        valid.reserve(rows);
        for (const RowBlock& block : m_blocks)
        {
            const auto found = block.columns().find(name);
            if (found == block.columns().end())
            {
                values.insert(values.end(), block.rows(), std::string());
                valid.insert(valid.end(), block.rows(), 0);
                continue;
            }
            const RowBlock::Column& column = found->second;
            if (column.kind != RowBlock::Column::Kind::Text)
                throw base::UsageError("the column '" + name + "' of '" + info.name + "' is a text column");
            values.insert(values.end(), column.texts.begin(), column.texts.end());
            if (column.valid.empty())
                valid.insert(valid.end(), block.rows(), 1);
            else
                valid.insert(valid.end(), column.valid.begin(), column.valid.end());
        }
        if (dictionary)
            out.emplace_back(name, dictionaryColumn(values, valid));
        else
            out.emplace_back(name, withValidity(QueryColumn::utf8(values), valid));
    };

    const auto addFlag = [&](Columns& out, const std::string& name)
    {
        std::vector<std::uint8_t> values;
        std::vector<std::uint8_t> valid;
        values.reserve(rows);
        valid.reserve(rows);
        for (const RowBlock& block : m_blocks)
        {
            const auto found = block.columns().find(name);
            if (found == block.columns().end())
            {
                values.insert(values.end(), block.rows(), 0);
                valid.insert(valid.end(), block.rows(), 0);
                continue;
            }
            const RowBlock::Column& column = found->second;
            if (column.kind != RowBlock::Column::Kind::Flag)
                throw base::UsageError("the column '" + name + "' of '" + info.name + "' is a flag column");
            values.insert(values.end(), column.flags.begin(), column.flags.end());
            if (column.valid.empty())
                valid.insert(valid.end(), block.rows(), 1);
            else
                valid.insert(valid.end(), column.valid.begin(), column.valid.end());
        }
        out.emplace_back(name, withValidity(QueryColumn::boolean(values), valid));
    };

    // a list column: the lists of the rows as compressed rows (a block without the column gives empty lists)
    const auto addList = [&](Columns& out, const std::string& name, bool text)
    {
        std::vector<std::int64_t> offsets{0};
        offsets.reserve(rows + 1);
        std::vector<std::int32_t> integers;
        std::vector<std::string> texts;
        for (const RowBlock& block : m_blocks)
        {
            const auto found = block.columns().find(name);
            if (found == block.columns().end())
            {
                offsets.insert(offsets.end(), block.rows(), offsets.back());
                continue;
            }
            const RowBlock::Column& column = found->second;
            if (column.kind != (text ? RowBlock::Column::Kind::TextList : RowBlock::Column::Kind::IntList))
                throw base::UsageError("the column '" + name + "' of '" + info.name + "' is a list column");
            for (std::size_t row = 0; row < block.rows(); ++row)
            {
                if (text)
                    texts.insert(texts.end(), column.textLists[row].begin(), column.textLists[row].end());
                else
                    integers.insert(integers.end(), column.intLists[row].begin(), column.intLists[row].end());
                offsets.push_back(static_cast<std::int64_t>(text ? texts.size() : integers.size()));
            }
        }
        const auto length = static_cast<std::int64_t>(offsets.size()) - 1;
        QueryColumn values = text ? QueryColumn::utf8(texts) : QueryColumn::primitive(std::move(integers));
        out.emplace_back(name, QueryColumn::list(ColumnBuffer::adopt(std::move(offsets)), std::move(values), length));
    };

    const auto addSteps = [&](Columns& out)
    {
        const auto columnOf = [&](results::StepKind kind, bool mode)
        {
            std::vector<double> values;
            values.reserve(rows);
            for (const RowBlock& block : m_blocks)
            {
                if (block.stepKind() == kind)
                {
                    const std::vector<double>& source = mode ? block.eigenvalues() : block.stepValues();
                    if (source.empty())
                        values.insert(values.end(), block.rows(), std::nan(""));
                    else
                        values.insert(values.end(), source.begin(), source.end());
                }
                else
                {
                    values.insert(values.end(), block.rows(), std::nan(""));
                }
            }
            return values;
        };
        if (kinds[static_cast<std::size_t>(results::StepKind::Time)])
            out.emplace_back("time", realColumn(columnOf(results::StepKind::Time, false), false));
        if (kinds[static_cast<std::size_t>(results::StepKind::Frequency)])
            out.emplace_back("freq", realColumn(columnOf(results::StepKind::Frequency, false), false));
        if (kinds[static_cast<std::size_t>(results::StepKind::Mode)])
        {
            const std::vector<double> modes = columnOf(results::StepKind::Mode, false);
            std::vector<std::int32_t> numbers(rows, 0);
            std::vector<std::uint8_t> valid(rows, 1);
            for (std::size_t row = 0; row < rows; ++row)
            {
                if (std::isnan(modes[row]))
                    valid[row] = 0;
                else
                    numbers[row] = static_cast<std::int32_t>(std::lround(modes[row]));
            }
            out.emplace_back("mode", withValidity(QueryColumn::primitive(std::move(numbers)), valid));
            out.emplace_back("eigenvalue", realColumn(columnOf(results::StepKind::Mode, true), false));
        }
        if (kinds[static_cast<std::size_t>(results::StepKind::LoadFactor)])
            out.emplace_back("loadStep", realColumn(columnOf(results::StepKind::LoadFactor, false), false));
    };

    const auto addOrigin = [&](Columns& out)
    {
        if (anyDesign && !listed.contains("designCycle"))
        {
            std::vector<std::int32_t> cycles;
            std::vector<std::uint8_t> valid;
            for (const RowBlock& block : m_blocks)
            {
                cycles.insert(cycles.end(), block.rows(), std::max(block.designCycle(), 0));
                valid.insert(valid.end(), block.rows(), block.designCycle() >= 0 ? 1 : 0);
            }
            out.emplace_back("designCycle", withValidity(QueryColumn::primitive(std::move(cycles)), valid));
        }
        if (context.attachedSetCount() > 1 && !listed.contains("analysis"))
        {
            std::vector<std::string> labels;
            labels.reserve(rows);
            for (const RowBlock& block : m_blocks)
                labels.insert(labels.end(), block.rows(), block.analysis());
            out.emplace_back("analysis", dictionaryColumn(labels));
        }
    };

    bool sourceListed = false;
    // `analysis` and `designCycle` come before `subcase`; a query that has no subcase column gets them first
    if (std::none_of(set.columns.begin(), set.columns.end(),
                     [](const ColumnSpec& spec) { return spec.name == "subcase"; }))
        jobs.emplace_back([&](Columns& out) { addOrigin(out); });
    for (const ColumnSpec& spec : set.columns)
    {
        if (spec.name == "source")
            sourceListed = true;
        const std::string& name = spec.name;
        switch (spec.kind)
        {
        case ColumnKind::Step:
            jobs.emplace_back([&](Columns& out) { addSteps(out); });
            break;
        case ColumnKind::Int32:
            if (name == "subcase")
                jobs.emplace_back([&](Columns& out) { addOrigin(out); });
            jobs.emplace_back([&, name](Columns& out) { addInteger(out, name, false); });
            break;
        case ColumnKind::Int64:
            jobs.emplace_back([&, name](Columns& out) { addInteger(out, name, true); });
            break;
        case ColumnKind::Float64:
            jobs.emplace_back([&, name](Columns& out) { addReal(out, name, false); });
            break;
        case ColumnKind::Result:
            jobs.emplace_back([&, name](Columns& out) { addReal(out, name, true); });
            break;
        case ColumnKind::Bool:
            jobs.emplace_back([&, name](Columns& out) { addFlag(out, name); });
            break;
        case ColumnKind::Text:
            jobs.emplace_back([&, name](Columns& out) { addText(out, name, false); });
            break;
        case ColumnKind::Dictionary:
            jobs.emplace_back([&, name](Columns& out) { addText(out, name, true); });
            break;
        case ColumnKind::IntList:
            jobs.emplace_back([&, name](Columns& out) { addList(out, name, false); });
            break;
        case ColumnKind::TextList:
            jobs.emplace_back([&, name](Columns& out) { addList(out, name, true); });
            break;
        default:
            throw base::UsageError("the query '" + info.name +
                                   "' has a column of a type the result tables do not make");
        }
    }
    std::vector<Columns> made(jobs.size());
    const results::DecodeContext decode = context.decode();
    const auto run = [&](std::size_t index) { jobs[index](made[index]); };
    if (decode.pool != nullptr && jobs.size() > 1 && rows > 10'000)
        base::parallelFor(*decode.pool, jobs.size(), run, decode.token);
    else
        for (std::size_t index = 0; index < jobs.size(); ++index)
            run(index);
    for (Columns& columns : made)
        for (auto& [name, column] : columns)
            table.addColumn(name, std::move(column));
    // the provenance
    std::vector<SourceNote> notes;
    const auto noteFor = [&](std::int16_t provider) -> SourceNote&
    {
        const Provider& entry = m_providers[static_cast<std::size_t>(provider)];
        for (SourceNote& existing : notes)
            if (existing.provider == entry.name && existing.approximate == entry.approximate)
                return existing;
        SourceNote created;
        created.provider = entry.name;
        created.approximate = entry.approximate;
        notes.push_back(created);
        return notes.back();
    };
    const auto addColumns = [&](SourceNote& note, const std::vector<std::string>& names)
    {
        for (const std::string& name : names)
            if (std::find(note.columns.begin(), note.columns.end(), name) == note.columns.end())
                note.columns.push_back(name);
    };
    // The `source` column as entries of its dictionary: the rows of a provider share one text, made once, unless a row
    // has a note of its own.
    std::map<std::string, std::int32_t> sourceEntries;
    std::vector<std::int32_t> sourceRows;
    const auto sourceEntry = [&](const std::string& text)
    { return sourceEntries.try_emplace(text, static_cast<std::int32_t>(sourceEntries.size())).first->second; };
    const bool wantSource = (context.selection().withSource || forceSource) && !sourceListed;
    if (wantSource)
        sourceRows.reserve(rows);
    for (const RowBlock& block : m_blocks)
    {
        std::vector<std::string> valueColumns;
        for (const auto& entry : block.columns())
        {
            if (entry.second.kind != RowBlock::Column::Kind::Real &&
                entry.second.kind != RowBlock::Column::Kind::Complex)
                continue;
            const auto spec = std::find_if(set.columns.begin(), set.columns.end(),
                                           [&](const ColumnSpec& item) { return item.name == entry.first; });
            if (spec != set.columns.end() && spec->kind == ColumnKind::Result)
                valueColumns.push_back(entry.first);
        }
        std::sort(valueColumns.begin(), valueColumns.end());
        const std::vector<std::int16_t>& providers = block.providerRows();
        if (providers.empty())
        {
            if (wantSource)
                sourceRows.insert(sourceRows.end(), block.rows(), sourceEntry(std::string()));
            continue;
        }
        std::map<std::int16_t, std::int64_t> counts;
        std::int16_t lastProvider = -1;
        std::int32_t lastEntry = 0;
        for (std::size_t row = 0; row < block.rows(); ++row)
        {
            if (providers[row] < 0)
            {
                if (wantSource)
                    sourceRows.push_back(sourceEntry(std::string()));
                continue;
            }
            ++counts[providers[row]];
            if (!wantSource)
                continue;
            const bool noted = !block.suffixes().empty() && !block.suffixes()[row].empty();
            if (!noted && providers[row] == lastProvider)
            {
                sourceRows.push_back(lastEntry);
                continue;
            }
            const Provider& provider = m_providers[static_cast<std::size_t>(providers[row])];
            std::string text = sourceText(provider.name, provider.approximate);
            if (noted)
            {
                sourceRows.push_back(sourceEntry(text + "; " + block.suffixes()[row]));
                continue;
            }
            lastProvider = providers[row];
            lastEntry = sourceEntry(text);
            sourceRows.push_back(lastEntry);
        }
        for (const auto& [provider, count] : counts)
        {
            SourceNote& note = noteFor(provider);
            note.rows += count;
            addColumns(note, valueColumns);
            const std::string& status =
                block.validationStatus().empty() ? context.readStatus() : block.validationStatus();
            if (!status.empty() && (note.status.empty() || (note.status == "validated" && status != "validated") ||
                                    (note.status == "notValidated" && status == "experimental")))
                note.status = status;
        }
    }
    for (const Note& entry : m_notes)
    {
        SourceNote& note = noteFor(entry.provider);
        note.rows += entry.rows;
        addColumns(note, entry.columns);
        if (note.status.empty())
            note.status = context.readStatus();
    }
    if (wantSource)
    {
        // the dictionary in the order of its texts, as every dictionary column of a query has it
        std::vector<std::string> dictionary;
        std::vector<std::int32_t> sortedEntry(sourceEntries.size());
        for (const auto& [text, entry] : sourceEntries)
        {
            sortedEntry[static_cast<std::size_t>(entry)] = static_cast<std::int32_t>(dictionary.size());
            dictionary.push_back(text);
        }
        for (std::int32_t& entry : sourceRows)
            entry = sortedEntry[static_cast<std::size_t>(entry)];
        table.addColumn("source", QueryColumn::dictionary(std::move(sourceRows), dictionary));
    }
    table.setSources(std::move(notes));
    table.setContractVersion(model::DATA_CONTRACT_VERSION);
    return std::make_shared<const QueryTable>(std::move(table));
}

} // namespace oor::query
