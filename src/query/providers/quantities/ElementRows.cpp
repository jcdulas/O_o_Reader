#include "oor/query/providers/quantities/ElementRows.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/providers/quantities/ElementQuantities.hpp"
#include "oor/query/results/RowAccumulator.hpp"
#include "oor/results/core/ResultCatalogue.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <limits>
#include <tuple>

namespace oor::query
{

std::uint64_t stepKey(double step) noexcept
{
    // a step is the same step to the precision of a 32-bit file: compare the single precision value
    const float narrowed = static_cast<float>(step);
    std::uint32_t bits = 0;
    std::memcpy(&bits, &narrowed, sizeof bits);
    return bits;
}

std::string cardOfContainer(const std::string& elementName)
{
    // A container is named after its card type ("ShellQuad4Composite" holds ShellQuad4 rows); the rows and the argument
    // `types` give the card by its type name ("ShellQuad4"), like the other queries.
    std::string type(results::elementCardOf(elementName));
    if (type.empty())
    {
        type = elementName;
    }
    return type;
}

std::string containerOfCard(const std::string& card)
{
    return card;
}

void ElementFlat::keepRows(const std::vector<std::size_t>& keep)
{
    const auto gather = [&](auto& vector)
    {
        if (vector.empty())
            return;
        using Item = typename std::decay_t<decltype(vector)>::value_type;
        std::vector<Item> kept;
        kept.reserve(keep.size());
        for (const std::size_t row : keep)
            kept.push_back(std::move(vector[row]));
        vector = std::move(kept);
    };
    gather(eid);
    gather(node);
    gather(layer);
    gather(step);
    gather(eigenvalue);
    gather(position);
    gather(location);
    for (auto& entry : values)
        gather(entry.second);
    for (auto& entry : imaginary)
        gather(entry.second);
    for (auto& entry : texts)
        gather(entry.second);
}

ElementFlat ElementFlat::rowLabels(const std::vector<std::size_t>& keep) const
{
    ElementFlat flat;
    flat.set = set;
    flat.analysis = analysis;
    flat.subcase = subcase;
    flat.designCycle = designCycle;
    flat.stepKind = stepKind;
    flat.elementName = elementName;
    flat.card = card;
    flat.frame = frame;
    flat.status = status;
    flat.precision = precision;
    flat.magnitudePhase = magnitudePhase;
    flat.provider = provider;
    const auto gather = [&](const auto& from, auto& to)
    {
        if (from.empty())
            return;
        to.reserve(keep.size());
        for (const std::size_t row : keep)
            to.push_back(from[row]);
    };
    gather(eid, flat.eid);
    gather(node, flat.node);
    gather(layer, flat.layer);
    gather(step, flat.step);
    gather(eigenvalue, flat.eigenvalue);
    gather(position, flat.position);
    gather(location, flat.location);
    return flat;
}

void ElementFlatSet::beginProvider(std::int16_t index, const ProviderInfo& info)
{
    m_current = index;
    if (m_providers.size() <= static_cast<std::size_t>(index))
        m_providers.resize(static_cast<std::size_t>(index) + 1);
    m_providers[static_cast<std::size_t>(index)] = info;
}

std::int64_t ElementFlatSet::add(ElementFlat&& flat)
{
    if (flat.rows() == 0)
        return 0;
    flat.provider = m_current;
    const GroupKey key{flat.set, flat.subcase, flat.designCycle, flat.elementName};
    Group& group = m_groups[key];
    // the rows an earlier provider gave for this element type of this subcase
    std::unordered_set<RowKey, RowKeyHash> earlier;
    for (const std::size_t index : group.flats)
    {
        const ElementFlat& other = m_flats[index];
        if (other.provider == m_current)
            continue;
        for (std::size_t row = 0; row < other.rows(); ++row)
            earlier.insert(RowKey{other.eid[row], other.step.empty() ? 0 : stepKey(other.step[row])});
    }
    if (!earlier.empty())
    {
        std::vector<std::size_t> keep;
        keep.reserve(flat.rows());
        for (std::size_t row = 0; row < flat.rows(); ++row)
            if (!earlier.contains(RowKey{flat.eid[row], flat.step.empty() ? 0 : stepKey(flat.step[row])}))
                keep.push_back(row);
        if (keep.empty())
            return 0;
        if (keep.size() != flat.rows())
            flat.keepRows(keep);
    }
    const std::size_t index = m_flats.size();
    const std::int64_t rows = static_cast<std::int64_t>(flat.rows());
    group.flats.push_back(index);
    m_flats.push_back(std::move(flat));
    m_rows += rows;
    return rows;
}

bool ElementFlatSet::hasGroup(std::size_t set, std::int32_t subcase, std::int32_t designCycle,
                              const std::string& elementName) const
{
    const auto found = m_groups.find(GroupKey{set, subcase, designCycle, elementName});
    return found != m_groups.end() && !found->second.flats.empty();
}

bool ElementFlatSet::coversStatic(std::size_t set, std::int32_t subcase, std::int32_t designCycle,
                                  const std::string& elementName, std::span<const std::int32_t> eids) const
{
    if (eids.empty())
        return true;
    const auto found = m_groups.find(GroupKey{set, subcase, designCycle, elementName});
    if (found == m_groups.end())
        return false;
    // the key `add` gives a row of a static table (its step NaN)
    const std::uint64_t noStep = stepKey(std::numeric_limits<double>::quiet_NaN());
    std::vector<std::int32_t> given;
    for (const std::size_t index : found->second.flats)
    {
        const ElementFlat& other = m_flats[index];
        if (other.provider == m_current)
            continue;
        for (std::size_t row = 0; row < other.rows(); ++row)
            if ((other.step.empty() ? 0 : stepKey(other.step[row])) == noStep)
                given.push_back(other.eid[row]);
    }
    // the rows of a table come by increasing element mostly: sorted already, they are not sorted again
    if (!std::is_sorted(given.begin(), given.end()))
        std::sort(given.begin(), given.end());
    if (std::is_sorted(eids.begin(), eids.end()))
        return std::includes(given.begin(), given.end(), eids.begin(), eids.end());
    return std::all_of(eids.begin(), eids.end(),
                       [&](std::int32_t eid) { return std::binary_search(given.begin(), given.end(), eid); });
}

bool ElementFlatSet::covers(std::size_t set, std::int32_t subcase, std::int32_t designCycle,
                            const std::string& elementName, std::int32_t eid) const
{
    const auto found = m_groups.find(GroupKey{set, subcase, designCycle, elementName});
    if (found == m_groups.end())
        return false;
    for (const std::size_t index : found->second.flats)
        if (std::find(m_flats[index].eid.begin(), m_flats[index].eid.end(), eid) != m_flats[index].eid.end())
            return true;
    return false;
}

namespace
{

std::vector<std::int64_t> sortedOnce(std::vector<std::int64_t> ids)
{
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

} // namespace

ElementRequest ElementRequest::from(const QueryArguments& arguments)
{
    ElementRequest request;
    request.elementIds = arguments.ids("elementIds");
    // Type selectors use the exact, case-sensitive card names stored in the rows.
    for (const std::string& name : arguments.texts("types"))
    {
        request.types.push_back(name);
    }
    request.propertyIds = arguments.ids("propertyIds");
    request.location = arguments.textOr("location", "");
    request.layers = arguments.ids("layers");
    if (!request.layers)
        request.layers = arguments.ids("plies"); // the plies of a laminate are its layers
    if (arguments.isText("axis"))
        request.axisName = arguments.textOr("axis", "");
    else if (arguments.has("axis"))
    {
        const std::vector<double> vector = arguments.reals("axis");
        if (vector.size() != 3)
            throw QueryError("axis is \"x\", \"y\", \"z\" or a vector of three numbers");
        request.axisVector = std::array<double, 3>{vector[0], vector[1], vector[2]};
    }
    request.theory = arguments.textOr("theory", "");
    if (request.elementIds)
        request.m_elements = sortedOnce(*request.elementIds);
    if (request.propertyIds)
        request.m_properties = sortedOnce(*request.propertyIds);
    if (request.layers)
        request.m_layers = sortedOnce(*request.layers);
    return request;
}

bool ElementRequest::wantsElement(std::int32_t eid) const
{
    return !elementIds || std::binary_search(m_elements.begin(), m_elements.end(), eid);
}

bool ElementRequest::wantsType(const std::string& card) const
{
    return types.empty() || std::find(types.begin(), types.end(), card) != types.end();
}

bool ElementRequest::wantsLayer(std::int32_t layer) const
{
    return !layers || std::binary_search(m_layers.begin(), m_layers.end(), layer);
}

bool ElementRequest::wantsProperty(std::int32_t pid) const
{
    return !propertyIds || std::binary_search(m_properties.begin(), m_properties.end(), pid);
}

std::vector<std::int32_t> missingElements(const ElementRequest& request, const ElementFlatSet& set)
{
    std::vector<std::int32_t> missing;
    if (!request.elementIds)
        return missing;
    // the elements asked for, sorted, each marked when a row has it (the rows of an element come together: a row of
    // the element of the row before is not looked up again)
    std::vector<std::int32_t> wanted(request.elementIds->begin(), request.elementIds->end());
    std::sort(wanted.begin(), wanted.end());
    wanted.erase(std::unique(wanted.begin(), wanted.end()), wanted.end());
    std::vector<std::uint8_t> present(wanted.size(), 0);
    // the flats of the cases mostly have every element: the search stops once each has a row; a flat with the elements
    // of the one looked at before adds none (the subcases of one element type: a companion package)
    std::size_t found = 0;
    const ElementFlat* before = nullptr;
    for (const ElementFlat& flat : set.flats())
    {
        if (found == wanted.size())
            break;
        if (before != nullptr && before->eid == flat.eid)
            continue;
        before = &flat;
        for (std::size_t row = 0; row < flat.rows(); ++row)
        {
            if (row > 0 && flat.eid[row] == flat.eid[row - 1])
                continue;
            const auto at = std::lower_bound(wanted.begin(), wanted.end(), flat.eid[row]);
            if (at != wanted.end() && *at == flat.eid[row] &&
                present[static_cast<std::size_t>(at - wanted.begin())] == 0)
            {
                present[static_cast<std::size_t>(at - wanted.begin())] = 1;
                ++found;
            }
        }
    }
    for (std::size_t index = 0; index < wanted.size(); ++index)
        if (present[index] == 0)
            missing.push_back(wanted[index]);
    return missing;
}

namespace
{

std::string listIds(const std::vector<std::int32_t>& ids)
{
    std::string text;
    std::size_t shown = 0;
    for (const std::int32_t id : ids)
    {
        if (shown++ == 8)
        {
            text += ", ... (" + std::to_string(ids.size()) + " in all)";
            break;
        }
        text += (text.empty() ? "" : ", ") + std::to_string(id);
    }
    return text;
}

} // namespace

void noteInputs(RowAccumulator& accumulator, const ElementFlatSet& set, const std::vector<std::string>& columns)
{
    std::map<std::int16_t, std::int64_t> rows;
    for (const ElementFlat& flat : set.flats())
        if (flat.provider >= 0 && !flat.isComplex())
            rows[flat.provider] += static_cast<std::int64_t>(flat.rows());
    for (const auto& [provider, count] : rows)
    {
        const ProviderInfo& info = set.providers()[static_cast<std::size_t>(provider)];
        accumulator.note(accumulator.addProvider(info.name, info.approximate), columns, count);
    }
}

ElementResolution resolveElements(const ResultQueryContext& context, const ElementChain& chain,
                                  const ElementRequest& request, const std::string& what,
                                  const std::vector<std::string>& fields)
{
    ElementResolution resolution;
    resolution.attempts = chain.resolve(context, request, resolution.set);
    resolution.missing = missingElements(request, resolution.set);
    if (resolution.set.rowCount() == 0)
    {
        if (context.selection().onMissing == OnMissing::Raise)
            throw QuantityUnavailableError(describeAttempts(what, resolution.attempts));
        return resolution;
    }
    if (resolution.missing.empty())
        return resolution;
    if (context.selection().onMissing == OnMissing::Raise)
        throw QuantityUnavailableError(describeAttempts(what, resolution.attempts) + "\nno value for the elements " +
                                       listIds(resolution.missing) +
                                       "; onMissing=\"null\" keeps them as rows of nulls, \"skip\" leaves them out");
    if (context.selection().onMissing != OnMissing::Null)
        return resolution;
    // a row of nulls for each element asked for that has no value, at each step the other rows have
    constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();
    using Key = std::tuple<std::size_t, std::int32_t, std::int32_t, results::StepKind, double>;
    std::set<Key> seen;
    std::map<Key, const ElementFlat*> sample;
    for (const ElementFlat& flat : resolution.set.flats())
        for (std::size_t row = 0; row < flat.rows(); ++row)
        {
            const Key key{flat.set, flat.subcase, flat.designCycle, flat.stepKind,
                          flat.step.empty() ? NOT_A_NUMBER : flat.step[row]};
            if (seen.insert(key).second)
                sample[key] = &flat;
        }
    std::vector<ElementFlat> nulls;
    for (const Key& key : seen)
    {
        const ElementFlat& like = *sample.at(key);
        ElementFlat flat;
        const std::size_t count = resolution.missing.size();
        for (const std::string& field : fields)
            flat.values[field] = std::vector<double>(count, NOT_A_NUMBER);
        flat.set = like.set;
        flat.analysis = like.analysis;
        flat.subcase = like.subcase;
        flat.designCycle = like.designCycle;
        flat.stepKind = like.stepKind;
        flat.frame = like.frame;
        flat.precision = like.precision;
        flat.provider = -1;
        flat.eid = resolution.missing;
        flat.node.assign(count, 0);
        flat.layer.assign(count, 0);
        flat.step.assign(count, std::get<4>(key));
        flat.position.assign(count, NOT_A_NUMBER);
        flat.location.assign(count, std::string());
        nulls.push_back(std::move(flat));
    }
    for (ElementFlat& flat : nulls)
        resolution.set.flats().push_back(std::move(flat));
    return resolution;
}

} // namespace oor::query
