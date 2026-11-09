#include "MarginData.hpp"

#include "../r2_stress_strain/PlyData.hpp"
#include "../r2_stress_strain/StressData.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/providers/quantities/FastenerInteraction.hpp"
#include "oor/query/providers/quantities/MaterialAllowables.hpp"
#include "oor/query/results/ResultModelIndex.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <tuple>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

} // namespace

std::vector<ParameterInfo> marginParameters()
{
    return {parameter("criterion", ParameterKind::Text, "(required)",
                      "\"vonMises\", \"maxPrincipal\", \"minPrincipal\", \"composite\", \"bond\" or "
                      "\"fastener\""),
            parameter("allowables", ParameterKind::Values, "None",
                      "a dict keyed by (\"material\", mid), (\"property\", propId) or (\"element\", elementId), or one "
                      "value for all; "
                      "pairs (tension, shear) for fasteners"),
            parameter("factor", ParameterKind::Real, "1.0", "the factor of safety on the loads"),
            parameter("theory", ParameterKind::Text, "None",
                      "composites: None (the stored index, else the property's theory), \"maxStress\", "
                      "\"maxStrain\", \"hill\", \"hoffman\", \"tsaiWu\", or a theory of failure modes: "
                      "\"hashin\", \"puck\", \"larc03\""),
            parameter("exponents", ParameterKind::Reals, "(2.0, 3.0)",
                      "fasteners: the exponents of the tension and the shear")};
}

