#include "oor/analysis/components/LoadSets.hpp"
#include "oor/analysis/components/SubcaseCombination.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/threading/ParallelFor.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/cards/loads/facePressureLoad/FacePressureLoadTable.hpp"
#include "oor/model/cards/loads/pointsPressureLoad/PointsPressureLoadTable.hpp"
#include "oor/model/cards/loads/shellPressureLoad/ShellPressureLoadTable.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/common/LoadVectors.hpp"
#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/named/common/SetSelection.hpp"
#include "oor/query/named/common/Topology.hpp"
#include "oor/query/named/topology/t8_loads_constraints/LoadConstraintQueries.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <optional>
#include <span>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// A shell as a pressure sees it: its corners in the order of its card (the mid-side points left out), their positions,
// its unit normal (the cross product of the diagonals of a quadrilateral, of two sides of a triangle: the sense of the
// order of its grid points) and its area.
struct Shell
{
    std::int32_t eid = 0;
    std::uint16_t type = 0;
    std::size_t count = 0;
    std::array<base::EntityId, 4> nodes{};
    std::array<base::Vec3, 4> corners{};
    base::Vec3 normal;
    base::Vec3 centre;
    std::optional<base::Vec3> plane; // a warped four-noded quadrilateral: the normal of its mean plane
};

std::optional<Shell> shellOf(const model::Model& model, const NodePositions& positions, std::int32_t eid,
                             std::vector<base::EntityId>& slots)
{
    const auto location = model.locate(base::IdSpace::Element, eid);
    if (!location)
        return std::nullopt;
    const analysis::ElementShape shape = analysis::ElementTopology::shapeOf(location->table);
    const std::size_t count = shape == analysis::ElementShape::Triangle        ? 3
                              : shape == analysis::ElementShape::Quadrilateral ? 4
                                                                               : 0;
    if (count == 0 || analysis::ElementTopology::alternatingCorners(location->table))
        return std::nullopt;
    slots.clear();
    analysis::ElementTopology::slotsOf(*model.findTable(location->table), location->table, location->row, slots);
    if (slots.size() < count)
        return std::nullopt;
    Shell shell;
    shell.eid = eid;
    shell.type = location->table;
    shell.count = count;
    for (std::size_t corner = 0; corner < count; ++corner)
    {
        if (slots[corner] == 0 || !positions.has(slots[corner]))
            return std::nullopt;
        shell.nodes[corner] = slots[corner];
        shell.corners[corner] = positions.position(slots[corner]);
        shell.centre += shell.corners[corner] / static_cast<double>(count);
    }
    const base::Vec3 area = count == 3
                                ? base::cross(shell.corners[1] - shell.corners[0], shell.corners[2] - shell.corners[0])
                                : base::cross(shell.corners[2] - shell.corners[0], shell.corners[3] - shell.corners[1]);
    if (!(base::norm(area) > 0.0))
        return std::nullopt;
    shell.normal = base::normalized(area);
    // a PLOAD4 on a warped four-noded quadrilateral acts on its mean plane (not a CQUAD8, whose surface is its own)
    if (count == 4 && slots.size() == 4)
        shell.plane = warpedMeanPlane(shell.corners);
    return shell;
}

// The mean of a pressure given at the corners over the surface of the shell: the integral of the pressure interpolated
// by the shape functions over the area. On a triangle the
// mean of the three corners; on a quadrilateral the 2 x 2 Gauss rule, exact for a flat face (the mean of the corners
// for a parallelogram). On a warped four-noded quadrilateral the area is that of the projection on its mean plane, as
// the loads of the pressure are made (warpedMeanPlane).
double meanOver(const Shell& shell, const std::array<double, 4>& pressures)
{
    if (shell.count == 3)
        return (pressures[0] + pressures[1] + pressures[2]) / 3.0;
    constexpr std::array<double, 4> XI{-1.0, 1.0, 1.0, -1.0};
    constexpr std::array<double, 4> ETA{-1.0, -1.0, 1.0, 1.0};
    const double gauss = 1.0 / std::sqrt(3.0);
    double force = 0.0;
    double area = 0.0;
    for (const double xi : {-gauss, gauss})
        for (const double eta : {-gauss, gauss})
        {
            base::Vec3 alongXi;
            base::Vec3 alongEta;
            double pressure = 0.0;
            for (std::size_t corner = 0; corner < 4; ++corner)
            {
                pressure += 0.25 * (1.0 + XI[corner] * xi) * (1.0 + ETA[corner] * eta) * pressures[corner];
                alongXi += (0.25 * XI[corner] * (1.0 + ETA[corner] * eta)) * shell.corners[corner];
                alongEta += (0.25 * ETA[corner] * (1.0 + XI[corner] * xi)) * shell.corners[corner];
            }
            const base::Vec3 dA = base::cross(alongXi, alongEta);
            const double weight = shell.plane ? base::dot(dA, *shell.plane) : base::norm(dA);
            force += pressure * weight;
            area += weight;
        }
    return force / area;
}

