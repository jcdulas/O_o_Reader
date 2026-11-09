#pragma once

#include "oor/model/cards/coordinate_systems/generalFrame3/GeneralFrame3Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct GeneralFrame3Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const GeneralFrame3Card& card);
    [[nodiscard]] static std::optional<GeneralFrame3Card> read(FieldReader& reader);
    static void write(const GeneralFrame3Card& card, FieldWriter& writer);
};

// All the CORD3G coordinate systems of a model (in the coordinate system ID space).
class GeneralFrame3Table final : public TypedTable<GeneralFrame3Table, GeneralFrame3Card>
{
public:
    static constexpr std::string_view NAME = "GeneralFrame3";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Coordinate, base::IdSpace::Coordinate};
    using Format = GeneralFrame3Format;

    GeneralFrame3Table()
        : m_methodName(std::string(GeneralFrame3Card::DEFAULT_METHOD_NAME)),
          m_matrixForm(std::string(GeneralFrame3Card::DEFAULT_MATRIX_FORM))
    {
    }

    [[nodiscard]] static base::EntityId cardId(const GeneralFrame3Card& card) noexcept { return card.id; }
    std::uint32_t append(const GeneralFrame3Card& card, base::CardKey key);
    [[nodiscard]] GeneralFrame3Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const GeneralFrame3Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::string>& methodNameColumn() noexcept { return m_methodName; }
    [[nodiscard]] Column<std::string>& matrixFormColumn() noexcept { return m_matrixForm; }
    [[nodiscard]] Column<std::array<base::EntityId, 3>>& angleFunctionIdsColumn() noexcept
    {
        return m_angleFunctionIds;
    }
    [[nodiscard]] Column<base::EntityId>& referenceFrameIdColumn() noexcept { return m_referenceFrameId; }

private:
    Column<base::EntityId> m_id;
    Column<std::string> m_methodName;
    Column<std::string> m_matrixForm;
    Column<std::array<base::EntityId, 3>> m_angleFunctionIds;
    Column<base::EntityId> m_referenceFrameId;
};

} // namespace oor::model
