#include "EntityFrames.hpp"

#include "FrameSupport.hpp"
#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/analysis/frames/FrameMath.hpp"
#include "oor/analysis/frames/NodeDisplacementFrames.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/analysis/mass/Laminate.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <numbers>
#include <tuple>
#include <unordered_map>
#include <utility>

namespace oor::analysis::detail
{

using results::StoredFrame;

namespace
{

constexpr double NAN_VALUE = std::numeric_limits<double>::quiet_NaN();

base::Mat3 rotationAboutZ(double angle) noexcept
{
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);
    return base::Mat3{{cosine, -sine, 0.0, sine, cosine, 0.0, 0.0, 0.0, 1.0}};
}

base::Vec3 nanPoint() noexcept
{
    return base::Vec3{NAN_VALUE, NAN_VALUE, NAN_VALUE};
}

// The geometry the frames of the entities are made of.
struct Geometry
{
    std::vector<base::Mat3> nodeOutput; // grid points: the CD system
    std::vector<base::Mat3> element;
    std::vector<base::Mat3> material;
};

struct SystemFrame
{
    CoordinateFrame frame;
    bool rectangular = true;
};

// A system of the model as a role asks for it: its own kind, or read as cylindrical or spherical about its axes.
SystemFrame systemFrameOf(const model::Model& model, const FrameRole& role, base::ThreadPool* pool)
{
    BasicPositionCache& cache = BasicPositionCache::of(model, pool);
    const std::lock_guard<std::mutex> lock(cache.mutex());
    if (!cache.transform().hasSystem(role.system))
        throw base::UsageError("the model has no coordinate system " + std::to_string(role.system));
    SystemFrame result;
    result.frame = cache.transform().frame(role.system);
    if (role.kind == StoredFrame::Cylindrical)
        result.frame.kind = CoordinateKind::Cylindrical;
    else if (role.kind == StoredFrame::Spherical)
        result.frame.kind = CoordinateKind::Spherical;
    result.rectangular = result.frame.kind == CoordinateKind::Rectangular;
    return result;
}

bool isCoordinateRole(StoredFrame kind) noexcept
{
    return kind == StoredFrame::Coordinate || kind == StoredFrame::Cylindrical || kind == StoredFrame::Spherical;
}

} // namespace

base::Mat3 EntityFrames::storedOfRow(std::size_t row) const noexcept
{
    const base::Mat3& entity = stored[entityOfRow[row]];
    return storedIsPly && !plyAngle.empty() ? entity * rotationAboutZ(plyAngle[row]) : entity;
}

base::Mat3 EntityFrames::targetOfRow(std::size_t row) const noexcept
{
    const base::Mat3& entity = target[entityOfRow[row]];
    return targetIsPly && !plyAngle.empty() ? entity * rotationAboutZ(plyAngle[row]) : entity;
}

