#include "oor/analysis/components/SubcaseCombination.hpp"

#include <cmath>

namespace oor::analysis
{

namespace
{

// The numbers of a command's value, in order (a list or a single number); empty when the command is absent.
std::vector<double> numbersOf(const model::Subcase& subcase, const char* command)
{
    const model::CaseEntry* entry = model::CaseControl::ownCommand(subcase, command);
    std::vector<double> numbers;
    if (entry == nullptr)
        return numbers;
    const model::CaseValue& value = entry->value;
    if (value.kind == model::CaseValueKind::RealList)
        numbers = value.reals;
    else if (value.kind == model::CaseValueKind::Real)
        numbers.push_back(value.real);
    else if (value.kind == model::CaseValueKind::IntegerList)
        numbers.assign(value.integers.begin(), value.integers.end());
    else if (value.kind == model::CaseValueKind::Integer)
        numbers.push_back(static_cast<double>(value.integer));
    return numbers;
}

// The subcases of `wanted` kind before `index`, each with the coefficient of a list aligned on the last of them.
std::vector<CombinationTerm> alignedTerms(const std::vector<const model::Subcase*>& subcases, std::size_t index,
                                          model::SubcaseKind wanted, const std::vector<double>& coefficients,
                                          double fallback)
{
    std::vector<CombinationTerm> terms;
    for (std::size_t before = 0; before < index; ++before)
    {
        if (subcases[before]->kind == wanted)
            terms.push_back(CombinationTerm{before, fallback});
    }
    if (coefficients.empty())
        return terms;
    for (auto& term : terms)
        term.coefficient = 0.0;
    // The last coefficient goes to the last subcase, and so on backwards.
    std::size_t term = terms.size();
    std::size_t coefficient = coefficients.size();
    while (term > 0 && coefficient > 0)
    {
        --term;
        --coefficient;
        terms[term].coefficient = coefficients[coefficient];
    }
    return terms;
}

} // namespace

std::vector<CombinationTerm> SubcaseCombination::termsOf(const std::vector<const model::Subcase*>& subcases,
                                                         std::size_t index)
{
    const model::Subcase& combination = *subcases[index];
    if (combination.kind == model::SubcaseKind::SymmetryCombination)
    {
        const std::vector<double> coefficients = numbersOf(combination, "SymmetryCombinationFactors");
        return alignedTerms(subcases, index, model::SubcaseKind::SymmetrySubcase, coefficients, 1.0);
    }
    if (combination.kind != model::SubcaseKind::SubcaseCombination)
        return {};
    const std::vector<double> explicitList = numbersOf(combination, "SubcaseCombinationFactors1");
    if (explicitList.size() < 3)
        return alignedTerms(subcases, index, model::SubcaseKind::Subcase,
                            numbersOf(combination, "SubcaseCombinationFactors"), 0.0);
    // SUBSEQ1 = S0, S1, SUB1, S2, SUB2, ...
    std::vector<CombinationTerm> terms;
    const double overall = explicitList[0];
    for (std::size_t item = 1; item + 1 < explicitList.size(); item += 2)
    {
        const auto wantedId = static_cast<std::int64_t>(std::llround(explicitList[item + 1]));
        for (std::size_t before = 0; before < index; ++before)
        {
            if (subcases[before]->kind == model::SubcaseKind::Subcase && subcases[before]->id == wantedId)
            {
                terms.push_back(CombinationTerm{before, overall * explicitList[item]});
                break;
            }
        }
    }
    return terms;
}

} // namespace oor::analysis
