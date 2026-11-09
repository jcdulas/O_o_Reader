#pragma once

#include "oor/model/cards/aero/aeroController/AeroControllerCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct AeroControllerFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroControllerCard& card);
    [[nodiscard]] static std::optional<AeroControllerCard> read(FieldReader& reader);
    static void write(const AeroControllerCard& card, FieldWriter& writer);
};

// All the AEPARM cards, in the category AeroTrimVariable that AESTAT shares. The ID is ignored by the solver, so the
// space is IdSpace::Group (several cards may carry the same number). The card names no other card by number.
class AeroControllerTable final : public TypedTable<AeroControllerTable, AeroControllerCard>
{
public:
    static constexpr std::string_view NAME = "AeroController";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroTrimVariable, base::IdSpace::Group};
    using Format = AeroControllerFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroControllerCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroControllerCard& card, base::CardKey key);
    [[nodiscard]] AeroControllerCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroControllerCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::string>& labelColumn() noexcept { return m_label; }
    [[nodiscard]] Column<std::string>& unitsColumn() noexcept { return m_units; }

private:
    Column<base::EntityId> m_id;
    Column<std::string> m_label;
    Column<std::string> m_units;
};

} // namespace oor::model
