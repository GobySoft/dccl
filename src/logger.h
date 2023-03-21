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

#include <iostream>
#include <string>
#include <deque>
#include <iomanip>
#include <boost/signals2.hpp>
#include <cstdio>

namespace dccl {
    namespace logger {
        /// Verbosity levels used by the Logger
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
    
    namespace internal
    {
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
    }

    /// The DCCL Logger class. Do not instantiate this class directly. Rather, use the dccl::dlog object. 
    class Logger : public std::ostream {
      public:
      Logger() : std::ostream(&buf_) { }
        virtual ~Logger() { }

        /// \brief Indicates the verbosity of the Logger until the next std::flush or std::endl. The boolean return is used to take advantage of short-circuit evaluation of && to avoid spending CPU time generating log files that if they are not used.
        ///
        /// The typical usage is
        /// \code
        /// dlog.is(INFO) && dlog << "Something of interest." << std::endl;
        /// dlog.is(WARN, ENCODE) && dlog << "Something bad happened while encoding." << std::endl;
        /// \endcode
        /// \param verbosity The verbosity level to tag the following message with. These levels are used to direct the output of dlog to different logs or omit them completely.
        /// \param group The group that this message belongs to.
        bool is(logger::Verbosity verbosity, logger::Group group = logger::GENERAL) {
            if (!buf_.contains(verbosity)) {
                return false;
            } else {
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
        template<typename Slot>
            void connect(int verbosity_mask, Slot slot) {
            buf_.connect(verbosity_mask, slot);
        }

        /// \brief Connect the output of one or more given verbosities to a member function
        ///
        /// \param verbosity_mask A bitmask representing the verbosity or verbosities to send to this slot. For example, you can use connect(WARN | INFO, this, &MyClass::slot) to send both WARN and INFO messages to MyClass::slot.
        /// \param obj Pointer to the instantiation of the class of which this member function should be called.
        /// \param mem_func Member function of type
        /// (void*) (const std::string& msg, logger::Verbosity vrb, logger::Group grp)
        template<typename Obj> 
            void connect(
                int verbosity_mask, Obj* obj,
                void(Obj::*mem_func)(const std::string& msg, logger::Verbosity vrb, logger::Group grp))
        {
#if BOOST_VERSION >= 106000
            using boost::placeholders::_1;
            using boost::placeholders::_2;
            using boost::placeholders::_3;            
#endif
            connect(verbosity_mask, boost::bind(mem_func, obj, _1, _2, _3));
        }

        /// \brief Connect the output of one or more given verbosities to a std::ostream
        ///
        /// \param verbosity_mask A bitmask representing the verbosity or verbosities to send to this slot. For example, you can use connect(WARN | INFO, &std::cout) to send both WARN and INFO messages to std::cout.
        /// \param os Pointer to a std::ostream
        /// \param add_timestamp If true, prepend the current timestamp of the message to each log message.
        void connect(int verbosity_mask, std::ostream* os,
                     bool add_timestamp = true)
        {
#if BOOST_VERSION >= 106000
            using boost::placeholders::_1;
            using boost::placeholders::_2;
            using boost::placeholders::_3;            
#endif
            buf_.connect(verbosity_mask, boost::bind(to_ostream, _1, _2, _3, os, add_timestamp));
        }

        /// \brief Disconnect all slots for one or more given verbosities
        void disconnect(int verbosity_mask)
        { buf_.disconnect(verbosity_mask); }
        
      private:
        internal::LogBuffer buf_;
     
    };
 
    extern Logger dlog;
}

#endif // DCCLLOGGER20121009H
