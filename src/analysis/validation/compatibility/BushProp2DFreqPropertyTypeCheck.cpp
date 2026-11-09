#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/properties/bushProp2DFreq/BushProp2DFreqTable.hpp"

#include <string>
#include <string_view>

namespace oor::analysis
{

namespace
{

// A PBSH2DT gives the frequency dependent terms of the PBUSH2D that has its ID, and the PBUSH2D must exist (its
// values are the nominal ones). The ID resolves to any property, so a PBSH2DT with the
// ID of a PSHELL reads without a reference problem and means nothing.
class BushProp2DFreqPropertyTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.bush-prop2d-freq-property-type", base::Severity::Error, "compatibility",
                                    "A frequency-dependent 2D bush property has the ID of a property that is not a 2D "
                                    "bush property."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::BushProp2DFreqTable>(
            model,
            [&](const model::BushProp2DFreqCard& card, model::CardLocation where)
            {
                const std::optional<model::CardLocation> property = model.locate(base::IdSpace::Property, card.id);
                if (!property)
                    return; // a property that does not exist is the business of ref.unresolved
                const std::string_view name = cardNameAt(model, *property);
                if (name == "BushProp2D")
                    return;
                sink.reportCard(where, "id",
                                "extends property " + std::to_string(card.id) + ", which is a " + std::string(name) +
                                    ": a " + std::string("BushProp2DFreq") + " needs a " + std::string("BushProp2D"),
                                "give the " + std::string("BushProp2DFreq") + " the " + std::string("id") + " of a " +
                                    std::string("BushProp2D"));
            });
    }
};

} // namespace

void addBushProp2DFreqPropertyTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<BushProp2DFreqPropertyTypeCheck>());
}

} // namespace oor::analysis
