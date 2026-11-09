#pragma once

#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace oor::analysis
{

// The units a solver condenses, as the code that groups load cases, stamps constraints or finds spiders sees them: the
// components of the model, or the groups a caller gives (GroupSet). Units are numbered from 1. A node or an element can
// be in several units (two groups may overlap) or in none.
class UnitMembership
{
public:
    UnitMembership() = default;
    UnitMembership(const UnitMembership&) = default;
    UnitMembership& operator=(const UnitMembership&) = default;
    UnitMembership(UnitMembership&&) = default;
    UnitMembership& operator=(UnitMembership&&) = default;
    virtual ~UnitMembership() = default;

    [[nodiscard]] virtual std::size_t unitCount() const noexcept = 0;
    // Appends the units that hold the node (none for a node of no unit).
    virtual void unitsOfNode(base::EntityId node, std::vector<std::int32_t>& units) const = 0;
    // Appends the units that hold the element.
    virtual void unitsOfElement(model::CardLocation element, std::vector<std::int32_t>& units) const = 0;
    // The nodes of a unit, sorted by ID.
    [[nodiscard]] virtual std::span<const base::EntityId> nodesOf(std::int32_t unit) const = 0;
};

// The components of a model as units: unit `id` is component `id`.
class ComponentMembership final : public UnitMembership
{
public:
    explicit ComponentMembership(const ComponentModel& components) : m_components(components) {}

    [[nodiscard]] std::size_t unitCount() const noexcept override { return m_components.componentCount(); }
    void unitsOfNode(base::EntityId node, std::vector<std::int32_t>& units) const override
    {
        if (const std::int32_t component = m_components.componentOfNode(node); component != 0)
            units.push_back(component);
    }
    void unitsOfElement(model::CardLocation element, std::vector<std::int32_t>& units) const override
    {
        if (const std::int32_t component = m_components.componentOfElement(element); component != 0)
            units.push_back(component);
    }
    [[nodiscard]] std::span<const base::EntityId> nodesOf(std::int32_t unit) const override
    {
        return m_components.nodesOf(unit);
    }

private:
    const ComponentModel& m_components;
};

} // namespace oor::analysis
