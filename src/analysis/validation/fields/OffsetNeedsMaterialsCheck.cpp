#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/fields/FieldChecks.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/elements/shellQuad8/ShellQuad8Table.hpp"
#include "oor/model/cards/elements/shellQuad4Drilling/ShellQuad4DrillingTable.hpp"
#include "oor/model/cards/elements/shellTria3/ShellTria3Table.hpp"
#include "oor/model/cards/elements/shellTria6/ShellTria6Table.hpp"
#include "oor/model/cards/elements/shellTria3Drilling/ShellTria3DrillingTable.hpp"
#include "oor/model/cards/properties/shellProp/ShellPropTable.hpp"

namespace oor::analysis
{

namespace
{

// format reference, PSHELL: MID1 and MID2 must be given when the connection entry has an offset (ZOFFS): the offset
// couples membrane and bending.
class OffsetNeedsMaterialsCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"field.offset-needs-materials", base::Severity::Error, "fields",
                                    "A shell element has an offset and its shell property lacks the membrane or the "
                                    "bending material."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        scan<model::ShellQuad4Table>(context, sink);
        scan<model::ShellTria3Table>(context, sink);
        scan<model::ShellQuad8Table>(context, sink);
        scan<model::ShellTria6Table>(context, sink);
        scan<model::ShellQuad4DrillingTable>(context, sink);
        scan<model::ShellTria3DrillingTable>(context, sink);
    }

private:
    template <typename ElementTable>
    static void scan(const ValidationContext& context, FindingSink& sink)
    {
        const model::Model& model = context.model();
        forEachCard<ElementTable>(
            model,
            [&](const typename ElementTable::CardType& card, model::CardLocation where)
            {
                if (card.offset == 0.0)
                    return;
                const std::optional<model::CardLocation> property = model.locate(base::IdSpace::Property, card.propId);
                if (!property || !isType<model::ShellPropTable>(*property))
                    return;
                const model::ShellPropCard pshell = cardAt<model::ShellPropTable>(model, *property);
                if (pshell.membraneMatId > 0 && pshell.bendingMatId > 0)
                    return;
                const std::string shellProp = std::string("ShellProp");
                sink.reportCard(where, "offset",
                                std::string("offset") + " is " + std::to_string(card.offset) + " but " + shellProp +
                                    " " + std::to_string(card.propId) + " has no " + std::string("membraneMatId") +
                                    " or no " + std::string("bendingMatId"),
                                "give both materials on the " + shellProp + ", or remove the offset");
            });
    }
};

} // namespace

void addOffsetNeedsMaterialsCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<OffsetNeedsMaterialsCheck>());
}

} // namespace oor::analysis
