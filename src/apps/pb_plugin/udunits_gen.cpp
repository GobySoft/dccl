// Copyright 2026:
//   GobySoft, LLC (2013-)
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

#include "udunits_gen.h"

#include <stdexcept>
#include <utility>

#include "dccl/units/udunits_special_units.h"

using dccl::units::udunits::UnitPtr;

namespace
{
const dccl::units::udunits::UnitSystem& unit_system()
{
    static const dccl::units::udunits::UnitSystem system;
    return system;
}

// The basic units that a parsed unit is a product of, with their powers
struct Factors
{
    std::vector<std::pair<const ut_unit*, int>> factors;
    std::string error;
};

ut_status visit_basic(const ut_unit* unit, void* arg)
{
    static_cast<Factors*>(arg)->factors.emplace_back(unit, 1);
    return UT_SUCCESS;
}

ut_status visit_product(const ut_unit*, int count, const ut_unit* const* basic_units,
                        const int* powers, void* arg)
{
    for (int i = 0; i < count; ++i)
        static_cast<Factors*>(arg)->factors.emplace_back(basic_units[i], powers[i]);
    return UT_SUCCESS;
}

ut_status visit_galilean(const ut_unit*, double, const ut_unit* underlying, double, void* arg);

ut_status visit_timestamp(const ut_unit*, const ut_unit*, double, void* arg)
{
    static_cast<Factors*>(arg)->error = "a timestamp unit";
    return UT_VISIT_ERROR;
}

ut_status visit_logarithmic(const ut_unit*, double, const ut_unit*, void* arg)
{
    static_cast<Factors*>(arg)->error = "a logarithmic unit";
    return UT_VISIT_ERROR;
}

const ut_visitor factor_visitor{visit_basic, visit_product, visit_galilean, visit_timestamp,
                                visit_logarithmic};

// scale and offset are found with a converter, so only the underlying unit's factors matter here
ut_status visit_galilean(const ut_unit*, double, const ut_unit* underlying, double, void* arg)
{
    return ut_accept_visitor(underlying, &factor_visitor, arg);
}
} // namespace

void dccl::units::udunits::validate(const std::string& unit)
{
    try
    {
        unit_system().parse(unit);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("(dccl.field).udunits ") + e.what());
    }
}

dccl::units::udunits::BoostUnits dccl::units::udunits::to_boost_units(const std::string& unit)
{
    const auto& system = unit_system();
    UnitPtr parsed = system.parse(unit);

    Factors factors;
    if (ut_accept_visitor(parsed.get(), &factor_visitor, &factors) != UT_SUCCESS)
        throw std::runtime_error("(dccl.field).udunits \"" + unit + "\" is " +
                                 (factors.error.empty() ? "not supported" : factors.error) +
                                 ", which has no Boost Units equivalent.");

    // Boost Units base dimensions in the order they're written, with the UDUNITS-2 base unit of each
    struct BaseDimension
    {
        std::string name;
        UnitPtr unit;
        int power;
    };
    auto stock = [&](const char* symbol)
    { return UnitPtr(ut_parse(system.system(), symbol, UT_ASCII), &ut_free); };
    auto clone = [](const ut_unit* u) { return UnitPtr(ut_clone(u), &ut_free); };
    std::vector<BaseDimension> base_dimensions;
    base_dimensions.push_back({"length", stock("m"), 0});
    base_dimensions.push_back({"mass", stock("kg"), 0});
    base_dimensions.push_back({"time", stock("s"), 0});
    base_dimensions.push_back({"current", stock("A"), 0});
    base_dimensions.push_back({"temperature", stock("K"), 0});
    base_dimensions.push_back({"amount", stock("mol"), 0});
    base_dimensions.push_back({"luminous_intensity", stock("cd"), 0});
    base_dimensions.push_back({"plane_angle", clone(system.plane_angle()), 0});
    base_dimensions.push_back({"solid_angle", clone(system.solid_angle()), 0});
    base_dimensions.push_back({"information", clone(system.information()), 0});

    for (const auto& factor : factors.factors)
    {
        bool found = false;
        for (auto& base_dimension : base_dimensions)
        {
            if (ut_compare(factor.first, base_dimension.unit.get()) == 0)
            {
                base_dimension.power += factor.second;
                found = true;
                break;
            }
        }
        if (!found)
            throw std::runtime_error("(dccl.field).udunits \"" + unit +
                                     "\" uses a base unit that Boost Units has no dimension for.");
    }

    // the coherent SI unit of the same dimension, to find scale and offset
    BoostUnits boost_units;
    UnitPtr si(ut_clone(ut_get_dimensionless_unit_one(system.system())), &ut_free);
    for (const auto& base_dimension : base_dimensions)
    {
        if (base_dimension.power == 0)
            continue;
        boost_units.dimensions.push_back(base_dimension.name);
        boost_units.powers.push_back(base_dimension.power);
        UnitPtr raised(ut_raise(base_dimension.unit.get(), base_dimension.power), &ut_free);
        si.reset(ut_multiply(si.get(), raised.get()));
    }
    if (boost_units.dimensions.empty())
    {
        boost_units.dimensions.push_back("dimensionless");
        boost_units.powers.push_back(1);
    }

    cv_converter* converter = ut_get_converter(parsed.get(), si.get());
    if (!converter)
        throw std::runtime_error("(dccl.field).udunits \"" + unit +
                                 "\" can't be converted to SI units.");
    boost_units.offset = cv_convert_double(converter, 0.0);
    boost_units.scale = cv_convert_double(converter, 1.0) - boost_units.offset;
    cv_free(converter);

    const bool temperature_only = boost_units.dimensions.size() == 1 &&
                                  boost_units.dimensions[0] == "temperature" &&
                                  boost_units.powers[0] == 1;
    if (boost_units.offset != 0 && !temperature_only)
        throw std::runtime_error("(dccl.field).udunits \"" + unit +
                                 "\" has an offset, which Boost Units only supports for "
                                 "temperatures.");

    return boost_units;
}
