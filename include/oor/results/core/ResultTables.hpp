#pragma once

#include "oor/results/core/NamedResult.hpp"
#include "oor/results/core/ResultFile.hpp"
#include "oor/results/core/ResultFilter.hpp"
#include "oor/results/core/ResultKind.hpp"
#include "oor/results/core/ResultTable.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace oor::results
{

// The tables of one kind (the displacements, the SPC forces, ...; for a family kind, those of one element type or one
// variant), keyed by subcase. A subcase has one table of a kind unless the file holds several variants of it (another
// analysis, another superelement, another design cycle, the same table written twice with different points);
// `tablesOf` then returns all of them in the order of the file and `ofAnalysis` and `ofDesignCycle` narrow them.
class ResultContainer
{
public:
    ResultContainer() = default;
    explicit ResultContainer(ResultKind kind, ResultVariant variant = ResultVariant::Plain)
        : m_kind(kind), m_variant(variant)
    {
    }
    // The container of the tables of a family kind for one sub-name: an element type ("ShellQuad4") or a variant of the
    // grid point stresses ("surface").
    ResultContainer(ResultKind kind, std::string elementName, ResultVariant variant = ResultVariant::Plain)
        : m_kind(kind), m_variant(variant), m_elementName(std::move(elementName))
    {
    }

    [[nodiscard]] ResultKind kind() const noexcept { return m_kind; }
    [[nodiscard]] ResultVariant variant() const noexcept { return m_variant; }
    // The sub-name of a family container ("ShellQuad4", "surface"); empty for the other kinds.
    [[nodiscard]] const std::string& elementName() const noexcept { return m_elementName; }
    [[nodiscard]] const std::vector<std::shared_ptr<ResultTable>>& tables() const noexcept { return m_tables; }
    [[nodiscard]] std::size_t tableCount() const noexcept { return m_tables.size(); }
    // The subcases that have a table, increasing, each once.
    [[nodiscard]] std::vector<std::int32_t> subcases() const;
    // The tables of a subcase, in the order of the file; empty when there is none.
    [[nodiscard]] std::vector<std::shared_ptr<ResultTable>> tablesOf(std::int32_t subcase) const;
    // The tables of one analysis (an analysis code: 1 static, 2 modes, 5 frequency, 6 transient, ...).
    [[nodiscard]] ResultContainer ofAnalysis(std::int32_t analysisCode) const;
    // The tables of one design cycle of a SOL 200 run.
    [[nodiscard]] ResultContainer ofDesignCycle(std::int32_t designCycle) const;
    // The design cycles the tables have, increasing, each once (a table without one is not listed).
    [[nodiscard]] std::vector<std::int32_t> designCycles() const;

    void add(std::shared_ptr<ResultTable> table) { m_tables.push_back(std::move(table)); }

private:
    ResultKind m_kind = ResultKind::Displacements;
    ResultVariant m_variant = ResultVariant::Plain;
    std::string m_elementName;
    std::vector<std::shared_ptr<ResultTable>> m_tables;
};

// Every table of a result file that a decoder reads, by variant and kind. Made from the index only: building it
// decodes nothing.
class ResultTables
{
public:
    ResultTables();

    // The container of a kind that is not a family (displacements, eigenvalues, ...) in a variant. A family kind has
    // one container per element type or variant: see `elementContainer`; asked here it gives an empty one.
    [[nodiscard]] const ResultContainer& container(ResultKind kind,
                                                   ResultVariant variant = ResultVariant::Plain) const noexcept;
    [[nodiscard]] ResultContainer& mutableContainer(ResultKind kind, ResultVariant variant = ResultVariant::Plain);
    // The container of a family kind (stress, strain, forces, failure indices, strain energy, grid point stresses) for
    // one element type or variant ("ShellQuad4", "surface"); an empty one when the file holds no such table.
    [[nodiscard]] const ResultContainer& elementContainer(ResultKind kind, std::string_view elementName,
                                                          ResultVariant variant = ResultVariant::Plain) const;
    // The same, made when it does not exist yet: for the code that builds the tables.
    [[nodiscard]] ResultContainer& mutableElementContainer(ResultKind kind, const std::string& elementName,
                                                           ResultVariant variant = ResultVariant::Plain);
    // The sub-names of a family kind that have a table, in alphabetical order.
    [[nodiscard]] std::vector<std::string> elementNames(ResultKind kind,
                                                        ResultVariant variant = ResultVariant::Plain) const;
    // Every table, container by container.
    [[nodiscard]] std::vector<std::shared_ptr<ResultTable>> all() const;
    [[nodiscard]] std::size_t tableCount() const noexcept;
    // The design cycles of the tables, increasing, each once.
    [[nodiscard]] std::vector<std::int32_t> designCycles() const;

    // The named results (the grid point weight, the matrices, the optimisation tables): every one, or those of a kind.
    [[nodiscard]] const std::vector<std::shared_ptr<NamedResult>>& namedResults() const noexcept { return m_named; }
    [[nodiscard]] std::vector<std::shared_ptr<NamedResult>> named(std::string_view kind) const;
    [[nodiscard]] std::size_t namedCount() const noexcept { return m_named.size(); }
    void addNamed(std::shared_ptr<NamedResult> result) { m_named.push_back(std::move(result)); }

    // The tables of `file` that the filter keeps (the file builds them: ResultFile::tables). The subcases, tables,
    // variants and design cycles members of the filter choose tables; the steps, components and IDs are applied by
    // each table (the IDs when it is read). Throws EmptySelectionError when the file holds tables and the filter keeps
    // none of them.
    [[nodiscard]] static ResultTables build(const std::shared_ptr<const ResultFile>& file, const ResultFilter& filter);

    // The containers of `tables` and `named` that the filter keeps: what a file format calls from its `tables`, so that
    // every format filters the same way.
    [[nodiscard]] static ResultTables fromTables(const std::vector<std::shared_ptr<ResultTable>>& tables,
                                                 const std::vector<std::shared_ptr<NamedResult>>& named,
                                                 const ResultFilter& filter);

private:
    using Key = std::tuple<int, int, std::string>; // variant, kind, sub-name
    std::map<Key, ResultContainer, std::less<>> m_containers;
    std::vector<std::shared_ptr<NamedResult>> m_named;
};

} // namespace oor::results
