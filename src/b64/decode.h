// Copyright 2009-2016 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
//                     GobySoft, LLC (for 2013-)
//                     Massachusetts Institute of Technology (for 2007-2014)
//                     Community contributors (see AUTHORS file)
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
// :mode=c++:
/*
decode.h - c++ wrapper for a base64 decoding algorithm

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/
#ifndef BASE64_DECODE_H
#define BASE64_DECODE_H

#include <iostream>

namespace dccl
{
    namespace base64
    {
	extern "C"
	{
#include "cdecode.h"
	}

	struct decoder
	{
            base64_decodestate _state;
            int _buffersize;

        decoder(int buffersize_in = BUFFERSIZE)
        : _buffersize(buffersize_in)
		{}

            int decode(char value_in)
		{
                    return base64_decode_value(value_in);
		}

            int decode(const char* code_in, const int length_in, char* plaintext_out)
		{
                    return base64_decode_block(code_in, length_in, plaintext_out, &_state);
		}

            void decode(std::istream& istream_in, std::ostream& ostream_in)
		{
                    base64_init_decodestate(&_state);
                    //
                    const int N = _buffersize;
                    char* code = new char[N];
                    char* plaintext = new char[N];
                    int codelength;
                    int plainlength;

                    do
                    {
                        istream_in.read((char*)code, N);
                        codelength = istream_in.gcount();
                        plainlength = decode(code, codelength, plaintext);
                        ostream_in.write((const char*)plaintext, plainlength);
                    }
                    while (istream_in.good() && codelength > 0);
                    //
                    base64_init_decodestate(&_state);

                    delete [] code;
                    delete [] plaintext;
		}
	};

    } // namespace base64
}



#endif // BASE64_DECODE_H

