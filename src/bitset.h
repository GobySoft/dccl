// Copyright 2012-2017:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   Nathan Knotts <nknotts@gmail.com>
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
#ifndef DCCLBITSET20120424H
#define DCCLBITSET20120424H

#include <algorithm>
#include <cstring>
#include <deque>
#include <limits>
#include <string>

#include "exception.h"

namespace dccl
{
/// \brief A variable size container of bits (subclassed from std::deque<bool>) with an optional hierarchy. Similar to set::bitset but can be resized at runtime and has the ability to have parent Bitsets that can give bits to their children.
///
/// This is the class used within DCCL hold the encoded message as it is created. The front() of the deque represents the least significant bit (lsb) and the back() is the most significant bit (msb). DCCL messages are encoded and decoded starting with the  lsb and ending at the msb. The hierarchy is used to represent parent bit pools from which the child can pull more bits from to decode. The top level Bitset represents the entire encoded message, whereas the children are the message fields.
class Bitset : public std::deque<bool>
{
  public:
    /// \brief Construct an empty Bitset.
    ///
    /// \param parent Pointer to a bitset that should be consider this Bitset's parent for calls to get_more_bits()
    explicit Bitset(Bitset* parent = nullptr) : parent_(parent) {}

    /// \brief Construct a Bitset of a certain initial size and value.
    ///
    /// \param num_bits Initial size of this Bitset
    /// \param value Initial value of the bits in this Bitset
    /// \param parent Pointer to a bitset that should be consider this Bitset's parent for calls to get_more_bits()
    explicit Bitset(size_type num_bits, unsigned long value = 0, Bitset* parent = nullptr)
        : std::deque<bool>(num_bits, false), parent_(parent)
    {
        from(value, num_bits);
    }

    ~Bitset() = default;

    /// \brief Retrieve more bits from the parent Bitset
    ///
    /// Get (and remove) bits from the little end of the parent bitset and add them to the big end of our bitset,
    /// (the parent will request from their parent if required).
    /// \param num_bits Number of bits to get.
    /// \throw Exception The parent (and up the hierarchy, if applicable) do not have num_bits to give up.
    void get_more_bits(size_type num_bits);

    /// \brief Logical AND in place
    ///
    /// Apply the result of a logical AND of this Bitset and another to this Bitset.
    /// \param rhs The Bitset to perform the operation on with this Bitset.
    /// \return A reference to the resulting Bitset
    /// \throw Exception The size of the two Bitsets are not equal.
    Bitset& operator&=(const Bitset& rhs)
    {
        if (rhs.size() != size())
            throw(dccl::Exception("Bitset operator&= requires this->size() == rhs.size()"));

        for (size_type i = 0; i != this->size(); ++i) (*this)[i] &= rhs[i];
        return *this;
    }

    /// \brief Logical OR in place
    ///
    /// Apply the result of a logical OR of this Bitset and another to this Bitset.
    /// \param rhs The Bitset to perform the operation on with this Bitset.
    /// \return  A reference to the resulting Bitset
    /// \throw Exception The size of the two Bitsets are not equal.
    Bitset& operator|=(const Bitset& rhs)
    {
        if (rhs.size() != size())
            throw(dccl::Exception("Bitset operator|= requires this->size() == rhs.size()"));

        for (size_type i = 0; i != this->size(); ++i) (*this)[i] |= rhs[i];
        return *this;
    }

    /// \brief Logical XOR in place
    ///
    /// Apply the result of a logical XOR of this Bitset and another to this Bitset.
    /// \param rhs The Bitset to perform the operation on with this Bitset.
    /// \return A reference to the resulting Bitset
    /// \throw Exception The size of the two Bitsets are not equal.
    Bitset& operator^=(const Bitset& rhs)
    {
        if (rhs.size() != size())
            throw(dccl::Exception("Bitset operator^= requires this->size() == rhs.size()"));

        for (size_type i = 0; i != this->size(); ++i) (*this)[i] ^= rhs[i];
        return *this;
    }

    //            Bitset& operator-=(const Bitset& rhs);

    /// \brief Left shift in place
    ///
    /// Shifts the Bitset to the left and inserts false (0) to the new least significant bits and discards any bits
    /// that come off the most significant end. This operation does not change the size of the Bitset.
    /// \param n The number of bits to shift. This is equivalent to multiplying the Bitset by 2^n if the Bitset can hold the result
    /// \return  A reference to the resulting Bitset
    Bitset& operator<<=(size_type n)
    {
        for (size_type i = 0; i < n; ++i)
        {
            push_front(false);
            pop_back();
        }
        return *this;
    }

