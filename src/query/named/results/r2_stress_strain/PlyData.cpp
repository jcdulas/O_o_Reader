#include "PlyData.hpp"

#include "oor/base/threading/ParallelFor.hpp"
#include "oor/query/results/TableValues.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <optional>
#include <tuple>
#include <unordered_map>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// A row within the rows of one subcase: the step, the element and the ply.
struct RowKey
{
    std::uint64_t step = 0;
    std::int32_t eid = 0;
    std::int32_t layer = 0;
    bool operator==(const RowKey& other) const noexcept
    {
        return step == other.step && eid == other.eid && layer == other.layer;
    }
};

struct RowKeyHash
{
    std::size_t operator()(const RowKey& key) const noexcept
    {
        return static_cast<std::size_t>(key.step * 0x9E3779B97F4A7C15ULL) ^
               (static_cast<std::size_t>(static_cast<std::uint32_t>(key.eid)) * 0xC2B2AE3D27D4EB4FULL) ^
               static_cast<std::size_t>(static_cast<std::uint32_t>(key.layer));
    }
};

RowKey keyOf(const ElementFlat& flat, std::size_t row) noexcept
{
    return {flat.step.empty() ? 0 : stepKey(flat.step[row]), flat.eid[row], flat.layer[row]};
}

// The rows of one subcase of a result set: the failure flats of the same are matched to a stress flat.
using GroupKey = std::tuple<std::size_t, std::int32_t, std::int32_t>;

GroupKey groupOf(const ElementFlat& flat)
{
    return {flat.set, flat.subcase, flat.designCycle};
}

// What a failure flat gives of a row, found once for the flat.
struct FailureColumns
{
    const std::vector<double>* index = nullptr;
    const std::vector<double>* bond = nullptr; // null when the flat has none
    const std::vector<std::string>* theory = nullptr;
    double provider = -1.0;
};

// The failure row of each row of a stress flat: the flat (in the failure set) and the row, -1 for none. The failure
// rows of a subcase mostly come in the order of the stress rows (the rows of the same table, or some of them): they
// are walked along the stress rows, else looked up by step, element and ply. Where two failure rows have the same
// key, the later one gives the row.
struct RowMatch
{
    std::vector<std::int32_t> flat;
    std::vector<std::int32_t> row;
};

RowMatch matchRows(const ElementFlat& stress, const std::vector<ElementFlat>& failures,
                   const std::vector<std::size_t>& candidates)
{
    RowMatch match;
    if (candidates.empty())
        return match;
    const std::size_t rows = stress.rows();
    match.flat.assign(rows, -1);
    match.row.assign(rows, -1);
    bool walked = true;
    for (const std::size_t index : candidates)
    {
        const ElementFlat& failure = failures[index];
        std::size_t at = 0;
        for (std::size_t row = 0; walked && row < failure.rows(); ++row)
        {
            const RowKey key = keyOf(failure, row);
            while (at < rows && !(keyOf(stress, at) == key))
                ++at;
            if (at == rows)
            {
                walked = false;
                break;
            }
            match.flat[at] = static_cast<std::int32_t>(index);
            match.row[at] = static_cast<std::int32_t>(row);
            ++at;
        }
        if (!walked)
            break;
    }
    if (walked)
        return match;
    std::unordered_map<RowKey, std::pair<std::int32_t, std::int32_t>, RowKeyHash> found;
    for (const std::size_t index : candidates)
        for (std::size_t row = 0; row < failures[index].rows(); ++row)
            found[keyOf(failures[index], row)] = {static_cast<std::int32_t>(index), static_cast<std::int32_t>(row)};
    for (std::size_t row = 0; row < rows; ++row)
    {
        const auto entry = found.find(keyOf(stress, row));
        match.flat[row] = entry != found.end() ? entry->second.first : -1;
        match.row[row] = entry != found.end() ? entry->second.second : -1;
    }
    return match;
}

// The failure index of the stress rows without the flats of the chain (COMPOSITE-API-AUDIT P2): the stored index of
// the same step, element and ply of the failure index table of the subcase (an element the table has a row of takes
// none from the recomputation, as the chain's sink drops its rows), else recomputed in `finish` from the stresses and
// the allowables, as RecomputedFailureIndexProvider computes it. Made only when the stored tables match the stress
// flats one to one, table to table, step to step and row to row (every stored row of a stress row); else nothing, and
// the chain gives the rows.
struct StoredIndexTable
{
    std::shared_ptr<const results::ResultData> decoded;
    int index = -1;
    int bond = -1;
    std::vector<std::string> theory; // per row, as the provider normalises and carries it
    std::vector<std::uint8_t> kept;  // per row: the request keeps it
    std::size_t keptCount = 0;
};

