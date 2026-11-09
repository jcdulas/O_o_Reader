#pragma once

#include <memory>

namespace oor::model
{
class Model;
}

namespace oor::query
{

// How a table finds the model it was made from, when `apply` has to write to it. A table is a copy, independent of the
// model: it must not keep the model alive, so it does not hold the model but a link that gives it back while it exists
// (the layer that owns the model objects makes the link: a weak reference to the Python object, for instance). A link
// whose model is gone gives null.
class ModelLink
{
public:
    virtual ~ModelLink() = default;
    [[nodiscard]] virtual std::shared_ptr<model::Model> resolve() const = 0;
};

} // namespace oor::query
