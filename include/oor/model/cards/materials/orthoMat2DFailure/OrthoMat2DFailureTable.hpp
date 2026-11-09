#pragma once

#include "oor/model/cards/materials/orthoMat2DFailure/OrthoMat2DFailureCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

struct OrthoMat2DFailureFormat
{
    // The words of the keyword fields and the value a word names (case does not matter; the word of
    // OrthoMat2DFailureTheory::None is empty and the empty text is that value).
    [[nodiscard]] static const char* theoryWord(OrthoMat2DFailureTheory theory) noexcept;
    [[nodiscard]] static std::optional<OrthoMat2DFailureTheory> parseTheory(std::string_view text) noexcept;
    [[nodiscard]] static const char* shearFailureWord(OrthoMat2DFailureShearFailure value) noexcept;
    [[nodiscard]] static std::optional<OrthoMat2DFailureShearFailure> parseShearFailure(std::string_view text) noexcept;
    [[nodiscard]] static const char* degradationWord(OrthoMat2DFailureDegradation value) noexcept;
    [[nodiscard]] static std::optional<OrthoMat2DFailureDegradation> parseDegradation(std::string_view text) noexcept;
    [[nodiscard]] static const char* degradationStartWord(OrthoMat2DFailureDegradationStart value) noexcept;
    [[nodiscard]] static std::optional<OrthoMat2DFailureDegradationStart> parseDegradationStart(
        std::string_view text) noexcept;

    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const OrthoMat2DFailureCard& card);
    [[nodiscard]] static std::optional<OrthoMat2DFailureCard> read(FieldReader& reader);
    static void write(const OrthoMat2DFailureCard& card, FieldWriter& writer);
};

// All the MAT8A cards. The ID space is the card type's own: the ID is the one of the MAT8 that the card extends, so it
// is also a reference to a material (renumbering the materials follows into it). The cards are kept whole (CardRows);
// the query engine reads the ID only.
class OrthoMat2DFailureTable final : public TypedTable<OrthoMat2DFailureTable, OrthoMat2DFailureCard>
{
public:
    static constexpr std::string_view NAME = "OrthoMat2DFailure";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = OrthoMat2DFailureFormat;

    [[nodiscard]] static base::EntityId cardId(const OrthoMat2DFailureCard& card) noexcept { return card.id; }
    std::uint32_t append(const OrthoMat2DFailureCard& card, base::CardKey key);
    [[nodiscard]] OrthoMat2DFailureCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const OrthoMat2DFailureCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<OrthoMat2DFailureCard> m_cards;
};

} // namespace oor::model
