#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/superelement/SuperelementChecks.hpp"
#include "oor/model/cards/superelements/superelementBoundarySearch/SuperelementBoundarySearchTable.hpp"
#include "oor/model/cards/superelements/superelementBulkOptions/SuperelementBulkOptionsTable.hpp"
#include "oor/model/cards/superelements/superelementConnection/SuperelementConnectionTable.hpp"
#include "oor/model/cards/superelements/superelementDesignLink/SuperelementDesignLinkTable.hpp"
#include "oor/model/cards/superelements/superelementDesignResp2/SuperelementDesignResp2Table.hpp"
#include "oor/model/cards/superelements/superelementDesignResp3/SuperelementDesignResp3Table.hpp"
#include "oor/model/cards/superelements/superelementElemTransfer/SuperelementElemTransferTable.hpp"
#include "oor/model/cards/superelements/superelementExcludedNodes/SuperelementExcludedNodesTable.hpp"
#include "oor/model/cards/superelements/superelementGeneralizedPointCount/SuperelementGeneralizedPointCountTable.hpp"
#include "oor/model/cards/superelements/superelementInteriorNodes/SuperelementInteriorNodesTable.hpp"
#include "oor/model/cards/superelements/superelementLabel/SuperelementLabelTable.hpp"
#include "oor/model/cards/superelements/superelementLocation/SuperelementLocationTable.hpp"
#include "oor/model/cards/superelements/superelementMirrorPlane/SuperelementMirrorPlaneTable.hpp"

namespace oor::analysis
{

namespace
{

// A card that the format reference puts in the main Bulk Data, found after a BEGIN SUPER. The format reference says
// what becomes of each: SuperelementConnection, SuperelementExcludedNodes, SuperelementLocation and
// SuperelementMirrorPlane are a fatal error there; SuperelementInteriorNodes,
// SuperelementElemTransfer, SuperelementLabel, SuperelementGeneralizedPointCount, SuperelementDesignLink,
// SuperelementDesignResp2 and SuperelementDesignResp3 are ignored. SuperelementBulkOptions and
// SuperelementBoundarySearch say "main Bulk Data" without telling what happens: they are reported with the ignored
// ones, the milder word. The cards looked at are those of the part the check runs on.
class CardInPartCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"superelement.card-in-part", base::Severity::Error, "superelement",
                                    "A card that belongs in the main Bulk Data (superelement interior points, options, "
                                    "connections, ...) is in a BEGIN SUPER part, where it is ignored or is a fatal "
                                    "error."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const std::optional<model::PartInfo>& part = context.model().partInfo();
        if (!part || part->kind != "SuperelementPart")
            return;
        constexpr std::string_view STOPS = "it is a fatal error there";
        constexpr std::string_view IGNORES = "it is ignored there";
        report<model::SuperelementConnectionTable>(context, sink, STOPS);
        report<model::SuperelementExcludedNodesTable>(context, sink, STOPS);
        report<model::SuperelementLocationTable>(context, sink, STOPS);
        report<model::SuperelementMirrorPlaneTable>(context, sink, STOPS);
        report<model::SuperelementInteriorNodesTable>(context, sink, IGNORES);
        report<model::SuperelementElemTransferTable>(context, sink, IGNORES);
        report<model::SuperelementLabelTable>(context, sink, IGNORES);
        report<model::SuperelementGeneralizedPointCountTable>(context, sink, IGNORES);
        report<model::SuperelementDesignLinkTable>(context, sink, IGNORES);
        report<model::SuperelementDesignResp2Table>(context, sink, IGNORES);
        report<model::SuperelementDesignResp3Table>(context, sink, IGNORES);
        report<model::SuperelementBulkOptionsTable>(context, sink, IGNORES);
        report<model::SuperelementBoundarySearchTable>(context, sink, IGNORES);
    }

private:
    // The card is named in the message by its keyword (cardLabel of the table's card type).
    template <typename TableT>
    static void report(const ValidationContext& context, FindingSink& sink, std::string_view consequence)
    {
        const std::string name = std::string(TableT::NAME);
        forEachCard<TableT>(context.model(),
                            [&](const auto&, model::CardLocation where)
                            {
                                sink.reportCard(where, "",
                                                name + " belongs in the main Bulk Data and is in " + context.part() +
                                                    ": " + std::string(consequence),
                                                "move the card out of the part, before the first BEGIN SUPER");
                            });
    }
};

} // namespace

void addCardInPartCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<CardInPartCheck>());
}

} // namespace oor::analysis
