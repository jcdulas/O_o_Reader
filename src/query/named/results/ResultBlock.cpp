#include "oor/query/named/results/ResultBlock.hpp"

#include "oor/base/threading/ParallelFor.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/NamedQueryRegistry.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/providers/quantities/CompositeQuantities.hpp"
#include "oor/query/providers/quantities/ElementQuantities.hpp"
#include "oor/query/providers/quantities/LaminateStrainRows.hpp"
#include "oor/query/results/RowAccumulator.hpp"
#include "oor/query/results/TableValues.hpp"
#include "r2_stress_strain/PlyData.hpp"
#include "r2_stress_strain/StressData.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string_view>
#include <tuple>
#include <unordered_map>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();
constexpr double NO_STEP = -std::numeric_limits<double>::infinity(); // sorts before every step

// One flat of rows and what the block takes of it.
struct FlatRows
{
    const ElementFlat* flat = nullptr;
    std::vector<const std::vector<double>*> values;    // per component of the block
    std::vector<double> tension;                       // fastenerForces: max(axial, 0), as the query gives it
    std::string source;                                // the provider of the rows ("" none)
    const ScalarCompletion* completion = nullptr;      // per row, the derived quantities noted with the source
    const std::vector<double>* rowProviders = nullptr; // per row, the provider in `providers` (-1 none)
    const std::vector<ProviderInfo>* providers = nullptr;
};

// The rows of a kind, from the providers of its query, as the query resolves them (the same chains, the same
// arguments, the same refusals).
struct KindRows
{
    std::optional<StressData> stress;
    std::optional<ElementResolution> resolution;
    std::optional<PlyData> plies;
    std::vector<ElementFlat> plyStrains;
    std::vector<ElementFlat> derived; // the rows a kind computes from those of the providers (the laminate strains)
    std::vector<FlatRows> flats;
};

// The quantities of the plies a direct block of "plyStress" derives from the rows of the stored ply tables
// (COMPOSITE-API-AUDIT P1), as buildPlyData derives them from the flats: the failure index stored by the file (the row
// of the same step, element and ply of the failure index table of the subcase: an element and step that table has a row
// of take none from the recomputation, as the chain's sink drops them), else recomputed from the stresses and the
// allowables by the theory asked or the property's; the strength ratio by the theory of the index, else the
// property's; the bonding index stored with the index, else max(|t1z|, |t2z|) / SB; the indices of the modes; and the
// global ID of each ply. They are computed only when the block asks for one of them (with the index: its provider is
// the source of the case, and it says the theory of the ratio). `usable()` is false when the stored failure index
// tables do not match the stress tables one to one, table to table and step to step (the flats then make the block).
class PlyRows
{
public:
    static constexpr std::size_t FIELDS = 8; // the stored fields of a row, those of plyStressFields
    enum Derived : std::uint8_t
    {
        FailureIndex,
        StrengthRatio,
        BondIndex,
        FiberIndex,
        MatrixIndex,
        DerivedCount
    };
    static const std::vector<std::string>& derivedNames()
    {
        static const std::vector<std::string> NAMES = {"failureIndex", "strengthRatio", "bondIndex", "fiberIndex",
                                                       "matrixIndex"};
        return NAMES;
    }

    PlyRows(const ResultQueryContext& context, const ElementRequest& request,
            const std::vector<DirectForceTable>& tables, const std::vector<std::string>& components)
        : m_tables(tables.data()), m_laminates(context)
    {
        for (const std::string& name : components)
        {
            const auto found = std::find(derivedNames().begin(), derivedNames().end(), name);
            if (found != derivedNames().end())
                m_asked[static_cast<std::size_t>(found - derivedNames().begin())] = true;
        }
        m_derives = std::find(m_asked.begin(), m_asked.end(), true) != m_asked.end();
        const QuantityInfo chain = failureIndexChain().info();
        m_storedText = sourceText(chain.providers[0].name, chain.providers[0].approximate);
        m_recomputedText = sourceText(chain.providers[1].name, chain.providers[1].approximate);
        const std::string& asked = request.theory;
        if (!asked.empty() && plyTheoryOf(asked) == PlyTheory::None)
        {
            m_usable = false; // the providers refuse it, naming the theories
            return;
        }
        m_askedTheory = asked.empty() ? EMPTY_THEORY : codeOf(asked);
        m_recompute = context.hasModel() && !ProviderSwitches::isDisabled(chain.providers[1].name);
        // the index of every row of a block that derives something: its provider is the source of the case, and a row
        // whose recomputed index is NaN takes the property's theory for its ratio
        m_needIndex = m_derives;
        // the laminate of each element, read once a run of rows; the facts of each row, shared by the tables of the
        // same rows of a result set
        m_rows.resize(tables.size());
        for (std::size_t index = 0; index < tables.size(); ++index)
        {
            const DirectForceTable& table = tables[index];
            if (index > 0 && tables[index - 1].set == table.set && tables[index - 1].layer == table.layer)
            {
                m_rows[index] = m_rows[index - 1];
                continue;
            }
            const auto& ids = table.decoded->ids;
            const std::vector<std::int32_t>& layers = *table.layer;
            for (std::size_t row = 0; row < ids.size(); ++row)
                if (row == 0 || ids[row] != ids[row - 1])
                    (void)m_laminates.facts(table.set, ids[row]);
            auto facts = std::make_shared<std::vector<RowFacts>>(ids.size());
            const LaminateFacts* laminate = nullptr;
            std::uint8_t laminateTheory = EMPTY_THEORY;
            for (std::size_t row = 0; row < ids.size(); ++row)
            {
                if (row == 0 || ids[row] != ids[row - 1])
                {
                    laminate = m_laminates.found(table.set, ids[row]);
                    laminateTheory = laminate != nullptr ? codeOf(laminate->theory) : EMPTY_THEORY;
                }
                RowFacts& entry = (*facts)[row];
                entry.laminate = laminate;
                const auto ply = static_cast<std::size_t>(layers[row] - 1);
                entry.ply = laminate != nullptr && ply < laminate->plies.size() ? &laminate->plies[ply] : nullptr;
                entry.laminateTheory = laminateTheory;
                entry.recomputedTheory = asked.empty() ? laminateTheory : m_askedTheory;
            }
            m_rows[index] = std::move(facts);
        }
        m_failure.resize(tables.size());
        if (m_derives && asked.empty() && !ProviderSwitches::isDisabled(chain.providers[0].name))
            matchStoredIndices(context, request, tables);
    }

    [[nodiscard]] bool usable() const noexcept { return m_usable; }
    [[nodiscard]] bool derives() const noexcept { return m_derives; }

    // The derived values of the row `row` at the step `step` of `table` (`fields` its FIELDS stored values) into `out`
    // (DerivedCount, those asked), and the bits of the providers of its failure index (1 stored, 2 recomputed).
    std::uint8_t derive(const DirectForceTable& table, std::size_t step, std::size_t row, const double* fields,
                        double* out) const
    {
        const std::size_t index = static_cast<std::size_t>(&table - m_tables);
        const RowFacts& facts = (*m_rows[index])[row];
        static const LaminateFacts NONE;
        const LaminateFacts& laminate = facts.laminate != nullptr ? *facts.laminate : NONE;
        std::fill(out, out + DerivedCount, NOT_A_NUMBER);
        double failure = NOT_A_NUMBER;
        double bond = NOT_A_NUMBER;
        std::uint8_t rowTheory = EMPTY_THEORY;
        std::uint8_t provider = 0;
        const FailureMatch& match = m_failure[index];
        if (match.table != nullptr && match.covered[row] != 0)
        {
            // the stored index of the row (an element and step the table covers takes none from the recomputation)
            const std::int32_t at = match.rows[row];
            if (at >= 0)
            {
                const StoredIndices& stored = *match.table;
                const TableValues values(*stored.decoded);
                failure = values.real(step, static_cast<std::size_t>(at), static_cast<std::size_t>(stored.index));
                if (stored.bond >= 0)
                    bond = values.real(step, static_cast<std::size_t>(at), static_cast<std::size_t>(stored.bond));
                rowTheory = stored.theory[static_cast<std::size_t>(at)];
                provider = 1;
            }
        }
        else if (m_needIndex && m_recompute && facts.recomputedTheory != EMPTY_THEORY && facts.ply != nullptr &&
                 facts.ply->allowables)
        {
            const PlyIndex value = plyFailureIndex(static_cast<PlyTheory>(facts.recomputedTheory), fields[0], fields[1],
                                                   fields[2], *facts.ply->allowables);
            if (!std::isnan(value.index))
            {
                failure = value.index;
                rowTheory = facts.recomputedTheory;
                provider = 2;
            }
        }
        out[FailureIndex] = failure;
        if (m_asked[BondIndex])
            out[BondIndex] = std::isnan(bond) ? bondIndex(fields[3], fields[4], laminate.bondAllowable, false) : bond;
        if (!(m_asked[StrengthRatio] || m_asked[FiberIndex] || m_asked[MatrixIndex]) || facts.ply == nullptr)
            return provider;
        const std::uint8_t used = rowTheory != EMPTY_THEORY ? rowTheory : facts.laminateTheory;
        if (!facts.ply->allowables || used == EMPTY_THEORY)
            return provider;
        const auto theory = static_cast<PlyTheory>(used);
        if (theory == PlyTheory::Hashin || theory == PlyTheory::Puck || theory == PlyTheory::Larc03)
        {
            const PlyModes modes =
                plyFailureModes(modeTheoryName(theory), fields[0], fields[1], fields[2], *facts.ply->allowables);
            if (!std::isnan(modes.index))
                out[StrengthRatio] = modes.ratio;
            out[FiberIndex] = modes.fiber;
            out[MatrixIndex] = modes.matrix;
            return provider;
        }
        const PlyIndex value = plyFailureIndex(theory, fields[0], fields[1], fields[2], *facts.ply->allowables);
        if (!std::isnan(value.index))
            out[StrengthRatio] = scaleToLimit(value, 1.0);
        return provider;
    }

