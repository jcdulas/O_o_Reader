#pragma once

#include "oor/model/cards/superelements/superelementLabel/SuperelementLabelCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct SuperelementLabelFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementLabelCard& card);
    [[nodiscard]] static std::optional<SuperelementLabelCard> read(FieldReader& reader);
    static void write(const SuperelementLabelCard& card, FieldWriter& writer);
};

// All the SELABEL cards, category Superelement, no ID of their own (IdSpace::None). One column for each field.
class SuperelementLabelTable final : public TypedTable<SuperelementLabelTable, SuperelementLabelCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementLabel";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::None};
    using Format = SuperelementLabelFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementLabelCard&) noexcept { return 0; }
    std::uint32_t append(const SuperelementLabelCard& card, base::CardKey key);
    [[nodiscard]] SuperelementLabelCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SuperelementLabelCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }
    [[nodiscard]] Column<std::string>& entryLabelColumn() noexcept { return m_entryLabel; }

private:
    Column<base::EntityId> m_superelementId;
    Column<std::string> m_entryLabel;
};

} // namespace oor::model
