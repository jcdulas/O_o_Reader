#include "StressData.hpp"

#include "oor/base/threading/ParallelFor.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/results/ResultModelIndex.hpp"
#include "oor/query/results/ResultTableSource.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

} // namespace

StressData resolveStress(const ResultQueryContext& context, const ElementRequest& request, bool strain)
{
    StressData data;
    data.strain = strain;
    data.request = request;
    ElementResolution resolution = resolveElements(context, strain ? strainChain() : stressChain(), request,
                                                   strain ? "the strain components" : "the stress components",
                                                   strain ? strainFields() : stressFields());
    data.set = std::move(resolution.set);
    data.attempts = std::move(resolution.attempts);
    data.missing = std::move(resolution.missing);
    // each flat completed on a thread of the call (a flat is its own: the same numbers on any number of threads)
    std::vector<ElementFlat>& flats = data.set.flats();
    data.completions.resize(flats.size());
    const auto complete = [&](std::size_t index) { data.completions[index] = completeScalars(flats[index], strain); };
    const results::DecodeContext decode = context.decode();
    if (decode.pool != nullptr && flats.size() > 1)
        base::parallelFor(*decode.pool, flats.size(), complete, decode.token);
    else
        for (std::size_t index = 0; index < flats.size(); ++index)
            complete(index);
    return data;
}

bool isStressQuantity(const std::string& quantity, bool strain)
{
    const std::vector<std::string>& names = strain ? strainFields() : stressFields();
    return std::find(names.begin(), names.end(), quantity) != names.end() || quantity == "maxPrincipal" ||
           quantity == "minPrincipal";
}

const std::vector<double>& quantityOf(const ElementFlat& flat, const std::string& quantity)
{
    static const std::map<std::string, std::pair<std::string, std::string>> PRINCIPAL = {
        {"o1", {"o1", "e1"}},
        {"o2", {"o2", "e2"}},
        {"o3", {"o3", "e3"}},
        {"maxPrincipal", {"o1", "e1"}},
        {"minPrincipal", {"o2", "e2"}}};
    std::string name = quantity;
    if (const auto found = PRINCIPAL.find(quantity); found != PRINCIPAL.end())
        name = flat.values.contains(found->second.first) ? found->second.first : found->second.second;
    // The smallest principal value of a solid is its third one (o2 is the middle one there); a flat holds one element
    // type, and only the solids fill o3: a shell has two principal values, a bar its largest and smallest combined
    // stresses in o1 and o2 (as the margins choose, MarginData).
    if (quantity == "minPrincipal")
    {
        const std::string third = name == "o2" ? "o3" : "e3";
        const auto field = flat.values.find(third);
        if (field != flat.values.end() &&
            std::any_of(field->second.begin(), field->second.end(), [](double value) { return !std::isnan(value); }))
            name = third;
    }
    const auto field = flat.values.find(name);
    if (field == flat.values.end())
        throw QueryError("'" + quantity + "' is not a stress or strain quantity of these results");
    return field->second;
}

