#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/aeroMachFreqPairs2/AeroMachFreqPairs2Table.hpp"
#include "oor/model/cards/aero/aeroMachFreqTable1/AeroMachFreqTable1Table.hpp"
#include "oor/model/cards/aero/aeroStripPanel4/AeroStripPanel4Table.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// A Mach number of exactly 1.0 on an MKAERO1 or MKAERO2 is not supported, except together with a CAERO4 (strip theory):
// the Doublet-Lattice theory is for Mach numbers below 1.0 and the ZONA51 theory above it.
class MachOneCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.mach-one", base::Severity::Error, "aero",
                                    "A Mach number and frequency card has the Mach number 1.0 and the model has no "
                                    "strip theory panel."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        bool hasStrips = false;
        forEachCard<model::AeroStripPanel4Table>(
            model, [&](const model::AeroStripPanel4Card&, model::CardLocation) { hasStrips = true; });
        if (hasStrips)
            return;
        forEachCard<model::AeroMachFreqTable1Table>(
            model,
            [&](const model::AeroMachFreqTable1Card& card, model::CardLocation where)
            {
                for (std::size_t position = 0; position < card.machs.size(); ++position)
                    look(model, sink, where, "machNumber" + std::to_string(position + 1), card.machs[position]);
            });
        forEachCard<model::AeroMachFreqPairs2Table>(
            model,
            [&](const model::AeroMachFreqPairs2Card& card, model::CardLocation where)
            {
                for (std::size_t position = 0; position < card.pairs.size(); ++position)
                    look(model, sink, where, "machNumber" + std::to_string(position + 1), card.pairs[position].mach);
            });
    }

private:
    static void look(const model::Model&, FindingSink& sink, model::CardLocation where, std::string field, double mach)
    {
        if (mach == 1.0)
        {
            sink.reportCard(
                where, std::move(field),
                "the Mach number 1.0 is not supported without a " + std::string("AeroStripPanel4") + " panel",
                "use a Mach number below or above 1.0");
        }
    }
};

} // namespace

void addMachOneCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<MachOneCheck>());
}

} // namespace oor::analysis