    // The global ID of the ply of a row (0 for a PCOMP, or an element without a laminate).
    [[nodiscard]] std::int32_t globalId(const DirectForceTable& table, std::size_t row) const
    {
        const RowFacts& facts = (*m_rows[static_cast<std::size_t>(&table - m_tables)])[row];
        return facts.ply != nullptr ? facts.ply->globalId : 0;
    }

    // The sources of a case whose rows took the index of the providers `bits`, as the flats' rows name them.
    [[nodiscard]] std::string sourceOf(std::uint8_t bits) const
    {
        std::set<std::string> texts;
        if ((bits & 1U) != 0)
            texts.insert(m_storedText);
        if ((bits & 2U) != 0)
            texts.insert(m_recomputedText);
        std::string joined;
        for (const std::string& text : texts)
            joined += (joined.empty() ? "" : "; ") + text;
        return joined;
    }

private:
    // A theory by its code: a PlyTheory, or EMPTY_THEORY for none named ("" as the providers' rows give it).
    static constexpr std::uint8_t EMPTY_THEORY = 255;
    static std::uint8_t codeOf(const std::string& theory)
    {
        return theory.empty() ? EMPTY_THEORY : static_cast<std::uint8_t>(plyTheoryOf(theory));
    }
    static const std::string& modeTheoryName(PlyTheory theory)
    {
        static const std::string HASHIN = "hashin";
        static const std::string PUCK = "puck";
        static const std::string LARC03 = "larc03";
        return theory == PlyTheory::Hashin ? HASHIN : theory == PlyTheory::Puck ? PUCK : LARC03;
    }

    struct RowFacts
    {
        const LaminateFacts* laminate = nullptr;
        const PlyFacts* ply = nullptr;
        std::uint8_t laminateTheory = EMPTY_THEORY;
        std::uint8_t recomputedTheory = EMPTY_THEORY;
    };
    // A failure index table of the plies as StoredFailureIndexProvider reads it: the columns of the index and of the
    // bonding index (-1 none), the theory of each row (written on the first rows of an element, it holds for the rows
    // that follow), the rows the request keeps.
    struct StoredIndices
    {
        std::shared_ptr<const results::ResultData> decoded;
        int index = -1;
        int bond = -1;
        std::vector<std::uint8_t> theory;
        std::vector<std::uint8_t> kept;
        std::size_t keptCount = 0;
        std::size_t set = 0;
        std::int32_t subcase = 0;
        std::int32_t designCycle = 0;
        std::string elementName;
        bool stepped = false;
    };
    // The stored rows of a stress table: the row of the same element and ply of each of its rows (-1 none), and whether
    // the table has a row of its element.
    struct FailureMatch
    {
        const StoredIndices* table = nullptr;
        std::vector<std::int32_t> rows;
        std::vector<std::uint8_t> covered;
    };

    void matchStoredIndices(const ResultQueryContext& context, const ElementRequest& request,
                            const std::vector<DirectForceTable>& tables)
    {
        std::vector<TableRef> refs;
        for (const std::string& name : context.elementNames(results::ResultKind::FailureIndices))
        {
            if (name.size() < 9 || name.compare(name.size() - 9, 9, "Composite") != 0 ||
                !request.wantsType(cardOfContainer(name)))
                continue;
            for (const TableRef& ref : context.elementTables(results::ResultKind::FailureIndices, name, false))
                refs.push_back(ref);
        }
        if (refs.empty())
            return;
        m_stored.resize(refs.size());
        const auto read = [&](std::size_t at)
        {
            const TableRef& ref = refs[at];
            const results::ResultTableMeta& meta = ref.table->meta();
            StoredIndices& stored = m_stored[at];
            std::shared_ptr<const results::ResultData> decoded = context.decodeRows(ref, request.elementIds);
            if (!decoded)
                return;
            const TableValues values(*decoded);
            stored.index = values.component("failureIndex");
            if (stored.index < 0 || values.isComplex())
                return;
            stored.bond = values.component("bondFailureIndex");
            const results::ResultText* theories = nullptr;
            for (const results::ResultText& text : decoded->texts)
                if (text.name == "failureTheory")
                    theories = &text;
            const std::size_t rows = values.rows();
            stored.theory.assign(rows, EMPTY_THEORY);
            stored.kept.assign(rows, 0);
            std::uint8_t current = EMPTY_THEORY;
            for (std::size_t row = 0; row < rows; ++row)
            {
                if (row == 0 || decoded->ids[row] != decoded->ids[row - 1])
                    current = EMPTY_THEORY;
                if (theories != nullptr && !theories->at(row).empty())
                    current = codeOf(storedTheoryName(theories->at(row)));
                stored.theory[row] = current;
                if (request.wantsElement(decoded->ids[row]))
                {
                    stored.kept[row] = 1;
                    ++stored.keptCount;
                }
            }
            if (stored.keptCount == 0 || values.steps() == 0)
                return;
            stored.set = ref.set;
            stored.subcase = meta.subcase;
            stored.designCycle = meta.designCycle;
            stored.elementName = meta.elementName;
            stored.stepped = meta.stepKind == results::StepKind::Time ||
                             meta.stepKind == results::StepKind::Frequency ||
                             meta.stepKind == results::StepKind::Mode || meta.stepKind == results::StepKind::LoadFactor;
            stored.decoded = std::move(decoded);
        };
        const results::DecodeContext decode = context.decode();
        if (decode.pool != nullptr && refs.size() > 1)
            base::parallelFor(*decode.pool, refs.size(), read, decode.token);
        else
            for (std::size_t at = 0; at < refs.size(); ++at)
                read(at);
        // each table of indices with rows asked for is matched to the one stress table of its result set, subcase,
        // design cycle and element type, step for step: else the flats
        using GroupKey = std::tuple<std::size_t, std::int32_t, std::int32_t, std::string>;
        std::map<GroupKey, std::vector<std::size_t>> stressOf;
        for (std::size_t index = 0; index < tables.size(); ++index)
            stressOf[{tables[index].set, tables[index].subcase, tables[index].designCycle, tables[index].elementName}]
                .push_back(index);
        std::vector<std::uint8_t> taken(tables.size(), 0);
        for (const StoredIndices& stored : m_stored)
        {
            if (!stored.decoded)
                continue;
            const auto found = stressOf.find({stored.set, stored.subcase, stored.designCycle, stored.elementName});
            if (found == stressOf.end() || found->second.size() != 1 || taken[found->second.front()] != 0)
            {
                m_usable = false;
                return;
            }
            const std::size_t index = found->second.front();
            taken[index] = 1;
            const DirectForceTable& table = tables[index];
            const results::ResultData& stress = *table.decoded;
            const results::ResultData& indices = *stored.decoded;
            bool sameSteps = stored.stepped == table.stepped && indices.stepCount == stress.stepCount;
            for (std::size_t step = 0; sameSteps && table.stepped && step < stress.stepCount; ++step)
                sameSteps = stepKey(indices.steps[step]) == stepKey(stress.steps[step]);
            if (!sameSteps)
            {
                m_usable = false;
                return;
            }
            FailureMatch& match = m_failure[index];
            match.table = &stored;
            if (!matchRows(stress, *table.layer, stored, match))
            {
                m_usable = false;
                return;
            }
        }
    }

    // The stored row of each stress row (the same element and ply; the later of two), walked along the two tables in
    // their order, else looked up; false when a stored row asked for has no stress row.
    static bool matchRows(const results::ResultData& stress, const std::vector<std::int32_t>& layers,
                          const StoredIndices& stored, FailureMatch& match)
    {
        const results::ResultData& indices = *stored.decoded;
        const std::size_t rows = stress.rowCount;
        match.rows.assign(rows, -1);
        match.covered.assign(rows, 0);
        std::size_t matched = 0;
        std::size_t at = 0;
        bool walked = true;
        for (std::size_t row = 0; walked && row < indices.rowCount; ++row)
        {
            if (stored.kept[row] == 0)
                continue;
            while (at < rows && !(stress.ids[at] == indices.ids[row] && layers[at] == indices.layers[row]))
                ++at;
            if (at == rows)
                walked = false;
            else
                match.rows[at++] = static_cast<std::int32_t>(row);
        }
        if (!walked)
        {
            std::fill(match.rows.begin(), match.rows.end(), -1);
            std::unordered_map<std::uint64_t, std::int32_t> byKey;
            const auto keyOf = [](std::int32_t eid, std::int32_t layer)
            {
                return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(eid)) << 32U) |
                       static_cast<std::uint32_t>(layer);
            };
            for (std::size_t row = 0; row < indices.rowCount; ++row)
                if (stored.kept[row] != 0)
                    byKey[keyOf(indices.ids[row], indices.layers[row])] = static_cast<std::int32_t>(row);
            for (std::size_t row = 0; row < rows; ++row)
            {
                const auto found = byKey.find(keyOf(stress.ids[row], layers[row]));
                if (found != byKey.end())
                    match.rows[row] = found->second;
            }
        }
        // every stored row asked for is some stress row's (else the flats add rows of their own), and the elements
        // the stored table has a row of
        std::vector<std::int32_t> elements;
        for (std::size_t row = 0; row < indices.rowCount; ++row)
            if (stored.kept[row] != 0 && (elements.empty() || elements.back() != indices.ids[row]))
                elements.push_back(indices.ids[row]);
        std::sort(elements.begin(), elements.end());
        elements.erase(std::unique(elements.begin(), elements.end()), elements.end());
        std::vector<std::uint8_t> used(indices.rowCount, 0);
        for (std::size_t row = 0; row < rows; ++row)
        {
            if (match.rows[row] >= 0 && used[static_cast<std::size_t>(match.rows[row])] == 0)
            {
                used[static_cast<std::size_t>(match.rows[row])] = 1;
                ++matched;
            }
            match.covered[row] = std::binary_search(elements.begin(), elements.end(), stress.ids[row]) ? 1 : 0;
        }
        return matched == stored.keptCount;
    }

    const DirectForceTable* m_tables;
    LaminateLookup m_laminates;
    std::array<bool, DerivedCount> m_asked{};
    bool m_derives = false;
    bool m_usable = true;
    bool m_recompute = false;
    bool m_needIndex = false;
    std::uint8_t m_askedTheory = EMPTY_THEORY;
    std::string m_storedText;
    std::string m_recomputedText;
    std::vector<std::shared_ptr<const std::vector<RowFacts>>> m_rows;
    std::vector<StoredIndices> m_stored;
    std::vector<FailureMatch> m_failure;
};

