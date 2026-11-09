#pragma once

#include "oor/model/cards/constraints/cyclicSupport/CyclicSupportCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct CyclicSupportFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CyclicSupportCard& card);
    [[nodiscard]] static std::optional<CyclicSupportCard> read(FieldReader& reader);
    static void write(const CyclicSupportCard& card, FieldWriter& writer);
};

// All the CYSUP cards. A support has no ID of its own (IdSpace::None), like the SUPORT it resembles; the format
// reference allows one supported grid point in the model, which a check between cards looks at. The grid point is a
// reference.
class CyclicSupportTable final : public TypedTable<CyclicSupportTable, CyclicSupportCard>
{
public:
    static constexpr std::string_view NAME = "CyclicSupport";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Constraint, base::IdSpace::None};
    using Format = CyclicSupportFormat;

    [[nodiscard]] static base::EntityId cardId(const CyclicSupportCard&) noexcept { return 0; }
    std::uint32_t append(const CyclicSupportCard& card, base::CardKey key);
    [[nodiscard]] CyclicSupportCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const CyclicSupportCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& nodeIdColumn() noexcept { return m_nodeId; }
    [[nodiscard]] Column<std::int32_t>& componentsColumn() noexcept { return m_components; }

private:
    Column<base::EntityId> m_nodeId;
    Column<std::int32_t> m_components;
};

} // namespace oor::model
