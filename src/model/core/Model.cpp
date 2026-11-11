#include "oor/model/core/Model.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/superelements/superelementBulkOptions/SuperelementBulkOptionsTable.hpp"

#include <algorithm>
#include <atomic>
#include <utility>

namespace oor::model
{

void Model::restoreNextKey(std::uint32_t next)
{
    if (next < m_nextKey)
        throw base::UsageError("the next card key cannot move backwards");
    for (const auto& table : m_tables)
        if (table && table->rows().rowCount() &&
            table->rows().keyAt(table->rows().rowCount() - 1).value >= next)
            throw base::UsageError("the next card key must follow every stored card key");
    m_nextKey = next;
}

namespace
{

// One counter for all models: a content version is never given twice.
std::uint64_t nextContentVersion() noexcept
{
    static std::atomic<std::uint64_t> counter{1};
    return counter.fetch_add(1) + 1;
}

// The superelement that superelement `seid` copies (the RSEID of an SEBULK of type REPEAT or MIRROR), 0 when it is not
// a copy. A copy has no part of its own: its cards name the IDs of the part it copies.
base::EntityId copiedSuperelement(const Model& root, base::EntityId seid) noexcept
{
    const SuperelementBulkOptionsTable* bulks = root.findTable<SuperelementBulkOptionsTable>();
    if (bulks == nullptr)
        return 0;
    for (std::uint32_t row = 0; row < bulks->rows().rowCount(); ++row)
    {
        if (!bulks->rows().isAlive(row) || bulks->idAtRow(row) != seid)
            continue;
        const SuperelementBulkOptionsCard card = bulks->card(row);
        if (card.partType == SuperelementBulkOptionsType::Repeat ||
            card.partType == SuperelementBulkOptionsType::Mirror)
            return card.referenceSuperelementId;
    }
    return 0;
}

// How many copies of copies are followed before the search gives up (a loop of copies has no part).
constexpr int MAX_COPY_STEPS = 8;

} // namespace

void Model::noteContentChange() noexcept
{
    m_contentVersion = nextContentVersion();
}

const std::unordered_set<base::EntityId>& Model::setIdsOf(CardCategory category) const
{
    if (m_setIds.version != m_contentVersion)
    {
        m_setIds.ids.clear();
        m_setIds.version = m_contentVersion;
    }
    auto found = m_setIds.ids.find(category);
    if (found != m_setIds.ids.end())
        return found->second;
    std::unordered_set<base::EntityId> ids;
    const CardRegistry& registry = CardRegistry::instance();
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        const CardTable* table = findTable(type);
        // A card with no ID (SUPORT among the constraints, ASET among the sets) is in no set.
        if (table == nullptr || registry.entry(type).info.category != category ||
            registry.entry(type).info.idSpace == base::IdSpace::None)
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (table->rows().isAlive(row))
                ids.insert(table->idAt(row));
        }
    }
    return m_setIds.ids.emplace(category, std::move(ids)).first->second;
}

void Model::noteSetIdAdded(const CardTypeInfo& info, base::EntityId id)
{
    if (info.idSpace != base::IdSpace::None)
    {
        if (auto found = m_setIds.ids.find(info.category); found != m_setIds.ids.end())
            found->second.insert(id);
    }
    m_setIds.version = m_contentVersion;
}

void Model::noteControlChange() noexcept
{
    m_controlVersion = nextContentVersion();
}

Model::Model()
    : m_tables(CardRegistry::instance().size()), m_controlVersion(nextContentVersion()), m_lineage(nextContentVersion())
{
}

ControlSections& Model::control()
{
    if (m_control.use_count() > 1)
        m_control = std::make_shared<ControlSections>(*m_control);
    noteControlChange(); // the caller may edit them
    return *m_control;
}

