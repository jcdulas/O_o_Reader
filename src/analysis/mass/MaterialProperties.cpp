#include "oor/analysis/mass/MaterialProperties.hpp"

#include "oor/model/cards/materials/anisoMat2D/AnisoMat2DTable.hpp"
#include "oor/model/cards/materials/anisoMat3D/AnisoMat3DTable.hpp"
#include "oor/model/cards/materials/axisymOrthoMat/AxisymOrthoMatTable.hpp"
#include "oor/model/cards/materials/fluidMat/FluidMatTable.hpp"
#include "oor/model/cards/materials/hyperelasticMat/HyperelasticMatTable.hpp"
#include "oor/model/cards/materials/hyperelasticPolynomialMat/HyperelasticPolynomialMatTable.hpp"
#include "oor/model/cards/materials/isoMat/IsoMatTable.hpp"
#include "oor/model/cards/materials/orthoMat2D/OrthoMat2DTable.hpp"
#include "oor/model/cards/materials/orthoMat3D/OrthoMat3DTable.hpp"
#include "oor/model/cards/thermal/heatAnisoMat/HeatAnisoMatTable.hpp"
#include "oor/model/cards/thermal/heatIsoMat/HeatIsoMatTable.hpp"

#include <limits>

namespace oor::analysis
{

namespace
{

template <typename TableT>
typename TableT::CardType cardOf(const model::Model& model, model::CardLocation location)
{
    return static_cast<const TableT&>(*model.findTable(location.table)).card(location.row);
}

// The position in the 21 values of a MAT9 card (the upper triangle of the 6 x 6 matrix by rows) of the term (row,
// column).
std::size_t mat9Index(std::size_t row, std::size_t column) noexcept
{
    if (column < row)
        std::swap(row, column);
    return row * 6 - row * (row - 1) / 2 + (column - row);
}

} // namespace

std::optional<double> MaterialProperties::density(const model::Model& model, base::EntityId mid)
{
    const auto location = model.locate(base::IdSpace::Material, mid);
    if (!location)
    {
        // A heat transfer material carries a density of its own, RHO (1.0 when blank): the mass of a
        // model that is analysed for heat transfer is the one this density gives. A structural material of the same ID
        // wins, as when the reference is resolved.
        const auto thermal = model.locate(base::IdSpace::ThermalMaterial, mid);
        if (!thermal)
            return std::nullopt;
        if (thermal->table == model::typeIndexOf<model::HeatIsoMatTable>())
            return cardOf<model::HeatIsoMatTable>(model, *thermal).density;
        if (thermal->table == model::typeIndexOf<model::HeatAnisoMatTable>())
            return cardOf<model::HeatAnisoMatTable>(model, *thermal).density;
        return std::nullopt;
    }
    const std::uint16_t type = location->table;
    if (type == model::typeIndexOf<model::IsoMatTable>())
        return cardOf<model::IsoMatTable>(model, *location).density;
    if (type == model::typeIndexOf<model::AnisoMat2DTable>())
        return cardOf<model::AnisoMat2DTable>(model, *location).density;
    if (type == model::typeIndexOf<model::AxisymOrthoMatTable>())
        return cardOf<model::AxisymOrthoMatTable>(model, *location).density;
    if (type == model::typeIndexOf<model::OrthoMat2DTable>())
        return cardOf<model::OrthoMat2DTable>(model, *location).density;
    if (type == model::typeIndexOf<model::AnisoMat3DTable>())
        return cardOf<model::AnisoMat3DTable>(model, *location).density;
    if (type == model::typeIndexOf<model::FluidMatTable>())
        return cardOf<model::FluidMatTable>(model, *location).density;
    if (type == model::typeIndexOf<model::OrthoMat3DTable>())
        return cardOf<model::OrthoMat3DTable>(model, *location).density;
    if (type == model::typeIndexOf<model::HyperelasticPolynomialMatTable>())
        return cardOf<model::HyperelasticPolynomialMatTable>(model, *location).density;
    if (type == model::typeIndexOf<model::HyperelasticMatTable>())
        return cardOf<model::HyperelasticMatTable>(model, *location).density;
    return std::nullopt;
}

std::string MaterialProperties::planeStressCards(const model::Model&)
{
    return std::string("IsoMat") + ", " + std::string("AnisoMat2D") + ", " + std::string("OrthoMat2D") + " or " +
           std::string("AnisoMat3D");
}

std::optional<PlaneMaterial> MaterialProperties::planeStress(const model::Model& model, base::EntityId mid)
{
    const auto location = model.locate(base::IdSpace::Material, mid);
    if (!location)
        return std::nullopt;
    const std::uint16_t type = location->table;
    PlaneMaterial material;
    if (type == model::typeIndexOf<model::IsoMatTable>())
    {
        const model::IsoMatCard card = cardOf<model::IsoMatTable>(model, *location);
        const double factor = card.youngsModulus / (1.0 - card.poissonRatio * card.poissonRatio);
        material.stiffness = {factor, card.poissonRatio * factor, 0.0, card.poissonRatio * factor, factor, 0.0, 0.0,
                              0.0,    card.shearModulus};
        material.shear = {card.shearModulus, card.shearModulus};
        material.density = card.density;
        return material;
    }
    if (type == model::typeIndexOf<model::AnisoMat2DTable>())
    {
        const model::AnisoMat2DCard card = cardOf<model::AnisoMat2DTable>(model, *location);
        // G11 G12 G13 G22 G23 G33
        const auto& g = card.stiffness;
        material.stiffness = {g[0], g[1], g[2], g[1], g[3], g[4], g[2], g[4], g[5]};
        material.shear = {g[5], g[5]};
        material.density = card.density;
        return material;
    }
    if (type == model::typeIndexOf<model::OrthoMat2DTable>())
    {
        const model::OrthoMat2DCard card = cardOf<model::OrthoMat2DTable>(model, *location);
        const double nu21 = card.poissonRatio12 * card.youngsModulus2 / card.youngsModulus1;
        const double denominator = 1.0 - card.poissonRatio12 * nu21;
        material.stiffness = {card.youngsModulus1 / denominator,
                              card.poissonRatio12 * card.youngsModulus2 / denominator,
                              0.0,
                              card.poissonRatio12 * card.youngsModulus2 / denominator,
                              card.youngsModulus2 / denominator,
                              0.0,
                              0.0,
                              0.0,
                              card.shearModulus12};
        // a G1Z or G2Z blank or zero means no transverse shear flexibility, an infinite
        // modulus.
        const auto transverse = [](const std::optional<double>& modulus)
        { return modulus && *modulus > 0.0 ? *modulus : std::numeric_limits<double>::infinity(); };
        material.shear = {transverse(card.shearModulus1Z), transverse(card.shearModulus2Z)};
        material.density = card.density;
        return material;
    }
    if (type == model::typeIndexOf<model::AnisoMat3DTable>())
    {
        const model::AnisoMat3DCard card = cardOf<model::AnisoMat3DTable>(model, *location);
        // The stresses are (x, y, z, xy, yz, zx): the plane stress law condenses z out of the in-plane terms, and the
        // transverse shear moduli are the yz and zx terms.
        constexpr std::size_t IN_PLANE[3] = {0, 1, 3};
        constexpr std::size_t THROUGH = 2;
        const auto term = [&](std::size_t row, std::size_t column) { return card.stiffness[mat9Index(row, column)]; };
        for (std::size_t row = 0; row < 3; ++row)
        {
            for (std::size_t column = 0; column < 3; ++column)
            {
                material.stiffness[row * 3 + column] =
                    term(IN_PLANE[row], IN_PLANE[column]) -
                    term(IN_PLANE[row], THROUGH) * term(THROUGH, IN_PLANE[column]) / term(THROUGH, THROUGH);
            }
        }
        material.shear = {term(5, 5), term(4, 4)};
        material.density = card.density;
        return material;
    }
    return std::nullopt;
}

} // namespace oor::analysis
