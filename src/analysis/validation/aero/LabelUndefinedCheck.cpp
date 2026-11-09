#include "AeroCheckSupport.hpp"
#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/aero/aeroControlVector/AeroControlVectorTable.hpp"
#include "oor/model/cards/aero/aeroTrim/AeroTrimTable.hpp"
#include "oor/model/cards/aero/aeroTrim2/AeroTrim2Table.hpp"
#include "oor/model/cards/aero/aeroTrimLink/AeroTrimLinkTable.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// The word that stands for the intercept aerodynamics in a UXVEC; it is no trim
// variable.
constexpr std::string_view INTERCEPT_LABEL = "Intercept";

// A TRIM, a TRIM2, an AELINK and a UXVEC name trim variables by label (they are AESURF, AESTAT or AEPARM
// labels). A label that none defines is a variable the solver does not have, and
// the trim fails.
class LabelUndefinedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "aero.label-undefined", base::Severity::Error, "aero",
        "A trim card, trim link or control vector names a trim variable that no control surface, rigid trim variable "
        "or controller defines."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const auto labels = controlLabels(model);
        const auto look = [&](const std::string& label, model::CardLocation where, const char* field)
        {
            if (labels.contains(std::string(label)))
                return;
            sink.reportCard(where, field,
                            "the label " + label + " names no " + std::string("AeroRigidTrimVariable") + ", " +
                                std::string("AeroControlSurface") + " or " + std::string("AeroController"),
                            "define the trim variable, or correct the label");
        };
        forEachCard<model::AeroTrimTable>(model,
                                          [&](const model::AeroTrimCard& card, model::CardLocation where)
                                          {
                                              for (const model::AeroTrimVariable& variable : card.variables)
                                                  look(variable.label, where, "labels");
                                          });
        forEachCard<model::AeroTrim2Table>(model,
                                           [&](const model::AeroTrim2Card& card, model::CardLocation where)
                                           {
                                               for (const model::AeroTrim2Variable& variable : card.variables)
                                                   look(variable.label, where, "label");
                                           });
        forEachCard<model::AeroTrimLinkTable>(model,
                                              [&](const model::AeroTrimLinkCard& card, model::CardLocation where)
                                              {
                                                  look(card.dependentLabel, where, "dependentLabel");
                                                  for (const model::LabeledValue& term : card.terms)
                                                      look(term.label, where, "independentLabels");
                                              });
        forEachCard<model::AeroControlVectorTable>(
            model,
            [&](const model::AeroControlVectorCard& card, model::CardLocation where)
            {
                for (const model::LabeledValue& variable : card.variables)
                {
                    if (!((variable.label) == (INTERCEPT_LABEL)))
                        look(variable.label, where, "labels");
                }
            });
    }
};

} // namespace

void addLabelUndefinedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<LabelUndefinedCheck>());
}

} // namespace oor::analysis
