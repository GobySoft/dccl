#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dccl/codec.h>
#include <dccl/codecs2/field_codec_default.h>
#include <dccl/dynamic_protobuf_manager.h>
#include <google/protobuf/message.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <set>
#include <sstream>
#include <string>
#include <utility>

namespace py = pybind11;
namespace gp = google::protobuf;

namespace
{
py::object symbol_db()
{
    static py::object db =
        py::module_::import("google.protobuf.symbol_database").attr("Default")();
    return db;
}

std::string get_full_name_from_py(const py::object& obj)
{
    if (py::hasattr(obj, "DESCRIPTOR"))
        return py::str(obj.attr("DESCRIPTOR").attr("full_name"));
    if (py::hasattr(obj, "full_name"))
        return py::str(obj.attr("full_name"));
    throw py::type_error("Expected a protobuf message, descriptor, or full name string.");
}

const gp::Descriptor* descriptor_from_name(const std::string& name)
{
    const gp::Descriptor* desc = dccl::DynamicProtobufManager::find_descriptor(name);
    if (!desc)
        throw py::value_error("Could not find a type by that name.");
    return desc;
}

const gp::Descriptor* descriptor_from_py(const py::object& obj)
{
    if (py::isinstance<py::str>(obj))
        return descriptor_from_name(py::cast<std::string>(obj));
    return descriptor_from_name(get_full_name_from_py(obj));
}

std::string bytes_from_py(const py::object& obj)
{
    py::bytes b = py::bytes(obj);
    return b;
}

std::unique_ptr<gp::Message> py_to_cpp_msg(const py::object& py_msg)
{
    std::string name = get_full_name_from_py(py_msg);
    gp::Message* raw = nullptr;
    try
    {
        raw = dccl::DynamicProtobufManager::new_protobuf_message<gp::Message*>(name);
    }
    catch (const std::runtime_error&)
    {
        throw py::value_error("Could not convert to a known DCCL protobuf type.");
    }
    std::unique_ptr<gp::Message> msg(raw);

    py::bytes serialized = py_msg.attr("SerializeToString")();
    std::string encoded = serialized;
    if (!msg->ParseFromString(encoded))
        throw py::value_error("Failed to parse serialized protobuf message.");
    return msg;
}

py::object cpp_to_py_msg(const gp::Message& msg)
{
    py::object cls = symbol_db().attr("GetSymbol")(msg.GetTypeName());
    py::object py_msg = cls();
    std::string encoded;
    msg.SerializeToString(&encoded);
    py_msg.attr("ParseFromString")(py::bytes(encoded));
    return py_msg;
}

std::string info_for_descriptor(const dccl::Codec& codec, const gp::Descriptor* desc,
                                int user_id)
{
    std::ostringstream oss;
    codec.info(desc, &oss, user_id);
    return oss.str();
}

std::string info_all(const dccl::Codec& codec)
{
    std::ostringstream oss;
    codec.info_all(&oss);
    return oss.str();
}

std::set<unsigned> skip_ids_from_py(const py::object& obj)
{
    std::set<unsigned> skip_ids;
    if (obj.is_none())
        return skip_ids;

    for (const py::handle item : obj)
    {
        long value = py::cast<long>(item);
        if (value < 0)
            throw py::type_error("ids_to_skip cannot contain negative values.");
        skip_ids.insert(static_cast<unsigned>(value));
    }
    return skip_ids;
}

std::atomic<dccl::int64> g_time_epoch_usec{0};

struct PyTimeClock
{
    using time_point = std::chrono::time_point<std::chrono::system_clock>;
    static time_point now()
    {
        return time_point(std::chrono::microseconds(g_time_epoch_usec.load()));
    }
};

void set_time_clock_epoch_seconds(dccl::int64 seconds)
{
    g_time_epoch_usec.store(seconds * 1000000);
    dccl::v2::TimeCodecClock::set_clock<PyTimeClock>();
}

void set_time_clock_epoch_microseconds(dccl::int64 usec)
{
    g_time_epoch_usec.store(usec);
    dccl::v2::TimeCodecClock::set_clock<PyTimeClock>();
}

void use_system_clock() { dccl::v2::TimeCodecClock::set_clock<std::chrono::system_clock>(); }
} // namespace

