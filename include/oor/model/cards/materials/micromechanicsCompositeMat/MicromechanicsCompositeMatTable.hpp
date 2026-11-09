#pragma once

#include "oor/model/cards/materials/micromechanicsCompositeMat/MicromechanicsCompositeMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct MicromechanicsCompositeMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MicromechanicsCompositeMatCard& card);
    [[nodiscard]] static std::optional<MicromechanicsCompositeMatCard> read(FieldReader& reader);
    static void write(const MicromechanicsCompositeMatCard& card, FieldWriter& writer);
};

// All the MATDIGI cards. The ID is a material ID, in the space every material type shares (IdSpace::Material). UDID
// names a UDNAME card: a UserDefinedName reference. The density is NaN where blank.
class MicromechanicsCompositeMatTable final
    : public TypedTable<MicromechanicsCompositeMatTable, MicromechanicsCompositeMatCard>
{
public:
    static constexpr std::string_view NAME = "MicromechanicsCompositeMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = MicromechanicsCompositeMatFormat;

    [[nodiscard]] static base::EntityId cardId(const MicromechanicsCompositeMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const MicromechanicsCompositeMatCard& card, base::CardKey key);
    [[nodiscard]] MicromechanicsCompositeMatCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const MicromechanicsCompositeMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& userDefinedNameIdColumn() noexcept { return m_userDefinedNameId; }
    [[nodiscard]] Column<double>& densityColumn() noexcept { return m_density; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_userDefinedNameId;
    Column<double> m_density;
};

} // namespace oor::model
