#include "StressData.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r2_stress_strain/StressStrainQueries.hpp"
#include "oor/query/results/NamedArrays.hpp"

#include <cmath>
#include <map>
#include <set>
#include <tuple>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

struct Extreme
{
    double value = 0.0;
    std::size_t set = 0;
    std::int32_t subcase = 0;
    double step = 0.0;
    std::int32_t layer = 0;
    std::string location;
};

struct Group
{
    std::size_t set = 0;
    std::string card;
    Extreme maximum;
    Extreme minimum;
    Precision precision = Precision::Double;
    bool stored = false; // from an envelope a solver gave (the named result `envelope`), not from the tables
};

// A row of an envelope a set made in memory holds (the named result `envelope`), of the stress or strain quantity asked
// for: the extremes over its load cases.
struct StoredRow
{
    std::size_t set = 0;
    std::int32_t eid = 0;
    std::string card;
    std::int32_t layer = 1;
    std::string location;
    Extreme maximum;
    Extreme minimum;
    Precision precision = Precision::Double;
};

std::vector<StoredRow> storedEnvelopeRows(const ResultQueryContext& context, const ElementRequest& request,
                                          const std::string& quantity, bool strain)
{
    std::vector<StoredRow> rows;
    for (const ResultQueryContext::NamedRef& named : context.named("envelope"))
    {
        const std::shared_ptr<const results::NamedResultData> data = named.result->data(context.decode());
        const results::ResultText* result = data->findText("result");
        const results::ResultText* elementType = data->findText("elementType");
        const results::ResultText* component = data->findText("component");
        if (result == nullptr || elementType == nullptr || component == nullptr)
            continue;
        const std::vector<std::string> results = result->values();
        const std::vector<std::string> elementTypes = elementType->values();
        const std::vector<std::string> components = component->values();
        const auto ints = [&](const char* name)
        {
            const results::ResultArray* array = data->find(name);
            return array != nullptr ? arrayInts(*array) : std::vector<std::int32_t>();
        };
        const auto reals = [&](const char* name)
        {
            const results::ResultArray* array = data->find(name);
            return array != nullptr ? arrayDoubles(*array) : std::vector<double>();
        };
        const std::vector<std::int32_t> ids = ints("id");
        const std::vector<std::int32_t> nodes = ints("nodeId");
        const std::vector<std::int32_t> layers = ints("layer");
        const std::vector<double> maxima = reals("max");
        const std::vector<std::int32_t> maxSubcases = ints("maxSubcase");
        const std::vector<double> maxSteps = reals("maxStep");
        const std::vector<double> minima = reals("min");
        const std::vector<std::int32_t> minSubcases = ints("minSubcase");
        const std::vector<double> minSteps = reals("minStep");
        const results::ResultArray* maxArray = data->find("max");
        const Precision precision = maxArray != nullptr && maxArray->type == results::ArrayType::Float32
                                        ? Precision::Single
                                        : Precision::Double;
        for (std::size_t row = 0; row < ids.size(); ++row)
        {
            if (results[row] != (strain ? "strain" : "stress") || components[row] != quantity ||
                elementTypes[row].empty())
                continue;
            const std::string card = cardOfContainer(elementTypes[row]);
            if (!request.wantsElement(ids[row]) || !request.wantsType(card) || !request.wantsLayer(layers[row]) ||
                (request.location == "center" && nodes[row] != 0) || (request.location == "corners" && nodes[row] == 0))
                continue;
            StoredRow stored;
            stored.set = named.set;
            stored.eid = ids[row];
            stored.card = card;
            stored.layer = layers[row];
            stored.location = nodes[row] == 0 ? "center" : "corner";
            stored.maximum =
                Extreme{maxima[row], named.set, maxSubcases[row], maxSteps[row], layers[row], stored.location};
            stored.minimum =
                Extreme{minima[row], named.set, minSubcases[row], minSteps[row], layers[row], stored.location};
            stored.precision = precision;
            rows.push_back(std::move(stored));
        }
    }
    return rows;
}

