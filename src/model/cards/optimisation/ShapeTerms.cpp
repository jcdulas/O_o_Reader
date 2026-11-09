#include "oor/model/cards/optimisation/ShapeTerms.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::vector<ShapeTerm> readShapeTerms(FieldReader& reader, int firstField)
{
    std::vector<ShapeTerm> terms;
    for (std::size_t index = 0; index < MAX_SHAPE_TERMS; ++index)
    {
        const int field = firstField + 2 * static_cast<int>(index);
        if (reader.isBlank(field))
        {
            if (!reader.isBlank(field + 1))
                reader.error("card.missing-field", "a scale factor needs its column", "columns");
            continue;
        }
        ShapeTerm term;
        term.column = reader.requiredInteger(field, "columns");
        term.scale = reader.real(field + 1, "scaleFactors", 1.0);
        terms.push_back(term);
    }
    return terms;
}

void writeShapeTerms(FieldWriter& writer, int firstField, const std::vector<ShapeTerm>& terms)
{
    int field = firstField;
    for (const ShapeTerm& term : terms)
    {
        writer.integer(field, term.column);
        writer.realUnlessDefault(field + 1, term.scale, 1.0);
        field += 2;
    }
}

std::optional<CardProblem> checkShapeTerms(const std::vector<ShapeTerm>& terms)
{
    if (terms.empty())
        return CardProblem{"card.missing-field",
                           "at least one column (" + std::string(CardProblem::FIELD) + ") is required", "columns"};
    if (terms.size() > MAX_SHAPE_TERMS)
        return CardProblem{"card.bad-value",
                           "at most three columns (" + std::string(CardProblem::FIELD) + ") are allowed", "columns"};
    for (const ShapeTerm& term : terms)
    {
        if (term.column < 1)
            return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer",
                               "columns"};
        if (const auto problem = checkReal(term.scale, "scaleFactors", RealRule::Finite))
            return problem;
    }
    return std::nullopt;
}

} // namespace oor::model
