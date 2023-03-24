// Copyright 2012-2021:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   Chris Murphy <cmurphy@aphysci.com>
//
//
// This file is part of the Dynamic Compact Control Language Library
// ("DCCL").
//
// DCCL is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// DCCL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DCCL.  If not, see <http://www.gnu.org/licenses/>.
#ifndef DCCLLOGGER20121009H
#define DCCLLOGGER20121009H

#include <cstdio>
#include <deque>
#include <functional>
#include <iomanip>
#include <iostream>
#include <stack>
#include <string>
#include <vector>

#include "dccl/thread_safety.h"

namespace dccl
{
namespace logger
{
/// Verbosity levels used by the Logger
enum Verbosity
{
    WARN = 1 << 1,
    INFO = 1 << 2,
    DEBUG1 = 1 << 3,
    DEBUG2 = 1 << 4,
    DEBUG3 = 1 << 5,
    ALL = DEBUG3 | (DEBUG3 - 1),
    WARN_PLUS = WARN | (WARN - 1),
    INFO_PLUS = INFO | (INFO - 1),
    DEBUG1_PLUS = DEBUG1 | (DEBUG1 - 1),
    DEBUG2_PLUS = DEBUG2 | (DEBUG2 - 1),
    DEBUG3_PLUS = DEBUG3 | (DEBUG3 - 1),
    UNKNOWN = 0
};
enum Group
{
    GENERAL,
    ENCODE,
    DECODE,
    SIZE
};

} // namespace logger

void to_ostream(const std::string& msg, dccl::logger::Verbosity vrb, dccl::logger::Group grp,
                std::ostream* os, bool add_timestamp);

class Logger;

namespace internal
{
class LogBuffer : public std::streambuf
{
  public:
    LogBuffer() : buffer_(1) {}
    ~LogBuffer() {}

    /// connect a signal to a slot (function pointer or similar)
    template <typename Slot> void connect(int verbosity_mask, Slot slot)
    {
        enabled_verbosities_ |= verbosity_mask;
        if (verbosity_mask & logger::WARN)
            warn_signal.emplace_back(slot);
        if (verbosity_mask & logger::INFO)
            info_signal.emplace_back(slot);
        if (verbosity_mask & logger::DEBUG1)
            debug1_signal.emplace_back(slot);
        if (verbosity_mask & logger::DEBUG2)
            debug2_signal.emplace_back(slot);
        if (verbosity_mask & logger::DEBUG3)
            debug3_signal.emplace_back(slot);
    }

    void disconnect(int verbosity_mask)
    {
        enabled_verbosities_ &= ~verbosity_mask;
        if (verbosity_mask & logger::WARN)
            warn_signal.clear();
        if (verbosity_mask & logger::INFO)
            info_signal.clear();
        if (verbosity_mask & logger::DEBUG1)
            debug1_signal.clear();
        if (verbosity_mask & logger::DEBUG2)
            debug2_signal.clear();
        if (verbosity_mask & logger::DEBUG3)
            debug3_signal.clear();
    }

    /// sets the verbosity level until the next sync()
    void set_verbosity(logger::Verbosity verbosity) { verbosity_.push(verbosity); }

    void set_group(logger::Group group) { group_.push(group); }

    bool contains(logger::Verbosity verbosity) const { return verbosity & enabled_verbosities_; }

  private:
    /// virtual inherited from std::streambuf.
    /// Called when std::endl or std::flush is inserted into the stream
    int sync() override;

    /// virtual inherited from std::streambuf. Called when something is inserted into the stream
    /// Called when std::endl or std::flush is inserted into the stream
    int overflow(int c = EOF) override;

    void display(const std::string& s)
    {
        if (verbosity() & logger::WARN)
        {
            for (auto& slot : warn_signal) slot(s, logger::WARN, group());
        }
        if (verbosity() & logger::INFO)
        {
            for (auto& slot : info_signal) slot(s, logger::INFO, group());
        }
        if (verbosity() & logger::DEBUG1)
        {
            for (auto& slot : debug1_signal) slot(s, logger::DEBUG1, group());
        }
        if (verbosity() & logger::DEBUG2)
        {
            for (auto& slot : debug2_signal) slot(s, logger::DEBUG2, group());
        }
        if (verbosity() & logger::DEBUG3)
        {
            for (auto& slot : debug3_signal) slot(s, logger::DEBUG3, group());
        }
    }

    logger::Verbosity verbosity()
    {
        return verbosity_.empty() ? logger::UNKNOWN : verbosity_.top();
    }
    logger::Group group() { return group_.empty() ? logger::GENERAL : group_.top(); }

