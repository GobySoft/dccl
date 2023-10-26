# DCCL Interface Descriptor Language (IDL)

DCCL uses the Google Protocol Buffers (Protobuf) language to define messages. The DCCL IDL is defined as extensions to the Protobuf language message and field options to allow more compact encoding than is possible with the default Protobuf meta-data. You should familiarize yourself with basic Protobuf usage before reading the rest of this document: see [https://protobuf.dev/overview/](https://protobuf.dev/overview/).

An example DCCL message is as follows:

```protobuf
import "dccl/option_extensions.proto";

message NavigationReport {
  option (dccl.msg) = { codec_version: 4
                        id: 124
                        max_bytes: 32 };
  required double x = 1 [(dccl.field) = { min: -10000 max: 10000 precision: 1 }];
  required double y = 2 [(dccl.field) = { min: -10000 max: 10000 precision: 1 }];
  required double z = 3 [(dccl.field) = { min: -5000 max: 0 precision: 0 }];
  enum VehicleClass { AUV = 1; USV = 2; SHIP = 3; }
  optional VehicleClass veh_class = 4;
  optional bool battery_ok = 5;
}
```

In the above message, the snippet `option (dccl.msg) = { codec_version: 4 id: 124 max_bytes: 32 };` represents the **message** options extensions since they affect the design of the entire DCCL message (in this case "NavigationReport"). The **field** options affect a given field, e.g. `[(dccl.field) = { min: -10000 max: 10000 precision: 1 }];`

The full Protobuf definition of the DCCL extensions is given in option_extensions.proto (as messages DCCLFieldOptions and DCCLMessageOptions).

## DCCL options

The available DCCL options is given in the following Table 1:

### Table 1: Definition of the DCCL Interface Description Language

| Extension Name           | Extension Type | Explanation                                                                        | Applicable Fields | Symbol | Default      |
|--------------------------|----------------|------------------------------------------------------------------------------------|-------------------|--------|--------------|
| **Message Extensions** *[a]*   |           |                                         |                   |        |            |
| `(dccl.msg).id`          | int32          | Unique identifying integer for this message                                        |                   |        | -            |
| `(dccl.msg).omit_id`          | bool          | If true, omit (dccl.msg).id from the message definition and encoded message. This requires some other means of determining the message type at the receiver (useful when wrapping DCCL messages in another protocol or when using only a single DCCL type in a particular link). |                   |        | False           |
| `(dccl.msg).max_bytes`   | uint32         | Enforced upper bound for the encoded message length                                |                   |        | -            |
| `(dccl.msg).codec_version` | int32        | Default codec set to use (corresponds to DCCL major version)                       |                   |        | -            |
| `(dccl.msg).codec`       | string         | Name of the codec to use for encoding the base message.                            |                   |        | dccl.defaultN, where N is codec_version |
| `(dccl.msg).codec_group` | string         | Group of codecs to be used for encoding the fields.                                |                   |        | dccl.defaultN |
| `(dccl.msg).unit_system` | string         | Unit system to use for all fields in this message (unless explicitly specified in the field definition.)                                |                   |        | "si" |
| **Field Extensions (core)** *[b]*   |           |                                         |                   |        |           |
| `(dccl.field).precision` | int32          | Decimal digits to preserve; can be negative.                                       | double, float, (u)intN *[c]* (negative values of precision only)    | \f$p\f$    | 0            |
| `(dccl.field).resolution` | double          |Defines the spacing between encoded values (generalized alternative to *precision*)  | double, float, (u)intN |  \f$dx = 10^{-p}\f$    | 1            |
| `(dccl.field).min`       | double         | Minimum value that this field can contain (inclusive). Should be an exact multiple of \f$dx = 10^{-p}\f$     | (u)intN , double, float | \f$x_m\f$ | -  |
| `(dccl.field).max`       | double         | Maximum value that this field can contain (inclusive). Should be an exact multiple of \f$dx = 10^{-p}\f$                              | (u)intN, double, float | \f$x_M\f$ | -  |
| `(dccl.field).max_length`| uint32         | Maximum length (in bytes) that can be encoded                                      | string, bytes     | \f$L_M\f$  | -            |
| `(dccl.field).min_repeat`| uint32         | Minimum number of repeated values.                                                 | all _repeated_    | \f$r_m\f$  | -            |
| `(dccl.field).max_repeat`| uint32         | Maximum number of repeated values.                                                 | all _repeated_    | \f$r_M\f$  | -            |
| `(dccl.field).codec`     | string         | Codec to use for this field (if omitted, the default is used). | all | - | - |
| `(dccl.field).units`     | Units     | Physical dimensions and units system information                                   | (u)intN, double, float | - | - |
| `(dccl.field).dynamic_conditions`     | Conditions     | Runtime defined field inclusion or bounds using Lua scripts                                  | all | - | - |
| **Field Extensions (special purpose)** *[b]*   |           |                                         |                   |        |             |
| `(dccl.field).omit`      | bool           | Do not include field in encoded message                          | all               | -      | False        |
| `(dccl.field).in_head`      | bool           | If true, the field is included in the DCCL header (encoded after the DCCL ID but before the rest of the fields. Not encrypted when using encryption)                          | all               | -      | False        |
| `(dccl.field).packed_enum`     | bool     | If false, encode use enum assigned values, not enum index values (0-N in order of definition)                                  | enum | - | True |
| `(dccl.field).static_value`     | string     | Statically defined value for StaticCodec (codec = "dccl.static")                                 | all | - | - |
| `(dccl.field).num_days`     | uint32     |  Number of days to include in TimeCodec  (codec = "dccl.time") encoding (+/- 12 hours of validity for each day added.)                                 | double, (u)int64 | - | 1 |

- *[a]*: Extensions of `google.protobuf.MessageOptions`
- *[b]*: Extensions of `google.protobuf.FieldOptions`
- *[c]*: (u)intN refers to any of the integer types: int32, int64, uint32, uint64, sint32, sint64, fixed32, fixed64, sfixed32, sfixed64

### DCCL ID: (dccl.msg).id

The DCCL ID is used to uniquely identify a given message name without having to encode the name in the message (encoding a number is much cheaper than a string). To interoperate with other groups, please see [http://gobysoft.org/wiki/DcclIdTable](http://gobysoft.org/wiki/DcclIdTable). For private work, please use IDs 124-127 (one-byte) and 128-255 (two-byte). 

### DCCL Maximum bytes: (dccl.msg).max_bytes

This value is the maximum message size before you get an error from DCCL. This is a design tool to help ensure messages do not exceed a desired value, typically the path maximum transmission unit (MTU). Messages that do not take the actual max_bytes size are encoded only as the size they take up (i.e. they are not padded to max_bytes).

### DCCL Codec Version: (dccl.msg).codec_version

This option sets the default codec version (which is not wire-compatibility between Goby/DCCL 2, DCCL 3, and DCCL 4). This should always be set to "4" when you are able to use DCCL v4 for all nodes that deploy this message, or an earlier version if required (e.g., if one of the nodes has access only to DCCL3, use "3").

### Precision versus Resolution

DCCL numeric fields originally were only defined using *precision* which is a negative power of 10 to which the message should be rounded. For example, a precision of 2 will round the message the hundredths place (\f$10^{-2}\f$), and a precision of -1 will round to the tens place (\f$10^1\f$). 

Now DCCL also provides a more generalized and intuitive option *resolution* that can be used instead of *precision*. *Resolution* doesn't have to be a power of 10, so for example, a resolution of 0.25 would round to the closest quarter of a value, and a resolution of 30 would round to the nearest multiple of 30 (-30, 0, 30, 60, etc.). When using *resolution*, the *min* and *max* values must be a multiple of the resolution. For example, if *resolution* is 0.25, *min* could be -0.5, 0.75, etc. but not 0.33 or -0.12 (and similarly for *max*).

Use whichever definition you prefer, and keep in mind that when using precision this is exactly equivalent to setting resolution as \f$10^-precision\f$ (e.g., *precision* of 2 is a *resolution* of 0.01).

## DCCL Static Units

Since the DCCL field bounds (min, max, and precision) are often based off the physical origins of the data, it is important to define the units of measure of those fields. The DCCL IDL has support for defining the units of a numeric field's quantity. When using the DCCL C++ library, this support is directly connected to the Boost Units C++ library: [Boost Units](http://www.boost.org/doc/html/boost_units.html). The units of a given field are given by two parameters: the physical dimension (e.g., length, force, mass, etc.), and the unit system which defaults to the International System of Units (SI). The units of the field can also be specified directly, outside of a canonical system (e.g., nautical mile, fathom, yard, knot, etc.).

The fields defined with units generate additional C++ methods using the DCCL plugin (_protoc-gen-dccl_) to the GPB compiler (_protoc_). The Debian package for the plugin is 
```bash
sudo apt-get install dccl4-compiler
```

These additional methods provide accessors and mutators for the dimensioned Boost Units quantities, with full static "unit safety", and correct conversions between different units of the same dimensions (e.g., feet to meters). Unit safety is defined as static (compiler-checked) dimensional analysis. The term is a blending of the (computer science) notion of type safety with (physical) dimensional analysis. For example, in a unit-safe system, the compiler will not allow the user to set a field with dimensions of length to a quantity of hours.

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

### DCCL Units Generated Code

The DCCL Units C++ generated accessors and mutators mirror those provided by the standard Google Protocol Buffers compiler for numeric fields, with the method name appended by the string "_with_units". For the standard Protobuf generated code see [Google Protocol Buffers Reference](https://developers.google.com/protocol-buffers/docs/reference/cpp-generated). DCCL Units are only valid on numeric fields (either singular or repeated). Two accessors are provided for convenience: a non-template accessor that returns the value as the Quantity (i.e., `boost::units::quantity`) defined in the DCCL field, and a template accessor that can take any valid Boost Units Quantity (i.e., a type with the same dimensions as the DCCL field) and return the value in that type, accounting for all conversion factors.

#### Singular Numeric Fields

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

#### Repeated Numeric Fields

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

### DCCL Units examples

Here are a few example DCCL messages which include unit specification:

#### AUVStatus

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

#### CTDMessage

See `test/dccl_units/ctd_message_units.proto`

#### CommandMessage 

See `test/dccl_units/command_message.proto`

## DCCL Dynamic Conditions (DCCL4+)

Dynamic Conditions is a new feature in DCCL4 that allows for conditional encoding of the fields based on runtime conditions (values of other parts of the message). This feature allows you to omit a field, mark a field "required", or change the values of the min/max bounds based on the value of one or more fields in the message.

Each dynamic condition is a string that contains a script written in Lua ([https://www.lua.org/](https://www.lua.org/)) that is evaluated each time the message is encoded or decoded.

The available dynamic_conditions are:

- **required_if**: The Lua script must return a boolean value: true means the field is now required (overriding the optional or required value in the .proto file).
- **omit_if**: The Lua script must return a boolean value: true means the field is omitted from the encoded message.
- **only_if**: A commonly used combination of required_if and omit_if. If true, this is the same as required_if returning true. If false, this is the same as omit_if returning true.
- **min**: The Lua script must return a numeric value setting the new minimum value. Note that this cannot be less than the hardcoded (dccl.field).min value (if it is, (dccl.field).min is used instead).
- **max**: The Lua script must return a numeric value setting the new maximum value. Note that this cannot be greater than the hardcoded (dccl.field).max value (if it is, (dccl.field).max is used instead).

### Special variables

Within the Lua script you are given access to some special variables set by DCCL:

- **this** (a Lua table acting as a struct) is the defined as the current contents of the innermost Message
- **root** (a Lua table) is the outermost message.
- **this_index** (a numeric, aka integer) is the index to the current repeated field element if this (sub)message is contained within a repeated field.

For example, given the following message:

```proto
import "dccl/option_extensions.proto";

message TestMsg
{
    option (dccl.msg) = {
        id: 2,
        max_bytes: 512,
        codec_version: 4
    };

    enum State
    {
        STATE_1 = 1;
        STATE_2 = 2;
        STATE_3 = 3;
    }

    required State state = 1;

    optional int32 a = 2 [(dccl.field) = {
        min: 0
        max: 200
        dynamic_conditions {
            required_if: "return this.state == 'STATE_1'"
            omit_if: "return this.state ~= 'STATE_1'"
        }
    }];
}
```

"this" and "root" refer to the contents of TestMsg.

However, given a different message:

```proto
import "dccl/option_extensions.proto";

message TestMsg
{
    option (dccl.msg) = {
        id: 2,
        max_bytes: 512,
        codec_version: 4
    };

    message Child
    {
        enum IncludeI
        {
            UNUSED = 0;
            YES = 1;
            NO = 2;
        }

        required IncludeI include_i = 1;
        optional int32 i = 2 [(dccl.field) = {
            min: 0
            max: 255
            dynamic_conditions { only_if: "return this.include_i == 'YES'" }

        }];
    }

    required Child child = 11;

}
```

Now within the context of field "i", "this" refers to Child, and "root" refers to TestMsg.

### Other notes

Since this is a common idiom and to reduce extra code, you can omit "return" if it is the first part of the Lua script. That is, these are identical:

```proto
dynamic_conditions { only_if: "return this.include_i == 'YES'" }
dynamic_conditions { only_if: "this.include_i == 'YES'" }
```

If your script doesn't begin with "return" you must put it in explicitly:

```proto
dynamic_conditions { omit_if: "a = 3; return a == this.field_c" }
```

The Lua Protobuf functionality uses this wonderful Github project: [https://github.com/starwing/lua-protobuf](https://github.com/starwing/lua-protobuf). Please reference the documentation in the event you need more details about the "this" or "root" tables, which are built using this library.

For more details, and an example usage, see the dccl_dynamic_conditions unit test.