struct DirectFlat
{
    const StoredIndexTable* stored = nullptr;
    std::size_t block = 0;             // the rows of one step
    std::vector<std::int32_t> rows;    // the stored row of each row of a step (-1 none)
    std::vector<std::uint8_t> covered; // the stored table has a row of the element
};

struct DirectFailure
{
    std::vector<StoredIndexTable> tables;
    std::vector<DirectFlat> flats;
    bool recompute = false;
};

std::optional<DirectFailure> directFailure(const ResultQueryContext& context, const ElementRequest& request,
                                           const std::vector<ElementFlat>& stresses)
{
    const QuantityInfo chain = failureIndexChain().info();
    if (stresses.empty() || ProviderSwitches::isDisabled(PLY_FAILURE_SWITCH) ||
        ProviderSwitches::isDisabled(chain.providers[0].name) || ProviderSwitches::isDisabled(chain.providers[1].name))
        return std::nullopt;
    if (!request.theory.empty() && plyTheoryOf(request.theory) == PlyTheory::None)
        return std::nullopt; // the recomputation refuses it, naming the theories
    for (const ElementFlat& flat : stresses)
        if (flat.isComplex())
            return std::nullopt;
    DirectFailure direct;
    direct.recompute = context.hasModel();
    direct.flats.resize(stresses.size());
    using GroupKey = std::tuple<std::size_t, std::int32_t, std::int32_t, std::string>;
    std::map<GroupKey, std::vector<std::size_t>> flatsOf;
    for (std::size_t index = 0; index < stresses.size(); ++index)
        flatsOf[{stresses[index].set, stresses[index].subcase, stresses[index].designCycle,
                 stresses[index].elementName}]
            .push_back(index);
    if (!request.theory.empty())
        return direct; // the stored index is that of the property: every row recomputed
    std::vector<TableRef> refs;
    for (const std::string& name : context.elementNames(results::ResultKind::FailureIndices))
    {
        if (name.size() < 9 || name.compare(name.size() - 9, 9, "Composite") != 0 ||
            !request.wantsType(cardOfContainer(name)))
            continue;
        for (const TableRef& ref : context.elementTables(results::ResultKind::FailureIndices, name, false))
            refs.push_back(ref);
    }
    direct.tables.resize(refs.size());
    for (std::size_t at = 0; at < refs.size(); ++at)
    {
        const TableRef& ref = refs[at];
        const results::ResultTableMeta& meta = ref.table->meta();
        StoredIndexTable& table = direct.tables[at];
        std::shared_ptr<const results::ResultData> decoded = context.decodeRows(ref, request.elementIds);
        if (!decoded)
            continue;
        const TableValues values(*decoded);
        table.index = values.component("failureIndex");
        if (table.index < 0 || values.isComplex())
            continue;
        table.bond = values.component("bondFailureIndex");
        const results::ResultText* theories = nullptr;
        for (const results::ResultText& text : decoded->texts)
            if (text.name == "failureTheory")
                theories = &text;
        table.theory.assign(values.rows(), std::string());
        table.kept.assign(values.rows(), 0);
        std::string current;
        for (std::size_t row = 0; row < values.rows(); ++row)
        {
            const std::int32_t element = decoded->ids[row];
            if (row == 0 || element != decoded->ids[row - 1])
                current.clear();
            if (theories != nullptr && !theories->at(row).empty())
                current = storedTheoryName(theories->at(row));
            table.theory[row] = current;
            if (request.wantsElement(element) && request.wantsLayer(decoded->layers[row]) &&
                (!request.propertyIds || request.wantsProperty(context.elementCard(ref.set, element).pid)))
            {
                table.kept[row] = 1;
                ++table.keptCount;
            }
        }
        if (table.keptCount == 0 || values.steps() == 0)
            continue;
        // the one stress flat of its result set, subcase, design cycle and element type
        const auto found = flatsOf.find({ref.set, meta.subcase, meta.designCycle, meta.elementName});
        if (found == flatsOf.end() || found->second.size() != 1)
            return std::nullopt;
        const ElementFlat& flat = stresses[found->second.front()];
        DirectFlat& entry = direct.flats[found->second.front()];
        if (entry.stored != nullptr)
            return std::nullopt;
        // the rows of one step, the same at each step as the stored table's
        const std::size_t steps = values.steps();
        if (flat.rows() % steps != 0)
            return std::nullopt;
        entry.block = flat.rows() / steps;
        const bool stepped = !flat.step.empty() && !std::isnan(flat.step[0]);
        for (std::size_t step = 0; step < steps; ++step)
        {
            const std::size_t first = step * entry.block;
            if (stepped ? (decoded->steps.size() != steps || stepKey(flat.step[first]) != stepKey(decoded->steps[step]))
                        : !flat.step.empty() && !std::isnan(flat.step[first]))
                return std::nullopt;
            for (std::size_t row = 1; row < entry.block; ++row)
                if (flat.eid[first + row] != flat.eid[row] || flat.layer[first + row] != flat.layer[row] ||
                    (stepped && stepKey(flat.step[first + row]) != stepKey(flat.step[first])))
                    return std::nullopt;
        }
        // the stored row of each row of a step: walked along the two, else looked up
        entry.rows.assign(entry.block, -1);
        std::size_t next = 0;
        bool walked = true;
        for (std::size_t row = 0; walked && row < values.rows(); ++row)
        {
            if (table.kept[row] == 0)
                continue;
            while (next < entry.block &&
                   !(flat.eid[next] == decoded->ids[row] && flat.layer[next] == decoded->layers[row]))
                ++next;
            if (next == entry.block)
                walked = false;
            else
                entry.rows[next++] = static_cast<std::int32_t>(row);
        }
        if (!walked)
        {
            std::fill(entry.rows.begin(), entry.rows.end(), -1);
            std::unordered_map<std::uint64_t, std::int32_t> byKey;
            const auto keyOf = [](std::int32_t eid, std::int32_t layer)
            {
                return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(eid)) << 32U) |
                       static_cast<std::uint32_t>(layer);
            };
            for (std::size_t row = 0; row < values.rows(); ++row)
                if (table.kept[row] != 0)
                    byKey[keyOf(decoded->ids[row], decoded->layers[row])] = static_cast<std::int32_t>(row);
            for (std::size_t row = 0; row < entry.block; ++row)
            {
                const auto at = byKey.find(keyOf(flat.eid[row], flat.layer[row]));
                if (at != byKey.end())
                    entry.rows[row] = at->second;
            }
        }
        // every stored row kept is a stress row's (else the chain adds rows of its own)
        std::vector<std::uint8_t> used(values.rows(), 0);
        std::size_t matched = 0;
        for (const std::int32_t row : entry.rows)
            if (row >= 0 && used[static_cast<std::size_t>(row)] == 0)
            {
                used[static_cast<std::size_t>(row)] = 1;
                ++matched;
            }
        if (matched != table.keptCount)
            return std::nullopt;
        std::vector<std::int32_t> elements;
        for (std::size_t row = 0; row < values.rows(); ++row)
            if (table.kept[row] != 0)
                elements.push_back(decoded->ids[row]);
        std::sort(elements.begin(), elements.end());
        elements.erase(std::unique(elements.begin(), elements.end()), elements.end());
        entry.covered.assign(entry.block, 0);
        for (std::size_t row = 0; row < entry.block; ++row)
            entry.covered[row] = std::binary_search(elements.begin(), elements.end(), flat.eid[row]) ? 1 : 0;
        table.decoded = std::move(decoded);
        entry.stored = &table;
    }
    return direct;
}