// The derived components of the kind "plyStress" (those its flats compute: none is stored as such).
bool derivesPlies(const std::vector<std::string>& components)
{
    return std::any_of(components.begin(), components.end(),
                       [](const std::string& name)
                       {
                           return std::find(PlyRows::derivedNames().begin(), PlyRows::derivedNames().end(), name) !=
                                  PlyRows::derivedNames().end();
                       });
}

// The source of the rows of a ply block that derives nothing: the ply stress (strain) table.
std::string plyTableSource(bool strain)
{
    return sourceText(strain ? "ply strain table of the result file" : "ply stress table of the result file", false);
}

void resolveKind(const ResultQueryContext& context, const ResultBlockKind& kind, KindRows& rows,
                 const SandwichCore& sandwichCore, const std::vector<std::string>& components)
{
    const ElementRequest request = ElementRequest::from(context.arguments());
    const auto providerSource = [](const std::vector<ProviderInfo>& providers, std::int16_t index)
    {
        if (index < 0 || static_cast<std::size_t>(index) >= providers.size())
            return std::string();
        const ProviderInfo& info = providers[static_cast<std::size_t>(index)];
        return sourceText(info.name, info.approximate);
    };
    if (kind.kind == "stress" || kind.kind == "strain")
    {
        rows.stress = resolveStress(context, request, kind.kind == "strain");
        const std::vector<ElementFlat>& flats = rows.stress->set.flats();
        rows.flats.reserve(flats.size());
        for (std::size_t index = 0; index < flats.size(); ++index)
        {
            FlatRows entry;
            entry.flat = &flats[index];
            entry.source = providerSource(rows.stress->set.providers(), flats[index].provider);
            entry.completion = &rows.stress->completions[index];
            rows.flats.push_back(std::move(entry));
        }
        return;
    }
    if (kind.kind == "plyStress" && derivesPlies(components))
    {
        if (request.propertyIds)
            context.requireModel("find the property of each element");
        rows.plies = buildPlyData(context, request, false); // what a block reads
        if (rows.plies->flats.empty() && context.selection().onMissing == OnMissing::Raise)
            throw QuantityUnavailableError(describeAttempts("the plies of the laminates", rows.plies->attempts) +
                                           "\nand the result file has no stress table of laminates either");
        rows.flats.reserve(rows.plies->flats.size());
        for (const ElementFlat& flat : rows.plies->flats)
        {
            FlatRows entry;
            entry.flat = &flat;
            entry.rowProviders = &flat.values.at("failureProvider");
            entry.providers = &rows.plies->providers;
            rows.flats.push_back(std::move(entry));
        }
        return;
    }
    if (kind.kind == "plyStrain" || kind.kind == "plyStress")
    {
        // The strains of the plies have no query: the rows are those of the ply strain tables, as compositePlyTable
        // reads the ply stress tables (in the axes of each ply), with the global ID of each ply from the model; the
        // stresses of a block that derives nothing are the rows of the ply stress tables alone.
        const bool strain = kind.kind == "plyStrain";
        if (request.propertyIds)
            context.requireModel("find the property of each element");
        // the tables read on the threads of the call, the laminate of an element looked up once
        rows.plyStrains = flattenPlyTables(context, plyTables(context, strain, request), strain, request);
        LaminateLookup laminates(context);
        for (std::size_t index = 0; index < rows.plyStrains.size(); ++index)
            laminates.prepare(rows.plyStrains[index], index > 0 ? &rows.plyStrains[index - 1] : nullptr);
        const auto plyIds = [&](std::size_t index)
        {
            ElementFlat& flat = rows.plyStrains[index];
            std::vector<double> plyId(flat.rows(), NOT_A_NUMBER);
            const LaminateFacts* laminate = nullptr;
            for (std::size_t row = 0; row < flat.rows(); ++row)
            {
                if (row == 0 || flat.eid[row] != flat.eid[row - 1])
                    laminate = laminates.found(flat.set, flat.eid[row]);
                const auto ply = static_cast<std::size_t>(flat.layer[row] - 1);
                if (laminate != nullptr && ply < laminate->plies.size() && laminate->plies[ply].globalId > 0)
                    plyId[row] = laminate->plies[ply].globalId;
            }
            flat.values["plyId"] = std::move(plyId);
        };
        const results::DecodeContext decode = context.decode();
        if (decode.pool != nullptr && rows.plyStrains.size() > 1)
            base::parallelFor(*decode.pool, rows.plyStrains.size(), plyIds, decode.token);
        else
            for (std::size_t index = 0; index < rows.plyStrains.size(); ++index)
                plyIds(index);
        if (rows.plyStrains.empty() && context.selection().onMissing == OnMissing::Raise)
            throw QuantityUnavailableError(
                strain ? "the strains of the plies: the result file has no strain table of laminates "
                         "for the elements and subcases asked for (the output was not requested)"
                       : "the stresses of the plies: the result file has no stress table of laminates "
                         "for the elements and subcases asked for (the output was not requested)");
        rows.flats.reserve(rows.plyStrains.size());
        for (const ElementFlat& flat : rows.plyStrains)
        {
            FlatRows entry;
            entry.flat = &flat;
            entry.source = plyTableSource(strain);
            rows.flats.push_back(std::move(entry));
        }
        return;
    }
    if (kind.kind == "laminateStrains" || kind.kind == "sandwich")
    {
        const bool sandwich = kind.kind == "sandwich";
        // the resultants in the element frame, turned into the material axes of each element where its ABD is
        const std::string& frame = context.selection().frameName;
        if (context.selection().frameSystem != 0 || (!frame.empty() && frame != "element" && frame != "material"))
            throw QueryError(
                std::string("readBlock: the ") + (sandwich ? "sandwich resultants" : "laminate strains") +
                " are in the element or the material frame, not in '" +
                (frame.empty() ? "coordinate system " + std::to_string(context.selection().frameSystem) : frame) + "'");
        context.requireModel("find the laminate of each shell");
        ElementRequest forces = request;
        forces.tableFrame = "element";
        rows.resolution = resolveElements(context, shellResultantChain(), forces, "the force resultants of the shells",
                                          resultBlockKind("shellForces").components);
        rows.derived = sandwich
                           ? sandwichFlats(context, rows.resolution->set.flats(), frame == "material", sandwichCore)
                           : laminateStrainFlats(context, rows.resolution->set.flats(), frame == "material");
        rows.flats.reserve(rows.derived.size());
        for (const ElementFlat& flat : rows.derived)
        {
            FlatRows entry;
            entry.flat = &flat;
            const std::string forcesSource = providerSource(rows.resolution->set.providers(), flat.provider);
            entry.source = forcesSource.empty()
                               ? std::string()
                               : forcesSource + (sandwich ? "; faces and core by classical lamination theory"
                                                          : "; laminate strains by classical lamination theory");
            rows.flats.push_back(std::move(entry));
        }
        return;
    }
    if (kind.kind == "shellForces")
        rows.resolution = resolveElements(context, shellResultantChain(), request, "the force resultants of the shells",
                                          kind.components);
    else if (kind.kind == "shearPanelForces")
        rows.resolution = resolveElements(context, shearPanelForceChain(), request, "the forces of the shear panels",
                                          shearPanelForceFields());
    else if (kind.kind == "elementForces")
        rows.resolution = resolveElements(context, oneDimensionalForceChain(), request,
                                          "the forces of the one-dimensional elements", oneDimensionalFields());
    else
        rows.resolution =
            resolveElements(context, fastenerForceChain(), request, "the forces of the fasteners", fastenerFields());
    const std::vector<ElementFlat>& flats = rows.resolution->set.flats();
    rows.flats.reserve(flats.size());
    for (const ElementFlat& flat : flats)
    {
        FlatRows entry;
        entry.flat = &flat;
        entry.source = providerSource(rows.resolution->set.providers(), flat.provider);
        if (kind.kind == "fastenerForces")
        {
            const std::vector<double>& axial = flat.values.at("axial");
            entry.tension.resize(axial.size());
            for (std::size_t row = 0; row < axial.size(); ++row)
                entry.tension[row] = std::isnan(axial[row]) ? axial[row] : std::max(axial[row], 0.0);
        }
        rows.flats.push_back(std::move(entry));
    }
}

// What tells one place of an element from another, in the order of the places: the grid point (the centre, 0, first),
// the layer, the station along a beam (NO_STEP elsewhere) and the name. The name is a view of the text of a flat,
// which outlives the block.
using PlaceKey = std::tuple<std::int32_t, std::int32_t, double, std::string_view>;
struct Place
{
    std::size_t index = 0;
    double position = NOT_A_NUMBER; // the position the first row of the place gives
};

// A case: the label of its result set ("" with one), the subcase and the step (NO_STEP for none).
using CaseKey = std::tuple<std::string_view, std::int32_t, double>;

// True when two flats have the same rows: the same element, place and position in each, in the same order (the tables
// of the subcases of one element type mostly do). The places of the rows of the one serve for the other.
bool sameRowLabels(const ElementFlat& one, const ElementFlat& other)
{
    // the positions bit for bit: a row without one has NaN, which equals nothing
    return one.eid == other.eid && one.node == other.node && one.layer == other.layer &&
           one.location == other.location && one.position.size() == other.position.size() &&
           std::memcmp(one.position.data(), other.position.data(), one.position.size() * sizeof(double)) == 0;
}

// The slot of each row of a flat in the block (element * locations + place), SIZE_MAX for a row of an element that is
// not asked for.
constexpr std::size_t NO_SLOT = SIZE_MAX;