PYBIND11_MODULE(_dccl, m)
{
    m.doc() = "DCCL bindings (pybind11)";

    dccl::DynamicProtobufManager::enable_compilation();

    py::register_exception<dccl::Exception>(m, "DcclException");
    py::register_exception<dccl::OutOfRangeException>(m, "OutOfRangeException",
                                                      PyExc_Exception);

    m.def("loadProtoFile", [](const std::string& filename) {
        dccl::DynamicProtobufManager::load_from_proto_file(filename);
    });
    m.def("addProtoIncludePath",
          [](const std::string& path) { dccl::DynamicProtobufManager::add_include_path(path); });

    m.def("set_time_clock_epoch_seconds", &set_time_clock_epoch_seconds,
          "Set the time codec clock using a fixed epoch time in seconds.");
    m.def("set_time_clock_epoch_microseconds", &set_time_clock_epoch_microseconds,
          "Set the time codec clock using a fixed epoch time in microseconds.");
    m.def("use_system_clock", &use_system_clock,
          "Reset the time codec clock to use std::chrono::system_clock.");

    py::class_<dccl::Codec> codec(m, "Codec", "The Dynamic CCL enCODer/DECoder.");
    codec.def(py::init<std::string, std::string>(),
              py::arg("id_codec") = dccl::Codec::default_id_codec_name(),
              py::arg("library_path") = std::string())
        .def("id",
             [](dccl::Codec& self, const py::object& obj) {
                 if (py::isinstance<py::str>(obj) || py::hasattr(obj, "DESCRIPTOR") ||
                     py::hasattr(obj, "full_name"))
                 {
                     const gp::Descriptor* desc = descriptor_from_py(obj);
                     return self.id(desc);
                 }
                 std::string bytes = bytes_from_py(obj);
                 return self.id(bytes);
             })
        .def("size",
             [](dccl::Codec& self, const py::object& py_msg) {
                 std::unique_ptr<gp::Message> msg = py_to_cpp_msg(py_msg);
                 return self.size(*msg);
             })
        .def("encode",
             [](dccl::Codec& self, const py::object& py_msg, bool header_only) {
                 std::unique_ptr<gp::Message> msg = py_to_cpp_msg(py_msg);
                 std::string bytes;
                 self.encode(&bytes, *msg, header_only);
                 return py::bytes(bytes);
             },
             py::arg("message"), py::arg("header_only") = false)
        .def("decode",
             [](dccl::Codec& self, const py::object& obj, bool header_only) {
                 std::string bytes = bytes_from_py(obj);
                 gp::Message* msg = self.decode<gp::Message*>(bytes, header_only);
                 std::unique_ptr<gp::Message> msg_guard(msg);
                 return cpp_to_py_msg(*msg);
             },
             py::arg("bytes"), py::arg("header_only") = false)
        .def("decode_into",
             [](dccl::Codec& self, const py::object& obj, const py::object& py_msg,
                bool header_only) {
                 std::string bytes = bytes_from_py(obj);
                 std::unique_ptr<gp::Message> msg = py_to_cpp_msg(py_msg);
                 self.decode(bytes, msg.get(), header_only);
                 std::string encoded;
                 msg->SerializeToString(&encoded);
                 py_msg.attr("ParseFromString")(py::bytes(encoded));
                 return py_msg;
             },
             py::arg("bytes"), py::arg("message"), py::arg("header_only") = false)
        .def("load",
             [](dccl::Codec& self, const py::object& obj) {
                 const gp::Descriptor* desc = descriptor_from_py(obj);
                 self.load(desc);
             })
        .def("load_hash",
             [](dccl::Codec& self, const py::object& obj) {
                 const gp::Descriptor* desc = descriptor_from_py(obj);
                 return self.load(desc);
             })
        .def("unload",
             [](dccl::Codec& self, const py::object& obj) {
                 if (py::isinstance<py::int_>(obj))
                 {
                     size_t dccl_id = py::cast<size_t>(obj);
                     self.unload(dccl_id);
                     return;
                 }
                 const gp::Descriptor* desc = descriptor_from_py(obj);
                 self.unload(desc);
             })
        .def("unload_all", [](dccl::Codec& self) { self.unload_all(); })
        .def("load_library",
             [](dccl::Codec& self, const std::string& path) { self.load_library(path); })
        .def("set_crypto_passphrase",
             [](dccl::Codec& self, const std::string& passphrase, const py::object& skip_obj) {
                 self.set_crypto_passphrase(passphrase, skip_ids_from_py(skip_obj));
             },
             py::arg("phrase"), py::arg("ids_to_skip") = py::none())
        .def("set_strict", [](dccl::Codec& self, bool enabled) { self.set_strict(enabled); })
        .def("set_console_width",
             [](dccl::Codec& self, unsigned width) { self.set_console_width(width); })
        .def("set_id_codec", [](dccl::Codec& self, const std::string& name) {
            self.set_id_codec(name);
        })
        .def("get_id_codec", [](dccl::Codec& self) { return self.get_id_codec(); })
        .def("info",
             [](dccl::Codec& self, const py::object& obj, int user_id) {
                 const gp::Descriptor* desc = descriptor_from_py(obj);
                 return info_for_descriptor(self, desc, user_id);
             },
             py::arg("type_or_message"), py::arg("user_id") = -1)
        .def("info_all", [](dccl::Codec& self) { return info_all(self); })
        .def("min_size",
             [](dccl::Codec& self, const py::object& obj) {
                 const gp::Descriptor* desc = descriptor_from_py(obj);
                 return self.min_size(desc);
             })
        .def("max_size",
             [](dccl::Codec& self, const py::object& obj) {
                 const gp::Descriptor* desc = descriptor_from_py(obj);
                 return self.max_size(desc);
             })
        .def("loaded", [](dccl::Codec& self) {
            py::dict out;
            for (const auto& entry : self.loaded())
                out[py::int_(entry.first)] = py::str(entry.second->full_name());
            return out;
        });
}
