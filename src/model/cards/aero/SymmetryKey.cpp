#include "oor/model/cards/aero/SymmetryKey.hpp"

#include <string>

namespace oor::model
{

std::optional<CardProblem> checkSymmetryKey(std::int32_t key, const char* field)
{
    if (key < -1 || key > 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be -1, 0 or 1", field};
    return std::nullopt;
}

} // namespace oor::model