// What the pressure cards of one subcase put on one shell, summed.
struct Loaded
{
    std::array<double, 4> corners{}; // the normal component at the corners
    double mean = 0.0;               // the normal component, mean over the surface
    base::Vec3 tangential;           // the tangential component of the mean, basic system
    std::int32_t cards = 0;
    bool directed = false; // a PLOAD4 along a direction (CID, N1 to N3)
    bool edges = false;    // a PLOAD4 along an edge (SORL = LINE), left out
    void add(const Loaded& other, double factor)
    {
        for (std::size_t corner = 0; corner < 4; ++corner)
            corners[corner] += factor * other.corners[corner];
        mean += factor * other.mean;
        tangential += factor * other.tangential;
        cards += other.cards;
        directed = directed || other.directed;
        edges = edges || other.edges;
    }
};

// The column of the card names of the rows, from the card type of each: a dictionary of the names in alphabetical
// order, as TableBuilder::dictionary makes it from the names (a column of millions of rows from a few types).
QueryColumn typeColumnOf(const std::vector<std::uint16_t>& types)
{
    std::vector<std::uint8_t> present(std::size_t{1} << 16U, 0);
    for (const std::uint16_t type : types)
        present[type] = 1;
    std::vector<std::pair<std::string_view, std::uint16_t>> names;
    for (std::size_t type = 0; type < present.size(); ++type)
        if (present[type] != 0)
            names.emplace_back(typeName(static_cast<std::uint16_t>(type)), static_cast<std::uint16_t>(type));
    std::sort(names.begin(), names.end());
    std::vector<std::int32_t> indexOf(present.size(), 0);
    std::vector<std::string> dictionary;
    for (const auto& [name, type] : names)
    {
        indexOf[type] = static_cast<std::int32_t>(dictionary.size());
        dictionary.emplace_back(name);
    }
    std::vector<std::int32_t> indices(types.size());
    for (std::size_t row = 0; row < types.size(); ++row)
        indices[row] = indexOf[types[row]];
    return QueryColumn::dictionary(std::move(indices), dictionary);
}

// What a subcase puts on one shell: the shell, its position among the shells loaded (in increasing IDs), the sum.
struct ShellPressure
{
    std::int32_t eid = 0;
    std::size_t slot = 0;
    Loaded loaded;
};
// The pressures of a subcase: the shells it loads, in increasing element IDs.
using SubcasePressures = std::vector<ShellPressure>;

// elementPressure: the pressure on each shell in each subcase.
class ElementPressureQuery final : public QueryBase
{
public:
    ElementPressureQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::string by = run.arguments.textOr("by", "subcase");
        if (by != "subcase" && by != "loadSet")
            throw QueryError("elementPressure: by is \"subcase\" or \"loadSet\", not \"" + by + "\"");
        const bool bySet = by == "loadSet";
        if (!bySet && run.arguments.has("setIds"))
            throw QueryError(
                "elementPressure: setIds selects the load sets of by=\"loadSet\"; by subcase, give "
                "subcases");
        const model::Model& model = *run.model;
        const analysis::LoadSetIndex loadSets(model);
        const model::CaseControl& caseControl = model.control().caseControl;
        const model::CaseLineages lineages(caseControl);
        const std::vector<const model::Subcase*> subcases = analysis::analysisSubcases(caseControl);
        const auto askedSubcases = sortedIds(run.arguments.ids("subcases"));
        const auto onlyElements = sortedIds(run.arguments.ids("elementIds"));
        const CardList elementList = listCards(model, typesOfCategory(model::CardCategory::Element));
        const NodePositions positions(model, run.context.pool());
        LoadVectors vectors(model, run.context.pool());
        std::vector<base::EntityId> slots;
        // the shell of each element, made when first asked for, by its position in the list of the elements
        std::vector<std::optional<Shell>> shells(elementList.ids.size());
        std::vector<std::uint8_t> shellMade(elementList.ids.size(), 0);
        static const std::optional<Shell> NO_SHELL;
        // Makes the shells of the elements at these positions of the list (increasing, each once) on the threads of the
        // call.
        const auto makeShells = [&](std::vector<std::size_t> places)
        {
            base::ThreadPool* const pool = run.context.pool();
            const std::size_t chunks = std::max<std::size_t>(
                1, std::min(pool != nullptr ? 4 * (pool->workerCount() + 1) : std::size_t{1}, places.size() / 1024));
            const auto chunk = [&](std::size_t number)
            {
                std::vector<base::EntityId> own;
                for (std::size_t item = number * places.size() / chunks; item < (number + 1) * places.size() / chunks;
                     ++item)
                    shells[places[item]] = shellOf(model, positions, elementList.ids[places[item]], own);
            };
            if (pool != nullptr && chunks > 1)
                base::parallelFor(*pool, chunks, chunk, run.context.token());
            else
                for (std::size_t number = 0; number < chunks; ++number)
                    chunk(number);
            for (const std::size_t at : places)
                shellMade[at] = 1;
        };
        const auto shellAt = [&](std::int32_t eid) -> const std::optional<Shell>&
        {
            const auto found = std::lower_bound(elementList.ids.begin(), elementList.ids.end(), eid);
            if (found == elementList.ids.end() || *found != eid)
                return NO_SHELL; // no element card has this ID
            const auto at = static_cast<std::size_t>(found - elementList.ids.begin());
            if (shellMade[at] == 0)
            {
                shells[at] = shellOf(model, positions, eid, slots);
                shellMade[at] = 1;
            }
            return shells[at];
        };
        // the shells by their sorted corner grid points, for a PLOAD (the lowest element ID of a stack), made once
        std::optional<std::map<std::array<base::EntityId, 4>, std::int32_t>> byCorners;
        const auto shellOnPoints = [&](const std::array<base::EntityId, 4>& points) -> std::optional<std::int32_t>
        {
            if (!byCorners)
            {
                byCorners.emplace();
                for (const std::int64_t id : elementList.ids)
                {
                    const auto eid = static_cast<std::int32_t>(id);
                    const std::optional<Shell>& shell = shellAt(eid);
                    if (!shell)
                        continue;
                    std::array<base::EntityId, 4> key = shell->nodes;
                    std::sort(key.begin(), key.end());
                    byCorners->emplace(key, eid); // the IDs come in increasing order: the first stays
                }
            }
            std::array<base::EntityId, 4> key = points;
            std::sort(key.begin(), key.end());
            const auto found = byCorners->find(key);
            if (found == byCorners->end())
                return std::nullopt;
            return found->second;
        };
        const auto wanted = [&](std::int32_t eid)
        {
            return !onlyElements ||
                   std::binary_search(onlyElements->begin(), onlyElements->end(), static_cast<std::int64_t>(eid));
        };

