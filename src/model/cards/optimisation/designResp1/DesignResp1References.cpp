#include "oor/model/cards/optimisation/designResp1/DesignResp1References.hpp"

#include "oor/model/cards/optimisation/DesignTargets.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

namespace oor::model
{

namespace
{

// What the integers after ATT1 name.
enum class Attached : std::uint8_t
{
    Nothing,
    Grids,
    ElementsOrProperties, // elements for PTYPE ELEM, else properties of the type PTYPE names
    Elements,
    Properties, // of the type PTYPE names
    Set,        // ATT1 only
};

constexpr std::array<std::string_view, 23> GRID_RESPONSES{"Displacement",
                                                          "ConstraintForce",
                                                          "FreqDisplacement",
                                                          "FreqVelocity",
                                                          "FreqAcceleration",
                                                          "FreqConstraintForce",
                                                          "SpectralDisplacement",
                                                          "SpectralVelocity",
                                                          "SpectralAcceleration",
                                                          "RootMeanSquareDisplacement",
                                                          "RootMeanSquareVelocity",
                                                          "RootMeanSquareAcceleration",
                                                          "TransientDisplacement",
                                                          "TransientVelocity",
                                                          "TransientAcceleration",
                                                          "TransientConstraintForce",
                                                          "Pressure",
                                                          "AcousticIntensity",
                                                          "AcousticFieldPressure",
                                                          "AcousticFieldIntensity",
                                                          "AcousticFieldVelocity",
                                                          "DynamicStiffness",
                                                          "NodeForcePanel"};

constexpr std::array<std::string_view, 15> ELEMENT_OR_PROPERTY_RESPONSES{
    "Stress",          "Strain",           "Force",
    "StrainEnergy",    "Fatigue",          "CompositeStrain",
    "CompositeStress", "CompositeFailure", "CompositeStrengthRatio",
    "FreqStress",      "FreqForce",        "FreqFatigue",
    "TransientStress", "TransientForce",   "AbsoluteStress"};

template <std::size_t N>
bool isOneOf(const std::string& word, const std::array<std::string_view, N>& words) noexcept
{
    return std::find(words.begin(), words.end(), word) != words.end();
}

Attached attachedBy(const std::string& responseType) noexcept
{
    if (isOneOf(responseType, GRID_RESPONSES))
        return Attached::Grids;
    if (isOneOf(responseType, ELEMENT_OR_PROPERTY_RESPONSES))
        return Attached::ElementsOrProperties;
    if (responseType == "NodeForce")
        return Attached::Elements;
    if (responseType == "FractionalMass" || responseType == "WeightPerPropertyId")
        return Attached::Properties;
    if (responseType == "Flutter" || responseType == "RadiatedPower")
        return Attached::Set;
    return Attached::Nothing;
}

bool isThru(const std::vector<DesignValue>& values, std::size_t index) noexcept
{
    return index < values.size() && values[index].isWord("Through");
}

// A number that is the end of a range may name nothing: the in-range target of the same kind.
ReferenceTarget inRange(ReferenceTarget target) noexcept
{
    switch (target)
    {
    case ReferenceTarget::Node:
        return ReferenceTarget::NodeInRange;
    case ReferenceTarget::Element:
        return ReferenceTarget::ElementInRange;
    case ReferenceTarget::Property:
        return ReferenceTarget::PropertyInRange;
    default:
        break;
    }
    return target;
}

void visitInteger(ReferenceVisitor& visitor, std::string_view field, ReferenceTarget target, DesignValue& value)
{
    if (value.isInteger() && value.integer > 0)
        value.integer = visitor.visit(field, target, value.integer);
}

// The target of the integers of ATTi, when there is one.
std::optional<ReferenceTarget> attachedTarget(const DesignResp1Card& card)
{
    switch (attachedBy(card.responseKind))
    {
    case Attached::Grids:
        return ReferenceTarget::Node;
    case Attached::Elements:
        return ReferenceTarget::Element;
    case Attached::ElementsOrProperties:
        if (card.ownerKind.isWord("Elements"))
            return ReferenceTarget::Element;
        [[fallthrough]];
    case Attached::Properties:
        if (card.ownerKind.isWord())
        {
            if (const auto target = targetOfDesignedCard(card.ownerKind.word); target == ReferenceTarget::Property)
                return target;
        }
        return std::nullopt;
    case Attached::Set:
    case Attached::Nothing:
        break;
    }
    return std::nullopt;
}

} // namespace

void visitDesignResp1References(DesignResp1Card& card, ReferenceVisitor& visitor)
{
    // The random analysis a PSD or an RMS response uses.
    if (card.responseKind == "SpectralDisplacement" || card.responseKind == "SpectralVelocity" ||
        card.responseKind == "SpectralAcceleration")
        visitInteger(visitor, "ownerKind", ReferenceTarget::RandomSet, card.ownerKind);
    if (card.responseKind == "RootMeanSquareDisplacement" || card.responseKind == "RootMeanSquareVelocity" ||
        card.responseKind == "RootMeanSquareAcceleration")
        visitInteger(visitor, "attributeB", ReferenceTarget::RandomSet, card.attributeB);
    if (card.responseKind == "NodeForcePanel")
        visitInteger(visitor, "attributeA", ReferenceTarget::Node, card.attributeA);
    if (card.responseKind == "WeightPerPropertyId")
        visitInteger(visitor, "attributeA", ReferenceTarget::Material, card.attributeA);

    if (attachedBy(card.responseKind) == Attached::Set)
    {
        if (!card.attributes.empty())
            visitInteger(visitor, "attributes", ReferenceTarget::Set, card.attributes.front());
        return;
    }
    const std::optional<ReferenceTarget> target = attachedTarget(card);
    if (!target)
        return;
    for (std::size_t index = 0; index < card.attributes.size(); ++index)
    {
        const bool endOfRange = (index > 0 && isThru(card.attributes, index - 1)) || isThru(card.attributes, index + 1);
        visitInteger(visitor, "attributes", endOfRange ? inRange(*target) : *target, card.attributes[index]);
    }
}

} // namespace oor::model
