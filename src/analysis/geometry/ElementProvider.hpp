#pragma once

#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/analysis/frames/FrameMath.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardRegistry.hpp"
#include "oor/model/core/CardTable.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

// What the element families of this folder share: how a card type gives its nodes and its geometry, the context a
// computation runs in (node rotations, properties), and the registry of card types.

namespace oor::analysis::detail
{

// What one thread needs while it computes the geometry of many elements: the model, the positions, the coordinate
// systems it met, and the thickness and area of the properties it looked up. One per chunk of elements, never shared.
class GeometryContext
{
public:
    GeometryContext(const model::Model& model, BasicPositionCache& cache) : m_model(model), m_cache(cache) {}

    [[nodiscard]] const model::Model& model() const noexcept { return m_model; }
    [[nodiscard]] BasicPositionCache& cache() const noexcept { return m_cache; }

    // The directions 1, 2, 3 of the displacement system (CD) of the grid point in `gridRow`, at its position, as the
    // columns of a matrix in the basic system. The identity for CD 0 and -1.
    [[nodiscard]] base::Mat3 displacementRotation(std::uint32_t gridRow) const;

    // The axes of coordinate system `cid` at a point of the basic system (the x, y, z of a rectangular system, R,
    // theta, z of a cylindrical one, R, theta, phi of a spherical one), as the columns of a matrix. Throws
    // CoordinateSystemError when the system does not exist.
    [[nodiscard]] base::Mat3 systemAxesAt(base::EntityId cid, const base::Vec3& basicPoint) const;

    // The point with these coordinates in coordinate system `cid` (read as the system's kind says), in the basic
    // system.
    [[nodiscard]] base::Vec3 pointInSystem(base::EntityId cid, const base::Vec3& coordinates) const;

    // The thickness of a shell property: PSHELL, PCOMP, PCOMPG, PSHEAR (NaN for another property or none).
    [[nodiscard]] double shellThickness(base::EntityId pid) const;
    // True for a PCOMP or PCOMPG: its thickness is the total of the plies, whatever the thicknesses of the corners.
    [[nodiscard]] bool isLaminate(base::EntityId pid) const;
    // The cross-section area of a rod, bar or beam property: PROD, PTUBE, PBAR, and the mean over the length of PBEAM,
    // PBEAML, PBEAM3, PBCOMP, PBARL (the section formulas of the mass step); the area of CONROD is on the card (NaN for
    // another property or none).
    [[nodiscard]] double sectionArea(base::EntityId pid) const;

private:
    [[nodiscard]] const CoordinateFrame& frameOf(base::EntityId cid) const;

    const model::Model& m_model;
    BasicPositionCache& m_cache;
    mutable std::unordered_map<base::EntityId, CoordinateFrame> m_frames;
    mutable std::unordered_map<base::EntityId, double> m_thickness;
    mutable std::unordered_map<base::EntityId, double> m_area;
};

// Everything the evaluation of one element is given.
struct Evaluation
{
    const GeometryContext& context;
    const model::CardTable& table;
    std::uint32_t row;
    std::uint8_t wanted; // the quantities asked for (bits of Quantity)
    std::span<const base::EntityId>
        nodeIds; // the node IDs of the element, in the order its provider gave them (0: blank)
    std::span<const std::int32_t> nodeRows; // the GRID row of each node, -1 when it is blank or not a grid point
    std::span<const base::Vec3> points;     // the basic position of each node, NaN when it has none

    [[nodiscard]] bool wants(Quantity quantity) const noexcept { return (wanted & bit(quantity)) != 0; }
};

// The geometry of one card type.
class ElementProvider
{
public:
    virtual ~ElementProvider() = default;
    // Appends the node IDs of the element in `row` (0 for a blank slot, so that positions in the list stay the ones of
    // the card; a card with a variable number of nodes gives as many as it has).
    virtual void nodes(const model::CardTable& table, std::uint32_t row, std::vector<base::EntityId>& ids) const = 0;
    // Sets the wanted quantities of `values` (the others are left as they are: NaN).
    virtual void evaluate(const Evaluation& evaluation, ElementValues& values) const = 0;
    // The card name, for messages.
    [[nodiscard]] virtual const char* name() const noexcept = 0;
};

// A provider written as a Shape: a struct with
//   static void nodes(const Table&, std::uint32_t row, std::vector<EntityId>&);
//   static void evaluate(const Table&, const Evaluation&, ElementValues&);
template <typename TableT, typename Shape>
class TableProvider final : public ElementProvider
{
public:
    void nodes(const model::CardTable& table, std::uint32_t row, std::vector<base::EntityId>& ids) const override
    {
        Shape::nodes(static_cast<const TableT&>(table), row, ids);
    }
    void evaluate(const Evaluation& evaluation, ElementValues& values) const override
    {
        Shape::evaluate(static_cast<const TableT&>(evaluation.table), evaluation, values);
    }
    [[nodiscard]] const char* name() const noexcept override { return TableT::NAME.data(); }
};

// The providers by card type. Filled once, by the registration function of each family.
class ProviderRegistry
{
public:
    template <typename TableT, typename Shape>
    void add()
    {
        const std::uint16_t type = model::typeIndexOf<TableT>();
        if (m_providers.size() <= type)
            m_providers.resize(static_cast<std::size_t>(type) + 1);
        m_providers[type] = std::make_unique<TableProvider<TableT, Shape>>();
    }
    [[nodiscard]] const ElementProvider* find(std::uint16_t type) const noexcept
    {
        return type < m_providers.size() ? m_providers[type].get() : nullptr;
    }
    [[nodiscard]] static const ProviderRegistry& instance();

private:
    std::vector<std::unique_ptr<ElementProvider>> m_providers;
};

// One function per family file; each adds its card types.
void registerTriangleProviders(ProviderRegistry& registry);
void registerQuadProviders(ProviderRegistry& registry);
void registerTetraProviders(ProviderRegistry& registry);
void registerPentaProviders(ProviderRegistry& registry);
void registerHexaProviders(ProviderRegistry& registry);
void registerPyramidProviders(ProviderRegistry& registry);
void registerLineProviders(ProviderRegistry& registry);
void registerPointProviders(ProviderRegistry& registry);
void registerRigidProviders(ProviderRegistry& registry);
void registerSurfaceProviders(ProviderRegistry& registry);
void registerAcousticProviders(ProviderRegistry& registry);
void registerInterfaceProviders(ProviderRegistry& registry);
void registerCrackSeamSlotProviders(ProviderRegistry& registry);
void registerSol700ScalarProviders(ProviderRegistry& registry);
void registerDummyProviders(ProviderRegistry& registry);
void registerRigidSplineProviders(ProviderRegistry& registry);
void registerPlotProviders(ProviderRegistry& registry);

} // namespace oor::analysis::detail
