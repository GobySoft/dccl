// Copyright 2012-2023:
//   GobySoft, LLC (2013-)
//   Massachusetts Institute of Technology (2007-2014)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
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
// This code is adapted from the reference code provided by Witten et al. "Arithmetic Coding for Data Compression," Communications of the ACM, June 1987, Vol 30, Number 6

#ifndef DCCLFIELDCODECARITHMETIC20120726H
#define DCCLFIELDCODECARITHMETIC20120726H

#include <algorithm>
#include <limits>


#include "dccl/field_codec_typed.h"

#include "dccl/arithmetic/protobuf/arithmetic.pb.h"
#include "dccl/arithmetic/protobuf/arithmetic_extensions.pb.h"

#include "dccl/logger.h"

#include "dccl/binary.h"
#include "dccl/thread_safety.h"

extern "C"
{
    void dccl3_load(dccl::Codec* dccl);
    void dccl3_unload(dccl::Codec* dccl);
    void dccl_arithmetic_load(dccl::Codec* dccl);
    void dccl_arithmetic_unload(dccl::Codec* dccl);
}

namespace dccl
{
/// DCCL Arithmetic Encoder Library namespace
namespace arith
{
class ModelManager;

ModelManager& model_manager(FieldCodecManagerLocal& manager);

class Model
{
  public:
    typedef uint32 freq_type;
    typedef int symbol_type; // google protobuf RepeatedField size type
    typedef double value_type;

    static constexpr symbol_type OUT_OF_RANGE_SYMBOL = -1;
    static constexpr symbol_type EOF_SYMBOL = -2;
    static constexpr symbol_type MIN_SYMBOL = EOF_SYMBOL;

    static constexpr int CODE_VALUE_BITS = 32;
    static constexpr int FREQUENCY_BITS = CODE_VALUE_BITS - 2;

    static constexpr freq_type MAX_FREQUENCY = (1 << FREQUENCY_BITS) - 1;

#if DCCL_THREAD_SUPPORT
    static std::recursive_mutex last_bits_map_mutex;
#define LOCK_LAST_BITS_MAP_MUTEX \
    std::lock_guard<std::recursive_mutex> l(dccl::arith::Model::last_bits_map_mutex);
#else
#define LOCK_LAST_BITS_MAP_MUTEX
#endif
    // maps message name -> map of field name -> last size (bits)
    static std::map<std::string, std::map<std::string, Bitset>> last_bits_map;

    Model(const protobuf::ArithmeticModel& user) : user_model_(user) {}

    enum ModelState
    {
        ENCODER,
        DECODER
    };

    symbol_type value_to_symbol(value_type value) const;
    value_type symbol_to_value(symbol_type symbol) const;
    symbol_type total_symbols() // EOF and OUT_OF_RANGE plus all user defined
    {
        return encoder_cumulative_freqs_.size();
    }

    const protobuf::ArithmeticModel& user_model() const { return user_model_; }

    symbol_type max_symbol() const { return user_model_.frequency_size() - 1; }

    freq_type total_freq(ModelState state) const
    {
        const auto& c_freqs =
            (state == ENCODER) ? encoder_cumulative_freqs_ : decoder_cumulative_freqs_;

        return c_freqs.at(max_symbol());
    }

    void update_model(symbol_type symbol, ModelState state);

    std::pair<freq_type, freq_type> symbol_to_cumulative_freq(symbol_type symbol,
                                                              ModelState state) const;
    std::pair<symbol_type, symbol_type>
    cumulative_freq_to_symbol(std::pair<freq_type, freq_type> c_freq_pair, ModelState state) const;

    friend class ModelManager;

  private:
    protobuf::ArithmeticModel user_model_;
    std::map<symbol_type, freq_type> encoder_cumulative_freqs_;
    std::map<symbol_type, freq_type> decoder_cumulative_freqs_;
};

class ModelManager
{
  public:
    static void set_model(dccl::Codec& codec, const protobuf::ArithmeticModel& model);

