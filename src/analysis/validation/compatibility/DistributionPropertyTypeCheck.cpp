#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/properties/propDistribution/PropDistributionTable.hpp"
#include "oor/model/cards/properties/propMatDistribution/PropMatDistributionTable.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// A PDISTB or PDISTBM whose PID names a property that is not of the kind TYPE says. The format asks the
// PID to point to an existing entry of that TYPE, and the names of the data a PDISTB
// gives are the ones of that kind of property, so a PID that names another kind has no datum of that name to replace.
// The reference to the property is the business of ref.unresolved: a PID that no property has is left alone here.
class DistributionPropertyTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.distribution-property-type", base::Severity::Error, "compatibility",
                                    "A property distribution names a property that is not of the kind its type gives."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::PropDistributionTable>(
            model, [&](const model::PropDistributionCard& card, model::CardLocation where)
            { judge(model, sink, where, card.type, card.propId); });
        forEachCard<model::PropMatDistributionTable>(
            model, [&](const model::PropMatDistributionCard& card, model::CardLocation where)
            { judge(model, sink, where, card.type, card.propId); });
    }

private:
    static void judge(const model::Model& model, FindingSink& sink, model::CardLocation where,
                      model::DistributedPropertyType type, base::EntityId pid)
    {
        const std::optional<model::CardLocation> property = model.locate(base::IdSpace::Property, pid);
        if (!property)
            return;
        // The word table holds the neutral card names: the card type of the property is compared by its name, and the
        // message shows the deck keywords.
        const std::string_view wantedName = model::keywordText(model::DISTRIBUTED_PROPERTY_WORDS, type);
        const std::string_view foundName = cardNameAt(model, *property);
        if (foundName == wantedName)
            return;
        const std::string wanted = std::string(wantedName);
        const std::string found = std::string(foundName);
        const std::string typeField = std::string("type");
        sink.reportCard(
            where, "propId",
            typeField + " says " + wanted + " but the property " + std::to_string(pid) + " is a " + found,
            "give the " + typeField + " of the property, or the " + std::string("propId") + " of a " + wanted);
    }
};

} // namespace

void addDistributionPropertyTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<DistributionPropertyTypeCheck>());
}

} // namespace oor::analysis
