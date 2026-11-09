#include "oor/analysis/mass/LaminationParameters.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <string>

namespace oor::analysis
{

namespace
{

constexpr double RADIANS = std::numbers::pi / 180.0;
constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// f(theta) and its first and second derivatives with respect to theta in radians.
struct AngleTerms
{
    std::array<double, 4> f{};
    std::array<double, 4> first{};
    std::array<double, 4> second{};
};

AngleTerms angleTerms(double degrees) noexcept
{
    const double theta = degrees * RADIANS;
    const double c2 = std::cos(2.0 * theta);
    const double s2 = std::sin(2.0 * theta);
    const double c4 = std::cos(4.0 * theta);
    const double s4 = std::sin(4.0 * theta);
    return {
        {c2, s2, c4, s4}, {-2.0 * s2, 2.0 * c2, -4.0 * s4, 4.0 * c4}, {-4.0 * c2, -4.0 * s2, -16.0 * c4, -16.0 * s4}};
}

// z^p and its first two derivatives, p = 1, 2, 3.
double power(double z, int p) noexcept
{
    return p == 1 ? z : (p == 2 ? z * z : z * z * z);
}
double powerSlope(double z, int p) noexcept
{
    return p == 1 ? 1.0 : (p == 2 ? 2.0 * z : 3.0 * z * z);
}
double powerCurvature(double z, int p) noexcept
{
    return p == 1 ? 0.0 : (p == 2 ? 2.0 : 6.0 * z);
}

} // namespace

MaterialInvariants materialInvariants(double e1, double e2, double nu12, double g12) noexcept
{
    const double nu21 = nu12 * e2 / e1;
    const double divisor = 1.0 - nu12 * nu21;
    MaterialInvariants result;
    result.q11 = e1 / divisor;
    result.q22 = e2 / divisor;
    result.q12 = nu12 * e2 / divisor;
    result.q66 = g12;
    const double q11 = result.q11;
    const double q22 = result.q22;
    const double q12 = result.q12;
    const double q66 = result.q66;
    result.u = {(3.0 * q11 + 3.0 * q22 + 2.0 * q12 + 4.0 * q66) / 8.0, (q11 - q22) / 2.0,
                (q11 + q22 - 2.0 * q12 - 4.0 * q66) / 8.0, (q11 + q22 + 6.0 * q12 - 4.0 * q66) / 8.0,
                (q11 + q22 - 2.0 * q12 + 4.0 * q66) / 8.0};
    return result;
}

std::array<double, 45> invariantMatrices(const std::array<double, 5>& u) noexcept
{
    const double u1 = u[0];
    const double u2 = u[1];
    const double u3 = u[2];
    const double u4 = u[3];
    const double u5 = u[4];
    return {u1,  u4,  0.0,      u4,  u1,  0.0,      0.0,      0.0,      u5,   // G0
            u2,  0.0, 0.0,      0.0, -u2, 0.0,      0.0,      0.0,      0.0,  // G1: cos 2 theta
            0.0, 0.0, u2 / 2.0, 0.0, 0.0, u2 / 2.0, u2 / 2.0, u2 / 2.0, 0.0,  // G2: sin 2 theta
            u3,  -u3, 0.0,      -u3, u3,  0.0,      0.0,      0.0,      -u3,  // G3: cos 4 theta
            0.0, 0.0, u3,       0.0, 0.0, -u3,      u3,       -u3,      0.0}; // G4: sin 4 theta
}

void rowLaminationParameters(std::span<const double> angles, std::span<const double> thickness,
                             std::span<const double> kept, std::size_t rows, std::span<double> xi,
                             std::span<double> totals)
{
    const std::size_t layers = angles.size();
    std::vector<AngleTerms> terms(layers);
    for (std::size_t k = 0; k < layers; ++k)
        terms[k] = angleTerms(angles[k]);
    std::vector<double> layerThickness(layers);
    std::vector<double> node(layers + 1);
    for (std::size_t row = 0; row < rows; ++row)
    {
        // the arithmetic of stackLaminationParameters on the row's layers
        double h = 0.0;
        for (std::size_t k = 0; k < layers; ++k)
        {
            layerThickness[k] = thickness[k] * kept[row * layers + k];
            if (!(layerThickness[k] >= 0.0))
                throw std::invalid_argument("a ply thickness is negative or not a number (row " + std::to_string(row) +
                                            ")");
            h += layerThickness[k];
        }
        if (!(h > 0.0) || !std::isfinite(h))
            throw std::invalid_argument("the stack has no thickness (row " + std::to_string(row) + ")");
        double below = 0.0;
        for (std::size_t k = 0; k <= layers; ++k)
        {
            const double share = k == layers ? 1.0 : below / h;
            node[k] = k == 0 ? -1.0 : (k == layers ? 1.0 : 2.0 * share - 1.0);
            if (k < layers)
                below += layerThickness[k];
        }
        std::array<double, 12> values{};
        for (int x = 0; x < 3; ++x)
            for (std::size_t k = 0; k < layers; ++k)
            {
                const double rise = power(node[k + 1], x + 1) - power(node[k], x + 1);
                for (std::size_t i = 0; i < 4; ++i)
                    values[static_cast<std::size_t>(x) * 4 + i] += 0.5 * terms[k].f[i] * rise;
            }
        std::copy(values.begin(), values.end(), xi.begin() + static_cast<std::ptrdiff_t>(12 * row));
        totals[row] = h;
    }
}

StackLaminationParameters stackLaminationParameters(std::span<const double> angles, std::span<const double> thickness,
                                                    std::span<const std::uint32_t> plyOf, std::size_t plyCount,
                                                    bool gradient, bool hessian)
{
    gradient = gradient || hessian;
    const std::size_t layers = angles.size();
    if (thickness.size() != layers || plyOf.size() != layers)
        throw std::invalid_argument("the angles, the thicknesses and the plies of the layers differ in number");
    double h = 0.0;
    for (const double value : thickness)
    {
        if (!(value >= 0.0))
            throw std::invalid_argument("a ply thickness is negative or not a number");
        h += value;
    }
    if (!(h > 0.0) || !std::isfinite(h))
        throw std::invalid_argument("the stack has no thickness");
    StackLaminationParameters result;
    result.thickness = h;
    // the nodes between the layers in z' (the first -1, the last 1), and u = s / h
    std::vector<double> node(layers + 1);
    std::vector<double> share(layers + 1);
    double below = 0.0;
    for (std::size_t k = 0; k <= layers; ++k)
    {
        share[k] = k == layers ? 1.0 : below / h;
        node[k] = k == 0 ? -1.0 : (k == layers ? 1.0 : 2.0 * share[k] - 1.0);
        if (k < layers)
            below += thickness[k];
    }
    std::vector<AngleTerms> terms(layers);
    for (std::size_t k = 0; k < layers; ++k)
        terms[k] = angleTerms(angles[k]);
    for (int x = 0; x < 3; ++x)
        for (std::size_t k = 0; k < layers; ++k)
        {
            const double rise = power(node[k + 1], x + 1) - power(node[k], x + 1);
            for (std::size_t i = 0; i < 4; ++i)
                result.xi[static_cast<std::size_t>(x) * 4 + i] += 0.5 * terms[k].f[i] * rise;
        }
    if (!gradient)
        return result;

    const std::size_t plies = plyCount;
    result.dAngle.assign(12 * plies, 0.0);
    result.dThickness.assign(12 * plies, 0.0);
    if (hessian)
    {
        result.hAngleAngle.assign(12 * plies * plies, 0.0);
        result.hAngleThickness.assign(12 * plies * plies, 0.0);
        result.hThicknessThickness.assign(12 * plies * plies, 0.0);
    }
    // the derivative of node n with respect to the thickness of layer j: 2 ([j < n] - u_n) / h (end nodes fixed)
    const auto nodeSlope = [&](std::size_t n, std::size_t j)
    { return n == 0 || n == layers ? 0.0 : 2.0 * ((j < n ? 1.0 : 0.0) - share[n]) / h; };
    std::vector<double> layerThickness(layers);
    std::vector<double> suffix(layers + 2);
    std::vector<double> suffixShare(layers + 2);
    std::vector<double> curvatureSuffix(layers + 2);
    std::vector<double> curvatureShare(layers + 2);
    std::vector<double> layerHessian(hessian ? layers * layers : 0);
    for (int x = 0; x < 3; ++x)
    {
        const int p = x + 1;
        for (std::size_t i = 0; i < 4; ++i)
        {
            const std::size_t c = static_cast<std::size_t>(x) * 4 + i;
            // angles: 1/2 f'(theta_k) (g(z_k+1) - g(z_k)), the second derivative on the diagonal
            for (std::size_t k = 0; k < layers; ++k)
            {
                const double rise = power(node[k + 1], p) - power(node[k], p);
                result.dAngle[c * plies + plyOf[k]] += 0.5 * terms[k].first[i] * RADIANS * rise;
                if (hessian)
                    result.hAngleAngle[(c * plies + plyOf[k]) * plies + plyOf[k]] +=
                        0.5 * terms[k].second[i] * RADIANS * RADIANS * rise;
            }
            // thicknesses: the interior nodes n = 1 ... L - 1, alpha_n = 1/2 (f_(n-1) - f_n)
            // d xi / d t_j = sum_n alpha_n g'(z_n) 2 ([j < n] - u_n) / h
            std::fill(suffix.begin(), suffix.end(), 0.0);
            std::fill(suffixShare.begin(), suffixShare.end(), 0.0);
            std::fill(curvatureSuffix.begin(), curvatureSuffix.end(), 0.0);
            std::fill(curvatureShare.begin(), curvatureShare.end(), 0.0);
            double slopeShare = 0.0;      // sum_n gamma_n u_n, gamma_n = alpha_n g'(z_n)
            double curvatureShare2 = 0.0; // sum_n beta_n u_n^2, beta_n = alpha_n g''(z_n)
            for (std::size_t n = layers - 1; n >= 1; --n)
            {
                const double alpha = 0.5 * (terms[n - 1].f[i] - terms[n].f[i]);
                const double gamma = alpha * powerSlope(node[n], p);
                const double beta = alpha * powerCurvature(node[n], p);
                suffix[n] = suffix[n + 1] + gamma;
                curvatureSuffix[n] = curvatureSuffix[n + 1] + beta;
                curvatureShare[n] = curvatureShare[n + 1] + beta * share[n];
                slopeShare += gamma * share[n];
                curvatureShare2 += beta * share[n] * share[n];
                if (n == 1)
                    break;
            }
            for (std::size_t j = 0; j < layers; ++j)
            {
                // the nodes above layer j are n > j: suffix[j + 1]
                layerThickness[j] = 2.0 * (suffix[j + 1] - slopeShare) / h;
                result.dThickness[c * plies + plyOf[j]] += layerThickness[j];
            }
            if (!hessian)
                continue;
            // sum_n beta_n a_ni a_nj + gamma_n c_nij, a_ni = 2 ([i < n] - u_n) / h,
            // c_nij = -2 / h^2 ([i < n] + [j < n] - 2 u_n)
            for (std::size_t a = 0; a < layers; ++a)
                for (std::size_t b = 0; b < layers; ++b)
                {
                    const std::size_t top = std::max(a, b);
                    const double products =
                        curvatureSuffix[top + 1] - curvatureShare[a + 1] - curvatureShare[b + 1] + curvatureShare2;
                    const double second = suffix[a + 1] + suffix[b + 1] - 2.0 * slopeShare;
                    layerHessian[a * layers + b] = 4.0 / (h * h) * products - 2.0 / (h * h) * second;
                }
            for (std::size_t a = 0; a < layers; ++a)
                for (std::size_t b = 0; b < layers; ++b)
                    result.hThicknessThickness[(c * plies + plyOf[a]) * plies + plyOf[b]] +=
                        layerHessian[a * layers + b];
            // angle of layer k and thickness of layer j: 1/2 f'(theta_k) (g'(z_k+1) dz_k+1/dt_j - g'(z_k) dz_k/dt_j)
            for (std::size_t k = 0; k < layers; ++k)
            {
                const double factor = 0.5 * terms[k].first[i] * RADIANS;
                for (std::size_t j = 0; j < layers; ++j)
                {
                    const double moved =
                        powerSlope(node[k + 1], p) * nodeSlope(k + 1, j) - powerSlope(node[k], p) * nodeSlope(k, j);
                    result.hAngleThickness[(c * plies + plyOf[k]) * plies + plyOf[j]] += factor * moved;
                }
            }
        }
    }
    return result;
}

std::ptrdiff_t angleIndex(std::span<const double> angleSet, double angle) noexcept
{
    for (std::size_t index = 0; index < angleSet.size(); ++index)
    {
        double difference = std::fmod(angle - angleSet[index], 180.0);
        if (difference < 0.0)
            difference += 180.0;
        if (difference <= 1e-6 || difference >= 180.0 - 1e-6)
            return static_cast<std::ptrdiff_t>(index);
    }
    return -1;
}

LaminationAbd abdFromLaminationParameters(const std::array<double, 45>& gamma, double h, const double* xiA,
                                          const double* xiB, const double* xiD, std::size_t components, bool gradient,
                                          bool hessian)
{
    gradient = gradient || hessian;
    const bool withB = xiB != nullptr;
    const std::size_t n = components;
    const std::size_t offsetA = 1;
    const std::size_t offsetB = 1 + n;
    const std::size_t offsetD = withB ? 1 + 2 * n : 1 + n;
    const std::size_t count = offsetD + n;
    LaminationAbd result;
    std::array<double, 9> hatA{};
    std::array<double, 9> hatB{};
    std::array<double, 9> hatD{};
    for (std::size_t e = 0; e < 9; ++e)
    {
        hatA[e] = gamma[e];
        hatD[e] = gamma[e];
        for (std::size_t i = 0; i < n; ++i)
        {
            const double term = gamma[(i + 1) * 9 + e];
            hatA[e] += term * xiA[i];
            hatD[e] += term * xiD[i];
            if (withB)
                hatB[e] += term * xiB[i];
        }
        result.a[e] = h * hatA[e];
        result.b[e] = h * h / 4.0 * hatB[e];
        result.d[e] = h * h * h / 12.0 * hatD[e];
    }
    if (!gradient)
        return result;
    result.da.assign(9 * count, 0.0);
    result.db.assign(9 * count, 0.0);
    result.dd.assign(9 * count, 0.0);
    if (hessian)
    {
        result.ha.assign(9 * count * count, 0.0);
        result.hb.assign(9 * count * count, 0.0);
        result.hd.assign(9 * count * count, 0.0);
    }
    const auto both = [&](std::vector<double>& out, std::size_t e, std::size_t first, std::size_t second, double value)
    {
        out[(e * count + first) * count + second] = value;
        out[(e * count + second) * count + first] = value;
    };
    for (std::size_t e = 0; e < 9; ++e)
    {
        result.da[e * count] = hatA[e];
        result.db[e * count] = h / 2.0 * hatB[e];
        result.dd[e * count] = h * h / 4.0 * hatD[e];
        if (hessian)
        {
            result.hb[(e * count) * count] = hatB[e] / 2.0;
            result.hd[(e * count) * count] = h / 2.0 * hatD[e];
        }
        for (std::size_t i = 0; i < n; ++i)
        {
            const double term = gamma[(i + 1) * 9 + e];
            result.da[e * count + offsetA + i] = h * term;
            result.dd[e * count + offsetD + i] = h * h * h / 12.0 * term;
            if (withB)
                result.db[e * count + offsetB + i] = h * h / 4.0 * term;
            if (!hessian)
                continue;
            both(result.ha, e, 0, offsetA + i, term);
            both(result.hd, e, 0, offsetD + i, h * h / 4.0 * term);
            if (withB)
                both(result.hb, e, 0, offsetB + i, h / 2.0 * term);
        }
    }
    return result;
}

std::vector<std::uint32_t> feasibilityClasses(std::size_t angleCount)
{
    if (angleCount > MAX_FEASIBILITY_ANGLES)
        throw std::invalid_argument("the feasibility takes at most " + std::to_string(MAX_FEASIBILITY_ANGLES) +
                                    " angles");
    std::vector<std::uint32_t> classes;
    if (angleCount < 2)
        return classes;
    const std::uint32_t full = (std::uint32_t{1} << angleCount) - 1U;
    for (std::uint32_t mask = 1; mask < full; mask += 2) // the sets holding the first angle
        classes.push_back(mask);
    return classes;
}

void momentFeasibility(std::span<const double> v, std::span<const double> b, std::span<const double> w,
                       std::span<const std::uint32_t> classes, double* rows, double* slopes, double* curvatures)
{
    for (std::size_t index = 0; index < classes.size(); ++index)
    {
        double vs = 0.0;
        double bs = 0.0;
        double ws = 0.0;
        for (std::size_t angle = 0; angle < v.size(); ++angle)
        {
            if ((classes[index] >> angle & 1U) == 0U)
                continue;
            vs += v[angle];
            bs += b.empty() ? 0.0 : b[angle];
            ws += w[angle];
        }
        const double us = 1.0 - vs;
        // the lower row, then the upper row
        for (int upper = 0; upper < 2; ++upper)
        {
            const std::size_t row = 2 * index + static_cast<std::size_t>(upper);
            const double s = upper == 0 ? vs : us; // the measure of the set whose bound it is
            double* slope = slopes != nullptr ? slopes + 3 * row : nullptr;
            double* curvature = curvatures != nullptr ? curvatures + 9 * row : nullptr;
            if (!(s > 0.0))
            {
                const double limit = upper == 0 ? ws - vs * vs * vs : 1.0 - us * us * us - ws;
                rows[row] = bs == 0.0 ? limit : -std::numeric_limits<double>::infinity();
                if (slope != nullptr)
                    std::fill(slope, slope + 3, NOT_A_NUMBER);
                if (curvature != nullptr)
                    std::fill(curvature, curvature + 9, NOT_A_NUMBER);
                continue;
            }
            const double coupling = 3.0 * bs * bs / (4.0 * s);
            rows[row] = upper == 0 ? ws - vs * vs * vs - coupling : 1.0 - us * us * us - coupling - ws;
            // d/ds of the row in the measure s: -3 s^2 + 3 b^2 / (4 s^2); s = v (lower) or 1 - v (upper)
            const double bySet = -3.0 * s * s + 3.0 * bs * bs / (4.0 * s * s);
            const double sign = upper == 0 ? 1.0 : -1.0; // ds / dv
            if (slope != nullptr)
            {
                slope[0] = sign * bySet;
                slope[1] = -3.0 * bs / (2.0 * s);
                slope[2] = upper == 0 ? 1.0 : -1.0;
            }
            if (curvature != nullptr)
            {
                const double vv = -6.0 * s - 3.0 * bs * bs / (2.0 * s * s * s);
                const double vb = sign * 3.0 * bs / (2.0 * s * s);
                const double bb = -3.0 / (2.0 * s);
                const std::array<double, 9> values{vv, vb, 0.0, vb, bb, 0.0, 0.0, 0.0, 0.0};
                std::copy(values.begin(), values.end(), curvature);
            }
        }
    }
}

std::array<double, 2> equilibriumTransverseShear(std::span<const std::array<double, 9>> stiffness,
                                                 std::span<const std::array<double, 4>> shear,
                                                 std::span<const double> thickness)
{
    const std::size_t layers = thickness.size();
    if (stiffness.size() != layers || shear.size() != layers)
        throw std::invalid_argument("the stiffness, the shear and the thickness of the layers differ in number");
    std::vector<double> z(layers + 1, 0.0);
    for (std::size_t k = 0; k < layers; ++k)
        z[k + 1] = z[k] + thickness[k];
    if (!(z[layers] > 0.0))
        throw std::invalid_argument("the stack has no thickness");
    // three Gauss points integrate tau^2, of degree 4 in a layer, exactly
    const std::array<double, 3> points{-std::sqrt(0.6), 0.0, std::sqrt(0.6)};
    const std::array<double, 3> weights{5.0 / 9.0, 8.0 / 9.0, 5.0 / 9.0};
    std::array<double, 2> result{};
    for (std::size_t direction = 0; direction < 2; ++direction)
    {
        const std::size_t entry = direction == 0 ? 0 : 4;   // Q-bar 11 or 22
        const std::size_t modulus = direction == 0 ? 0 : 3; // G_xz or G_yz
        double a = 0.0;
        double b = 0.0;
        double d = 0.0;
        bool rigid = false;
        for (std::size_t k = 0; k < layers; ++k)
        {
            const double e = stiffness[k][entry];
            a += e * (z[k + 1] - z[k]);
            b += e * (z[k + 1] * z[k + 1] - z[k] * z[k]) / 2.0;
            d += e * (z[k + 1] * z[k + 1] * z[k + 1] - z[k] * z[k] * z[k]) / 3.0;
            rigid = rigid || std::isinf(shear[k][modulus]);
        }
        if (rigid)
        {
            result[direction] = std::numeric_limits<double>::infinity();
            continue;
        }
        const double neutral = b / a;
        const double rigidity = d - b * b / a;
        double below = 0.0;
        double compliance = 0.0;
        for (std::size_t k = 0; k < layers; ++k)
        {
            const double e = stiffness[k][entry];
            const double half = (z[k + 1] - z[k]) / 2.0;
            const double middle = (z[k + 1] + z[k]) / 2.0;
            const double start = (z[k] - neutral) * (z[k] - neutral);
            for (std::size_t point = 0; point < 3; ++point)
            {
                const double height = middle + half * points[point];
                const double tau = (below + 0.5 * e * ((height - neutral) * (height - neutral) - start)) / rigidity;
                compliance += weights[point] * half * tau * tau / shear[k][modulus];
            }
            below += 0.5 * e * ((z[k + 1] - neutral) * (z[k + 1] - neutral) - start);
        }
        result[direction] = 1.0 / compliance;
    }
    return result;
}

std::vector<double> angleMomentMap(std::span<const double> angles)
{
    const std::size_t count = angles.size();
    if (count == 0 || count > 5)
        throw std::invalid_argument("the lamination parameters give the moments of one to five angles (" +
                                    std::to_string(count) + " given): carry the moments of a larger set as variables");
    std::vector<double> m(5 * count); // M row by row, 5 x K
    for (std::size_t k = 0; k < count; ++k)
    {
        const AngleTerms terms = angleTerms(angles[k]);
        m[k] = 1.0;
        for (std::size_t i = 0; i < 4; ++i)
            m[(i + 1) * count + k] = terms.f[i];
    }
    // N = M^T M (K x K), inverted by Gauss-Jordan with partial pivoting
    std::vector<double> normal(count * count, 0.0);
    for (std::size_t a = 0; a < count; ++a)
        for (std::size_t b = 0; b < count; ++b)
            for (std::size_t r = 0; r < 5; ++r)
                normal[a * count + b] += m[r * count + a] * m[r * count + b];
    std::vector<double> inverse(count * count, 0.0);
    for (std::size_t a = 0; a < count; ++a)
        inverse[a * count + a] = 1.0;
    for (std::size_t column = 0; column < count; ++column)
    {
        std::size_t pivot = column;
        for (std::size_t row = column + 1; row < count; ++row)
            if (std::fabs(normal[row * count + column]) > std::fabs(normal[pivot * count + column]))
                pivot = row;
        if (std::fabs(normal[pivot * count + column]) < 1e-10)
            throw std::invalid_argument(
                "the angles do not give independent moments (two angles equal modulo 180 degrees, or a set that the "
                "lamination parameters cannot separate)");
        for (std::size_t j = 0; j < count; ++j)
        {
            std::swap(normal[column * count + j], normal[pivot * count + j]);
            std::swap(inverse[column * count + j], inverse[pivot * count + j]);
        }
        const double scale = normal[column * count + column];
        for (std::size_t j = 0; j < count; ++j)
        {
            normal[column * count + j] /= scale;
            inverse[column * count + j] /= scale;
        }
        for (std::size_t row = 0; row < count; ++row)
        {
            if (row == column)
                continue;
            const double factor = normal[row * count + column];
            for (std::size_t j = 0; j < count; ++j)
            {
                normal[row * count + j] -= factor * normal[column * count + j];
                inverse[row * count + j] -= factor * inverse[column * count + j];
            }
        }
    }
    std::vector<double> map(count * 5, 0.0); // P = N^-1 M^T, K x 5
    for (std::size_t k = 0; k < count; ++k)
        for (std::size_t r = 0; r < 5; ++r)
            for (std::size_t j = 0; j < count; ++j)
                map[k * 5 + r] += inverse[k * count + j] * m[r * count + j];
    return map;
}

} // namespace oor::analysis
