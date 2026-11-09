#include "oor/analysis/validation/design/DesignChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/optimisation/designResp1/DesignResp1Table.hpp"

#include <optional>
#include <string>

namespace oor::analysis
{

namespace
{

using model::DesignValue;

bool isInteger(const DesignValue& value, int lowest = 1, int highest = 1000000000)
{
    return value.isInteger() && value.integer >= lowest && value.integer <= highest;
}

// What ATTA and ATTB must be for the types of response whose attributes the format reference gives as a code or a
// number. The types it does not list (the frequency, random, flutter, aeroelastic and
// fatigue responses, whose attributes are frequencies, words or sets) are not judged.
//
//   WEIGHT                      ATTA and ATTB a row and a column from 1 to 6, or blank
//   VOLUME                      both blank
//   FRMASS, COMP                ATTB blank
//   EIGN, FREQ, LAMA, CEIG      ATTA the mode number (an integer); the ATTB of a CEIG is ALPHA or OMEGA
//   DISP, SPCFORCE, GPFORCE     ATTA the component, 1 to 6 (an integer)
//   STRESS, STRAIN, FORCE, ESE  ATTA the item code (an integer); ATTB blank, or the mode number
//   CSTRESS, CSTRAIN, CFAILURE, CSTRAT   ATTA the item code (an integer); ATTB blank, or the ply or lamina number
//   TOTSE                       ATTA blank
//
// SPCFORCE and GPFORCE take no ATTB.
struct Problem
{
    std::string field; // "attributeA" or "attributeB"
    std::string message;
};

std::optional<Problem> problemWith(const model::Model&, const model::DesignResp1Card& card)
{
    const std::string& type = card.responseKind;
    const DesignValue& a = card.attributeA;
    const DesignValue& b = card.attributeB;
    const auto describe = [](const DesignValue& value) { return model::describeDesignValue(value); };
    const std::string attaLabel = std::string("attributeA");
    const std::string attbLabel = std::string("attributeB");
    const auto atta = [&](const std::string& what) -> std::optional<Problem>
    {
        return Problem{"attributeA",
                       attaLabel + " of a " + type + " is " + what + ", not " + describe(card.attributeA)};
    };
    const auto attb = [&](const std::string& what) -> std::optional<Problem>
    {
        return Problem{"attributeB",
                       attbLabel + " of a " + type + " is " + what + ", not " + describe(card.attributeB)};
    };

    if (type == "Weight")
    {
        if (!a.isBlank() && !isInteger(a, 1, 6))
            return atta("a row number from 1 to 6 or blank");
        if (!b.isBlank() && !isInteger(b, 1, 6))
            return attb("a column number from 1 to 6 or blank");
    }
    else if (type == "Volume")
    {
        if (!a.isBlank())
            return atta("blank");
        if (!b.isBlank())
            return attb("blank");
    }
    else if (type == "FractionalMass" || type == "TotalCompliance")
    {
        if (!b.isBlank())
            return attb("blank");
    }
    else if (type == "Eigenvalue" || type == "Frequency" || type == "BucklingFactor" || type == "ComplexEigenvalue")
    {
        if (!isInteger(a))
            return atta("the number of the mode (an integer above zero)");
        if (type == "ComplexEigenvalue" && !b.isBlank() && !b.isWord("Alpha") && !b.isWord("Omega"))
            return attb("ALPHA, OMEGA or blank");
    }
    else if (type == "Displacement" || type == "ConstraintForce" || type == "NodeForce")
    {
        if (!isInteger(a, 1, 6))
            return atta("the component of the response (an integer from 1 to 6)");
        if (type != "Displacement" && !b.isBlank())
            return attb("blank");
    }
    else if (type == "Stress" || type == "Strain" || type == "Force" || type == "StrainEnergy" ||
             type == "CompositeStress" || type == "CompositeStrain" || type == "CompositeFailure" ||
             type == "CompositeStrengthRatio")
    {
        if (!isInteger(a))
            return atta("the item code of the response (an integer above zero)");
    }
    else if (type == "TotalStrainEnergy")
    {
        if (!a.isBlank())
            return atta("blank");
    }
    if ((type == "Displacement" || type == "Stress" || type == "Strain" || type == "Force" || type == "StrainEnergy" ||
         type == "CompositeStress" || type == "CompositeStrain" || type == "CompositeFailure" ||
         type == "CompositeStrengthRatio") &&
        !b.isBlank() && !isInteger(b))
        return attb("blank or an integer (the mode, the ply or the lamina)");
    return std::nullopt;
}

// The ATTA and ATTB of a DRESP1 must fit its type of response: a mode number where the type wants an item code, a real
// where it wants a component, an attribute where the type takes none.
class ResponseAttributesCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"design.response-attributes", base::Severity::Warning, "design",
                                    "An attribute of a basic design response does not fit its type of response."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachCard<model::DesignResp1Table>(
            context.model(),
            [&](const model::DesignResp1Card& card, model::CardLocation where)
            {
                if (const std::optional<Problem> problem = problemWith(context.model(), card))
                    sink.reportCard(where, problem->field, problem->message,
                                    "give the attributes that this type of response takes");
            });
    }
};

} // namespace

void addResponseAttributesCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ResponseAttributesCheck>());
}

} // namespace oor::analysis