namespace
{

// The frames of the entities `unique` (grid points or elements, sorted): what buildEntityFrames gives of each,
// without the rows. `linked` is 0 for an entity that is not linked to a card of the model.
struct EntitySet
{
    std::vector<base::Mat3> stored;
    std::vector<base::Mat3> target;
    std::vector<base::Vec3> point;
    std::vector<std::uint8_t> passThrough;
    std::vector<std::uint8_t> linked;
    std::vector<std::uint8_t> same; // EntityFrames::sameFrame, but for the plies
    bool targetIsStored = false;    // EntityFrames::targetIsStored, but for the plies
};

// True when two frames are one within the rounding of their axes: each entry within 1e-14 (the change between them a
// turn of less than 1e-14 radians). The material frame of a CQUAD4 without MCID and THETA (from the line G1-G2) and its
// element frame (from the diagonals) are one on a rectangle, to their rounding only.
bool sameFrames(const base::Mat3& a, const base::Mat3& b) noexcept
{
    constexpr double ROUNDING = 1.0e-14;
    for (std::size_t entry = 0; entry < a.values.size(); ++entry)
        if (!(std::fabs(a.values[entry] - b.values[entry]) <= ROUNDING))
            return false;
    return true;
}

// Notes the entities whose target is their stored frame (EntityFrames::sameFrame), once for the set.
EntitySet& noteTargetIsStored(EntitySet& set)
{
    set.targetIsStored = !set.target.empty() && set.target.size() == set.stored.size();
    if (!set.targetIsStored)
        return set;
    set.same.assign(set.stored.size(), 0);
    for (std::size_t entity = 0; entity < set.stored.size(); ++entity)
    {
        set.same[entity] = set.linked[entity] != 0 && set.passThrough[entity] == 0 &&
                                   sameFrames(set.stored[entity], set.target[entity])
                               ? 1
                               : 0;
        set.targetIsStored =
            set.targetIsStored && set.linked[entity] != 0 && (set.passThrough[entity] != 0 || set.same[entity] != 0);
    }
    return set;
}

EntitySet computeEntityFrames(const model::Model& model, const results::ResultAttachment& attachment, bool derived,
                              FrameFamily family, const StoredFrameInfo& stored, const std::optional<FrameRole>& target,
                              const std::vector<std::int32_t>& unique, base::ThreadPool* pool)
{
    EntitySet out;
    const bool nodal = family == FrameFamily::NodalVector || family == FrameFamily::NodeForceBalance;
    const bool solid = family == FrameFamily::SolidStress || family == FrameFamily::SolidStrain;
    const FrameRole storedRole{stored.kind, stored.system};
    const std::size_t count = unique.size();
    out.stored.assign(count, nanFrame());
    out.point.assign(count, nanPoint());
    out.passThrough.assign(count, 0);

    const bool needsTarget = target.has_value();
    const auto uses = [&](StoredFrame kind) { return stored.kind == kind || (needsTarget && target->kind == kind); };
    Geometry geometry;
    std::vector<std::optional<model::CardLocation>> locations(count);

    if (nodal)
    {
        const std::uint16_t gridType = model::typeIndexOf<model::NodeTable>();
        const model::NodeTable* grids = model.findTable<model::NodeTable>();
        std::vector<std::uint32_t> gridRows;
        std::vector<std::uint32_t> gridEntity;
        for (std::size_t entity = 0; entity < count; ++entity)
        {
            locations[entity] = attachment.nodeLocation(model, unique[entity]);
            if (!locations[entity])
                continue;
            if (locations[entity]->table == gridType && grids != nullptr)
            {
                gridRows.push_back(locations[entity]->row);
                gridEntity.push_back(static_cast<std::uint32_t>(entity));
            }
            else
            {
                out.passThrough[entity] = 1; // a scalar point: no components to turn
                out.stored[entity] = base::Mat3::identity();
            }
        }
        geometry.nodeOutput.assign(count, nanFrame());
        if (!gridRows.empty())
        {
            const std::vector<base::Mat3> rotations = NodeDisplacementFrames::rotations(model, gridRows, pool);
            const BasicPositionCache& cache = BasicPositionCache::of(model, pool);
            for (std::size_t index = 0; index < gridRows.size(); ++index)
            {
                geometry.nodeOutput[gridEntity[index]] = rotations[index];
                out.point[gridEntity[index]] = cache.position(gridRows[index]);
            }
        }
    }
    else
    {
        // the elements, by card type: the geometry of each group at once
        std::map<std::uint16_t, std::vector<std::uint32_t>> rowsOfType;
        std::map<std::uint16_t, std::vector<std::uint32_t>> entitiesOfType;
        for (std::size_t entity = 0; entity < count; ++entity)
        {
            locations[entity] = attachment.elementLocation(model, unique[entity]);
            if (!locations[entity])
                continue;
            rowsOfType[locations[entity]->table].push_back(locations[entity]->row);
            entitiesOfType[locations[entity]->table].push_back(static_cast<std::uint32_t>(entity));
        }
        geometry.element.assign(count, nanFrame());
        geometry.material.assign(count, nanFrame());
        std::uint8_t wanted = bit(Quantity::Center);
        if (uses(StoredFrame::Element))
            wanted |= bit(Quantity::Frame);
        if (uses(StoredFrame::Material) || uses(StoredFrame::Ply))
            wanted |= bit(Quantity::MaterialFrame);
        for (const auto& [type, rows] : rowsOfType)
        {
            const ElementArrays arrays = ElementGeometry::compute(model, type, rows, wanted, pool);
            const std::vector<std::uint32_t>& entities = entitiesOfType[type];
            for (std::size_t index = 0; index < entities.size(); ++index)
            {
                const std::uint32_t entity = entities[index];
                out.point[entity] = arrays.centers[index];
                if ((wanted & bit(Quantity::Frame)) != 0)
                    geometry.element[entity] = arrays.frames[index];
                if ((wanted & bit(Quantity::MaterialFrame)) != 0)
                    geometry.material[entity] = arrays.materialFrames[index];
            }
        }
    }

    // a frame of a role, for every entity
    const auto frameOfRole = [&](const FrameRole& role, std::vector<base::Mat3>& into)
    {
        into.assign(count, nanFrame());
        std::optional<SystemFrame> system;
        if (isCoordinateRole(role.kind))
            system = systemFrameOf(model, role, pool);
        for (std::size_t entity = 0; entity < count; ++entity)
        {
            if (!locations[entity])
                continue;
            if (out.passThrough[entity] != 0)
            {
                into[entity] = base::Mat3::identity();
                continue;
            }
            switch (role.kind)
            {
            case StoredFrame::Basic:
                into[entity] = base::Mat3::identity();
                break;
            case StoredFrame::NodeOutputSystem:
                into[entity] = geometry.nodeOutput.empty() ? nanFrame() : geometry.nodeOutput[entity];
                break;
            case StoredFrame::Element:
                into[entity] = geometry.element.empty() ? nanFrame() : geometry.element[entity];
                break;
            case StoredFrame::Material:
            case StoredFrame::Ply:
                into[entity] = geometry.material.empty() ? nanFrame() : geometry.material[entity];
                break;
            case StoredFrame::Coordinate:
            case StoredFrame::Cylindrical:
            case StoredFrame::Spherical:
                into[entity] = system->rectangular ? system->frame.axes : system->frame.directionsAt(out.point[entity]);
                break;
            case StoredFrame::Unknown:
            case StoredFrame::Mixed:
                break;
            }
        }
    };

    if (solid && !derived)
    {
        // the material system of a solid is the CORDM of its property: 0 the basic system, a rectangular system its own
        const SystemDefinitions definitions = readSystemDefinitions(model);
        std::unordered_map<base::EntityId, base::Mat3> axes;
        for (std::size_t entity = 0; entity < count; ++entity)
        {
            if (!locations[entity])
                continue;
            const auto pid = elementPropertyId(model, *locations[entity]);
            const auto cordm = pid ? solidCordm(model, *pid) : std::nullopt;
            if (!cordm)
                continue;
            if (*cordm == 0)
            {
                out.stored[entity] = base::Mat3::identity();
            }
            else if (*cordm > 0 && definitions.contains(*cordm) &&
                     definitions.at(*cordm).kind == CoordinateKind::Rectangular)
            {
                auto found = axes.find(*cordm);
                if (found == axes.end())
                    found =
                        axes.emplace(*cordm,
                                     systemFrameOf(model, FrameRole{StoredFrame::Coordinate, *cordm}, pool).frame.axes)
                            .first;
                out.stored[entity] = found->second;
            }
        }
    }
    else
    {
        frameOfRole(storedRole, out.stored);
    }
    if (needsTarget)
        frameOfRole(*target, out.target);
    out.linked.assign(count, 0);
    for (std::size_t entity = 0; entity < count; ++entity)
        out.linked[entity] = locations[entity] ? 1 : 0;
    return out;
}

} // namespace

} // namespace oor::analysis::detail

