#pragma once

#include "oor/model/cards/elements/bar2Defaults/Bar2DefaultsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct Bar2DefaultsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const Bar2DefaultsCard& card);
    [[nodiscard]] static std::optional<Bar2DefaultsCard> read(FieldReader& reader);
    static void write(const Bar2DefaultsCard& card, FieldWriter& writer);
};

// The BAROR cards of a model (at most one is allowed). The card has no ID.
class Bar2DefaultsTable final : public TypedTable<Bar2DefaultsTable, Bar2DefaultsCard>
{
public:
    static constexpr std::string_view NAME = "Bar2Defaults";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = Bar2DefaultsFormat;

    Bar2DefaultsTable() : m_offsetKind(std::string(Bar2DefaultsCard::DEFAULT_OFFSET_KIND)) {}

    [[nodiscard]] static base::EntityId cardId(const Bar2DefaultsCard&) noexcept { return 0; }
    std::uint32_t append(const Bar2DefaultsCard& card, base::CardKey key);
    [[nodiscard]] Bar2DefaultsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const Bar2DefaultsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;
    // Reports a second BAROR: the solver allows one.
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<double, 3>>& orientationColumn() noexcept { return m_orientation; }
    [[nodiscard]] Column<base::EntityId>& orientationNodeColumn() noexcept { return m_orientationNode; }
    [[nodiscard]] Column<std::string>& offsetKindColumn() noexcept { return m_offsetKind; }

private:
    Column<base::EntityId> m_propId;
    Column<std::array<double, 3>> m_orientation;
    Column<base::EntityId> m_orientationNode;
    Column<std::string> m_offsetKind;
};

} // namespace oor::model