    Model& find(const std::string& name)
    {
        std::map<std::string, Model>::iterator it = arithmetic_models_.find(name);
        if (it == arithmetic_models_.end())
            throw(Exception("Cannot find model called: " + name));
        else
            return it->second;
    }

  private:
    void _set_model(const protobuf::ArithmeticModel& model)
    {
        Model new_model(model);
        _create_and_validate_model(&new_model);
        if (arithmetic_models_.count(model.name()))
            arithmetic_models_.erase(model.name());
        arithmetic_models_.insert(std::make_pair(model.name(), new_model));
    }

    void _create_and_validate_model(Model* model)
    {
        if (!model->user_model_.IsInitialized())
        {
            throw(Exception("Invalid model: " + model->user_model_.DebugString() +
                            "Missing fields: " + model->user_model_.InitializationErrorString()));
        }

        Model::freq_type cumulative_freq = 0;
        for (Model::symbol_type symbol = Model::MIN_SYMBOL, n = model->user_model_.frequency_size();
             symbol < n; ++symbol)
        {
            Model::freq_type freq;
            if (symbol == Model::EOF_SYMBOL)
                freq = model->user_model_.eof_frequency();
            else if (symbol == Model::OUT_OF_RANGE_SYMBOL)
                freq = model->user_model_.out_of_range_frequency();
            else
                freq = model->user_model_.frequency(symbol);

            if (freq == 0 && symbol != Model::OUT_OF_RANGE_SYMBOL && symbol != Model::EOF_SYMBOL)
            {
                throw(Exception("Invalid model: " + model->user_model_.DebugString() +
                                "All frequencies must be nonzero."));
            }
            cumulative_freq += freq;
            model->encoder_cumulative_freqs_.insert(std::make_pair(symbol, cumulative_freq));
        }

        // must have separate models for adaptive encoding.
        model->decoder_cumulative_freqs_ = model->encoder_cumulative_freqs_;

        if (model->total_freq(Model::ENCODER) > Model::MAX_FREQUENCY)
        {
            throw(Exception("Invalid model: " + model->user_model_.DebugString() +
                            "Sum of all frequencies must be less than " +
                            std::to_string(Model::MAX_FREQUENCY) +
                            " in order to use 64 bit arithmetic"));
        }

        if (model->user_model_.value_bound_size() != model->user_model_.frequency_size() + 1)
        {
            throw(Exception("Invalid model: " + model->user_model_.DebugString() +
                            "`value_bound` size must be exactly 1 more than number of symbols (= "
                            "size of `frequency`)."));
        }

        // is `value_bound` repeated field strictly monotonically increasing?
        if (std::adjacent_find(
                model->user_model_.value_bound().begin(), model->user_model_.value_bound().end(),
                std::greater_equal<Model::value_type>()) != model->user_model_.value_bound().end())
        {
            throw(Exception("Invalid model: " + model->user_model_.DebugString() +
                            "`value_bound` must be monotonically increasing."));
        }
    }

  private:
    std::map<std::string, Model> arithmetic_models_;
};

template <typename FieldType = Model::value_type>
class ArithmeticFieldCodecBase : public RepeatedTypedFieldCodec<Model::value_type, FieldType>
{
  public:
    static constexpr uint64 TOP_VALUE =
        (static_cast<uint64>(1) << Model::CODE_VALUE_BITS) - 1; // 11111111...
    static constexpr uint64 HALF =
        (static_cast<uint64>(1) << (Model::CODE_VALUE_BITS - 1)); // 10000000...
    static constexpr uint64 FIRST_QTR = HALF >> 1;                // 01000000...
    static constexpr uint64 THIRD_QTR = HALF + FIRST_QTR;         // 11000000...

    Bitset encode_repeated(const std::vector<Model::value_type>& wire_value)
    {
        return encode_repeated(wire_value, true);
    }

