#include "dccl/logger.h"

namespace dccl {
 dccl::Logger dlog;

 int LogBuffer::sync() {
     // all but last one
     while(buffer_.size() > 1) {
         display(buffer_.front());
         buffer_.pop_front();
     }
     verbosity_ = logger::INFO;

     return 0;
 }

 int LogBuffer::overflow(int c) {
     if (c == EOF) { return c; }
     else if(c == '\n') { buffer_.push_back(std::string()); }
     else { buffer_.back().push_back(c); }
     return c;
 }
}
