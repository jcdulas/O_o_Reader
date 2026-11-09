#include "oor/analysis/components/LoadSets.hpp"

#include "oor/model/cards/constraints/enforcedMotionConstr/EnforcedMotionConstrTable.hpp"
#include "oor/model/cards/loads/accelFieldLoad/AccelFieldLoadTable.hpp"
#include "oor/model/cards/loads/accelNodeLoad/AccelNodeLoadTable.hpp"
#include "oor/model/cards/loads/axialDeformationLoad/AxialDeformationLoadTable.hpp"
#include "oor/model/cards/loads/beamDistributedLoad/BeamDistributedLoadTable.hpp"
#include "oor/model/cards/loads/defaultTemperatureLoad/DefaultTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/facePressureLoad/FacePressureLoadTable.hpp"
#include "oor/model/cards/loads/gravityLoad/GravityLoadTable.hpp"
#include "oor/model/cards/loads/lineElemTemperatureLoad/LineElemTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/loadCombination/LoadCombinationTable.hpp"
#include "oor/model/cards/loads/nodeTemperatureLoad/NodeTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/plateTemperatureLoad/PlateTemperatureLoadTable.hpp"
#include "oor/model/cards/loads/pointForceLoad/PointForceLoadTable.hpp"
#include "oor/model/cards/loads/pointForceLoad1/PointForceLoad1Table.hpp"
#include "oor/model/cards/loads/pointForceLoad2/PointForceLoad2Table.hpp"
#include "oor/model/cards/loads/pointMomentLoad/PointMomentLoadTable.hpp"
#include "oor/model/cards/loads/pointMomentLoad1/PointMomentLoad1Table.hpp"
#include "oor/model/cards/loads/pointMomentLoad2/PointMomentLoad2Table.hpp"
#include "oor/model/cards/loads/pointsPressureLoad/PointsPressureLoadTable.hpp"
#include "oor/model/cards/loads/rotationalForceLoad/RotationalForceLoadTable.hpp"
#include "oor/model/cards/loads/scalarPointLoad/ScalarPointLoadTable.hpp"
#include "oor/model/cards/loads/shellPressureLoad/ShellPressureLoadTable.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/core/CardRegistry.hpp"
#include "oor/model/references/ReferenceChecker.hpp"

#include <algorithm>
#include <map>
#include <optional>

