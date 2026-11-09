#include "oor/model/cards/optimisation/designOptimParams/DesignOptimParamsValues.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/optimisation/designOptimParams/DesignOptimParamsCatalogue.hpp"
#include "oor/model/cards/optimisation/designOptimParams/DesignOptimParamsTable.hpp"

namespace oor::model
{

std::optional<DesignOptimParamsValue> designOptimParamsValue(const Model& model, std::string_view name)
{
    std::optional<DesignValue> given;
    if (const DesignOptimParamsTable* table = model.findTable<DesignOptimParamsTable>())
    {
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            for (const DesignOptimParamsEntry& entry : table->card(row).entries)
            {
                if ((entry.name == name))
                    given = entry.value;
            }
        }
    }
    if (given)
        return DesignOptimParamsValue{*given, true};
    const DesignOptimParamsInfo* info = findDesignOptimParam(name);
    if (info == nullptr || !info->hasDefault)
        return std::nullopt;
    const DesignValue value = info->kind == DesignOptimParamsKind::Integer
                                  ? DesignValue::ofInteger(static_cast<std::int32_t>(info->defaultValue))
                                  : DesignValue::ofReal(info->defaultValue);
    return DesignOptimParamsValue{value, false};
}

std::optional<double> designOptimParamsNumber(const Model& model, std::string_view name)
{
    const std::optional<DesignOptimParamsValue> value = designOptimParamsValue(model, name);
    if (!value)
        return std::nullopt;
    if (value->value.isInteger())
        return static_cast<double>(value->value.integer);
    if (value->value.isReal())
        return value->value.real;
    return std::nullopt;
}

} // namespace oor::model
