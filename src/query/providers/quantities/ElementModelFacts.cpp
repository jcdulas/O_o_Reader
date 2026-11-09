#include "oor/query/providers/quantities/ElementModelFacts.hpp"

#include "oor/analysis/frames/StoredFrames.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/model/cards/materials/isoMat/IsoMatTable.hpp"
#include "oor/model/cards/properties/rodProp/RodPropTable.hpp"
#include "oor/model/cards/properties/shellProp/ShellPropTable.hpp"
#include "oor/model/cards/properties/solidProp/SolidPropTable.hpp"

#include <cmath>

namespace oor::query
{

namespace
{

// The MAT1 with this ID, or nothing (another kind of material, a missing one).
std::optional<model::IsoMatCard> isotropicMaterial(const model::Model& model, base::EntityId mid, std::string& why)
{
    const auto location = model.locate(base::IdSpace::Material, mid);
    if (!location)
    {
        why = "the material " + std::to_string(mid) + " does not exist";
        return std::nullopt;
    }
    if (location->table != model::typeIndexOf<model::IsoMatTable>())
    {
        why =
            "the material " + std::to_string(mid) +
            " is not a MAT1 (only a linear isotropic material gives a stress from a strain, or a strain from a stress)";
        return std::nullopt;
    }
    const model::IsoMatCard card =
        static_cast<const model::IsoMatTable&>(*model.findTable(location->table)).card(location->row);
    if (!(card.youngsModulus > 0.0))
    {
        why = "the MAT1 " + std::to_string(mid) + " has no Young's modulus";
        return std::nullopt;
    }
    return card;
}

} // namespace

ElementModelFacts::ElementModelFacts(const ResultQueryContext& context, std::size_t set)
    : m_context(context), m_set(set), m_model(context.modelOf(set))
{
}

const ShellFacts& ElementModelFacts::shell(std::int32_t resultEid)
{
    const auto found = m_shells.find(resultEid);
    if (found != m_shells.end())
        return found->second;
    ShellFacts facts;
    const auto card = m_context.elementCard(m_set, resultEid);
    do
    {
        if (m_model == nullptr)
        {
            facts.why = "no model is attached to the results";
            break;
        }
        if (!card.linked || card.pid == 0)
        {
            facts.why = "the element is not linked to a card of the model that has a property";
            break;
        }
        const auto property = m_model->locate(base::IdSpace::Property, card.pid);
        if (!property || property->table != model::typeIndexOf<model::ShellPropTable>())
        {
            facts.why = "the property " + std::to_string(card.pid) + " is not a PSHELL";
            break;
        }
        const model::ShellPropCard shell =
            static_cast<const model::ShellPropTable&>(*m_model->findTable(property->table)).card(property->row);
        if (shell.membraneMatId <= 0 || (shell.bendingMatId != 0 && shell.bendingMatId != shell.membraneMatId) ||
            shell.couplingMatId != 0 || std::fabs(shell.bendingStiffnessRatio - 1.0) > 1e-12)
        {
            facts.why =
                "the PSHELL " + std::to_string(card.pid) +
                " does not have one material for membrane and bending with the plain bending stiffness (MID1 = MID2, "
                "12I/T**3 = 1, no MID4)";
            break;
        }
        std::string why;
        const auto material = isotropicMaterial(*m_model, shell.membraneMatId, why);
        if (!material)
        {
            facts.why = why;
            break;
        }
        const auto element = m_model->locate(base::IdSpace::Element, card.modelId);
        double thickness = shell.thickness;
        if (element)
        {
            const double geometric =
                analysis::ElementGeometry::of(*m_model, *element, analysis::bit(analysis::Quantity::Thickness))
                    .thickness;
            if (std::isfinite(geometric) && geometric > 0.0)
                thickness = geometric;
        }
        if (!(thickness > 0.0))
        {
            facts.why = "the shell has no thickness";
            break;
        }
        facts.ok = true;
        facts.thickness = thickness;
        // a blank fibre distance is half the thickness of the element; one given is
        // as given
        facts.z1 = shell.fiberDistance1.value_or(-0.5 * thickness);
        facts.z2 = shell.fiberDistance2.value_or(0.5 * thickness);
        facts.e = material->youngsModulus;
        facts.nu = material->poissonRatio;
        facts.g = material->shearModulus > 0.0 ? material->shearModulus
                                               : material->youngsModulus / (2.0 * (1.0 + material->poissonRatio));
        facts.pid = card.pid;
    } while (false);
    return m_shells.emplace(resultEid, std::move(facts)).first->second;
}

const RodFacts& ElementModelFacts::rod(std::int32_t resultEid)
{
    const auto found = m_rods.find(resultEid);
    if (found != m_rods.end())
        return found->second;
    RodFacts facts;
    const auto card = m_context.elementCard(m_set, resultEid);
    do
    {
        if (m_model == nullptr)
        {
            facts.why = "no model is attached to the results";
            break;
        }
        if (!card.linked || card.pid == 0)
        {
            facts.why = "the element is not linked to a card of the model that has a property";
            break;
        }
        const auto property = m_model->locate(base::IdSpace::Property, card.pid);
        if (!property || property->table != model::typeIndexOf<model::RodPropTable>())
        {
            facts.why = "the property " + std::to_string(card.pid) + " is not a PROD";
            break;
        }
        const model::RodPropCard rod =
            static_cast<const model::RodPropTable&>(*m_model->findTable(property->table)).card(property->row);
        std::string why;
        const auto material = isotropicMaterial(*m_model, rod.matId, why);
        if (!material)
        {
            facts.why = why;
            break;
        }
        if (!(rod.area > 0.0))
        {
            facts.why = "the PROD " + std::to_string(card.pid) + " has no area";
            break;
        }
        facts.ok = true;
        facts.area = rod.area;
        facts.torsion = rod.torsion;
        facts.coefficient = rod.stressCoefficient;
        facts.e = material->youngsModulus;
        facts.g = material->shearModulus > 0.0 ? material->shearModulus
                                               : material->youngsModulus / (2.0 * (1.0 + material->poissonRatio));
        facts.pid = card.pid;
    } while (false);
    return m_rods.emplace(resultEid, std::move(facts)).first->second;
}

const SolidFacts& ElementModelFacts::solid(std::int32_t resultEid)
{
    const auto found = m_solids.find(resultEid);
    if (found != m_solids.end())
        return found->second;
    SolidFacts facts;
    const auto card = m_context.elementCard(m_set, resultEid);
    do
    {
        if (m_model == nullptr)
        {
            facts.why = "no model is attached to the results";
            break;
        }
        if (!card.linked || card.pid == 0)
        {
            facts.why = "the element is not linked to a card of the model that has a property";
            break;
        }
        const auto property = m_model->locate(base::IdSpace::Property, card.pid);
        if (!property || property->table != model::typeIndexOf<model::SolidPropTable>())
        {
            facts.why = "the property " + std::to_string(card.pid) + " is not a PSOLID";
            break;
        }
        const model::SolidPropCard solid =
            static_cast<const model::SolidPropTable&>(*m_model->findTable(property->table)).card(property->row);
        std::string why;
        const auto material = isotropicMaterial(*m_model, solid.matId, why);
        if (!material)
        {
            facts.why = why;
            break;
        }
        facts.ok = true;
        facts.e = material->youngsModulus;
        facts.nu = material->poissonRatio;
        facts.g = material->shearModulus > 0.0 ? material->shearModulus
                                               : material->youngsModulus / (2.0 * (1.0 + material->poissonRatio));
        facts.pid = card.pid;
    } while (false);
    return m_solids.emplace(resultEid, std::move(facts)).first->second;
}

} // namespace oor::query
