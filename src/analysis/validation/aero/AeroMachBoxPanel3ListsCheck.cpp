#include "AeroCheckSupport.hpp"
#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/aeroMachBoxPanel3/AeroMachBoxPanel3Table.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// The AEFACT lists LISTW, LISTC1 and LISTC2 of a CAERO3 hold the (x, y) pairs of the aerodynamic grid points (format
// reference), so each has an even number of values. A list that does not exist is `ref.unresolved`; a
// blank LISTC1 or LISTC2 is 0 and names none.
class AeroMachBoxPanel3ListsCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "aero.aero-mach-box-panel3-lists", base::Severity::Error, "aero",
        "An aerodynamic factor list of a Mach box panel does not hold (x, y) pairs: its number of values is odd."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::AeroMachBoxPanel3Table>(
            model,
            [&](const model::AeroMachBoxPanel3Card& card, model::CardLocation where)
            {
                look(model, sink, where, "wingPointListId", card.wingPointListId);
                look(model, sink, where, "controlPointListId1", card.controlPointListId1);
                look(model, sink, where, "controlPointListId2", card.controlPointListId2);
            });
    }

private:
    static void look(const model::Model& model, FindingSink& sink, model::CardLocation where, const char* field,
                     base::EntityId list)
    {
        const std::optional<std::size_t> size = aefactSize(model, list);
        if (size && *size % 2 != 0)
        {
            sink.reportCard(where, field,
                            "the " + std::string("AeroFactorList") + " " + std::to_string(list) + " holds " +
                                std::to_string(*size) + " values, and the (x, y) pairs need an even number",
                            "give an x and a y for each point");
        }
    }
};

} // namespace

void addAeroMachBoxPanel3ListsCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<AeroMachBoxPanel3ListsCheck>());
}

} // namespace oor::analysis
