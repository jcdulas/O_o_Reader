#include "oor/query/results/ResultSets.hpp"

#include "oor/analysis/frames/FrameRotation.hpp"
#include "oor/analysis/frames/StoredFrames.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/NamedQuery.hpp"

#include <algorithm>
#include <filesystem>
#include <mutex>
#include <set>
#include <string_view>

namespace oor::query
{

namespace
{

bool contains(const std::vector<std::int64_t>& list, std::int64_t value)
{
    return std::find(list.begin(), list.end(), value) != list.end();
}

std::string joinNames(const std::vector<std::string>& names)
{
    std::string text;
    for (const std::string& name : names)
        text += (text.empty() ? "" : ", ") + name;
    return text;
}

// The step selection of one kind of step as a filter member.
void applyStepSelection(results::ResultFilter& filter, const StepSelection& selection)
{
    if (selection.isSlice)
    {
        results::IndexSlice slice;
        slice.start = selection.start;
        slice.stop = selection.stop;
        slice.step = selection.step;
        filter.stepSlice = slice;
    }
    else
    {
        filter.steps = selection.values;
    }
}

} // namespace

ResultSelection ResultSelection::from(const QueryArguments& arguments)
{
    ResultSelection selection;
    selection.subcases = arguments.ids("subcases");
    selection.times = arguments.steps("times");
    selection.freqs = arguments.steps("freqs");
    selection.modes = arguments.steps("modes");
    selection.loadSteps = arguments.steps("loadSteps");
    if (arguments.isText("frame"))
    {
        selection.frameName = arguments.textOr("frame", "");
        for (char& character : selection.frameName)
            character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
        static const std::set<std::string> NAMES = {"basic", "element", "material", "ply", "output", "cut"};
        if (!NAMES.contains(selection.frameName))
            throw QueryError(
                "frame is None (as stored), \"basic\", \"element\", \"material\", \"ply\", \"output\", \"cut\" "
                "(sectionCut, sectionCuts) or the ID of a coordinate system, not '" +
                selection.frameName + "'");
    }
    else if (const auto system = arguments.ids("frame"))
    {
        selection.frameSystem = system->empty() ? 0 : system->front();
        if (selection.frameSystem == 0)
            selection.frameName = "basic";
    }
    selection.complexForm = arguments.textOr("complexForm", "");
    if (!selection.complexForm.empty() && selection.complexForm != "realImag" && selection.complexForm != "magPhase")
        throw QueryError("complexForm is None (as stored), \"realImag\" or \"magPhase\", not '" +
                         selection.complexForm + "'");
    selection.results = arguments.texts("results");
    selection.analyses = arguments.texts("analyses");
    selection.designCycles = arguments.ids("designCycles");
    const std::string missing = arguments.textOr("onMissing", "raise");
    if (missing == "raise")
        selection.onMissing = OnMissing::Raise;
    else if (missing == "null")
        selection.onMissing = OnMissing::Null;
    else if (missing == "skip")
        selection.onMissing = OnMissing::Skip;
    else
        throw QueryError("onMissing is \"raise\", \"null\" or \"skip\", not '" + missing + "'");
    selection.withSource = arguments.boolOr("withSource", false);
    selection.envelope = arguments.textOr("envelopeBy", "");
    selection.envelopeValue = arguments.textOr("envelopeValue", "");
    if (!selection.envelope.empty() && selection.envelope != "max" && selection.envelope != "min" &&
        selection.envelope != "absMax")
        throw QueryError("envelopeBy is None, \"max\", \"min\" or \"absMax\", not '" + selection.envelope + "'");
    if (arguments.has("envelopeOver"))
    {
        selection.envelopeOver = arguments.texts("envelopeOver");
        for (const std::string& name : selection.envelopeOver)
            if (name != "analyses" && name != "subcases" && name != "steps" && name != "designCycles")
                throw QueryError("envelopeOver names analyses, subcases, steps or designCycles, not '" + name + "'");
        if (selection.envelopeOver.empty())
            throw QueryError("envelopeOver names at least one of analyses, subcases, steps, designCycles");
    }
    return selection;
}

const std::vector<ParameterInfo>& resultParameters()
{
    static const std::vector<ParameterInfo> PARAMETERS = {
        parameter("subcases", ParameterKind::IdList, "None", "the subcases to keep"),
        parameter("times", ParameterKind::Steps, "None",
                  "the times to keep (transient results): a list of values or a slice of the positions"),
        parameter("freqs", ParameterKind::Steps, "None", "the frequencies to keep (frequency response)"),
        parameter("modes", ParameterKind::Steps, "None", "the mode numbers to keep (modal results)"),
        parameter("loadSteps", ParameterKind::Steps, "None", "the load steps to keep (non-linear static results)"),
        parameter(
            "frame", ParameterKind::IdListOrText, "None",
            "None (as stored), \"basic\", \"element\", \"material\", \"ply\", \"output\" or a coordinate system ID"),
        parameter("complexForm", ParameterKind::Text, "None",
                  "None (as stored), \"realImag\" or \"magPhase\" for complex results"),
        parameter("results", ParameterKind::TextList, "None",
                  "which attached result files to use, by analysis label or file name (None: all)"),
        parameter("analyses", ParameterKind::TextList, "None", "the analysis labels to keep"),
        parameter("onMissing", ParameterKind::Text, "\"raise\"",
                  "what to do with a quantity no provider can give: \"raise\", \"null\" or \"skip\""),
        parameter("withSource", ParameterKind::Bool, "False", "add a `source` column with the provider of each row"),
        parameter("designCycles", ParameterKind::IdList, "None", "the design cycles to keep (SOL 200 results)"),
        parameter(
            "envelopeBy", ParameterKind::Text, "None",
            "None, \"max\", \"min\" or \"absMax\": replace the rows that differ only in the dimensions of "
            "`envelopeOver` by the critical one (the extreme of the quantity of the query), whose `analysis` is the "
            "critical analysis; only the queries that rank a quantity take it"),
        parameter(
            "envelopeValue", ParameterKind::Text, "None",
            "the column the envelope decides by when it is not the quantity the query ranks (a stress component of "
            "`stressTable`, a translation of `displacementsTable`)"),
        parameter("envelopeOver", ParameterKind::TextList, "(\"analyses\",)",
                  "the dimensions the envelope is taken over: \"analyses\", \"subcases\", \"steps\", "
                  "\"designCycles\""),
    };
    return PARAMETERS;
}

std::vector<ResultSet> collectResultSets(const QueryRun& run)
{
    if (!run.resultSets.empty())
        return run.resultSets;
    std::vector<ResultSet> sets;
    if (run.model)
    {
        for (const std::shared_ptr<results::ResultAttachment>& attachment : results::ResultAttachment::of(*run.model))
            sets.push_back(ResultSet{attachment->analysis(), attachment->tables(), attachment, attachment->file()});
    }
    return sets;
}

std::string describeAttempts(const std::string& quantity, const std::vector<ProviderAttempt>& attempts)
{
    std::string text = "no way of obtaining " + quantity + " gave it";
    if (attempts.empty())
        return text + " (no provider is registered)";
    text += "; the ways tried, in order:";
    std::size_t number = 1;
    for (const ProviderAttempt& attempt : attempts)
    {
        text += "\n  " + std::to_string(number++) + ". " + attempt.provider +
                (attempt.approximate ? " (approximate)" : "") + ": ";
        if (attempt.rows > 0)
            text += "gave " + std::to_string(attempt.rows) + " rows, not all that were asked";
        else
            text += attempt.reason.empty() ? "gave nothing" : attempt.reason;
    }
    return text;
}

ResultQueryContext::ResultQueryContext(const QueryRun& run, const QueryInfo& info)
    : m_context(run.context),
      m_arguments(run.arguments),
      m_name(info.name),
      m_selection(ResultSelection::from(run.arguments)),
      m_model(run.model)
{
    if (info.needsModel && !m_model)
        throw QueryError("the query '" + info.name +
                         "' needs the model (it reads the geometry, the properties or the load cards): run it as "
                         "model.queries." +
                         info.name + "(...) with the results attached to the model; a result file alone has none");
    std::vector<ResultSet> all = collectResultSets(run);
    m_attachedCount = all.size();
    if (all.empty())
        throw QueryError("the query '" + info.name +
                         "' reads results and there are none: attach a result file to the model "
                         "(model.attach(results) or readResults(path, model=model))");
    // the labels of the sets, looked up at once (one file per load case: thousands of sets)
    std::set<std::string_view> known;
    for (const ResultSet& set : all)
        known.insert(set.analysis);
    for (const std::string& label : m_selection.analyses)
    {
        if (!known.contains(label))
        {
            constexpr std::size_t SHOWN = 10;
            std::vector<std::string> labels;
            for (const ResultSet& set : all)
            {
                if (labels.size() == SHOWN)
                    break;
                labels.push_back("'" + set.analysis + "'");
            }
            std::string list = joinNames(labels);
            if (all.size() > SHOWN)
                list += " and " + std::to_string(all.size() - SHOWN) + " more";
            throw QueryError("analyses names '" + label + "', which is not the label of a result set (" + list + ")");
        }
    }
    for (const std::string& name : m_selection.results)
    {
        const bool found = std::any_of(all.begin(), all.end(),
                                       [&](const ResultSet& set)
                                       {
                                           if (set.analysis == name)
                                               return true;
                                           if (!set.file)
                                               return false;
                                           const std::filesystem::path path(set.file->path());
                                           return set.file->path() == name || path.filename().string() == name;
                                       });
        if (!found)
            throw QueryError("results names '" + name + "', which is neither the label nor the file of a result set");
    }
    for (ResultSet& set : all)
    {
        if (!m_selection.analyses.empty() && std::find(m_selection.analyses.begin(), m_selection.analyses.end(),
                                                       set.analysis) == m_selection.analyses.end())
            continue;
        if (!m_selection.results.empty())
        {
            const bool wanted = std::any_of(
                m_selection.results.begin(), m_selection.results.end(),
                [&](const std::string& name)
                {
                    return set.analysis == name ||
                           (set.file && (set.file->path() == name ||
                                         std::filesystem::path(set.file->path()).filename().string() == name));
                });
            if (!wanted)
                continue;
        }
        m_sets.push_back(std::move(set));
    }
    m_setModelOnce = std::make_unique<std::once_flag[]>(m_sets.size());
    m_setModels.resize(m_sets.size());
}

const std::shared_ptr<model::Model>& ResultQueryContext::attachedModel(std::size_t set) const
{
    std::call_once(m_setModelOnce[set],
                   [&]
                   {
                       if (m_sets[set].attachment)
                           m_setModels[set] = m_sets[set].attachment->model();
                   });
    return m_setModels[set];
}

results::DecodeContext ResultQueryContext::decode() const
{
    results::DecodeContext context;
    context.pool = m_context.pool();
    context.token = m_context.token();
    return context;
}

std::shared_ptr<const results::ResultData> ResultQueryContext::decodeRows(
    const TableRef& ref, const std::optional<std::vector<std::int64_t>>& elementIds,
    const std::optional<std::vector<std::int64_t>>& pointIds) const
{
    if (!elementIds && !pointIds)
        return ref.table->data(decode());
    results::ResultFilter filter;
    if (elementIds)
        filter.elementIds = std::vector<std::int32_t>(elementIds->begin(), elementIds->end());
    if (pointIds)
        filter.ids = std::vector<std::int32_t>(pointIds->begin(), pointIds->end());
    // the selection of a table not yet decoded is applied while it decodes: its emptiness may show only then
    try
    {
        return ref.table->select(filter)->data(decode());
    }
    catch (const base::EmptySelectionError&)
    {
        return nullptr;
    }
}

const model::Model& ResultQueryContext::requireModel(const std::string& why) const
{
    if (!m_model)
        throw QueryError("the query '" + m_name + "' needs the model to " + why + ": run it as model.queries." +
                         m_name + "(...) with the results attached to the model");
    return *m_model;
}

const model::Model* ResultQueryContext::modelOf(std::size_t set) const
{
    if (set < m_sets.size())
    {
        if (const std::shared_ptr<model::Model>& model = attachedModel(set))
            return model.get();
    }
    return m_model.get();
}

void ResultQueryContext::noteStatus(std::string_view status) const
{
    const auto rank = [](std::string_view text)
    { return text == "experimental" ? 3 : (text == "notValidated" ? 2 : (text == "validated" ? 1 : 0)); };
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (rank(status) > rank(m_readStatus))
        m_readStatus = std::string(status);
}

std::optional<TableRef> ResultQueryContext::prepare(std::size_t set, const std::shared_ptr<results::ResultTable>& table,
                                                    bool rotate, const std::string& frameName,
                                                    std::int64_t frameSystem) const
{
    const results::ResultTableMeta& meta = table->meta();
    if (m_selection.subcases && !contains(*m_selection.subcases, meta.subcase))
        return std::nullopt;
    if (m_selection.designCycles && (meta.designCycle < 0 || !contains(*m_selection.designCycles, meta.designCycle)))
        return std::nullopt;
    results::ResultFilter filter;
    const std::optional<StepSelection>* wanted = nullptr;
    switch (meta.stepKind)
    {
    case results::StepKind::Time:
        wanted = &m_selection.times;
        break;
    case results::StepKind::Frequency:
        wanted = &m_selection.freqs;
        break;
    case results::StepKind::Mode:
        wanted = &m_selection.modes;
        break;
    case results::StepKind::LoadFactor:
        wanted = &m_selection.loadSteps;
        break;
    default:
        break;
    }
    if (wanted != nullptr && wanted->has_value())
        applyStepSelection(filter, **wanted);
    noteStatus(results::resultStatusName(meta.status));
    std::shared_ptr<results::ResultTable> current = table;
    if (!filter.isEmpty())
    {
        try
        {
            current = table->select(filter);
        }
        catch (const base::EmptySelectionError&)
        {
            return std::nullopt;
        }
    }
    if (rotate && frameName != "cut" && (!frameName.empty() || frameSystem != 0))
    {
        const model::Model* model = modelOf(set);
        const std::shared_ptr<results::ResultAttachment>& attachment = m_sets[set].attachment;
        if (model == nullptr || !attachment)
            throw QueryError("the query '" + m_name + "' was asked for the frame '" +
                             (frameName.empty() ? std::to_string(frameSystem) : frameName) +
                             "' and the results have no model to turn their values with: attach the results to the "
                             "model and run model.queries." +
                             m_name + "(...)");
        analysis::TargetFrame target;
        if (frameName == "basic")
            target.kind = analysis::FrameTargetKind::Basic;
        else if (frameName == "element")
            target.kind = analysis::FrameTargetKind::Element;
        else if (frameName == "material")
            target.kind = analysis::FrameTargetKind::Material;
        else if (frameName == "ply")
            target.kind = analysis::FrameTargetKind::Ply;
        else if (frameName == "output")
            target.kind = analysis::FrameTargetKind::NodeOutput;
        else
        {
            target.kind = analysis::FrameTargetKind::Coordinate;
            target.system = static_cast<std::int32_t>(frameSystem);
        }
        // turned when read: the rows a query selects of it are the only ones turned
        std::shared_ptr<const model::Model> owner = attachedModel(set);
        if (owner == nullptr)
            owner = m_model;
        // the frames of the entities are kept for the other tables of the model: the subcases of the query, and the
        // result sets attached to the same model (an element is the same card through any of them)
        std::shared_ptr<analysis::FrameCache> cache;
        {
            const std::lock_guard<std::mutex> lock(m_mutex);
            std::shared_ptr<analysis::FrameCache>& kept = m_frameCaches[owner.get()];
            if (!kept)
                kept = std::make_shared<analysis::FrameCache>();
            cache = kept;
        }
        current = analysis::FrameRotation::rotateWhenRead(owner, attachment, current, target, decode(), cache);
    }
    return TableRef{set, current};
}

std::vector<TableRef> ResultQueryContext::tables(results::ResultKind kind, bool rotate,
                                                 results::ResultVariant variant) const
{
    std::vector<TableRef> found;
    for (std::size_t set = 0; set < m_sets.size(); ++set)
    {
        for (const std::shared_ptr<results::ResultTable>& table : m_sets[set].tables->container(kind, variant).tables())
            if (const auto ref = prepare(set, table, rotate, m_selection.frameName, m_selection.frameSystem))
                found.push_back(*ref);
    }
    return found;
}

std::vector<TableRef> ResultQueryContext::tablesIn(results::ResultKind kind, const std::string& frameName,
                                                   results::ResultVariant variant) const
{
    std::vector<TableRef> found;
    for (std::size_t set = 0; set < m_sets.size(); ++set)
        for (const std::shared_ptr<results::ResultTable>& table : m_sets[set].tables->container(kind, variant).tables())
            if (const auto ref = prepare(set, table, true, frameName, 0))
                found.push_back(*ref);
    return found;
}

std::int64_t ResultQueryContext::resultantSystem() const
{
    if (m_selection.frameSystem != 0)
        return m_selection.frameSystem;
    if (m_selection.frameName == "cut" && (m_name == "sectionCut" || m_name == "sectionCuts"))
        return 0; // the axes of the cut are applied by the query
    if (!m_selection.frameName.empty() && m_selection.frameName != "basic")
        throw QueryError("the query '" + m_name +
                         "' adds forces and moments over several points: it needs one frame, the basic system or a "
                         "coordinate system ID, not '" +
                         m_selection.frameName + "'");
    return 0;
}

std::vector<TableRef> ResultQueryContext::elementTablesIn(results::ResultKind kind, std::string_view elementName,
                                                          const std::string& frameName,
                                                          results::ResultVariant variant) const
{
    std::vector<TableRef> found;
    for (std::size_t set = 0; set < m_sets.size(); ++set)
        for (const std::shared_ptr<results::ResultTable>& table :
             m_sets[set].tables->elementContainer(kind, elementName, variant).tables())
            if (const auto ref = prepare(set, table, true, frameName, 0))
                found.push_back(*ref);
    return found;
}

std::vector<TableRef> ResultQueryContext::resultantTables(results::ResultKind kind,
                                                          results::ResultVariant variant) const
{
    // the vectors are turned to the basic system, where the moments are taken; the sum is turned to the system asked
    // for
    (void)resultantSystem();
    std::vector<TableRef> found;
    for (std::size_t set = 0; set < m_sets.size(); ++set)
        for (const std::shared_ptr<results::ResultTable>& table : m_sets[set].tables->container(kind, variant).tables())
            if (const auto ref = prepare(set, table, true, "basic", 0))
                found.push_back(*ref);
    return found;
}

std::vector<TableRef> ResultQueryContext::resultantElementTables(results::ResultKind kind, std::string_view elementName,
                                                                 results::ResultVariant variant) const
{
    (void)resultantSystem();
    std::vector<TableRef> found;
    for (std::size_t set = 0; set < m_sets.size(); ++set)
        for (const std::shared_ptr<results::ResultTable>& table :
             m_sets[set].tables->elementContainer(kind, elementName, variant).tables())
            if (const auto ref = prepare(set, table, true, "basic", 0))
                found.push_back(*ref);
    return found;
}

std::vector<TableRef> ResultQueryContext::elementTables(results::ResultKind kind, std::string_view elementName,
                                                        bool rotate, results::ResultVariant variant) const
{
    std::vector<TableRef> found;
    for (std::size_t set = 0; set < m_sets.size(); ++set)
    {
        for (const std::shared_ptr<results::ResultTable>& table :
             m_sets[set].tables->elementContainer(kind, elementName, variant).tables())
            if (const auto ref = prepare(set, table, rotate, m_selection.frameName, m_selection.frameSystem))
                found.push_back(*ref);
    }
    return found;
}

std::vector<std::string> ResultQueryContext::elementNames(results::ResultKind kind,
                                                          results::ResultVariant variant) const
{
    std::set<std::string> names;
    for (const ResultSet& set : m_sets)
        for (const std::string& name : set.tables->elementNames(kind, variant))
            names.insert(name);
    return {names.begin(), names.end()};
}

std::vector<ResultQueryContext::NamedRef> ResultQueryContext::named(std::string_view kind) const
{
    std::vector<NamedRef> found;
    for (std::size_t set = 0; set < m_sets.size(); ++set)
    {
        for (const std::shared_ptr<results::NamedResult>& result : m_sets[set].tables->named(kind))
        {
            const results::NamedResultMeta& meta = result->meta();
            if (m_selection.subcases && meta.subcase > 0 && !contains(*m_selection.subcases, meta.subcase))
                continue;
            if (m_selection.designCycles && meta.designCycle >= 0 &&
                !contains(*m_selection.designCycles, meta.designCycle))
                continue;
            noteStatus(results::resultStatusName(meta.status));
            found.push_back(NamedRef{set, result});
        }
    }
    return found;
}

std::string ResultQueryContext::frameNameOf(const TableRef& ref) const
{
    const results::ResultTableMeta& meta = ref.table->meta();
    if (meta.derivation.rfind("inFrame(", 0) == 0)
        return std::string(results::storedFrameName(meta.storedFrame));
    const model::Model* model = modelOf(ref.set);
    const std::shared_ptr<results::ResultAttachment>& attachment = m_sets[ref.set].attachment;
    if (model != nullptr && attachment)
        return std::string(
            results::storedFrameName(analysis::StoredFrames::describe(*model, *attachment, *ref.table, decode()).kind));
    return std::string(results::storedFrameName(analysis::StoredFrames::describeWithoutModel(*ref.table).kind));
}

ResultQueryContext::ElementCard ResultQueryContext::elementCard(std::size_t set, std::int32_t resultId) const
{
    const std::uint64_t key = (static_cast<std::uint64_t>(set) << 32U) | static_cast<std::uint32_t>(resultId);
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        if (const auto found = m_cards.find(key); found != m_cards.end())
            return found->second;
    }
    ElementCard card;
    const model::Model* model = modelOf(set);
    const std::shared_ptr<results::ResultAttachment>& attachment = m_sets[set].attachment;
    if (model != nullptr && attachment && resultId > 0)
    {
        if (const auto location = attachment->elementLocation(*model, resultId))
        {
            card.linked = true;
            card.modelId = static_cast<std::int32_t>(model->findTable(location->table)->idAt(location->row));
            card.card = std::string(model::CardRegistry::instance().entry(location->table).info.name);
            if (const auto pid = analysis::StoredFrames::propertyOf(*model, *location))
                card.pid = static_cast<std::int32_t>(*pid);
        }
    }
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_cards.emplace(key, card);
    return card;
}

std::int32_t ResultQueryContext::modelNodeId(std::size_t set, std::int32_t resultId) const
{
    const model::Model* model = modelOf(set);
    const std::shared_ptr<results::ResultAttachment>& attachment = m_sets[set].attachment;
    if (model == nullptr || !attachment || resultId <= 0)
        return 0;
    return attachment->modelNodeId(*model, resultId);
}

} // namespace oor::query