// stressEnvelope: for each element, the largest and smallest value of a stress (or strain) over the subcases, steps,
// layers and locations named in `over`, and where they are; what is not in `over` keeps the rows apart.
class StressEnvelopeQuery final : public ResultQuery
{
public:
    StressEnvelopeQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const QueryArguments& arguments = context.arguments();
        const std::string quantity = arguments.textOr("quantity", "vonMises");
        const bool strain = arguments.boolOr("strain", false);
        if (!isStressQuantity(quantity, strain))
            throw QueryError("quantity '" + quantity + "' is not a " + (strain ? "strain" : "stress") +
                             " quantity (vonMises, " +
                             (strain ? "exx eyy ezz exy eyz ezx e1 e2 e3" : "oxx oyy ozz txy tyz tzx o1 o2 o3") +
                             ", maxShear, axial, torsion, maxPrincipal, minPrincipal)");
        std::set<std::string> over = {"subcases", "steps", "layers", "locations", "analyses"};
        if (arguments.has("over"))
        {
            over.clear();
            for (const std::string& name : arguments.texts("over"))
            {
                if (name != "subcases" && name != "steps" && name != "layers" && name != "locations" &&
                    name != "analyses")
                    throw QueryError("over names subcases, steps, layers, locations or analyses, not '" + name + "'");
                over.insert(name);
            }
        }
        const ElementRequest request = ElementRequest::from(arguments);
        if (request.propertyIds)
            context.requireModel("find the property of each element");
        // the envelopes a solver gave in memory are extremes over its subcases and steps already: they are read when
        // the call takes the extremes over both
        const std::vector<StoredRow> stored = over.contains("subcases") && over.contains("steps")
                                                  ? storedEnvelopeRows(context, request, quantity, strain)
                                                  : std::vector<StoredRow>();
        StressData data;
        try
        {
            data = resolveStress(context, request, strain);
        }
        catch (const QuantityUnavailableError&)
        {
            if (stored.empty())
                throw;
        }

