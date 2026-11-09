#include "oor/model/cards/optimisation/DesignTerms.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace oor::model
{

std::vector<DesignTerm> readDesignTerms(FieldReader& reader, int firstField, const char* idName,
                                        const char* coefficientName, bool allowFromProperty, bool& fromProperty)
{
    fromProperty = false;
    std::vector<DesignTerm> terms;
    for (int field = firstField; field <= reader.lastUsedField(); field += 2)
    {
        const bool noId = reader.isBlank(field);
        const bool noCoefficient = reader.isBlank(field + 1);
        if (noId && noCoefficient)
            continue;
        if (noId || noCoefficient)
        {
            reader.error(
                "card.missing-field",
                std::string(idName) + " and " + std::string(coefficientName) + " go together: one of them is blank",
                noId ? idName : coefficientName);
            return terms;
        }
        DesignTerm term;
        term.designVarId = reader.requiredInteger(field, idName);
        if (allowFromProperty && terms.empty() && (reader.text(field + 1) == "PropertyValue"))
            fromProperty = true;
        else
            term.coefficient = reader.requiredReal(field + 1, coefficientName);
        terms.push_back(term);
    }
    return terms;
}

void writeDesignTerms(FieldWriter& writer, int firstField, const std::vector<DesignTerm>& terms, bool fromProperty)
{
    int field = firstField;
    for (std::size_t index = 0; index < terms.size(); ++index, field += 2)
    {
        writer.integer(field, terms[index].designVarId);
        if (index == 0 && fromProperty)
            writer.text(field + 1, "PropertyValue");
        else
            writer.real(field + 1, terms[index].coefficient);
    }
}

std::optional<CardProblem> checkDesignTerms(const std::vector<DesignTerm>& terms, bool fromProperty, bool distinct,
                                            const char* idName, const char* coefficientName)
{
    if (terms.empty())
        return CardProblem{"card.missing-field", "at least one " + std::string(CardProblem::FIELD) + " is required",
                           idName};
    for (std::size_t index = 0; index < terms.size(); ++index)
    {
        if (const auto problem = checkRequiredId(terms[index].designVarId, idName))
            return problem;
        const bool isPval = index == 0 && fromProperty;
        if (!isPval)
        {
            if (const auto problem = checkReal(terms[index].coefficient, coefficientName, RealRule::Finite))
                return problem;
        }
        for (std::size_t earlier = 0; distinct && earlier < index; ++earlier)
        {
            if (terms[earlier].designVarId == terms[index].designVarId)
                return CardProblem{
                    "card.bad-value",
                    std::string("the design variable ") + std::to_string(terms[index].designVarId) + " is listed twice",
                    idName};
        }
    }
    return std::nullopt;
}

void visitDesignTerms(ReferenceVisitor& visitor, std::vector<DesignTerm>& terms, const char* idName)
{
    for (DesignTerm& term : terms)
        visitId(visitor, idName, ReferenceTarget::DesignVariable, term.designVarId);
}

} // namespace oor::model