    Bitset encode_repeated(const std::vector<Model::value_type>& wire_value, bool update_model)
    {
        using dccl::dlog;
        using namespace dccl::logger;
        Model& model = current_model();

        uint64 low = 0;          // lowest code value (0.0 in decimal version)
        uint64 high = TOP_VALUE; // highest code value (1.0 in decimal version)
        int bits_to_follow = 0;  // bits to follow with after expanding around half
        Bitset bits;

        for (unsigned value_index = 0, n = max_repeat(); value_index < n; ++value_index)
        {
            Model::symbol_type symbol = Model::EOF_SYMBOL;

            if (wire_value.size() > value_index)
            {
                Model::value_type value = wire_value[value_index];
                dlog.is(DEBUG3) && dlog << "(ArithmeticFieldCodec) value is : " << value
                                        << std::endl;

                symbol = model.value_to_symbol(value);
            }

            // if out-of-range is given no frequency, end encoding
            if (symbol == Model::OUT_OF_RANGE_SYMBOL &&
                model.user_model().out_of_range_frequency() == 0)
            {
                dlog.is(DEBUG2) && dlog << "(ArithmeticFieldCodec) out of range symbol, but no "
                                           "frequency given; ending encoding"
                                        << std::endl;

                symbol = Model::EOF_SYMBOL;
            }

            // if EOF_SYMBOL is given no frequency, use most probable symbol and give a warning
            if (symbol == Model::EOF_SYMBOL && model.user_model().eof_frequency() == 0)
            {
                dlog.is(DEBUG2) && dlog << "(ArithmeticFieldCodec) end of file, but no frequency "
                                           "given; filling with most probable symbol"
                                        << std::endl;
                symbol = *std::max_element(model.user_model().frequency().begin(),
                                           model.user_model().frequency().end());
            }

            dlog.is(DEBUG3) && dlog << "(ArithmeticFieldCodec) symbol is : " << symbol << std::endl;

            dlog.is(DEBUG3) && dlog << "(ArithmeticFieldCodec) current interval: ["
                                    << (double)low / TOP_VALUE << "," << (double)high / TOP_VALUE
                                    << ")" << std::endl;

            uint64 range = (high - low) + 1;

            std::pair<Model::freq_type, Model::freq_type> c_freq_range =
                model.symbol_to_cumulative_freq(symbol, Model::ENCODER);

            dlog.is(DEBUG3) && dlog << "(ArithmeticFieldCodec) input symbol (" << symbol
                                    << ") cumulative freq: [" << c_freq_range.first << ","
                                    << c_freq_range.second << ")" << std::endl;

            high = low + (range * c_freq_range.second) / model.total_freq(Model::ENCODER) - 1;
            low += (range * c_freq_range.first) / model.total_freq(Model::ENCODER);

            dlog.is(DEBUG3) && dlog << "(ArithmeticFieldCodec) input symbol (" << symbol
                                    << ") interval: [" << (double)low / TOP_VALUE << ","
                                    << (double)high / TOP_VALUE << ")" << std::endl;

            dlog.is(DEBUG3) &&
                dlog << "(ArithmeticFieldCodec) Q1: " << Bitset(Model::CODE_VALUE_BITS, FIRST_QTR)
                     << ", Q2: " << Bitset(Model::CODE_VALUE_BITS, HALF)
                     << ", Q3 : " << Bitset(Model::CODE_VALUE_BITS, THIRD_QTR)
                     << ", top: " << Bitset(Model::CODE_VALUE_BITS, TOP_VALUE) << std::endl;

            dlog.is(DEBUG3) && dlog << "(ArithmeticFieldCodec) low:  "
                                    << Bitset(Model::CODE_VALUE_BITS, low).to_string() << std::endl;
            dlog.is(DEBUG3) && dlog << "(ArithmeticFieldCodec) high: "
                                    << Bitset(Model::CODE_VALUE_BITS, high).to_string()
                                    << std::endl;

            if (update_model)
                model.update_model(symbol, Model::ENCODER);

            for (;;)
            {
                if (high < HALF)
                {
                    bit_plus_follow(&bits, &bits_to_follow, 0);
                    dlog.is(DEBUG3) &&
                        dlog << "(ArithmeticFieldCodec): completely in [0, 0.5): EXPAND"
                             << std::endl;
                }
                else if (low >= HALF)
                {
                    bit_plus_follow(&bits, &bits_to_follow, 1);
                    low -= HALF;
                    high -= HALF;
                    dlog.is(DEBUG3) &&
                        dlog << "(ArithmeticFieldCodec): completely in [0.5, 1): EXPAND"
                             << std::endl;
                }
                else if (low >= FIRST_QTR && high < THIRD_QTR)
                {
                    dlog.is(DEBUG3) &&
                        dlog << "(ArithmeticFieldCodec): straddle middle [0.25, 0.75): EXPAND"
                             << std::endl;

                    bits_to_follow += 1;
                    low -= FIRST_QTR;
                    high -= FIRST_QTR;
                }
                else
                    break;

                low <<= 1;
                high <<= 1;
                high += 1;

                dlog.is(DEBUG3) && dlog << "(ArithmeticFieldCodec) low:  "
                                        << Bitset(Model::CODE_VALUE_BITS, low).to_string()
                                        << std::endl;
                dlog.is(DEBUG3) && dlog << "(ArithmeticFieldCodec) high: "
                                        << Bitset(Model::CODE_VALUE_BITS, high).to_string()
                                        << std::endl;

                dlog.is(DEBUG3) && dlog << "(ArithmeticFieldCodec) current interval: ["
                                        << (double)low / TOP_VALUE << ","
                                        << (double)high / TOP_VALUE << ")" << std::endl;
            }

            // nothing more to do, we're encoding all the data and an EOF
            if (value_index == wire_value.size())
                break;
        }

        // output exactly the number of bits required to unambiguously
        // store the final range's state
        // 0    .     .     .     1
        // |                      | -- output nothing, unless we have follow bits
        // |                |       -- output a single 0
        if (low == 0) // high must be greater than half
        {
            if (high != TOP_VALUE || bits_to_follow > 0)
                bit_plus_follow(&bits, &bits_to_follow, 0);
        }
        // 0    .     .     .     1
        //       |                | -- output a single 1
        else if (high == TOP_VALUE) // 0 < low < half
        {
            bit_plus_follow(&bits, &bits_to_follow, 1);
        }
        // 0    .     .     .     1
        //     |           |        -- output 01
        //         |           |    -- output 10
        else
        {
            bits_to_follow += 1;
            bit_plus_follow(&bits, &bits_to_follow, (low < FIRST_QTR) ? 0 : 1);
        }

        if (FieldCodecBase::dccl_field_options().GetExtension(arithmetic).debug_assert())
        {
            LOCK_LAST_BITS_MAP_MUTEX
            // bit of a hack so I can get at the exact bit field sizes
            Model::last_bits_map[FieldCodecBase::this_descriptor()->full_name()]
                                [FieldCodecBase::this_field()->name()] = bits;
        }

        return bits;
    }

