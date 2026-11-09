#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/aeroStaticParams/AeroStaticParamsTable.hpp"
#include "oor/model/cards/aero/aeroUnsteadyParams/AeroUnsteadyParamsTable.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// The Case Control commands that run an aeroelastic analysis need its physical data: the flutter and gust analyses the
// AERO card (every aerodynamic run needs it), the static trim and the divergence
// analysis the AEROS card (static aeroelastic runs need it). A deck whose AERO or AEROS is in a
// file the model does not have is not told apart from one that lacks it.
class AeroCardMissingCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "aero.card-missing",
        base::Severity::Error,
        "aero",
        "A subcase asks for a flutter, gust, trim or divergence analysis and the deck has no unsteady or static "
        "aerodynamic parameters card.",
        false,
        CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& root = context.root();
        const bool hasAero = root.findTable<model::AeroUnsteadyParamsTable>() != nullptr &&
                             root.findTable<model::AeroUnsteadyParamsTable>()->rows().aliveCount() > 0;
        const bool hasAeros = root.findTable<model::AeroStaticParamsTable>() != nullptr &&
                              root.findTable<model::AeroStaticParamsTable>()->rows().aliveCount() > 0;
        const model::CaseControl& caseControl = root.control().caseControl;
        const model::CaseLineages lineages(caseControl);
        for (const model::Subcase* subcase : analysisSubcases(caseControl))
        {
            const std::string statement(model::subcaseKindName(subcase->kind));
            for (const char* command : {"FlutterMethodSelection", "GustSelection"})
            {
                if (!hasAero && hasEffectiveCommand(lineages, *subcase, command))
                {
                    sink.reportStatement(statement, subcase->id,
                                         std::string(command) +
                                             " asks for an aeroelastic analysis and the deck has no " +
                                             std::string("AeroUnsteadyParams") + " card",
                                         "add the " + std::string("AeroUnsteadyParams") + " card");
                }
            }
            for (const char* command : {"TrimSelection", "DivergenceSelection"})
            {
                if (!hasAeros && hasEffectiveCommand(lineages, *subcase, command))
                {
                    sink.reportStatement(statement, subcase->id,
                                         std::string(command) +
                                             " asks for a static aeroelastic analysis and the deck has no " +
                                             std::string("AeroStaticParams") + " card",
                                         "add the " + std::string("AeroStaticParams") + " card");
                }
            }
        }
    }
};

} // namespace

void addAeroCardMissingCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<AeroCardMissingCheck>());
}

} // namespace oor::analysis
