#pragma once

#include "oor/analysis/connections/ContactBodies.hpp"
#include "oor/analysis/connections/ContactOptions.hpp"
#include "oor/analysis/connections/ContactPairs.hpp"
#include "oor/analysis/connections/ContactSurfaces.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <limits>
#include <memory>
#include <optional>
#include <vector>

namespace oor::analysis
{

// The body a meeting grid point belongs to: the pair's secondary body (searched from the secondary body) or its primary
// body (the search the other way, format reference ISEARCH 0 and 2).
enum class SearchSide : std::uint8_t
{
    Secondary,
    Primary
};

// "secondary" or "primary".
[[nodiscard]] std::string_view searchSideName(SearchSide side) noexcept;

// One grid point of a pair that meets: the body whose grid point it is, the body whose surface it projects on, the face
// of that surface and the projection.
struct ContactMeeting
{
    base::EntityId nid = 0;
    const ContactBody* secondary = nullptr; // the body of the grid point
    const ContactBody* primary = nullptr;   // the body it meets
    const ContactFace* face = nullptr;      // the face of the primary surface
    base::Vec3 point;                       // the projection on the face, basic system
    double gap = 0.0;                       // between the skins, positive outside the primary body
    double distance = 0.0;                  // max(gap, 0)
    FaceCoordinates coordinates;            // of `point` on the face
    SearchSide searchedFrom = SearchSide::Secondary;
    // From the grid point to the point of its shells' skin that meets the face (the the solver rule, a grid point of shells
    // on the face of a solid); zero when the grid point itself meets it.
    base::Vec3 surfaceOffset;
};

// What the search found for one pair.
struct ContactPairResult
{
    // The distance used: the pair's, else the caller's, else 5 % of the median secondary edge (the components' rule);
    // the ERROR of the pair, else of BCPARA, else the solver's automatic value (the the solver rule).
    double tolerance = 0.0;
    // True when at least one grid point of the secondary surface meets the primary surface (or, for a pair that
    // searches both ways, one of the primary surface meets the secondary surface).
    bool meets = false;
    bool isReversed = false;              // every meeting was found from the primary body
    std::vector<ContactMeeting> meetings; // by search side, then increasing grid point
    double minDistance = std::numeric_limits<double>::quiet_NaN();
    double maxDistance = std::numeric_limits<double>::quiet_NaN();
};

// The contact between the bodies of a model: the bodies, the pairs of
// every contact table of the Bulk Data (ContactPairs) and, for each pair, the grid points of the secondary surface that
// meet the primary surface, by one of two rules (ContactRule).
//
// The components' rule: the distance of a pair is its ERROR or DQNEAR (the MAXD of a BCTSET pair), else `tolerance`,
// else 5 % of the median edge length of the secondary surface (of the primary surface when the secondary one is a
// BCGRID, which has no edge). A grid point meets a face when the gap between their skins is at most that distance, and
// when it overlaps a shell however deep (a stiffener whose edge lies on the mid surface of a panel). A pair that finds
// no meeting from its secondary body and searches both ways (format reference BCTABLE and BCONPRG, ISEARCH 0 or 2) is searched from
// its primary body; the meetings then name the primary body as the body of the grid point. The pairs of a Bulk Data
// with no table (every two bodies) are kept only when they meet.
//
// The the solver rule: the selection of the solver for a glued pair, see
// SolverContactSelection.
//
// The shells count with their thickness and offset in both rules, as the digit B of COPTS and COPTM says (ShellSkins).
//
// Immutable, not copyable (the pairs and meetings point into the bodies and surfaces it owns); cached with the model
// for each tolerance and rule (ComponentCache).
class ContactSearch
{
public:
    // The fraction of the median edge length of the secondary surface that is the default distance.
    static constexpr double DEFAULT_TOLERANCE_FRACTION = 0.05;

    [[nodiscard]] static std::unique_ptr<const ContactSearch> compute(const model::Model& model,
                                                                      std::optional<double> tolerance,
                                                                      ContactRule rule = ContactRule::Components,
                                                                      base::ThreadPool* pool = nullptr);
    // Cached with the model until its content changes.
    [[nodiscard]] static std::shared_ptr<const ContactSearch> of(const model::Model& model,
                                                                 std::optional<double> tolerance,
                                                                 ContactRule rule = ContactRule::Components,
                                                                 base::ThreadPool* pool = nullptr);

    ContactSearch(const ContactSearch&) = delete;
    ContactSearch& operator=(const ContactSearch&) = delete;

    [[nodiscard]] ContactRule rule() const noexcept { return m_rule; }
    [[nodiscard]] const ContactBodies& bodies() const noexcept { return m_bodies; }
    [[nodiscard]] const std::vector<ContactPair>& pairs() const noexcept { return m_pairs; }
    // Parallel to pairs().
    [[nodiscard]] const std::vector<ContactPairResult>& results() const noexcept { return m_results; }
    // True when the pairs come from tables (a pair that does not meet is then reported by `contact.pair-apart`).
    [[nodiscard]] bool fromTables() const noexcept { return m_fromTables; }
    [[nodiscard]] bool isEmpty() const noexcept { return m_pairs.empty(); }

private:
    ContactSearch() = default;

    ContactRule m_rule = ContactRule::Components;
    ContactBodies m_bodies;
    std::unique_ptr<ContactSurfaces> m_surfaces;
    std::vector<ContactPair> m_pairs;
    std::vector<ContactPairResult> m_results;
    bool m_fromTables = false;
};

} // namespace oor::analysis