PlyData buildPlyDataBy(const ResultQueryContext& context, const ElementRequest& request, bool whole, bool straight)
{
    PlyData data;
    // the stresses of the plies, made on the threads of the call; the recomputed failure index reads them too
    std::vector<ElementFlat> stresses = flattenPlyTables(context, plyTables(context, false, request), false, request);
    ElementRequest failureRequest = request;
    failureRequest.plyStresses = &stresses;
    // the failure index: stored, else recomputed; without the flats of the chain when the stored tables allow it
    const std::optional<DirectFailure> direct =
        straight ? directFailure(context, request, stresses) : std::optional<DirectFailure>();
    ElementFlatSet failure;
    if (direct)
    {
        const QuantityInfo chain = failureIndexChain().info();
        data.providers = chain.providers;
    }
    else
    {
        data.attempts = failureIndexChain().resolve(context, failureRequest, failure);
        data.providers = failure.providers();
    }
    const std::vector<ElementFlat>& failures = failure.flats();
    std::map<GroupKey, std::vector<std::size_t>> failureGroups;
    std::vector<FailureColumns> columns(failures.size());
    for (std::size_t index = 0; index < failures.size(); ++index)
    {
        const ElementFlat& source = failures[index];
        failureGroups[groupOf(source)].push_back(index);
        columns[index].index = &source.values.at("failureIndex");
        if (const auto stored = source.values.find("bondIndex"); stored != source.values.end())
            columns[index].bond = &stored->second;
        columns[index].theory = &source.texts.at("failureTheory");
        columns[index].provider = source.provider;
    }
    LaminateLookup laminates(context);
    for (std::size_t index = 0; index < stresses.size(); ++index)
        laminates.prepare(stresses[index], index > 0 ? &stresses[index - 1] : nullptr);

    // The failure rows of each stress row, and the columns the model and the theories give, a flat at a time on the
    // threads of the call.
    std::vector<RowMatch> matches(stresses.size());
    const auto finish = [&](std::size_t flatIndex)
    {
        ElementFlat& flat = stresses[flatIndex];
        const auto group = failureGroups.find(groupOf(flat));
        if (group != failureGroups.end())
            matches[flatIndex] = matchRows(flat, failures, group->second);
        const DirectFlat* straightFlat = direct ? &direct->flats[flatIndex] : nullptr;
        const std::string& asked = request.theory;
        const RowMatch& match = matches[flatIndex];
        const std::size_t rows = flat.rows();
        std::vector<double> index(rows, NOT_A_NUMBER);
        std::vector<double> ratio(rows, NOT_A_NUMBER);
        std::vector<double> plyId(rows, NOT_A_NUMBER);
        const std::size_t wholeRows = whole ? rows : 0; // the fields a block does not read
        std::vector<double> material(wholeRows, NOT_A_NUMBER);
        std::vector<double> theta(wholeRows, NOT_A_NUMBER);
        std::vector<double> thickness(wholeRows, NOT_A_NUMBER);
        std::vector<double> provider(rows, -1.0);
        std::vector<double> quadratic(wholeRows, NOT_A_NUMBER);
        std::vector<double> linear(wholeRows, NOT_A_NUMBER);
        std::vector<double> bond(rows, NOT_A_NUMBER);
        std::vector<double> fiber(rows, NOT_A_NUMBER);
        std::vector<double> matrix(rows, NOT_A_NUMBER);
        std::vector<std::string> mode(wholeRows);
        std::vector<std::string> theory(wholeRows);
        const auto column = [&](const char* name) -> const std::vector<double>*
        {
            const auto found = flat.values.find(name);
            return found != flat.values.end() ? &found->second : nullptr;
        };
        const std::vector<double>* t1z = column("t1z");
        const std::vector<double>* t2z = column("t2z");
        const std::vector<double>* o11 = column("o11");
        const std::vector<double>* o22 = column("o22");
        const std::vector<double>* t12 = column("t12");
        static const LaminateFacts NONE;
        static const std::string NO_THEORY;
        const LaminateFacts* laminate = &NONE;
        const std::string* lastTheory = nullptr;
        PlyTheory parsed = PlyTheory::None;
        PlyTheory recomputedTheory = PlyTheory::None;
        for (std::size_t row = 0; row < rows; ++row)
        {
            if (row == 0 || flat.eid[row] != flat.eid[row - 1])
            {
                const LaminateFacts* found = laminates.found(flat.set, flat.eid[row]);
                laminate = found != nullptr ? found : &NONE;
                recomputedTheory = plyTheoryOf(asked.empty() ? laminate->theory : asked);
            }
            const std::string* rowTheory = &NO_THEORY;
            if (straightFlat != nullptr)
            {
                const std::size_t block = straightFlat->block;
                if (straightFlat->stored != nullptr && straightFlat->covered[row % block] != 0)
                {
                    // the stored index of the row (an element the table covers takes none from the recomputation)
                    const std::int32_t at = straightFlat->rows[row % block];
                    if (at >= 0)
                    {
                        const StoredIndexTable& table = *straightFlat->stored;
                        const TableValues values(*table.decoded);
                        const auto storedRow = static_cast<std::size_t>(at);
                        index[row] = values.real(row / block, storedRow, static_cast<std::size_t>(table.index));
                        if (table.bond >= 0)
                            bond[row] = values.real(row / block, storedRow, static_cast<std::size_t>(table.bond));
                        rowTheory = &table.theory[storedRow];
                        provider[row] = 0.0;
                        if (whole)
                            theory[row] = *rowTheory;
                    }
                }
                else if (direct->recompute && o11 != nullptr)
                {
                    // recomputed as RecomputedFailureIndexProvider does, by the theory asked or the property's
                    const std::string& recomputed = asked.empty() ? laminate->theory : asked;
                    const auto ply = static_cast<std::size_t>(flat.layer[row] - 1);
                    if (!recomputed.empty() && ply < laminate->plies.size() && laminate->plies[ply].allowables)
                    {
                        const PlyIndex value = plyFailureIndex(recomputedTheory, (*o11)[row], (*o22)[row], (*t12)[row],
                                                               *laminate->plies[ply].allowables);
                        if (!std::isnan(value.index))
                        {
                            index[row] = value.index;
                            rowTheory = &recomputed;
                            provider[row] = 1.0;
                            if (whole)
                                theory[row] = recomputed;
                        }
                    }
                }
            }
            else if (!match.flat.empty() && match.flat[row] >= 0)
            {
                const FailureColumns& source = columns[static_cast<std::size_t>(match.flat[row])];
                const auto at = static_cast<std::size_t>(match.row[row]);
                index[row] = (*source.index)[at];
                if (source.bond != nullptr)
                    bond[row] = (*source.bond)[at];
                rowTheory = &(*source.theory)[at];
                provider[row] = source.provider;
                if (whole)
                    theory[row] = *rowTheory;
            }
            // the bonding index: stored by the solver with the SB of the property, else from the interlaminar shears
            if (std::isnan(bond[row]) && t1z != nullptr)
                bond[row] = bondIndex((*t1z)[row], (*t2z)[row], laminate->bondAllowable, false);
            const auto ply = static_cast<std::size_t>(flat.layer[row] - 1);
            if (ply >= laminate->plies.size())
                continue;
            const PlyFacts& facts = laminate->plies[ply];
            plyId[row] = facts.globalId > 0 ? facts.globalId : NOT_A_NUMBER;
            if (whole)
            {
                material[row] = facts.material;
                theta[row] = facts.angle;
                thickness[row] = facts.thickness;
            }
            const std::string& usedTheory = !rowTheory->empty() ? *rowTheory : laminate->theory;
            if (!facts.allowables || usedTheory.empty() || o11 == nullptr)
                continue;
            // the theory of the row before, mostly
            if (lastTheory == nullptr || !(usedTheory == *lastTheory))
            {
                lastTheory = &usedTheory;
                parsed = plyTheoryOf(usedTheory);
            }
            const double s1 = (*o11)[row];
            const double s2 = (*o22)[row];
            const double s12 = (*t12)[row];
            // the modes of the theories that have them, which one governs, and their strength ratio; else the index of
            // the theory, a quadratic of the stresses, and its root
            if (parsed == PlyTheory::Hashin || parsed == PlyTheory::Puck || parsed == PlyTheory::Larc03)
            {
                PlyModes modes = plyFailureModes(usedTheory, s1, s2, s12, *facts.allowables);
                if (!std::isnan(modes.index))
                    ratio[row] = modes.ratio;
                fiber[row] = modes.fiber;
                matrix[row] = modes.matrix;
                if (whole)
                    mode[row] = std::move(modes.mode);
                continue;
            }
            const PlyIndex value = plyFailureIndex(parsed, s1, s2, s12, *facts.allowables);
            if (!std::isnan(value.index))
            {
                ratio[row] = scaleToLimit(value, 1.0);
                if (whole)
                {
                    quadratic[row] = value.quadratic;
                    linear[row] = value.linear;
                }
            }
        }
        flat.values["failureIndex"] = std::move(index);
        flat.values["strengthRatio"] = std::move(ratio);
        flat.values["plyId"] = std::move(plyId);
        flat.values["failureProvider"] = std::move(provider);
        flat.values["bondIndex"] = std::move(bond);
        flat.values["fiberIndex"] = std::move(fiber);
        flat.values["matrixIndex"] = std::move(matrix);
        if (whole)
        {
            flat.values["matId"] = std::move(material);
            flat.values["theta"] = std::move(theta);
            flat.values["thickness"] = std::move(thickness);
            flat.values["indexQuadratic"] = std::move(quadratic);
            flat.values["indexLinear"] = std::move(linear);
            flat.texts["failureMode"] = std::move(mode);
            flat.texts["failureTheory"] = std::move(theory);
        }
    };
    const results::DecodeContext decode = context.decode();
    if (decode.pool != nullptr && stresses.size() > 1)
        base::parallelFor(*decode.pool, stresses.size(), finish, decode.token);
    else
        for (std::size_t index = 0; index < stresses.size(); ++index)
            finish(index);

    if (direct)
    {
        // the rows each way gave, as the chain counts them; none at all: the chain says why
        std::int64_t stored = 0;
        std::int64_t recomputed = 0;
        for (const ElementFlat& flat : stresses)
            for (const double way : flat.values.at("failureProvider"))
            {
                stored += way == 0.0 ? 1 : 0;
                recomputed += way == 1.0 ? 1 : 0;
            }
        if (stored + recomputed == 0)
            return buildPlyDataBy(context, request, whole, false);
        for (std::size_t way = 0; way < data.providers.size(); ++way)
            data.attempts.push_back(ProviderAttempt{data.providers[way].name, data.providers[way].approximate,
                                                    way == 0 ? stored : recomputed, std::string()});
        data.flats = std::move(stresses);
        return data;
    }
    // the failure rows a stress row took
    std::vector<std::vector<std::uint8_t>> used(failures.size());
    for (std::size_t index = 0; index < failures.size(); ++index)
        used[index].assign(failures[index].rows(), 0);
    for (const RowMatch& match : matches)
        for (std::size_t row = 0; row < match.flat.size(); ++row)
            if (match.flat[row] >= 0)
                used[static_cast<std::size_t>(match.flat[row])][static_cast<std::size_t>(match.row[row])] = 1;
    data.flats = std::move(stresses);

    // the failure indices of plies whose stresses are not in the file: rows with no stress
    for (std::size_t index = 0; index < failures.size(); ++index)
    {
        const ElementFlat& source = failures[index];
        std::vector<std::size_t> keep;
        for (std::size_t row = 0; row < source.rows(); ++row)
            if (used[index][row] == 0)
                keep.push_back(row);
        if (keep.empty())
            continue;
        ElementFlat flat = source.rowLabels(keep);
        const std::size_t rows = flat.rows();
        for (const std::string& field : plyStressFields())
            flat.values[field] = std::vector<double>(rows, NOT_A_NUMBER);
        // the failure index of the kept rows, with the provider that gave it
        std::vector<double> value;
        std::vector<double> bond;
        std::vector<std::string> theory;
        const FailureColumns& from = columns[index];
        for (const std::size_t row : keep)
        {
            value.push_back((*from.index)[row]);
            bond.push_back(from.bond != nullptr ? (*from.bond)[row] : NOT_A_NUMBER);
            if (whole)
                theory.push_back((*from.theory)[row]);
        }
        flat.values["failureIndex"] = std::move(value);
        flat.values["strengthRatio"] = std::vector<double>(rows, NOT_A_NUMBER);
        flat.values["plyId"] = std::vector<double>(rows, NOT_A_NUMBER);
        flat.values["failureProvider"] = std::vector<double>(rows, static_cast<double>(source.provider));
        flat.values["bondIndex"] = std::move(bond);
        flat.values["fiberIndex"] = std::vector<double>(rows, NOT_A_NUMBER);
        flat.values["matrixIndex"] = std::vector<double>(rows, NOT_A_NUMBER);
        if (whole)
        {
            for (const char* field : {"matId", "theta", "thickness", "indexQuadratic", "indexLinear"})
                flat.values[field] = std::vector<double>(rows, NOT_A_NUMBER);
            flat.texts["failureMode"] = std::vector<std::string>(rows);
            flat.texts["failureTheory"] = std::move(theory);
        }
        data.flats.push_back(std::move(flat));
    }
    return data;
}

