import os, dccl, navreport_pb2

dccl.addProtoIncludePath(os.path.abspath("."))
dccl.addProtoIncludePath(os.path.abspath("/path/to/dccl/include"))

dccl.loadProtoFile(os.path.abspath("./navreport.proto"))

codec = dccl.Codec()
codec.load("NavigationReport")

# SENDER
r_out = navreport_pb2.NavigationReport(x=450, y=550, z=-100, veh_class=navreport_pb2.NavigationReport.AUV, battery_ok=True)
encoded_bytes = codec.encode(r_out)

# send encoded_bytes across your link

# RECEIVER
decoded_msg = codec.decode(encoded_bytes)
print decoded_msg