        // PERFORMANCE: what a pressure card puts on each of its shells (at the scale 1) is made once
        // per card, then each load set adds them with the factors of its LOAD entries, the load sets on the threads of
        // the call. A complete base has thousands of subcases, each a LOAD combination of the same few unit sets: the
        // geometry of a shell (its mean pressure over the surface, its normal) was made again for every subcase. The
        // sums are made in the same order as before (the cards in the order of the set, the shells in the order of a
        // card), so the values are the same to the bit.
        struct Contribution
        {
            std::int32_t eid = 0;
            std::size_t slot = 0;  // the position of the shell among those any card loads (increasing element IDs)
            std::size_t place = 0; // its position in the list of the elements
            Loaded one;
        };
        const auto placeOf = [&](std::int32_t eid)
        {
            return static_cast<std::size_t>(std::lower_bound(elementList.ids.begin(), elementList.ids.end(), eid) -
                                            elementList.ids.begin());
        };
        // Appends what a pressure card puts on each of its shells (at the scale 1) to `result`, in the order of the
        // card. A card along a direction needs the vectors, which keep caches of the systems: without `withVectors`
        // it is left (false returned) for the calling thread. The shells and the map of the corners must be made
        // already to run it on several threads (byCorners, makeShells).
        const auto appendCard = [&](const analysis::ElementaryLoad& load, std::vector<Contribution>& result,
                                    bool withVectors) -> bool
        {
            const std::uint16_t type = load.card.table;
            if (type == model::typeIndexOf<model::PointsPressureLoadTable>())
            {
                const model::PointsPressureLoadCard data =
                    static_cast<const model::PointsPressureLoadTable&>(*model.findTable(type)).card(load.card.row);
                std::array<base::EntityId, 4> points{data.nodes[0], data.nodes[1], data.nodes[2], data.nodes[3]};
                const std::optional<std::int32_t> eid = shellOnPoints(points);
                if (!eid || !wanted(*eid))
                    return true;
                const Shell& shell = *shellAt(*eid);
                // the pressure acts along the normal of the order of the PLOAD's grid points
                const std::size_t count = points[3] == 0 ? 3 : 4;
                const base::Vec3 a = positions.position(points[0]);
                const base::Vec3 normal =
                    count == 3 ? base::cross(positions.position(points[1]) - a, positions.position(points[2]) - a)
                               : base::cross(positions.position(points[2]) - a,
                                             positions.position(points[3]) - positions.position(points[1]));
                const double sense = base::dot(normal, shell.normal) < 0.0 ? -1.0 : 1.0;
                Loaded one;
                one.cards = 1;
                one.mean = sense * data.pressure;
                for (std::size_t corner = 0; corner < shell.count; ++corner)
                    one.corners[corner] = one.mean;
                result.push_back(Contribution{*eid, 0, placeOf(*eid), one});
                return true;
            }
            std::array<double, 4> given{};
            bool line = false;
            std::optional<std::pair<std::int32_t, base::Vec3>> direction;
            if (type == model::typeIndexOf<model::ShellPressureLoadTable>())
            {
                given.fill(static_cast<const model::ShellPressureLoadTable&>(*model.findTable(type))
                               .card(load.card.row)
                               .pressure);
            }
            else
            {
                const model::FacePressureLoadCard data =
                    static_cast<const model::FacePressureLoadTable&>(*model.findTable(type)).card(load.card.row);
                given = data.pressures;
                line = data.kind == model::PressureKind::Line;
                const base::Vec3 vector{data.direction[0], data.direction[1], data.direction[2]};
                if (base::norm(vector) > 0.0)
                    direction = std::make_pair(data.frameId, vector);
            }
            if (direction && !line && !withVectors)
                return false;
            for (const analysis::IdRun& range : load.elements)
            {
                const std::span<const std::int32_t> ids = idsInRun(elementList.ids, range.first, range.last);
                const auto first = static_cast<std::size_t>(ids.data() - elementList.ids.data());
                for (std::size_t item = 0; item < ids.size(); ++item)
                {
                    const std::int32_t eid = ids[item];
                    if (!wanted(eid))
                        continue;
                    const std::optional<Shell>& shell = shellAt(eid);
                    if (!shell)
                        continue; // the face of a solid, or an element that is no shell
                    Loaded one;
                    if (line)
                    {
                        one.edges = true; // a load along an edge, not a pressure on the face
                        result.push_back(Contribution{eid, 0, first + item, one});
                        continue;
                    }
                    one.cards = 1;
                    double cosine = 1.0;
                    base::Vec3 across;
                    if (direction)
                    {
                        // the direction in the basic system at the centre (it turns with a cylindrical system)
                        const base::Vec3 along =
                            base::normalized(vectors.inBasic(direction->first, shell->centre, direction->second));
                        cosine = base::dot(along, shell->normal);
                        across = along - cosine * shell->normal;
                        one.directed = true;
                    }
                    for (std::size_t corner = 0; corner < shell->count; ++corner)
                        one.corners[corner] = cosine * given[corner];
                    const double mean = meanOver(*shell, given);
                    one.mean = cosine * mean;
                    one.tangential = mean * across;
                    result.push_back(Contribution{eid, 0, first + item, one});
                }
            }
            return true;
        };
        // the cards of the subcases, each made once (a set of a LOAD combination is reached by many subcases)
        std::map<std::pair<std::uint16_t, std::uint32_t>, std::vector<Contribution>> byCard;
        const auto contributionsOf = [&](const analysis::ElementaryLoad& load) -> std::vector<Contribution>&
        {
            const auto key = std::make_pair(load.card.table, load.card.row);
            auto found = byCard.find(key);
            if (found != byCard.end())
                return found->second;
            std::vector<Contribution>& result = byCard[key];
            appendCard(load, result, true);
            return result;
        };

