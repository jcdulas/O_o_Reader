#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/properties/tiltingPadBearingProp/TiltingPadBearingPropTable.hpp"
#include "oor/model/cards/properties/userElemService/UserElemServiceTable.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// A THPAD whose bearing has no ELEMUDS that calls the THPAD service. The RID of a THPAD matches the PID of an ELEMUDS
// and of a PBUSH2D, and the ELEMUDS needs UNAME=THPAD when the external bearing service is used; a THPAD without it is
// data that no service reads. The PBUSH2D is the business of ref.unresolved.
class TiltingPadBearingPropServiceCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.tilting-pad-bearing-prop-service", base::Severity::Error, "compatibility",
                                    "A tilting pad bearing property has no user element service of its number that "
                                    "calls the tilting pad bearing routine."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const model::UserElemServiceTable* services = model.findTable<model::UserElemServiceTable>();
        forEachCard<model::TiltingPadBearingPropTable>(
            model,
            [&](const model::TiltingPadBearingPropCard& card, model::CardLocation where)
            {
                const std::string pidField = std::string("id");
                const std::string unameField = std::string("userRoutineName");
                const std::optional<model::CardLocation> service =
                    services == nullptr
                        ? std::nullopt
                        : model.locateInType(model::typeIndexOf<model::UserElemServiceTable>(), card.id);
                if (!service)
                {
                    sink.reportCard(
                        where, "id",
                        "no " + std::string("UserElemService") + " has the " + pidField + " " + std::to_string(card.id),
                        "add an " + std::string("UserElemService") + " with this " + pidField + " and the " +
                            unameField + " THPAD");
                    return;
                }
                const std::string uname = services->card(service->row).userRoutineName;
                if (!((uname) == ("THPAD")))
                    sink.reportCard(
                        where, "id",
                        "the " + std::string("UserElemService") + " " + std::to_string(card.id) + " has the " +
                            unameField + " \"" + uname + "\", not THPAD, so it does not call the THPAD service",
                        "set the " + unameField + " of the " + std::string("UserElemService") + " to THPAD");
            });
    }
};

} // namespace

void addTiltingPadBearingPropServiceCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<TiltingPadBearingPropServiceCheck>());
}

} // namespace oor::analysis