namespace oor::analysis
{

namespace
{

constexpr int MAX_DEPTH = 8; // a LOAD of LOADs deeper than this is a loop or a mistake

template <typename TableT>
std::uint16_t typeOf()
{
    return model::typeIndexOf<TableT>();
}

void addNodes(ElementaryLoad& load, std::initializer_list<base::EntityId> nodes)
{
    for (const base::EntityId node : nodes)
    {
        if (node != 0)
            load.nodes.push_back(node);
    }
}

// The grid points of an ACCEL1 run that the model has: a single ID as it is, a range ID by ID while it is short, else
// through the GRID cards (so that a range over a wide span of IDs costs the number of grid points, not the span).
void addGridsInRun(const model::Model& model, const model::ContactRun& run, std::vector<base::EntityId>& nodes)
{
    constexpr std::int64_t LONG_SPAN = 65536;
    if (!run.isRange())
    {
        nodes.push_back(run.first);
        return;
    }
    const std::int64_t step = std::max<std::int64_t>(run.increment, 1);
    const std::int64_t first = std::min<std::int64_t>(run.first, run.last);
    const std::int64_t last = std::max<std::int64_t>(run.first, run.last);
    const auto inRun = [&](std::int64_t id) { return id >= first && id <= last && (id - first) % step == 0; };
    if (last - first < LONG_SPAN)
    {
        for (std::int64_t id = first; id <= last; id += step)
        {
            if (model.locate(base::IdSpace::Node, static_cast<base::EntityId>(id)))
                nodes.push_back(static_cast<base::EntityId>(id));
        }
        return;
    }
    const model::NodeTable* grids = model.findTable<model::NodeTable>();
    if (grids == nullptr)
        return;
    std::vector<base::EntityId> found;
    for (std::uint32_t row = 0; row < grids->rows().rowCount(); ++row)
    {
        if (grids->rows().isAlive(row) && inRun(grids->idAtRow(row)))
            found.push_back(grids->idAtRow(row));
    }
    std::sort(found.begin(), found.end());
    nodes.insert(nodes.end(), found.begin(), found.end());
}

// The load of one card (not a LOAD): kind, nodes, elements.
bool describe(const model::Model& model, model::CardLocation location, ElementaryLoad& load)
{
    const std::uint16_t type = location.table;
    const model::CardTable& table = *model.findTable(type);
    const std::uint32_t row = location.row;
    if (type == typeOf<model::PointForceLoadTable>())
    {
        load.kind = LoadKind::Force;
        addNodes(load, {static_cast<const model::PointForceLoadTable&>(table).card(row).node});
    }
    else if (type == typeOf<model::PointForceLoad1Table>())
    {
        load.kind = LoadKind::Force;
        addNodes(load, {static_cast<const model::PointForceLoad1Table&>(table).card(row).node});
    }
    else if (type == typeOf<model::PointForceLoad2Table>())
    {
        load.kind = LoadKind::Force;
        addNodes(load, {static_cast<const model::PointForceLoad2Table&>(table).card(row).node});
    }
    else if (type == typeOf<model::PointMomentLoadTable>())
    {
        load.kind = LoadKind::Moment;
        addNodes(load, {static_cast<const model::PointMomentLoadTable&>(table).card(row).node});
    }
    else if (type == typeOf<model::PointMomentLoad1Table>())
    {
        load.kind = LoadKind::Moment;
        addNodes(load, {static_cast<const model::PointMomentLoad1Table&>(table).card(row).node});
    }
    else if (type == typeOf<model::PointMomentLoad2Table>())
    {
        load.kind = LoadKind::Moment;
        addNodes(load, {static_cast<const model::PointMomentLoad2Table&>(table).card(row).node});
    }
    else if (type == typeOf<model::ScalarPointLoadTable>())
    {
        load.kind = LoadKind::ScalarLoad;
        for (const model::PointLoad& point : static_cast<const model::ScalarPointLoadTable&>(table).card(row).loads)
            addNodes(load, {point.node});
    }
    else if (type == typeOf<model::EnforcedMotionConstrTable>())
    {
        load.kind = LoadKind::EnforcedMotion;
        const model::EnforcedMotionConstrCard card =
            static_cast<const model::EnforcedMotionConstrTable&>(table).card(row);
        addNodes(load, {card.nodes[0], card.nodes[1]});
    }
    else if (type == typeOf<model::PointsPressureLoadTable>())
    {
        load.kind = LoadKind::Pressure;
        const model::PointsPressureLoadCard card = static_cast<const model::PointsPressureLoadTable&>(table).card(row);
        addNodes(load, {card.nodes[0], card.nodes[1], card.nodes[2], card.nodes[3]});
    }
    else if (type == typeOf<model::BeamDistributedLoadTable>())
    {
        load.kind = LoadKind::LineLoad;
        const base::EntityId eid = static_cast<const model::BeamDistributedLoadTable&>(table).card(row).elemId;
        load.elements.push_back(IdRun{eid, eid});
    }
    else if (type == typeOf<model::ShellPressureLoadTable>())
    {
        load.kind = LoadKind::Pressure;
        for (const std::array<base::EntityId, 2>& run :
             static_cast<const model::ShellPressureLoadTable&>(table).runsAt(row))
            load.elements.push_back(IdRun{run[0], run[1]});
    }
    else if (type == typeOf<model::FacePressureLoadTable>())
    {
        load.kind = LoadKind::Pressure;
        const model::FacePressureLoadCard card = static_cast<const model::FacePressureLoadTable&>(table).card(row);
        load.elements.push_back(IdRun{card.elemId, card.elemId2 != 0 ? card.elemId2 : card.elemId});
    }
    else if (type == typeOf<model::GravityLoadTable>())
    {
        load.kind = LoadKind::Gravity;
    }
    else if (type == typeOf<model::AccelFieldLoadTable>())
    {
        load.kind = LoadKind::Acceleration;
    }
    else if (type == typeOf<model::AccelNodeLoadTable>())
    {
        load.kind = LoadKind::PointAcceleration;
        // format reference ACCEL1: the grid points between the ends of a THRU need not exist; the load acts on those
        // that do.
        for (const model::ContactRun& run : static_cast<const model::AccelNodeLoadTable&>(table).card(row).grids)
            addGridsInRun(model, run, load.nodes);
    }
    else if (type == typeOf<model::RotationalForceLoadTable>())
    {
        load.kind = LoadKind::Rotation;
    }
    else if (type == typeOf<model::NodeTemperatureLoadTable>())
    {
        load.kind = LoadKind::NodeTemperature;
        for (const model::NodeTemperature& point :
             static_cast<const model::NodeTemperatureLoadTable&>(table).card(row).points)
            addNodes(load, {point.node});
    }
    else if (type == typeOf<model::DefaultTemperatureLoadTable>())
    {
        load.kind = LoadKind::DefaultTemperature;
    }
    else if (type == typeOf<model::PlateTemperatureLoadTable>())
    {
        load.kind = LoadKind::ElementTemperature;
        for (const model::ElementRun& run :
             static_cast<const model::PlateTemperatureLoadTable&>(table).card(row).elements)
            load.elements.push_back(IdRun{run.first, run.last});
    }
    else if (type == typeOf<model::LineElemTemperatureLoadTable>())
    {
        load.kind = LoadKind::ElementTemperature;
        for (const model::ElementRun& run :
             static_cast<const model::LineElemTemperatureLoadTable&>(table).card(row).elements)
            load.elements.push_back(IdRun{run.first, run.last});
    }
    else if (type == typeOf<model::AxialDeformationLoadTable>())
    {
        load.kind = LoadKind::Deformation;
        for (const model::ElementDeformation& item :
             static_cast<const model::AxialDeformationLoadTable&>(table).card(row).deformations)
            load.elements.push_back(IdRun{item.element, item.element});
    }
    else
    {
        load.kind = LoadKind::Other;
        return false;
    }
    return true;
}

// `index` null: the cards of a set are looked up in the tables of the model.
void expandInto(const model::Model& model, const LoadSetIndex* index, base::EntityId sid, double scale, int depth,
                std::vector<ElementaryLoad>& out)
{
    if (depth > MAX_DEPTH)
        return;
    const std::uint16_t loadType = typeOf<model::LoadCombinationTable>();
    const std::vector<model::CardLocation> found = index == nullptr
                                                       ? model::cardsOfSet(model, model::CardCategory::Load, sid)
                                                       : std::vector<model::CardLocation>{};
    for (const model::CardLocation location : index == nullptr ? found : index->cardsOf(sid))
    {
        if (location.table == loadType)
        {
            const model::LoadCombinationCard card =
                static_cast<const model::LoadCombinationTable&>(*model.findTable(loadType)).card(location.row);
            for (const model::LoadCombinationTerm& term : card.terms)
                expandInto(model, index, term.loadCombinationId, scale * card.scale * term.scale, depth + 1, out);
            continue;
        }
        ElementaryLoad load;
        load.card = location;
        load.sourceSid = sid;
        load.scale = scale;
        if (describe(model, location, load))
            out.push_back(std::move(load));
    }
}

// The first LOAD entry of the set `sid`, if it is a combination.
std::optional<model::LoadCombinationCard> loadEntryOf(const model::Model& model, const LoadSetIndex& index,
                                                      base::EntityId sid)
{
    const std::vector<model::CardLocation>& entries = index.loadEntriesOf(sid);
    if (entries.empty())
        return std::nullopt;
    const std::uint16_t loadType = typeOf<model::LoadCombinationTable>();
    return static_cast<const model::LoadCombinationTable&>(*model.findTable(loadType)).card(entries.front().row);
}

// The sets the LOAD entry `card` reaches (LoadSets::reachedSets), `factor` and `scale` those it was reached with (the S
// of the first entry not in `factor`); `path`: the LOAD entries on the way.
void reachInto(const model::Model& model, const LoadSetIndex& index, const model::LoadCombinationCard& card,
               double factor, double scale, std::vector<base::EntityId>& path, std::vector<ReachedSet>& out)
{
    path.push_back(card.id);
    for (const model::LoadCombinationTerm& term : card.terms)
    {
        // every factor is in `scale`, in the order of expandInto
        const double termScale = scale * card.scale * term.scale;
        if (const std::optional<model::LoadCombinationCard> nested = loadEntryOf(model, index, term.loadCombinationId))
        {
            if (std::find(path.begin(), path.end(), term.loadCombinationId) == path.end())
                reachInto(model, index, *nested, factor * term.scale * nested->scale, termScale, path, out);
            continue;
        }
        out.push_back(ReachedSet{term.loadCombinationId, factor * term.scale, termScale});
    }
    path.pop_back();
}

// The sets expandInto meets from `sid` with the scale `scale` at the depth `depth` (LoadSets::expansionSets).
void expansionInto(const model::Model& model, const LoadSetIndex& index, base::EntityId sid, double scale, int depth,
                   std::vector<ReachedSet>& out)
{
    if (depth > MAX_DEPTH)
        return;
    const std::uint16_t loadType = typeOf<model::LoadCombinationTable>();
    for (const model::CardLocation location : index.loadEntriesOf(sid))
    {
        const model::LoadCombinationCard card =
            static_cast<const model::LoadCombinationTable&>(*model.findTable(loadType)).card(location.row);
        for (const model::LoadCombinationTerm& term : card.terms)
            expansionInto(model, index, term.loadCombinationId, scale * card.scale * term.scale, depth + 1, out);
    }
    if (index.hasOwnCards(sid))
        out.push_back(ReachedSet{sid, scale, scale});
}

} // namespace

LoadSetIndex::LoadSetIndex(const model::Model& model)
{
    const model::CardRegistry& registry = model::CardRegistry::instance();
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        const model::CardTable* table = model.findTable(type);
        if (table == nullptr || registry.entry(type).info.category != model::CardCategory::Load ||
            registry.entry(type).info.idSpace == base::IdSpace::None)
            continue;
        const bool isLoad = type == typeOf<model::LoadCombinationTable>();
        bool any = false;
        std::optional<base::EntityId> previous; // the cards of a set mostly follow one another
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            const base::EntityId sid = table->idAt(row);
            m_sets[sid].push_back(model::CardLocation{type, row});
            if (isLoad)
                m_loadEntries[sid].push_back(model::CardLocation{type, row});
            else if (previous != sid)
                m_withOwnCards.insert(sid);
            previous = sid;
            any = true;
        }
        if (any)
            m_types.push_back(type);
    }
}