    void bit_plus_follow(Bitset* bits, int* bits_to_follow, bool bit)
    {
        bits->push_back(bit);
        dccl::dlog.is(dccl::logger::DEBUG3) &&
            dccl::dlog << "(ArithmeticFieldCodec): emitted bit: " << bit << std::endl;

        while (*bits_to_follow)
        {
            dccl::dlog.is(dccl::logger::DEBUG3) &&
                dccl::dlog << "(ArithmeticFieldCodec): emitted bit (from follow): " << !bit
                           << std::endl;

            bits->push_back(!bit);
            (*bits_to_follow) -= 1;
        }
    }

    std::vector<Model::value_type> decode_repeated(Bitset* bits)
    {
        using dccl::dlog;
        using namespace dccl::logger;

        std::vector<Model::value_type> values;
        Model& model = current_model();

        uint64 value = 0;
        uint64 low = 0;
        uint64 high = TOP_VALUE;

        // offset from `bits` to currently examined `value`
        // there are `bit_stream_offset` zeros in the lower bits of `value`
        int bit_stream_offset = Model::CODE_VALUE_BITS - bits->size();

        for (int i = 0, n = Model::CODE_VALUE_BITS; i < n; ++i)
        {
            if (i >= bit_stream_offset)
                value |=
                    (static_cast<uint64>((*bits)[bits->size() - (i - bit_stream_offset) - 1]) << i);
        }

        dlog.is(DEBUG3) && dlog << "(ArithmeticFieldCodec): starting value: "
                                << Bitset(Model::CODE_VALUE_BITS, value).to_string() << std::endl;

        for (unsigned value_index = 0, n = max_repeat(); value_index < n; ++value_index)
        {
            uint64 range = (high - low) + 1;

            Model::symbol_type symbol = bits_to_symbol(bits, value, bit_stream_offset, low, range);

            dlog.is(DEBUG3) && dlog << "(ArithmeticFieldCodec) symbol is: " << symbol << std::endl;

            std::pair<Model::freq_type, Model::freq_type> c_freq_range =
                model.symbol_to_cumulative_freq(symbol, Model::DECODER);

            dlog.is(DEBUG3) && dlog << "(ArithmeticFieldCodec) input symbol (" << symbol
                                    << ") cumulative freq: [" << c_freq_range.first << ","
                                    << c_freq_range.second << ")" << std::endl;

            high = low + (range * c_freq_range.second) / model.total_freq(Model::DECODER) - 1;
            low += (range * c_freq_range.first) / model.total_freq(Model::DECODER);

            model.update_model(symbol, Model::DECODER);

            if (symbol == Model::EOF_SYMBOL)
                break;

            values.push_back(model.symbol_to_value(symbol));

            dlog.is(DEBUG3) && dlog << "(ArithmeticFieldCodec) value is: " << values.back()
                                    << std::endl;

            for (;;)
            {
                if (high < HALF)
                {
                    // nothing
                }
                else if (low >= HALF)
                {
                    value -= HALF;
                    low -= HALF;
                    high -= HALF;
                }
                else if (low >= FIRST_QTR && high < THIRD_QTR)
                {
                    value -= FIRST_QTR;
                    low -= FIRST_QTR;
                    high -= FIRST_QTR;
                }
                else
                    break;

                low <<= 1;
                high <<= 1;
                high += 1;
                value <<= 1;
                bit_stream_offset += 1;
            }
        }

        // for debugging / testing
        if (FieldCodecBase::dccl_field_options().GetExtension(arithmetic).debug_assert())
        {
            LOCK_LAST_BITS_MAP_MUTEX
            // must consume same bits as encoded makes
            Bitset in = Model::last_bits_map[FieldCodecBase::this_descriptor()->full_name()]
                                            [FieldCodecBase::this_field()->name()];

            dlog.is(DEBUG3) && dlog << "(ArithmeticFieldCodec) bits used is (" << bits->size()
                                    << "):     " << *bits << std::endl;
            dlog.is(DEBUG3) && dlog << "(ArithmeticFieldCodec) bits original is (" << in.size()
                                    << "): " << in << std::endl;

            assert(in == *bits);
        }

        return values;
    }

