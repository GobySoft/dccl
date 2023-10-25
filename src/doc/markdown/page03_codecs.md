# DCCL Encoders/Decoders (codecs)

DCCL messages are encoded and decoded using a set of "field codecs" that are responsible for encoding and decoding a given field. The library comes with a default implementation for all the Google Protocol Buffers (Protobuf) types. It is also possible to define custom field codecs to encode one or more fields of a given message (or even the entire message).

## Encoding algorithm

The following pseudo-code gives the process of encoding a DCCL message (using the dccl::v4::DefaultMessageCodec). Note this is not precisely how the actual C++ code works, but is rather given to explain the encoded message structure. Keep in mind that DCCL messages are always encoded and decoded from the least significant bit to the most significant bit.

```pseudo
function Encode(DCCL Message)
1: Initialize global Bits to an empty bitset
2: Encode the identifier by calling EncodeId with the value of dccl.msg.id
3: Encode the header by calling EncodeFields with all fields in the DCCL Message where in_head is true
5: Encode the body by calling in sequence
     EncodeOneof with all the oneofs defined in the DCCL Message, when present
     EncodeFields with all fields in the DCCL Message where in_head is false
6: Optionally encrypt the body using the header as a nonce
7: Convert Bits to a string of bytes and return this string
```

```pseudo
function EncodeId(dccl.msg.id)
1: Find the identifier codec (defaults to dccl::DefaultIdentifierCodec, otherwise specified in the dccl::Codec constructor)
2: Encode the value of dccl.msg.id using this codec and append result to Bits
```

```pseudo
function EncodeOneof(oneofs)
1: For each oneof in oneofs
2:   Encode the size of oneof (equal to the number of the possible cases, including the one where no field is set) as an unsigned integer and append result to Bits.
```

```pseudo
function EncodeFields(fields)
1: For each field in fields
2:   Find the correct FieldCodec by the name given to dccl::FieldCodecManager::add()
       If (dccl.field).codec is explicitly set use that name.
       Else if this is an embedded message type and (dccl.msg).codec is explicitly set in that message definition, then use that name.
       Else if (dccl.msg).codec_group is set in the root message, use that name.
       Else use the name "dccl.defaultN", where (dccl.msg).codec_version = N (set in the root message or defaults to 2)
3:   Encode field and append result to Bits. If field belongs to a oneof, it is encoded as a required field if and only if it is set. If field is an embedded Message, EncodeFields is called recursively with the fields of the embedded Message.
4: Append zero bits to Bits until the length of Bits is an integer number of bytes
```

Definitions:
1. **Byte**: exactly eight (8) bits
2. **Field**: a numbered field in the Google Protobuf Message
3. **dccl::Bitset**: A set of bits without byte boundaries. The **front** is the least significant bit, and the **back** is the most significant bit. Thus, appending to a Bitset means add these bits to the most significant bit.


## Example Encoding

The following DCCL Message "CommandMessage" illustrates a basic command's DCCL definition, showing the components of the encoded message. Note: LSB = least significant byte, MSB = most significant byte.

![CommandMessage Example](codecs-ex-msg.png)