RowBlock stressBlock(const ResultQueryContext& context, const ElementFlat& flat, const ScalarCompletion& completion,
                     std::int16_t provider, bool strain)
{
    const std::size_t rows = flat.rows();
    RowBlock block(rows);
    block.ints("subcase", std::vector<std::int32_t>(rows, flat.subcase));
    block.ints("elementId", flat.eid);
    std::vector<std::int32_t> pid(rows, 0);
    std::vector<std::uint8_t> pidValid(rows, 0);
    for (std::size_t row = 0; row < rows; ++row)
    {
        const std::int32_t found = context.elementCard(flat.set, flat.eid[row]).pid;
        if (found > 0)
        {
            pid[row] = found;
            pidValid[row] = 1;
        }
    }
    block.texts("type", std::vector<std::string>(rows, flat.card),
                std::vector<std::uint8_t>(rows, flat.card.empty() ? 0 : 1));
    block.ints("propId", std::move(pid), std::move(pidValid));
    block.texts("location", flat.location);
    std::vector<std::uint8_t> nodeValid(rows);
    std::vector<std::uint8_t> layerValid(rows);
    for (std::size_t row = 0; row < rows; ++row)
    {
        nodeValid[row] = flat.node[row] > 0 ? 1 : 0;
        layerValid[row] = flat.layer[row] > 0 ? 1 : 0;
    }
    block.ints("nodeId", flat.node, std::move(nodeValid));
    block.ints("layer", flat.layer, std::move(layerValid));
    const std::vector<std::string>& fields = strain ? strainFields() : stressFields();
    for (const std::string& field : fields)
    {
        const auto real = flat.values.find(field);
        if (real == flat.values.end())
            continue;
        const auto imaginary = flat.imaginary.find(field);
        if (imaginary != flat.imaginary.end())
            block.complex(field, real->second, imaginary->second);
        else
            block.reals(field, real->second);
    }
    block.texts("frame", std::vector<std::string>(rows, flat.frame));
    if (flat.stepKind != results::StepKind::None)
        block.steps(flat.stepKind, flat.step, flat.eigenvalue);
    block.origin(flat.set, flat.analysis, flat.designCycle);
    block.precision(flat.precision);
    block.storedAsMagnitudePhase(flat.magnitudePhase);
    block.validation(flat.status);
    if (provider >= 0)
        block.provider(provider);
    if (std::any_of(completion.computed.begin(), completion.computed.end(),
                    [](std::uint8_t bits) { return bits != 0; }))
    {
        std::vector<std::string> notes(rows);
        for (std::size_t row = 0; row < rows; ++row)
            notes[row] = completion.note(row);
        block.sourceSuffix(std::move(notes));
    }
    return block;
}

QueryInfo stressTableInfo(bool strain)
{
    QueryInfo info;
    info.name = strain ? "strainTable" : "stressTable";
    info.family = "R2";
    info.resultQuery = true;
    info.envelopeValue = "vonMises";
    const std::string quantity = strain ? "strain" : "stress";
    info.description =
        std::string("The ") + quantity +
        "s of the elements, one row for each subcase, step, element, place in the element (the centre or a corner) and "
        "layer. With `layout=\"unified\"` every element type shares the same columns: a shell fills the three in-plane "
        "components and its major and minor principal values, a solid the six components and the three principal "
        "values, "
        "a rod `axial` and `torsion`, a bar or beam `axial` and, in `o1` and `o2`, the largest and the smallest "
        "combined " +
        quantity +
        " at each end; `vonMises` and `maxShear` come from the table when it stores them, else they are computed "
        "from "
        "the components (`table.sources` and `withSource` say which). A " +
        quantity + " that the result file does not hold is recomputed when a way exists (see the providers of the " +
        quantity +
        " components: the other table, the linear material, the element forces and the property); each row says which "
        "way "
        "gave it. With `layout=\"native\"` the table has the columns of the result file for the one element type "
        "`types` "
        "names. Complex results give each value as `_re`, `_im` or `_mag`, `_phase`.";
    info.parameters = {
        parameter("elementIds", ParameterKind::IdList, "None", "the elements to list"),
        parameter("types", ParameterKind::TextList, "None", "the element types (\"ShellQuad4\", ...)"),
        parameter("propertyIds", ParameterKind::IdList, "None", "the properties of the elements (needs the model)"),
        parameter("location", ParameterKind::Text, "None", "None (all), \"center\" or \"corners\""),
        parameter("layers", ParameterKind::IdList, "None", "the layers to keep (1 = Z1, 2 = Z2, a ply number)"),
        parameter("layout", ParameterKind::Text, "\"unified\"",
                  "\"unified\" or \"native\" (one element type, the columns of the file)")};
    info.defaultSort = "subcase, [step], elementId, nodeId, layer";
    ColumnListBuilder columns;
    columns.int32("subcase")
        .step()
        .int32("elementId")
        .dictionary("type")
        .int32("propId")
        .dictionary("location")
        .int32("nodeId")
        .int32("layer");
    if (strain)
        columns.results("fiberDistance exx eyy ezz exy eyz ezx e1 e2 e3 vonMises maxShear axial torsion");
    else
        columns.results("fiberDistance oxx oyy ozz txy tyz tzx o1 o2 o3 vonMises maxShear axial torsion");
    columns.dictionary("frame");
    info.columnSets.push_back(columns.build());
    return info;
}

