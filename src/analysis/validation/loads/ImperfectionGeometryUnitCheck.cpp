#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/loads/LoadChecks.hpp"
#include "oor/model/cards/loads/imperfectionGeometry/ImperfectionGeometryTable.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// A group of an IMPGEOM reads its imperfection from a file, and the file is named by a unit that an ASSIGN statement
// gives (a zero or blank UNIT needs the UNITi). The card keeps
// the groups resolved, so a group whose unit is 0 has neither a UNITi nor a UNIT to read its file from.
class ImperfectionGeometryUnitCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "load.imperfection-geometry-unit", base::Severity::Error, "loads",
        "A group of an imperfection geometry card has no file unit: neither the group nor the card gives one."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        // The unit of group i is UNITi: the keyword of the unit of the card followed by the number of the group.
        const std::string unit = std::string("fileUnit");
        forEachCard<model::ImperfectionGeometryTable>(
            model,
            [&](const model::ImperfectionGeometryCard& card, model::CardLocation where)
            {
                for (std::size_t index = 0; index < card.groups.size(); ++index)
                {
                    if (card.groups[index].fileUnit != 0)
                        continue;
                    sink.reportCard(where, "fileUnit",
                                    "group " + std::to_string(index + 1) + " has no file unit: " + unit +
                                        std::to_string(index + 1) + " and " + unit + " are both 0",
                                    "give the unit of the imperfection file (the number of its ASSIGN statement) in "
                                    "the group or in the card");
                }
            });
    }
};

} // namespace

void addImperfectionGeometryUnitCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ImperfectionGeometryUnitCheck>());
}

} // namespace oor::analysis
