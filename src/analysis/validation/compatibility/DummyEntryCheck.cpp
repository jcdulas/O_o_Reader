#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"

#include <set>
#include <string>

namespace oor::analysis
{

namespace
{

// The digit of UserDefined1 through UserDefined9, optionally followed by a role such as Attributes or Prop.
char dummyNumber(std::string_view name, std::string_view suffix = {}) noexcept
{
    constexpr std::string_view PREFIX = "UserDefined";
    if (name.size() != PREFIX.size() + 1 + suffix.size() || !name.starts_with(PREFIX) || !name.ends_with(suffix))
        return 0;
    const char digit = name[PREFIX.size()];
    return digit >= '1' && digit <= '9' ? digit : 0;
}

// The user dummy elements need the attributes entry of their number: the fields of a CDUMi and a PDUMi are defined by
// the ADUMi of the same number, and the crack tip elements are dummy elements of the
// numbers 8 and 9 (they need an ADUM8 and an ADUM9).
class DummyEntryCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "compat.dummy-entry", base::Severity::Error, "compatibility",
        "A user dummy element or property has no attributes card of its number, or a crack tip element has no dummy "
        "attributes card of its number (8 or 9)."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const model::CardRegistry& registry = model::CardRegistry::instance();
        std::set<char> attributes; // the numbers of the ADUMi entries the model has
        for (std::uint16_t type = 1; type < registry.size(); ++type)
        {
            const model::CardTable* table = model.findTable(type);
            const char number = dummyNumber(registry.entry(type).info.name, "Attributes");
            if (table != nullptr && number != 0 && table->rows().aliveCount() > 0)
                attributes.insert(number);
        }
        for (std::uint16_t type = 1; type < registry.size(); ++type)
        {
            const model::CardTable* table = model.findTable(type);
            if (table == nullptr)
                continue;
            const std::string_view name = registry.entry(type).info.name;
            char needed = dummyNumber(name);
            if (needed == 0)
                needed = dummyNumber(name, "Prop");
            if (name == "CrackTip2D")
                needed = '8';
            else if (name == "CrackTip3D")
                needed = '9';
            if (needed == 0 || attributes.contains(needed))
                continue;
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (!table->rows().isAlive(row))
                    continue;
                const std::string attributesCard = std::string("UserDefined") + needed + "Attributes";
                sink.reportCard(model::CardLocation{type, row}, "",
                                std::string(name) + " needs an " + attributesCard + " entry and the model has none",
                                "add the " + attributesCard + " entry that defines its fields");
            }
        }
    }
};

} // namespace

void addDummyEntryCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<DummyEntryCheck>());
}

} // namespace oor::analysis
