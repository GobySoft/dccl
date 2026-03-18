#ifndef DCCL_UNITS_CONDUCTIVITY_H
#define DCCL_UNITS_CONDUCTIVITY_H

#include <boost/units/systems/si/conductivity.hpp>
#include <boost/units/systems/si/prefixes.hpp>

namespace dccl
{
namespace units
{
// 1 uS/cm = 10^-4 S/m
typedef boost::units::make_scaled_unit<
    boost::units::si::conductivity,
    boost::units::scale<10, boost::units::static_rational<-4>>>::type microsiemens_per_cm_unit;

static const microsiemens_per_cm_unit microsiemens_per_cm;

typedef boost::units::si::conductivity siemens_per_m_unit;
static const siemens_per_m_unit siemens_per_m;

} // namespace units
} // namespace dccl

#endif
