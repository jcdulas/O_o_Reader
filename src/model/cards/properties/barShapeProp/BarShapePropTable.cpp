#include "oor/model/cards/properties/barShapeProp/BarShapePropTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/properties/sections/SectionCatalogue.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_DIMENSION_FIELD = 9;

const char* dimensionName(std::size_t index)
{
    static constexpr const char* NAMES[10] = {"dimension1", "dimension2", "dimension3", "dimension4", "dimension5",
                                              "dimension6", "dimension7", "dimension8", "dimension9", "dimension10"};
    return NAMES[index < 10 ? index : 9];
}

} // namespace

std::size_t BarShapePropFormat::dimensionCount(std::string_view type) noexcept
{
    return sections::dimensionCount(type);
}

std::optional<BarShapePropCard> BarShapePropFormat::read(FieldReader& reader)
{
    BarShapePropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    if (const std::string_view group = reader.text(3); !group.empty() && !(group == sections::STANDARD_SECTION_LIBRARY))
        reader.error("card.bad-value",
                     "only the standard section library (" + std::string("sectionGroup") +
                         " blank or its name) is supported, found \"" + std::string(group) + "\"",
                     "sectionGroup");
    card.kind = std::string(reader.text(4));
    const std::size_t count = dimensionCount(card.kind);
    if (card.kind.empty())
    {
        reader.error("card.missing-field", std::string("kind") + " is required", "kind");
        return card;
    }
    if (count == 0)
    {
        reader.error("card.bad-value",
                     std::string("kind") + " \"" + card.kind + "\" is not a section of the standard library", "kind");
        return card;
    }
    for (int field = 5; field <= 8; ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value", "fields 5 to 8 of a " + std::string(reader.cardName()) + " must be blank",
                         "kind");
    }
    for (std::size_t index = 0; index < count; ++index)
    {
        const int field = FIRST_DIMENSION_FIELD + static_cast<int>(index);
        if (card.kind == "DoubleBoxSection" && index >= 4)
        {
            // DIM5 to DIM8 default to DIM4, DIM9 and DIM10 to DIM6.
            const double fallback = index < 8 ? card.dims[3] : card.dims[5];
            card.dims.push_back(reader.real(field, dimensionName(index), fallback));
        }
        else
        {
            card.dims.push_back(reader.requiredReal(field, dimensionName(index)));
        }
    }
    const int nsmField = FIRST_DIMENSION_FIELD + static_cast<int>(count);
    card.nonStructMass = reader.real(nsmField, "nonStructMass", 0.0);
    if (reader.lastUsedField() > nsmField)
        reader.error("card.bad-value", "a " + card.kind + " section takes " + std::to_string(count) + " dimensions",
                     "nonStructMass");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BarShapePropFormat::check(const BarShapePropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.matId <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "matId"};
    const std::size_t count = dimensionCount(card.kind);
    if (count == 0)
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be a section of the standard library, not \"" + card.kind + "\"",
            "kind"};
    if (card.dims.size() != count)
        return CardProblem{"card.bad-value",
                           "a " + card.kind + " section takes " + std::to_string(count) + " dimensions, not " +
                               std::to_string(card.dims.size()),
                           "dimension1"};
    for (std::size_t index = 0; index < count; ++index)
    {
        if (!(card.dims[index] > 0.0))
            return CardProblem{"card.bad-value", "the dimensions must be greater than 0.0", dimensionName(index)};
    }
    return std::nullopt;
}

void BarShapePropFormat::write(const BarShapePropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    // GROUP is blank: the standard library is the default and the only one supported.
    writer.text(4, card.kind);
    for (std::size_t index = 0; index < card.dims.size(); ++index)
        writer.real(FIRST_DIMENSION_FIELD + static_cast<int>(index), card.dims[index]);
    writer.realUnlessDefault(FIRST_DIMENSION_FIELD + static_cast<int>(card.dims.size()), card.nonStructMass, 0.0);
}

std::uint32_t BarShapePropTable::append(const BarShapePropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_kind.push(card.kind);
    m_dims.push(card.dims);
    m_nonStructMass.push(card.nonStructMass);
    return row;
}

BarShapePropCard BarShapePropTable::card(std::uint32_t row) const
{
    BarShapePropCard card;
    card.id = m_id.get(row);
    card.matId = m_matId.get(row);
    card.kind = m_kind.get(row);
    const auto dims = m_dims.get(row);
    card.dims.assign(dims.begin(), dims.end());
    card.nonStructMass = m_nonStructMass.get(row);
    return card;
}

void BarShapePropTable::store(std::uint32_t row, const BarShapePropCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_kind.set(row, card.kind);
    m_dims.set(row, card.dims);
    m_nonStructMass.set(row, card.nonStructMass);
}

void BarShapePropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_kind.keepRows(rows);
    m_dims.keepRows(rows);
    m_nonStructMass.keepRows(rows);
}

void BarShapePropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
}

} // namespace oor::model