std::shared_ptr<const QueryTable> stressTable(const ResultQueryContext& context, const QueryInfo& info, bool strain)
{
    const QueryArguments& arguments = context.arguments();
    const std::string layout = arguments.textOr("layout", "unified");
    if (layout != "unified" && layout != "native")
        throw QueryError("layout is \"unified\" or \"native\", not '" + layout + "'");
    const ElementRequest request = ElementRequest::from(arguments);
    if (!request.location.empty() && request.location != "center" && request.location != "corners")
        throw QueryError("location is None, \"center\" or \"corners\", not '" + request.location + "'");
    if (request.propertyIds)
        context.requireModel("find the property of each element");
    if (layout == "native")
    {
        const std::set<std::string> cards(request.types.begin(), request.types.end());
        if (cards.size() != 1)
            throw QueryError(
                "layout=\"native\" needs a single element type: give types=[\"ShellQuad4\"] (the columns of the "
                "file are those of one element type)");
        const std::string container = containerOfCard(*cards.begin());
        const results::ResultKind kind = strain ? results::ResultKind::Strain : results::ResultKind::Stress;
        std::vector<std::shared_ptr<const QueryTable>> parts;
        for (const TableRef& ref : context.elementTables(kind, container, false))
        {
            std::shared_ptr<results::ResultTable> table = ref.table;
            results::ResultFilter filter;
            if (request.elementIds)
                filter.elementIds = std::vector<std::int32_t>(request.elementIds->begin(), request.elementIds->end());
            if (request.layers)
                filter.layers = std::vector<std::int32_t>(request.layers->begin(), request.layers->end());
            try
            {
                if (filter.hasRowSelection())
                    table = table->select(filter);
                parts.push_back(resultTableAsQuery(table->data(context.decode()), *table, false));
            }
            catch (const base::EmptySelectionError&)
            {
            }
        }
        if (parts.empty())
        {
            if (context.selection().onMissing == OnMissing::Raise)
                throw QuantityUnavailableError("no " + std::string(strain ? "strain" : "stress") + " table of " +
                                               *cards.begin() +
                                               " in the result file for the subcases and elements asked for");
            return emptyTable(info.defaultColumns());
        }
        return concatenateResultTables(
            parts, "the " + std::string(strain ? "strain" : "stress") + " tables of " + *cards.begin());
    }
    const StressData data = resolveStress(context, request, strain);
    if (data.set.flats().empty())
        return emptyTable(info.defaultColumns());
    RowAccumulator accumulator;
    std::vector<std::int16_t> providers;
    for (const ProviderInfo& provider : data.set.providers())
        providers.push_back(accumulator.addProvider(provider.name, provider.approximate));
    const std::int16_t derived = accumulator.addProvider("computed from the components", false);
    for (std::size_t index = 0; index < data.set.flats().size(); ++index)
    {
        const ElementFlat& flat = data.set.flats()[index];
        const ScalarCompletion& completion = data.completions[index];
        accumulator.note(derived, {"vonMises"}, completion.vonMises);
        accumulator.note(derived, {strain ? "e1" : "o1", strain ? "e2" : "o2", strain ? "e3" : "o3"},
                         completion.principal);
        accumulator.note(derived, {"maxShear"}, completion.maxShear);
        accumulator.add(stressBlock(
            context, flat, completion,
            flat.provider >= 0 ? providers[static_cast<std::size_t>(flat.provider)] : std::int16_t(-1), strain));
    }
    std::vector<std::string> keys = leadingKeys();
    keys.insert(keys.end(), {"elementId", "nodeId", "layer"});
    return sortedResult(accumulator.finish(context, info), context.exec(), keys);
}

void noteInputs(RowAccumulator& accumulator, const StressData& data, const std::vector<std::string>& columns)
{
    noteInputs(accumulator, data.set, columns);
}

