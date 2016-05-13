#ifndef Exception20100812H
#define Exception20100812H

#include <stdexcept>

namespace dccl
{
    /// \brief Exception class for DCCL
    class Exception : public std::runtime_error {
      public:
      Exception(const std::string& s)
          : std::runtime_error(s)
        { }

    };

    /// \brief Exception used to signal null (non-existent) value within field codecs during decode.
    class NullValueException : public Exception
    {
      public:
      NullValueException()
          : Exception("NULL Value")
        { }    
    };
        
}


#endif