    unsigned size_repeated(const std::vector<Model::value_type>& wire_values)
    {
        // we should really cache this for efficiency
        return encode_repeated(wire_values, false).size();
    }

    // this maximum size will be upper bounded by: ceil(log_2(1/P)) + 1 where P is the
    // probability of this least probable set of symbols
    unsigned max_size_repeated()
    {
        using dccl::log2;
        Model& model = current_model();

        // if user doesn't provide out_of_range frequency, set it to max to force this
        // calculation to return the lowest probability symbol in use
        Model::freq_type out_of_range_freq = model.user_model().out_of_range_frequency();
        if (out_of_range_freq == 0)
            out_of_range_freq = Model::MAX_FREQUENCY;

        Model::value_type lowest_frequency =
            std::min(out_of_range_freq, *std::min_element(model.user_model().frequency().begin(),
                                                          model.user_model().frequency().end()));

        // full of least probable symbols
        unsigned size_least_probable = (unsigned)(std::ceil(
            max_repeat() * (log2(model.total_freq(Model::ENCODER)) - log2(lowest_frequency))));

        dccl::dlog.is(dccl::logger::DEBUG3) &&
            dccl::dlog << "(ArithmeticFieldCodec) size_least_probable: " << size_least_probable
                       << std::endl;

        Model::freq_type eof_freq = model.user_model().eof_frequency();
        // almost full of least probable symbols plus EOF
        unsigned size_least_probable_plus_eof =
            (unsigned)((eof_freq != 0)
                           ? std::ceil(max_repeat() * log2(model.total_freq(Model::ENCODER)) -
                                       (max_repeat() - 1) * log2(lowest_frequency) - log2(eof_freq))
                           : 0);

        dccl::dlog.is(dccl::logger::DEBUG3) &&
            dccl::dlog << "(ArithmeticFieldCodec) size_least_probable_plus_eof: "
                       << size_least_probable_plus_eof << std::endl;

        return std::max(size_least_probable_plus_eof, size_least_probable) + 1;
    }