std::vector<ElementExtreme> elementExtremes(const StressData& data, const std::string& quantity, bool perSubcase)
{
    std::map<std::pair<std::int32_t, std::int32_t>, ElementExtreme> found;
    for (const ElementFlat& flat : data.set.flats())
    {
        if (flat.isComplex() || flat.eid.empty())
            continue;
        const std::vector<double>& values = quantityOf(flat, quantity);
        for (std::size_t row = 0; row < flat.rows(); ++row)
        {
            const double value = values[row];
            if (std::isnan(value))
                continue;
            const auto key = std::make_pair(flat.eid[row], perSubcase ? flat.subcase : 0);
            const double step = flat.step.empty() ? NOT_A_NUMBER : flat.step[row];
            auto [position, inserted] = found.try_emplace(key);
            ElementExtreme& extreme = position->second;
            if (inserted)
            {
                extreme.set = flat.set;
                extreme.eid = flat.eid[row];
                extreme.card = flat.card;
                extreme.subcase = key.second;
                extreme.maxValue = extreme.minValue = value;
                extreme.maxSet = extreme.minSet = flat.set;
                extreme.maxSubcase = extreme.minSubcase = flat.subcase;
                extreme.maxStep = extreme.minStep = step;
                extreme.maxLayer = extreme.minLayer = flat.layer[row];
                extreme.maxLocation = extreme.minLocation = flat.location[row];
                extreme.precision = flat.precision;
                continue;
            }
            if (value > extreme.maxValue)
            {
                extreme.maxValue = value;
                extreme.maxSet = flat.set;
                extreme.maxSubcase = flat.subcase;
                extreme.maxStep = step;
                extreme.maxLayer = flat.layer[row];
                extreme.maxLocation = flat.location[row];
            }
            if (value < extreme.minValue)
            {
                extreme.minValue = value;
                extreme.minSet = flat.set;
                extreme.minSubcase = flat.subcase;
                extreme.minStep = step;
                extreme.minLayer = flat.layer[row];
                extreme.minLocation = flat.location[row];
            }
            if (flat.precision == Precision::Double)
                extreme.precision = Precision::Double;
        }
    }
    std::vector<ElementExtreme> result;
    result.reserve(found.size());
    for (auto& entry : found)
        result.push_back(std::move(entry.second));
    return result;
}

QueryInfo stressByGroupInfo(bool byMaterial)
{
    QueryInfo info;
    info.name = byMaterial ? "stressByMaterial" : "stressByProperty";
    info.family = "R2";
    info.resultQuery = true;
    info.needsModel = true;
    const std::string group = byMaterial ? "material" : "property";
    info.description =
        std::string("For each ") + group +
        " of the model, the largest and the smallest value of a stress (a strain with `strain`) over its elements, "
        "subcases and steps, the element and subcase where each occurs, and the mean over the elements of the maximum "
        "of "
        "each. `quantity` is `vonMises`, a component or a principal value. Needs the model to know the " +
        group +
        (byMaterial ? " of each element (the main material: the first one of the property)." : " of each element.");
    info.parameters = {parameter("quantity", ParameterKind::Text, "\"vonMises\"", "the stress or strain quantity"),
                       parameter("strain", ParameterKind::Bool, "False", "a strain quantity instead of a stress")};
    info.defaultSort = byMaterial ? "matId" : "propId";
    ColumnListBuilder columns;
    columns.int32(byMaterial ? "matId" : "propId")
        .int64("elementCount")
        .result("max")
        .int32("maxElementId")
        .dictionary("maxAnalysis")
        .int32("maxSubcase");
    columns.result("min").int32("minElementId").dictionary("minAnalysis").int32("minSubcase").real("mean");
    info.columnSets.push_back(columns.build());
    return info;
}

