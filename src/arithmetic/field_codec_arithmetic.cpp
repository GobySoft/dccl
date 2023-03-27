// Copyright 2012-2017:
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
#include "field_codec_arithmetic.h"
#include "dccl/codec.h"
#include "dccl/field_codec_manager.h"

using dccl::dlog;
using namespace dccl::logger;

const dccl::arith::Model::symbol_type dccl::arith::Model::OUT_OF_RANGE_SYMBOL;
const dccl::arith::Model::symbol_type dccl::arith::Model::EOF_SYMBOL;
const dccl::arith::Model::symbol_type dccl::arith::Model::MIN_SYMBOL;
const int dccl::arith::Model::CODE_VALUE_BITS;
const int dccl::arith::Model::FREQUENCY_BITS;
const dccl::arith::Model::freq_type dccl::arith::Model::MAX_FREQUENCY;

#if DCCL_THREAD_SUPPORT
std::recursive_mutex dccl::arith::Model::last_bits_map_mutex;
#endif
std::map<std::string, std::map<std::string, dccl::Bitset>> dccl::arith::Model::last_bits_map;

// shared library load
extern "C"
{
    void dccl3_load(dccl::Codec* dccl) { dccl_arithmetic_load(dccl); }

    void dccl_arithmetic_load(dccl::Codec* dccl)
    {
        using namespace dccl;
        using namespace dccl::arith;

        dccl->manager().add<ArithmeticFieldCodec<int32>>("_arithmetic");
        dccl->manager().add<ArithmeticFieldCodec<int64>>("_arithmetic");
        dccl->manager().add<ArithmeticFieldCodec<uint32>>("_arithmetic");
        dccl->manager().add<ArithmeticFieldCodec<uint64>>("_arithmetic");
        dccl->manager().add<ArithmeticFieldCodec<double>>("_arithmetic");
        dccl->manager().add<ArithmeticFieldCodec<float>>("_arithmetic");
        dccl->manager().add<ArithmeticFieldCodec<bool>>("_arithmetic");
        dccl->manager().add<ArithmeticFieldCodec<const google::protobuf::EnumValueDescriptor*>>(
            "_arithmetic");

        dccl->manager().add<ArithmeticFieldCodec<int32>>("dccl.arithmetic");
        dccl->manager().add<ArithmeticFieldCodec<int64>>("dccl.arithmetic");
        dccl->manager().add<ArithmeticFieldCodec<uint32>>("dccl.arithmetic");
        dccl->manager().add<ArithmeticFieldCodec<uint64>>("dccl.arithmetic");
        dccl->manager().add<ArithmeticFieldCodec<double>>("dccl.arithmetic");
        dccl->manager().add<ArithmeticFieldCodec<float>>("dccl.arithmetic");
        dccl->manager().add<ArithmeticFieldCodec<bool>>("dccl.arithmetic");
        dccl->manager().add<ArithmeticFieldCodec<const google::protobuf::EnumValueDescriptor*>>(
            "dccl.arithmetic");
    }
    void dccl3_unload(dccl::Codec* dccl) { dccl_arithmetic_unload(dccl); }

    void dccl_arithmetic_unload(dccl::Codec* dccl)
    {
        using namespace dccl;
        using namespace dccl::arith;

        dccl->manager().remove<ArithmeticFieldCodec<int32>>("_arithmetic");
        dccl->manager().remove<ArithmeticFieldCodec<int64>>("_arithmetic");
        dccl->manager().remove<ArithmeticFieldCodec<uint32>>("_arithmetic");
        dccl->manager().remove<ArithmeticFieldCodec<uint64>>("_arithmetic");
        dccl->manager().remove<ArithmeticFieldCodec<double>>("_arithmetic");
        dccl->manager().remove<ArithmeticFieldCodec<float>>("_arithmetic");
        dccl->manager().remove<ArithmeticFieldCodec<bool>>("_arithmetic");
        dccl->manager().remove<ArithmeticFieldCodec<const google::protobuf::EnumValueDescriptor*>>(
            "_arithmetic");

        dccl->manager().remove<ArithmeticFieldCodec<int32>>("dccl.arithmetic");
        dccl->manager().remove<ArithmeticFieldCodec<int64>>("dccl.arithmetic");
        dccl->manager().remove<ArithmeticFieldCodec<uint32>>("dccl.arithmetic");
        dccl->manager().remove<ArithmeticFieldCodec<uint64>>("dccl.arithmetic");
        dccl->manager().remove<ArithmeticFieldCodec<double>>("dccl.arithmetic");
        dccl->manager().remove<ArithmeticFieldCodec<float>>("dccl.arithmetic");
        dccl->manager().remove<ArithmeticFieldCodec<bool>>("dccl.arithmetic");
        dccl->manager().remove<ArithmeticFieldCodec<const google::protobuf::EnumValueDescriptor*>>(
            "dccl.arithmetic");
    }
}