MarginSet computeMargins(const ResultQueryContext& context)
{
    const QueryArguments& arguments = context.arguments();
    MarginSet result;
    result.criterion = arguments.textOr("criterion", "");
    const std::string& criterion = result.criterion;
    if (!arguments.has("criterion"))
        throw QueryError(
            "the margins need a `criterion`: \"vonMises\", \"maxPrincipal\", \"minPrincipal\", \"composite\", "
            "\"bond\" or \"fastener\"");
    if (criterion != "vonMises" && criterion != "maxPrincipal" && criterion != "minPrincipal" &&
        criterion != "composite" && criterion != "bond" && criterion != "fastener")
        throw QueryError(
            "criterion is \"vonMises\", \"maxPrincipal\", \"minPrincipal\", \"composite\", \"bond\" or "
            "\"fastener\", not '" +
            criterion + "'");
    const double factor = arguments.realOr("factor", 1.0);
    if (!(factor > 0.0))
        throw QueryError("factor is a factor of safety, a positive number");
    std::vector<double> exponents = arguments.reals("exponents");
    if (exponents.empty())
        exponents = {2.0, 3.0};
    if (exponents.size() != 2 || !(exponents[0] > 0.0) || !(exponents[1] > 0.0))
        throw QueryError("exponents are two positive numbers (the exponent of the tension, then of the shear)");
    const CallerAllowables caller(arguments.values("allowables"));
    const ElementRequest request = ElementRequest::from(arguments);
    if (request.propertyIds)
        context.requireModel("find the property of each element");
    ResultModelIndex index(context);

    if (criterion == "composite" || criterion == "bond")
    {
        // `bond`: the bonding index of the plies (the solver's failure index for bonding, against the PCOMP SB), linear
        // in the loads, judged as the failure index is
        const bool bond = criterion == "bond";
        const std::string field = bond ? "bondIndex" : "failureIndex";
        const PlyData data = buildPlyData(context, request);
        result.providers = data.providers;
        if (data.flats.empty() || std::none_of(data.flats.begin(), data.flats.end(),
                                               [&](const ElementFlat& flat)
                                               {
                                                   return std::any_of(flat.values.at(field).begin(),
                                                                      flat.values.at(field).end(),
                                                                      [](double v) { return !std::isnan(v); });
                                               }))
        {
            if (context.selection().onMissing == OnMissing::Raise)
            {
                if (bond)
                    throw QuantityUnavailableError(
                        "the bonding index of the plies: the failure index table holds none, and the laminates have "
                        "no SB (PCOMP) or the stress tables of the plies no interlaminar shear");
                throw QuantityUnavailableError(describeAttempts("the composite failure index", data.attempts));
            }
            return result;
        }
        // The reserve factor of an element is the smallest of its plies: each ply's own (its strength ratio), not that
        // of the ply of the largest index, which a quadratic theory can tell apart. A ply whose index gives no factor
        // (an index of 0 or less, no quadratic part) is not critical; of plies that are none, the largest index stands.
        using Key = std::tuple<std::size_t, std::int32_t, std::int32_t, std::uint64_t, std::int32_t>;
        // the MAT8 of the plies, for the strength ratio of the theories of failure modes at the caller's limit
        std::map<std::pair<const model::Model*, std::int32_t>, std::optional<PlyMaterial>> materials;
        const auto materialOf = [&](std::size_t set, std::int32_t mid) -> const std::optional<PlyMaterial>&
        {
            const model::Model* model = context.modelOf(set);
            auto [found, added] = materials.try_emplace({model, mid});
            if (added && model != nullptr && mid > 0)
                found->second = orthoMat2DAllowables(*model, mid);
            return found->second;
        };
        // PERFORMANCE (COMPOSITE-API-AUDIT P2): the rows are compared by their numbers only, the columns of a flat
        // looked up once, the property of an element and the caller's allowable once a run of its plies, the entry of
        // a key kept across the rows of that key; the rows of the margin table are made for the plies kept alone
        struct Candidate
        {
            double rank = 0.0; // the reserve factor, infinity when it gives none
            double value = 0.0;
            double rf = NOT_A_NUMBER;
            double allowable = 1.0;
            bool byCaller = false;
            std::int32_t mid = 0;
            std::int32_t pid = 0;
            std::size_t flat = 0;
            std::size_t row = 0;
        };
        std::map<Key, Candidate> kept;
        for (std::size_t flatIndex = 0; flatIndex < data.flats.size(); ++flatIndex)
        {
            const ElementFlat& flat = data.flats[flatIndex];
            const std::vector<double>& indices = flat.values.at(field);
            const std::vector<double>& mids = flat.values.at("matId");
            const std::vector<double>* quadratic = bond ? nullptr : &flat.values.at("indexQuadratic");
            const std::vector<double>* linear = bond ? nullptr : &flat.values.at("indexLinear");
            const std::vector<double>& o11 = flat.values.at("o11");
            const std::vector<double>& o22 = flat.values.at("o22");
            const std::vector<double>& t12 = flat.values.at("t12");
            const std::vector<std::string>& theories = flat.texts.at("failureTheory");
            std::int32_t lastElement = 0;
            std::int32_t pid = 0;
            bool element = false;
            std::optional<Key> lastKey;
            std::map<Key, Candidate>::iterator at;
            for (std::size_t row = 0; row < flat.rows(); ++row)
            {
                const double value = indices[row];
                if (std::isnan(value))
                    continue;
                const double step = flat.step.empty() ? NOT_A_NUMBER : flat.step[row];
                const Key key{flat.set, flat.subcase, flat.designCycle, std::isnan(step) ? 0 : stepKey(step),
                              flat.eid[row]};
                const double material = mids[row];
                const std::int32_t mid = std::isnan(material) ? 0 : static_cast<std::int32_t>(material);
                if (!element || flat.eid[row] != lastElement)
                {
                    pid = context.elementCard(flat.set, flat.eid[row]).pid;
                    lastElement = flat.eid[row];
                    element = true;
                }
                const std::optional<double> given =
                    caller.any() ? caller.find(flat.eid[row], pid, mid, 0) : std::optional<double>();
                const double allowable = given.value_or(1.0);
                PlyIndex plyIndex;
                plyIndex.index = value;
                plyIndex.quadratic = bond ? 0.0 : (*quadratic)[row];
                plyIndex.linear = bond ? value : (*linear)[row];
                // the theory's own reserve factor when the model gives it; else the index is taken to scale with the
                // loads
                double scale = std::isnan(plyIndex.quadratic) || std::isnan(plyIndex.linear)
                                   ? NOT_A_NUMBER
                                   : scaleToLimit(plyIndex, allowable);
                const std::string& rowTheory = theories[row];
                if (!bond && isModeTheory(rowTheory))
                    if (const std::optional<PlyMaterial>& ply = materialOf(flat.set, mid))
                        scale = plyStrengthRatio(rowTheory, o11[row], o22[row], t12[row], *ply, allowable);
                double rf = NOT_A_NUMBER;
                if (!std::isnan(scale))
                    rf = scale / factor;
                else if (value > 0.0)
                    rf = allowable / (factor * value);
                const double rank = std::isnan(rf) ? std::numeric_limits<double>::infinity() : rf;
                bool inserted = false;
                if (!lastKey || *lastKey != key)
                {
                    std::tie(at, inserted) = kept.try_emplace(key);
                    lastKey = key;
                }
                Candidate& entry = at->second;
                if (!inserted && (rank > entry.rank || (rank == entry.rank && !(value > entry.value))))
                    continue;
                entry = Candidate{rank, value, rf, allowable, given.has_value(), mid, pid, flatIndex, row};
            }
        }
        for (const auto& [key, candidate] : kept)
        {
            const ElementFlat& flat = data.flats[candidate.flat];
            const std::size_t row = candidate.row;
            const std::string& theory = flat.texts.at("failureTheory")[row];
            MarginRow entry;
            entry.allowable = candidate.allowable;
            entry.allowableSource = candidate.byCaller ? "caller"
                                    : bond ? "bonding index (SB)"
                                           : (theory.empty() ? "failure index" : "failure index (" + theory + ")");
            entry.factor = factor;
            entry.rf = candidate.rf;
            entry.ms = candidate.rf - 1.0;
            entry.mid = candidate.mid;
            entry.pid = candidate.pid;
            entry.set = flat.set;
            entry.analysis = flat.analysis;
            entry.subcase = flat.subcase;
            entry.designCycle = flat.designCycle;
            entry.stepKind = flat.stepKind;
            entry.step = flat.step.empty() ? NOT_A_NUMBER : flat.step[row];
            entry.eigenvalue = flat.eigenvalue.empty() ? NOT_A_NUMBER : flat.eigenvalue[row];
            entry.eid = flat.eid[row];
            entry.card = flat.card;
            entry.layer = flat.layer[row];
            entry.location = "ply " + std::to_string(flat.layer[row]);
            entry.criterion = criterion;
            entry.value = candidate.value;
            entry.precision = flat.precision;
            entry.status = flat.status;
            // the provider is that of the failure index; the bonding index has its own way, said by its source
            const double provider = bond ? -1.0 : flat.values.at("failureProvider")[row];
            entry.provider = provider >= 0.0 ? static_cast<std::int16_t>(provider) : std::int16_t(-1);
            result.rows.push_back(std::move(entry));
        }
        return result;
    }

    if (criterion == "fastener")
    {
        const ElementResolution resolution =
            resolveElements(context, fastenerForceChain(), request, "the forces of the fasteners", fastenerFields());
        result.providers = resolution.set.providers();
        for (const ElementFlat& flat : resolution.set.flats())
        {
            if (flat.isComplex())
                continue;
            const std::vector<double>& axial = flat.values.at("axial");
            const std::vector<double>& shear = flat.values.at("shear");
            for (std::size_t row = 0; row < flat.rows(); ++row)
            {
                MarginRow entry;
                entry.set = flat.set;
                entry.analysis = flat.analysis;
                entry.subcase = flat.subcase;
                entry.designCycle = flat.designCycle;
                entry.stepKind = flat.stepKind;
                entry.step = flat.step.empty() ? NOT_A_NUMBER : flat.step[row];
                entry.eigenvalue = flat.eigenvalue.empty() ? NOT_A_NUMBER : flat.eigenvalue[row];
                entry.eid = flat.eid[row];
                entry.card = flat.card;
                entry.pid = context.elementCard(flat.set, flat.eid[row]).pid;
                entry.criterion = "fastener";
                entry.precision = flat.precision;
                entry.status = flat.status;
                entry.provider = flat.provider;
                entry.factor = factor;
                entry.tension = std::isnan(axial[row]) ? NOT_A_NUMBER : std::max(axial[row], 0.0);
                entry.shear = shear[row];
                const auto tensionAllowable = caller.find(entry.eid, entry.pid, 0, 0);
                const auto shearAllowable = caller.find(entry.eid, entry.pid, 0, 1);
                entry.tensionAllowable = tensionAllowable.value_or(NOT_A_NUMBER);
                entry.shearAllowable = shearAllowable.value_or(NOT_A_NUMBER);
                entry.allowable = 1.0;
                entry.allowableSource = tensionAllowable && shearAllowable ? "caller" : "none given";
                if (!(entry.tensionAllowable > 0.0) || !(entry.shearAllowable > 0.0) || std::isnan(entry.tension) ||
                    std::isnan(entry.shear))
                {
                    entry.allowable = NOT_A_NUMBER;
                    result.rows.push_back(entry);
                    continue;
                }
                entry.value = fastenerInteraction(entry.tension, entry.shear, entry.tensionAllowable,
                                                  entry.shearAllowable, exponents[0], exponents[1]);
                const double scale = fastenerLoadMultiplier(entry.tension, entry.shear, entry.tensionAllowable,
                                                            entry.shearAllowable, exponents[0], exponents[1]);
                if (!std::isnan(scale))
                {
                    entry.rf = scale / factor;
                    entry.ms = entry.rf - 1.0;
                }
                result.rows.push_back(entry);
            }
        }
        return result;
    }

    // the metallic criteria, from the stresses of the shells, solids and rods
    const StressData data = resolveStress(context, request, false);
    result.providers = data.set.providers();
    const bool compression = criterion == "minPrincipal";
    using Key = std::tuple<std::size_t, std::int32_t, std::int32_t, std::uint64_t, std::int32_t>;
    std::map<Key, MarginRow> best;
    for (const ElementFlat& flat : data.set.flats())
    {
        const ElementFamily family = familyOfContainer(flat.elementName);
        if (flat.isComplex() ||
            (family != ElementFamily::Shell && family != ElementFamily::Solid && family != ElementFamily::Rod))
            continue;
        const auto field = [&](const char* name) -> const std::vector<double>& { return flat.values.at(name); };
        for (std::size_t row = 0; row < flat.rows(); ++row)
        {
            double value = NOT_A_NUMBER;
            if (family == ElementFamily::Rod)
            {
                const double axial = field("axial")[row];
                const double torsion = std::isnan(field("torsion")[row]) ? 0.0 : field("torsion")[row];
                const double radius = std::sqrt(0.25 * axial * axial + torsion * torsion);
                value = criterion == "vonMises"       ? std::sqrt(axial * axial + 3.0 * torsion * torsion)
                        : criterion == "maxPrincipal" ? 0.5 * axial + radius
                                                      : 0.5 * axial - radius;
            }
            else if (criterion == "vonMises")
            {
                value = field("vonMises")[row];
            }
            else if (criterion == "maxPrincipal")
            {
                value = field("o1")[row];
            }
            else
            {
                value = family == ElementFamily::Solid ? field("o3")[row] : field("o2")[row];
            }
            if (std::isnan(value))
                continue;
            const double step = flat.step.empty() ? NOT_A_NUMBER : flat.step[row];
            const Key key{flat.set, flat.subcase, flat.designCycle, std::isnan(step) ? 0 : stepKey(step),
                          flat.eid[row]};
            auto [position, inserted] = best.try_emplace(key);
            MarginRow& entry = position->second;
            if (!inserted && !(compression ? value < entry.value : value > entry.value))
                continue;
            entry.set = flat.set;
            entry.analysis = flat.analysis;
            entry.subcase = flat.subcase;
            entry.designCycle = flat.designCycle;
            entry.stepKind = flat.stepKind;
            entry.step = step;
            entry.eigenvalue = flat.eigenvalue.empty() ? NOT_A_NUMBER : flat.eigenvalue[row];
            entry.eid = flat.eid[row];
            entry.card = flat.card;
            entry.layer = flat.layer[row];
            entry.location = flat.location[row];
            entry.criterion = criterion;
            entry.value = value;
            entry.precision = flat.precision;
            entry.status = flat.status;
            entry.provider = flat.provider;
        }
    }
    std::set<std::string> reasons; // why an allowable is missing, by material
    for (auto& [key, entry] : best)
    {
        entry.pid = context.elementCard(entry.set, entry.eid).pid;
        entry.factor = factor;
        const std::int32_t material = index.material(entry.set, entry.eid);
        entry.mid = material;
        const std::optional<double> byMaterial = caller.find(entry.eid, entry.pid, material, 0);
        if (byMaterial)
        {
            entry.allowable = *byMaterial;
            entry.allowableSource = "caller";
        }
        else
        {
            const model::Model* model = context.modelOf(entry.set);
            if (model == nullptr)
                throw QueryError(
                    "the margins on '" + criterion +
                    "' read the allowable from the material card of each element, and the results have no model: give "
                    "`allowables` for every element, or attach the results to the model");
            entry.allowable = isoMatAllowable(*model, material, compression, entry.allowableSource);
            if (std::isnan(entry.allowable))
            {
                reasons.insert(entry.allowableSource + " (element " + std::to_string(entry.eid) + ")");
                entry.allowableSource.clear();
            }
        }
        if (entry.allowable > 0.0)
        {
            const double load = compression ? -entry.value : entry.value;
            if (load > 0.0)
            {
                entry.rf = entry.allowable / (factor * load);
                entry.ms = entry.rf - 1.0;
            }
        }
        result.rows.push_back(entry);
    }
    if (!reasons.empty())
    {
        if (context.selection().onMissing == OnMissing::Raise)
        {
            std::string text =
                "no way of obtaining the margin allowable gave it; the ways tried, in order:\n  1. caller: "
                "`allowables` gives "
                "none for the elements (by element, property, material or for all)\n  2. material card: ";
            std::size_t shown = 0;
            for (const std::string& reason : reasons)
            {
                if (shown++ == 3)
                {
                    text += ", and " + std::to_string(reasons.size() - 3) + " more";
                    break;
                }
                text += (shown == 1 ? "" : "; ") + reason;
            }
            text +=
                "\ngive `allowables`, complete the card, or use onMissing=\"null\" (rows without margin) or \"skip\"";
            throw QuantityUnavailableError(text);
        }
        if (context.selection().onMissing == OnMissing::Skip)
            result.rows.erase(std::remove_if(result.rows.begin(), result.rows.end(),
                                             [](const MarginRow& row) { return std::isnan(row.allowable); }),
                              result.rows.end());
    }
    return result;
}