std::shared_ptr<const QueryTable> stressByGroup(const ResultQueryContext& context, const QueryInfo& info,
                                                bool byMaterial)
{
    const QueryArguments& arguments = context.arguments();
    const std::string quantity = arguments.textOr("quantity", "vonMises");
    const bool strain = arguments.boolOr("strain", false);
    if (!isStressQuantity(quantity, strain))
        throw QueryError("quantity '" + quantity + "' is not a " + (strain ? "strain" : "stress") + " quantity");
    const StressData data = resolveStress(context, ElementRequest::from(arguments), strain);
    const std::vector<ElementExtreme> extremes = elementExtremes(data, quantity, false);
    if (extremes.empty())
        return emptyTable(info.defaultColumns());
    struct Group
    {
        std::int64_t count = 0;
        double sum = 0.0;
        double maximum = 0.0;
        std::int32_t maxEid = 0;
        std::int32_t maxSubcase = 0;
        std::size_t maxSet = 0;
        double minimum = 0.0;
        std::int32_t minEid = 0;
        std::int32_t minSubcase = 0;
        std::size_t minSet = 0;
        bool single = true;
    };
    ResultModelIndex index(context);
    std::map<std::int32_t, Group> groups;
    for (const ElementExtreme& extreme : extremes)
    {
        const std::int32_t id =
            byMaterial ? index.material(extreme.set, extreme.eid) : context.elementCard(extreme.set, extreme.eid).pid;
        if (id <= 0)
            continue;
        auto [position, inserted] = groups.try_emplace(id);
        Group& group = position->second;
        ++group.count;
        group.sum += extreme.maxValue;
        group.single = group.single && extreme.precision == Precision::Single;
        if (inserted || extreme.maxValue > group.maximum)
        {
            group.maximum = extreme.maxValue;
            group.maxEid = extreme.eid;
            group.maxSubcase = extreme.maxSubcase;
            group.maxSet = extreme.maxSet;
        }
        if (inserted || extreme.minValue < group.minimum)
        {
            group.minimum = extreme.minValue;
            group.minEid = extreme.eid;
            group.minSubcase = extreme.minSubcase;
            group.minSet = extreme.minSet;
        }
    }
    if (groups.empty())
        return emptyTable(info.defaultColumns());
    std::vector<std::int32_t> key;
    std::vector<std::int32_t> maxEid;
    std::vector<std::int32_t> maxSubcase;
    std::vector<std::int32_t> minEid;
    std::vector<std::int32_t> minSubcase;
    std::vector<std::string> maxAnalysis;
    std::vector<std::string> minAnalysis;
    std::vector<std::int32_t> count;
    std::vector<double> maximum;
    std::vector<double> minimum;
    std::vector<double> mean;
    bool single = true;
    for (const auto& [id, group] : groups)
    {
        key.push_back(id);
        count.push_back(static_cast<std::int32_t>(group.count));
        maximum.push_back(group.maximum);
        maxEid.push_back(group.maxEid);
        maxSubcase.push_back(group.maxSubcase);
        minimum.push_back(group.minimum);
        minEid.push_back(group.minEid);
        minSubcase.push_back(group.minSubcase);
        maxAnalysis.push_back(context.sets()[group.maxSet].analysis);
        minAnalysis.push_back(context.sets()[group.minSet].analysis);
        mean.push_back(group.sum / static_cast<double>(group.count));
        single = single && group.single;
    }
    RowBlock block(key.size());
    block.ints(byMaterial ? "matId" : "propId", std::move(key));
    block.ints("elementCount", std::move(count));
    block.reals("max", std::move(maximum));
    block.ints("maxElementId", std::move(maxEid));
    block.texts("maxAnalysis", std::move(maxAnalysis));
    block.ints("maxSubcase", std::move(maxSubcase));
    block.reals("min", std::move(minimum));
    block.ints("minElementId", std::move(minEid));
    block.texts("minAnalysis", std::move(minAnalysis));
    block.ints("minSubcase", std::move(minSubcase));
    block.reals("mean", std::move(mean));
    block.precision(single ? Precision::Single : Precision::Double);
    RowAccumulator accumulator;
    noteInputs(accumulator, data, {"max", "min", "mean"});
    accumulator.add(std::move(block));
    return accumulator.finish(context, info);
}

} // namespace oor::query