    /// \brief Right shift in place
    ///
    /// Shifts the Bitset to the right and inserts false (0) to the new most significant bits and discards any bits
    /// that come off the least significant end. This operation does not change the size of the Bitset.
    /// \param n The number of bits to shift. This is equivalent to dividing the Bitset by 2^n
    /// \return  A reference to the resulting Bitset
    Bitset& operator>>=(size_type n)
    {
        for (size_type i = 0; i < n; ++i)
        {
            push_back(false);
            pop_front();
        }
        return *this;
    }

    /// \brief Left shift
    ///
    /// Same as operator<<=() but does not modify this Bitset.
    Bitset operator<<(size_type n) const
    {
        Bitset copy(*this);
        copy <<= n;
        return copy;
    }

    /// \brief Right shift
    ///
    /// Same as operator>>=() but does not modify this Bitset.
    Bitset operator>>(size_type n) const
    {
        Bitset copy(*this);
        copy >>= n;
        return copy;
    }

    /// \brief Set a bit to a given value
    ///
    /// \param n bit to set
    /// \param val value to set the bit to
    /// \return A reference to the resulting Bitset
    Bitset& set(size_type n, bool val = true)
    {
        (*this)[n] = val;
        return *this;
    }

    /// \brief Set all bits true
    ///
    /// \return A reference to the resulting Bitset
    Bitset& set()
    {
        for (bool& it : *this) it = true;
        return *this;
    }

    /// \brief Reset a bit (i.e. set it to false)
    ///
    /// \param n bit to reset
    /// \return A reference to the resulting Bitset
    Bitset& reset(size_type n) { return set(n, false); }

    /// \brief Set all bits false
    ///
    /// \return A reference to the resulting Bitset
    Bitset& reset()
    {
        for (bool& it : *this) it = false;
        return *this;
    }

    /// \brief Flip (toggle) a bit
    ///
    /// \param n bit to flip
    /// \return A reference to the resulting Bitset
    Bitset& flip(size_type n) { return set(n, !(*this)[n]); }

    /// \brief Flip (toggle) all bits
    ///
    /// \return A reference to the resulting Bitset
    Bitset& flip()
    {
        for (size_type i = 0, n = size(); i < n; ++i) flip(i);
        return *this;
    }

    /// \brief Test a bit (return its value)
    ///
    /// \param n bit to test
    /// \return value of the bit
    bool test(size_type n) const { return (*this)[n]; }

    /* bool any() const; */
    /* bool none() const; */
    /* Bitset operator~() const; */
    /* size_type count() const; */

    /// \brief Sets value of the Bitset to the contents of an integer
    ///
    /// \param value Value to give the Bitset.
    /// \param num_bits The resulting size of the Bitset.
    template <typename IntType>
    void from(IntType value, size_type num_bits = std::numeric_limits<IntType>::digits)
    {
        this->resize(num_bits);
        for (int i = 0, n = std::min<size_type>(std::numeric_limits<IntType>::digits, size());
             i < n; ++i)
        {
            if (value & (static_cast<IntType>(1) << i))
                (*this)[i] = true;
        }
    }

    /// \brief Sets value of the Bitset to the contents of an unsigned long integer. Equivalent to from<unsigned long>()
    void from_ulong(unsigned long value,
                    size_type num_bits = std::numeric_limits<unsigned long>::digits)
    {
        from<unsigned long>(value, num_bits);
    }

    /// \brief Returns the value of the Bitset as a integer
    ///
    /// \return Value of the bitset
    /// \throw Exception The integer type cannot represent the current Bitset.
    template <typename IntType> IntType to() const
    {
        if (size() > static_cast<size_type>(std::numeric_limits<IntType>::digits))
            throw(Exception("Type IntType cannot represent current bitset (this->size() > "
                            "std::numeric_limits<IntType>::digits)"));

        IntType out = 0;
        for (int i = 0, n = size(); i < n; ++i)
        {
            if ((*this)[i])
                out |= (static_cast<IntType>(1) << i);
        }

        return out;
    }

    /// \brief Returns the value of the Bitset as an unsigned long integer. Equivalent to to<unsigned long>().
    unsigned long to_ulong() const { return to<unsigned long>(); }

    /// \brief Returns the value of the Bitset as a printable string, where each bit is represented by '1' or '0'. The msb is written into the zero index of the string, so it is printed msb to lsb (as is standard for writing numbers).
    std::string to_string() const
    {
        std::string s(size(), 0);
        int i = 0;
        for (auto it = rbegin(), n = rend(); it != n; ++it)
        {
            s[i] = (*it) ? '1' : '0';
            ++i;
        }
        return s;
    }

