#pragma once

#include "oor/model/cards/matrices/directTable/DirectTableCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct DirectTableFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DirectTableCard& card);
    [[nodiscard]] static std::optional<DirectTableCard> read(FieldReader& reader);
    static void write(const DirectTableCard& card, FieldWriter& writer);
};

// All the DTI cards. A card has no numeric ID (IdSpace::None): it is found by its name and record number. The values of
// every card are lists in CSR form, one per member of a value.
class DirectTableTable final : public TypedTable<DirectTableTable, DirectTableCard>
{
public:
    static constexpr std::string_view NAME = "DirectTable";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = DirectTableFormat;

    [[nodiscard]] static base::EntityId cardId(const DirectTableCard&) noexcept { return 0; }
    std::uint32_t append(const DirectTableCard& card, base::CardKey key);
    [[nodiscard]] DirectTableCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const DirectTableCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a DTI refers to no card

    [[nodiscard]] Column<std::string>& entryNameColumn() noexcept { return m_entryName; }
    [[nodiscard]] Column<std::int32_t>& recordNumberColumn() noexcept { return m_recordNumber; }
    [[nodiscard]] VariableLengthColumn<std::uint8_t>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& integerColumn() noexcept { return m_integer; }
    [[nodiscard]] VariableLengthColumn<double>& realColumn() noexcept { return m_real; }
    [[nodiscard]] VariableLengthColumn<std::string>& textColumn() noexcept { return m_text; }

private:
    Column<std::string> m_entryName;
    Column<std::int32_t> m_recordNumber;
    VariableLengthColumn<std::uint8_t> m_kind;
    VariableLengthColumn<std::int32_t> m_integer;
    VariableLengthColumn<double> m_real;
    VariableLengthColumn<std::string> m_text;
};

} // namespace oor::model
