#pragma once

#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <vector>

namespace oor::analysis
{

// The components whose revision stamp (RevisionStamp) differs between two states of a model: what an edit, such as a
// design point given by `model.design.apply`, changed, so that a solver re-reduces those components only.
class ComponentChanges
{
public:
    // The numbers of the components of `after` whose stamp differs from the stamp of the same component in `before`,
    // ascending. When the two states do not have the same components (an edit changed the connectivity, so the numbers
    // name other sets of elements), every component of `after` is listed. The stamps come from the component cache of
    // each model (computed when the model has none for its content).
    [[nodiscard]] static std::vector<std::int32_t> between(const model::Model& before, const model::Model& after,
                                                           base::ThreadPool* pool = nullptr);
};

} // namespace oor::analysis
