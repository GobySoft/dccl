#ifndef DCCL_UNITS_DEGREES_H
#define DCCL_UNITS_DEGREES_H

#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/si/angular_velocity.hpp>

namespace dccl
{
namespace units
{
typedef boost::units::divide_typeof_helper<boost::units::degree::plane_angle,
                                           boost::units::si::time>::type degrees_per_second_unit;

static const degrees_per_second_unit degrees_per_second;

} // namespace units
} // namespace dccl

#endif
