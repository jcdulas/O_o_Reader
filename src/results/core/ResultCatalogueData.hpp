#pragma once

#include "oor/results/core/ResultKind.hpp"

#include <vector>

// The raw rows of the result catalogue, private to the results module. ResultCatalogue.cpp turns them into
// TableLayout objects once.
namespace oor::results::catalogue
{

struct LayoutRow
{
    ResultKind kind;
    const char* elementType; // "" for the kinds that are not families
    bool complex;
    const char* components;   // separated by spaces
    const char* alternatives; // lists separated by ';', the names of one list by spaces
    const char* rowTexts;     // separated by spaces
};

struct CardRow
{
    const char* elementType; // "ShellQuad4Composite"
    const char* card;        // the card type: "ShellQuad4"
};

[[nodiscard]] const std::vector<LayoutRow>& layoutRows();
[[nodiscard]] const std::vector<CardRow>& cardRows();

} // namespace oor::results::catalogue