namespace oor::analysis
{

struct FrameCache::Impl
{
    // what the frames of an entity depend on besides its ID: the family of the table, whether a frame change made it,
    // its stored frame and the frame of the change
    using Key = std::tuple<std::uint8_t, bool, std::uint8_t, std::int32_t, bool, std::uint8_t, std::int32_t>;
    struct Entry
    {
        base::Mat3 stored;
        base::Mat3 target;
        base::Vec3 point;
        std::uint8_t passThrough = 0;
        std::uint8_t linked = 0;
    };
    std::mutex mutex;
    // held while a set of frames is made: the tables of one call start together on the threads of the call, and each
    // made the set of its entities (the frames of every entity again, then looked up one by one under the lock): 7 ms
    // a table against 0.3 ms for its decoding for 7 453 shells; one thread makes it, the others then
    // take it as the last set
    std::mutex making;
    std::map<Key, std::unordered_map<std::int32_t, Entry>> entries;
    // the frames of the last list of entities asked for, by key: the tables of the subcases mostly have the same
    // entities, which are then taken whole rather than looked up one by one
    std::map<Key, std::pair<std::vector<std::int32_t>, std::shared_ptr<const detail::EntitySet>>> lastSets;
};

FrameCache::FrameCache() : m_impl(std::make_unique<Impl>()) {}

FrameCache::~FrameCache() = default;

} // namespace oor::analysis