        using Key = std::tuple<std::size_t, std::int32_t, std::int32_t, std::uint64_t, std::int32_t, std::string>;
        std::map<Key, Group> groups;
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
                const double step = flat.step.empty() ? NOT_A_NUMBER : flat.step[row];
                const Key key{over.contains("analyses") ? 0 : flat.set,
                              flat.eid[row],
                              over.contains("subcases") ? -1 : flat.subcase,
                              over.contains("steps") ? 0 : (std::isnan(step) ? 0 : stepKey(step)),
                              over.contains("layers") ? -1 : flat.layer[row],
                              over.contains("locations") ? std::string() : flat.location[row]};
                auto [position, inserted] = groups.try_emplace(key);
                Group& group = position->second;
                const Extreme here{value, flat.set, flat.subcase, step, flat.layer[row], flat.location[row]};
                if (inserted)
                {
                    group.set = flat.set;
                    group.card = flat.card;
                    group.maximum = here;
                    group.minimum = here;
                    group.precision = flat.precision;
                    continue;
                }
                if (value > group.maximum.value)
                    group.maximum = here;
                if (value < group.minimum.value)
                    group.minimum = here;
                if (flat.precision == Precision::Double)
                    group.precision = Precision::Double;
            }
        }
        for (const StoredRow& row : stored)
        {
            if (request.propertyIds && !request.wantsProperty(context.elementCard(row.set, row.eid).pid))
                continue;
            const Key key{over.contains("analyses") ? 0 : row.set,
                          row.eid,
                          -1,
                          0,
                          over.contains("layers") ? -1 : row.layer,
                          over.contains("locations") ? std::string() : row.location};
            auto [position, inserted] = groups.try_emplace(key);
            Group& group = position->second;
            if (inserted)
            {
                group.set = row.set;
                group.card = row.card;
                group.maximum = row.maximum;
                group.minimum = row.minimum;
                group.precision = row.precision;
                group.stored = true;
                continue;
            }
            group.stored = true;
            if (row.maximum.value > group.maximum.value)
                group.maximum = row.maximum;
            if (row.minimum.value < group.minimum.value)
                group.minimum = row.minimum;
        }
        if (groups.empty())
            return emptyTable(info().defaultColumns());
        const bool overAnalyses = over.contains("analyses");
        // with the analyses not in `over`, each result set has its rows (and its `analysis` column); else one block
        std::map<std::size_t, std::vector<const std::pair<const Key, Group>*>> bySet;
        for (const auto& entry : groups)
            bySet[overAnalyses ? 0 : entry.second.set].push_back(&entry);
        RowAccumulator accumulator;
        std::vector<std::int16_t> providers;
        for (const ProviderInfo& provider : data.set.providers())
            providers.push_back(accumulator.addProvider(provider.name, provider.approximate));
        const std::int16_t storedProvider =
            stored.empty() ? std::int16_t{-1} : accumulator.addProvider("stored envelope of the result set", false);
        std::int64_t storedGroups = 0;
        const auto labelOf = [&](std::size_t set) { return context.sets()[set].analysis; };
        for (const auto& [set, entries] : bySet)
        {
            const std::size_t count = entries.size();
            std::vector<std::int32_t> eid;
            std::vector<std::string> type;
            std::vector<std::int32_t> pid;
            std::vector<std::uint8_t> pidValid;
            std::vector<double> maxValue, minValue, absMax, maxStep, minStep;
            std::vector<std::int32_t> maxSubcase, minSubcase, maxLayer, minLayer;
            std::vector<std::uint8_t> maxLayerValid, minLayerValid;
            std::vector<std::string> maxLocation, minLocation, maxAnalysis, minAnalysis;
            std::vector<std::int16_t> rowProviders;
            bool single = true;
            for (const auto* entry : entries)
            {
                const Key& key = entry->first;
                const Group& group = entry->second;
                storedGroups += group.stored ? 1 : 0;
                rowProviders.push_back(group.stored ? storedProvider
                                                    : (providers.size() == 1 ? providers.front() : std::int16_t{-1}));
                eid.push_back(std::get<1>(key));
                type.push_back(group.card);
                const std::int32_t property = context.elementCard(group.set, std::get<1>(key)).pid;
                pid.push_back(property);
                pidValid.push_back(property > 0 ? 1 : 0);
                maxValue.push_back(group.maximum.value);
                minValue.push_back(group.minimum.value);
                absMax.push_back(std::max(std::fabs(group.maximum.value), std::fabs(group.minimum.value)));
                maxAnalysis.push_back(labelOf(group.maximum.set));
                minAnalysis.push_back(labelOf(group.minimum.set));
                maxSubcase.push_back(group.maximum.subcase);
                minSubcase.push_back(group.minimum.subcase);
                maxStep.push_back(group.maximum.step);
                minStep.push_back(group.minimum.step);
                maxLayer.push_back(group.maximum.layer);
                minLayer.push_back(group.minimum.layer);
                maxLayerValid.push_back(group.maximum.layer > 0 ? 1 : 0);
                minLayerValid.push_back(group.minimum.layer > 0 ? 1 : 0);
                maxLocation.push_back(group.maximum.location);
                minLocation.push_back(group.minimum.location);
                single = single && group.precision == Precision::Single;
            }
            RowBlock block(count);
            block.ints("elementId", std::move(eid));
            block.texts("type", std::move(type));
            block.ints("propId", std::move(pid), std::move(pidValid));
            block.reals("max", std::move(maxValue));
            block.texts("maxAnalysis", std::move(maxAnalysis));
            block.ints("maxSubcase", std::move(maxSubcase));
            block.reals("maxStep", std::move(maxStep));
            block.ints("maxLayer", std::move(maxLayer), std::move(maxLayerValid));
            block.texts("maxLocation", std::move(maxLocation));
            block.reals("min", std::move(minValue));
            block.texts("minAnalysis", std::move(minAnalysis));
            block.ints("minSubcase", std::move(minSubcase));
            block.reals("minStep", std::move(minStep));
            block.ints("minLayer", std::move(minLayer), std::move(minLayerValid));
            block.texts("minLocation", std::move(minLocation));
            block.reals("absmax", std::move(absMax));
            block.precision(single ? Precision::Single : Precision::Double);
            if (!overAnalyses)
                block.origin(set, labelOf(set), -1);
            if (providers.size() == 1 && stored.empty())
                block.provider(providers.front());
            else if (providers.size() <= 1)
                block.providers(std::move(rowProviders));
            accumulator.add(std::move(block));
        }
        if (providers.size() > 1)
        {
            noteInputs(accumulator, data, {"max", "min", "absmax"});
            if (storedGroups > 0)
                accumulator.note(storedProvider, {"max", "min", "absmax"}, storedGroups);
        }
        return accumulator.finish(context, info());
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "stressEnvelope";
        info.family = "R2";
        info.resultQuery = true;
        info.description =
            "For each element, the largest and the smallest value of a stress (or, with `strain`, of a strain) over "
            "the "
            "dimensions named in `over` (subcases, steps, layers, locations, analyses), and where each occurs "
            "(`maxAnalysis` and `minAnalysis` are the labels of the critical analyses); a dimension that is not in "
            "`over` keeps the rows apart (an element has a row for each of its layers when \"layers\" is not named, a "
            "row for each result set when \"analyses\" is not named). `absmax` is the largest magnitude. `quantity` is "
            "`vonMises`, a component or a principal value. The envelope a solver gave in memory (the named result "
            "`envelope`, its rows of `stress` or `strain` and of the quantity) is read too when `over` names the "
            "subcases and the steps: its extremes count with those of the tables.";
        info.parameters = {parameter("quantity", ParameterKind::Text, "\"vonMises\"", "the stress or strain quantity"),
                           parameter("over", ParameterKind::TextList,
                                     "(\"subcases\", \"steps\", \"layers\", \"locations\", \"analyses\")",
                                     "the dimensions the extremes are taken over"),
                           parameter("strain", ParameterKind::Bool, "False", "a strain quantity instead of a stress"),
                           parameter("elementIds", ParameterKind::IdList, "None", "the elements to look at"),
                           parameter("types", ParameterKind::TextList, "None", "the element types"),
                           parameter("propertyIds", ParameterKind::IdList, "None",
                                     "the properties of the elements (needs the model)")};
        info.defaultSort = "elementId";
        ColumnListBuilder columns;
        columns.int32("elementId").dictionary("type").int32("propId").result("max").dictionary("maxAnalysis");
        columns.int32("maxSubcase").real("maxStep");
        columns.int32("maxLayer").dictionary("maxLocation").result("min").dictionary("minAnalysis");
        columns.int32("minSubcase").real("minStep");
        columns.int32("minLayer").dictionary("minLocation").result("absmax");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addStressEnvelopeQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<StressEnvelopeQuery>());
}

} // namespace oor::query
