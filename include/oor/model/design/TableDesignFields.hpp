#pragma once

#include "oor/model/design/DesignFieldNames.hpp"
#include "oor/model/design/DesignableCard.hpp"

#include <span>
#include <string_view>

namespace oor::model
{

// The plumbing between the three functions a card type writes and the `DesignableCard` the design model uses: it finds
// the table in the model, takes the card of the row and puts it back when its fields changed. It knows nothing about
// the fields of any card; those are the functions the card type passes in.
//
//   find   which field a name or a position is on this card
//   read   the value of a field
//   write  gives the fields their values (all of those of the card at once, see DesignableCard::write)
//   column the contract column of a field (see DesignColumn)
template <typename TableT>
class TableDesignFields final : public DesignableCard
{
public:
    using Card = typename TableT::CardType;

    struct Functions
    {
        DesignFieldLookup (*find)(const Card& card, const DesignField& field);
        double (*read)(const Card& card, const DesignFieldFacts& facts);
        void (*write)(Card& card, std::span<const DesignFieldValue> values);
        DesignColumn (*column)(const Card& card, const DesignFieldFacts& facts);
    };

    TableDesignFields(std::string_view name, Functions functions) : m_name(name), m_functions(functions) {}

    [[nodiscard]] std::string_view cardName() const noexcept override { return m_name; }

    [[nodiscard]] DesignFieldLookup find(const Model& model, std::uint32_t row, const DesignField& field) const override
    {
        return m_functions.find(table(model).card(row), field);
    }

    [[nodiscard]] double read(const Model& model, std::uint32_t row, const DesignFieldFacts& facts) const override
    {
        return m_functions.read(table(model).card(row), facts);
    }

    [[nodiscard]] DesignColumn column(const Model& model, std::uint32_t row,
                                      const DesignFieldFacts& facts) const override
    {
        return m_functions.column(table(model).card(row), facts);
    }

    void write(Model& model, std::uint32_t row, std::span<const DesignFieldValue> values) const override
    {
        TableT& target = model.template table<TableT>();
        Card card = target.card(row);
        m_functions.write(card, values);
        target.store(row, card);
    }

private:
    static const TableT& table(const Model& model) { return *model.template findTable<TableT>(); }

    std::string_view m_name;
    Functions m_functions;
};

} // namespace oor::model
