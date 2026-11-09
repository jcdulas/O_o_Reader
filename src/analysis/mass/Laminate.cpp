#include "oor/analysis/mass/Laminate.hpp"

#include "oor/analysis/mass/MassErrors.hpp"
#include "oor/analysis/mass/MaterialProperties.hpp"
#include "oor/model/cards/properties/compProp/CompPropTable.hpp"
#include "oor/model/cards/properties/compPropGlobal/CompPropGlobalTable.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <string>

namespace oor::analysis
{

namespace
{

using Matrix3 = std::array<double, 9>;

// T times S, both 3 x 3 row by row.
Matrix3 multiply(const Matrix3& left, const Matrix3& right)
{
    Matrix3 product{};
    for (std::size_t row = 0; row < 3; ++row)
    {
        for (std::size_t column = 0; column < 3; ++column)
        {
            double sum = 0.0;
            for (std::size_t k = 0; k < 3; ++k)
                sum += left[row * 3 + k] * right[k * 3 + column];
            product[row * 3 + column] = sum;
        }
    }
    return product;
}

// The stiffness of a ply turned by `angle` (degrees, from the x axis of the element to the fibre direction): the
// stress transformation back from the ply axes times Q times the strain transformation into them.
Matrix3 turned(const Matrix3& stiffness, double angleInDegrees)
{
    const double angle = angleInDegrees * std::numbers::pi / 180.0;
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    const Matrix3 stressBack = {c * c, s * s, -2.0 * c * s, s * s, c * c, 2.0 * c * s, c * s, -c * s, c * c - s * s};
    const Matrix3 strainInto = {c * c, s * s, c * s, s * s, c * c, -c * s, -2.0 * c * s, 2.0 * c * s, c * c - s * s};
    return multiply(multiply(stressBack, stiffness), strainInto);
}

// The 2 x 2 transverse shear matrix of a ply turned by `angle`: R^T diag(G1z, G2z) R.
std::array<double, 4> turnedShear(const std::array<double, 2>& moduli, double angleInDegrees)
{
    const double angle = angleInDegrees * std::numbers::pi / 180.0;
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    return {c * c * moduli[0] + s * s * moduli[1], c * s * (moduli[0] - moduli[1]), c * s * (moduli[0] - moduli[1]),
            s * s * moduli[0] + c * c * moduli[1]};
}

template <typename Ply>
void stackPlies(const std::vector<Ply>& given, double z0, bool mirror, std::vector<LaminatePly>& result)
{
    std::vector<LaminatePly> ordered;
    for (const Ply& ply : given)
        ordered.push_back(LaminatePly{ply.matId, ply.thickness, ply.thetaInDegrees, 0.0, 0.0});
    if (mirror)
    {
        // A symmetric laminate lists the plies from the bottom to the middle plane; the others are the same ones in the
        // opposite order.
        for (std::size_t index = given.size(); index-- > 0;)
            ordered.push_back(
                LaminatePly{given[index].matId, given[index].thickness, given[index].thetaInDegrees, 0.0, 0.0});
    }
    double z = z0;
    for (LaminatePly& ply : ordered)
    {
        ply.zBottom = z;
        z += ply.thickness;
        ply.zTop = z;
    }
    result = std::move(ordered);
}

} // namespace

std::array<double, 9> Laminate::turnedStiffness(const std::array<double, 9>& stiffness, double angleInDegrees)
{
    return turned(stiffness, angleInDegrees);
}

bool Laminate::supports(std::uint16_t propertyType) noexcept
{
    return propertyType == model::typeIndexOf<model::CompPropTable>() ||
           propertyType == model::typeIndexOf<model::CompPropGlobalTable>();
}

std::vector<LaminatePly> Laminate::stack(const model::Model& model, model::CardLocation location)
{
    std::vector<LaminatePly> result;
    const model::CardTable* table = model.findTable(location.table);
    if (table == nullptr)
        return result;
    if (location.table == model::typeIndexOf<model::CompPropTable>())
    {
        const model::CompPropCard card = static_cast<const model::CompPropTable&>(*table).card(location.row);
        stackPlies(card.plies, card.bottomOffset, card.option == model::LaminateOption::Sym, result);
    }
    else if (location.table == model::typeIndexOf<model::CompPropGlobalTable>())
    {
        const model::CompPropGlobalCard card =
            static_cast<const model::CompPropGlobalTable&>(*table).card(location.row);
        stackPlies(card.plies, card.bottomOffset, false, result);
    }
    return result;
}

std::optional<LaminateStiffness> Laminate::of(const model::Model& model, model::CardLocation location)
{
    if (!supports(location.table) || model.findTable(location.table) == nullptr)
        return std::nullopt;
    model::LaminateOption option = model::LaminateOption::None;
    base::EntityId pid = 0;
    bool bottomOffsetGiven = true;
    if (location.table == model::typeIndexOf<model::CompPropTable>())
    {
        const model::CompPropCard card =
            static_cast<const model::CompPropTable&>(*model.findTable(location.table)).card(location.row);
        option = card.option;
        pid = card.id;
        bottomOffsetGiven = card.bottomOffsetGiven;
    }
    else
    {
        const model::CompPropGlobalCard card =
            static_cast<const model::CompPropGlobalTable&>(*model.findTable(location.table)).card(location.row);
        option = card.option;
        pid = card.id;
        bottomOffsetGiven = card.bottomOffsetGiven;
    }
    LaminateStiffness result;
    bool rigidInShear = false; // a ply without transverse shear flexibility
    result.plies = stack(model, location);
    // the plies of the card each layer is (a SYM laminate lists the bottom half: the top half mirrors it)
    const std::size_t layers = result.plies.size();
    const bool mirrored = option == model::LaminateOption::Sym;
    result.plyCount = mirrored ? layers / 2 : layers;
    for (std::size_t layer = 0; layer < layers; ++layer)
        result.plyOf.push_back(
            static_cast<std::uint32_t>(mirrored && layer >= result.plyCount ? layers - 1 - layer : layer));
    for (const LaminatePly& ply : result.plies)
        result.totalThickness += ply.thickness;
    for (const LaminatePly& ply : result.plies)
    {
        const std::optional<PlaneMaterial> material = MaterialProperties::planeStress(model, ply.materialId);
        if (!material)
        {
            throw MassError("the laminate of property " + std::to_string(pid) + " has a ply of material " +
                            std::to_string(ply.materialId) + ", which does not exist or is not a " +
                            MaterialProperties::planeStressCards(model));
        }
        result.massPerArea += material->density * ply.thickness;
        const Matrix3 stiffness = turned(material->stiffness, ply.angleInDegrees);
        result.layerStiffness.push_back(stiffness);
        const double infinite = std::numeric_limits<double>::infinity();
        result.layerShear.push_back(std::isinf(material->shear[0]) || std::isinf(material->shear[1])
                                        ? std::array<double, 4>{infinite, 0.0, 0.0, infinite}
                                        : turnedShear(material->shear, ply.angleInDegrees));
        const double dz = ply.zTop - ply.zBottom;
        const double dz2 = (ply.zTop * ply.zTop - ply.zBottom * ply.zBottom) / 2.0;
        const double dz3 = (ply.zTop * ply.zTop * ply.zTop - ply.zBottom * ply.zBottom * ply.zBottom) / 3.0;
        for (std::size_t index = 0; index < 9; ++index)
        {
            result.a[index] += stiffness[index] * dz;
            result.b[index] += stiffness[index] * dz2;
            result.d[index] += stiffness[index] * dz3;
        }
        if (std::isinf(material->shear[0]) || std::isinf(material->shear[1]))
        {
            rigidInShear = true;
            continue;
        }
        const std::array<double, 4> shear = turnedShear(material->shear, ply.angleInDegrees);
        for (std::size_t index = 0; index < 4; ++index)
            result.g[index] += Laminate::SHEAR_FACTOR * shear[index] * ply.thickness;
    }
    if (rigidInShear)
    {
        const double infinite = std::numeric_limits<double>::infinity();
        result.g = {infinite, 0.0, 0.0, infinite};
    }
    switch (option)
    {
    case model::LaminateOption::Mem:
        result.b.fill(0.0);
        result.d.fill(0.0);
        break;
    case model::LaminateOption::Bend:
        result.a.fill(0.0);
        result.b.fill(0.0);
        break;
    case model::LaminateOption::Smear:
    case model::LaminateOption::SmCore:
        result.a.fill(std::numeric_limits<double>::quiet_NaN());
        result.b.fill(std::numeric_limits<double>::quiet_NaN());
        result.d.fill(std::numeric_limits<double>::quiet_NaN());
        result.g.fill(std::numeric_limits<double>::quiet_NaN());
        break;
    case model::LaminateOption::None:
    case model::LaminateOption::Sym:
        break;
    }
    result.bottomOffsetGiven = bottomOffsetGiven;
    return result;
}

std::optional<LaminateStiffness> Laminate::of(const model::Model& model, model::CardLocation location, bool gradient)
{
    return of(model, location, gradient, false);
}

std::optional<LaminateStiffness> Laminate::of(const model::Model& model, model::CardLocation location, bool gradient,
                                              bool hessian)
{
    std::optional<LaminateStiffness> result = of(model, location);
    if (!result || !(gradient || hessian))
        return result;
    std::vector<double> thickness;
    for (const LaminatePly& ply : result->plies)
        thickness.push_back(ply.thickness);
    const std::optional<double> z0 = result->bottomOffsetGiven && !result->plies.empty()
                                         ? std::optional<double>(result->plies.front().zBottom)
                                         : std::nullopt;
    LaminateMatrices derived = matrices(result->layerStiffness, result->layerShear, thickness, result->plyOf,
                                        result->plyCount, z0, true, hessian);
    // the option of the card, as for the matrices: a matrix it zeroes has no derivative, a NaN one NaN ones
    const auto zeroed = [](const std::array<double, 9>& matrix)
    { return std::all_of(matrix.begin(), matrix.end(), [](double value) { return value == 0.0; }); };
    const auto nan = [](const std::array<double, 9>& matrix) { return matrix[0] != matrix[0]; };
    const auto adjust = [&](const std::array<double, 9>& matrix, const std::array<double, 9>& computed,
                            std::vector<std::array<double, 9>>& derivatives)
    {
        // MEM zeroes B and D, BEND A and B, SMEAR and SMCORE make every matrix NaN
        if (nan(matrix))
            for (auto& entry : derivatives)
                entry.fill(std::numeric_limits<double>::quiet_NaN());
        else if (zeroed(matrix) && !zeroed(computed))
            for (auto& entry : derivatives)
                entry.fill(0.0);
    };
    adjust(result->a, derived.a, derived.aDerivatives);
    adjust(result->b, derived.b, derived.bDerivatives);
    adjust(result->d, derived.d, derived.dDerivatives);
    adjust(result->a, derived.a, derived.aHessian);
    adjust(result->b, derived.b, derived.bHessian);
    adjust(result->d, derived.d, derived.dHessian);
    result->aHessian = std::move(derived.aHessian);
    result->bHessian = std::move(derived.bHessian);
    result->dHessian = std::move(derived.dHessian);
    if (result->g[0] != result->g[0])
        for (auto& entry : derived.gDerivatives)
            entry.fill(std::numeric_limits<double>::quiet_NaN());
    result->aDerivatives = std::move(derived.aDerivatives);
    result->bDerivatives = std::move(derived.bDerivatives);
    result->dDerivatives = std::move(derived.dDerivatives);
    result->gDerivatives = std::move(derived.gDerivatives);
    return result;
}

namespace
{

// The second derivatives of A, B and D with respect to the thicknesses of two plies. The faces of
// the layers are affine in the thicknesses: with the thickness of layer m, the top of layer k moves by s + [m <= k]
// and its bottom by s + [m < k], s the slope of the bottom of the stack (0 for a given Z0, -1/2 for a blank one). So
// A is linear, and for two layers m <= n
//   d2 B / dt_m dt_n = sum_k Q_k (c_top(k) - c_bottom(k)),
//   d2 D / dt_m dt_n = 2 sum_k Q_k (c_top(k) top_k - c_bottom(k) bottom_k),
// c_top(k) = (s + [m <= k]) (s + [n <= k]), c_bottom(k) = (s + [m < k]) (s + [n < k]). The two factors are equal
// but at k = m and k = n (s^2 below m, s (s + 1) between, (s + 1)^2 above n), so B's is s Q_m + (s + 1) Q_n
// ((2 s + 1) Q_m for m = n) and D's is the sum of those factors times Q_k t_k by ranges of k, with the terms of the
// layers m and n: linear in the layers for each pair.
void addHessians(LaminateMatrices& result, std::span<const std::array<double, 9>> stiffness,
                 std::span<const double> thickness, const std::vector<double>& bottoms, const std::vector<double>& tops,
                 std::span<const std::uint32_t> plyOf, std::size_t plyCount, double s)
{
    const std::size_t layers = thickness.size();
    result.aHessian.assign(plyCount * plyCount, {});
    result.bHessian.assign(plyCount * plyCount, {});
    result.dHessian.assign(plyCount * plyCount, {});
    // the sums of Q_k t_k over the layers below each one
    std::vector<std::array<double, 9>> below(layers + 1, std::array<double, 9>{});
    for (std::size_t layer = 0; layer < layers; ++layer)
        for (std::size_t index = 0; index < 9; ++index)
            below[layer + 1][index] = below[layer][index] + stiffness[layer][index] * thickness[layer];
    const double low = s * s;
    const double middle = s * (s + 1.0);
    const double high = (s + 1.0) * (s + 1.0);
    const auto range = [&](std::size_t from, std::size_t to, std::size_t index)
    { return to > from ? below[to][index] - below[from][index] : 0.0; };
    for (std::size_t m = 0; m < layers; ++m)
        for (std::size_t n = m; n < layers; ++n)
        {
            const std::size_t first = plyOf[m];
            const std::size_t second = plyOf[n];
            for (std::size_t index = 0; index < 9; ++index)
            {
                const double qm = stiffness[m][index];
                const double qn = stiffness[n][index];
                double b = 0.0;
                double d = 0.0;
                if (m == n)
                {
                    b = (2.0 * s + 1.0) * qm;
                    d = 2.0 * (low * range(0, m, index) + (high * tops[m] - low * bottoms[m]) * qm +
                               high * range(m + 1, layers, index));
                }
                else
                {
                    b = s * qm + (s + 1.0) * qn;
                    d = 2.0 * (low * range(0, m, index) + (middle * tops[m] - low * bottoms[m]) * qm +
                               middle * range(m + 1, n, index) + (high * tops[n] - middle * bottoms[n]) * qn +
                               high * range(n + 1, layers, index));
                }
                // layer pairs (m, n) and (n, m) both, on the plies they are
                result.bHessian[first * plyCount + second][index] += b;
                result.dHessian[first * plyCount + second][index] += d;
                if (m != n)
                {
                    result.bHessian[second * plyCount + first][index] += b;
                    result.dHessian[second * plyCount + first][index] += d;
                }
            }
        }
}

} // namespace

LaminateMatrices Laminate::matrices(std::span<const std::array<double, 9>> stiffness,
                                    std::span<const std::array<double, 4>> shear, std::span<const double> thickness,
                                    std::span<const std::uint32_t> plyOf, std::size_t plyCount,
                                    std::optional<double> z0, bool gradient, bool hessian)
{
    LaminateMatrices result;
    const std::size_t layers = thickness.size();
    double total = 0.0;
    for (const double value : thickness)
        total += value;
    // the derivative of the bottom of the stack with respect to the thickness of a layer
    const double bottomSlope = z0 ? 0.0 : -0.5;
    double z = z0.value_or(-total / 2.0);
    bool rigidInShear = false;
    std::vector<double> bottoms(layers);
    std::vector<double> tops(layers);
    for (std::size_t layer = 0; layer < layers; ++layer)
    {
        bottoms[layer] = z;
        z += thickness[layer];
        tops[layer] = z;
        const double bottom = bottoms[layer];
        const double top = tops[layer];
        for (std::size_t index = 0; index < 9; ++index)
        {
            result.a[index] += stiffness[layer][index] * (top - bottom);
            result.b[index] += stiffness[layer][index] * (top * top - bottom * bottom) / 2.0;
            result.d[index] += stiffness[layer][index] * (top * top * top - bottom * bottom * bottom) / 3.0;
        }
        if (layer < shear.size())
        {
            if (std::isinf(shear[layer][0]) || std::isinf(shear[layer][3]))
                rigidInShear = true;
            else
                for (std::size_t index = 0; index < 4; ++index)
                    result.g[index] += SHEAR_FACTOR * shear[layer][index] * thickness[layer];
        }
    }
    if (rigidInShear)
    {
        const double infinite = std::numeric_limits<double>::infinity();
        result.g = {infinite, 0.0, 0.0, infinite};
    }
    if (!gradient)
        return result;
    result.aDerivatives.assign(plyCount, {});
    result.bDerivatives.assign(plyCount, {});
    result.dDerivatives.assign(plyCount, {});
    result.gDerivatives.assign(plyCount, {});
    // the thickness of layer j moves the bottom of layer k by bottomSlope + [j < k] and its top by bottomSlope +
    // [j <= k]
    for (std::size_t moved = 0; moved < layers; ++moved)
    {
        const std::size_t ply = plyOf[moved];
        for (std::size_t layer = 0; layer < layers; ++layer)
        {
            const double bottom = bottoms[layer];
            const double top = tops[layer];
            const double bottomMove = bottomSlope + (moved < layer ? 1.0 : 0.0);
            const double topMove = bottomSlope + (moved <= layer ? 1.0 : 0.0);
            for (std::size_t index = 0; index < 9; ++index)
            {
                const double q = stiffness[layer][index];
                result.aDerivatives[ply][index] += q * (topMove - bottomMove);
                result.bDerivatives[ply][index] += q * (top * topMove - bottom * bottomMove);
                result.dDerivatives[ply][index] += q * (top * top * topMove - bottom * bottom * bottomMove);
            }
        }
        if (!rigidInShear && moved < shear.size())
            for (std::size_t index = 0; index < 4; ++index)
                result.gDerivatives[ply][index] += SHEAR_FACTOR * shear[moved][index];
    }
    if (hessian)
        addHessians(result, stiffness, thickness, bottoms, tops, plyOf, plyCount, bottomSlope);
    return result;
}

std::optional<LaminateStiffness> Laminate::of(const model::Model& model, base::EntityId pid)
{
    const auto location = model.locate(base::IdSpace::Property, pid);
    if (!location)
        return std::nullopt;
    return of(model, *location);
}

} // namespace oor::analysis
