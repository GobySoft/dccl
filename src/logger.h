// Copyright 2012- DCCL Developers Team (https://launchpad.net/~dccl-dev)
//
// This file is part of the DCCL Library.
//
// The DCCL Library is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The DCCL Library is distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DCCL.  If not, see <http://www.gnu.org/licenses/>.


#ifndef DCCLLOGGER20121009H
#define DCCLLOGGER20121009H

#include <iostream>
#include <string>
#include <deque>
#include <iomanip>
#include <boost/signals2.hpp>
#include <cstdio>

namespace dccl {
 namespace logger {
  enum Verbosity {
      WARN = 0x01, INFO = 0x02, DEBUG1 = 0x04, DEBUG2 = 0x08, DEBUG3 = 0x10,
      ALL = WARN | INFO | DEBUG1 | DEBUG2 | DEBUG3
  };
 }

 class LogBuffer : public std::streambuf {
 private:
     logger::Verbosity verbosity_;
     std::deque<std::string> buffer_;

 public:
     LogBuffer() : verbosity_(logger::INFO), buffer_(1) { }
     ~LogBuffer() { }

     /// virtual inherited from std::streambuf.
     /// Called when std::endl or std::flush is inserted into the stream
     int sync();

     /// virtual inherited from std::streambuf. Called when something is inserted into the stream
     /// Called when std::endl or std::flush is inserted into the stream
     int overflow(int c = EOF);

     /// connect a signal to a slot (function pointer or similar)
     template <typename Slot>
     void connect(int verbosity_mask, Slot slot) {
         enabled_verbosities_ |= verbosity_mask;
         if(verbosity_mask & logger::WARN) warn_signal.connect(slot);
         if(verbosity_mask & logger::INFO) info_signal.connect(slot);
         if(verbosity_mask & logger::DEBUG1) debug1_signal.connect(slot);
         if(verbosity_mask & logger::DEBUG2) debug2_signal.connect(slot);
         if(verbosity_mask & logger::DEBUG3) debug3_signal.connect(slot);
     }
 
     /// sets the verbosity level until the next sync()
     void set_verbosity(logger::Verbosity verbosity) {
         verbosity_ = verbosity;
     }
  
     bool contains(logger::Verbosity verbosity) {
         return verbosity & enabled_verbosities_;
     }

 private:
     void display(const std::string& s) {
         if(verbosity_ & logger::WARN) warn_signal(s, logger::WARN);
         if(verbosity_ & logger::INFO) info_signal(s, logger::INFO);
         if(verbosity_ & logger::DEBUG1) debug1_signal(s, logger::DEBUG1);
         if(verbosity_ & logger::DEBUG2) debug2_signal(s, logger::DEBUG2);
         if(verbosity_ & logger::DEBUG3) debug3_signal(s, logger::DEBUG3);
     }

     int enabled_verbosities_; // mask of verbosity settings enabled

     boost::signals2::signal<void (const std::string& msg, logger::Verbosity vrb)> warn_signal;
     boost::signals2::signal<void (const std::string& msg, logger::Verbosity vrb)> info_signal;
     boost::signals2::signal<void (const std::string& msg, logger::Verbosity vrb)> debug1_signal;
     boost::signals2::signal<void (const std::string& msg, logger::Verbosity vrb)> debug2_signal;
     boost::signals2::signal<void (const std::string& msg, logger::Verbosity vrb)> debug3_signal;
 };

 class Logger : public std::ostream {
 private:
     LogBuffer buf_;
 public:
     Logger() : std::ostream(&buf_) { }
     virtual ~Logger() { }

     bool is(logger::Verbosity verbosity) {
         if (!buf_.contains(verbosity)) {
             return false;
         } else {
             buf_.set_verbosity(verbosity);
             return true;
         }
     }

     /// connect a signal to a slot (function pointer or similar)
     template<typename Slot>
     void connect(int verbosity_mask, Slot slot) {
         buf_.connect(verbosity_mask, slot);
     }

     /// connect a signal to a member function
     template<typename Obj, typename A1, typename A2> 
     void connect(int verbosity_mask, Obj* obj, void(Obj::*mem_func)(A1, A2)) {
         connect(verbosity_mask, boost::bind(mem_func, obj, _1, _2));
     }     
 };
 
 extern Logger dlog;
}

#endif // DCCLLOGGER20121009H
