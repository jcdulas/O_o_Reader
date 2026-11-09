#pragma once

#include "oor/analysis/connections/FastenerPiercings.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/base/math/Vec3.hpp"
#include "oor/model/core/Model.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::analysis::detail
{

// What the piercing points of one fastener are found from: its card and the SWLDPRM projection tolerance that applies
// to its element type, in one shape for CFAST and CWELD.
struct FastenerInput
{
    static constexpr double DEFAULT_PROJTOL = 0.02;

    model::CardLocation card;
    base::EntityId eid = 0;
    FastenerForm form = FastenerForm::Prop;
    base::EntityId gs = 0;
    base::EntityId ga = 0;
    base::EntityId gb = 0;
    std::optional<base::Vec3> location; // XS YS ZS in the basic system
    // The surface of each side: an element ID (ELEM, ELPAT, ELEMID) or a property ID (PROP, PARTPAT); 0 when blank.
    base::EntityId ida = 0;
    base::EntityId idb = 0;
    std::array<base::EntityId, 8> gai{}; // GRIDID
    std::array<base::EntityId, 8> gbi{};
    char shapeA = 0; // GRIDID: 'Q' or 'T' from SPTYP; 0 for a side without grid points
    char shapeB = 0;
    double projtol = DEFAULT_PROJTOL;
};

// Reads the CFAST and CWELD cards of a model and the SWLDPRM card, by element ID.
class FastenerInputs
{
public:
    [[nodiscard]] static std::vector<FastenerInput> read(const model::Model& model);
};

} // namespace oor::analysis::detail