// The block of the forces of bars and beams, of shells, or of the stresses and
// strains of shells, solids, rods and shear panels, or of the plies (COMPOSITE-API-AUDIT P1) straight
// from their stored tables: the elements, places, cases, values and sources that the rows of the providers give through
// the passes of `readResultBlock`, without the flats (some 110 bytes a row made and freed for every subcase, most of
// the cost of such a block). `all` are the tables of stationForceTables, shellForceTables, storedStressTables or
// storedPlyTables, `fields` the fields of their `source`, `placeNames` the names of their place codes, `source` the
// text of the provider of their rows (with what a row of a stress or strain table completes, as the flats note it).
// `plies`: the global IDs of the plies and the components a ply block derives (the sources of their cases then those of
// the failure index, as the flats name them).
ResultBlock directForceBlock(const ResultQueryContext& context, ResultBlock block,
                             const std::optional<std::vector<std::int32_t>>& asked,
                             const std::vector<std::pair<std::string, std::int32_t>>& casePairs, bool narrow,
                             std::vector<DirectForceTable>& all, const std::vector<std::string>& fields,
                             const std::vector<std::string>& placeNames, const std::string& source,
                             const PlyRows* plies = nullptr)
{
    std::vector<const DirectForceTable*> tables;
    for (const DirectForceTable& table : all)
        if (casePairs.empty() || std::find(casePairs.begin(), casePairs.end(),
                                           std::make_pair(table.analysis, table.subcase)) != casePairs.end())
            tables.push_back(&table);
    const bool severalSets = context.attachedSetCount() > 1;
    // the elements asked for, else every one with a row by increasing ID
    std::vector<std::int32_t> every;
    if (!asked)
    {
        // the tables of the subcases of one element type mostly have the same elements: taken once
        const results::ResultData* before = nullptr;
        for (const DirectForceTable* table : tables)
        {
            const auto& ids = table->decoded->ids;
            if (before != nullptr && before->ids.size() == ids.size() &&
                std::equal(ids.begin(), ids.end(), before->ids.begin()))
                continue;
            before = table->decoded.get();
            every.insert(every.end(), ids.begin(), ids.end());
        }
        std::sort(every.begin(), every.end());
        every.erase(std::unique(every.begin(), every.end()), every.end());
    }
    const std::vector<std::int32_t>& elementOrder = asked ? *asked : every;

    // the elements in the order asked for, each once, and their positions by ID (looked up by a search, the row of
    // the same element as the row before not looked up again)
    std::vector<std::pair<std::int32_t, std::size_t>> byId;
    byId.reserve(elementOrder.size());
    if (std::adjacent_find(elementOrder.begin(), elementOrder.end(), std::greater_equal<>()) == elementOrder.end())
    {
        block.elementIds = elementOrder; // increasing: each once already
        for (std::size_t index = 0; index < elementOrder.size(); ++index)
            byId.emplace_back(elementOrder[index], index);
    }
    else
    {
        // the first place of each ID, then the IDs once in the order asked for
        for (std::size_t index = 0; index < elementOrder.size(); ++index)
            byId.emplace_back(elementOrder[index], index);
        std::stable_sort(byId.begin(), byId.end(),
                         [](const auto& one, const auto& other) { return one.first < other.first; });
        byId.erase(std::unique(byId.begin(), byId.end(),
                               [](const auto& one, const auto& other) { return one.first == other.first; }),
                   byId.end());
        std::vector<std::uint8_t> first(elementOrder.size(), 0);
        for (const auto& [id, index] : byId)
            first[index] = 1;
        std::vector<std::size_t> numberOf(elementOrder.size(), 0);
        for (std::size_t index = 0; index < elementOrder.size(); ++index)
            if (first[index] != 0)
            {
                numberOf[index] = block.elementIds.size();
                block.elementIds.push_back(elementOrder[index]);
            }
        for (auto& [id, index] : byId)
            index = numberOf[index];
    }
    block.elementCount = block.elementIds.size();

    // the rows of a table with the labels of the one before have its places (the subcases of one element type): the
    // tables of the same rows share their labels
    const auto sameLabels = [](const DirectForceTable& one, const DirectForceTable& other)
    {
        const results::ResultData& a = *one.decoded;
        const results::ResultData& b = *other.decoded;
        return one.bar == other.bar && one.station == other.station && one.location == other.location &&
               one.node == other.node && a.ids.size() == b.ids.size() &&
               (a.ids.data() == b.ids.data() || std::equal(a.ids.begin(), a.ids.end(), b.ids.begin()));
    };
    std::vector<std::size_t> layoutOf(tables.size());
    std::vector<std::size_t> layouts; // the first table of each layout
    for (std::size_t index = 0; index < tables.size(); ++index)
    {
        if (index > 0 && sameLabels(*tables[index], *tables[layouts.back()]))
        {
            layoutOf[index] = layouts.size() - 1;
            continue;
        }
        layoutOf[index] = layouts.size();
        layouts.push_back(index);
    }
    std::vector<const DirectForceTable*> layoutTables; // the first table of each layout
    for (const std::size_t first : layouts)
        layoutTables.push_back(tables[first]);
    // the element of each row of each layout
    constexpr std::uint32_t NO_ELEMENT = std::numeric_limits<std::uint32_t>::max();
    std::vector<std::vector<std::uint32_t>> elementOfRow(layouts.size());
    std::vector<std::size_t> given(layouts.size(), 0); // the rows of elements asked for
    for (std::size_t layout = 0; layout < layouts.size(); ++layout)
    {
        const DirectForceTable& table = *layoutTables[layout];
        const std::size_t perRow = table.bar ? 2 : 1;
        std::vector<std::uint32_t>& elements = elementOfRow[layout];
        elements.resize(table.location->size());
        std::size_t at = 0;
        for (std::size_t row = 0; row < table.location->size(); ++row)
        {
            const std::int32_t id = table.decoded->ids[row / perRow];
            if (at >= byId.size() || byId[at].first != id)
            {
                if (at + 1 < byId.size() && byId[at + 1].first == id)
                    ++at;
                else
                    at = static_cast<std::size_t>(
                        std::lower_bound(byId.begin(), byId.end(), std::make_pair(id, std::size_t{0})) - byId.begin());
            }
            // an element not asked for (a table taken whole): no place
            elements[row] =
                at < byId.size() && byId[at].first == id ? static_cast<std::uint32_t>(byId[at].second) : NO_ELEMENT;
            if (elements[row] != NO_ELEMENT)
                ++given[layout];
        }
    }
    // the tables that give no row of an element asked for are left out (the providers select their rows: they give
    // none of them)
    {
        std::vector<const DirectForceTable*> giving;
        std::vector<std::size_t> layoutOfGiving;
        for (std::size_t index = 0; index < tables.size(); ++index)
            if (given[layoutOf[index]] > 0)
            {
                giving.push_back(tables[index]);
                layoutOfGiving.push_back(layoutOf[index]);
            }
        tables = std::move(giving);
        layoutOf = std::move(layoutOfGiving);
    }
    // the places of each element: (grid point; layer; station, or NO_STEP when there is none; the code of the name),
    // sorted as PlaceKey sorts them (the codes in the order of the names), one run of `places` an element
    // (`placeStart`, `placesOf`): an element's rows come together, its run is then the last one while they come; one
    // that comes again later has its run moved to the end. With each, its position: the value of the position
    // component of the first row that gives it, at its first step (a fiber distance), else its station.
    using StationPlace = std::tuple<std::int32_t, std::int32_t, double, std::uint8_t>;
    struct PlaceEntry
    {
        StationPlace key;
        double position = NOT_A_NUMBER;
    };
    const auto keyOf = [](const DirectForceTable& table, std::size_t row)
    {
        const double station = table.station ? (*table.station)[row] : NOT_A_NUMBER;
        return StationPlace{table.node ? (*table.node)[row] : 0, table.layer ? (*table.layer)[row] : 0,
                            std::isnan(station) ? NO_STEP : station, (*table.location)[row]};
    };
    const auto positionOf = [](const DirectForceTable& table, std::size_t row, const StationPlace& key)
    {
        if (table.position < 0)
            return std::get<2>(key) == NO_STEP ? NOT_A_NUMBER : std::get<2>(key);
        const results::ResultData& decoded = *table.decoded;
        const std::size_t at =
            (row / (table.bar ? 2 : 1)) * decoded.componentCount + static_cast<std::size_t>(table.position);
        return decoded.type == results::ValueType::Float32 ? static_cast<double>(decoded.values<float>()[at])
                                                           : decoded.values<double>()[at];
    };
    const auto byKey = [](const PlaceEntry& entry, const StationPlace& key) { return entry.key < key; };
    std::vector<PlaceEntry> places;
    std::vector<std::size_t> placeStart(block.elementCount, 0);
    std::vector<std::uint32_t> placesOf(block.elementCount, 0);
    // the layout that first gave rows of each element, and the layouts whose elements another layout has too
    std::vector<std::int32_t> ownerOf(block.elementCount, -1);
    std::vector<std::uint8_t> overlapping(layouts.size(), 0);
    for (std::size_t layout = 0; layout < layouts.size(); ++layout)
    {
        const DirectForceTable& table = *layoutTables[layout];
        for (std::size_t row = 0; row < table.location->size(); ++row)
        {
            const std::uint32_t element = elementOfRow[layout][row];
            if (element == NO_ELEMENT)
                continue;
            if (ownerOf[element] < 0)
                ownerOf[element] = static_cast<std::int32_t>(layout);
            else if (ownerOf[element] != static_cast<std::int32_t>(layout))
                overlapping[layout] = overlapping[static_cast<std::size_t>(ownerOf[element])] = 1;
            std::uint32_t& count = placesOf[element];
            if (count > 0 && placeStart[element] + count != places.size())
            {
                // not the last run: moved to the end
                const std::size_t from = placeStart[element];
                placeStart[element] = places.size();
                for (std::size_t place = 0; place < count; ++place)
                    places.push_back(places[from + place]);
            }
            else if (count == 0)
                placeStart[element] = places.size();
            const StationPlace key = keyOf(table, row);
            const auto first = places.begin() + static_cast<std::ptrdiff_t>(placeStart[element]);
            const auto where = std::lower_bound(first, places.end(), key, byKey);
            if (where != places.end() && where->key == key)
                continue;
            places.insert(where, PlaceEntry{key, positionOf(table, row, key)});
            ++count;
        }
    }
    for (const std::uint32_t count : placesOf)
        block.locationCount = std::max<std::size_t>(block.locationCount, count);
    // the slot of each row of each layout
    std::vector<std::vector<std::size_t>> slots(layouts.size());
    for (std::size_t layout = 0; layout < layouts.size(); ++layout)
    {
        const DirectForceTable& table = *layoutTables[layout];
        slots[layout].resize(table.location->size());
        std::size_t last = 0; // the place of the row before
        for (std::size_t row = 0; row < table.location->size(); ++row)
        {
            const std::uint32_t element = elementOfRow[layout][row];
            if (element == NO_ELEMENT)
            {
                slots[layout][row] = NO_SLOT;
                continue;
            }
            // the place after that of the row before, mostly (the rows of an element in the order of its places)
            const PlaceEntry* own = places.data() + placeStart[element];
            const StationPlace key = keyOf(table, row);
            std::size_t place = row > 0 && elementOfRow[layout][row - 1] == element ? last + 1 : 0;
            if (place >= placesOf[element] || own[place].key != key)
                place = static_cast<std::size_t>(std::lower_bound(own, own + placesOf[element], key, byKey) - own);
            last = place;
            slots[layout][row] = element * block.locationCount + place;
        }
    }

    // the cases, in the order of their keys
    const auto caseOf = [&](const DirectForceTable& table, std::size_t step)
    {
        const double value = table.stepped ? table.decoded->steps[step] : NOT_A_NUMBER;
        return CaseKey{severalSets ? std::string_view(table.analysis) : std::string_view(), table.subcase,
                       std::isnan(value) ? NO_STEP : value};
    };
    std::map<CaseKey, std::size_t> caseIndex;
    for (const DirectForceTable* table : tables)
        for (std::size_t step = 0; step < table->decoded->stepCount; ++step)
            caseIndex.try_emplace(caseOf(*table, step), 0);
    std::size_t number = 0;
    for (auto& [key, index] : caseIndex)
    {
        index = number++;
        block.caseAnalyses.emplace_back(std::get<0>(key));
        block.caseSubcases.push_back(std::get<1>(key));
        block.caseSteps.push_back(std::get<2>(key) == NO_STEP ? NOT_A_NUMBER : std::get<2>(key));
    }
    block.caseCount = caseIndex.size();

    // the labels of the places
    const bool single = !tables.empty() && std::all_of(tables.begin(), tables.end(), [](const DirectForceTable* table)
                                                       { return precisionOf(*table->decoded) == Precision::Single; });
    const std::size_t placeCount = block.elementCount * block.locationCount;
    block.mask.assign(placeCount, 0);
    // the names as codes for a block asked narrow (the codes of the tables, one more for a masked place)
    const auto maskedCode = static_cast<std::uint8_t>(placeNames.size());
    if (narrow)
    {
        block.locationTexts = placeNames;
        block.locationTexts.emplace_back();
        block.locationCodes.assign(placeCount, maskedCode);
    }
    else
        block.locationNames.assign(placeCount, std::string());
    block.locationNodes.assign(placeCount, 0);
    block.layers.assign(placeCount, 0);
    block.layerIds.assign(placeCount, 0);
    block.positions.assign(placeCount, NOT_A_NUMBER);
    for (std::size_t element = 0; element < block.elementCount; ++element)
        for (std::size_t place = 0; place < placesOf[element]; ++place)
        {
            const std::size_t at = element * block.locationCount + place;
            const PlaceEntry& entry = places[placeStart[element] + place];
            block.mask[at] = 1;
            const std::uint8_t code = std::get<3>(entry.key);
            if (narrow)
                block.locationCodes[at] = code;
            else
                block.locationNames[at] = placeNames[code];
            block.locationNodes[at] = std::get<0>(entry.key);
            block.layers[at] = std::get<1>(entry.key);
            // the position component of a single precision block is that of a float32, as the providers' rows give it
            block.positions[at] = single && std::get<2>(entry.key) == NO_STEP
                                      ? static_cast<double>(static_cast<float>(entry.position))
                                      : entry.position;
        }

    // the global IDs of the plies, by place: the last layout that gives one, in order
    if (plies != nullptr)
        for (std::size_t layout = 0; layout < layouts.size(); ++layout)
            for (std::size_t row = 0; row < slots[layout].size(); ++row)
                if (slots[layout][row] != NO_SLOT)
                {
                    const std::int32_t id = plies->globalId(*layoutTables[layout], row);
                    if (id > 0)
                        block.layerIds[slots[layout][row]] = id;
                }
    // the values, each table on a thread of its own when no two tables give rows of one case (else in order: the
    // later rows of a place win, as the passes of the providers' rows write them)
    block.single = single;
    const bool narrowed = narrow && single;
    const std::size_t valueCount = block.caseCount * placeCount * block.componentCount;
    const results::DecodeContext decode = context.decode();
    // tables of one case write apart when they are of layouts that no other layout shares an element with (a bar's
    // and a beam's table of a subcase)
    std::vector<std::vector<std::size_t>> caseLayouts(block.caseCount);
    bool apart = true;
    for (std::size_t index = 0; apart && index < tables.size(); ++index)
        for (std::size_t step = 0; apart && step < tables[index]->decoded->stepCount; ++step)
        {
            std::vector<std::size_t>& taken = caseLayouts[caseIndex.at(caseOf(*tables[index], step))];
            const std::size_t layout = layoutOf[index];
            apart =
                taken.empty() ||
                (overlapping[layout] == 0 && std::find(taken.begin(), taken.end(), layout) == taken.end() &&
                 std::none_of(taken.begin(), taken.end(), [&](std::size_t other) { return overlapping[other] != 0; }));
            taken.push_back(layout);
        }
    // every slot of every case written by a row (no element without a row or with fewer places in a case): the block
    // is not filled with NaN first (a write of every one of its pages saved)
    bool covered = apart;
    if (covered)
    {
        std::vector<std::size_t> distinct(layouts.size(), 0);
        std::vector<std::uint8_t> seen(placeCount, 0);
        for (std::size_t layout = 0; layout < layouts.size(); ++layout)
        {
            std::fill(seen.begin(), seen.end(), std::uint8_t{0});
            for (const std::size_t slot : slots[layout])
                if (slot != NO_SLOT && seen[slot] == 0)
                {
                    seen[slot] = 1;
                    ++distinct[layout];
                }
        }
        for (const std::vector<std::size_t>& taken : caseLayouts)
        {
            std::size_t written = 0;
            for (const std::size_t layout : taken)
                written += distinct[layout];
            covered = covered && written == placeCount;
        }
    }
    if (narrowed && covered)
        block.singleValues.resize(valueCount); // every value written by the rows, on the threads
    else if (narrowed)
    {
        // NaN where no row writes, first written on the threads
        block.singleValues.resize(valueCount);
        constexpr std::size_t CHUNK = std::size_t{1} << 18U;
        const auto nan = [&](std::size_t chunk)
        {
            float* first = block.singleValues.data() + chunk * CHUNK;
            std::fill(first, first + std::min(CHUNK, valueCount - chunk * CHUNK),
                      std::numeric_limits<float>::quiet_NaN());
        };
        const std::size_t chunks = (valueCount + CHUNK - 1) / CHUNK;
        if (decode.pool != nullptr && chunks > 1)
            base::parallelFor(*decode.pool, chunks, nan, decode.token);
        else
            for (std::size_t chunk = 0; chunk < chunks; ++chunk)
                nan(chunk);
    }
    else
        block.values.assign(valueCount, NOT_A_NUMBER);
    double* wideValues = narrowed ? nullptr : block.values.data();
    float* singleValues = narrowed ? block.singleValues.data() : nullptr;
    // the field of each component of the block (a derived component of the plies: fields.size() + its number)
    std::vector<std::size_t> fieldOf;
    for (const std::string& name : block.components)
    {
        std::size_t field = static_cast<std::size_t>(std::find(fields.begin(), fields.end(), name) - fields.begin());
        if (field == fields.size() && plies != nullptr)
            field += static_cast<std::size_t>(
                std::find(PlyRows::derivedNames().begin(), PlyRows::derivedNames().end(), name) -
                PlyRows::derivedNames().begin());
        fieldOf.push_back(field);
    }
    const bool derived = plies != nullptr && plies->derives();
    // per table and step, the providers of the failure index of its rows (bits 1 stored, 2 recomputed)
    std::vector<std::vector<std::uint8_t>> providerBits(derived ? tables.size() : 0);
    // the scalars a stress or strain row completes are computed only when the block takes one (else only what would be
    // computed, for the sources)
    const bool derivedAsked = std::any_of(block.components.begin(), block.components.end(),
                                          [](const std::string& name)
                                          {
                                              return name == "o1" || name == "o2" || name == "o3" || name == "e1" ||
                                                     name == "e2" || name == "e3" || name == "vonMises" ||
                                                     name == "maxShear";
                                          });
    // per table and step, the completions of its rows (bit `c` set for the bits `c` of ScalarCompletion)
    std::vector<std::vector<std::uint8_t>> completed(tables.size());
    const auto fillCompleted = [&](std::size_t index)
    {
        const DirectForceTable& table = *tables[index];
        const results::ResultData& decoded = *table.decoded;
        const std::vector<std::size_t>& slotOfRow = slots[layoutOf[index]];
        const std::size_t width = decoded.componentCount;
        const float* singles = decoded.type == results::ValueType::Float32 ? decoded.values<float>().data() : nullptr;
        const double* doubles = singles == nullptr ? decoded.values<double>().data() : nullptr;
        const bool solid = table.completion == DirectForceTable::Completion::Solid;
        constexpr std::size_t FIRST_PRINCIPAL = 7; // o1 (e1) in the fields of a row, then o2 and o3
        std::vector<std::uint8_t>& seen = completed[index];
        seen.assign(decoded.stepCount, 0);
        for (std::size_t step = 0; step < decoded.stepCount; ++step)
        {
            const std::size_t caseNumber = caseIndex.at(caseOf(table, step));
            const std::size_t stepBase = step * decoded.rowCount * width;
            for (std::size_t row = 0; row < slotOfRow.size(); ++row)
            {
                if (slotOfRow[row] == NO_SLOT)
                    continue;
                const std::size_t rowBase = stepBase + row * width;
                StressRow fieldsOfRow;
                for (std::size_t field = 0; field < STRESS_FIELD_COUNT; ++field)
                {
                    const int column = table.source[0][field];
                    fieldsOfRow[field] = column < 0 ? NOT_A_NUMBER
                                         : singles != nullptr
                                             ? static_cast<double>(singles[rowBase + static_cast<std::size_t>(column)])
                                             : doubles[rowBase + static_cast<std::size_t>(column)];
                }
                // a solid's principal values sorted, as flattenStressTable sorts them
                if (solid && !std::isnan(fieldsOfRow[FIRST_PRINCIPAL]) &&
                    !std::isnan(fieldsOfRow[FIRST_PRINCIPAL + 1]) && !std::isnan(fieldsOfRow[FIRST_PRINCIPAL + 2]))
                    std::sort(fieldsOfRow.begin() + FIRST_PRINCIPAL, fieldsOfRow.begin() + FIRST_PRINCIPAL + 3,
                              std::greater<>());
                const std::uint8_t bits = completeScalarRow(fieldsOfRow, table.strain, solid, derivedAsked);
                seen[step] = static_cast<std::uint8_t>(seen[step] | (1U << bits));
                const std::size_t base = (caseNumber * placeCount + slotOfRow[row]) * block.componentCount;
                for (std::size_t component = 0; component < block.componentCount; ++component)
                {
                    const double value = fieldsOfRow[fieldOf[component]];
                    if (singleValues != nullptr)
                        singleValues[base + component] = static_cast<float>(value);
                    else
                        wideValues[base + component] = single ? static_cast<double>(static_cast<float>(value)) : value;
                }
            }
        }
    };
    // the rows of the plies with what a block derives of them (PlyRows), each row's stored fields read once
    const auto fillPlies = [&](std::size_t index)
    {
        const DirectForceTable& table = *tables[index];
        const results::ResultData& decoded = *table.decoded;
        const std::vector<std::size_t>& slotOfRow = slots[layoutOf[index]];
        const std::size_t width = decoded.componentCount;
        const float* singles = decoded.type == results::ValueType::Float32 ? decoded.values<float>().data() : nullptr;
        const double* doubles = singles == nullptr ? decoded.values<double>().data() : nullptr;
        std::vector<std::uint8_t>& bits = providerBits[index];
        bits.assign(decoded.stepCount, 0);
        std::array<double, PlyRows::FIELDS + PlyRows::DerivedCount> values{};
        for (std::size_t step = 0; step < decoded.stepCount; ++step)
        {
            const std::size_t caseNumber = caseIndex.at(caseOf(table, step));
            const std::size_t stepBase = step * decoded.rowCount * width;
            for (std::size_t row = 0; row < slotOfRow.size(); ++row)
            {
                if (slotOfRow[row] == NO_SLOT)
                    continue;
                const std::size_t rowBase = stepBase + row * width;
                for (std::size_t field = 0; field < PlyRows::FIELDS; ++field)
                {
                    const int column = table.source[0][field];
                    values[field] = column < 0 ? NOT_A_NUMBER
                                    : singles != nullptr
                                        ? static_cast<double>(singles[rowBase + static_cast<std::size_t>(column)])
                                        : doubles[rowBase + static_cast<std::size_t>(column)];
                }
                bits[step] = static_cast<std::uint8_t>(
                    bits[step] | plies->derive(table, step, row, values.data(), values.data() + PlyRows::FIELDS));
                const std::size_t base = (caseNumber * placeCount + slotOfRow[row]) * block.componentCount;
                for (std::size_t component = 0; component < block.componentCount; ++component)
                {
                    const double value = values[fieldOf[component]];
                    if (singleValues != nullptr)
                        singleValues[base + component] = static_cast<float>(value);
                    else
                        wideValues[base + component] = single ? static_cast<double>(static_cast<float>(value)) : value;
                }
            }
        }
    };
    const auto fill = [&](std::size_t index)
    {
        if (derived)
        {
            fillPlies(index);
            return;
        }
        if (tables[index]->completion != DirectForceTable::Completion::None)
        {
            fillCompleted(index);
            return;
        }
        const DirectForceTable& table = *tables[index];
        const results::ResultData& decoded = *table.decoded;
        const std::vector<std::size_t>& slotOfRow = slots[layoutOf[index]];
        const std::size_t perRow = table.bar ? 2 : 1;
        const std::size_t width = decoded.componentCount;
        const float* singles = decoded.type == results::ValueType::Float32 ? decoded.values<float>().data() : nullptr;
        const double* doubles = singles == nullptr ? decoded.values<double>().data() : nullptr;
        std::array<std::array<int, 16>, 2> from{};
        for (std::size_t end = 0; end < perRow; ++end)
            for (std::size_t component = 0; component < block.componentCount; ++component)
                from[end][component] = fieldOf[component] < fields.size() ? table.source[end][fieldOf[component]] : -1;
        for (std::size_t step = 0; step < decoded.stepCount; ++step)
        {
            const std::size_t caseNumber = caseIndex.at(caseOf(table, step));
            const std::size_t stepBase = step * decoded.rowCount * width;
            for (std::size_t row = 0; row < slotOfRow.size(); ++row)
            {
                if (slotOfRow[row] == NO_SLOT)
                    continue;
                const std::size_t base = (caseNumber * placeCount + slotOfRow[row]) * block.componentCount;
                const std::size_t rowBase = stepBase + (row / perRow) * width;
                const auto& columns = from[row % perRow];
                for (std::size_t component = 0; component < block.componentCount; ++component)
                {
                    const int column = columns[component];
                    const double value = column < 0 ? NOT_A_NUMBER
                                         : singles != nullptr
                                             ? static_cast<double>(singles[rowBase + static_cast<std::size_t>(column)])
                                             : doubles[rowBase + static_cast<std::size_t>(column)];
                    if (singleValues != nullptr)
                        singleValues[base + component] = static_cast<float>(value);
                    else
                        wideValues[base + component] = single ? static_cast<double>(static_cast<float>(value)) : value;
                }
            }
        }
    };
    if (apart && decode.pool != nullptr && tables.size() > 1)
        base::parallelFor(*decode.pool, tables.size(), fill, decode.token);
    else
        for (std::size_t index = 0; index < tables.size(); ++index)
            fill(index);
    // the sources of each case: the provider, with what its rows completed (as the notes of the flats give it)
    std::vector<std::uint8_t> caseCompleted(block.caseCount, 0);
    bool strain = false;
    for (std::size_t index = 0; index < tables.size(); ++index)
    {
        strain = strain || tables[index]->strain;
        // a table that completes nothing gives rows of elements asked for at each of its steps (the tables that give
        // none are left out): its rows have the source alone
        const bool none = tables[index]->completion == DirectForceTable::Completion::None;
        for (std::size_t step = 0; step < tables[index]->decoded->stepCount; ++step)
        {
            std::uint8_t& seen = caseCompleted[caseIndex.at(caseOf(*tables[index], step))];
            seen = static_cast<std::uint8_t>(seen | (none ? 1U : completed[index][step]));
        }
    }
    block.sources.reserve(block.caseCount);
    if (derived)
    {
        // the providers of the failure index of the rows of each case, as the flats' rows name them
        std::vector<std::uint8_t> caseBits(block.caseCount, 0);
        for (std::size_t index = 0; index < tables.size(); ++index)
            for (std::size_t step = 0; step < tables[index]->decoded->stepCount; ++step)
            {
                std::uint8_t& seen = caseBits[caseIndex.at(caseOf(*tables[index], step))];
                seen = static_cast<std::uint8_t>(seen | providerBits[index][step]);
            }
        for (const std::uint8_t bits : caseBits)
            block.sources.push_back(plies->sourceOf(bits));
        return block;
    }
    for (std::size_t caseNumber = 0; caseNumber < block.caseCount; ++caseNumber)
    {
        if (caseCompleted[caseNumber] <= 1)
        {
            block.sources.push_back(source); // nothing completed
            continue;
        }
        std::set<std::string> texts;
        for (unsigned bits = 0; bits < 8; ++bits)
            if ((caseCompleted[caseNumber] & (1U << bits)) != 0)
                texts.insert(bits == 0
                                 ? source
                                 : source + "; " + ScalarCompletion::noteOf(static_cast<std::uint8_t>(bits), strain));
        std::string joined;
        for (const std::string& text : texts)
            joined += (joined.empty() ? "" : "; ") + text;
        block.sources.push_back(std::move(joined));
    }
    return block;
}