Model::Model(const Model& other)
    : m_spaceIndexes(other.m_spaceIndexes),
      m_typeIndexes(other.m_typeIndexes),
      m_nextKey(other.m_nextKey),
      m_structureVersion(other.m_structureVersion),
      m_contentVersion(other.m_contentVersion),
      m_controlVersion(other.m_controlVersion),
      m_lineage(other.m_lineage),
      m_derived(other.m_derived),
      m_contentCache(other.m_contentCache),
      m_results(other.m_results),
      m_diagnostics(other.m_diagnostics),
      m_control(other.m_control),
      m_documents(other.m_documents),
      m_files(other.m_files),
      m_markers(other.m_markers),
      m_comments(other.m_comments),
      m_placements(other.m_placements),
      m_partInfo(other.m_partInfo),
      m_residualAfterParts(other.m_residualAfterParts)
{
    m_tables.reserve(other.m_tables.size());
    for (const std::unique_ptr<CardTable>& table : other.m_tables)
        m_tables.push_back(table ? table->clone() : nullptr);
    for (const std::unique_ptr<Model>& part : other.m_parts)
        m_parts.push_back(std::make_unique<Model>(*part));
    adoptParts();
}

// Every member is moved by name: a model that is moved must tell its parts where it is now (the parts keep their
// addresses), and a member added to the class has to be added here too.
Model::Model(Model&& other) noexcept
    : m_tables(std::move(other.m_tables)),
      m_spaceIndexes(std::move(other.m_spaceIndexes)),
      m_typeIndexes(std::move(other.m_typeIndexes)),
      m_nextKey(other.m_nextKey),
      m_structureVersion(other.m_structureVersion),
      m_contentVersion(other.m_contentVersion),
      m_controlVersion(other.m_controlVersion),
      m_lineage(other.m_lineage),
      m_derived(std::move(other.m_derived)),
      m_contentCache(std::move(other.m_contentCache)),
      m_results(std::move(other.m_results)),
      m_diagnostics(std::move(other.m_diagnostics)),
      m_control(std::move(other.m_control)),
      m_documents(std::move(other.m_documents)),
      m_files(std::move(other.m_files)),
      m_markers(std::move(other.m_markers)),
      m_comments(std::move(other.m_comments)),
      m_placements(std::move(other.m_placements)),
      m_partInfo(std::move(other.m_partInfo)),
      m_parts(std::move(other.m_parts)),
      m_residualAfterParts(std::move(other.m_residualAfterParts))
{
    adoptParts();
}

// The owner of this model stays as it is: replaceContents puts new contents in a part that its owner keeps.
Model& Model::operator=(Model&& other) noexcept
{
    if (this == &other)
        return *this;
    m_tables = std::move(other.m_tables);
    m_spaceIndexes = std::move(other.m_spaceIndexes);
    m_typeIndexes = std::move(other.m_typeIndexes);
    m_nextKey = other.m_nextKey;
    m_structureVersion = other.m_structureVersion;
    m_contentVersion = other.m_contentVersion;
    m_controlVersion = other.m_controlVersion;
    m_lineage = other.m_lineage;
    m_derived = std::move(other.m_derived);
    m_contentCache = std::move(other.m_contentCache);
    m_results = std::move(other.m_results);
    m_diagnostics = std::move(other.m_diagnostics);
    m_control = std::move(other.m_control);
    m_documents = std::move(other.m_documents);
    m_files = std::move(other.m_files);
    m_markers = std::move(other.m_markers);
    m_comments = std::move(other.m_comments);
    m_placements = std::move(other.m_placements);
    m_partInfo = std::move(other.m_partInfo);
    m_parts = std::move(other.m_parts);
    m_residualAfterParts = std::move(other.m_residualAfterParts);
    adoptParts();
    return *this;
}

void Model::adoptParts() noexcept
{
    for (const std::unique_ptr<Model>& part : m_parts)
        part->m_owner = this;
}

const std::map<std::string, std::string>& Model::documents() const noexcept
{
    static const std::map<std::string, std::string> EMPTY;
    return m_documents ? *m_documents : EMPTY;
}

