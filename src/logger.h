// Copyright 2009-2014 Toby Schneider (https://launchpad.net/~tes)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//                     DCCL Developers Team (https://launchpad.net/~dccl-dev)
//
// This file is part of the Dynamic Compact Control Language Library
// ("DCCL").
//
// DCCL is free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// DCCL is distributed in the hope that they will be useful,
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
            WARN = 1<<1,
            INFO = 1<<2,
            DEBUG1 = 1<<3,
            DEBUG2 = 1<<4,
            DEBUG3 = 1<<5,
            ALL = DEBUG3 | (DEBUG3-1),
            WARN_PLUS = WARN | (WARN - 1),
            INFO_PLUS = INFO | (INFO - 1),
            DEBUG1_PLUS = DEBUG1 | (DEBUG1 - 1),
            DEBUG2_PLUS = DEBUG2 | (DEBUG2 - 1),
            DEBUG3_PLUS = DEBUG3 | (DEBUG3 - 1)
        };
        enum Group 
        { GENERAL, ENCODE, DECODE, SIZE };
  
    }


    void to_ostream(const std::string& msg, dccl::logger::Verbosity vrb,
                    dccl::logger::Group grp, std::ostream* os, bool add_timestamp);
    
    
    class LogBuffer : public std::streambuf
    {
      public:
      LogBuffer() : verbosity_(logger::INFO), group_(logger::GENERAL), buffer_(1) { }
        ~LogBuffer() { }

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
     
        void disconnect(int verbosity_mask) {
            enabled_verbosities_ &= ~verbosity_mask;
            if(verbosity_mask & logger::WARN) warn_signal.disconnect_all_slots();
            if(verbosity_mask & logger::INFO) info_signal.disconnect_all_slots();
            if(verbosity_mask & logger::DEBUG1) debug1_signal.disconnect_all_slots();
            if(verbosity_mask & logger::DEBUG2) debug2_signal.disconnect_all_slots();
            if(verbosity_mask & logger::DEBUG3) debug3_signal.disconnect_all_slots();
        }
 
        /// sets the verbosity level until the next sync()
        void set_verbosity(logger::Verbosity verbosity)
        { verbosity_ = verbosity; }

        void set_group(logger::Group group)
        { group_ = group; }
        
  
        bool contains(logger::Verbosity verbosity)
        { return verbosity & enabled_verbosities_; }
 
     
      private:
        /// virtual inherited from std::streambuf.
        /// Called when std::endl or std::flush is inserted into the stream
        int sync();

        /// virtual inherited from std::streambuf. Called when something is inserted into the stream
        /// Called when std::endl or std::flush is inserted into the stream
        int overflow(int c = EOF);
     

        void display(const std::string& s) {
            if(verbosity_ & logger::WARN) warn_signal(s, logger::WARN, group_);
            if(verbosity_ & logger::INFO) info_signal(s, logger::INFO, group_);
            if(verbosity_ & logger::DEBUG1) debug1_signal(s, logger::DEBUG1, group_);
            if(verbosity_ & logger::DEBUG2) debug2_signal(s, logger::DEBUG2, group_);
            if(verbosity_ & logger::DEBUG3) debug3_signal(s, logger::DEBUG3, group_);
        }

      private:
        logger::Verbosity verbosity_;
        logger::Group group_;
        std::deque<std::string> buffer_;
        int enabled_verbosities_; // mask of verbosity settings enabled

        typedef boost::signals2::signal<void (const std::string& msg, logger::Verbosity vrb, logger::Group grp)>
            LogSignal;
                                              
        LogSignal warn_signal, info_signal, debug1_signal, debug2_signal, debug3_signal;
        
    };

    class Logger : public std::ostream {
      public:
      Logger() : std::ostream(&buf_) { }
        virtual ~Logger() { }

        bool is(logger::Verbosity verbosity, logger::Group group = logger::GENERAL) {
            if (!buf_.contains(verbosity)) {
                return false;
            } else {
                buf_.set_verbosity(verbosity);
                buf_.set_group(group);
                return true;
            }
        }     
     
        /// connect a signal to a slot (function pointer or similar)
        template<typename Slot>
            void connect(int verbosity_mask, Slot slot) {
            buf_.connect(verbosity_mask, slot);
        }

        /// connect a signal to a member function
        template<typename Obj> 
            void connect(
                int verbosity_mask, Obj* obj,
                void(Obj::*mem_func)(const std::string& msg, logger::Verbosity vrb, logger::Group grp))
        { connect(verbosity_mask, boost::bind(mem_func, obj, _1, _2, _3)); }

        /// connect a verbosity to a ostream
        void connect(int verbosity_mask, std::ostream* os,
                     bool add_timestamp = true)
        {
            buf_.connect(verbosity_mask, boost::bind(to_ostream, _1, _2, _3, os, add_timestamp));
        }
     
        void disconnect(int verbosity_mask)
        { buf_.disconnect(verbosity_mask); }


     
      private:
        LogBuffer buf_;
     
    };
 
    extern Logger dlog;
}

#endif // DCCLLOGGER20121009H