// The block of a kind read straight from its stored tables, or nothing when the providers must be asked
// (stationForceTables, shellForceTables).
std::optional<ResultBlock> directBlock(const ResultQueryContext& context, const ResultBlockKind& kind,
                                       const ResultBlock& block, const std::optional<std::vector<std::int32_t>>& asked,
                                       const std::vector<std::pair<std::string, std::int32_t>>& casePairs, bool narrow)
{
    const ElementRequest request = ElementRequest::from(context.arguments());
    std::string source;
    if (kind.kind == "elementForces")
    {
        std::optional<std::vector<DirectForceTable>> found = stationForceTables(context, request, source);
        if (!found)
            return std::nullopt;
        return directForceBlock(context, block, asked, casePairs, narrow, *found, oneDimensionalFields(),
                                stationPlaceNames(), source);
    }
    if (kind.kind == "shellForces")
    {
        std::optional<std::vector<DirectForceTable>> found = shellForceTables(context, request, source);
        if (!found)
            return std::nullopt;
        return directForceBlock(context, block, asked, casePairs, narrow, *found, kind.components, shellPlaceNames(),
                                source);
    }
    if (kind.kind == "stress" || kind.kind == "strain")
    {
        const bool strain = kind.kind == "strain";
        std::optional<std::vector<DirectForceTable>> found = storedStressTables(context, request, strain, source);
        if (!found)
            return std::nullopt;
        return directForceBlock(context, block, asked, casePairs, narrow, *found,
                                strain ? strainFields() : stressFields(), shellPlaceNames(), source);
    }
    if (kind.kind == "plyStress" || kind.kind == "plyStrain")
    {
        // the plies (COMPOSITE-API-AUDIT P1): the stored rows straight into the block, what it derives of them only
        // when it asks for it
        const bool strain = kind.kind == "plyStrain";
        std::optional<std::vector<DirectForceTable>> found = storedPlyTables(context, request, strain);
        if (!found)
            return std::nullopt;
        const PlyRows plies(context, request, *found, block.components);
        if (!plies.usable())
            return std::nullopt;
        static const std::vector<std::string> PLACE_NAMES = {"ply"};
        return directForceBlock(context, block, asked, casePairs, narrow, *found,
                                strain ? plyStrainFields() : plyStressFields(), PLACE_NAMES, plyTableSource(strain),
                                &plies);
    }
    return std::nullopt;
}

} // namespace