void Model::setDocument(std::string name, std::string text)
{
    if (name.empty() || name.size() > 255 || name.find('\0') != std::string::npos)
        throw base::UsageError("a document name must contain 1 to 255 bytes without a zero byte");
    if (const auto found = documents().find(name); found != documents().end() && found->second == text)
        return;
    auto next = std::make_shared<std::map<std::string, std::string>>(documents());
    next->insert_or_assign(std::move(name), std::move(text));
    m_documents = std::move(next);
}

bool Model::removeDocument(const std::string& name)
{
    if (!documents().contains(name))
        return false;
    auto next = std::make_shared<std::map<std::string, std::string>>(documents());
    next->erase(name);
    m_documents = std::move(next);
    return true;
}

Model& Model::operator=(const Model& other)
{
    if (this != &other)
    {
        Model copy(other);
        *this = std::move(copy);
    }
    return *this;
}

CardTable& Model::ensureTable(std::uint16_t typeIndex)
{
    std::unique_ptr<CardTable>& slot = m_tables[typeIndex];
    if (!slot)
        slot = CardRegistry::instance().entry(typeIndex).factory();
    return *slot;
}

// ------------------------------------------------------------------------------------------------ ID indexes

IdIndex* Model::indexFor(std::uint16_t type, base::IdSpace space) noexcept
{
    if (space == base::IdSpace::Group || space == base::IdSpace::None)
        return nullptr;
    if (space == base::IdSpace::PerCardType)
    {
        if (m_typeIndexes.empty())
            m_typeIndexes.resize(CardRegistry::instance().size());
        return &m_typeIndexes[type];
    }
    return &m_spaceIndexes[static_cast<std::size_t>(space)];
}

const IdIndex* Model::indexFor(std::uint16_t type, base::IdSpace space) const noexcept
{
    if (space == base::IdSpace::Group || space == base::IdSpace::None)
        return nullptr;
    if (space == base::IdSpace::PerCardType)
        return m_typeIndexes.empty() ? nullptr : &m_typeIndexes[type];
    return &m_spaceIndexes[static_cast<std::size_t>(space)];
}

std::optional<CardLocation> Model::locate(base::IdSpace space, base::EntityId id) const noexcept
{
    const IdIndex* index = indexFor(0, space);
    return index != nullptr ? index->find(id) : std::nullopt;
}

std::optional<CardLocation> Model::locateInType(std::uint16_t typeIndex, base::EntityId id) const noexcept
{
    const IdIndex* index = indexFor(typeIndex, base::IdSpace::PerCardType);
    return index != nullptr ? index->find(id) : std::nullopt;
}

std::optional<CardLocation> Model::locateKey(std::uint16_t typeIndex, base::CardKey key) const noexcept
{
    const CardTable* table = m_tables[typeIndex].get();
    if (table == nullptr)
        return std::nullopt;
    const std::int64_t row = table->rows().findLiveRow(key);
    if (row < 0)
        return std::nullopt;
    return CardLocation{typeIndex, static_cast<std::uint32_t>(row)};
}

void Model::checkFree(std::uint16_t type, base::IdSpace space, base::EntityId id) const
{
    const IdIndex* index = indexFor(type, space);
    if (index == nullptr || !index->find(id))
        return;
    throw base::DuplicateIdError("ID " + std::to_string(id) + " is already used by another card in the " +
                                 std::string(base::idSpaceName(space)) + " ID space");
}

void Model::indexInsert(std::uint16_t type, base::IdSpace space, base::EntityId id, CardLocation location)
{
    if (IdIndex* index = indexFor(type, space))
        index->insert(id, location);
}

void Model::reserveIds(std::uint16_t type, std::size_t additional)
{
    if (IdIndex* index = indexFor(type, CardRegistry::instance().entry(type).info.idSpace))
        index->reserve(index->size() + additional);
}

