#pragma once

#include "oor/base/math/Vec3.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/cards/thermal/ThermalRuns.hpp"
#include "oor/model/core/Model.hpp"

#include <span>
#include <unordered_map>
#include <vector>

// What the queries of the thermal model share: the surface elements with their geometry, the IDs a list of runs stands
// for, and the heat generation multiplier of a conduction element.

namespace oor::query::thermal
{

// A CHBDYE, CHBDYG or CHBDYP with the geometry the analysis gives it (NaN where it has none).
struct SurfaceElement
{
    model::CardLocation location;
    double area = 0.0;
    base::Vec3 normal;
    base::Vec3 center;
};

// The surface elements of a model by ID, the geometry computed once for all of them.
class SurfaceElements
{
public:
    SurfaceElements(const model::Model& model, base::ThreadPool* pool);

    [[nodiscard]] const SurfaceElement* find(base::EntityId eid) const noexcept;
    // The IDs of the surface elements, ascending.
    [[nodiscard]] const std::vector<base::EntityId>& ids() const noexcept { return m_ids; }

private:
    std::unordered_map<base::EntityId, SurfaceElement> m_entries;
    std::vector<base::EntityId> m_ids;
};

// The IDs among `existing` (ascending) that the runs stand for, in the order of the card. A range takes the elements
// that exist in it (a step BY n keeps the ones n apart from the first); a single ID is kept when it exists, a negative
// one (the back face of a surface element) by its absolute value. An ID that does not exist is left out.
[[nodiscard]] std::vector<base::EntityId> existingIdsOfRuns(const std::vector<model::ThermalRun>& runs,
                                                             std::span<const base::EntityId> existing);

} // namespace oor::query::thermal
