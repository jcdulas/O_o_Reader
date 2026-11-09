#pragma once

#include "oor/analysis/validation/engine/ElementMeasures.hpp"
#include "oor/analysis/validation/engine/ReferenceCensus.hpp"
#include "oor/base/threading/CancellationToken.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace oor::analysis
{

// What the caller of the validation can set besides the choice of checks.
struct ValidationOptions
{
    // The checks to run: codes ("ref.unresolved"), families ("references") or the prefix of a code ("ref"); "all" adds
    // the opt-in checks. Empty: every check that is not opt-in.
    std::vector<std::string> select;
    // Checks left out, named the same way (they win over `select`).
    std::vector<std::string> ignore;
    // The findings below this severity are dropped.
    base::Severity minSeverity = base::Severity::Warning;
    // geometry.coincident: grid points closer than this distance (in the units of the deck) coincide.
    double coincidentTolerance = 1.0e-6;
    const base::CancellationToken* token = nullptr;
};

// What the checks of one model share during a run: the model, the options, and the things several checks need that are
// worth computing once (each is made the first time a check asks, by whichever thread asks first). Everything is const:
// the checks run at the same time.
class ValidationContext
{
public:
    // `model` is the model checked (the residual structure, or a part); `root` the residual structure of the deck;
    // `part` is "" for the residual structure and "SUPER 1" for a part; `partIndex` is 0 for the residual structure and
    // the position (from 1) of the part otherwise.
    ValidationContext(const model::Model& root, const model::Model& model, std::string part, std::size_t partIndex,
                      const ValidationOptions& options);
    ValidationContext(const ValidationContext&) = delete;
    ValidationContext& operator=(const ValidationContext&) = delete;

    [[nodiscard]] const model::Model& model() const noexcept { return m_model; }
    [[nodiscard]] const model::Model& root() const noexcept { return m_root; }
    [[nodiscard]] const std::string& part() const noexcept { return m_part; }
    [[nodiscard]] std::size_t partIndex() const noexcept { return m_partIndex; }
    [[nodiscard]] const ValidationOptions& options() const noexcept { return m_options; }
    // True when the validation was cancelled: a long check looks at it now and then.
    [[nodiscard]] bool cancelled() const noexcept
    {
        return m_options.token != nullptr && m_options.token->isCancelled();
    }
    // Every model of the deck, the residual structure first then the parts in file order (nested parts included).
    [[nodiscard]] const std::vector<const model::Model*>& allModels() const noexcept { return m_all; }
    void setAllModels(std::vector<const model::Model*> models) { m_all = std::move(models); }
    // The model the reference scan visits instead of the one checked: a copy of it, made by the engine before the
    // checks run. Visiting a card's references writes each ID back where it was (and may clone a column buffer that a
    // copy of the model shares), so the scan must not touch a model that other checks read at the same time.
    void setScanModel(const model::Model* copy) noexcept { m_scan = copy; }

    // The solution sequence number of the SOL statement of the deck (the residual structure holds the control
    // sections); nothing when there is no SOL or its name is not one of the format reference.
    [[nodiscard]] std::optional<std::int64_t> solution() const;

    // Every reference field of the model looked at once.
    [[nodiscard]] const ReferenceCensus& references() const;

    // The lengths, areas and volumes of the rods, bars, shells and solids of the model.
    [[nodiscard]] const ElementMeasures& measures() const;

    // The IDs of the grid points and the scalar points of the model, sorted.
    [[nodiscard]] const std::vector<base::EntityId>& sortedNodeIds() const;

private:
    const model::Model& m_root;
    const model::Model& m_model;
    std::string m_part;
    std::size_t m_partIndex;
    const ValidationOptions& m_options;
    std::vector<const model::Model*> m_all;
    const model::Model* m_scan = nullptr;

    mutable std::once_flag m_censusFlag;
    mutable std::unique_ptr<ReferenceCensus> m_census;
    mutable std::once_flag m_measuresFlag;
    mutable std::unique_ptr<ElementMeasures> m_measures;
    mutable std::once_flag m_nodesFlag;
    mutable std::vector<base::EntityId> m_nodeIds;
};

} // namespace oor::analysis
