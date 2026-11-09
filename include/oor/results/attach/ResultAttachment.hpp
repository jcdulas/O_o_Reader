#pragma once

#include "oor/base/diagnostics/Diagnostic.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/model/storage/TableAttachment.hpp"
#include "oor/results/core/ResultFile.hpp"
#include "oor/results/core/ResultFilter.hpp"
#include "oor/results/core/ResultTable.hpp"
#include "oor/results/core/ResultTables.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace oor::results
{

// What has become of the model an attachment was made with, as far as the versions of the model tell (Model::
// structureVersion and contentVersion).
enum class AttachmentState : std::uint8_t
{
    Current, // the model is as it was when the results were attached (or refreshed)
    Edited,  // cards were edited, added or removed, but not the connectivity: the links are still right
    Stale // the structure changed (elements or nodes added, removed, renumbered, a compaction): the links are unknown
};

[[nodiscard]] std::string_view attachmentStateName(AttachmentState state) noexcept;

// Where a result ID goes in the model: the card key (it follows the card through a renumbering) and the card type.
struct CardLink
{
    std::int32_t resultId = 0; // the ID in the file
    std::uint32_t key = 0;     // the card key of the card the ID found when the results were attached
    std::uint16_t type = 0;    // its card type
};

struct AttachOptions
{
    // The label of the result set: the file name when empty.
    std::string analysis;
    // Raise (DeckError, with the diagnostic) on the first error of the consistency check.
    bool strict = false;
    // The filter the file was read with (kept so that `model.results` can give the same view back).
    ResultFilter filter;
};

// A result set attached to a model: the result IDs of every table linked to the cards of the
// model, first by ID (when the results are attached), then by card key (the key the ID found), so that the link
// survives a renumbering of the model and a deleted card makes it dead instead of pointing at another card. The
// attachment keeps the versions of the model it saw and says whether the model has changed since: a change of the
// structure makes it stale (frame changes refuse until `refresh`), any other edit marks it edited.
//
// The attachment does not own the model: the model owns its attachments (ModelResults, in its results slot), the
// attachment only remembers the model weakly, for the objects that start from the results and need the model.
//
// Thread safety: attaching, refreshing and detaching edit the model's slot and need exclusive access to the model;
// every other member is const and safe to call from several threads.
class ResultAttachment
{
public:
    // Attaches `file` (with its `tables`) to the model: links the IDs, checks the consistency and records the versions.
    // Attaching the same tables twice to a model replaces the first attachment (a re-attach is a refresh). Throws
    // DeckError in strict mode when the check finds an error (and attaches nothing). `context` decodes the labels of
    // the tables on its threads (only the IDs: one step and one component of each table).
    [[nodiscard]] static std::shared_ptr<ResultAttachment> attach(const std::shared_ptr<model::Model>& model,
                                                                  std::shared_ptr<ResultFile> file,
                                                                  std::shared_ptr<ResultTables> tables,
                                                                  const AttachOptions& options = {},
                                                                  const DecodeContext& context = {});
    // The attachment of these tables to the model when attaching them again would change nothing (a companion package,
    // 2026-10-13): attached under the label the attach would give (the one asked for, else the file name), the model
    // unchanged since (Current), every table of the set linked by it (no table added since), no filter asked for or
    // kept, and in strict mode a check that found no error. Null otherwise: the attach links the tables again.
    [[nodiscard]] static std::shared_ptr<ResultAttachment> unchanged(const model::Model& model,
                                                                     const std::shared_ptr<ResultTables>& tables,
                                                                     const AttachOptions& options);
    // The attachments of a model, in the order they were made.
    [[nodiscard]] static std::vector<std::shared_ptr<ResultAttachment>> of(const model::Model& model);
    // Removes the attachment from the model; false when it was not attached to it. The tables forget it.
    static bool detach(model::Model& model, const std::shared_ptr<ResultAttachment>& attachment);

    // Checks the links again against the model as it is now and records its versions: a link follows its card by key
    // (so a renumbering does not break it), a link whose card was deleted is dropped and reported, an ID that had no
    // card stays without one (a card added later never takes the place of one that was deleted; to link by ID again,
    // attach again). Throws like `attach`.
    void refresh(const model::Model& model, bool strict = false, const DecodeContext& context = {});

    [[nodiscard]] const std::shared_ptr<ResultFile>& file() const noexcept { return m_file; }
    [[nodiscard]] const std::shared_ptr<ResultTables>& tables() const noexcept { return m_tables; }
    [[nodiscard]] const ResultFilter& filter() const noexcept { return m_filter; }
    // The label of the result set (the file name unless given).
    [[nodiscard]] const std::string& analysis() const noexcept { return m_analysis; }
    void setAnalysis(std::string analysis) { m_analysis = std::move(analysis); }
    // Gives the set another label: the labels of the sets attached to one model are all different (the `analysis`
    // column and the `analyses` filter name a set by it). Throws UsageError when another set of the model has it, or
    // when the label is empty.
    void rename(const model::Model& model, const std::string& label);
    // True when a set attached to the model other than `except` carries the label.
    [[nodiscard]] static bool labelInUse(const model::Model& model, const std::string& label,
                                         const ResultAttachment* except = nullptr);
    // The model the results were attached to, if it is still there; null for the attachment of a copy of a model. It is
    // found through a function, because who owns the model is not this layer's business: attach makes one that holds
    // the model weakly (a weak_ptr), and the Python bindings replace it with one that holds a weak reference to the
    // Python object of the model (a shared_ptr that Python hands out cannot be held weakly).
    using ModelResolver = std::function<std::shared_ptr<model::Model>()>;
    [[nodiscard]] std::shared_ptr<model::Model> model() const { return m_resolver ? m_resolver() : nullptr; }
    void setModel(const std::shared_ptr<model::Model>& model);
    void setModelResolver(ModelResolver resolver) { m_resolver = std::move(resolver); }

    // What the consistency check found at the last attach or refresh: node and element IDs the model does not have,
    // elements of another type, tables that cover part of the model.
    [[nodiscard]] const std::vector<base::Diagnostic>& diagnostics() const noexcept { return m_diagnostics; }
    // The versions of the model at the last attach or refresh, and what the model has become.
    [[nodiscard]] std::uint64_t structureVersion() const noexcept { return m_structureVersion; }
    [[nodiscard]] std::uint64_t contentVersion() const noexcept { return m_contentVersion; }
    [[nodiscard]] AttachmentState state(const model::Model& model) const noexcept;
    // True when the check found no warning and no error and the model is as it was.
    [[nodiscard]] bool consistent(const model::Model& model) const noexcept;
    // The snapshot of what decides the frames of the results (the analysis layer computes it: the displacement systems
    // of the nodes and PARAM,OMID), taken at the last attach or refresh.
    [[nodiscard]] std::uint64_t frameSignature() const noexcept { return m_frameSignature; }
    void setFrameSignature(std::uint64_t signature) noexcept { m_frameSignature = signature; }

    // The links: result IDs of grid points and elements (increasing) with the cards they found.
    [[nodiscard]] const std::vector<CardLink>& nodeLinks() const noexcept { return m_nodes ? *m_nodes : NO_LINKS; }
    [[nodiscard]] const std::vector<CardLink>& elementLinks() const noexcept
    {
        return m_elements ? *m_elements : NO_LINKS;
    }
    // The card a result ID is linked to now (through its card key): nothing when the ID has no link or its card was
    // deleted.
    [[nodiscard]] std::optional<model::CardLocation> nodeLocation(const model::Model& model,
                                                                  std::int32_t resultId) const noexcept;
    [[nodiscard]] std::optional<model::CardLocation> elementLocation(const model::Model& model,
                                                                     std::int32_t resultId) const noexcept;
    // The ID the card has in the model now (after a renumbering it is not the one of the file); 0 for no link.
    [[nodiscard]] std::int32_t modelNodeId(const model::Model& model, std::int32_t resultId) const noexcept;
    [[nodiscard]] std::int32_t modelElementId(const model::Model& model, std::int32_t resultId) const noexcept;
    // The result IDs linked to a card key (a card of the model asking for its results): empty when none.
    [[nodiscard]] std::vector<std::int32_t> resultIdsOf(std::uint16_t type, std::uint32_t key) const;

private:
    void link(const model::Model& model, bool strict, bool byKeyFirst, const DecodeContext& context);
    void snapshot(const model::Model& model) noexcept;

    std::shared_ptr<ResultFile> m_file;
    std::shared_ptr<ResultTables> m_tables;
    ResultFilter m_filter;
    std::string m_analysis;
    ModelResolver m_resolver;
    static inline const std::vector<CardLink> NO_LINKS{};

    // The links are replaced as a whole, never edited: the copies of a model share them, with no work per ID.
    std::shared_ptr<const std::vector<CardLink>> m_nodes;
    std::shared_ptr<const std::vector<CardLink>> m_elements;
    std::vector<base::Diagnostic> m_diagnostics;
    std::uint64_t m_structureVersion = 0;
    std::uint64_t m_contentVersion = 0;
    std::uint64_t m_frameSignature = 0;
};

// The attachments of one model: the object its results slot holds. Copied with the model (the copy gets its own list of
// attachments, sharing the result files and tables, with the links as they are).
class ModelResults final : public model::TableAttachment
{
public:
    [[nodiscard]] std::unique_ptr<model::TableAttachment> clone() const override;

    // The list of a model; made empty when the model has none yet.
    [[nodiscard]] static ModelResults& of(const model::Model& model);
    // The list of a model, or null.
    [[nodiscard]] static const ModelResults* peek(const model::Model& model) noexcept;

    [[nodiscard]] const std::vector<std::shared_ptr<ResultAttachment>>& items() const noexcept { return m_items; }
    std::vector<std::shared_ptr<ResultAttachment>>& mutableItems() noexcept { return m_items; }

private:
    std::vector<std::shared_ptr<ResultAttachment>> m_items;
};

} // namespace oor::results