    unsigned min_size_repeated()
    {
        using dccl::log2;
        const Model& model = current_model();

        if (model.user_model().is_adaptive())
            return 0; // force examining bits from the beginning on decode

        // if user doesn't provide out_of_range frequency, set it to 1 (minimum) to force this
        // calculation to return the highest probability symbol in use
        Model::freq_type out_of_range_freq = model.user_model().out_of_range_frequency();
        if (out_of_range_freq == 0)
            out_of_range_freq = 1;

        Model::freq_type eof_freq = model.user_model().eof_frequency();
        // just EOF
        unsigned size_empty =
            (unsigned)((eof_freq != 0)
                           ? std::ceil(log2(model.total_freq(Model::ENCODER)) - log2(eof_freq))
                           : std::numeric_limits<unsigned>::max());

        dccl::dlog.is(dccl::logger::DEBUG3) &&
            dccl::dlog << "(ArithmeticFieldCodec) size_empty: " << size_empty << std::endl;

        // full with most probable symbol
        Model::value_type highest_frequency =
            std::max(out_of_range_freq, *std::max_element(model.user_model().frequency().begin(),
                                                          model.user_model().frequency().end()));

        unsigned size_most_probable = (unsigned)(std::ceil(
            max_repeat() * (log2(model.total_freq(Model::ENCODER)) - log2(highest_frequency))));

        dccl::dlog.is(dccl::logger::DEBUG3) &&
            dccl::dlog << "(ArithmeticFieldCodec) size_most_probable: " << size_most_probable
                       << std::endl;

        return std::min(size_empty, size_most_probable);
    }

    void validate()
    {
        FieldCodecBase::require(FieldCodecBase::dccl_field_options().HasExtension(arithmetic),
                                "missing (dccl.field).arithmetic");

        std::string model_name =
            FieldCodecBase::dccl_field_options().GetExtension(arithmetic).model();
        try
        {
            model_manager().find(model_name);
        }
        catch (Exception& e)
        {
            FieldCodecBase::require(false, "no such (dccl.field).arithmetic.model called \"" +
                                               model_name + "\" loaded.");
        }
    }

    // end inherited methods

