#pragma once

// Helpers shared by the sources of src/io/hdf5. This header includes the libhdf5 headers: it is private to this folder
// (tools/check_layering.py allows HDF5 headers here and nowhere else).

#include "oor/base/errors/Errors.hpp"
#include "oor/io/hdf5/Hdf5Handle.hpp"
#include "oor/io/hdf5/Hdf5Limits.hpp"
#include "oor/io/hdf5/Hdf5Type.hpp"

#include <hdf5.h>

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace oor::io::detail
{

// libhdf5 is built without thread safety here: every operation of the generic layer holds this lock for its whole
// duration, so that two Python threads (the GIL is released during a read) never run in the library together.
[[nodiscard]] std::mutex& hdf5Mutex();

// Silences the error stack (once); called by every entry point of the generic layer.
void ensureInitialised();

// The description of the innermost error of the library's stack, "" when there is none. Call right after a failed call.
[[nodiscard]] std::string lastErrorDetail();

[[noreturn]] void fail(const std::string& what);

[[nodiscard]] Hdf5Handle checked(hid_t identifier, const std::string& what);

[[nodiscard]] inline hid_t id(const Hdf5Handle& handle) noexcept
{
    return static_cast<hid_t>(handle.get());
}

// a * b, or a LimitExceededError that says `what` when the product does not fit in 64 bits.
[[nodiscard]] std::uint64_t checkedProduct(std::uint64_t first, std::uint64_t second, const std::string& what);

[[nodiscard]] std::uint64_t checkedProduct(const std::vector<std::uint64_t>& values, const std::string& what);

// The description of a libhdf5 type. Throws LimitExceededError past the limits on nesting and members.
[[nodiscard]] Hdf5Type describeType(hid_t type, const Hdf5Limits& limits, std::uint32_t depth = 0);

// Throws IoError when a component of `path` (relative to `base`, or absolute) is an external link: the generic layer
// never follows one. A soft link is followed by name (to look for one behind it, at most 16 hops); a component that
// does not exist ends the check (the open that follows reports it).
void refuseExternalLinks(hid_t base, const std::string& path);

[[nodiscard]] Hdf5Handle openFileReadOnly(const std::string& utf8Path);

// The text with every byte that is not valid UTF-8 replaced by U+FFFD. A message carries names that come from the file,
// which may be damaged; Python refuses to build an exception from text that is not UTF-8 (it would raise
// UnicodeDecodeError instead of the error of the library).
[[nodiscard]] std::string validUtf8(const std::string& text);

// Runs `function`; an error of the library that comes out of it carries a message that is valid UTF-8.
template <typename Function>
auto cleanErrors(Function&& function) -> decltype(function())
{
    try
    {
        return function();
    }
    catch (const base::LimitExceededError& error)
    {
        throw base::LimitExceededError(validUtf8(error.what()));
    }
    catch (const base::UsageError& error)
    {
        throw base::UsageError(validUtf8(error.what()));
    }
    catch (const base::IoError& error)
    {
        throw base::IoError(validUtf8(error.what()));
    }
}

} // namespace oor::io::detail
