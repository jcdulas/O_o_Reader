#pragma once

#include "oor/model/cards/aero/aeroBodyProp2/AeroBodyProp2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct AeroBodyProp2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroBodyProp2Card& card);
    [[nodiscard]] static std::optional<AeroBodyProp2Card> read(FieldReader& reader);
    static void write(const AeroBodyProp2Card& card, FieldWriter& writer);
};

// All the PAERO2 cards. The PID is in the space the five PAERO cards share (IdSpace::AeroProperty, category
// AeroProperty): a CAERO2 names it (ReferenceTarget::AeroProperty). LRSB, LRIB, LTH1 and LTH2 are AEFACT cards
// (ReferenceTarget::AeroFactor).
class AeroBodyProp2Table final : public TypedTable<AeroBodyProp2Table, AeroBodyProp2Card>
{
public:
    static constexpr std::string_view NAME = "AeroBodyProp2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroProperty, base::IdSpace::AeroProperty};
    using Format = AeroBodyProp2Format;

    [[nodiscard]] static base::EntityId cardId(const AeroBodyProp2Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroBodyProp2Card& card, base::CardKey key);
    [[nodiscard]] AeroBodyProp2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroBodyProp2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& orientationColumn() noexcept { return m_orientation; }
    [[nodiscard]] Column<double>& widthColumn() noexcept { return m_width; }
    [[nodiscard]] Column<double>& aspectRatioColumn() noexcept { return m_aspectRatio; }
    [[nodiscard]] Column<base::EntityId>& slenderHalfWidthListIdColumn() noexcept { return m_slenderHalfWidthListId; }
    [[nodiscard]] Column<base::EntityId>& interferenceHalfWidthListIdColumn() noexcept
    {
        return m_interferenceHalfWidthListId;
    }
    [[nodiscard]] Column<base::EntityId>& angleListId1Column() noexcept { return m_angleListId1; }
    [[nodiscard]] Column<base::EntityId>& angleListId2Column() noexcept { return m_angleListId2; }
    [[nodiscard]] Column<base::EntityId>& angleList1FirstElemColumn() noexcept { return m_angleList1FirstElem; }
    [[nodiscard]] Column<base::EntityId>& angleList1LastElemColumn() noexcept { return m_angleList1LastElem; }
    [[nodiscard]] Column<base::EntityId>& angleList2FirstElemColumn() noexcept { return m_angleList2FirstElem; }
    [[nodiscard]] Column<base::EntityId>& angleList2LastElemColumn() noexcept { return m_angleList2LastElem; }

private:
    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_orientation;
    Column<double> m_width;
    Column<double> m_aspectRatio;
    Column<base::EntityId> m_slenderHalfWidthListId;
    Column<base::EntityId> m_interferenceHalfWidthListId;
    Column<base::EntityId> m_angleListId1;
    Column<base::EntityId> m_angleListId2;
    Column<base::EntityId> m_angleList1FirstElem;
    Column<base::EntityId> m_angleList1LastElem;
    Column<base::EntityId> m_angleList2FirstElem;
    Column<base::EntityId> m_angleList2LastElem;
};

} // namespace oor::model