RowBlock marginBlock(const ResultQueryContext& context, const std::vector<const MarginRow*>& rows,
                     const std::vector<std::int16_t>& providers)
{
    (void)context;
    const std::size_t count = rows.size();
    RowBlock block(count);
    std::vector<std::int32_t> subcase, eid, pid, mid, layer;
    std::vector<std::uint8_t> pidValid, midValid, layerValid, locationValid, sourceValid;
    std::vector<std::string> type, location, criterion, source;
    std::vector<double> value, allowable, factor, rf, ms, tension, shear, tensionAllowable, shearAllowable, steps,
        eigenvalues;
    std::vector<std::int16_t> provider;
    bool single = true;
    for (const MarginRow* row : rows)
    {
        subcase.push_back(row->subcase);
        eid.push_back(row->eid);
        type.push_back(row->card);
        pid.push_back(row->pid);
        pidValid.push_back(row->pid > 0 ? 1 : 0);
        mid.push_back(row->mid);
        midValid.push_back(row->mid > 0 ? 1 : 0);
        layer.push_back(row->layer);
        layerValid.push_back(row->layer > 0 ? 1 : 0);
        location.push_back(row->location);
        locationValid.push_back(row->location.empty() ? 0 : 1);
        criterion.push_back(row->criterion);
        value.push_back(row->value);
        allowable.push_back(row->allowable);
        source.push_back(row->allowableSource);
        sourceValid.push_back(row->allowableSource.empty() ? 0 : 1);
        factor.push_back(row->factor);
        rf.push_back(row->rf);
        ms.push_back(row->ms);
        tension.push_back(row->tension);
        shear.push_back(row->shear);
        tensionAllowable.push_back(row->tensionAllowable);
        shearAllowable.push_back(row->shearAllowable);
        steps.push_back(row->step);
        eigenvalues.push_back(row->eigenvalue);
        provider.push_back(row->provider >= 0 ? providers[static_cast<std::size_t>(row->provider)] : std::int16_t(-1));
        single = single && row->precision == Precision::Single;
    }
    block.ints("subcase", std::move(subcase));
    block.ints("elementId", std::move(eid));
    block.texts("type", std::move(type));
    block.ints("propId", std::move(pid), std::move(pidValid));
    block.ints("matId", std::move(mid), std::move(midValid));
    block.ints("layer", std::move(layer), std::move(layerValid));
    block.texts("location", std::move(location), std::move(locationValid));
    block.texts("criterion", std::move(criterion));
    block.reals("value", std::move(value));
    block.reals("allowable", std::move(allowable));
    block.texts("allowableSource", std::move(source), std::move(sourceValid));
    block.reals("factor", std::move(factor));
    block.reals("rf", std::move(rf));
    block.reals("ms", std::move(ms));
    block.reals("tension", std::move(tension));
    block.reals("shear", std::move(shear));
    block.reals("tensionAllowable", std::move(tensionAllowable));
    block.reals("shearAllowable", std::move(shearAllowable));
    const MarginRow& first = *rows.front();
    if (first.stepKind != results::StepKind::None)
    {
        const bool anyEigen =
            std::any_of(eigenvalues.begin(), eigenvalues.end(), [](double v) { return !std::isnan(v); });
        block.steps(first.stepKind, std::move(steps), anyEigen ? std::move(eigenvalues) : std::vector<double>());
    }
    block.origin(first.set, first.analysis, first.designCycle);
    block.precision(single ? Precision::Single : Precision::Double);
    block.validation(first.status);
    block.providers(std::move(provider));
    return block;
}

} // namespace oor::query
