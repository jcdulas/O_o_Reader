#pragma once

#include "oor/base/errors/Errors.hpp"
#include "oor/deck/raw/CardName.hpp"
#include "oor/deck/raw/RawCard.hpp"
#include "oor/model/core/CardTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <string>
#include <vector>

namespace oor::model
{

// Cards the library does not know, or that failed to parse in tolerant mode: kept as text (name and field texts) and
// written back unchanged at their place. Nothing about them is interpreted.
class UnknownCardTable final : public CardTable
{
public:
    static constexpr std::string_view NAME = "UNKNOWN";

    [[nodiscard]] const CardTypeInfo& info() const noexcept override;

    // Adds a card from its raw fields; the text is copied (the source file may be unmapped afterwards).
    std::uint32_t append(base::CardKey key, const deck::CardName& name, std::span<const std::string_view> fields);

    [[nodiscard]] deck::CardName nameAt(std::uint32_t row) const;
    // The field texts of the card in `row`.
    [[nodiscard]] std::vector<std::string> fieldsAt(std::uint32_t row) const;

    [[nodiscard]] base::EntityId idAt(std::uint32_t) const override { return 0; }
    void setIdAt(std::uint32_t, base::EntityId) override {}
    // A card kept as text is not edited in place (it is removed and added again as text).
    void moveLastRowTo(std::uint32_t /*row*/) override
    {
        throw base::UsageError("a card the library does not know cannot be edited in place");
    }

    [[nodiscard]] bool readCards(FieldReader&, KeySource&, std::vector<std::uint32_t>&) override
    {
        return false; // see append()
    }
    void writeCard(std::uint32_t, FieldWriter&) const override {} // written by name/fields
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}
    void compact() override;
    [[nodiscard]] std::unique_ptr<CardTable> clone() const override
    {
        return std::make_unique<UnknownCardTable>(*this);
    }

private:
    std::vector<deck::CardName> m_names;
    // The field texts of a card, joined with '\x1f' (blank fields are empty between separators).
    std::vector<std::string> m_text;
};

} // namespace oor::model