        // the contributions of a card (a run of them) and its factor
        struct Term
        {
            const Contribution* first = nullptr;
            std::size_t count = 0;
            double factor = 1.0;
        };
        const auto termOf = [](const std::vector<Contribution>& contributions, double factor)
        { return Term{contributions.data(), contributions.size(), factor}; };
        std::vector<base::EntityId> setIds;
        std::vector<std::vector<Term>> termsOfSet;
        // by load set: the contributions of the cards of each set, one array a set (each card is in its own set)
        std::vector<std::vector<Contribution>> ofSet;
        base::ThreadPool* const threads = run.context.pool();
        if (bySet)
        {
            // a companion package: each load set once, its own cards (a LOAD combination replaced by the sets it
            // reaches, as pressureTable(expandCombinations=False)); the sets of the subcases asked for, or of sids,
            // or every set.
            std::vector<base::EntityId> asked =
                askedSets(model, run.arguments, model::LOAD_SELECTION).value_or(allLoadSets(model));
            const LoadSetListing listing(model, loadSets, std::move(asked), false);
            const std::vector<std::uint16_t> types =
                sortedTypesOf<model::PointsPressureLoadTable, model::ShellPressureLoadTable,
                              model::FacePressureLoadTable>();
            setIds = listing.sets();
            termsOfSet.resize(setIds.size());
            std::vector<std::vector<analysis::ElementaryLoad>> loadsOfSet(setIds.size());
            bool anyPload = false;
            for (std::size_t at = 0; at < setIds.size(); ++at)
            {
                run.context.checkCancelled();
                for (analysis::ElementaryLoad& load : listing.loadsOf(setIds[at], types))
                    if (load.kind == analysis::LoadKind::Pressure)
                    {
                        anyPload = anyPload || load.card.table == model::typeIndexOf<model::PointsPressureLoadTable>();
                        loadsOfSet[at].push_back(std::move(load));
                    }
            }
            // PERFORMANCE (the profile of a fuselage section: 101 sets, 73 M rows of 2.8 M shells, 50 s on one thread):
            // the shells made on the threads of the call, then the sets, each its cards in their order into its own
            // array (the same contributions as on one thread); a set with a card along a direction on this thread.
            std::vector<std::size_t> every;
            for (std::size_t at = 0; at < elementList.ids.size(); ++at)
                if (wanted(elementList.ids[at]))
                    every.push_back(at);
            makeShells(std::move(every));
            if (anyPload)
                (void)shellOnPoints({0, 0, 0, 0}); // the map of the shells by their corners, made once here
            ofSet.resize(setIds.size());
            std::vector<std::vector<std::size_t>> countsOfSet(setIds.size());
            std::vector<std::uint8_t> left(setIds.size(), 0);
            const auto makeSet = [&](std::size_t at, bool withVectors)
            {
                ofSet[at].clear();
                countsOfSet[at].clear();
                for (const analysis::ElementaryLoad& load : loadsOfSet[at])
                {
                    const std::size_t before = ofSet[at].size();
                    if (!appendCard(load, ofSet[at], withVectors))
                        return false;
                    countsOfSet[at].push_back(ofSet[at].size() - before);
                }
                return true;
            };
            const auto parallelSet = [&](std::size_t at) { left[at] = makeSet(at, false) ? 0 : 1; };
            if (threads != nullptr && setIds.size() > 1)
                base::parallelFor(*threads, setIds.size(), parallelSet, run.context.token());
            else
                for (std::size_t at = 0; at < setIds.size(); ++at)
                    parallelSet(at);
            for (std::size_t at = 0; at < setIds.size(); ++at)
                if (left[at] != 0)
                    (void)makeSet(at, true);
            for (std::size_t at = 0; at < setIds.size(); ++at)
            {
                const Contribution* first = ofSet[at].data();
                for (std::size_t card = 0; card < countsOfSet[at].size(); ++card)
                {
                    termsOfSet[at].push_back(Term{first, countsOfSet[at][card], loadsOfSet[at][card].scale});
                    first += countsOfSet[at][card];
                }
            }
        }