const std::vector<model::CardLocation>& LoadSetIndex::cardsOf(base::EntityId sid) const
{
    static const std::vector<model::CardLocation> NONE;
    const auto found = m_sets.find(sid);
    return found != m_sets.end() ? found->second : NONE;
}

const std::vector<model::CardLocation>& LoadSetIndex::loadEntriesOf(base::EntityId sid) const
{
    static const std::vector<model::CardLocation> NONE;
    const auto found = m_loadEntries.find(sid);
    return found != m_loadEntries.end() ? found->second : NONE;
}

bool LoadSetIndex::hasOwnCards(base::EntityId sid) const
{
    return m_withOwnCards.contains(sid);
}

bool LoadSetIndex::hasType(std::uint16_t typeIndex) const noexcept
{
    return std::binary_search(m_types.begin(), m_types.end(), typeIndex);
}

std::vector<ElementaryLoad> LoadSets::expand(const model::Model& model, base::EntityId sid, double scale)
{
    std::vector<ElementaryLoad> loads;
    expandInto(model, nullptr, sid, scale, 0, loads);
    return loads;
}

std::vector<ElementaryLoad> LoadSets::expand(const model::Model& model, const LoadSetIndex& index, base::EntityId sid,
                                             double scale)
{
    std::vector<ElementaryLoad> loads;
    expandInto(model, &index, sid, scale, 0, loads);
    return loads;
}

