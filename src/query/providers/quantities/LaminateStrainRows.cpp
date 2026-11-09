#include "oor/query/providers/quantities/LaminateStrainRows.hpp"

#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/analysis/mass/LaminateStrains.hpp"
#include "oor/analysis/mass/MassErrors.hpp"
#include "oor/query/results/ResultModelIndex.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <numbers>
#include <tuple>
#include <unordered_map>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// What the strains of one element need: the inverse of the ABD of its property and the angle of its material axis
// from its element x axis (degrees). No inverse for an element without a shell property.
struct ElementLaminate
{
    std::shared_ptr<const analysis::AbdMatrix> inverse;
    std::shared_ptr<const analysis::SandwichLayup> sandwich; // a laminate's faces and core (SandwichCore)
    double angle = 0.0;
};

class Laminates
{
public:
    // `core`: the core plies of the sandwiches (null: the thickest ply of each).
    explicit Laminates(const ResultQueryContext& context, const SandwichCore* core = nullptr)
        : m_context(context), m_index(context), m_core(core)
    {
    }

    const ElementLaminate& of(std::size_t set, std::int32_t eid)
    {
        const std::uint64_t key = (static_cast<std::uint64_t>(set) << 32U) | static_cast<std::uint32_t>(eid);
        if (const auto found = m_elements.find(key); found != m_elements.end())
            return found->second;
        ElementLaminate laminate;
        const model::Model* model = m_context.modelOf(set);
        const std::optional<model::CardLocation> element = m_index.element(set, eid);
        const std::optional<model::CardLocation> property = m_index.propertyLocation(set, eid);
        if (model != nullptr && element && property)
        {
            laminate.inverse = inverseOf(*model, *property);
            laminate.sandwich = sandwichOf(*model, *property);
            const analysis::ElementValues geometry = analysis::ElementGeometry::of(
                *model, *element,
                analysis::bit(analysis::Quantity::Frame) | analysis::bit(analysis::Quantity::MaterialFrame));
            // the material axis in the element axes
            const base::Vec3 axis = geometry.materialFrame.column(0);
            laminate.angle =
                std::atan2(base::dot(axis, geometry.frame.column(1)), base::dot(axis, geometry.frame.column(0))) *
                180.0 / std::numbers::pi;
            if (!std::isfinite(laminate.angle))
                laminate.inverse = nullptr;
        }
        return m_elements.emplace(key, laminate).first->second;
    }

private:
    std::shared_ptr<const analysis::AbdMatrix> inverseOf(const model::Model& model, model::CardLocation property)
    {
        const auto key = std::make_tuple(&model, property.table, property.row);
        if (const auto found = m_inverses.find(key); found != m_inverses.end())
            return found->second;
        std::shared_ptr<const analysis::AbdMatrix> inverse;
        try
        {
            if (const auto stiffness = analysis::shellStiffness(model, property))
                inverse = std::make_shared<const analysis::AbdMatrix>(analysis::invertAbd(analysis::abdOf(*stiffness)));
        }
        catch (const analysis::MassError&)
        {
            // a ply or a material without a plane stress law: no strains
        }
        m_inverses.emplace(key, inverse);
        return inverse;
    }

    std::shared_ptr<const analysis::SandwichLayup> sandwichOf(const model::Model& model, model::CardLocation property)
    {
        const auto key = std::make_tuple(&model, property.table, property.row);
        if (const auto found = m_sandwiches.find(key); found != m_sandwiches.end())
            return found->second;
        std::shared_ptr<const analysis::SandwichLayup> sandwich;
        try
        {
            if (auto layup = analysis::sandwichLayup(model, property, coreOf(model, property));
                layup && layup->coreThickness > 0.0)
                sandwich = std::make_shared<const analysis::SandwichLayup>(std::move(*layup));
        }
        catch (const analysis::MassError&)
        {
            // a ply or a material without a plane stress law: no faces
        }
        m_sandwiches.emplace(key, sandwich);
        return sandwich;
    }