    Model::symbol_type bits_to_symbol(Bitset* bits, uint64& value, int& bit_stream_offset,
                                      uint64 low, uint64 range)
    {
        Model& model = current_model();

        for (;;)
        {
            uint64 value_high = (bit_stream_offset > 0)
                                    ? value + ((static_cast<uint64>(1) << bit_stream_offset) - 1)
                                    : value;

            dccl::dlog.is(dccl::logger::DEBUG3) &&
                dccl::dlog << "(ArithmeticFieldCodec): value range: ["
                           << Bitset(Model::CODE_VALUE_BITS, value) << ","
                           << Bitset(Model::CODE_VALUE_BITS, value_high) << ")" << std::endl;

            Model::freq_type cumulative_freq =
                ((value - low + 1) * model.total_freq(Model::DECODER) - 1) / range;
            Model::freq_type cumulative_freq_high =
                ((value_high - low + 1) * model.total_freq(Model::DECODER) - 1) / range;

            dccl::dlog.is(dccl::logger::DEBUG3) &&
                dccl::dlog << "(ArithmeticFieldCodec): c_freq: " << cumulative_freq
                           << ", c_freq_high: " << cumulative_freq_high << std::endl;

            std::pair<Model::symbol_type, Model::symbol_type> symbol_pair =
                model.cumulative_freq_to_symbol(
                    std::make_pair(cumulative_freq, cumulative_freq_high), Model::DECODER);

            dccl::dlog.is(dccl::logger::DEBUG3) &&
                dccl::dlog << "(ArithmeticFieldCodec): symbol: " << symbol_pair.first << ", "
                           << symbol_pair.second << std::endl;

            if (symbol_pair.first == symbol_pair.second)
                return symbol_pair.first;

            // add another bit to disambiguate
            bits->get_more_bits(1);

            dccl::dlog.is(dccl::logger::DEBUG3) &&
                dccl::dlog << "(ArithmeticFieldCodec): bits: " << *bits << std::endl;

            --bit_stream_offset;
            value |= static_cast<uint64>(bits->back()) << bit_stream_offset;

            dccl::dlog.is(dccl::logger::DEBUG3) &&
                dccl::dlog << "(ArithmeticFieldCodec): ambiguous (symbol could be "
                           << symbol_pair.first << " or " << symbol_pair.second << ")" << std::endl;
        }

        return 0;
    }

    dccl::int32 max_repeat()
    {
        return FieldCodecBase::this_field()->is_repeated()
                   ? FieldCodecBase::dccl_field_options().max_repeat()
                   : 1;
    }

    Model& current_model()
    {
        std::string name = FieldCodecBase::dccl_field_options().GetExtension(arithmetic).model();
        return model_manager().find(name);
    }

    ModelManager& model_manager() { return dccl::arith::model_manager(this->manager()); }
};

// constant integer definitions
template <typename FieldType> const uint64 ArithmeticFieldCodecBase<FieldType>::TOP_VALUE;
template <typename FieldType> const uint64 ArithmeticFieldCodecBase<FieldType>::FIRST_QTR;
template <typename FieldType> const uint64 ArithmeticFieldCodecBase<FieldType>::HALF;
template <typename FieldType> const uint64 ArithmeticFieldCodecBase<FieldType>::THIRD_QTR;

template <typename FieldType>
class ArithmeticFieldCodec : public ArithmeticFieldCodecBase<FieldType>
{
    Model::value_type pre_encode(const FieldType& field_value)
    {
        return static_cast<Model::value_type>(field_value);
    }

    FieldType post_decode(const Model::value_type& wire_value)
    {
        return static_cast<FieldType>(wire_value);
    }
};

template <>
class ArithmeticFieldCodec<const google::protobuf::EnumValueDescriptor*>
    : public ArithmeticFieldCodecBase<const google::protobuf::EnumValueDescriptor*>
{
  public:
    Model::value_type pre_encode(const google::protobuf::EnumValueDescriptor* const& field_value)
    {
        return field_value->number();
    }

    const google::protobuf::EnumValueDescriptor* post_decode(const Model::value_type& wire_value)
    {
        const google::protobuf::EnumDescriptor* e = FieldCodecBase::this_field()->enum_type();
        const google::protobuf::EnumValueDescriptor* return_value =
            e->FindValueByNumber((int)wire_value);

        if (return_value)
            return return_value;
        else
            throw NullValueException();
    }
};

} // namespace arith
} // namespace dccl

#endif
