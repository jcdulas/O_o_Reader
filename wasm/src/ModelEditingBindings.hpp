#pragma once
#include "oor/api/Deck.hpp"

#include <emscripten/bind.h>

namespace oor::wasm
{
void bindModelEditing(emscripten::class_<api::Deck>& binding);
}