dccl::arith::Model::symbol_type dccl::arith::Model::value_to_symbol(value_type value) const
{
    if (value < *user_model_.value_bound().begin() ||
        value > *(user_model_.value_bound().end() - 1))
        return Model::OUT_OF_RANGE_SYMBOL;

    google::protobuf::RepeatedField<double>::const_iterator upper_it =
        std::upper_bound(user_model_.value_bound().begin(), user_model_.value_bound().end(), value);

    google::protobuf::RepeatedField<double>::const_iterator lower_it =
        (upper_it == user_model_.value_bound().begin()) ? upper_it : upper_it - 1;

    double lower_diff = std::abs((*lower_it) * (*lower_it) - value * value);
    double upper_diff = std::abs((*upper_it) * (*upper_it) - value * value);

    //    std::cout << "value: " << value << std::endl;
    //    std::cout << "lower_value: " << *lower_it << std::endl;
    //    std::cout << "upper_value: " << *upper_it << std::endl;

    symbol_type symbol =
        ((lower_diff < upper_diff) ? lower_it : upper_it) - user_model_.value_bound().begin();

    return symbol;
}

dccl::arith::Model::value_type dccl::arith::Model::symbol_to_value(symbol_type symbol) const
{
    if (symbol == EOF_SYMBOL)
        throw(Exception("EOF symbol has no value."));

    value_type value = (symbol == Model::OUT_OF_RANGE_SYMBOL)
                           ? std::numeric_limits<value_type>::quiet_NaN()
                           : user_model_.value_bound(symbol);

    return value;
}

std::pair<dccl::arith::Model::freq_type, dccl::arith::Model::freq_type>
dccl::arith::Model::symbol_to_cumulative_freq(symbol_type symbol, ModelState state) const
{
    const auto& c_freqs =
        (state == ENCODER) ? encoder_cumulative_freqs_ : decoder_cumulative_freqs_;

    auto c_freq_it = c_freqs.find(symbol);
    std::pair<freq_type, freq_type> c_freq_range;
    c_freq_range.second = c_freq_it->second;
    if (c_freq_it == c_freqs.begin())
    {
        c_freq_range.first = 0;
    }
    else
    {
        c_freq_it--;
        c_freq_range.first = c_freq_it->second;
    }
    return c_freq_range;
}

std::pair<dccl::arith::Model::symbol_type, dccl::arith::Model::symbol_type>
dccl::arith::Model::cumulative_freq_to_symbol(std::pair<freq_type, freq_type> c_freq_pair,
                                              ModelState state) const
{
    const auto& c_freqs =
        (state == ENCODER) ? encoder_cumulative_freqs_ : decoder_cumulative_freqs_;

    std::pair<symbol_type, symbol_type> symbol_pair;

    // find the symbol for which the cumulative frequency is greater than
    // e.g.
    // symbol: 0   freq: 10   c_freq: 10 [0 ... 10)
    // symbol: 1   freq: 15   c_freq: 25 [10 ... 25)
    // symbol: 2   freq: 10   c_freq: 35 [25 ... 35)
    // searching for c_freq of 30 should return symbol 2
    // searching for c_freq of 10 should return symbol 1
    auto search = c_freq_pair.first;
    for (const auto& p : c_freqs)
    {
        if (search < p.second)
        {
            symbol_pair.first = p.first;
            break;
        }
    }

    if (symbol_pair.first == c_freqs.rbegin()->first)
        symbol_pair.second = symbol_pair.first; // last symbol can't be ambiguous on the low end
    else if (c_freqs.find(symbol_pair.first)->second > c_freq_pair.second)
        symbol_pair.second = symbol_pair.first; // unambiguously this symbol
    else
        symbol_pair.second = symbol_pair.first + 1;

    return symbol_pair;
}

void dccl::arith::Model::update_model(symbol_type symbol, ModelState state)
{
    if (!user_model_.is_adaptive())
        return;

    auto& c_freqs = (state == ENCODER) ? encoder_cumulative_freqs_ : decoder_cumulative_freqs_;

    if (dlog.check(DEBUG3))
    {
        dlog.is(DEBUG3) && dlog << "Model was: " << std::endl;
        for (symbol_type i = MIN_SYMBOL, n = max_symbol(); i <= n; ++i)
        {
            auto it = c_freqs.find(i);
            if (it != c_freqs.end())
                dlog.is(DEBUG3) && dlog << "Symbol: " << it->first << ", c_freq: " << it->second
                                        << std::endl;
        }
    }

    for (symbol_type i = max_symbol(), n = symbol; i >= n; --i)
    {
        auto it = c_freqs.find(i);
        if (it != c_freqs.end())
            ++it->second;
    }

    if (dlog.check(DEBUG3))
    {
        dlog.is(DEBUG3) && dlog << "Model is now: " << std::endl;
        for (symbol_type i = MIN_SYMBOL, n = max_symbol(); i <= n; ++i)
        {
            auto it = c_freqs.find(i);
            if (it != c_freqs.end())
                dlog.is(DEBUG3) && dlog << "Symbol: " << it->first << ", c_freq: " << it->second
                                        << std::endl;
        }
    }

    dlog.is(DEBUG3) && dlog << "total freq: " << total_freq(state) << std::endl;
}

void dccl::arith::ModelManager::set_model(dccl::Codec& codec,
                                          const protobuf::ArithmeticModel& model)
{
    model_manager(codec.manager())._set_model(model);
}

dccl::arith::ModelManager& dccl::arith::model_manager(FieldCodecManagerLocal& manager)
{
    if (!manager.codec_data().template has_codec_specific_data<ArithmeticFieldCodecBase<>>())
    {
        auto model_manager = std::make_shared<dccl::any>(ModelManager());
        manager.codec_data().template set_codec_specific_data<ArithmeticFieldCodecBase<>>(
            model_manager);
    }
    return dccl::any_cast<ModelManager&>(
        *manager.codec_data().template codec_specific_data<ArithmeticFieldCodecBase<>>());
}
