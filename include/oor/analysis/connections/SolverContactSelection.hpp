#pragma once

#include "oor/analysis/connections/ContactPairs.hpp"
#include "oor/analysis/connections/ContactSearch.hpp"
#include "oor/analysis/connections/ContactSurfaces.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <vector>

namespace oor::analysis
{

// The node pairs of contact as the solver selects them for a glued pair (glued contact for a linear solver):
//   * the distance tolerance is the ERROR of the pair, else of BCPARA, else the solver's automatic value: the smallest
//   of the
//     smallest element dimension of a body divided by 20 and its thinnest shell divided by 4 (format reference BCPARA,
//     ERROR), over every body of a pair (one value for the model), or over the two bodies of each pair with BCPARA
//     ERRBAS 1;
//   * the BIAS of the pair biases the zone: a gap between -(1 + BIAS) ERROR and (1 - BIAS) ERROR meets (the BIAS is 0
//     for a glued pair unless the pair gives one: format reference BCPARA, BIAS);
//   * ISEARCH 0 searches from the body of the lower ID to the other one, then the other way, adding what does not
//     conflict with the first constraints; 1 from the secondary body only; 2 from
//     the body of the smaller characteristic length (the smallest outer edge over 20 and the thinnest shell over 4, the
//     thickness left out with BCPARA THKOFF 1), the lower ID while it is at most 1.05 times the other's;
//   * a grid point is left out when it is a corner of the face it would be tied to, when a rigid element or an MPC
//     equation already makes it dependent, when an earlier pair made it dependent, and when the search the other way
//     would tie it while it holds a constraint of the first (a grid point the other body's points are tied to) or tie
//     it to a face one of whose corners is tied: never a loop of dependencies.
// What the format reference does not say (the solver's choice among points of an edge, its handling of a constrained
// point) is the library's assumption.
class SolverContactSelection
{
public:
    // The meetings of each pair (parallel to `pairs`), in the order of the pairs.
    [[nodiscard]] static std::vector<ContactPairResult> select(const model::Model& model,
                                                               const std::vector<ContactPair>& pairs,
                                                               const ContactSurfaces& surfaces, bool errorPerPair,
                                                               bool thicknessOffInOrder, base::ThreadPool* pool);

    // the solver's automatic ERROR from one surface: the smallest element dimension over 20 and the thinnest shell over
    // 4, the smaller (infinite for a surface with neither).
    [[nodiscard]] static double automaticError(const ContactSurface& surface) noexcept;
};

} // namespace oor::analysis
