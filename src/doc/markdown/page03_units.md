# DCCL Units

This page gives the details of DCCL's support for the units of numeric fields. For the suggested usage, see [DCCL Static Units](#dccl-static-units) in the IDL documentation.

The units of a field are given in the `.proto` file with either `(dccl.field).udunits` (a [UDUNITS-2](https://docs.unidata.ucar.edu/udunits/current/) string; the suggested interface) or `(dccl.field).units` (given directly in terms of [Boost Units](http://www.boost.org/doc/html/boost_units.html)), but not both. From either, the DCCL plugin (_protoc-gen-dccl_) to the GPB compiler (_protoc_) generates `_with_units()` accessors and mutators for the dimensioned Boost Units quantities, with full static "unit safety", and correct conversions between different units of the same dimensions (e.g., feet to meters). Unit safety is defined as static (compiler-checked) dimensional analysis. The term is a blending of the (computer science) notion of type safety with (physical) dimensional analysis. For example, in a unit-safe system, the compiler will not allow the user to set a field with dimensions of length to a quantity of hours.

Optionally, libdccl can also convert fields with `(dccl.field).udunits` to and from units given as strings at run time (see [Run-time conversion with UDUNITS-2](#run-time-conversion-with-udunits-2-optional)).


## UDUNITS-2 units: (dccl.field).udunits

The units are given as any unit string that UDUNITS-2 can parse, with a few DCCL-specific differences listed below. The field's `min`, `max` and `resolution` are in the units given. For example:

```
optional double speed = 6
    [(dccl.field) = { udunits: "knot" min: 0 max: 20 resolution: 0.1 }];
optional double heading = 7
    [(dccl.field) = { udunits: "degree" min: 0 max: 360 resolution: 0.1 }];
optional int32 depth = 8
    [(dccl.field) = { udunits: "ft" min: 0 max: 20000 }];
optional double water_temperature = 11
    [(dccl.field) = { udunits: "degC" min: -5 max: 40 resolution: 0.01 }];
optional double latitude = 13
    [(dccl.field) = { udunits: "degree_north" min: -90 max: 90 resolution: 0.000001 }];
optional double modem_rate = 15
    [(dccl.field) = { udunits: "bit/s" min: 0 max: 10000 resolution: 10 }];
```

`(dccl.field).udunits` can only be used on numeric fields.

### Differences from the stock UDUNITS-2 database {#differences-from-the-stock-udunits-2-database}

UDUNITS-2 treats angles as dimensionless, which would allow (for example) a heading to be converted to a percentage. Boost Units, and DCCL, treat these as separate dimensions, so DCCL gives the following units their own dimension when parsing, without modifying the UDUNITS-2 database:

| DCCL dimension | Units |
|----------------|-------|
| Plane angle    | `radian` (`rad`), `degree` (`arcdeg`, `°`), `arcminute`, `arcsecond`, `grade`, `revolution` (`circle`, `cycle`, `turn`, `rotation`), `degree_north`, `degree_east`, `degree_true`, `degree_west` (and the CF spellings of these, such as `degrees_north` and `degreeN`). Rotation rates (`rpm`, `rps`, `cps`) are plane angle per time. |
| Solid angle    | `steradian` (`sr`). This makes `lumen`, `lux`, `phot` and `footcandle` luminous intensity times solid angle (per area), as in Boost Units. |
| Information    | `bit`, `byte` and `octet`, with SI prefixes (e.g., `kilobyte` is 1000 bytes). UDUNITS-2 has no symbols for these: `B` is the bel and `b` is the barn. A data rate such as `bit/s` does not convert to `Hz`. |

Some UDUNITS-2 units are rejected by the plugin since they are rarely what was intended:

- `ppt` and `pptv` are parts per *trillion* in UDUNITS-2, not parts per thousand. Use `1e-3` for a dimensionless part per thousand, or the actual ratio (e.g., `g/kg`).
- `PSU`, `psu` and `PSS` are not UDUNITS-2 units. Practical salinity (PSS-78) is dimensionless: write `1`, as the CF conventions do for `sea_water_practical_salinity`.
- Timestamps (e.g., `seconds since 1970-01-01`) and logarithmic units have no Boost Units equivalent. Use the `dccl.time` codec for timestamps.
- Units with an offset (other than temperatures such as `degC` and `degF`) cannot be represented in Boost Units.

Note that `deg` is not a UDUNITS-2 unit: use `degree`.

### Generated code from (dccl.field).udunits

When _protoc-gen-dccl_ is built with both UDUNITS-2 and Boost Units (see [Building with units support](#building-with-units-support)), each field with `(dccl.field).udunits` gets the same `_with_units()` methods as described in [DCCL Units Generated Code](#dccl-units-generated-code). The field's Boost Units type is always the coherent SI unit of its dimension (with the plane angle, solid angle and information dimensions measured in radians, steradians and bits), and the conversion from the field's own unit is built into the generated methods. For example, for `udunits: "knot"` on a double field `speed`:

```cpp
static const char* speed_udunits() { return "knot"; }
typedef boost::units::derived_dimension< boost::units::length_base_dimension,1, boost::units::time_base_dimension,-1 >::type speed_dimension;
typedef boost::units::unit<speed_dimension,boost::units::si::system> speed_unit;
static constexpr double speed_si_scale = 0.51444444444444448;

template<typename Quantity >
  void set_speed_with_units(Quantity value_w_units)
  { set_speed(static_cast<double>(boost::units::quantity<speed_unit,double >(value_w_units).value() / speed_si_scale)); };

template<typename Quantity >
  Quantity speed_with_units() const
  { return Quantity((speed() * speed_si_scale) * speed_unit()); };

boost::units::quantity< speed_unit,double > speed_with_units() const
  { return speed_with_units<boost::units::quantity< speed_unit,double > >(); };
```

So `speed_with_units()` returns meters per second, `speed_with_units<quantity<Knot>>()` returns knots, and `speed()` is still the value in knots that DCCL encodes. Some details:

- Integer fields are rounded to the nearest integer when set (e.g., 100 m into a `ft` integer field is stored as 328).
- Units with an offset (`degC`, `degF`) give a `boost::units::absolute<>` temperature, so conversions between temperature scales account for the offset. Units without an offset, such as `K`, give a plain (relative) temperature; for a temperature difference in Celsius degrees use `K`.
- `static const char* <field>_udunits()` returns the unit string from the `.proto` file.
- The plugin reports an error if `<field>_udunits` (or `<field>_with_udunits`, when run-time conversion is enabled) is also the name of a field in the message.

If _protoc-gen-dccl_ is built with UDUNITS-2 but not Boost Units, only `<field>_udunits()` is generated (and the run-time accessors, if enabled).

## Boost Units metadata: (dccl.field).units {#boost-units-metadata-dcclfieldunits}

The original DCCL units interface is given directly in terms of Boost Units. The units of a given field are given by two parameters: the physical dimension (e.g., length, force, mass, etc.), and the unit system which defaults to the International System of Units (SI). The units of the field can also be specified directly, outside of a canonical system (e.g., nautical mile, fathom, yard, knot, etc.). This requires _protoc-gen-dccl_ to be built with Boost Units: if it is not, `(dccl.field).units` is an error.

![Base Dimensions](base-dimensions.png)

The Units field extension has the following options:

- `base_dimensions` (string): Specifies the dimensions of the field as a combination of powers of the base dimensions given in Table 3. For example, acceleration would be defined as `"LT^-2"`.
- `derived_dimensions` (string): As a convenience alternative to the `base_dimensions` specification, any of the Boost Units "derived dimensions" can be used. For example, instead of `base_dimensions: "L^-1 M T^-2"` for pressure, one can use `derived_dimensions: "pressure"`. Multiplication and division of derived dimensions are also supported using the "*" and "/" operators. The available derived dimensions are those listed here: [Dimensions Reference](http://www.boost.org/doc/html/boost_units/Reference.html#dimensions_reference). The string to use here is the name of the dimension as given in Boost Units, minus the "_dimension". For example, to use boost::units::acceleration_dimension, specify "acceleration" in this field.
- `system` (string, defaults to "si"): A boost::units or user-defined system of units to use for this field. Defaults to the SI system with base units of kelvin (temperature), second (time), meter (length), kilogram (mass), candela (luminous intensity), mole (amount of substance) and ampere (electric current). The available Boost Units systems include:
  - "si": [SI System Reference](http://www.boost.org/doc/html/boost_units/Reference.html#si_system_reference)
  - "cgs": [CGS System Reference](http://www.boost.org/doc/html/boost_units/Reference.html#cgs_system_reference)
  - "temperature::celsius": [Temperature System Reference](http://www.boost.org/doc/html/boost_units/Reference.html#temperature_system_reference)
  - "temperature::fahrenheit": [Temperature System Reference](http://www.boost.org/doc/html/boost_units/Reference.html#temperature_system_reference)
  - "angle::degree": [Trigonometry and Angle System Reference](http://www.boost.org/doc/html/boost_units/Reference.html#trigonometry_and_angle_system_reference)
  - "angle::gradian": [Trigonometry and Angle System Reference](http://www.boost.org/doc/html/boost_units/Reference.html#trigonometry_and_angle_system_reference)
  - "angle::revolution": [Trigonometry and Angle System Reference](http://www.boost.org/doc/html/boost_units/Reference.html#trigonometry_and_angle_system_reference)

  You can also create your own system and use it in DCCL. To do this, make the namespace the same as the include file path, where "::" is replaced by "/". For example, if you define a system "foo::barsystem", you need to provide the definition of this system to the C++ compiler in "foo/barsystem.hpp".
- `relative_temperature` (bool, defaults to false): A special extension only used for temperature fields. Setting this to true means that the temperature is relative (i.e., a difference of absolute temperatures) instead of an absolute temperature. This matters to support correct unit conversions between different temperature systems. For example, relative degrees Kelvin are the same as relative degrees Celsius, but the absolute scales differ by 273.15 degrees.
- `unit` (string): As an alternative to the `dimensions` and `system` specification, the field can be set to use particular (typically non-SI) units. A few examples of such units that are still often encountered in the marine domain are `unit: "metric::nautical_mile"`, `unit: "metric::bar"`, and `unit: "us::yard"`. Here you can use any of the Boost Units Base Unit types, given here: [Base Units by Category](http://www.boost.org/doc/html/boost_units/Reference.html#boost_units.Reference.base_units_by_category). The string to specify here is the Base Unit to use, minus the "boost::units::" prefix and the "_base_unit" suffix. For example, `boost::units::metric::nautical_mile_base_unit` should be specified as "metric::nautical_mile" in this field.
- `custom`: Use custom units defined in your code.
	+ `unit` (string): Fully qualified name for the unit (including namespaces). For example, "dccl::units::microsiemens_per_cm_unit". This is a [`boost::units::unit` type](https://www.boost.org/doc/libs/latest/doc/html/doxygen/units_reference/classboost_1_1units_1_1unit.html).
	+ `header` (string): Relative path to header defining this custom unit. For example, "dccl/units/conductivity.h"

## DCCL Units Generated Code {#dccl-units-generated-code}

The DCCL Units C++ generated accessors and mutators mirror those provided by the standard Google Protocol Buffers compiler for numeric fields, with the method name appended by the string "_with_units". For the standard Protobuf generated code see [Google Protocol Buffers Reference](https://developers.google.com/protocol-buffers/docs/reference/cpp-generated). DCCL Units are only valid on numeric fields (either singular or repeated). Two accessors are provided for convenience: a non-template accessor that returns the value as the Quantity (i.e., `boost::units::quantity`) defined in the DCCL field, and a template accessor that can take any valid Boost Units Quantity (i.e., a type with the same dimensions as the DCCL field) and return the value in that type, accounting for all conversion factors.

### Singular Numeric Fields

For a singular (optional or required) field "foo" with the following parameters:

- Type: "[foo type]", the original unit-less field type (e.g. `google::protobuf::int32`, `double`)
- Unit dimension: "[foo dimension]" (e.g. `boost::units::length_dimension`, `boost::units::derived_dimension<boost::units::length_base_dimension,1, boost::units::time_base_dimension,-1>::type`)
- Unit system: "[foo system]" (e.g. `boost::units::si::system`, `boost::units::degree::system`)

the following additional methods are defined for unit safe access to the DCCL message:

```cpp
typedef [foo dimension] foo_dimension;  
typedef boost::units::unit<foo_dimension, [foo system]> foo_unit;
  
// set the field's value using the given Quantity (which must have the same dimensions as foo_dimension), performing all necessary conversions (e.g. from yards to meters).
template<typename Quantity >
  void set_foo_with_units(Quantity value_w_units)
  { set_foo(boost::units::quantity<foo_unit,google::protobuf::int32 >(value_w_units).value() ); };
  
// get the field's value using the given Quantity (which must have the same dimensions as foo_dimension), performing all necessary conversions.
template<typename Quantity >
  Quantity foo_with_units() const
  { return Quantity(foo() * foo_unit()); };
  
// get the field's value as a Quantity of foo_unit, as defined in the DCCL message.
boost::units::quantity< foo_unit > foo_with_units() const
  { return foo_with_units<boost::units::quantity< foo_unit, [foo type] > >(); };
```

### Repeated Numeric Fields

For a repeated field "foo" with the following parameters:

- Type: "[foo type]"
- Unit dimension: "[foo dimension]"
- Unit system: "[foo system]"

the following additional methods are defined for unit safe access to the DCCL message:

```cpp
typedef [foo dimension] foo_dimension; 
typedef boost::units::unit<foo_dimension, [foo system]> foo_unit;

// set a given index of the repeated field using the given Quantity.
template<typename Quantity >
  void set_foo_with_units(int index, Quantity value_w_units)
  { set_foo(index, boost::units::quantity<foo_unit, [foo type]>(value_w_units).value() ); };
  
// add a new value to the end of the repeated field using the given Quantity.
template<typename Quantity >
  void add_foo_with_units(Quantity value_w_units)
  { add_foo(boost::units::quantity<foo_unit,google::protobuf::int32 >(value_w_units).value() ); };
  
// get the field's value using the given Quantity at the given index
template<typename Quantity >
  Quantity foo_with_units(int index) const
  { return Quantity(foo(index) * foo_unit()); };
  
// get the field's value as a Quantity of foo_unit at a given index
boost::units::quantity< foo_unit > foo_with_units(int index) const
  { return foo_with_units<boost::units::quantity< foo_unit,google::protobuf::int32 > >(index); };
```

## DCCL Units examples

Here are a few example DCCL messages which include unit specification:

### AUVStatus

See `test/dccl_units/auv_status.proto`

For example, to set an AUVStatus message's `x` and `y` fields to meters (the default for the base dimension of length, since the default system is SI), and then later access them as nautical miles, one can use this C++ example:

```cpp
using namespace boost::units;
typedef metric::nautical_mile_base_unit::unit_type 
   NauticalMile;

AUVStatus status;
status.set_x_with_units(1000*si::meters);
status.set_y_with_units(500*si::meters);

quantity<NauticalMile> x_nm(status.x_with_units());
quantity<NauticalMile> y_nm(status.y_with_units());
```

The value of `x_nm` is 0.54 nautical miles and `y_nm` is 0.27 nautical miles.

### CTDMessage

See `test/dccl_units/ctd_message_units.proto`

### CommandMessage 

See `test/dccl_units/command_message.proto`

### AUVStatus with UDUNITS-2

See `test/dccl_udunits_boost/auv_status.proto`, which defines an AUVStatus message using `(dccl.field).udunits`, and `test/dccl_udunits_boost/test.cpp`, which sets and reads it using the generated Boost Units methods. The example in [DCCL Static Units](#dccl-static-units) is taken from these.

## Run-time conversion with UDUNITS-2 (optional) {#run-time-conversion-with-udunits-2-optional}

When DCCL is built with `-Denable_udunits_runtime=ON` (this requires `enable_udunits`, and is off by default), libdccl links to UDUNITS-2 and can convert fields with `(dccl.field).udunits` to and from units given as strings at run time. This is useful when the units are not known at compile time, such as in tools that handle any DCCL message using Protobuf reflection, or when Boost Units is not available. For compile-time checked code, prefer the Boost Units methods above.

With this option, the plugin also generates the following methods for each field `foo` with `(dccl.field).udunits` (shown for a singular double field):

```cpp
// get the field's value converted to the given unit, e.g. foo_with_udunits("km/h")
double foo_with_udunits(const std::string& unit) const;
// set the field from a value in the given unit, e.g. set_foo_with_udunits(1.5, "m/s")
void set_foo_with_udunits(double value, const std::string& unit);
```

Repeated fields get `foo_with_udunits(int index, unit)`, `set_foo_with_udunits(int index, value, unit)` and `add_foo_with_udunits(value, unit)`. As with the Boost Units methods, integer fields are rounded to the nearest integer when set.

The same conversions are available directly, and for any message through reflection, in `dccl/units/udunits.h` (available when `DCCL_HAS_UDUNITS_RUNTIME` in `dccl/def.h` is 1):

```cpp
namespace dccl::units::udunits
{
// convert value from one unit to another, e.g. convert(1, "knot", "m/s")
double convert(double value, const std::string& from, const std::string& to);
// whether from can be converted to to
bool convertible(const std::string& from, const std::string& to);
// the field's (dccl.field).udunits, or an empty string if it has none
std::string unit_of(const google::protobuf::FieldDescriptor* field);
// the field's value in the given unit (index is required for repeated fields)
double get(const google::protobuf::Message& msg, const google::protobuf::FieldDescriptor* field,
           const std::string& unit, int index = -1);
// set the field from a value in the given unit (index -1 adds to a repeated field)
void set(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field,
         double value, const std::string& unit, int index = -1);
}
```

These parse units with the same [differences from the stock UDUNITS-2 database](#differences-from-the-stock-udunits-2-database) as the plugin, so (for example) `degree` does not convert to `percent`. Errors (unparseable units, units that cannot be converted, or a field without `(dccl.field).udunits`) throw `dccl::Exception`. Parsed conversions are cached, and the functions are thread-safe when DCCL is built with thread support. See `test/dccl_udunits_runtime/test.cpp` for examples.

## Building with units support {#building-with-units-support}

The units methods are generated by _protoc-gen-dccl_, which is always built. Which units interfaces it supports depends on the libraries found when DCCL is built:

| CMake option | Default | Requires | Effect |
|--------------|---------|----------|--------|
| `enable_units` | ON if Boost is found | Boost headers (`libboost-dev`) | `(dccl.field).units`, and Boost Units methods for `(dccl.field).udunits` |
| `enable_udunits` | ON if UDUNITS-2 is found | `libudunits2-dev` | `(dccl.field).udunits` in _protoc-gen-dccl_ |
| `enable_udunits_runtime` | OFF | `enable_udunits` | Run-time conversion in libdccl, and the `_with_udunits` methods |

For the suggested combination (UDUNITS-2 interface with Boost Units runtime), build DCCL with both Boost and UDUNITS-2 installed and leave `enable_udunits_runtime` off:

```bash
sudo apt install libboost-dev libudunits2-dev
```

A field whose units need a library that _protoc-gen-dccl_ was not built with is a _protoc_ error that names the CMake option to enable, so messages with units are never silently compiled without them.

Packagers or projects that want to restrict what is generated (for example, to avoid requiring Boost headers in code that uses the generated messages) can pass the `units` parameter to the plugin: `--dccl_out=units=VALUE:OUTPUT_DIR`, where `VALUE` is one of `none`, `boost`, `udunits` or `both` (the default is everything that was built in). Downstream CMake projects can check `DCCL_HAS_BOOST_UNITS`, `DCCL_PLUGIN_HAS_UDUNITS` and `DCCL_HAS_UDUNITS_RUNTIME`, set by `find_package(DCCL)`.
