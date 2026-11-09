#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/aeroBody2/AeroBody2Table.hpp"
#include "oor/model/cards/aero/aeroFactorList/AeroFactorListTable.hpp"
#include "oor/model/cards/aero/aeroLiftingPanel1/AeroLiftingPanel1Table.hpp"
#include "oor/model/cards/aero/aeroPistonPanel5/AeroPistonPanel5Table.hpp"
#include "oor/model/cards/aero/aeroStripPanel4/AeroStripPanel4Table.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// The AEFACT that cuts a panel, a strip row or a body into boxes lists the division points, one more than there are
// boxes: at least two, and rising, or the boxes would be empty or folded back. Only
// a list the card uses is judged: the number of equal divisions comes first (format reference, NSPAN).
class DivisionListCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "aero.division-list", base::Severity::Error, "aero",
        "The aerodynamic factor list that gives the division points of an aerodynamic panel has fewer than two points "
        "or is not increasing."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::AeroLiftingPanel1Table>(
            model,
            [&](const model::AeroLiftingPanel1Card& card, model::CardLocation where)
            {
                look(model, sink, where, "spanDivisionListId", card.spanBoxCount, card.spanDivisionListId);
                look(model, sink, where, "chordDivisionListId", card.chordBoxCount, card.chordDivisionListId);
            });
        forEachCard<model::AeroBody2Table>(
            model,
            [&](const model::AeroBody2Card& card, model::CardLocation where)
            {
                look(model, sink, where, "lsbValue", card.slenderElemCount, card.slenderListId);
                look(model, sink, where, "lInteger", card.interferenceElemCount, card.interferenceListId);
            });
        forEachCard<model::AeroStripPanel4Table>(
            model, [&](const model::AeroStripPanel4Card& card, model::CardLocation where)
            { look(model, sink, where, "spanDivisionListId", card.spanBoxCount, card.spanDivisionListId); });
        forEachCard<model::AeroPistonPanel5Table>(
            model, [&](const model::AeroPistonPanel5Card& card, model::CardLocation where)
            { look(model, sink, where, "spanDivisionListId", card.spanBoxCount, card.spanDivisionListId); });
    }

private:
    // An AEFACT that does not exist is `ref.unresolved`.
    static void look(const model::Model& model, FindingSink& sink, model::CardLocation where, const char* field,
                     std::int32_t count, base::EntityId list)
    {
        if (count > 0 || list <= 0)
            return;
        const auto* table = model.findTable<model::AeroFactorListTable>();
        if (table == nullptr)
            return;
        const std::optional<model::CardLocation> found =
            model.locateInType(model::typeIndexOf<model::AeroFactorListTable>(), list);
        if (!found)
            return;
        const std::span<const double> points = table->valuesAt(found->row);
        if (points.size() < 2)
        {
            sink.reportCard(where, field,
                            "the " + std::string("AeroFactorList") + " " + std::to_string(list) + " has " +
                                std::to_string(points.size()) +
                                " division point(s): at least two are needed to make a box",
                            "give the division points, one more than the number of boxes");
            return;
        }
        for (std::size_t index = 1; index < points.size(); ++index)
        {
            if (!(points[index] > points[index - 1]))
            {
                sink.reportCard(where, field,
                                "the division points of the " + std::string("AeroFactorList") + " " +
                                    std::to_string(list) + " are not increasing (point " + std::to_string(index + 1) +
                                    ")",
                                "list the division points in increasing order");
                return;
            }
        }
    }
};

} // namespace

void addDivisionListCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<DivisionListCheck>());
}

} // namespace oor::analysis
