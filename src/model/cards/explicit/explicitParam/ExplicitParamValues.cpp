#include "oor/model/cards/explicit/explicitParam/ExplicitParamValues.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/explicitParam/ExplicitParamCatalogue.hpp"
#include "oor/model/cards/explicit/explicitParam/ExplicitParamTable.hpp"

#include <algorithm>
#include <utility>

namespace oor::model
{

std::optional<ExplicitParamSetting> explicitParamSetting(const Model& model, std::string_view name)
{
    const ExplicitParamTable* table = model.findTable<ExplicitParamTable>();
    std::optional<ExplicitParamCard> last;
    if (table != nullptr)
    {
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            ExplicitParamCard card = table->card(row);
            if (card.entryName == name)
                last = std::move(card);
        }
    }
    if (last)
        return ExplicitParamSetting{resolveExplicitParamValues(*last), true};

    // No card: the defaults of a known entry with one layout.
    ExplicitParamCard empty;
    empty.entryName = std::string(name);
    std::vector<ExplicitParamValue> values = resolveExplicitParamValues(empty);
    const bool anyDefault =
        std::ranges::any_of(values, [](const ExplicitParamValue& value) { return !value.isBlank(); });
    if (!anyDefault)
        return std::nullopt;
    return ExplicitParamSetting{std::move(values), false};
}

} // namespace oor::model
