#pragma once

#include "oor/base/threading/ProgressCounter.hpp"
#include "oor/deck/records/ModelFileLexer.hpp"
#include "oor/model/format/CardLayouts.hpp"

#include <memory>

namespace oor::model
{
struct ModelReadOptions
{
    bool strict = false;
    deck::ReadOptions files;
    std::shared_ptr<const CardLayouts> layouts;
    base::ProgressCounter* progress = nullptr;
};
} // namespace oor::model
