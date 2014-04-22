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



#ifndef DCCLBINARY20100713H
#define DCCLBINARY20100713H

#include <iomanip>
#include <cmath>
#include <sstream>

#include "dccl/common.h"

namespace dccl
{
    /// \name Binary encoding
    //@{

    /// \brief Decodes a (little-endian) hexadecimal string to a byte string. Index 0 and 1 (first byte) of `in` are written to index 0 (first byte) of `out`
    ///
    /// \param in hexadecimal string (e.g. "544f4d" or "544F4D")
    /// \param out pointer to string to store result (e.g. "TOM").
    inline void hex_decode(const std::string& in, std::string* out)
    {
        static const short char0_9_to_number = 48;
        static const short charA_F_to_number = 55; 
        static const short chara_f_to_number = 87; 
   
        int in_size = in.size();
        int out_size = in_size >> 1;
        if(in_size & 1)
            ++out_size;

        out->assign(out_size, '\0');
        for(int i = (in_size & 1) ? -1 : 0, n = in_size;
            i < n;
            i += 2)
        {
            int out_i = (in_size & 1) ? (i+1) / 2 : i/2;
	
            if(i >= 0)
            {
                if(in[i] >= '0' && in[i] <= '9')
                    (*out)[out_i] |= ((in[i]-char0_9_to_number) & 0x0f) << 4;
                else if(in[i] >= 'A' && in[i] <= 'F')
                    (*out)[out_i] |= ((in[i]-charA_F_to_number) & 0x0f) << 4;
                else if(in[i] >= 'a' && in[i] <= 'f')
                    (*out)[out_i] |= ((in[i]-chara_f_to_number) & 0x0f) << 4;
            }
	
            if(in[i+1] >= '0' && in[i+1] <= '9')
                (*out)[out_i] |= (in[i+1]-char0_9_to_number) & 0x0f;
            else if(in[i+1] >= 'A' && in[i+1] <= 'F')
                (*out)[out_i] |= (in[i+1]-charA_F_to_number) & 0x0f;
            else if(in[i+1] >= 'a' && in[i+1] <= 'f')
                (*out)[out_i] |= (in[i+1]-chara_f_to_number) & 0x0f;
        }	 
    }

    inline std::string hex_decode(const std::string& in)
    {
        std::string out;
        hex_decode(in, &out);
        return out;
    }

    /// \brief Encodes a (little-endian) hexadecimal string from a byte string. Index 0 of `in` is written to index 0 and 1 (first byte) of `out`
    ///
    /// \param in byte string to encode (e.g. "TOM")
    /// \param out pointer to string to store result (e.g. "544f4d")
    /// \param upper_case set true to use upper case for the alphabet characters (i.e. A,B,C,D,E,F), otherwise lowercase is used (a,b,c,d,e,f).
    inline void hex_encode(const std::string& in, std::string* out, bool upper_case = false)
    {
        static const short char0_9_to_number = 48;
        static const short charA_F_to_number = 55; 
        static const short chara_f_to_number = 87; 

        int in_size = in.size();
        int out_size = in_size << 1;
    
        out->resize(out_size);
        for(int i = 0, n = in_size;
            i < n;
            ++i)
        {
            short msn = (in[i] >> 4) & 0x0f;
            short lsn = in[i] & 0x0f;

            if(msn >= 0 && msn <= 9)
                (*out)[2*i] = msn + char0_9_to_number;
            else if(msn >= 10 && msn <= 15)
                (*out)[2*i] = msn + (upper_case ? charA_F_to_number : chara_f_to_number);
	
            if(lsn >= 0 && lsn <= 9)
                (*out)[2*i+1] = lsn + char0_9_to_number;
            else if(lsn >= 10 && lsn <= 15)
                (*out)[2*i+1] = lsn + (upper_case ? charA_F_to_number : chara_f_to_number);

        }
    }

    inline std::string hex_encode(const std::string& in)
    {
        std::string out;
        hex_encode(in, &out);
        return out;
    }
        
    /// \return ceil(log2(v))
    inline unsigned ceil_log2(dccl::uint64 v)
    {
        // r will be one greater (ceil) if v is not a power of 2
        unsigned r = ((v & (v - 1)) == 0) ? 0 : 1;
        while (v >>= 1)
            r++;
        return r;
    }
        
    inline unsigned long ceil_log2(double d)
    { return ceil_log2(static_cast<dccl::uint64>(std::ceil(d))); }

    inline unsigned long ceil_log2(int i) 
    { return ceil_log2(static_cast<dccl::uint64>(i)); } 

    inline unsigned long ceil_log2(long i) 
    { return ceil_log2(static_cast<dccl::uint64>(i)); }
    
    inline unsigned long ceil_log2(unsigned i) 
    { return ceil_log2(static_cast<dccl::uint64>(i)); }     
    
    inline double log2(double d)
    {
        static double log_2 = log(2);
        return log(d)/log_2;
    }
        


    //@}
}

#endif
