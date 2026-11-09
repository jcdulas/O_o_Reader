#include "oor/analysis/mass/LaminateStrains.hpp"

#include "oor/analysis/mass/MassErrors.hpp"
#include "oor/analysis/mass/MaterialProperties.hpp"
#include "oor/model/cards/properties/shellProp/ShellPropTable.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <string>

namespace oor::analysis
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// The inverse of the n x n matrix `matrix` (row by row) by Gauss-Jordan elimination with partial pivoting; nothing when
// a pivot is negligible against the largest entry of the matrix (a singular matrix).
template <std::size_t N>
std::optional<std::array<double, N * N>> inverted(const std::array<double, N * N>& matrix)
{
    double largest = 0.0;
    for (const double value : matrix)
    {
        if (!std::isfinite(value))
            return std::nullopt;
        largest = std::max(largest, std::fabs(value));
    }
    if (largest == 0.0)
        return std::nullopt;
    const double negligible = largest * 1.0e-12;
    std::array<double, N * N> left = matrix;
    std::array<double, N * N> right{};
    for (std::size_t index = 0; index < N; ++index)
        right[index * N + index] = 1.0;
    for (std::size_t column = 0; column < N; ++column)
    {
        std::size_t pivot = column;
        for (std::size_t row = column + 1; row < N; ++row)
            if (std::fabs(left[row * N + column]) > std::fabs(left[pivot * N + column]))
                pivot = row;
        if (std::fabs(left[pivot * N + column]) <= negligible)
            return std::nullopt;
        if (pivot != column)
            for (std::size_t k = 0; k < N; ++k)
            {
                std::swap(left[pivot * N + k], left[column * N + k]);
                std::swap(right[pivot * N + k], right[column * N + k]);
            }
        const double scale = 1.0 / left[column * N + column];
        for (std::size_t k = 0; k < N; ++k)
        {
            left[column * N + k] *= scale;
            right[column * N + k] *= scale;
        }
        for (std::size_t row = 0; row < N; ++row)
        {
            if (row == column)
                continue;
            const double factor = left[row * N + column];
            if (factor == 0.0)
                continue;
            for (std::size_t k = 0; k < N; ++k)
            {
                left[row * N + k] -= factor * left[column * N + k];
                right[row * N + k] -= factor * right[column * N + k];
            }
        }
    }
    return right;
}

// The 3 x 3 block (row, column) of a 6 x 6 matrix.
std::array<double, 9> blockOf(const AbdMatrix& matrix, std::size_t row, std::size_t column)
{
    std::array<double, 9> block{};
    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t j = 0; j < 3; ++j)
            block[i * 3 + j] = matrix[(row * 3 + i) * 6 + column * 3 + j];
    return block;
}

void setBlock(AbdMatrix& matrix, std::size_t row, std::size_t column, const std::array<double, 9>& block)
{
    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t j = 0; j < 3; ++j)
            matrix[(row * 3 + i) * 6 + column * 3 + j] = block[i * 3 + j];
}

// The plane stress law of a material of the PSHELL, scaled; zeros for a blank material.
std::array<double, 9> lawOf(const model::Model& model, base::EntityId pid, base::EntityId mid, double scale)
{
    std::array<double, 9> law{};
    if (mid <= 0)
        return law;
    const std::optional<PlaneMaterial> material = MaterialProperties::planeStress(model, mid);
    if (!material)
        throw MassError("the " + std::string("ShellProp") + " " + std::to_string(pid) + " has the material " +
                        std::to_string(mid) + ", which does not exist or is not a " +
                        MaterialProperties::planeStressCards(model));
    for (std::size_t index = 0; index < 9; ++index)
        law[index] = material->stiffness[index] * scale;
    return law;
}

// The in-plane rotation of a plane tensor by `angleInDegrees`: the tensor given as (xx, yy, xy), `engineering` when xy
// is an engineering shear (twice the tensor component).
std::array<double, 3> turnedTensor(const std::array<double, 3>& tensor, double angleInDegrees, bool engineering)
{
    const double angle = angleInDegrees * std::numbers::pi / 180.0;
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    const double xy = engineering ? tensor[2] / 2.0 : tensor[2];
    const double xx = tensor[0] * c * c + tensor[1] * s * s + 2.0 * xy * s * c;
    const double yy = tensor[0] * s * s + tensor[1] * c * c - 2.0 * xy * s * c;
    const double turnedXy = (tensor[1] - tensor[0]) * s * c + xy * (c * c - s * s);
    return {xx, yy, engineering ? 2.0 * turnedXy : turnedXy};
}

} // namespace

AbdMatrix abdOf(const LaminateStiffness& stiffness) noexcept
{
    AbdMatrix abd{};
    setBlock(abd, 0, 0, stiffness.a);
    setBlock(abd, 0, 1, stiffness.b);
    setBlock(abd, 1, 0, stiffness.b);
    setBlock(abd, 1, 1, stiffness.d);
    return abd;
}

AbdMatrix invertAbd(const AbdMatrix& abd) noexcept
{
    if (const auto whole = inverted<6>(abd))
        return *whole;
    AbdMatrix result;
    result.fill(NOT_A_NUMBER);
    const std::array<double, 9> coupling = blockOf(abd, 0, 1);
    const bool uncoupled = std::all_of(coupling.begin(), coupling.end(), [](double value) { return value == 0.0; });
    if (!uncoupled)
        return result;
    // a membrane-only or bending-only property: each block alone, the singular one NaN
    std::array<double, 9> zero{};
    setBlock(result, 0, 1, zero);
    setBlock(result, 1, 0, zero);
    if (const auto membrane = inverted<3>(blockOf(abd, 0, 0)))
        setBlock(result, 0, 0, *membrane);
    if (const auto bending = inverted<3>(blockOf(abd, 1, 1)))
        setBlock(result, 1, 1, *bending);
    return result;
}

