#pragma once

#include <memory>

namespace oor::analysis
{

// The frames of the grid points and elements of a model, kept while several tables of it are turned (the tables of the
// subcases of one query have the same entities): give one cache to the frame changes of one model, and drop it when the
// model may change. Safe to share between threads.
class FrameCache
{
public:
    FrameCache();
    ~FrameCache();
    FrameCache(const FrameCache&) = delete;
    FrameCache& operator=(const FrameCache&) = delete;

    struct Impl;
    [[nodiscard]] Impl& impl() noexcept { return *m_impl; }

private:
    std::unique_ptr<Impl> m_impl;
};

} // namespace oor::analysis