  private:
    std::stack<logger::Verbosity> verbosity_;
    std::stack<logger::Group> group_;
    std::deque<std::string> buffer_;
    int enabled_verbosities_; // mask of verbosity settings enabled

    using LogSignal = std::vector<
        std::function<void(const std::string& msg, logger::Verbosity vrb, logger::Group grp)>>;

    LogSignal warn_signal, info_signal, debug1_signal, debug2_signal, debug3_signal;
};
} // namespace internal

/// The DCCL Logger class. Do not instantiate this class directly. Rather, use the dccl::dlog object.
class Logger : public std::ostream
{
  public:
    Logger() : std::ostream(&buf_) {}
    virtual ~Logger() {}

    /// \brief Same as is() but doesn't set the verbosity or lock the mutex.
    bool check(logger::Verbosity verbosity) { return buf_.contains(verbosity); }

    /// \brief Indicates the verbosity of the Logger until the next std::flush or std::endl. The boolean return is used to take advantage of short-circuit evaluation of && to avoid spending CPU time generating log files that if they are not used. This locks the dlog mutex when running with DCCL_THREAD_SUPPORT
    ///
    /// The typical usage is
    /// \code
    /// dlog.is(INFO) && dlog << "Something of interest." << std::endl;
    /// dlog.is(WARN, ENCODE) && dlog << "Something bad happened while encoding." << std::endl;
    /// \endcode
    /// \param verbosity The verbosity level to tag the following message with. These levels are used to direct the output of dlog to different logs or omit them completely.
    /// \param group The group that this message belongs to.
    bool is(logger::Verbosity verbosity, logger::Group group = logger::GENERAL)
    {
        if (!buf_.contains(verbosity))
        {
            return false;
        }
        else
        {
#if DCCL_THREAD_SUPPORT
            g_dlog_mutex.lock();
#endif
            buf_.set_verbosity(verbosity);
            buf_.set_group(group);
            return true;
        }
    }

    /// \brief Connect the output of one or more given verbosities to a slot (function pointer or similar)
    ///
    /// \param verbosity_mask A bitmask representing the verbosity or verbosities to send to this slot. For example, you can use connect(WARN | INFO, slot) to send both WARN and INFO messages to slot.
    /// \param slot The slot must be a function pointer or like object (e.g. boost::function) of type
    /// (void*) (const std::string& msg, logger::Verbosity vrb, logger::Group grp)
    template <typename Slot> void connect(int verbosity_mask, Slot slot)
    {
        LOCK_DLOG_MUTEX
        buf_.connect(verbosity_mask, slot);
    }

    /// \brief Connect the output of one or more given verbosities to a member function
    ///
    /// \param verbosity_mask A bitmask representing the verbosity or verbosities to send to this slot. For example, you can use connect(WARN | INFO, this, &MyClass::slot) to send both WARN and INFO messages to MyClass::slot.
    /// \param obj Pointer to the instantiation of the class of which this member function should be called.
    /// \param mem_func Member function of type
    /// (void*) (const std::string& msg, logger::Verbosity vrb, logger::Group grp)
    template <typename Obj>
    void connect(int verbosity_mask, Obj* obj,
                 void (Obj::*mem_func)(const std::string& msg, logger::Verbosity vrb,
                                       logger::Group grp))
    {
        LOCK_DLOG_MUTEX
        connect(verbosity_mask, std::bind(mem_func, obj, std::placeholders::_1,
                                          std::placeholders::_2, std::placeholders::_3));
    }

    /// \brief Connect the output of one or more given verbosities to a std::ostream
    ///
    /// \param verbosity_mask A bitmask representing the verbosity or verbosities to send to this slot. For example, you can use connect(WARN | INFO, &std::cout) to send both WARN and INFO messages to std::cout.
    /// \param os Pointer to a std::ostream
    /// \param add_timestamp If true, prepend the current timestamp of the message to each log message.
    void connect(int verbosity_mask, std::ostream* os, bool add_timestamp = true)
    {
        LOCK_DLOG_MUTEX
        buf_.connect(verbosity_mask,
                     std::bind(to_ostream, std::placeholders::_1, std::placeholders::_2,
                               std::placeholders::_3, os, add_timestamp));
    }

    /// \brief Disconnect all slots for one or more given verbosities
    void disconnect(int verbosity_mask)
    {
        LOCK_DLOG_MUTEX
        buf_.disconnect(verbosity_mask);
    }

  private:
    internal::LogBuffer buf_;
};

extern Logger dlog;
} // namespace dccl

#endif // DCCLLOGGER20121009H