        // The subcases whose pressures are needed: those asked for, and those the combinations among them combine
        // (always earlier ones).
        const auto isCombination = [&](std::size_t index)
        {
            return subcases[index]->kind == model::SubcaseKind::SubcaseCombination ||
                   subcases[index]->kind == model::SubcaseKind::SymmetryCombination;
        };
        std::vector<std::uint8_t> needed(subcases.size(), 0);
        std::vector<std::uint8_t> combined(subcases.size(), 0); // combined by a combination that is needed
        for (std::size_t index = bySet ? 0 : subcases.size(); index-- > 0;)
        {
            if (!askedSubcases ||
                std::binary_search(askedSubcases->begin(), askedSubcases->end(), std::int64_t{subcases[index]->id}))
                needed[index] = 1;
            if (needed[index] != 0 && isCombination(index))
                for (const analysis::CombinationTerm& term : analysis::SubcaseCombination::termsOf(subcases, index))
                    if (term.coefficient != 0.0 && term.subcaseIndex < index)
                    {
                        needed[term.subcaseIndex] = 1;
                        combined[term.subcaseIndex] = 1;
                    }
        }

        // The load set of each subcase that has one, and the pressure cards of each set, expanded once (the cards'
        // contributions made here, on the calling thread: the caches of the shells and of the vectors are not shared).
        std::vector<std::optional<base::EntityId>> setOf(subcases.size());
        for (std::size_t index = 0; index < subcases.size(); ++index)
        {
            if (bySet || needed[index] == 0 || isCombination(index))
                continue;
            if (const auto sid = analysis::effectiveInteger(lineages, *subcases[index], "StaticLoadSelection"))
                if (const auto id = base::toEntityId(*sid))
                {
                    setOf[index] = *id;
                    setIds.push_back(*id);
                }
        }
        if (!bySet)
        {
            std::sort(setIds.begin(), setIds.end());
            setIds.erase(std::unique(setIds.begin(), setIds.end()), setIds.end());
            termsOfSet.resize(setIds.size());
            for (std::size_t at = 0; at < setIds.size(); ++at)
            {
                run.context.checkCancelled();
                for (const analysis::ElementaryLoad& load : analysis::LoadSets::expand(model, loadSets, setIds[at]))
                    if (load.kind == analysis::LoadKind::Pressure)
                        termsOfSet[at].push_back(termOf(contributionsOf(load), load.scale));
            }
        }
        // the shells any card loads, in increasing element IDs: the sums of a subcase are made in an array of them
        // (PERFORMANCE: by the places of the shells in the list of the elements, sorted by ID, rather than a sort of
        // the 73 M contributions of a profile and a search for each)
        std::vector<std::uint32_t> slotOfPlace(elementList.ids.size(), 0);
        const auto forEachContribution = [&](const auto& body)
        {
            for (auto& [key, contributions] : byCard)
                for (Contribution& contribution : contributions)
                    body(contribution);
            for (std::vector<Contribution>& contributions : ofSet)
                for (Contribution& contribution : contributions)
                    body(contribution);
        };
        forEachContribution([&](const Contribution& contribution) { slotOfPlace[contribution.place] = 1; });
        std::vector<std::int32_t> loadedIds;
        for (std::size_t place = 0; place < slotOfPlace.size(); ++place)
            if (slotOfPlace[place] != 0)
            {
                slotOfPlace[place] = static_cast<std::uint32_t>(loadedIds.size());
                loadedIds.push_back(elementList.ids[place]);
            }
        forEachContribution([&](Contribution& contribution) { contribution.slot = slotOfPlace[contribution.place]; });
        const auto setIndexOf = [&](base::EntityId sid)
        { return static_cast<std::size_t>(std::lower_bound(setIds.begin(), setIds.end(), sid) - setIds.begin()); };