std::vector<ElementaryLoad> LoadSets::ownLoads(const model::Model& model, const LoadSetIndex& index, base::EntityId sid,
                                               std::span<const std::uint16_t> types)
{
    const std::uint16_t loadType = typeOf<model::LoadCombinationTable>();
    std::vector<ElementaryLoad> loads;
    loads.reserve(index.cardsOf(sid).size());
    for (const model::CardLocation location : index.cardsOf(sid))
    {
        if (location.table == loadType ||
            (!types.empty() && !std::binary_search(types.begin(), types.end(), location.table)))
            continue;
        ElementaryLoad load;
        load.card = location;
        load.sourceSid = sid;
        if (describe(model, location, load))
            loads.push_back(std::move(load));
    }
    return loads;
}

std::vector<ReachedSet> LoadSets::reachedSets(const model::Model& model, const LoadSetIndex& index, base::EntityId sid)
{
    std::vector<ReachedSet> sets;
    if (const std::optional<model::LoadCombinationCard> card = loadEntryOf(model, index, sid))
    {
        std::vector<base::EntityId> path;
        reachInto(model, index, *card, 1.0, 1.0, path, sets);
    }
    else if (!index.cardsOf(sid).empty())
        sets.push_back(ReachedSet{sid, 1.0, 1.0});
    return sets;
}