void Model::registerRead(CardLocation location, base::EntityId id, base::DiagnosticSink& sink,
                         const std::string& fileName, std::uint32_t line)
{
    const CardTypeInfo& info = CardRegistry::instance().entry(location.table).info;
    noteContentChange();
    IdIndex* index = indexFor(location.table, info.idSpace);
    if (index == nullptr)
        return;
    const std::optional<CardLocation> previous = index->insert(id, location);
    if (info.idSpace == base::IdSpace::Element)
        noteStructureChange();
    if (!previous)
        return;
    if (info.repeatsAllowed && previous->table == location.table)
    {
        m_tables[previous->table]->rows().kill(previous->row); // the same object defined again: one row stays
        return;
    }
    const base::SourceLocation where{fileName, line, std::string(info.name), ""};
    const std::string message = "ID " + std::to_string(id) + " is used by two cards in the " +
                                std::string(base::idSpaceName(info.idSpace)) + " ID space";
    if (sink.mode() == base::ReadMode::Strict)
    {
        sink.error("deck.duplicate-id", message, where);
    }
    else
    {
        sink.warning("deck.duplicate-id", message + "; the last one is kept", where);
        m_tables[previous->table]->rows().kill(previous->row); // the last card wins
    }
}

void Model::remove(CardLocation location)
{
    CardTable* table = m_tables[location.table].get();
    if (table == nullptr || !table->rows().isAlive(location.row))
        return;
    const CardTypeInfo& info = table->info();
    if (IdIndex* index = indexFor(location.table, info.idSpace))
    {
        const base::EntityId id = table->idAt(location.row);
        const std::optional<CardLocation> held = index->find(id);
        if (held && *held == location)
            index->erase(id);
    }
    table->rows().kill(location.row);
    if (info.idSpace == base::IdSpace::Element)
        noteStructureChange();
    else
        noteContentChange();
}

void Model::setId(CardLocation location, base::EntityId id)
{
    CardTable& table = *m_tables[location.table];
    const CardTypeInfo& info = table.info();
    const base::EntityId old = table.idAt(location.row);
    if (old == id)
        return;
    IdIndex* index = indexFor(location.table, info.idSpace);
    if (index != nullptr)
    {
        if (index->find(id))
            throw base::DuplicateIdError("ID " + std::to_string(id) + " is already used by another card in the " +
                                         std::string(base::idSpaceName(info.idSpace)) + " ID space");
        index->erase(old);
        index->insert(id, location);
    }
    table.setIdAt(location.row, id);
    noteContentChange();
}

void Model::rebuildIndexes()
{
    for (IdIndex& index : m_spaceIndexes)
        index.clear();
    for (IdIndex& index : m_typeIndexes)
        index.clear();
    for (std::size_t type = 0; type < m_tables.size(); ++type)
    {
        const CardTable* table = m_tables[type].get();
        if (table == nullptr)
            continue;
        const base::IdSpace space = table->info().idSpace;
        IdIndex* index = indexFor(static_cast<std::uint16_t>(type), space);
        if (index == nullptr)
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (table->rows().isAlive(row))
                index->insert(table->idAt(row), CardLocation{static_cast<std::uint16_t>(type), row});
        }
    }
}

namespace
{

// Runs finishRead on every table of a model and of its parts, in registry order (as the reader does).
void finishAllTables(Model& model, const ReadContext& context)
{
    for (std::uint16_t type = 1; type < CardRegistry::instance().size(); ++type)
    {
        if (CardTable* table = model.findTable(type))
            table->finishRead(model, context);
    }
    for (const std::unique_ptr<Model>& part : model.parts())
        finishAllTables(*part, context);
}

} // namespace

void Model::applyDefaults()
{
    base::DiagnosticSink problems(base::ReadMode::Tolerant);
    finishAllTables(*this, ReadContext{problems});
    noteContentChange(); // the default cards may have changed any card they govern
    // What a table finds wrong when the whole Bulk Data is read (a second GRDSET, a matrix column with no header) is
    // reported with its severity: a warning does not stop the read, but it is not dropped either.
    if (problems.diagnostics().empty())
        return;
    if (problems.hasErrors() && isStrict())
        throw base::DeckError(*problems.firstError());
    m_diagnostics.append(std::move(problems));
}

