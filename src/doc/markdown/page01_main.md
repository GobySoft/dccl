# Dynamic Compact Control Language (DCCL)

![DCCL Image](dccl-50cols.png)

The Dynamic Compact Control Language (DCCL) is a language for marshalling (or roughly analogously: source encoding or compressing) object-based messages for extremely low throughput network links. Originally designed for commanding and retrieving data from autonomous underwater vehicles over acoustic modem links, DCCL has found additional uses in the robotics community (such as for sending messages over satellite or degraded land-based links). It is suitable for use when having a very small encoded message size is of much more importance than the speed of encoding and decoding these messages.

DCCL provides two main components:
1. An [interface descriptor language (IDL)](page02_idl.md) for defining messages based as an extension to Google Protocol Buffers (GPB).
2. A set of [built-in encoders and decoders (codecs)](page03_codecs.md) that operate on the messages defined in the DCCL IDL. 

In addition to the built-in codecs, further field codecs can be defined as extensions to the DCCL library to optimally encode specific sources of data. For example, two sets of these codecs are included with the core DCCL distribution as plugin shared libraries: an arithmetic encoder and a collection of REMUS CCL compatible codecs. DCCL can be thought of as an alternative encoder to the one that is included with the GPB library. DCCL will produce more compact messages than GPB, but at the cost of additional design and CPU time.

## Quick Start (Ubuntu or Debian)

1. Grab dependencies:
   ```shell
   echo "deb http://packages.gobysoft.org/ubuntu/release/ `lsb_release -c -s`/" | sudo tee /etc/apt/sources.list.d/gobysoft_release.list
   sudo apt-key adv --recv-key --keyserver hkp://keyserver.ubuntu.com:80 19478082E2F8D3FE
   sudo apt update
   sudo apt install libdccl4-dev
   ```

2. Find or create a plain GPB message (navreport.proto):
   ```protobuf
   syntax="proto2";
   message NavigationReport {
     required double x = 1;
     required double y = 2;
     required double z = 3;
     enum VehicleClass { AUV = 1; USV = 2; SHIP = 3; }
     optional VehicleClass veh_class = 4;
     optional bool battery_ok = 5;
   }
   ```