std::vector<ReachedSet> LoadSets::expansionSets(const model::Model& model, const LoadSetIndex& index,
                                                base::EntityId sid)
{
    std::vector<ReachedSet> sets;
    expansionInto(model, index, sid, 1.0, 0, sets);
    return sets;
}

LoadKind LoadSets::kindOfType(std::uint16_t typeIndex)
{
    static const std::map<std::uint16_t, LoadKind> KINDS = []
    {
        std::map<std::uint16_t, LoadKind> kinds;
        kinds[typeOf<model::PointForceLoadTable>()] = LoadKind::Force;
        kinds[typeOf<model::PointForceLoad1Table>()] = LoadKind::Force;
        kinds[typeOf<model::PointForceLoad2Table>()] = LoadKind::Force;
        kinds[typeOf<model::PointMomentLoadTable>()] = LoadKind::Moment;
        kinds[typeOf<model::PointMomentLoad1Table>()] = LoadKind::Moment;
        kinds[typeOf<model::PointMomentLoad2Table>()] = LoadKind::Moment;
        kinds[typeOf<model::ScalarPointLoadTable>()] = LoadKind::ScalarLoad;
        kinds[typeOf<model::EnforcedMotionConstrTable>()] = LoadKind::EnforcedMotion;
        kinds[typeOf<model::PointsPressureLoadTable>()] = LoadKind::Pressure;
        kinds[typeOf<model::BeamDistributedLoadTable>()] = LoadKind::LineLoad;
        kinds[typeOf<model::ShellPressureLoadTable>()] = LoadKind::Pressure;
        kinds[typeOf<model::FacePressureLoadTable>()] = LoadKind::Pressure;
        kinds[typeOf<model::GravityLoadTable>()] = LoadKind::Gravity;
        kinds[typeOf<model::AccelFieldLoadTable>()] = LoadKind::Acceleration;
        kinds[typeOf<model::AccelNodeLoadTable>()] = LoadKind::PointAcceleration;
        kinds[typeOf<model::RotationalForceLoadTable>()] = LoadKind::Rotation;
        kinds[typeOf<model::NodeTemperatureLoadTable>()] = LoadKind::NodeTemperature;
        kinds[typeOf<model::DefaultTemperatureLoadTable>()] = LoadKind::DefaultTemperature;
        kinds[typeOf<model::PlateTemperatureLoadTable>()] = LoadKind::ElementTemperature;
        kinds[typeOf<model::LineElemTemperatureLoadTable>()] = LoadKind::ElementTemperature;
        kinds[typeOf<model::AxialDeformationLoadTable>()] = LoadKind::Deformation;
        return kinds;
    }();
    const auto found = KINDS.find(typeIndex);
    return found == KINDS.end() ? LoadKind::Other : found->second;
}

} // namespace oor::analysis