MidPlaneStrains strainsOf(const AbdMatrix& inverse, const std::array<double, 3>& n,
                          const std::array<double, 3>& m) noexcept
{
    const std::array<double, 6> loads = {n[0], n[1], n[2], m[0], m[1], m[2]};
    // a row of the inverse that is not defined (NaN) gives NaN, the others their strains
    std::array<double, 6> found{};
    for (std::size_t row = 0; row < 6; ++row)
    {
        double sum = 0.0;
        for (std::size_t column = 0; column < 6; ++column)
            sum += inverse[row * 6 + column] * loads[column];
        found[row] = sum;
    }
    return MidPlaneStrains{{found[0], found[1], found[2]}, {found[3], found[4], found[5]}};
}

std::optional<LaminateStiffness> shellStiffness(const model::Model& model, model::CardLocation property)
{
    if (Laminate::supports(property.table))
        return Laminate::of(model, property);
    if (property.table != model::typeIndexOf<model::ShellPropTable>() || model.findTable(property.table) == nullptr)
        return std::nullopt;
    const model::ShellPropCard card =
        static_cast<const model::ShellPropTable&>(*model.findTable(property.table)).card(property.row);
    LaminateStiffness result;
    const double t = card.thickness;
    result.totalThickness = t;
    result.a = lawOf(model, card.id, card.membraneMatId, t);
    // B = -T^2 G4: the plate moments of the solver are -int sigma z dz, those of the lamination theory +int sigma z dz
    // (a companion package D-319, D-597), so the coupling of the card turns sign; A and D do not (an even power of z)
    result.b = lawOf(model, card.id, card.couplingMatId, -t * t);
    result.d = lawOf(model, card.id, card.bendingMatId, card.bendingStiffnessRatio * t * t * t / 12.0);
    if (card.shearMatId > 0)
    {
        const std::optional<PlaneMaterial> material = MaterialProperties::planeStress(model, card.shearMatId);
        if (!material)
            throw MassError("the " + std::string("ShellProp") + " " + std::to_string(card.id) + " has the material " +
                            std::to_string(card.shearMatId) + ", which does not exist or is not a " +
                            MaterialProperties::planeStressCards(model));
        const double scale = card.shearThicknessRatio * t;
        result.g = {material->shear[0] * scale, 0.0, 0.0, material->shear[1] * scale};
    }
    if (card.membraneMatId > 0)
    {
        if (const auto density = MaterialProperties::density(model, card.membraneMatId))
            result.massPerArea = *density * t;
    }
    return result;
}

double strainAlong(double ex, double ey, double gxy, double angleInDegrees) noexcept
{
    const double angle = angleInDegrees * std::numbers::pi / 180.0;
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    return ex * c * c + ey * s * s + gxy * s * c;
}

std::array<double, 3> turnStrains(const std::array<double, 3>& strains, double angleInDegrees) noexcept
{
    return turnedTensor(strains, angleInDegrees, true);
}

std::array<double, 3> turnResultants(const std::array<double, 3>& resultants, double angleInDegrees) noexcept
{
    return turnedTensor(resultants, angleInDegrees, false);
}

std::optional<SandwichLayup> sandwichLayup(const model::Model& model, model::CardLocation property,
                                           std::optional<std::size_t> corePly)
{
    if (!Laminate::supports(property.table))
        return std::nullopt;
    const std::vector<LaminatePly> plies = Laminate::stack(model, property);
    if (plies.empty())
        return std::nullopt;
    std::size_t core = 0;
    if (corePly)
    {
        if (*corePly < 1 || *corePly > plies.size())
            return std::nullopt;
        core = *corePly - 1;
    }
    else
    {
        for (std::size_t index = 1; index < plies.size(); ++index)
            if (plies[index].thickness > plies[core].thickness)
                core = index;
    }
    SandwichLayup result;
    result.corePly = core + 1;
    result.coreMaterial = plies[core].materialId;
    result.coreThickness = plies[core].thickness;
    result.coreZBottom = plies[core].zBottom;
    result.coreZTop = plies[core].zTop;
    if (const std::optional<PlaneMaterial> material = MaterialProperties::planeStress(model, plies[core].materialId))
        result.coreShear = material->shear;
    else
        throw MassError("the core ply of the laminate has the material " + std::to_string(plies[core].materialId) +
                        ", which does not exist or is not a " + MaterialProperties::planeStressCards(model));
    const auto addTo = [&](SandwichFace& face, const LaminatePly& ply)
    {
        const std::optional<PlaneMaterial> material = MaterialProperties::planeStress(model, ply.materialId);
        if (!material)
            throw MassError("a face ply of the laminate has the material " + std::to_string(ply.materialId) +
                            ", which does not exist or is not a " + MaterialProperties::planeStressCards(model));
        if (face.plies == 0)
            face.zBottom = ply.zBottom;
        face.zTop = ply.zTop;
        ++face.plies;
        face.thickness += ply.thickness;
        const std::array<double, 9> stiffness = Laminate::turnedStiffness(material->stiffness, ply.angleInDegrees);
        const double dz = ply.zTop - ply.zBottom;
        const double dz2 = (ply.zTop * ply.zTop - ply.zBottom * ply.zBottom) / 2.0;
        for (std::size_t index = 0; index < 9; ++index)
        {
            face.a[index] += stiffness[index] * dz;
            face.b[index] += stiffness[index] * dz2;
        }
    };
    for (std::size_t index = 0; index < plies.size(); ++index)
    {
        if (index < core)
            addTo(result.bottom, plies[index]);
        else if (index > core)
            addTo(result.top, plies[index]);
    }
    return result;
}

} // namespace oor::analysis
