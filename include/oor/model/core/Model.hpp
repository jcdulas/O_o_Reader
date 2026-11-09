#pragma once

#include "oor/base/diagnostics/DiagnosticSink.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/base/ids/CardKey.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/base/ids/IdSpace.hpp"
#include "oor/model/control/case_control/CaseControl.hpp"
#include "oor/model/control/executive_control/ExecutiveControl.hpp"
#include "oor/model/control/file_management/FileManagement.hpp"
#include "oor/model/core/CardRegistry.hpp"
#include "oor/model/core/CardTable.hpp"
#include "oor/model/storage/IdIndex.hpp"
#include "oor/model/storage/TableAttachment.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace oor::model
{

// A model source file (descriptor only: its text is not kept).
struct SourceFileInfo
{
    std::filesystem::path path;
    std::string includeText; // the Include path that opened it (empty for the main file)
    std::int32_t parent = -1;
    std::uint32_t includeLine = 0;
};

enum class SourceMarkerKind : std::uint8_t
{
    IncludeBegin, // an INCLUDE statement: the cards of the included file follow
    IncludeEnd,   // the end of an included file
    Comments      // comment lines that no card follows: the end of a file or of a section
};

// What sits between the cards of a Bulk Data section, before the first card whose key is >= atKey: an INCLUDE, the end
// of an included file, or comment lines after the last card of a file, a part or the section. Markers with the same key
// are in the order of the deck.
struct SourceMarker
{
    std::uint32_t atKey = 0;
    SourceMarkerKind kind = SourceMarkerKind::IncludeBegin;
    std::string text;            // the INCLUDE statement as written, or the comment lines (joined with '\n', no '$')
    std::uint32_t fileIndex = 0; // the included file (IncludeBegin only)
};

// The comment lines before a card, joined with newlines without their leading #.
struct CommentRecord
{
    base::CardKey key;
    std::string text;
};

// A model part with its own identifier spaces.
struct PartInfo
{
    std::string kind;
    std::int64_t id = 0;
    std::int64_t massId = 0;
    bool appends = false;
};

struct ControlSections
{
    FileManagement fileManagement;
    ExecutiveControl executive;
    CaseControl caseControl;
};

// A model: the cards of a deck, stored by card type in column tables, with their ID indexes, the control sections,
// the diagnostics of its reading and the parts of the deck.
//
// Cards are identified by a card key, given when the card is created and never reused; the entity ID is a field of
// the card and can change (renumbering). The tables are sorted by key, which is creation order, which in a file is
// file order: that is the order cards are written in.
//
// Thread safety: concurrent reads are fine; any change needs exclusive access.
class Model
{
public:
    Model();
    Model(const Model& other);
    Model& operator=(const Model& other);
    // A moved model takes its parts with it: they are told who holds them now.
    Model(Model&& other) noexcept;
    Model& operator=(Model&& other) noexcept;
    ~Model() = default;

    // An independent model, made with no work per card: the column buffers, the ID indexes, the row books, the control
    // sections and the links of the attached results are shared, each cloned by the first side that edits it (the
    // comments of the cards and the few card types kept as whole cards are copied).
    [[nodiscard]] Model copy() const { return Model(*this); }

    // --- tables -----------------------------------------------------------------------------------------------

    [[nodiscard]] static std::size_t typeCount() { return CardRegistry::instance().size(); }
    // The table of a card type; null while no card of that type exists.
    [[nodiscard]] CardTable* findTable(std::uint16_t typeIndex) noexcept { return m_tables[typeIndex].get(); }
    [[nodiscard]] const CardTable* findTable(std::uint16_t typeIndex) const noexcept
    {
        return m_tables[typeIndex].get();
    }
    CardTable& ensureTable(std::uint16_t typeIndex);

    template <typename TableT>
    TableT& table()
    {
        return static_cast<TableT&>(ensureTable(typeIndexOf<TableT>()));
    }
    template <typename TableT>
    [[nodiscard]] const TableT* findTable() const noexcept
    {
        return static_cast<const TableT*>(m_tables[typeIndexOf<TableT>()].get());
    }

    // --- IDs --------------------------------------------------------------------------------------------------

    // Where the card with this ID is, in an ID space shared by several card types.
    [[nodiscard]] std::optional<CardLocation> locate(base::IdSpace space, base::EntityId id) const noexcept;
    // The same for a card type whose IDs are unique per type only (IdSpace::PerCardType).
    [[nodiscard]] std::optional<CardLocation> locateInType(std::uint16_t typeIndex, base::EntityId id) const noexcept;
    // The location of the live card with `key` in a table, or nullopt when it is dead or gone.
    [[nodiscard]] std::optional<CardLocation> locateKey(std::uint16_t typeIndex, base::CardKey key) const noexcept;

    [[nodiscard]] base::CardKey newKey() noexcept { return base::CardKey{m_nextKey++}; }

    // --- adding, removing, renumbering -------------------------------------------------------------------------

    // Adds a card. The table class gives the card type (TableT::CardType, TableT::idOf, TableT::append). Throws
    // DuplicateIdError when the ID is already used in its ID space; a failed add leaves no trace.
    template <typename TableT>
    CardLocation add(const typename TableT::CardType& card)
    {
        const std::uint16_t type = typeIndexOf<TableT>();
        const CardTypeInfo& info = CardRegistry::instance().entry(type).info;
        const base::EntityId id = TableT::cardId(card);
        checkFree(type, info.idSpace, id);
        TableT& target = table<TableT>();
        const std::uint32_t row = target.append(card, newKey());
        const CardLocation location{type, row};
        indexInsert(type, info.idSpace, id, location);
        const bool setIdsCurrent = m_setIds.version == m_contentVersion;
        if (info.idSpace == base::IdSpace::Element)
            noteStructureChange();
        else
            noteContentChange();
        if (setIdsCurrent)
            noteSetIdAdded(info, id);
        return location;
    }

    // Makes room in the ID index of a card type for `additional` more cards, so that adding them one by one does not
    // rehash the index again and again (batch construction). Changes nothing visible.
    void reserveIds(std::uint16_t type, std::size_t additional);

    // Removes a card (tombstone: other rows keep their place until compact()).
    void remove(CardLocation location);

    // Gives a card another entity ID. Throws DuplicateIdError when it is taken.
    void setId(CardLocation location, base::EntityId id);

    // Drops the dead rows of every table and rebuilds the ID indexes. Row numbers change; card keys do not.
    void compact();

    // Registers a card read from a deck (used by ModelFileReader): a duplicate ID is an error in strict mode, and in
    // tolerant mode a warning where the last card wins.
    void registerRead(CardLocation location, base::EntityId id, base::DiagnosticSink& sink, const std::string& fileName,
                      std::uint32_t line);

    // Rebuilds the ID indexes from the tables (after the IDs of many cards were changed by a renumbering).
    void reindex() { rebuildIndexes(); }

    // Gives the cards that still note blank fields the defaults of the card that governs them (GRDSET for GRID, BAROR
    // for CBAR, BEAMOR for CBEAM), in this model and in each of its parts: it runs the post-read hook
    // (CardTable::finishRead) again, so that a model built or edited in memory gets the defaults of a default card
    // added after its cards. The cards that note blank fields are those read from a deck while no default card existed
    // and those added with an omitted field (`add_grid` without `cp`); a field that holds a value, zero included, is
    // never changed, and a card that was given its defaults notes nothing any more. A second default card of a kind is
    // an error: it is reported in the diagnostics of a tolerant model and thrown (DeckError) by a strict one, after the
    // first card of that kind has been applied.
    void applyDefaults();

    // Puts the contents of `other` in this model (the way a failed operation is undone: `other` is a copy taken before
    // it). The model object stays the same, and so do its parts when their number is unchanged, so that handles and
    // Python objects that refer to them stay valid.
    void replaceContents(Model&& other);

    // The IDs of the live cards of a category that have one (the set IDs of the Group space among them: what a
    // reference to a load or constraint set looks for), made once for a contentVersion() and kept up to date by add(),
    // so that adding cards one by one that refer to sets (add_load) makes no pass over the cards of the model each
    // time: any other change makes them again at the next call. A cache filled from a const model: not for concurrent
    // callers (the reference check of a card being added reads it).
    [[nodiscard]] const std::unordered_set<base::EntityId>& setIdsOf(CardCategory category) const;

    // Number of live cards of all types.
    [[nodiscard]] std::size_t cardCount() const noexcept;

    // --- what the analysis layer keeps with the model --------------------------------------------------------------

    // A number that changes whenever the connectivity of the model may have changed: an element was added, removed or
    // had its nodes edited, the tables were compacted, nodes or elements were renumbered, a deck was read. Editing
    // a coordinate or any other field does not change it. A reader that derived something from the connectivity (the
    // node to element index, the tables of edges and faces) compares it with the value it saw.
    [[nodiscard]] std::uint64_t structureVersion() const noexcept { return m_structureVersion; }
    // Says that the connectivity changed (the editing layer calls it when it changes the nodes of an element). A change
    // of connectivity is a change of content.
    void noteStructureChange() noexcept
    {
        ++m_structureVersion;
        noteContentChange();
    }

    // A number that identifies the content of the cards: it changes whenever a card is added, removed, renumbered or
    // edited, the tables are compacted, a deck is read or the contents are replaced, and for nothing else (the Case
    // Control and the diagnostics are not cards). The number comes from one counter for all models, so two states of
    // content never share one: a reader that derived something from the cards (the revision stamps of the components)
    // keeps the number it saw and recomputes when it differs. A code that changes a card through the setters of its
    // table, bypassing the editing layer, must call noteContentChange() itself.
    [[nodiscard]] std::uint64_t contentVersion() const noexcept { return m_contentVersion; }
    void noteContentChange() noexcept;

    // A number that identifies the state of the control sections (the Executive Control, the Case Control, the File
    // Management): it changes each time the non-const control() gives them (an edit may follow), and is copied with
    // the model, which shares them. It comes from the counter of contentVersion(): two states never share one. A
    // reader that derived something from the Case Control (the NSM set it selects) keeps the number it saw and looks
    // again when it differs. A reader takes the sections through the const control() (std::as_const on a model that
    // is not const), which leaves the number as it is; a code that keeps the reference the non-const control() gave and
    // edits through it after a query must call it again (or noteControlChange()) to tell the change.
    [[nodiscard]] std::uint64_t controlVersion() const noexcept { return m_controlVersion; }
    void noteControlChange() noexcept;

    // The identity of a family of models: a model and the copies made of it (copy(), the copy constructor) have the
    // same lineage, two models read or made separately have different ones. Card keys mean the same thing within a
    // lineage (a copy keeps the key of every card), which is what a comparison of two models by card key relies on.
    [[nodiscard]] std::uint64_t lineage() const noexcept { return m_lineage; }
    // The place where a higher layer keeps what it derived from the connectivity. It is copied with the model (the copy
    // has its own, in the state of the original) and mutable on purpose: it is a cache, filled when a const model is
    // queried.
    [[nodiscard]] AttachmentSlot& derivedSlot() const noexcept { return m_derived; }
    // The place where a higher layer keeps what it derived from the content of the cards (the reference index, the
    // components and their stamps), valid for one contentVersion(); copied with the model like derivedSlot().
    [[nodiscard]] AttachmentSlot& contentCacheSlot() const noexcept { return m_contentCache; }
    // The place where the results layer keeps the result sets attached to the model (links by card key, snapshots of
    // the versions above). A copy of the model has its own list, with the same result sets (shared, not copied) and the
    // links in the state of the original.
    [[nodiscard]] AttachmentSlot& resultsSlot() const noexcept { return m_results; }

    // --- structure ----------------------------------------------------------------------------------------------

    [[nodiscard]] base::DiagnosticSink& diagnostics() noexcept { return m_diagnostics; }
    [[nodiscard]] const base::DiagnosticSink& diagnostics() const noexcept { return m_diagnostics; }
    [[nodiscard]] bool isStrict() const noexcept { return m_diagnostics.mode() == base::ReadMode::Strict; }
    void setStrict(bool strict)
    {
        m_diagnostics = base::DiagnosticSink(strict ? base::ReadMode::Strict : base::ReadMode::Tolerant);
    }

    // The control sections are shared by the copies of a model until one side asks for them to change: the non-const
    // control() clones them first when a copy shares them, so a reference it gives is good until the model is copied.
    [[nodiscard]] ControlSections& control();
    [[nodiscard]] const ControlSections& control() const noexcept { return *m_control; }
    [[nodiscard]] std::vector<SourceFileInfo>& files() noexcept { return m_files; }
    [[nodiscard]] const std::vector<SourceFileInfo>& files() const noexcept { return m_files; }
    [[nodiscard]] std::vector<SourceMarker>& markers() noexcept { return m_markers; }
    [[nodiscard]] const std::vector<SourceMarker>& markers() const noexcept { return m_markers; }
    [[nodiscard]] std::vector<CommentRecord>& comments() noexcept { return m_comments; }
    [[nodiscard]] const std::vector<CommentRecord>& comments() const noexcept { return m_comments; }
    // The comment lines before the card with this key, or empty.
    [[nodiscard]] std::string commentOf(base::CardKey key) const;
    void setComment(base::CardKey key, std::string text);

    // Cards added to a named source file: a card made by `add_*` is written at the end of the main
    // Bulk Data unless it is placed. card key -> index in files(); used when the INCLUDE structure is written.
    [[nodiscard]] std::unordered_map<std::uint32_t, std::uint32_t>& placements() noexcept { return m_placements; }
    [[nodiscard]] const std::unordered_map<std::uint32_t, std::uint32_t>& placements() const noexcept
    {
        return m_placements;
    }

    // Parts: models of their own, in file order. The residual structure is this model.
    [[nodiscard]] const std::optional<PartInfo>& partInfo() const noexcept { return m_partInfo; }
    // The neutral part kind and identifier used by diagnostics; empty for the main model.
    [[nodiscard]] std::string partLabel() const;
    [[nodiscard]] bool isPart() const noexcept { return m_partInfo.has_value(); }
    Model& addPart(PartInfo info);
    [[nodiscard]] std::vector<std::unique_ptr<Model>>& parts() noexcept { return m_parts; }
    [[nodiscard]] const std::vector<std::unique_ptr<Model>>& parts() const noexcept { return m_parts; }

    // The model that holds this part (the residual structure of the deck), null for the residual structure and for a
    // model that is not part of a deck. A copy of a part is a model of its own: it has no owner.
    [[nodiscard]] Model* owner() noexcept { return m_owner; }
    [[nodiscard]] const Model* owner() const noexcept { return m_owner; }
    // The residual structure of the deck this model belongs to: the owner of a part, the model itself otherwise.
    [[nodiscard]] Model& root() noexcept { return m_owner != nullptr ? *m_owner : *this; }
    [[nodiscard]] const Model& root() const noexcept { return m_owner != nullptr ? *m_owner : *this; }

    // The part `BEGIN <kind>=id` (with `MASSID=massId` for a part superelement that carries one) among the parts of the
    // root of this model, or null. `kind` is the neutral name of the kind (PartInfo::kind).
    [[nodiscard]] Model* findPart(std::string_view kind, std::int64_t id, std::int64_t massId = 0) noexcept;
    [[nodiscard]] const Model* findPart(std::string_view kind, std::int64_t id, std::int64_t massId = 0) const noexcept;

    // The model where the IDs of superelement `seid` live: the part BEGIN SUPER=seid when the deck has one; for a
    // superelement that is a REPEAT or MIRROR image of another (its SEBULK) the part of the one it copies, which has
    // no part of its own, a few steps at most; else the residual structure of the deck (0 is the residual structure
    // itself; the list superelements that SESET and SEELT define in it number their grid points and elements in the
    // one ID space of the main Bulk Data).
    [[nodiscard]] Model& superelementModel(base::EntityId seid) noexcept;
    [[nodiscard]] const Model& superelementModel(base::EntityId seid) const noexcept;

    // Gives the part another number (renumbering of superelements): the BEGIN line is written again with it.
    void setPartId(std::int64_t id);

    // Where the main Bulk Data of the deck is written relative to its parts: a deck that opens with its parts and puts
    // BEGIN BULK after them has its residual structure last, and this is the
    // BEGIN BULK line that precedes it. Empty for the usual order, residual structure first.
    [[nodiscard]] const std::optional<std::string>& residualBeginAfterParts() const noexcept
    {
        return m_residualAfterParts;
    }
    void setResidualBeginAfterParts(std::optional<std::string> beginLine)
    {
        m_residualAfterParts = std::move(beginLine);
    }

    // The key the next created card will get; the markers and comments are positioned by keys.
    [[nodiscard]] std::uint32_t nextKeyValue() const noexcept { return m_nextKey; }

private:
    void checkFree(std::uint16_t type, base::IdSpace space, base::EntityId id) const;
    void indexInsert(std::uint16_t type, base::IdSpace space, base::EntityId id, CardLocation location);
    [[nodiscard]] IdIndex* indexFor(std::uint16_t type, base::IdSpace space) noexcept;
    [[nodiscard]] const IdIndex* indexFor(std::uint16_t type, base::IdSpace space) const noexcept;
    void rebuildIndexes();

    static constexpr std::size_t SHARED_SPACES = static_cast<std::size_t>(base::IdSpace::AeroProperty) + 1;

    std::vector<std::unique_ptr<CardTable>> m_tables; // one slot per card type, null until a card of the type exists
    std::array<IdIndex, SHARED_SPACES> m_spaceIndexes;
    std::vector<IdIndex> m_typeIndexes; // IdSpace::PerCardType: one index per card type (empty until used)
    std::uint32_t m_nextKey = 0;
    std::uint64_t m_structureVersion = 1;
    std::uint64_t m_contentVersion = 1;
    std::uint64_t m_controlVersion = 1;
    std::uint64_t m_lineage;
    // setIdsOf(): the IDs by category, valid while `version` is the contentVersion() (0: none); not copied.
    struct SetIds
    {
        std::uint64_t version = 0;
        std::unordered_map<CardCategory, std::unordered_set<base::EntityId>> ids;
    };
    mutable SetIds m_setIds;
    // add() keeps setIdsOf() up to date: the ID of the card added, then the version it is valid for.
    void noteSetIdAdded(const CardTypeInfo& info, base::EntityId id);
    mutable AttachmentSlot m_derived;
    mutable AttachmentSlot m_contentCache;
    mutable AttachmentSlot m_results;
    base::DiagnosticSink m_diagnostics;

    std::shared_ptr<ControlSections> m_control = std::make_shared<ControlSections>();
    std::vector<SourceFileInfo> m_files;
    std::vector<SourceMarker> m_markers;
    std::vector<CommentRecord> m_comments;
    std::unordered_map<std::uint32_t, std::uint32_t> m_placements;
    std::optional<PartInfo> m_partInfo;
    std::vector<std::unique_ptr<Model>> m_parts;
    Model* m_owner = nullptr; // the residual structure that holds this part: not owned, set by addPart
    std::optional<std::string> m_residualAfterParts;

    // Tells every part that this model holds it (after the parts were copied or moved).
    void adoptParts() noexcept;
};

} // namespace oor::model
