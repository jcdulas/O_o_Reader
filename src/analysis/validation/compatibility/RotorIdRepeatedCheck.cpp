#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/SharedIds.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/solution_control/rotorModel3D/RotorModel3DTable.hpp"

namespace oor::analysis
{

namespace
{

// A rotor ID that two kinds of card have: the format reference asks the ROTORIDs of the ROTORAX, ROTORG and ROTORSE
// cards to be different (the format of each says so: a rotor is defined by one kind of card, of its own number). The
// cards of one type may share a ROTORID (several cards make a rotor). A ROTOR, the 3D model of a rotor, is left out: it
// goes with a line model of the same rotor in the frame FIX.
class RotorIdRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.rotor-id-repeated", base::Severity::Error, "compatibility",
                                    "Two rotor cards of different kinds (axisymmetric, line of grid points, "
                                    "superelement) have the same rotor number."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportIdsSharedByTypes(context, sink, model::CardCategory::Rotor, "rotor",
                               [](const model::Model&, model::CardLocation where)
                               { return isType<model::RotorModel3DTable>(where); });
    }
};

} // namespace

void addRotorIdRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<RotorIdRepeatedCheck>());
}

} // namespace oor::analysis