        // The sums of a subcase, by shell (an array of the loaded shells, made once a chunk of the work and left at
        // zero after each subcase).
        struct Dense
        {
            std::vector<Loaded> values;
            std::vector<std::uint8_t> reached;
        };
        const auto newDense = [&]
        { return Dense{std::vector<Loaded>(loadedIds.size()), std::vector<std::uint8_t>(loadedIds.size(), 0)}; };
        const auto sumSet = [&](std::size_t set, Dense& dense)
        {
            for (const Term& term : termsOfSet[set])
                for (const Contribution& contribution : std::span<const Contribution>(term.first, term.count))
                {
                    dense.values[contribution.slot].add(contribution.one, term.factor);
                    dense.reached[contribution.slot] = 1;
                }
        };
        const auto collect = [&](Dense& dense, SubcasePressures& out)
        {
            out.clear();
            for (std::size_t slot = 0; slot < loadedIds.size(); ++slot)
                if (dense.reached[slot] != 0)
                {
                    out.push_back(ShellPressure{loadedIds[slot], slot, dense.values[slot]});
                    dense.values[slot] = Loaded{};
                    dense.reached[slot] = 0;
                }
        };
        base::ThreadPool* pool = threads;
        // Runs `body(item, dense)` over `count` items in a few chunks a thread, each chunk with one array of sums.
        const auto inChunks = [&](std::size_t count, const auto& body)
        {
            const std::size_t chunks =
                std::min(count, pool != nullptr ? 4 * (pool->workerCount() + 1) : std::size_t{1});
            const auto chunk = [&](std::size_t number)
            {
                Dense dense = newDense();
                for (std::size_t item = number * count / chunks; item < (number + 1) * count / chunks; ++item)
                    body(item, dense);
            };
            if (pool != nullptr && chunks > 1)
                base::parallelFor(*pool, chunks, chunk, run.context.token());
            else
                for (std::size_t number = 0; number < chunks; ++number)
                    chunk(number);
        };

        // The shells each set loads (positions in the loaded shells, increasing): the rows of its subcases.
        std::vector<std::vector<std::uint32_t>> slotsOfSet(setIds.size());
        inChunks(setIds.size(),
                 [&](std::size_t set, Dense& dense)
                 {
                     for (const Term& term : termsOfSet[set])
                         for (const Contribution& contribution : std::span<const Contribution>(term.first, term.count))
                             dense.reached[contribution.slot] = 1;
                     for (std::size_t slot = 0; slot < loadedIds.size(); ++slot)
                         if (dense.reached[slot] != 0)
                         {
                             slotsOfSet[set].push_back(static_cast<std::uint32_t>(slot));
                             dense.reached[slot] = 0;
                         }
                 });

        // The pressures of the subcases a combination combines (the combinations themselves in order: one may combine
        // another), as lists; the other subcases are summed when their rows are made.
        static const SubcasePressures NONE;
        std::vector<SubcasePressures> held(subcases.size());
        std::vector<const SubcasePressures*> ofSubcase(subcases.size(), &NONE);
        {
            std::vector<std::size_t> plain; // the plain subcases a combination combines
            for (std::size_t index = 0; index < subcases.size(); ++index)
                if (combined[index] != 0 && !isCombination(index) && setOf[index])
                    plain.push_back(index);
            inChunks(plain.size(),
                     [&](std::size_t at, Dense& dense)
                     {
                         const std::size_t index = plain[at];
                         sumSet(setIndexOf(*setOf[index]), dense);
                         collect(dense, held[index]);
                     });
            for (const std::size_t index : plain)
                ofSubcase[index] = &held[index];
            Dense dense = newDense();
            for (std::size_t index = 0; index < subcases.size(); ++index)
            {
                run.context.checkCancelled();
                if (needed[index] == 0 || !isCombination(index))
                    continue;
                for (const analysis::CombinationTerm& term : analysis::SubcaseCombination::termsOf(subcases, index))
                {
                    if (term.coefficient == 0.0 || term.subcaseIndex >= index)
                        continue;
                    for (const ShellPressure& entry : *ofSubcase[term.subcaseIndex])
                    {
                        dense.values[entry.slot].add(entry.loaded, term.coefficient);
                        dense.reached[entry.slot] = 1;
                    }
                }
                collect(dense, held[index]);
                ofSubcase[index] = &held[index];
            }
        }