// A text column of a block by codes (the texts of a column of the plies are a few, in runs).
void codedTexts(RowBlock& block, const char* name, const std::vector<std::string>& texts)
{
    std::vector<std::string> dictionary;
    std::vector<std::int32_t> codes(texts.size());
    std::int32_t last = -1;
    for (std::size_t row = 0; row < texts.size(); ++row)
    {
        if (last < 0 || dictionary[static_cast<std::size_t>(last)] != texts[row])
        {
            const auto found = std::find(dictionary.begin(), dictionary.end(), texts[row]);
            last = static_cast<std::int32_t>(found - dictionary.begin());
            if (found == dictionary.end())
                dictionary.push_back(texts[row]);
        }
        codes[row] = last;
    }
    block.codedTexts(name, std::move(dictionary), std::move(codes));
}

} // namespace

PlyData buildPlyData(const ResultQueryContext& context, const ElementRequest& request, bool whole)
{
    return buildPlyDataBy(context, request, whole, true);
}

RowBlock plyBlock(const ResultQueryContext& context, ElementFlat&& flat, const std::vector<std::int16_t>& providers)
{
    const std::size_t rows = flat.rows();
    RowBlock block(rows);
    block.ints("subcase", std::vector<std::int32_t>(rows, flat.subcase));
    block.ints("elementId", flat.eid);
    std::vector<std::int32_t> pid(rows, 0);
    std::vector<std::uint8_t> pidValid(rows, 0);
    std::int32_t found = 0;
    for (std::size_t row = 0; row < rows; ++row)
    {
        // the property of an element once a run of its plies
        if (row == 0 || flat.eid[row] != flat.eid[row - 1])
            found = context.elementCard(flat.set, flat.eid[row]).pid;
        if (found > 0)
        {
            pid[row] = found;
            pidValid[row] = 1;
        }
    }
    block.ints("propId", std::move(pid), std::move(pidValid));
    block.ints("ply", flat.layer);
    const auto integerColumn = [&](const char* name, const char* field)
    {
        std::vector<std::int32_t> numbers(rows, 0);
        std::vector<std::uint8_t> valid(rows, 0);
        const std::vector<double>& source = flat.values.at(field);
        for (std::size_t row = 0; row < rows; ++row)
            if (!std::isnan(source[row]))
            {
                numbers[row] = static_cast<std::int32_t>(source[row]);
                valid[row] = 1;
            }
        block.ints(name, std::move(numbers), std::move(valid));
    };
    integerColumn("plyId", "plyId");
    integerColumn("matId", "matId");
    block.reals("theta", std::move(flat.values.at("theta")));
    block.reals("thickness", std::move(flat.values.at("thickness")));
    for (const char* field : {"o11", "o22", "t12", "t1z", "t2z"})
    {
        const auto imaginary = flat.imaginary.find(field);
        if (imaginary != flat.imaginary.end())
            block.complex(field, flat.values.at(field), imaginary->second);
        else
            block.reals(field, std::move(flat.values.at(field)));
    }
    block.reals("omax", std::move(flat.values.at("omax")));
    block.reals("omin", std::move(flat.values.at("omin")));
    block.reals("maxShear", std::move(flat.values.at("maxShear")));
    codedTexts(block, "failureTheory", flat.texts.at("failureTheory"));
    block.reals("failureIndex", std::move(flat.values.at("failureIndex")));
    block.reals("strengthRatio", std::move(flat.values.at("strengthRatio")));
    block.reals("bondIndex", std::move(flat.values.at("bondIndex")));
    block.reals("fiberIndex", std::move(flat.values.at("fiberIndex")));
    block.reals("matrixIndex", std::move(flat.values.at("matrixIndex")));
    codedTexts(block, "failureMode", flat.texts.at("failureMode"));
    if (flat.stepKind != results::StepKind::None)
        block.steps(flat.stepKind, flat.step, flat.eigenvalue);
    block.origin(flat.set, flat.analysis, flat.designCycle);
    block.precision(flat.precision);
    block.storedAsMagnitudePhase(flat.magnitudePhase);
    block.validation(flat.status);
    // the provider of the failure index of each row (the stress itself is always a stored table); a row with none has
    // none
    std::vector<std::int16_t> perRow(rows, std::int16_t(-1));
    const std::vector<double>& failureProvider = flat.values.at("failureProvider");
    for (std::size_t row = 0; row < rows; ++row)
        if (failureProvider[row] >= 0.0)
            perRow[row] = providers[static_cast<std::size_t>(failureProvider[row])];
    block.providers(std::move(perRow));
    return block;
}

} // namespace oor::query