An example of encoding the DCCL "CommandMessage" for a set of representative values is provided. The table displays the unencoded \f$x\f$ and encoded \f$x_{enc}\f$ values, according to the formulas in the [Default Field Codec Reference](#codecs-math). Below the table, the encoded message is shown in little endian format, in both hexadecimal and binary notations.

![Encoded CommandMessage](codecs-ex-enc.png)


## Default Field Codec Reference

You certainly don't need to know how the fields are encoded to use DCCL, but this may be of interest to those looking to optimize their usage of DCCL or to implement custom encoders. First we will casually introduce the default encoders, then you can reference "Table 2" below for precise details.

Remember that DCCL messages are always encoded and decoded from the least significant bit to the most significant bit.

### Default Identifier Codec

Each DCCL message must have a unique numeric ID (defined using (dccl.msg).id = N). To interoperate with other groups, please see [DcclIdTable](http://gobysoft.org/wiki/DcclIdTable). For private work, please use IDs 124-127 (one-byte) and 128-255 (two-byte).

This ID is used in lieu of the DCCL message name on the wire. It is encoded using a one- or two-byte value, allowing for a larger set of values to be used than a single byte would allow, but still preserving a set of one-byte identifiers for frequently sent messages. This is always the first 8 or 16 bits of the message so that the dccl::Codec knows which message to decode.

The first (least significant) bit of the ID determines if it is a one- or two-byte identifier. If the first bit is true, it's a two-byte identifier and the next 15 bits give the actual ID. If the first bit is false, the next 7 bits give the ID.

Two examples:
1. One byte ID [0-128): (dccl.msg).id = 124. Decimal 124 is binary 1111100, which is appended to a single 0 bit to indicate a one-byte ID. Thus, the ID on the wire is 11111000 (or 124*2 = 248).
2. Two byte ID [128-32768): (dccl.msg).id = 240. Decimal 240 is binary 0000000 11110000. This is appended to a single 1 bit to indicate a two-byte ID, giving an encoded ID of 00000001 11100001 (or 240*2+1 = 481).

### Default Numeric Codec

Numeric values are all encoded essentially the same way. Integers are treated as floating point values with zero precision. Precision is defined as the number of decimal places to preserve (e.g. precision = 3 means round to the closest thousandth, precision = -1 means round to the closest tens). Thus, integer fields can also have negative precision, if desired. Fields are bounded by a minimum and maximum allowable value, based on the underlying source of the data.

To encode, the numeric value is rounded to the desired precision, and then multiplied by the appropriate power of ten to make it an integer. Then it is increased or decreased so that zero (0) represents the minimum encodable value. At this point, it is simply an unsigned integer. To encode the optional field's "not set", an additional value (not an additional bit) is reserved. To allow "not set" to be the zero (0) encoded value, all other values are incremented by one.

This default encoder assumes unset fields are rare. If you commonly have unset optional fields, you may want to implement a "presence bit" encoder that uses a separate bit to indicate if a field is set or not. These are two extremes of the more general purpose idea of an entropy encoder, such as the arithmetic encoder. In that case, "not set" is simply another symbol that has a probability mass relative to the actual values to capture the frequency with which fields are set or not set.

For example:
```
required double x = 1 [(dccl.field) = { min: -10000 max: 10000 precision: 1 }];
```

The field takes 18 bits: \f$\lceil \log_2(10000-(-10000) \cdot 10^1 + 1) \rceil = \lceil 17.61 \rceil = 18\f$.

Say we wanted to encode the value 10.56:
- Round to tenths: 10.6
- Subtract the minimum value: 10.6 - (-10000) = 10010.6
- Multiply by precision orders of magnitude: 10010.6*10 = 100106 = binary 01 10000111 00001010

### Default Enumeration Codec

Enumerations are treated like unsigned integers, where the enumeration keys are given values based on the order they are declared (not the value given in the .proto file).

For example:
```
enum VehicleClass { AUV = 10; USV = 5; SHIP = 3; }
optional VehicleClass veh_class = 4;
```

In this case (for encoding): AUV is 0, USV is 1, SHIP is 2. After this mapping, the field is encoded exactly like an equivalent integer field (with max = 2, min = 0 in this case).

### Default Boolean Codec

Booleans are simple. If they are required, they are encoded with false = 0, true = 1. If they are optional, they are tribools with "not set" = 0, false = 1, true = 2.

### Default String Codec

Strings are given a maximum size in the proto file (max_length). A small integer (minimally sized like a required unsigned int field to encode 0 to max_length) is included first to specify the length of the following string. Then the string is encoded using the basic one-byte character values (ASCII).

For example:
```
optional string message = 4 [(dccl.field).max_length = 10];
```

Say we want to encode "HELLO":
- The string is length 5, so we insert 4 bits (\f$\lceil \log_2(10 + 1) \rceil = 4\f$) with value 5: 0101
- Following, we add the ASCII values: 01001111 01001100 01001100 01000101 01001000
- The full encoded value is thus 0100 11110100 11000100 11000100 01010100 1000

### Default Bytes Codec

Like the string codec, but not variable length. It always takes max_length bytes in the message, and if it is optional, a presence bit is added at the front. If the presence bit is false, the bytes are omitted.

### Default Message Codec

Sub-messages are encoded recursively. In the case of an optional message, a presence bit is added before the message fields. If the presence bit is false (indicating the message is not set), no further bits are used.

### Mathematical formulas for Default Field Codecs

See "Table 1" in the IDL for symbol definitions. The formulas below in Table 2 refer to DCCLv3 defaults (i.e., codec_version = 3 which is equivalent to codec = "dccl.default3"). A few things that may make it easier to read this table:
- Left bit-shifting by N bits (\f$x << N\f$) is equivalent to multiplying by powers of two to the N (\f$x \cdot 2^N\f$).
- Right bit-shifting by N bits (\f$x >> N\f$) is equivalent to dividing by powers of two to the N (\f$x / 2^N\f$).
- \f$\lceil x \rceil\f$ is the ceiling function, rounding up to the nearest integer.
- \f$\lfloor x \rfloor\f$ is the floor function, rounding down to the nearest integer.

| Field Type       | Encoding Bit Length Calculation | Example                                                                                                                      |
|------------------|---------------------------------|------------------------------------------------------------------------------------------------------------------------------|
| Identifier       | \f$\lceil \log_2(N + 1) \rceil\f$ if \f$N < 128\f$, \f$\lceil \log_2(N + 1) \rceil + 1\f$ if \f$N \geq 128\f$ | For ID 240, use \f$\lceil \log_2(240 + 1) \rceil + 1 = \lceil 8 \rceil + 1 = 9\f$ bits                                         |
| Numeric          | \f$\lceil \log_2((\text{max} - \text{min}) \cdot 10^{\text{precision}} + 2) \rceil\f$                  | For a field with min = -10000, max = 10000, and precision = 1, use \f$\lceil \log_2(20000 \cdot 10 + 2) \rceil = 18\f$ bits    |
| Enumeration      | \f$\lceil \log_2(\text{number of enum keys} + 1) \rceil\f$                                           | For an enum with keys: AUV, USV, SHIP, use \f$\lceil \log_2(3 + 1) \rceil = 2\f$ bits                                          |
| Boolean          | 1 bit for required, \f$\lceil \log_2(3) \rceil\f$ = 2 bits for optional                             | Required boolean: 1 bit, Optional tribool: 2 bits                                                                           |
| String           | \f$\lceil \log_2(\text{max_length} + 1) \rceil + 8 \cdot \text{actual length}\f$                     | For max_length = 10 and "HELLO": \f$\lceil \log_2(11) \rceil + 8 \cdot 5 = 4 + 40 = 44\f$ bits                                 |
| Bytes            | \f$\text{max_length} \cdot 8\f$ + 1 if optional                                                     | For max_length = 10: \f$10 \cdot 8 = 80\f$ bits, 81 if optional                                                                |
| Message          | Sum of bit lengths of all fields + 1 if optional                                                  | For a message with 3 numeric fields (each 18 bits) and 1 optional boolean: \f$3 \cdot 18 + 2 = 56\f$ bits, 57 if optional      |


![Codecs Table](codecs-table.png)

## Custom Field Codecs

To define your own codecs:
1. Subclass one of the base classes  	dccl::TypedFixedFieldCodec,  	dccl::TypedFieldCodec or  	dccl::RepeatedTypedFieldCodec
2. Add the class into the `dccl::FieldCodecManager` with a given string name
3. Set a given field's `(dccl.field).codec` to the name given to `dccl::FieldCodecManager`, or the `(dccl.msg).codec` to change the entire message's field codec.

See also:
- `dccl_custom_message/test.proto`
- `dccl_custom_message/test.cpp`
- `dccl_codec_group/test.proto`
- `dccl_codec_group/test.cpp`