    // little-endian
    // LSB = string[0]
    // MSB = string[N]

    /// \brief Returns the value of the Bitset to a byte string, where each character represents 8 bits of the Bitset. The string is used as a byte container, and is not intended to be printed.
    ///
    /// \return A string containing the value of the Bitset, with the least signficant byte in string[0] and the most significant byte in string[size()-1]
    std::string to_byte_string()
    {
        // number of bytes needed is ceil(size() / 8)
        std::string s(this->size() / 8 + (this->size() % 8 ? 1 : 0), 0);

        for (size_type i = 0, n = this->size(); i < n; ++i)
            s[i / 8] |= static_cast<char>((*this)[i] << (i % 8));

        return s;
    }

    /// \brief Generate a byte string representation of the Bitset, where each character represents 8 bits of the Bitset. The string is used as a byte container, and is not intended to be printed.
    /// \param buf An output string containing the value of the Bitset, with the least signficant byte in string[0] and the most significant byte in string[size()-1]
    /// \param max_len Maximum length of buf
    /// \return number of bytes written to buf
    /// \throw std::length_error if max_len < encoded length.
    size_t to_byte_string(char* buf, size_t max_len)
    {
        // number of bytes needed is ceil(size() / 8)
        size_t len = this->size() / 8 + (this->size() % 8 ? 1 : 0);

        if (max_len < len)
        {
            throw std::length_error("max_len must be >= len");
        }

        // initialize buffer to all zeroes
        std::fill_n(buf, len, 0);

        for (size_type i = 0, n = this->size(); i < n; ++i)
            buf[i / 8] |= static_cast<char>((*this)[i] << (i % 8));

        return len;
    }

    /// \brief Sets the value of the Bitset to the contents of a byte string, where each character represents 8 bits of the Bitset.
    ///
    /// \param s A string container the values where the least signficant byte in string[0] and the most significant byte in string[size()-1]
    void from_byte_string(const std::string& s) { from_byte_stream(s.begin(), s.end()); }

    /// \brief Sets the value of the Bitset to the contents of a byte string, where each character represents 8 bits of the Bitset.
    /// A string container the values where the least signficant byte in string[0] and the most significant byte in string[size()-1]
    /// \param begin Iterator pointing to the begining of the input buffer
    /// \param end Iterator pointing to the end of the input bufer
    template <typename CharIterator> void from_byte_stream(CharIterator begin, CharIterator end)
    {
        this->resize(std::distance(begin, end) * 8);
        int i = 0;
        for (CharIterator it = begin; it != end; ++it)
        {
            for (size_type j = 0; j < 8; ++j) (*this)[i * 8 + j] = (*it) & (1 << j);
            ++i;
        }
    }

    /// \brief Adds the bitset to the little end
    Bitset& prepend(const Bitset& bits)
    {
        for (auto it = bits.rbegin(), n = bits.rend(); it != n; ++it) push_front(*it);

        return *this;
    }

    /// \brief Adds the bitset to the big end
    Bitset& append(const Bitset& bits)
    {
        for (bool bit : bits) push_back(bit);

        return *this;
    }

  private:
    Bitset relinquish_bits(size_type num_bits, bool final_child);

  private:
    Bitset* parent_;
};

inline bool operator==(const Bitset& a, const Bitset& b)
{
    return (a.size() == b.size()) && std::equal(a.begin(), a.end(), b.begin());
}

inline bool operator<(const Bitset& a, const Bitset& b)
{
    for (int i = (std::max(a.size(), b.size()) - 1); i >= 0; --i)
    {
        bool a_bit = (i < static_cast<int>(a.size())) ? a[i] : 0;
        bool b_bit = (i < static_cast<int>(b.size())) ? b[i] : 0;

        if (a_bit > b_bit)
            return false;
        else if (a_bit < b_bit)
            return true;
    }
    return false;
}

inline Bitset operator&(const Bitset& b1, const Bitset& b2)
{
    Bitset out(b1);
    out &= b2;
    return out;
}

inline Bitset operator|(const Bitset& b1, const Bitset& b2)
{
    Bitset out(b1);
    out |= b2;
    return out;
}

inline Bitset operator^(const Bitset& b1, const Bitset& b2)
{
    Bitset out(b1);
    out ^= b2;
    return out;
}

inline std::ostream& operator<<(std::ostream& os, const Bitset& b) { return (os << b.to_string()); }

} // namespace dccl

inline void dccl::Bitset::get_more_bits(size_type num_bits) { relinquish_bits(num_bits, true); }

#endif