    // The core ply the caller declares for the laminate of `property`, or nullopt for the thickest.
    [[nodiscard]] std::optional<std::size_t> coreOf(const model::Model& model, model::CardLocation property) const
    {
        if (m_core == nullptr)
            return std::nullopt;
        if (!m_core->byProperty.empty())
        {
            const auto pid = static_cast<std::int32_t>(model.findTable(property.table)->idAt(property.row));
            if (const auto found = m_core->byProperty.find(pid); found != m_core->byProperty.end())
                return found->second;
        }
        return m_core->ply;
    }

    const ResultQueryContext& m_context;
    ResultModelIndex m_index;
    const SandwichCore* m_core = nullptr;
    std::unordered_map<std::uint64_t, ElementLaminate> m_elements;
    std::map<std::tuple<const model::Model*, std::uint16_t, std::uint32_t>, std::shared_ptr<const analysis::AbdMatrix>>
        m_inverses;
    std::map<std::tuple<const model::Model*, std::uint16_t, std::uint32_t>,
             std::shared_ptr<const analysis::SandwichLayup>>
        m_sandwiches;
};

} // namespace

const std::vector<std::string>& laminateStrainFields()
{
    static const std::vector<std::string> FIELDS = {"ex0", "ey0", "gxy0", "kx", "ky", "kxy"};
    return FIELDS;
}

std::vector<ElementFlat> laminateStrainFlats(const ResultQueryContext& context, const std::vector<ElementFlat>& forces,
                                             bool material)
{
    static const std::array<const char*, 6> RESULTANTS = {"nx", "ny", "nxy", "mx", "my", "mxy"};
    Laminates laminates(context);
    std::vector<ElementFlat> out;
    out.reserve(forces.size());
    for (const ElementFlat& flat : forces)
    {
        if (flat.isComplex())
            continue;
        ElementFlat strains = flat;
        strains.values.clear();
        strains.texts.clear();
        strains.frame = material ? "material" : "element";
        const std::size_t rows = flat.rows();
        std::array<const std::vector<double>*, 6> columns{};
        for (std::size_t index = 0; index < RESULTANTS.size(); ++index)
        {
            const auto found = flat.values.find(RESULTANTS[index]);
            columns[index] = found != flat.values.end() ? &found->second : nullptr;
        }
        std::array<std::vector<double>, 6> fields;
        for (std::vector<double>& field : fields)
            field.assign(rows, NOT_A_NUMBER);
        for (std::size_t row = 0; row < rows; ++row)
        {
            const ElementLaminate& laminate = laminates.of(flat.set, flat.eid[row]);
            if (!laminate.inverse)
                continue;
            std::array<double, 6> loads{};
            for (std::size_t index = 0; index < 6; ++index)
                loads[index] = columns[index] != nullptr ? (*columns[index])[row] : NOT_A_NUMBER;
            // the resultants in the material axes, where the ABD is; the moments of the format are those of the
            // lamination theory with the opposite sign (sigma = n / t - 12 m z / t^3)
            const std::array<double, 3> n = analysis::turnResultants({loads[0], loads[1], loads[2]}, laminate.angle);
            const std::array<double, 3> m = analysis::turnResultants({-loads[3], -loads[4], -loads[5]}, laminate.angle);
            analysis::MidPlaneStrains found = analysis::strainsOf(*laminate.inverse, n, m);
            if (!material)
            {
                found.strain = analysis::turnStrains(found.strain, -laminate.angle);
                found.curvature = analysis::turnStrains(found.curvature, -laminate.angle);
            }
            for (std::size_t index = 0; index < 3; ++index)
            {
                fields[index][row] = found.strain[index];
                fields[index + 3][row] = found.curvature[index];
            }
        }
        for (std::size_t index = 0; index < 6; ++index)
            strains.values[laminateStrainFields()[index]] = std::move(fields[index]);
        out.push_back(std::move(strains));
    }
    return out;
}

