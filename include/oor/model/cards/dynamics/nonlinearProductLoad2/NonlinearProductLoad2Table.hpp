#pragma once

#include "oor/model/cards/dynamics/nonlinearProductLoad2/NonlinearProductLoad2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct NonlinearProductLoad2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonlinearProductLoad2Card& card);
    [[nodiscard]] static std::optional<NonlinearProductLoad2Card> read(FieldReader& reader);
    static void write(const NonlinearProductLoad2Card& card, FieldWriter& writer);
};

// All the NOLIN2 cards. The ID is a nonlinear load set ID shared by many cards (IdSpace::Group, category DynamicLoad):
// the NONLINEAR Case Control command selects it.
class NonlinearProductLoad2Table final : public TypedTable<NonlinearProductLoad2Table, NonlinearProductLoad2Card>
{
public:
    static constexpr std::string_view NAME = "NonlinearProductLoad2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DynamicLoad, base::IdSpace::Group};
    using Format = NonlinearProductLoad2Format;

    [[nodiscard]] static base::EntityId cardId(const NonlinearProductLoad2Card& card) noexcept { return card.id; }
    std::uint32_t append(const NonlinearProductLoad2Card& card, base::CardKey key);
    [[nodiscard]] NonlinearProductLoad2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const NonlinearProductLoad2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& nodeIColumn() noexcept { return m_nodeI; }
    [[nodiscard]] Column<std::int32_t>& componentsIColumn() noexcept { return m_componentsI; }
    [[nodiscard]] Column<double>& scaleFactorColumn() noexcept { return m_scaleFactor; }
    [[nodiscard]] Column<base::EntityId>& nodeJColumn() noexcept { return m_nodeJ; }
    [[nodiscard]] Column<std::int32_t>& componentsJColumn() noexcept { return m_componentsJ; }
    [[nodiscard]] Column<base::EntityId>& nodeKColumn() noexcept { return m_nodeK; }
    [[nodiscard]] Column<std::int32_t>& componentsKColumn() noexcept { return m_componentsK; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_nodeI;
    Column<std::int32_t> m_componentsI;
    Column<double> m_scaleFactor;
    Column<base::EntityId> m_nodeJ;
    Column<std::int32_t> m_componentsJ;
    Column<base::EntityId> m_nodeK;
    Column<std::int32_t> m_componentsK;
};

} // namespace oor::model