const std::vector<ResultBlockKind>& resultBlockKinds()
{
    static const std::vector<ResultBlockKind> KINDS = {
        {.kind = "stress",
         .query = "stressTable",
         .components = {"oxx", "oyy", "ozz", "txy", "tyz", "tzx", "o1", "o2", "o3", "vonMises", "maxShear", "axial",
                        "torsion"},
         .layered = true,
         .nodes = true,
         .positionField = "fiberDistance"},
        {.kind = "strain",
         .query = "strainTable",
         .components = {"exx", "eyy", "ezz", "exy", "eyz", "ezx", "e1", "e2", "e3", "vonMises", "maxShear", "axial",
                        "torsion"},
         .layered = true,
         .nodes = true,
         .positionField = "fiberDistance"},
        {.kind = "plyStress",
         .query = "compositePlyTable",
         .components = {"o11", "o22", "t12", "t1z", "t2z", "omax", "omin", "maxShear", "failureIndex", "strengthRatio",
                        "bondIndex", "fiberIndex", "matrixIndex"},
         .onlyPlace = "ply",
         .layered = true,
         .layerIdField = "plyId"},
        {.kind = "plyStrain",
         .query = "compositePlyTable",
         .components = {"e11", "e22", "e12", "e1z", "e2z", "emax", "emin", "maxShear"},
         .onlyPlace = "ply",
         .layered = true,
         .layerIdField = "plyId"},
        {.kind = "shellForces",
         .query = "shellFluxes",
         .components = {"nx", "ny", "nxy", "mx", "my", "mxy", "qx", "qy"},
         .nodes = true},
        {.kind = "shearPanelForces",
         .query = "shellFluxes",
         .components = {"shearFlow", "rod12", "rod23", "rod34", "rod41"},
         .onlyPlace = "center"},
        {.kind = "laminateStrains",
         .query = "shellFluxes",
         .components = {"ex0", "ey0", "gxy0", "kx", "ky", "kxy"},
         .nodes = true},
        {.kind = "sandwich",
         .query = "shellFluxes",
         .components = {"n1x", "n1y", "n1xy", "n2x", "n2y", "n2xy", "tcx", "tcy"},
         .nodes = true},
        {.kind = "elementForces",
         .query = "elementForcesTable",
         .components = {"axial", "shear1", "shear2", "torque", "moment1", "moment2"},
         .positionField = "station"},
        {.kind = "fastenerForces",
         .query = "fastenerForces",
         .components = {"axial", "tension", "shear1", "shear2", "shear", "torque", "moment1", "moment2"},
         .onlyPlace = "center"},
    };
    return KINDS;
}