namespace oor::analysis::detail
{

namespace
{

// The frames of `unique` from the frames the cache holds of each entity, the missing ones computed and kept in it.
EntitySet cachedFrames(FrameCache::Impl& impl, const FrameCache::Impl::Key& key, const model::Model& model,
                       const results::ResultAttachment& attachment, bool derived, FrameFamily family,
                       const StoredFrameInfo& stored, const std::optional<FrameRole>& target,
                       const std::vector<std::int32_t>& unique, base::ThreadPool* pool)
{
    const bool needsTarget = target.has_value();
    const std::size_t count = unique.size();
    std::vector<std::int32_t> missing;
    {
        const std::lock_guard<std::mutex> lock(impl.mutex);
        const auto& known = impl.entries[key];
        for (const std::int32_t id : unique)
            if (!known.contains(id))
                missing.push_back(id);
    }
    if (!missing.empty())
    {
        const EntitySet computed =
            computeEntityFrames(model, attachment, derived, family, stored, target, missing, pool);
        const std::lock_guard<std::mutex> lock(impl.mutex);
        auto& known = impl.entries[key];
        for (std::size_t index = 0; index < missing.size(); ++index)
            known.try_emplace(
                missing[index],
                FrameCache::Impl::Entry{computed.stored[index], needsTarget ? computed.target[index] : nanFrame(),
                                        computed.point[index], computed.passThrough[index], computed.linked[index]});
    }
    EntitySet frames;
    const std::lock_guard<std::mutex> lock(impl.mutex);
    const auto& known = impl.entries[key];
    frames.stored.resize(count);
    frames.point.resize(count);
    frames.passThrough.resize(count);
    frames.linked.resize(count);
    if (needsTarget)
        frames.target.resize(count);
    for (std::size_t entity = 0; entity < count; ++entity)
    {
        const FrameCache::Impl::Entry& entry = known.at(unique[entity]);
        frames.stored[entity] = entry.stored;
        frames.point[entity] = entry.point;
        frames.passThrough[entity] = entry.passThrough;
        frames.linked[entity] = entry.linked;
        if (needsTarget)
            frames.target[entity] = entry.target;
    }
    return frames;
}

} // namespace

EntityFrames buildEntityFrames(const model::Model& model, const results::ResultAttachment& attachment,
                               const results::ResultTable& table, const results::ResultData& data, FrameFamily family,
                               const StoredFrameInfo& stored, const std::optional<FrameRole>& target,
                               base::ThreadPool* pool, FrameCache* cache, bool rowsWhenTargetIsStored)
{
    EntityFrames out;
    // a table that a frame change made says its frame itself; the others have the frame the model gives them
    const bool derived = table.meta().derivation.rfind("inFrame(", 0) == 0;
    const bool needsTarget = target.has_value();

    // the entities: the distinct grid points or elements of the rows (mostly in order already)
    std::vector<std::int32_t> unique(data.ids.begin(), data.ids.end());
    if (!std::is_sorted(unique.begin(), unique.end()))
        std::sort(unique.begin(), unique.end());
    unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
    const std::size_t count = unique.size();
    // the frames of each entity: those the cache has, the others computed (and kept in it); the set of the last
    // entities of the key is taken whole when they are the same (the tables of the subcases of one element type)
    std::shared_ptr<const EntitySet> frames;
    if (cache == nullptr)
    {
        EntitySet computed = computeEntityFrames(model, attachment, derived, family, stored, target, unique, pool);
        frames = std::make_shared<const EntitySet>(std::move(noteTargetIsStored(computed)));
    }
    else
    {
        const FrameCache::Impl::Key key{static_cast<std::uint8_t>(family),
                                        derived,
                                        static_cast<std::uint8_t>(stored.kind),
                                        stored.system,
                                        needsTarget,
                                        static_cast<std::uint8_t>(needsTarget ? target->kind : StoredFrame::Unknown),
                                        needsTarget ? target->system : 0};
        FrameCache::Impl& impl = cache->impl();
        const auto lastSet = [&]
        {
            const std::lock_guard<std::mutex> lock(impl.mutex);
            if (const auto found = impl.lastSets.find(key);
                found != impl.lastSets.end() && found->second.first == unique)
                frames = found->second.second;
        };
        lastSet();
        if (!frames)
        {
            const std::lock_guard<std::mutex> making(impl.making);
            lastSet(); // another thread may have made it while this one waited
            if (!frames)
            {
                EntitySet computed =
                    cachedFrames(impl, key, model, attachment, derived, family, stored, target, unique, pool);
                frames = std::make_shared<const EntitySet>(std::move(noteTargetIsStored(computed)));
                const std::lock_guard<std::mutex> lock(impl.mutex);
                impl.lastSets[key] = {unique, frames};
            }
        }
    }
    out.owner = frames;
    out.stored = frames->stored;
    out.target = frames->target;
    out.point = frames->point;
    out.passThrough = frames->passThrough;
    out.unlinkedEntities = static_cast<std::size_t>(std::count(frames->linked.begin(), frames->linked.end(), 0));

    // the plies: the angle of each ply from the material axis, by the laminate of the property of the element
    out.storedIsPly = stored.kind == StoredFrame::Ply;
    out.targetIsPly = needsTarget && target->kind == StoredFrame::Ply;
    if (!out.storedIsPly && !out.targetIsPly)
    {
        out.sameFrame = frames->same;
        out.targetIsStored = frames->targetIsStored;
    }
    // the entity of each row (none when the change is the identity and the caller does not turn the rows then)
    if (rowsWhenTargetIsStored || !out.targetIsStored)
    {
        out.entityOfRow.resize(data.ids.size());
        {
            std::int32_t lastId = 0;
            std::uint32_t lastEntity = 0;
            bool haveLast = false;
            for (std::size_t row = 0; row < data.ids.size(); ++row)
            {
                const std::int32_t id = data.ids[row];
                if (!haveLast || id != lastId)
                {
                    // the rows mostly come in the order of their entities: the next entity is tried before a search
                    if (haveLast && lastEntity + 1 < count && unique[lastEntity + 1] == id)
                        ++lastEntity;
                    else
                        lastEntity = static_cast<std::uint32_t>(std::lower_bound(unique.begin(), unique.end(), id) -
                                                                unique.begin());
                    lastId = id;
                    haveLast = true;
                }
                out.entityOfRow[row] = lastEntity;
            }
        }
    }
    if (stored.kind == StoredFrame::Ply || out.targetIsPly)
    {
        std::unordered_map<base::EntityId, std::vector<double>> angles;
        std::vector<const std::vector<double>*> anglesOfEntity(count, nullptr);
        for (std::size_t entity = 0; entity < count; ++entity)
        {
            const std::optional<model::CardLocation> location = attachment.elementLocation(model, unique[entity]);
            if (!location)
                continue;
            const auto pid = elementPropertyId(model, *location);
            if (!pid)
                continue;
            auto found = angles.find(*pid);
            if (found == angles.end())
            {
                std::vector<double> list;
                if (const auto property = model.locate(base::IdSpace::Property, *pid))
                    for (const LaminatePly& ply : Laminate::stack(model, *property))
                        list.push_back(ply.angleInDegrees * (std::numbers::pi / 180.0));
                found = angles.emplace(*pid, std::move(list)).first;
            }
            anglesOfEntity[entity] = &found->second;
        }
        out.plyAngle.assign(data.ids.size(), NAN_VALUE);
        for (std::size_t row = 0; row < data.ids.size(); ++row)
        {
            const std::vector<double>* list = anglesOfEntity[out.entityOfRow[row]];
            const std::int32_t layer = row < data.layers.size() ? data.layers[row] : 0;
            if (list != nullptr && layer >= 1 && static_cast<std::size_t>(layer) <= list->size())
                out.plyAngle[row] = (*list)[static_cast<std::size_t>(layer) - 1];
        }
    }
    return out;
}

} // namespace oor::analysis::detail