        // the rows: the subcases asked for in increasing IDs, the shells of each in increasing IDs, made on the
        // threads of the call
        std::vector<const Shell*> shellOf(loadedIds.size());
        for (std::size_t slot = 0; slot < loadedIds.size(); ++slot)
            shellOf[slot] = &*shellAt(loadedIds[slot]);
        std::vector<std::size_t> order(subcases.size());
        for (std::size_t index = 0; index < order.size(); ++index)
            order[index] = index;
        std::stable_sort(order.begin(), order.end(),
                         [&](std::size_t left, std::size_t right) { return subcases[left]->id < subcases[right]->id; });
        std::vector<std::size_t> kept;   // the subcases given, in order (by load set: the sets)
        std::vector<std::size_t> offset; // the first row of each
        std::size_t rowCount = 0;
        if (bySet)
        {
            for (std::size_t set = 0; set < setIds.size(); ++set)
            {
                kept.push_back(set);
                offset.push_back(rowCount);
                rowCount += slotsOfSet[set].size();
            }
        }
        const auto rowsOf = [&](std::size_t index) -> std::size_t
        {
            if (isCombination(index) || combined[index] != 0)
                return ofSubcase[index]->size();
            return setOf[index] ? slotsOfSet[setIndexOf(*setOf[index])].size() : 0;
        };
        for (const std::size_t index : bySet ? std::vector<std::size_t>{} : order)
        {
            const std::int64_t number = subcases[index]->id;
            if (askedSubcases && !std::binary_search(askedSubcases->begin(), askedSubcases->end(), number))
                continue;
            kept.push_back(index);
            offset.push_back(rowCount);
            rowCount += rowsOf(index);
        }
        std::vector<std::int32_t> subcaseColumn(rowCount);
        std::vector<std::int32_t> eid(rowCount);
        std::vector<std::uint16_t> typeOfRow(rowCount);
        std::vector<double> mean(rowCount);
        std::array<std::vector<double>, 4> corners;
        for (std::vector<double>& column : corners)
            column.resize(rowCount);
        std::array<std::vector<double>, 3> tangential;
        for (std::vector<double>& column : tangential)
            column.resize(rowCount);
        std::vector<std::int32_t> cards(rowCount);
        std::vector<std::uint8_t> directed(rowCount);
        std::vector<std::uint8_t> edges(rowCount);
        const auto putRow = [&](std::size_t row, std::int32_t number, std::size_t slot, const Loaded& loaded)
        {
            const Shell& shell = *shellOf[slot];
            subcaseColumn[row] = number;
            eid[row] = loadedIds[slot];
            typeOfRow[row] = shell.type;
            mean[row] = loaded.mean;
            for (std::size_t corner = 0; corner < 4; ++corner)
                corners[corner][row] = corner < shell.count ? loaded.corners[corner] : NOT_A_NUMBER;
            tangential[0][row] = loaded.tangential.x;
            tangential[1][row] = loaded.tangential.y;
            tangential[2][row] = loaded.tangential.z;
            cards[row] = loaded.cards;
            directed[row] = loaded.directed ? 1 : 0;
            edges[row] = loaded.edges ? 1 : 0;
        };
        inChunks(kept.size(),
                 [&](std::size_t at, Dense& dense)
                 {
                     std::size_t row = offset[at];
                     if (bySet)
                     {
                         // a load set: its own cards summed, its rows made from the sums
                         const std::size_t set = kept[at];
                         sumSet(set, dense);
                         for (const std::uint32_t slot : slotsOfSet[set])
                         {
                             putRow(row++, setIds[set], slot, dense.values[slot]);
                             dense.values[slot] = Loaded{};
                             dense.reached[slot] = 0;
                         }
                         return;
                     }
                     const std::size_t index = kept[at];
                     const auto number = static_cast<std::int32_t>(subcases[index]->id);
                     if (isCombination(index) || combined[index] != 0)
                     {
                         for (const ShellPressure& entry : *ofSubcase[index])
                             putRow(row++, number, entry.slot, entry.loaded);
                         return;
                     }
                     if (!setOf[index])
                         return;
                     // a plain subcase: its set summed here, its rows made from the sums
                     const std::size_t set = setIndexOf(*setOf[index]);
                     sumSet(set, dense);
                     for (const std::uint32_t slot : slotsOfSet[set])
                     {
                         putRow(row++, number, slot, dense.values[slot]);
                         dense.values[slot] = Loaded{};
                         dense.reached[slot] = 0;
                     }
                 });
        TableBuilder builder;
        builder.int32(bySet ? "loadSet" : "subcase", std::move(subcaseColumn));
        builder.int32("elementId", std::move(eid));
        builder.column("type", typeColumnOf(typeOfRow));
        builder.real("pressure", std::move(mean));
        static const char* const NAMES[4] = {"p1", "p2", "p3", "p4"};
        for (std::size_t corner = 0; corner < 4; ++corner)
            builder.real(NAMES[corner], std::move(corners[corner]));
        builder.real("tangentialX", std::move(tangential[0]));
        builder.real("tangentialY", std::move(tangential[1]));
        builder.real("tangentialZ", std::move(tangential[2]));
        builder.int32("cardCount", std::move(cards));
        builder.flag("directed", directed);
        builder.flag("edgeLoads", edges);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "elementPressure";
        info.family = "T8";
        info.description =
            "The pressure on each shell in each subcase: one row per subcase and shell loaded, the "
            "sum of every PLOAD, PLOAD2 and PLOAD4 that loads its face in the load set of the subcase, the factors of "
            "the LOAD entries applied; a combination subcase (SUBCOM with SUBSEQ or SUBSEQ1, SYMCOM) gives the sum of "
            "the pressures of the subcases it combines times their coefficients, so the pressure of a combined case "
            "is the combination of those of its unit subcases. Positive along the normal of the shell (the sense of "
            "the order of its grid points, as PLOAD2 and PLOAD4 load it). `p1` to `p4` are the normal components at "
            "the corners of the card (`p4` null on a triangle), `pressure` their mean over the surface (the integral "
            "of the pressure interpolated over the face, over its area: the mean of the corners on a parallelogram or "
            "a triangle; over the projection of the face on its mean plane for a warped CQUAD4 or CQUADR, on which a "
            "PLOAD4 acts). A PLOAD4 along a direction (CID, N1 to N3) gives its normal component, the pressure times "
            "the cosine between the direction (at the centre of the shell, in the basic system) and the normal, in "
            "these columns, and its tangential component, the mean pressure times the part of the unit direction in "
            "the plane, in `tangentialX/y/z` (basic system); `directed` says a shell has one. A PLOAD loads the "
            "shell whose corners are its grid points (the lowest element ID of a stack), with the sign of its order "
            "against the normal of the shell; a PLOAD on no shell is left out. A PLOAD4 along an edge (SORL = LINE) "
            "is no pressure on the face: left out, `edgeLoads` says so. `cardCount` the cards summed. The faces of "
            "solids are not listed. Linear in the cards. by=\"loadSet\": one row per load set and "
            "shell instead, the sum of the set's own cards (loadSet in place of subcase; a LOAD combination replaced "
            "by the sets it reaches, as pressureTable(expandCombinations=False)): the sets of the subcases asked "
            "for (their LOAD; a SUBCOM has none), of setIds, or every set; the pressure of a subcase is then the sum "
            "over "
            "the terms of its LOAD in loadCombinationsTable of combinedFactor times the rows of the set (within a "
            "rounding of the sum by subcase; to the bit for a subcase whose LOAD is the set itself).";
        info.detailParameter = "by";
        info.parameters = {parameter("subcases", ParameterKind::IdList, "None", "the subcases (all by default)"),
                           parameter("elementIds", ParameterKind::IdList, "None", "keep these elements"),
                           parameter("by", ParameterKind::Text, "\"subcase\"",
                                     "\"subcase\" (a row per subcase and shell) or \"loadSet\" (a row per load set and "
                                     "shell, the set's own cards)"),
                           parameter("setIds", ParameterKind::IdList, "None",
                                     "by=\"loadSet\": the load sets (a LOAD combination: the sets it reaches)")};
        info.defaultSort = "subcase, elementId";
        ColumnListBuilder columns;
        columns.int32("subcase").int32("elementId").dictionary("type").reals("pressure p1 p2 p3 p4");
        columns.vector3("tangential", "XYZ").int32("cardCount").flag("directed").flag("edgeLoads");
        info.columnSets.push_back(columns.build());
        ColumnListBuilder bySets;
        bySets.int32("loadSet").int32("elementId").dictionary("type").reals("pressure p1 p2 p3 p4");
        bySets.vector3("tangential", "XYZ").int32("cardCount").flag("directed").flag("edgeLoads");
        info.columnSets.push_back(bySets.build("loadSet"));
        return info;
    }
};

} // namespace

void addElementPressureQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ElementPressureQuery>());
}

} // namespace oor::query