const ResultBlockKind& resultBlockKind(std::string_view kind)
{
    for (const ResultBlockKind& entry : resultBlockKinds())
    {
        if (entry.kind == kind)
            return entry;
    }
    std::string names;
    for (const ResultBlockKind& entry : resultBlockKinds())
        names += (names.empty() ? "" : ", ") + entry.kind;
    throw QueryError("readBlock: the kind is one of " + names + ", not '" + std::string(kind) + "'");
}

ResultBlock readResultBlock(const QueryRun& run, const ResultBlockKind& kind,
                            const std::vector<std::string>& components,
                            const std::optional<std::vector<std::int32_t>>& elementOrder,
                            const std::vector<std::pair<std::string, std::int32_t>>& casePairs, bool narrow,
                            const SandwichCore& sandwichCore)
{
    if ((sandwichCore.ply || !sandwichCore.byProperty.empty()) && kind.kind != "sandwich")
        throw QueryError("readBlock: core declares the core ply of the kind \"sandwich\", not of '" + kind.kind + "'");
    ResultBlock block;
    block.components = components.empty() ? kind.components : components;
    for (const std::string& name : block.components)
    {
        if (std::find(kind.components.begin(), kind.components.end(), name) != kind.components.end())
            continue;
        std::string known;
        for (const std::string& component : kind.components)
            known += (known.empty() ? "" : ", ") + component;
        throw QueryError("readBlock: '" + name + "' is not a component of " + kind.kind + " (" + kind.query +
                         " gives " + known + ")");
    }
    block.componentCount = block.components.size();

    const NamedQuery* query = NamedQueryRegistry::instance().find(kind.query);
    const ResultQueryContext context(run, query->info());
    if (std::optional<ResultBlock> direct = directBlock(context, kind, block, elementOrder, casePairs, narrow))
        return std::move(*direct);
    KindRows rows;
    resolveKind(context, kind, rows, sandwichCore, block.components);
    const bool severalSets = context.attachedSetCount() > 1;

    // the values each flat gives of each component (the rows of nulls `onMissing="null"` adds are left out: an
    // element without a row is masked)
    std::vector<FlatRows*> kept;
    for (FlatRows& entry : rows.flats)
    {
        const ElementFlat& flat = *entry.flat;
        if (flat.provider < 0 || flat.rows() == 0)
            continue;
        if (!casePairs.empty() && std::find(casePairs.begin(), casePairs.end(),
                                            std::make_pair(flat.analysis, flat.subcase)) == casePairs.end())
            continue;
        if (flat.isComplex())
            throw QueryError("readBlock takes real results: the rows of '" + kind.kind +
                             "' are complex (take one phase with the result table's atPhase first)");
        for (const std::string& name : block.components)
        {
            const auto found = flat.values.find(name);
            entry.values.push_back(name == "tension" ? &entry.tension
                                                     : (found != flat.values.end() ? &found->second : nullptr));
        }
        kept.push_back(&entry);
    }

    // The elements in the order asked for, else by increasing ID.
    std::unordered_map<std::int32_t, std::size_t> elementIndex;
    if (elementOrder)
        block.elementIds = *elementOrder;
    else
    {
        // the rows of an element come together (its places, its fibres): one ID for each run
        for (const FlatRows* entry : kept)
        {
            const std::vector<std::int32_t>& ids = entry->flat->eid;
            for (std::size_t row = 0; row < ids.size(); ++row)
                if (row == 0 || ids[row] != ids[row - 1])
                    block.elementIds.push_back(ids[row]);
        }
        std::sort(block.elementIds.begin(), block.elementIds.end());
    }
    std::vector<std::int32_t> unique;
    for (const std::int32_t id : block.elementIds)
    {
        if (elementIndex.emplace(id, unique.size()).second)
            unique.push_back(id);
    }
    block.elementIds = std::move(unique);
    block.elementCount = block.elementIds.size();

    const auto placeOf = [&](const FlatRows& entry, std::size_t row)
    {
        const ElementFlat& flat = *entry.flat;
        const std::string_view name =
            !kind.onlyPlace.empty() ? std::string_view(kind.onlyPlace)
                                    : (row < flat.location.size() ? std::string_view(flat.location[row]) : "center");
        const double station =
            kind.positionField == "station" && row < flat.position.size() && !std::isnan(flat.position[row])
                ? flat.position[row]
                : NO_STEP;
        return PlaceKey{kind.nodes && row < flat.node.size() ? flat.node[row] : 0,
                        kind.layered && row < flat.layer.size() ? flat.layer[row] : 0, station, name};
    };
    // A query table gives its values in single precision when every block of rows is (RowAccumulator): the block gives
    // the same numbers.
    const bool single = !kept.empty() && std::all_of(kept.begin(), kept.end(), [](const FlatRows* entry)
                                                     { return entry->flat->precision == Precision::Single; });
    const auto asStored = [single](double value)
    { return single ? static_cast<double>(static_cast<float>(value)) : value; };
    block.single = single;
    const auto positionOf = [&](const FlatRows& entry, std::size_t row)
    {
        const ElementFlat& flat = *entry.flat;
        if (kind.positionField == "station")
            return row < flat.position.size() ? flat.position[row] : NOT_A_NUMBER;
        if (kind.positionField.empty())
            return NOT_A_NUMBER;
        const auto found = flat.values.find(kind.positionField);
        return found != flat.values.end() ? asStored(found->second[row]) : NOT_A_NUMBER;
    };
    const auto caseOf = [&](const ElementFlat& flat, std::size_t row)
    {
        return CaseKey{severalSets ? std::string_view(flat.analysis) : std::string_view(), flat.subcase,
                       flat.step.empty() || std::isnan(flat.step[row]) ? NO_STEP : flat.step[row]};
    };

    // Whether each flat has the rows of the one before (the subcases of one element type mostly do), and whether its
    // rows are of one case (one step, or none), found on the threads of the run: the comparisons are most of the time
    // of the passes below. The rows of the one before are those of the last flat a pass read the rows of (equal rows
    // are equal to the same rows).
    std::vector<std::uint8_t> sameAsBefore(kept.size(), 0);
    std::vector<std::uint8_t> oneCase(kept.size(), 0);
    {
        const results::DecodeContext decode = context.decode();
        const auto compare = [&](std::size_t index)
        {
            const ElementFlat& flat = *kept[index]->flat;
            if (index > 0)
                sameAsBefore[index] = sameRowLabels(*kept[index - 1]->flat, flat) ? 1 : 0;
            const auto stepOf = [&](std::size_t row) { return std::isnan(flat.step[row]) ? NO_STEP : flat.step[row]; };
            bool single = true;
            for (std::size_t row = 1; single && row < flat.step.size(); ++row)
                single = stepOf(row) == stepOf(0);
            oneCase[index] = single ? 1 : 0;
        };
        if (decode.pool != nullptr && kept.size() > 2)
            base::parallelFor(*decode.pool, kept.size(), compare, decode.token);
        else
            for (std::size_t index = 0; index < kept.size(); ++index)
                compare(index);
    }

    // First pass: the cases and the places of each element, in their order. A flat with the rows of the one before has
    // its places already: only its cases are new.
    std::map<CaseKey, std::size_t> caseIndex;
    std::vector<std::map<PlaceKey, Place>> places(block.elementCount);
    // the element and the place of each row of the flats whose slots the second pass finds (those without the rows of
    // the one before), as this pass finds them: they are not looked up again
    struct RowPlace
    {
        std::size_t element = NO_SLOT;
        const Place* place = nullptr;
    };
    std::vector<std::vector<RowPlace>> rowPlaces(kept.size());
    bool placedAnyRow = false; // whether the last flat whose places were read has a row of an element asked for
    for (std::size_t flatIndex = 0; flatIndex < kept.size(); ++flatIndex)
    {
        const FlatRows* entry = kept[flatIndex];
        const ElementFlat& flat = *entry->flat;
        if (flatIndex > 0 && oneCase[flatIndex] != 0 && sameAsBefore[flatIndex] != 0)
        {
            if (placedAnyRow)
                caseIndex.try_emplace(caseOf(flat, 0), 0);
            continue;
        }
        placedAnyRow = false;
        std::optional<CaseKey> lastCase;
        std::vector<RowPlace>* found = nullptr;
        if (flatIndex == 0 || sameAsBefore[flatIndex] == 0)
        {
            found = &rowPlaces[flatIndex];
            found->resize(flat.rows());
        }
        for (std::size_t row = 0; row < flat.rows(); ++row)
        {
            const auto element = elementIndex.find(flat.eid[row]);
            if (element == elementIndex.end())
                continue;
            placedAnyRow = true;
            if (const CaseKey key = caseOf(flat, row); !lastCase || key != *lastCase)
            {
                caseIndex.try_emplace(key, 0);
                lastCase = key;
            }
            auto [place, added] = places[element->second].try_emplace(placeOf(*entry, row));
            if (added)
                place->second.position = positionOf(*entry, row);
            if (found != nullptr)
                (*found)[row] = RowPlace{element->second, &place->second};
        }
    }
    std::size_t number = 0;
    for (auto& [key, index] : caseIndex)
    {
        index = number++;
        block.caseAnalyses.emplace_back(std::get<0>(key));
        block.caseSubcases.push_back(std::get<1>(key));
        block.caseSteps.push_back(std::get<2>(key) == NO_STEP ? NOT_A_NUMBER : std::get<2>(key));
    }
    block.caseCount = caseIndex.size();
    for (auto& own : places)
    {
        std::size_t slot = 0;
        for (auto& [key, place] : own)
            place.index = slot++;
        block.locationCount = std::max(block.locationCount, own.size());
    }

    // The labels of the places.
    const std::size_t placeCount = block.elementCount * block.locationCount;
    block.mask.assign(placeCount, 0);
    block.locationNames.assign(placeCount, std::string());
    block.locationNodes.assign(placeCount, 0);
    block.layers.assign(placeCount, 0);
    block.layerIds.assign(placeCount, 0);
    block.positions.assign(placeCount, NOT_A_NUMBER);
    for (std::size_t element = 0; element < block.elementCount; ++element)
    {
        for (const auto& [key, place] : places[element])
        {
            const std::size_t at = element * block.locationCount + place.index;
            block.mask[at] = 1;
            block.locationNames[at] = std::string(std::get<3>(key));
            block.locationNodes[at] = std::get<0>(key);
            block.layers[at] = std::get<1>(key);
            block.positions[at] = place.position;
        }
    }

    // Second pass: the values, the global IDs of the layers and the sources of each case. The slots of the rows are
    // found once for the flats that have the same rows. The rows of a flat are mostly of one case (a step at a time):
    // the case of the row before is tried first.
    // PERFORMANCE: a block of single precision values asked narrow is made float32 (the values of
    // before, half the memory, no copy to narrow them after)
    const bool narrowed = narrow && single;
    const std::size_t valueCount = block.caseCount * placeCount * block.componentCount;
    if (narrowed)
        block.singleValues.assign(valueCount, std::numeric_limits<float>::quiet_NaN());
    else
        block.values.assign(valueCount, NOT_A_NUMBER);
    double* wideValues = narrowed ? nullptr : block.values.data();
    float* singleValues = narrowed ? block.singleValues.data() : nullptr;
    std::vector<std::set<std::string>> caseSources(block.caseCount);
    std::vector<std::shared_ptr<const std::vector<std::size_t>>> slotsOf(kept.size());
    for (std::size_t flatIndex = 0; flatIndex < kept.size(); ++flatIndex)
    {
        if (flatIndex > 0 && sameAsBefore[flatIndex] != 0)
        {
            slotsOf[flatIndex] = slotsOf[flatIndex - 1];
            continue;
        }
        const ElementFlat& flat = *kept[flatIndex]->flat;
        auto slots = std::make_shared<std::vector<std::size_t>>(flat.rows(), NO_SLOT);
        const std::vector<RowPlace>& found = rowPlaces[flatIndex];
        for (std::size_t row = 0; row < flat.rows(); ++row)
            if (found[row].place != nullptr)
                (*slots)[row] = found[row].element * block.locationCount + found[row].place->index;
        slotsOf[flatIndex] = std::move(slots);
        std::vector<RowPlace>().swap(rowPlaces[flatIndex]);
    }
    const auto fill = [&](std::size_t flatIndex)
    {
        const FlatRows* entry = kept[flatIndex];
        const ElementFlat& flat = *entry->flat;
        const std::vector<std::size_t>& slotOfRow = *slotsOf[flatIndex];
        CaseKey lastCase{std::string_view(), 0, NOT_A_NUMBER};
        std::size_t caseNumber = SIZE_MAX;
        // the source of the row before, in this case: a row with the same adds nothing
        bool sourceSeen = false;
        const ProviderInfo* lastProvider = nullptr;
        std::uint8_t lastComputed = 0;
        for (std::size_t row = 0; row < flat.rows(); ++row)
        {
            const std::size_t at = slotOfRow[row];
            if (at == NO_SLOT)
                continue;
            // a flat of one step (or none) is of one case
            if (caseNumber == SIZE_MAX || oneCase[flatIndex] == 0)
            {
                const CaseKey key = caseOf(flat, row);
                if (caseNumber == SIZE_MAX || key != lastCase)
                {
                    caseNumber = caseIndex.at(key);
                    lastCase = key;
                    sourceSeen = false;
                }
            }
            const std::size_t base = (caseNumber * placeCount + at) * block.componentCount;
            for (std::size_t component = 0; component < block.componentCount; ++component)
            {
                const std::vector<double>* column = entry->values[component];
                if (column == nullptr)
                    continue;
                if (singleValues != nullptr)
                    singleValues[base + component] = static_cast<float>((*column)[row]);
                else
                    wideValues[base + component] = asStored((*column)[row]);
            }

            // the source: the provider of the flat, with the note of the row; or the provider of the row
            const ProviderInfo* rowProvider = nullptr;
            std::uint8_t computed = 0;
            if (entry->rowProviders != nullptr)
            {
                const double provider = (*entry->rowProviders)[row];
                if (provider < 0.0)
                    continue;
                rowProvider = &(*entry->providers)[static_cast<std::size_t>(provider)];
            }
            else if (entry->completion != nullptr)
                computed = entry->completion->computed[row];
            if (sourceSeen && rowProvider == lastProvider && computed == lastComputed)
                continue;
            std::string text =
                rowProvider != nullptr
                    ? sourceText(rowProvider->name, rowProvider->approximate)
                    : (computed == 0 ? entry->source : entry->source + "; " + entry->completion->note(row));
            if (!text.empty())
                caseSources[caseNumber].insert(std::move(text));
            sourceSeen = true;
            lastProvider = rowProvider;
            lastComputed = computed;
        }
    };
    // The global IDs of the layers, by place: the last flat that gives one, in order. A flat with the rows and the IDs
    // of the one before writes what it wrote.
    if (!kind.layerIdField.empty())
        for (std::size_t flatIndex = 0; flatIndex < kept.size(); ++flatIndex)
        {
            const std::vector<double>& layerIds = kept[flatIndex]->flat->values.at(kind.layerIdField);
            if (flatIndex > 0 && sameAsBefore[flatIndex] != 0 &&
                std::memcmp(layerIds.data(), kept[flatIndex - 1]->flat->values.at(kind.layerIdField).data(),
                            layerIds.size() * sizeof(double)) == 0)
                continue;
            const std::vector<std::size_t>& slotOfRow = *slotsOf[flatIndex];
            for (std::size_t row = 0; row < layerIds.size(); ++row)
                if (slotOfRow[row] != NO_SLOT && !std::isnan(layerIds[row]))
                    block.layerIds[slotOfRow[row]] = static_cast<std::int32_t>(layerIds[row]);
        }
    // The flats fill on the threads of the run when what they write cannot meet: each of one case, no two of the same
    // case (the same case and place in two flats would be the one of the later flat). Else one after the other, in
    // order.
    bool apart = std::all_of(oneCase.begin(), oneCase.end(), [](std::uint8_t single) { return single != 0; });
    if (apart)
    {
        std::vector<std::uint8_t> caseTaken(block.caseCount, 0);
        for (std::size_t flatIndex = 0; apart && flatIndex < kept.size(); ++flatIndex)
        {
            const auto found = caseIndex.find(caseOf(*kept[flatIndex]->flat, 0));
            if (found == caseIndex.end())
                continue; // no row of an element asked for: nothing to write
            apart = caseTaken[found->second] == 0;
            caseTaken[found->second] = 1;
        }
    }
    const results::DecodeContext decode = context.decode();
    if (apart && decode.pool != nullptr && kept.size() > 1)
        base::parallelFor(*decode.pool, kept.size(), fill, decode.token);
    else
        for (std::size_t flatIndex = 0; flatIndex < kept.size(); ++flatIndex)
            fill(flatIndex);
    for (const std::set<std::string>& names : caseSources)
    {
        std::string joined;
        for (const std::string& name : names)
            joined += (joined.empty() ? "" : "; ") + name;
        block.sources.push_back(joined);
    }
    return block;
}

} // namespace oor::query
