#pragma once

#include "oor/model/cards/materials/creepMat/CreepMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

struct CreepMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CreepMatCard& card);
    [[nodiscard]] static std::optional<CreepMatCard> read(FieldReader& reader);
    static void write(const CreepMatCard& card, FieldWriter& writer);

    // The text of a form ("" for a blank one) and the form of a word in any case (nothing for another word).
    [[nodiscard]] static const char* formWord(CreepMatForm form) noexcept;
    [[nodiscard]] static std::optional<CreepMatForm> parseForm(std::string_view text) noexcept;
};

// All the CREEP cards. The ID space is the card type's own: the ID is the one of the MAT1, MAT2 or MAT9 that the card
// extends, so it is also a reference to a material (renumbering the materials follows into it), and the three tables
// are references to TABLES1 cards (ReferenceTarget::StressTable). The coefficients are an (N, 7) NumPy view, NaN where
// blank.
class CreepMatTable final : public TypedTable<CreepMatTable, CreepMatCard>
{
public:
    static constexpr std::string_view NAME = "CreepMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = CreepMatFormat;

    CreepMatTable();

    [[nodiscard]] static base::EntityId cardId(const CreepMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const CreepMatCard& card, base::CardKey key);
    [[nodiscard]] CreepMatCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const CreepMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& referenceTemperatureColumn() noexcept { return m_referenceTemperature; }
    [[nodiscard]] Column<double>& exponentColumn() noexcept { return m_exponent; }
    [[nodiscard]] Column<std::uint8_t>& formColumn() noexcept { return m_form; }
    [[nodiscard]] Column<base::EntityId>& primaryStiffnessTableIdColumn() noexcept { return m_primaryStiffnessTableId; }
    [[nodiscard]] Column<base::EntityId>& primaryDampingTableIdColumn() noexcept { return m_primaryDampingTableId; }
    [[nodiscard]] Column<base::EntityId>& secondaryDampingTableIdColumn() noexcept { return m_secondaryDampingTableId; }
    [[nodiscard]] Column<double>& thresholdColumn() noexcept { return m_threshold; }
    [[nodiscard]] Column<std::int32_t>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] Column<std::array<double, 7>>& coefficientsColumn() noexcept { return m_coefficients; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_referenceTemperature;
    Column<double> m_exponent;
    Column<std::uint8_t> m_form;
    Column<base::EntityId> m_primaryStiffnessTableId;
    Column<base::EntityId> m_primaryDampingTableId;
    Column<base::EntityId> m_secondaryDampingTableId;
    Column<double> m_threshold;
    Column<std::int32_t> m_kind;
    Column<std::array<double, 7>> m_coefficients;
};

} // namespace oor::model
