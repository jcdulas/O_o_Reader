#pragma once

#include "oor/model/cards/thermal/viewFactorFace/ViewFactorFaceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct ViewFactorFaceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ViewFactorFaceCard& card);
    [[nodiscard]] static std::optional<ViewFactorFaceCard> read(FieldReader& reader);
    static void write(const ViewFactorFaceCard& card, FieldWriter& writer);
};

// All the VIEW cards. The ID is the IVIEW the surface elements name (IVIEWF, IVIEWB), in a space of its own (category
// ViewFactorFaceFactor, IdSpace::PerCardType). ICAVITY refers to the radiation cavity.
class ViewFactorFaceTable final : public TypedTable<ViewFactorFaceTable, ViewFactorFaceCard>
{
public:
    static constexpr std::string_view NAME = "ViewFactorFace";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ViewFactorFaceFactor, base::IdSpace::PerCardType};
    using Format = ViewFactorFaceFormat;

    ViewFactorFaceTable();

    [[nodiscard]] static base::EntityId cardId(const ViewFactorFaceCard& card) noexcept { return card.id; }
    std::uint32_t append(const ViewFactorFaceCard& card, base::CardKey key);
    [[nodiscard]] ViewFactorFaceCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ViewFactorFaceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& cavityIdColumn() noexcept { return m_cavityId; }
    [[nodiscard]] Column<std::string>& shadingColumn() noexcept { return m_shading; }
    [[nodiscard]] Column<std::int32_t>& betaSubdivisionsColumn() noexcept { return m_betaSubdivisions; }
    [[nodiscard]] Column<std::int32_t>& gammaSubdivisionsColumn() noexcept { return m_gammaSubdivisions; }
    [[nodiscard]] Column<double>& surfaceOffsetColumn() noexcept { return m_surfaceOffset; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_cavityId;
    Column<std::string> m_shading;
    Column<std::int32_t> m_betaSubdivisions;
    Column<std::int32_t> m_gammaSubdivisions;
    Column<double> m_surfaceOffset;
};

} // namespace oor::model
