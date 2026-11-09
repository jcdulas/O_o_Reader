#pragma once

#include "ElementProvider.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/analysis/mass/SectionFormulas.hpp"
#include "oor/base/math/Mat3.hpp"
#include "oor/base/math/Vec3.hpp"
#include "oor/model/core/CardRegistry.hpp"
#include "oor/model/core/CardTable.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

// What the element families of this folder share: the mass of one element, the context a computation runs in (the
// properties and materials it looked up) and the registry of card types that have mass.

namespace oor::analysis::detail
{

// The card an error message names: its card type ("ShellQuad4") or "property", and its ID. The message shows the card
// type by its deck keyword: "CQUAD4 10", "property 5".
struct Owner
{
    const char* card = "";
    base::EntityId id = 0;

    [[nodiscard]] std::string text(const model::Model&) const { return std::string(card) + " " + std::to_string(id); }
};

// The mass of one element in the basic system. `structural` is the mass of the material, `nonStructural` what the NSM
// fields of the property and the element add (the NSM cards are added by the caller). The mass is at `center`; a
// concentrated mass or a beam with non-structural mass off its axis also has `inertia`, the tensor about `center`.
struct MassValue
{
    double structural = 0.0;
    double nonStructural = 0.0;
    base::Vec3 center = undefinedVector();
    bool hasInertia = false;
    base::Mat3 inertia{{0, 0, 0, 0, 0, 0, 0, 0, 0}};
    base::EntityId property = 0;
    base::EntityId material = 0;
};

// The mass data of a shell, shear panel or plane property: what a unit of area weighs, or the density to multiply the
// volume with.
struct ShellData
{
    enum class Kind : std::uint8_t
    {
        Plate,    // PSHELL, PSHEAR: density times the thickness of the element
        Laminate, // PCOMP, PCOMPG: the plies' density times thickness, summed
        Volume,   // PLPLANE: density times the volume of the element (the thickness is the one of the element)
        None      // not a property of a shell
    };
    Kind kind = Kind::None;
    double density = 0.0;
    double massPerArea = 0.0; // Laminate: the sum of density times thickness
    double nsm = 0.0;         // per unit area
    base::EntityId material = 0;
};

// The mass data of a solid property.
struct SolidData
{
    bool found = false;
    bool massless =
        false; // a fluid element: its mass is not structural mass (the format reference: fluid masses are ignored)
    double density = 0.0;
    base::EntityId material = 0;
};

// What one thread needs while it computes the mass of many elements: the model, the geometry context and the properties
// and materials it looked up. One per chunk of elements, never shared.
class MassContext
{
public:
    // `massOnOffsets`: the mass of the elements with offsets is on the offsets (OffsetMass::Offsets), else at the grid
    // points.
    MassContext(GeometryContext& geometry, bool massOnOffsets) : m_geometry(geometry), m_massOnOffsets(massOnOffsets) {}

    [[nodiscard]] const model::Model& model() const noexcept { return m_geometry.model(); }
    [[nodiscard]] GeometryContext& geometry() const noexcept { return m_geometry; }
    [[nodiscard]] bool massOnOffsets() const noexcept { return m_massOnOffsets; }
    // The position of the grid point `nid` in the basic system; NaN for a node that is not a grid point.
    [[nodiscard]] base::Vec3 gridPosition(base::EntityId nid) const;

    // The density of a material; throws MassError (naming the card and the ID in `owner`) when it does not exist or has
    // no density.
    [[nodiscard]] double density(base::EntityId mid, const Owner& owner);
    // The same for the material of a property, named in the error as the property of the element in `owner`.
    [[nodiscard]] double propertyDensity(base::EntityId mid, base::EntityId pid, const Owner& owner);
    [[nodiscard]] const ShellData& shell(base::EntityId pid, const Owner& owner);
    [[nodiscard]] const SolidData& solid(base::EntityId pid, const Owner& owner);
    // The section and density of a rod, tube, bar or beam property.
    struct LineData
    {
        BeamSection section;
        double density = 0.0;
    };
    [[nodiscard]] const LineData& line(base::EntityId pid, const Owner& owner);
    // The mass of a PMASS, a PBUSH (M) or a PFAST. A property that does not exist is an error when `required` (the
    // PMASS of a CMASS1), else it has no mass (the PBUSH of a CBUSH, which is not a mass element).
    [[nodiscard]] double lumpedMass(base::EntityId pid, const Owner& owner, bool required);

private:
    GeometryContext& m_geometry;
    bool m_massOnOffsets = false;
    std::unordered_map<base::EntityId, double> m_density;
    std::unordered_map<base::EntityId, ShellData> m_shell;
    std::unordered_map<base::EntityId, SolidData> m_solid;
    std::unordered_map<base::EntityId, LineData> m_line;
    std::unordered_map<base::EntityId, double> m_lumped;
};

// The mass of one card type.
class MassProvider
{
public:
    virtual ~MassProvider() = default;
    // The geometry quantities (bits of Quantity) the evaluation needs.
    [[nodiscard]] virtual std::uint8_t quantities() const noexcept = 0;
    virtual void evaluate(MassContext& context, const model::CardTable& table, std::uint32_t row,
                          const ElementValues& geometry, MassValue& value) const = 0;
    // The card name, for messages.
    [[nodiscard]] virtual const char* name() const noexcept = 0;
};

// A provider written as a Shape: a struct with
//   static constexpr std::uint8_t QUANTITIES;
//   static void evaluate(const Table&, std::uint32_t row, MassContext&, const ElementValues&, MassValue&);
template <typename TableT, typename Shape>
class TableMassProvider final : public MassProvider
{
public:
    [[nodiscard]] std::uint8_t quantities() const noexcept override { return Shape::QUANTITIES; }
    void evaluate(MassContext& context, const model::CardTable& table, std::uint32_t row, const ElementValues& geometry,
                  MassValue& value) const override
    {
        Shape::evaluate(static_cast<const TableT&>(table), row, context, geometry, value);
    }
    [[nodiscard]] const char* name() const noexcept override { return TableT::NAME.data(); }
};

// The providers by card type. Filled once, by the registration function of each family. A card type without a provider
// has no mass (springs, dampers, rigid elements, ...).
class MassRegistry
{
public:
    template <typename TableT, typename Shape>
    void add()
    {
        const std::uint16_t type = model::typeIndexOf<TableT>();
        if (m_providers.size() <= type)
            m_providers.resize(static_cast<std::size_t>(type) + 1);
        m_providers[type] = std::make_unique<TableMassProvider<TableT, Shape>>();
    }
    [[nodiscard]] const MassProvider* find(std::uint16_t type) const noexcept
    {
        return type < m_providers.size() ? m_providers[type].get() : nullptr;
    }
    [[nodiscard]] static const MassRegistry& instance();

private:
    std::vector<std::unique_ptr<MassProvider>> m_providers;
};

void registerShellMass(MassRegistry& registry);
void registerSolidMass(MassRegistry& registry);
void registerLineMass(MassRegistry& registry);
void registerPointMass(MassRegistry& registry);

} // namespace oor::analysis::detail