3. Turn it into a DCCL message. Create bounds on the message and give it a unique identifier number using DCCL extensions:
   ```protobuf
   syntax="proto2";
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

4. (Optional, requires dccl4-apps) Learn about the sizes of your messages fields using the 'dccl' tool:
   ```shell
   $ dccl --analyze -f navreport.proto
   ||||||| Dynamic Compact Control Language (DCCL) Codec |||||||
   1 messages loaded.
   Field sizes are in bits unless otherwise noted.
   =================== 124: NavigationReport ===================
   Actual maximum size of message: 8 bytes / 64 bits
           dccl.id head...........................8
           user head..............................0
           body..................................53
           padding to full byte...................3
   Allowed maximum size of message: 32 bytes / 256 bits
   --------------------------- Header ---------------------------
   dccl.id head...................................8 {dccl.default.id}
   ---------------------------- Body ----------------------------
   NavigationReport..............................53 {dccl.default4}
           1. x..................................18 {dccl.default4}
           2. y..................................18 {dccl.default4}
           3. z..................................13 {dccl.default4}
           4. veh_class...........................2 {dccl.default4}
           5. battery_ok..........................2 {dccl.default4}
   ```

At this point you can decide to use C++, Python, or the command line tool `dccl`.

### C++

1. Run protoc to generate navreport.pb.h and navreport.pb.cc C++ files from your navreport.proto file.
   ```shell
   protoc --cpp_out=. navreport.proto -I . -I /usr/include
   ```

2. Use the dccl::Codec in your C++ code to encode and decode messages (quick.cpp):
   ```cpp
   #include <iostream>

   #include "dccl.h"
   #include "navreport.pb.h"

   int main()
   {
       std::string encoded_bytes;
       dccl::Codec codec;
       codec.load<NavigationReport>();
       // SENDER
       {
           NavigationReport r_out;
           r_out.set_x(450);
           r_out.set_y(550);
           r_out.set_z(-100);
           r_out.set_veh_class(NavigationReport::AUV);
           r_out.set_battery_ok(true);
           
           codec.encode(&encoded_bytes, r_out);
       }
       // send encoded_bytes across your link

       // RECEIVER
       if(codec.id(encoded_bytes) == codec.id<NavigationReport>())
       {
           NavigationReport r_in;
           codec.decode(encoded_bytes, &r_in);
           std::cout << r_in.ShortDebugString() << std::endl;
       }    
   }
   ```

3. Compile it:
   ```shell
   g++ quick.cpp -o quick navreport.pb.cc -ldccl -lprotobuf 
   ```

4. Run it:
   ```shell
   $ ./quick                                                                   
   x: 450 y: 550 z: -100 veh_class: AUV battery_ok: true
   ```

### Python

1. Install the Python DCCL apt package:
   ```shell
  $ sudo apt install python3-dccl4
   ```

2. Compile the Python output of your DCCL message
  ```shell
  $ protoc --python_out=. navreport.proto -I . -I /usr/include
  ```

3. Create a Python script (quick.py):
  ```python
  import os, dccl, navreport_pb2

  dccl.loadProtoFile(os.path.abspath("./navreport.proto"))

  codec = dccl.Codec()
  codec.load("NavigationReport")

  # SENDER
  r_out = navreport_pb2.NavigationReport(x=450, y=550, z=-100, veh_class=navreport_pb2.NavigationReport.AUV, battery_ok=True)
  encoded_bytes = codec.encode(r_out)

  # send encoded_bytes across your link

  # RECEIVER
  decoded_msg = codec.decode(encoded_bytes)
  print(decoded_msg)
  ```

4. Run it:
  ```bash
  $ python3 quick.py
  x: 450.0
  y: 550.0
  z: -100.0
  veh_class: AUV
  battery_ok: true
  ```


### dccl Command Line tool

1. Install the `dccl` tool
   ```shell
   sudo apt install dccl4-apps
   ```
 
2.  Encode using the command line tool `dccl`
   ```shell
   $ echo "x: 450 y: 550 z: -100 veh_class: AUV battery_ok: true" | dccl --encode --proto_file navreport.proto > msg.txt && xxd msg.txt
   0000000: f834 9871 7046 3213                      .4.qpF2.
   $ cat msg.txt | dccl --decode -f navreport.proto --omit_prefix
   x: 450 y: 550 z: -100 veh_class: AUV battery_ok: true
   ```


## Code

DCCL is written in C++ and is available under the terms of the Lesser GNU Public License.

- Source code DVCS (Git): [https://github.com/GobySoft/dccl](https://github.com/GobySoft/dccl)
  ```bash
  git clone https://github.com/GobySoft/dccl.git
  ```
  To compile you will need to have CMake, Google Protocol Buffers, and the header-only Boost libraries. (Boost is  optional, but provides the Units functionality and supports non-C++17 compliant compilers). On Debian/Ubuntu systems, this would be
  ```bash
  sudo apt install cmake libboost-dev libprotobuf-dev libprotoc-dev protobuf-compiler
  cd dccl
  ./build.sh
  ```

- Compiled release binary packages for Ubuntu LTS and Debian stable/oldstable: [https://packages.gobysoft.org/ubuntu/release/](https://packages.gobysoft.org/ubuntu/release/)
  ```bash
  echo "deb http://packages.gobysoft.org/ubuntu/release/ `lsb_release -c -s`/" | sudo tee /etc/apt/sources.list.d/gobysoft_release.list
  sudo apt-key adv --recv-key --keyserver hkp://keyserver.ubuntu.com:80 19478082E2F8D3FE
  sudo apt update
  sudo apt install libdccl4-dev dccl4-apps dccl4-doc dccl4-apps dccl4-compiler
  # optionally
  sudo apt install python3-dccl4
  ```

- Compiled continuous (latest HEAD of the 4.0 branch) binary packages for Ubuntu / Debian: [https://packages.gobysoft.org/ubuntu/continuous/](https://packages.gobysoft.org/ubuntu/continuous/)
  ```bash
  echo "deb http://packages.gobysoft.org/ubuntu/continuous/ `lsb_release -c -s`/" | sudo tee /etc/apt/sources.list.d/gobysoft_release.list
  sudo apt-key adv --recv-key --keyserver hkp://keyserver.ubuntu.com:80 19478082E2F8D3FE
  sudo apt update
  sudo apt install libdccl4-dev dccl4-apps dccl4-doc dccl4-apps dccl4-compiler
  # optionally
  sudo apt install python3-dccl4
  ```

- Debian packaging files (for Debian or derivatives): 
  ```bash
  git clone https://github.com/GobySoft/dccl.git
  cd dccl
  git clone https://github.com/GobySoft/dccl-debian
  ```

## Reference

- [DCCL Interface Descriptor Language (IDL) ](page02_idl.md) - documents the extensions to the Google Protocol Buffers language that encompass the DCCL interface descriptor language.
- [DCCL Encoders/Decoders (codecs)](page03_codecs.md) - gives the default codecs and describes the DCCL encoding and decoding process.
- [OCEANS 2015 Conference Paper](http://gobysoft.org/dl/oceans2015_dccl.pdf) on *The Dynamic Compact Control Language Version 3* presented in Genova, Italy in May 2015.

## History
DCCL grew out of the Goby project ([https://goby.software](https://goby.software)), where versions 1.0 and 2.0 still reside (in the equivalent Goby version). Goby 2.1 is the first version of that project to use the standalone DCCL version 3. DCCL v3 messages are compatible with Goby/DCCL v2 when messages are defined with `codec_version = 2` (the default). Goby/DCCL v1 uses XML for the IDL definition language, which can be converted to DCCL v2 messages using the `dccl_xml_to_dccl_proto` tool included in Goby. DCCL v4 changes the wire protocol slightly to fix usability bugs in the string and bytes fields, and to add support for the `oneof` construct.

## Authors
DCCL is a collaboration of those in the [DCCL Developers group](https://github.com/orgs/GobySoft/teams/dccl-dev) and [other members of the open source community](https://github.com/GobySoft/dccl/graphs/contributors). The original author and lead developer is [Toby Schneider](https://github.com/tsaubergine) (GobySoft, LLC: [https://gobysoft.org](https://gobysoft.org)).