const std::vector<std::string>& sandwichFields()
{
    static const std::vector<std::string> FIELDS = {"n1x", "n1y", "n1xy", "n2x", "n2y", "n2xy", "tcx", "tcy"};
    return FIELDS;
}

std::vector<ElementFlat> sandwichFlats(const ResultQueryContext& context, const std::vector<ElementFlat>& forces,
                                       bool material, const SandwichCore& core)
{
    static const std::array<const char*, 8> RESULTANTS = {"nx", "ny", "nxy", "mx", "my", "mxy", "qx", "qy"};
    Laminates laminates(context, &core);
    std::vector<ElementFlat> out;
    out.reserve(forces.size());
    for (const ElementFlat& flat : forces)
    {
        if (flat.isComplex())
            continue;
        ElementFlat parts = flat;
        parts.values.clear();
        parts.texts.clear();
        parts.frame = material ? "material" : "element";
        const std::size_t rows = flat.rows();
        std::array<const std::vector<double>*, 8> columns{};
        for (std::size_t index = 0; index < RESULTANTS.size(); ++index)
        {
            const auto found = flat.values.find(RESULTANTS[index]);
            columns[index] = found != flat.values.end() ? &found->second : nullptr;
        }
        std::array<std::vector<double>, 8> fields;
        for (std::vector<double>& field : fields)
            field.assign(rows, NOT_A_NUMBER);
        for (std::size_t row = 0; row < rows; ++row)
        {
            const ElementLaminate& laminate = laminates.of(flat.set, flat.eid[row]);
            if (!laminate.inverse || !laminate.sandwich)
                continue;
            std::array<double, 8> loads{};
            for (std::size_t index = 0; index < 8; ++index)
                loads[index] = columns[index] != nullptr ? (*columns[index])[row] : NOT_A_NUMBER;
            // the strains of the laminate in its material axes (the moments of the format are those of the theory with
            // the opposite sign), then the resultants of each face: A e0 + B k over its plies
            const std::array<double, 3> n = analysis::turnResultants({loads[0], loads[1], loads[2]}, laminate.angle);
            const std::array<double, 3> m = analysis::turnResultants({-loads[3], -loads[4], -loads[5]}, laminate.angle);
            const analysis::MidPlaneStrains strains = analysis::strainsOf(*laminate.inverse, n, m);
            const analysis::SandwichLayup& layup = *laminate.sandwich;
            const auto faceOf = [&](const analysis::SandwichFace& face)
            {
                std::array<double, 3> resultants{};
                for (std::size_t i = 0; i < 3; ++i)
                    for (std::size_t j = 0; j < 3; ++j)
                        resultants[i] +=
                            face.a[i * 3 + j] * strains.strain[j] + face.b[i * 3 + j] * strains.curvature[j];
                return material ? resultants : analysis::turnResultants(resultants, -laminate.angle);
            };
            const std::array<double, 3> bottom = faceOf(layup.bottom);
            const std::array<double, 3> top = faceOf(layup.top);
            // the transverse shears over the core: a vector of the plane, turned as one
            double qx = loads[6];
            double qy = loads[7];
            if (material)
            {
                const double angle = laminate.angle * std::numbers::pi / 180.0;
                const double turnedX = std::cos(angle) * qx + std::sin(angle) * qy;
                qy = -std::sin(angle) * qx + std::cos(angle) * qy;
                qx = turnedX;
            }
            for (std::size_t index = 0; index < 3; ++index)
            {
                fields[index][row] = bottom[index];
                fields[index + 3][row] = top[index];
            }
            fields[6][row] = qx / layup.coreThickness;
            fields[7][row] = qy / layup.coreThickness;
        }
        for (std::size_t index = 0; index < 8; ++index)
            parts.values[sandwichFields()[index]] = std::move(fields[index]);
        out.push_back(std::move(parts));
    }
    return out;
}

} // namespace oor::query
