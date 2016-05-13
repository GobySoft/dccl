#include <ctime>

#include "dccl/logger.h"

dccl::Logger dccl::dlog;

int dccl::internal::LogBuffer::sync() {
    // all but last one
    while(buffer_.size() > 1) {
        display(buffer_.front());
        buffer_.pop_front();
    }
    verbosity_ = logger::INFO;
    group_ = logger::GENERAL;
    
    return 0;
}

int dccl::internal::LogBuffer::overflow(int c) {
    if (c == EOF) { return c; }
    else if(c == '\n') { buffer_.push_back(std::string()); }
    else { buffer_.back().push_back(c); }
    return c;
}

void dccl::to_ostream(const std::string& msg, dccl::logger::Verbosity vrb,
                      dccl::logger::Group grp, std::ostream* os,
                      bool add_timestamp)
{
    std::string grp_str;
    switch(grp)
    {
        default:
        case logger::GENERAL: break;
        case logger::ENCODE: grp_str = "{encode}: "; break;
        case logger::DECODE: grp_str = "{decode}: "; break;
        case logger::SIZE: grp_str = "{size}: "; break;
    }
    
    std::time_t now = std::time(0);
    std::tm* t = std::gmtime(&now);

    if(add_timestamp)
    {
        *os << "[ " << (t->tm_year+1900) << "-"
            << std::setw(2) << std::setfill('0') << (t->tm_mon+1) << "-"
            << std::setw(2) << t->tm_mday
            << " "
            << std::setw(2) << t->tm_hour << ":"
            << std::setw(2) << t->tm_min << ":"
            << std::setw(2) << t->tm_sec << " ]: "
            << std::setfill(' ');
    }
    
    *os << grp_str << msg << std::endl;
    
}
