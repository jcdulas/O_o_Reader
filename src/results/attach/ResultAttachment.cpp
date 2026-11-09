#include "oor/results/attach/ResultAttachment.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/threading/ParallelFor.hpp"
#include "oor/results/core/ResultCatalogue.hpp"

#include <algorithm>
#include <filesystem>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>

namespace oor::results
{

std::string_view attachmentStateName(AttachmentState state) noexcept
{
    switch (state)
    {
    case AttachmentState::Current:
        return "current";
    case AttachmentState::Edited:
        return "edited";
    case AttachmentState::Stale:
        return "stale";
    }
    return "current";
}

namespace
{

// The results of the elements with this ID in the strain energy table: the total of the model is written as an element.
constexpr std::int32_t FIRST_NON_ELEMENT_ID = 100000000;
// The most diagnostics one check writes about tables; the others are counted.
constexpr std::size_t MAX_TABLE_DIAGNOSTICS = 40;
constexpr std::size_t IDS_SHOWN = 5;

// A list of IDs, increasing, each once; the tables that have the same list share it.
using IdList = std::shared_ptr<const std::vector<std::int32_t>>;

// The IDs of the grid points and of the elements a table talks about.
struct TableLabels
{
    std::shared_ptr<ResultTable> table;
    IdList nodes;
    IdList elements;
};

void sortUnique(std::vector<std::int32_t>& ids)
{
    // the rows of a table mostly come in increasing ID: they are then only checked
    if (!std::is_sorted(ids.begin(), ids.end()))
        std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
}

// Reads the labels of one table: one step and one component of it decoded, which is all the rows have in common.
TableLabels labelsOf(const std::shared_ptr<ResultTable>& table, const DecodeContext& context)
{
    TableLabels labels;
    labels.table = table;
    std::vector<std::int32_t> nodes;
    std::vector<std::int32_t> elements;
    const ResultKindInfo& info = resultKindInfo(table->meta().kind);
    if (info.rows == RowKind::Mode)
    {
        labels.nodes = std::make_shared<const std::vector<std::int32_t>>();
        labels.elements = labels.nodes;
        return labels;
    }
    std::shared_ptr<const ResultData> data;
    if (table->isDecoded())
    {
        data = table->data(context);
    }
    else
    {
        ResultFilter filter;
        filter.stepIndices = std::vector<std::int64_t>{0};
        if (!table->meta().components.empty())
            filter.components = std::vector<std::string>{table->meta().components.front()};
        data = table->select(filter)->data(context);
    }
    switch (info.rows)
    {
    case RowKind::GridPoint:
        for (std::size_t row = 0; row < data->ids.size(); ++row)
        {
            // extra points (3) and modal points (4) are not cards of the model
            const std::int32_t type = row < data->pointTypes.size() ? data->pointTypes[row] : 1;
            if (data->ids[row] > 0 && type != 3 && type != 4)
                nodes.push_back(data->ids[row]);
        }
        break;
    case RowKind::Element:
        for (const std::int32_t id : data->ids)
            if (id > 0 && id < FIRST_NON_ELEMENT_ID)
                elements.push_back(id);
        for (const std::int32_t id : data->nodeIds)
            if (id > 0)
                nodes.push_back(id);
        break;
    case RowKind::GridPointElement:
        for (const std::int32_t id : data->ids)
            if (id > 0)
                nodes.push_back(id);
        for (const std::int32_t id : data->elementIds)
            if (id > 0 && id < FIRST_NON_ELEMENT_ID)
                elements.push_back(id);
        break;
    case RowKind::Mode:
        break;
    }
    sortUnique(nodes);
    sortUnique(elements);
    labels.nodes = std::make_shared<const std::vector<std::int32_t>>(std::move(nodes));
    labels.elements = std::make_shared<const std::vector<std::int32_t>>(std::move(elements));
    return labels;
}

// What the check finds of one list of labels: the IDs that no card has, and the elements of another card type.
struct LabelCheck
{
    std::vector<std::int32_t> missingNodes;
    std::vector<std::int32_t> missingElements;
    std::vector<std::int32_t> wrongType;
};

const CardLink* findLink(const std::vector<CardLink>& links, std::int32_t id) noexcept
{
    const auto found = std::lower_bound(links.begin(), links.end(), id,
                                        [](const CardLink& link, std::int32_t value) { return link.resultId < value; });
    return found != links.end() && found->resultId == id ? &*found : nullptr;
}

std::string idList(const std::vector<std::int32_t>& ids)
{
    std::string text;
    for (std::size_t index = 0; index < ids.size() && index < IDS_SHOWN; ++index)
        text += (index == 0 ? "" : ", ") + std::to_string(ids[index]);
    if (ids.size() > IDS_SHOWN)
        text += ", ...";
    return text;
}

std::string describeTable(const ResultTableMeta& meta)
{
    std::string text = std::string(resultKindName(meta.kind));
    if (!meta.elementName.empty())
        text += "." + meta.elementName;
    return text + " (subcase " + std::to_string(meta.subcase) + ")";
}

std::uint32_t keyOf(const model::Model& model, model::CardLocation location) noexcept
{
    return model.findTable(location.table)->rows().keyAt(location.row).value;
}

} // namespace

// ---------------------------------------------------------------------------------------------- the list in the model

std::unique_ptr<model::TableAttachment> ModelResults::clone() const
{
    auto copy = std::make_unique<ModelResults>();
    copy->m_items.reserve(m_items.size());
    for (const std::shared_ptr<ResultAttachment>& item : m_items)
    {
        auto duplicate = std::make_shared<ResultAttachment>(*item);
        duplicate->setModelResolver(nullptr);
        copy->m_items.push_back(std::move(duplicate));
    }
    return copy;
}

ModelResults& ModelResults::of(const model::Model& model)
{
    model::AttachmentSlot& slot = model.resultsSlot();
    if (slot.get() == nullptr)
        slot.reset(std::make_unique<ModelResults>());
    return static_cast<ModelResults&>(*slot.get());
}

const ModelResults* ModelResults::peek(const model::Model& model) noexcept
{
    return static_cast<const ModelResults*>(model.resultsSlot().get());
}

// -------------------------------------------------------------------------------------------------------- attaching

void ResultAttachment::setModel(const std::shared_ptr<model::Model>& model)
{
    m_resolver = [weak = std::weak_ptr<model::Model>(model)] { return weak.lock(); };
}

void ResultAttachment::snapshot(const model::Model& model) noexcept
{
    m_structureVersion = model.structureVersion();
    m_contentVersion = model.contentVersion();
}

std::vector<std::shared_ptr<ResultAttachment>> ResultAttachment::of(const model::Model& model)
{
    const ModelResults* list = ModelResults::peek(model);
    return list != nullptr ? list->items() : std::vector<std::shared_ptr<ResultAttachment>>{};
}

std::shared_ptr<ResultAttachment> ResultAttachment::attach(const std::shared_ptr<model::Model>& model,
                                                           std::shared_ptr<ResultFile> file,
                                                           std::shared_ptr<ResultTables> tables,
                                                           const AttachOptions& options, const DecodeContext& context)
{
    if (!model || !file || !tables)
        throw base::UsageError("attach needs a model and a result file");
    if (std::shared_ptr<ResultAttachment> same = unchanged(*model, tables, options))
        return same;
    auto attachment = std::make_shared<ResultAttachment>();
    attachment->m_file = std::move(file);
    attachment->m_tables = std::move(tables);
    attachment->m_filter = options.filter;
    attachment->m_analysis = options.analysis.empty()
                                 ? std::filesystem::path(attachment->m_file->path()).filename().string()
                                 : options.analysis;
    // the labels of the sets of a model are all different, except that attaching the same tables again replaces the
    // set. A label that was asked for and is taken is an error; the default one (the file name) gets a number:
    // "plate.h5#2"
    {
        const std::vector<std::shared_ptr<ResultAttachment>> others = of(*model);
        const auto taken = [&](const std::string& label)
        {
            return std::any_of(others.begin(), others.end(), [&](const std::shared_ptr<ResultAttachment>& item)
                               { return item->m_analysis == label && item->m_tables != attachment->m_tables; });
        };
        if (taken(attachment->m_analysis))
        {
            if (!options.analysis.empty())
                throw base::UsageError("a result set labelled '" + attachment->m_analysis +
                                       "' is attached to the model already: give this one another label");
            const std::string base = attachment->m_analysis;
            for (int number = 2; taken(attachment->m_analysis); ++number)
                attachment->m_analysis = base + "#" + std::to_string(number);
        }
    }
    attachment->setModel(model);
    attachment->snapshot(*model);
    attachment->link(*model, options.strict, false, context);

    // a second attach of the same tables is a re-attach: it replaces the first
    std::vector<std::shared_ptr<ResultAttachment>>& items = ModelResults::of(*model).mutableItems();
    const auto same = std::find_if(items.begin(), items.end(), [&](const std::shared_ptr<ResultAttachment>& item)
                                   { return item->m_tables == attachment->m_tables; });
    if (same != items.end())
        *same = attachment;
    else
        items.push_back(attachment);
    for (const std::shared_ptr<ResultTable>& table : attachment->m_tables->all())
        table->setAttachment(attachment);
    return attachment;
}

namespace
{

bool noFilter(const ResultFilter& filter) noexcept
{
    return !filter.subcases && !filter.tables && !filter.ids && !filter.elementIds && !filter.layers &&
           !filter.designCycles && !filter.variants && !filter.components && !filter.hasStepSelection();
}

} // namespace

std::shared_ptr<ResultAttachment> ResultAttachment::unchanged(const model::Model& model,
                                                              const std::shared_ptr<ResultTables>& tables,
                                                              const AttachOptions& options)
{
    const ModelResults* list = ModelResults::peek(model);
    if (list == nullptr || !tables || !noFilter(options.filter))
        return nullptr;
    for (const std::shared_ptr<ResultAttachment>& item : list->items())
    {
        if (item->m_tables != tables)
            continue;
        if (item->state(model) != AttachmentState::Current || !noFilter(item->m_filter) ||
            (options.analysis.empty() ? std::filesystem::path(item->m_file->path()).filename().string()
                                      : options.analysis) != item->m_analysis)
            return nullptr;
        if (options.strict &&
            std::any_of(item->m_diagnostics.begin(), item->m_diagnostics.end(), [](const base::Diagnostic& diagnostic)
                        { return diagnostic.severity == base::Severity::Error; }))
            return nullptr;
        for (const std::shared_ptr<ResultTable>& table : tables->all())
            if (table->attachment() != item)
                return nullptr;
        return item;
    }
    return nullptr;
}

bool ResultAttachment::labelInUse(const model::Model& model, const std::string& label, const ResultAttachment* except)
{
    const ModelResults* list = ModelResults::peek(model);
    if (list == nullptr)
        return false;
    return std::any_of(list->items().begin(), list->items().end(), [&](const std::shared_ptr<ResultAttachment>& item)
                       { return item.get() != except && item->m_analysis == label; });
}

void ResultAttachment::rename(const model::Model& model, const std::string& label)
{
    if (label.empty())
        throw base::UsageError("the label of a result set is not empty");
    if (labelInUse(model, label, this))
        throw base::UsageError("a result set labelled '" + label + "' is attached to the model already");
    m_analysis = label;
}

bool ResultAttachment::detach(model::Model& model, const std::shared_ptr<ResultAttachment>& attachment)
{
    if (ModelResults::peek(model) == nullptr)
        return false;
    std::vector<std::shared_ptr<ResultAttachment>>& items = ModelResults::of(model).mutableItems();
    const auto found = std::find(items.begin(), items.end(), attachment);
    if (found == items.end())
        return false;
    items.erase(found);
    for (const std::shared_ptr<ResultTable>& table : attachment->m_tables->all())
        if (table->attachment() == attachment)
            table->setAttachment(std::weak_ptr<ResultAttachment>());
    attachment->m_resolver = nullptr;
    return true;
}

void ResultAttachment::refresh(const model::Model& model, bool strict, const DecodeContext& context)
{
    link(model, strict, true, context);
    snapshot(model);
}

AttachmentState ResultAttachment::state(const model::Model& model) const noexcept
{
    if (model.structureVersion() != m_structureVersion)
        return AttachmentState::Stale;
    if (model.contentVersion() != m_contentVersion)
        return AttachmentState::Edited;
    return AttachmentState::Current;
}

bool ResultAttachment::consistent(const model::Model& model) const noexcept
{
    if (state(model) != AttachmentState::Current)
        return false;
    return std::none_of(m_diagnostics.begin(), m_diagnostics.end(),
                        [](const base::Diagnostic& diagnostic) { return diagnostic.severity != base::Severity::Info; });
}

// -------------------------------------------------------------------------------------------------------- linking

// Finds the card of every ID the tables talk about and writes what is not right as diagnostics. An attach finds the
// card by ID; a refresh keeps the card an ID was linked to (by its key: a renumbering moved the ID, not the card) when
// the card is still there, and does not look an ID up again, so that a card added later never takes the place of one
// that was deleted.
void ResultAttachment::link(const model::Model& model, bool strict, bool byKeyFirst, const DecodeContext& context)
{
    std::vector<std::shared_ptr<ResultTable>> tables;
    std::size_t skippedSuperelement = 0;
    for (const std::shared_ptr<ResultTable>& table : m_tables->all())
    {
        if (table->meta().superelement > 0)
            ++skippedSuperelement;
        else
            tables.push_back(table);
    }
    // The labels of the tables, a batch of tables at a time read in parallel, each table on one thread: what takes the
    // time is the reading of the file, and several tables at once keep the disk busy. A table whose labels are those of
    // the last table of its kind and element type shares them, so that the subcases of a large file hold one list.
    std::vector<TableLabels> labelled(tables.size());
    DecodeContext oneThread = context;
    oneThread.pool = nullptr;
    const std::size_t batch = context.pool != nullptr ? 2 * (context.pool->workerCount() + 1) : 1;
    std::map<std::pair<ResultKind, std::string>, TableLabels> lastOfKind;
    for (std::size_t first = 0; first < tables.size(); first += batch)
    {
        const std::size_t count = std::min(batch, tables.size() - first);
        const auto read = [&](std::size_t index)
        { labelled[first + index] = labelsOf(tables[first + index], oneThread); };
        if (context.pool != nullptr && count > 1)
            base::parallelFor(*context.pool, count, read, context.token);
        else
            for (std::size_t index = 0; index < count; ++index)
                read(index);
        for (std::size_t index = first; index < first + count; ++index)
        {
            TableLabels& labels = labelled[index];
            TableLabels& previous = lastOfKind[{labels.table->meta().kind, labels.table->meta().elementName}];
            if (previous.nodes && *previous.nodes == *labels.nodes)
                labels.nodes = previous.nodes;
            if (previous.elements && *previous.elements == *labels.elements)
                labels.elements = previous.elements;
            previous = labels;
        }
    }
    std::vector<std::int32_t> nodeIds;
    std::vector<std::int32_t> elementIds;
    {
        std::set<const std::vector<std::int32_t>*> seen;
        for (const TableLabels& labels : labelled)
        {
            if (seen.insert(labels.nodes.get()).second)
                nodeIds.insert(nodeIds.end(), labels.nodes->begin(), labels.nodes->end());
            if (seen.insert(labels.elements.get()).second)
                elementIds.insert(elementIds.end(), labels.elements->begin(), labels.elements->end());
        }
    }
    sortUnique(nodeIds);
    sortUnique(elementIds);

    const auto resolve =
        [&](const std::vector<std::int32_t>& ids, const std::vector<CardLink>& before, base::IdSpace space)
    {
        std::vector<CardLink> links;
        links.reserve(ids.size());
        for (const std::int32_t id : ids)
        {
            std::optional<model::CardLocation> location;
            if (byKeyFirst)
            {
                if (const CardLink* old = findLink(before, id))
                    location = model.locateKey(old->type, base::CardKey{old->key});
            }
            else
            {
                location = model.locate(space, id);
            }
            if (location)
                links.push_back(CardLink{id, keyOf(model, *location), location->table});
        }
        return links;
    };
    // An attach shares the links of another result set of the model whose IDs are the same, every one found, when the
    // model has not changed since that set was linked: they are the links it would make (a companion package: one file
    // per load case holds one list of links for all the files, not one a file). A refresh keeps the cards of its own
    // links.
    std::shared_ptr<const std::vector<CardLink>> sharedNodes;
    std::shared_ptr<const std::vector<CardLink>> sharedElements;
    if (!byKeyFirst)
    {
        const auto sameIds =
            [](const std::shared_ptr<const std::vector<CardLink>>& links, const std::vector<std::int32_t>& ids)
        {
            if (!links || links->size() != ids.size())
                return false;
            for (std::size_t index = 0; index < ids.size(); ++index)
                if ((*links)[index].resultId != ids[index])
                    return false;
            return true;
        };
        if (const ModelResults* list = ModelResults::peek(model))
            for (auto other = list->items().rbegin(); other != list->items().rend(); ++other)
            {
                if (other->get() == this || (*other)->state(model) != AttachmentState::Current)
                    continue;
                if (!sharedNodes && sameIds((*other)->m_nodes, nodeIds))
                    sharedNodes = (*other)->m_nodes;
                if (!sharedElements && sameIds((*other)->m_elements, elementIds))
                    sharedElements = (*other)->m_elements;
                if (sharedNodes && sharedElements)
                    break;
            }
    }
    // the links found before are read while the new ones are made, then replaced
    m_nodes = sharedNodes
                  ? sharedNodes
                  : std::make_shared<const std::vector<CardLink>>(resolve(nodeIds, nodeLinks(), base::IdSpace::Node));
    m_elements = sharedElements ? sharedElements
                                : std::make_shared<const std::vector<CardLink>>(
                                      resolve(elementIds, elementLinks(), base::IdSpace::Element));

    // the check
    m_diagnostics.clear();
    std::size_t reported = 0;
    std::size_t withheld = 0;
    const auto report = [&](base::Severity severity, const char* code, std::string message)
    {
        if (reported >= MAX_TABLE_DIAGNOSTICS)
        {
            ++withheld;
            return;
        }
        ++reported;
        base::Diagnostic diagnostic;
        diagnostic.severity = severity;
        diagnostic.code = code;
        diagnostic.message = std::move(message);
        m_diagnostics.push_back(std::move(diagnostic));
    };
    const model::CardRegistry& registry = model::CardRegistry::instance();
    // the check of a list of labels is the same for every table that shares it
    std::map<std::tuple<const void*, const void*, std::string_view>, LabelCheck> checks;
    std::map<std::string_view, std::size_t> cardCounts; // the cards of the model of each element type

    for (const TableLabels& labels : labelled)
    {
        const ResultTableMeta& meta = labels.table->meta();
        const std::string_view expected = elementCardOf(meta.elementName);
        auto [found, added] = checks.try_emplace({labels.nodes.get(), labels.elements.get(), expected});
        LabelCheck& check = found->second;
        if (added)
        {
            // the labels and the links are both sorted by ID: one walk along the two
            const auto walk = [](const std::vector<std::int32_t>& ids, const std::vector<CardLink>& links,
                                 const auto& onLink, std::vector<std::int32_t>& missing)
            {
                auto link = links.begin();
                for (const std::int32_t id : ids)
                {
                    while (link != links.end() && link->resultId < id)
                        ++link;
                    if (link == links.end() || link->resultId != id)
                        missing.push_back(id);
                    else
                        onLink(id, *link);
                }
            };
            walk(*labels.nodes, nodeLinks(), [](std::int32_t, const CardLink&) {}, check.missingNodes);
            // the card types of the element type of the table, by index
            std::vector<std::uint16_t> types;
            if (!expected.empty())
                for (std::uint16_t type = 1; type < registry.size(); ++type)
                    if (registry.entry(type).info.name == expected)
                        types.push_back(type);
            walk(
                *labels.elements, elementLinks(),
                [&](std::int32_t id, const CardLink& link)
                {
                    if (!expected.empty() && std::find(types.begin(), types.end(), link.type) == types.end())
                        check.wrongType.push_back(id);
                },
                check.missingElements);
        }
        const std::vector<std::int32_t>& missingNodes = check.missingNodes;
        const std::vector<std::int32_t>& missingElements = check.missingElements;
        const std::vector<std::int32_t>& wrongType = check.wrongType;
        const std::string what = describeTable(meta);
        if (!missingNodes.empty())
            report(base::Severity::Warning, "attach.node-missing",
                   "table " + what + ": " + std::to_string(missingNodes.size()) + " of " +
                       std::to_string(labels.nodes->size()) + " grid point IDs are not in the model (" +
                       idList(missingNodes) + ")");
        if (!missingElements.empty())
            report(base::Severity::Warning, "attach.element-missing",
                   "table " + what + ": " + std::to_string(missingElements.size()) + " of " +
                       std::to_string(labels.elements->size()) + " element IDs are not in the model (" +
                       idList(missingElements) + ")");
        if (!wrongType.empty())
            report(base::Severity::Error, "attach.element-type",
                   "table " + what + ": " + std::to_string(wrongType.size()) + " elements are not " +
                       std::string(expected) + " in the model (" + idList(wrongType) + ")");
        // a table that covers part of the cards of its type: a count that differs (the output was asked for a set, or
        // the model was edited after the run)
        if (!labels.elements->empty() && !expected.empty() && missingElements.empty() && wrongType.empty())
        {
            auto [counted, first] = cardCounts.try_emplace(expected, 0);
            if (first)
                for (std::uint16_t type = 1; type < registry.size(); ++type)
                    if (registry.entry(type).info.name == expected)
                        if (const model::CardTable* table = model.findTable(type))
                            counted->second += table->rows().aliveCount();
            const std::size_t inModel = counted->second;
            if (labels.elements->size() < inModel)
                report(base::Severity::Info, "attach.element-count",
                       "table " + what + ": results for " + std::to_string(labels.elements->size()) + " of the " +
                           std::to_string(inModel) + " " + std::string(expected) + " of the model");
            else if (labels.elements->size() > inModel)
                report(base::Severity::Warning, "attach.element-count",
                       "table " + what + ": results for " + std::to_string(labels.elements->size()) + " " +
                           std::string(expected) + " but the model has " + std::to_string(inModel));
        }
    }
    // the subcases of the results that the Case Control of the model does not have (a deck with no SUBCASE has none to
    // compare)
    {
        // the subcases of the results (a few), looked for in one walk along those of the deck (a deck may have
        // thousands)
        std::set<std::int32_t> missing;
        for (const TableLabels& labels : labelled)
            if (labels.table->meta().subcase > 0)
                missing.insert(labels.table->meta().subcase);
        bool deckHasSubcases = false;
        for (const model::Subcase& subcase : model.control().caseControl.segments())
        {
            // the subcases after an AUXCASE analyse an auxiliary model
            if (subcase.kind == model::SubcaseKind::AuxiliaryCase)
                break;
            if (subcase.kind != model::SubcaseKind::Global && subcase.kind != model::SubcaseKind::Step &&
                subcase.kind != model::SubcaseKind::Substep)
            {
                deckHasSubcases = true;
                if (subcase.id >= std::numeric_limits<std::int32_t>::min() &&
                    subcase.id <= std::numeric_limits<std::int32_t>::max())
                    missing.erase(static_cast<std::int32_t>(subcase.id));
            }
        }
        if (deckHasSubcases)
            for (const std::int32_t subcase : missing)
                report(base::Severity::Info, "attach.subcase-missing",
                       "subcase " + std::to_string(subcase) +
                           " of the results is not a subcase of the Case Control of the model");
    }
    if (skippedSuperelement > 0)
        report(base::Severity::Info, "attach.superelement",
               std::to_string(skippedSuperelement) + " tables of superelements are not linked to this model");
    if (withheld > 0)
    {
        base::Diagnostic diagnostic;
        diagnostic.severity = base::Severity::Info;
        diagnostic.code = "attach.more";
        diagnostic.message = "and " + std::to_string(withheld) + " more table diagnostics";
        m_diagnostics.push_back(std::move(diagnostic));
    }
    if (strict)
    {
        for (const base::Diagnostic& diagnostic : m_diagnostics)
            if (diagnostic.severity == base::Severity::Error)
                throw base::DeckError(diagnostic);
    }
}

// -------------------------------------------------------------------------------------------------------- lookups

std::optional<model::CardLocation> ResultAttachment::nodeLocation(const model::Model& model,
                                                                  std::int32_t resultId) const noexcept
{
    const CardLink* link = findLink(nodeLinks(), resultId);
    return link != nullptr ? model.locateKey(link->type, base::CardKey{link->key}) : std::nullopt;
}

std::optional<model::CardLocation> ResultAttachment::elementLocation(const model::Model& model,
                                                                     std::int32_t resultId) const noexcept
{
    const CardLink* link = findLink(elementLinks(), resultId);
    return link != nullptr ? model.locateKey(link->type, base::CardKey{link->key}) : std::nullopt;
}

std::int32_t ResultAttachment::modelNodeId(const model::Model& model, std::int32_t resultId) const noexcept
{
    const std::optional<model::CardLocation> location = nodeLocation(model, resultId);
    return location ? model.findTable(location->table)->idAt(location->row) : 0;
}

std::int32_t ResultAttachment::modelElementId(const model::Model& model, std::int32_t resultId) const noexcept
{
    const std::optional<model::CardLocation> location = elementLocation(model, resultId);
    return location ? model.findTable(location->table)->idAt(location->row) : 0;
}

std::vector<std::int32_t> ResultAttachment::resultIdsOf(std::uint16_t type, std::uint32_t key) const
{
    std::vector<std::int32_t> ids;
    for (const std::vector<CardLink>* links : {&nodeLinks(), &elementLinks()})
        for (const CardLink& link : *links)
            if (link.type == type && link.key == key)
                ids.push_back(link.resultId);
    return ids;
}

} // namespace oor::results