void Model::replaceContents(Model&& other)
{
    std::vector<std::unique_ptr<Model>> keptParts = std::move(m_parts);
    std::vector<std::unique_ptr<Model>> otherParts = std::move(other.m_parts);
    *this = std::move(other);
    if (keptParts.size() == otherParts.size())
    {
        for (std::size_t index = 0; index < keptParts.size(); ++index)
            keptParts[index]->replaceContents(std::move(*otherParts[index]));
        m_parts = std::move(keptParts);
    }
    else
    {
        m_parts = std::move(otherParts);
    }
    adoptParts();
}

void Model::compact()
{
    for (const std::unique_ptr<CardTable>& table : m_tables)
    {
        if (table && table->rows().aliveCount() != table->rows().rowCount())
            table->compact();
    }
    rebuildIndexes();
    noteStructureChange(); // the rows moved
    for (const std::unique_ptr<Model>& part : m_parts)
        part->compact();
}

std::size_t Model::cardCount() const noexcept
{
    std::size_t count = 0;
    for (const std::unique_ptr<CardTable>& table : m_tables)
    {
        if (table)
            count += table->rows().aliveCount();
    }
    return count;
}

// --------------------------------------------------------------------------------------- comments and parts

std::string Model::commentOf(base::CardKey key) const
{
    const auto found =
        std::lower_bound(m_comments.begin(), m_comments.end(), key,
                         [](const CommentRecord& record, base::CardKey value) { return record.key < value; });
    return found != m_comments.end() && found->key == key ? found->text : std::string();
}

void Model::setComment(base::CardKey key, std::string text)
{
    auto found = std::lower_bound(m_comments.begin(), m_comments.end(), key,
                                  [](const CommentRecord& record, base::CardKey value) { return record.key < value; });
    if (found != m_comments.end() && found->key == key)
    {
        if (text.empty())
            m_comments.erase(found);
        else
            found->text = std::move(text);
    }
    else if (!text.empty())
    {
        m_comments.insert(found, CommentRecord{key, std::move(text)});
    }
}

Model& Model::addPart(PartInfo info)
{
    auto part = std::make_unique<Model>();
    part->m_partInfo = std::move(info);
    part->m_diagnostics = base::DiagnosticSink(m_diagnostics.mode());
    part->m_owner = this;
    m_parts.push_back(std::move(part));
    return *m_parts.back();
}

const Model* Model::findPart(std::string_view kind, std::int64_t id, std::int64_t massId) const noexcept
{
    for (const std::unique_ptr<Model>& part : root().m_parts)
    {
        const PartInfo& info = *part->m_partInfo;
        if (info.kind == kind && info.id == id && info.massId == massId)
            return part.get();
    }
    return nullptr;
}

Model* Model::findPart(std::string_view kind, std::int64_t id, std::int64_t massId) noexcept
{
    return const_cast<Model*>(std::as_const(*this).findPart(kind, id, massId));
}

const Model& Model::superelementModel(base::EntityId seid) const noexcept
{
    // A REPEAT or MIRROR superelement has no part: the grid points its SELOC and SECONCT name are those of the
    // superelement it copies, so the search goes on there.
    for (int step = 0; seid > 0 && step < MAX_COPY_STEPS; ++step)
    {
        if (const Model* part = findPart("SuperelementPart", seid))
            return *part;
        seid = copiedSuperelement(root(), seid);
    }
    return root();
}

Model& Model::superelementModel(base::EntityId seid) noexcept
{
    return const_cast<Model&>(std::as_const(*this).superelementModel(seid));
}

void Model::setPartId(std::int64_t id)
{
    if (m_partInfo)
        m_partInfo->id = id;
}

std::string Model::partLabel() const
{
    return m_partInfo ? m_partInfo->kind + " " + std::to_string(m_partInfo->id) : std::string{};
}

} // namespace oor::model
