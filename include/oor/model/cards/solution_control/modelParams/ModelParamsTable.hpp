#pragma once

#include "oor/model/cards/solution_control/modelParams/ModelParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct ModelParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModelParamsCard& card);
    [[nodiscard]] static std::optional<ModelParamsCard> read(FieldReader& reader);
    static void write(const ModelParamsCard& card, FieldWriter& writer);
};

// All the MDLPRM cards. A card has no ID (IdSpace::None). The names, kinds and values of every card are lists in CSR
// form, one per member of a parameter.
class ModelParamsTable final : public TypedTable<ModelParamsTable, ModelParamsCard>
{
public:
    static constexpr std::string_view NAME = "ModelParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = ModelParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const ModelParamsCard&) noexcept { return 0; }
    std::uint32_t append(const ModelParamsCard& card, base::CardKey key);
    [[nodiscard]] ModelParamsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ModelParamsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // an MDLPRM refers to no card

    [[nodiscard]] VariableLengthColumn<std::string>& nameColumn() noexcept { return m_name; }
    [[nodiscard]] VariableLengthColumn<std::uint8_t>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& integerColumn() noexcept { return m_integer; }
    [[nodiscard]] VariableLengthColumn<double>& realColumn() noexcept { return m_real; }
    [[nodiscard]] VariableLengthColumn<std::string>& textColumn() noexcept { return m_text; }

private:
    VariableLengthColumn<std::string> m_name;
    VariableLengthColumn<std::uint8_t> m_kind;
    VariableLengthColumn<std::int32_t> m_integer;
    VariableLengthColumn<double> m_real;
    VariableLengthColumn<std::string> m_text;
};

} // namespace oor::model
