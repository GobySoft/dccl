#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include "structmember.h"
#include "bytesobject.h"
#include <dccl.h>
#include <google/protobuf/message.h>

#include <string>

#if PY_MAJOR_VERSION >= 3
#define PyString_AS_STRING PyUnicode_AsUTF8
#endif

namespace gp = google::protobuf;

static PyObject *GPBSymbolDB;
static PyObject *DcclException;
static PyObject *DcclOutOfRangeException;

typedef struct {
    PyObject_HEAD
    dccl::Codec *codec;
    PyObject *codec_capsule;
} Codec;

static int py_pbmsg_to_cpp_pbmsg(PyObject *pyMsg, gp::Message **cppMsg) {
    // Get typename from the descriptor -- pyMsg.DESCRIPTOR.full_name -- and put in a string.
    PyObject *descriptor = PyObject_GetAttrString(pyMsg, "DESCRIPTOR");
    if (!descriptor) {
        PyErr_SetString(PyExc_TypeError, "Message had no DESCRIPTOR attribute.");
        return 0;
    }
    PyObject *py_full_name = PyObject_GetAttrString(descriptor, "full_name");
    Py_DECREF(descriptor);
    if (!py_full_name) {
        PyErr_SetString(PyExc_TypeError, "Message DESCRIPTOR had no full name.");
        return 0;
    }

    const char *ch_full_name = PyString_AS_STRING(py_full_name);
    if (!ch_full_name) {
        PyErr_SetString(PyExc_TypeError, "Message full_name was not a string.");
        return 0;
    }

    std::string full_name(ch_full_name);
    Py_DECREF(py_full_name);
    if (full_name.empty()) {
        PyErr_SetString(PyExc_TypeError, "Message full_name was not a string.");
        return 0;
    }

    // Now try to construct a C++ message with that name
    gp::Message *msg;
    try {
        msg = dccl::DynamicProtobufManager::new_protobuf_message<gp::Message*>(full_name);
    } catch (std::runtime_error &e) {
        // new_protobuf_message() throws a runtime_error instead of dccl::Exception
        PyErr_SetString(DcclException, "Could not convert to a known DCCL protobuf type.");
        return 0;
    } catch (...) {
        PyErr_SetString(DcclException, "Unexpected exception");
        return 0;
    }

    // Now that we have the C++ type, serialize the python data, and populate the C++ object.
    PyObject *result = PyObject_CallMethod(pyMsg, "SerializeToString", NULL);
    if (!result) {
        PyObject *ptype;
        PyObject *pvalue;
        PyObject *ptraceback;
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        PyErr_SetString(PyExc_RuntimeError, PyString_AS_STRING(PyObject_Repr(pvalue)));
        delete msg;
        return 0;
    } else if (!PyBytes_Check(result)) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to Serialize python protobuf message.");
        delete msg;
        return 0;
    }
    msg->ParseFromArray(PyBytes_AsString(result), PyBytes_Size(result));

    // If we made it here we were successful, and can set the pointer.
    *cppMsg = msg;
    return 1;
}

static PyObject* cpp_pbmsg_to_py_pbmsg(gp::Message *cppMsg) {
    // Create a Protobuf Message by looking up the Python prototype, and calling it to get a message
    PyObject *cls = PyObject_CallMethod(GPBSymbolDB, "GetSymbol", "s",
                                        cppMsg->GetTypeName().c_str());
    if (!cls) return NULL;
    PyObject *msg = PyObject_CallObject(cls, NULL);
    Py_DECREF(cls);
    if (!msg) return NULL;

    // Populate the python object from the C++ message
    std::string encoded;
    cppMsg->SerializeToString(&encoded);
#if PY_MAJOR_VERSION >= 3
    PyObject_CallMethod(msg, "ParseFromString", "y#", encoded.c_str(), encoded.size());
#else
    PyObject_CallMethod(msg, "ParseFromString", "s#", encoded.c_str(), encoded.size());
#endif

    return msg;
}

// new, dealloc, initializers...
static PyObject *Codec_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    return type->tp_alloc(type, 0);
}

static void Codec_dealloc(Codec* self) {
    Py_XDECREF(self->codec_capsule);
    if (self->codec) { delete self->codec; }
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static int Codec_init(Codec *self, PyObject *args, PyObject *kwds) {
    const char *id_codec=NULL, *library_path=NULL;
    static char *kwlist[] = {"id_codec", "library_path", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|zz", kwlist,
                                      &id_codec, &library_path)) { return -1; }

    std::string id_codec_str = id_codec ? id_codec : dccl::Codec::default_id_codec_name();
    std::string library_path_str = library_path ? library_path : "";

    try {
        self->codec = new dccl::Codec(id_codec_str, library_path_str);
        self->codec_capsule = PyCapsule_New(self->codec, "_dccl.Codec._CODEC", NULL);
    } catch (dccl::Exception &e) {
        PyErr_SetString(DcclException, e.what());
        return -1;
    } catch (...) {
        PyErr_SetString(DcclException, "unexpected exception");
        return -1;
    }

    return 0;
}

// Get the ID for an encoded message, or a message descriptor as a string (not the raw DESCRIPTOR object).
static PyObject *Codec_id(Codec *self, PyObject *args) {
    const char *bytes;
    Py_ssize_t bytes_len;
    unsigned id;

    if (!PyArg_ParseTuple(args, "s#", &bytes, &bytes_len))
        return NULL;
    try {
        std::string bytes_as_string = std::string(bytes, bytes_len);
        const gp::Descriptor* desc = dccl::DynamicProtobufManager::find_descriptor(bytes_as_string);
        if (!desc) {
            id = self->codec->id(bytes_as_string);
        }
        else {
            id = self->codec->id(desc);
        }
    } catch (dccl::Exception &e) {
        PyErr_SetString(DcclException, e.what());
        return NULL;
    } catch (...) {
        PyErr_SetString(DcclException, "unexpected exception");
        return NULL;
    }

    return Py_BuildValue("I", id);
}

static PyObject *Codec_encode(Codec *self, PyObject *args) {
    std::string bytes;
    gp::Message *msg = NULL;
    int header_only = 0;

    // Parse and convert the input into a gp::Message
    if (!PyArg_ParseTuple(args, "O&|i", &py_pbmsg_to_cpp_pbmsg, &msg, &header_only))
        return NULL;

    // Do the DCCL Encoding, and return the value as a string.
    try {
        self->codec->encode(&bytes, *msg, header_only != 0);
    } catch (dccl::Exception &e) {
        PyErr_SetString(DcclException, e.what());
        delete msg;
        return NULL;
    } catch (dccl::OutOfRangeException &e) {
        PyErr_SetString(DcclOutOfRangeException, e.what());
        delete msg;
        return NULL;
    } catch (...) {
        PyErr_SetString(DcclException, "unexpected exception");
        delete msg;
        return NULL;
    }
    delete msg;
#if PY_MAJOR_VERSION >= 3
    return Py_BuildValue("y#", bytes.c_str(), bytes.size());
#else
    return Py_BuildValue("s#", bytes.c_str(), bytes.size());
#endif

}

static PyObject *Codec_size(Codec *self, PyObject *args) {
    unsigned size = 0;
    gp::Message *msg = NULL;

    // Parse and convert the input into a gp::Message
    if (!PyArg_ParseTuple(args, "O&", &py_pbmsg_to_cpp_pbmsg, &msg))
        return NULL;

    // Do the DCCL Encoding, and return the value as a string.
    try {
        size = self->codec->size(*msg);
    } catch (dccl::Exception &e) {
        PyErr_SetString(DcclException, e.what());
        delete msg;
        return NULL;
    } catch (...) {
        PyErr_SetString(DcclException, "unexpected exception");
        delete msg;
        return NULL;
    }
    delete msg;
    return Py_BuildValue("I", size);
}

static PyObject *Codec_decode(Codec *self, PyObject *args) {
    const char *bytes;
    Py_ssize_t size = 0;
    int header_only = 0;

    // Parse inputs and convert to string
    if (!PyArg_ParseTuple(args, "s#|i", &bytes, &size, &header_only))
        return NULL;
    std::string bytestr(bytes, size);

    // Do DCCL Decoding, and get a gp::Message
    gp::Message *msg;
    try {
        msg = self->codec->decode<gp::Message*>(bytestr, header_only != 0);
    } catch (dccl::Exception &e) {
        PyErr_SetString(DcclException, e.what());
        return NULL;
    }

    // Convert the gp::Message to a Python Protobuf Message
    PyObject* pyMsg = cpp_pbmsg_to_py_pbmsg(msg);
    delete msg;
    return pyMsg;
}

static PyObject *Codec_decode_by_full_name(Codec *self, PyObject *args) {
    const char *bytes;
    Py_ssize_t size = 0;
    const char *full_name;
    int header_only = 0;

    // Parse inputs and convert to string
    if (!PyArg_ParseTuple(args, "s#s|i", &bytes, &size, &full_name, &header_only)) {
        return NULL;
    }
    std::string bytestr(bytes, size);

    // we need to map the full name to the class, via the type id
    // this isn't predictable for omit_id types which are assigned a negative ID on loading
    int expected_id = INT_MAX;
    for (auto &[k, v] : self->codec->loaded()) {
        if (v->full_name() == full_name) {
            expected_id = k;
        }
    }

    if (expected_id == INT_MAX) {
        throw(dccl::Exception("Provided message full name " + std::string(full_name) +
                                    " has not been loaded. Call load() before decoding this type."));
    } else {
        std::cout << "Found description.\n";
    }

    gp::Message *msg;
    try {
        if (!self->codec->loaded().count(expected_id)) {
            std::cout << expected_id << " not found.\n";
            for (auto &[k, v] : self->codec->loaded()) {
                std::cout << k << ":\t" << v->full_name() << "\n";
            }

            throw(dccl::Exception("Provided message id " + std::to_string(expected_id) +
                                    " has not been loaded. Call load() before decoding this type."));
        }

        const google::protobuf::Descriptor* desc = self->codec->loaded().at(expected_id);
        std::cout << desc->DebugString() << std::endl;

        if (!desc->options().GetExtension(dccl::msg).omit_id()) {
            int received_id = self->codec->id(bytestr);
            // this message should have the ID embedded, so check it matches
            if (!self->codec->loaded().count(received_id)) {
                throw(dccl::Exception("Parsed message id " + std::to_string(received_id) +
                                        " has not been loaded. Call load() before decoding this type."));
            }

            if (expected_id != received_id)
                throw(dccl::Exception("Received message with id " + std::to_string(received_id) + " (" +
                                self->codec->loaded().at(received_id)->full_name() +
                                ") but decode was called with message of id " +
                                std::to_string(expected_id) + " (" + desc->full_name() +
                                "). Ensure dccl::Codec::decode is called with the correct Protobuf "
                                "message or use the dynamic overloads of decode."));
        }

        // Do DCCL Decoding, and get a gp::Message
        msg = dccl::DynamicProtobufManager::new_protobuf_message<gp::Message*>(self->codec->loaded().find(expected_id)->second);
        self->codec->decode(bytestr, msg, header_only);
    } catch (dccl::Exception &e) {
        PyErr_SetString(DcclException, e.what());
        return NULL;
    }

    // Convert the gp::Message to a Python Protobuf Message
    PyObject* pyMsg = cpp_pbmsg_to_py_pbmsg(msg);
    delete msg;
    return pyMsg;
}

static PyObject *Codec_load(Codec *self, PyObject *args) {
    // Get the type name as a string
    const char *type_name_ch = NULL;
    if (!PyArg_ParseTuple(args, "s", &type_name_ch))
        return NULL;
    std::string type_name(type_name_ch);
    // Find the descriptor for that codec by name, and then feed it to codec->load.
    const gp::Descriptor* desc = dccl::DynamicProtobufManager::find_descriptor(type_name);
    if (!desc) {
        PyErr_SetString(PyExc_LookupError, "Could not find a type by that name.");
        return NULL;
    }
    try {
        self->codec->load(desc);
    } catch (dccl::Exception &e) {
        PyErr_SetString(DcclException, e.what());
        return NULL;
    } catch (...) {
        PyErr_SetString(DcclException, "unexpected exception");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *Codec_load_library(Codec *self, PyObject *args) {
    // Get the path as a string
    const char *path_ch = NULL;
    if (!PyArg_ParseTuple(args, "s", &path_ch))
        return NULL;
    std::string path(path_ch);
    try {
        self->codec->load_library(path);
    } catch (dccl::Exception &e) {
        PyErr_SetString(DcclException, e.what());
        return NULL;
    } catch (...) {
        PyErr_SetString(DcclException, "unexpected exception");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *Codec_set_crypto_passphrase(Codec *self, PyObject *args) {
    std::set<unsigned> skip_set;
    PyObject *skip_obj = NULL;
    const char *passphrase_ch = NULL;
    if (!PyArg_ParseTuple(args, "s|O", &passphrase_ch, &skip_obj))
        return NULL;
    // Convert the passphrase to a C++ string
    std::string passphrase(passphrase_ch);

    // Now populate the skip_set, if we were passed an iterable
    if (skip_obj != NULL) {
        PyObject *iterator = PyObject_GetIter(skip_obj);
        if (iterator == NULL) {
            PyErr_SetString(PyExc_TypeError,"ids_to_skip is not an iterable!");
            return NULL;
        }

        while (PyObject *item = PyIter_Next(iterator)) {
           long value = PyLong_AsLong(item);
           Py_DECREF(item);
           if (PyErr_Occurred()) {
               break;
           } else if (value < 0) {
               PyErr_SetString(PyExc_TypeError, "ids_to_skip cannot contain negative values");
               break;
           } else {
               skip_set.insert((unsigned)value);
           }
        }
        Py_DECREF(iterator);

        if (PyErr_Occurred()) {
            return NULL;
        }
    }

    try {
        self->codec->set_crypto_passphrase(passphrase, skip_set);
    } catch (dccl::Exception &e) {
        PyErr_SetString(DcclException, e.what());
        return NULL;
    } catch (...) {
        PyErr_SetString(DcclException, "unexpected exception");
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject *Codec_set_strict(Codec *self, PyObject *args) {
    int enabled = 0;
    if (!PyArg_ParseTuple(args, "i", &enabled))
        return NULL;
    try {
        // dccl::Codec instantiates with strict=false, so behaviour should be safe if the parsing above fails.
        self->codec->set_strict(enabled);
    } catch (dccl::Exception &e) {
        PyErr_SetString(DcclException, e.what());
        return NULL;
    } catch (...) {
        PyErr_SetString(DcclException, "unexpected exception");
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyMethodDef Codec_methods[] = {
    {"id", (PyCFunction)Codec_id, METH_VARARGS,
     "id(bytes)\n\nReturn the ID for a string or message."},
    {"size", (PyCFunction)Codec_size, METH_VARARGS,
     "size(message)\n\nProvide the encoded size (in bytes) of message."},
    {"encode", (PyCFunction)Codec_encode, METH_VARARGS,
     "encode(message[, header_only])\n\nReturn a DCCL-encoded string for message."},
    {"decode", (PyCFunction)Codec_decode, METH_VARARGS,
     "decode(bytes[, header_only])\n\nReturn a protobuf message decoded from bytes."},
    {"decode_with_full_name", (PyCFunction)Codec_decode_by_full_name, METH_VARARGS,
     "decode_with_full_name(bytes, full_name[, header_only])\n\nReturn a protobuf message decoded from bytes using type full name."},
    {"load", (PyCFunction)Codec_load, METH_VARARGS,
     "load(type_name)\n\nEnsure that type_name is registered for use with DCCL."},
    {"load_library", (PyCFunction)Codec_load_library, METH_VARARGS,
     "load_library(path)\n\nLoad any codecs present in the given shared library name."}, /* Could make support ctypes handles as well... */
    {"set_crypto_passphrase", (PyCFunction)Codec_set_crypto_passphrase, METH_VARARGS,
     "set_crypto_passphrase(phrase[, ids_to_skip])\n\nEnable encryption/decryption, except for ids_to_skip."},
    {"set_strict", (PyCFunction)Codec_set_strict, METH_VARARGS,
     "set_strict(enabled)\n\nDisable/Enable DCCL strict mode for boundary checking on encode."},
    {NULL}  /* Sentinel */
};

static PyMemberDef Codec_members[] = {
    {"_CODEC", T_OBJECT_EX, offsetof(Codec, codec_capsule), 1,
     "PyCapsule around pointer to underlying dccl::Codec*."},
    {NULL}  /* Sentinel */
};

static const char* Codec_doc = "The Dynamic CCL enCODer/DECoder\n\n __init__([id_codec, library])\n\
 Construct a Codec, optionally providing the name of an id_codec, and a path to a C library\
 that should be dynamically loaded.";

static PyTypeObject dccl_CodecType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "dccl.Codec",              /* tp_name */
    sizeof(Codec),             /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)Codec_dealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_compare */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    Codec_doc,                 /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    Codec_methods,             /* tp_methods */
    Codec_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Codec_init,      /* tp_init */
    0,                         /* tp_alloc */
    Codec_new,                 /* tp_new */
};


// DynamicProtobufManager has no real use on the Python side, since Python messages aren't
// easily converted from the c++ objects directly.  Rather than wrap the class, just have the
// two critical calls exposed at the module level.
static PyObject *dccl_addProtoIncludePath(PyObject *self, PyObject *args) {
    const char *path;

    if (!PyArg_ParseTuple(args, "s", &path))
        return NULL;
    std::string pathstr = path;
    dccl::DynamicProtobufManager::add_include_path(pathstr);
    Py_RETURN_NONE;
}

static PyObject *dccl_loadProtoFile(PyObject *self, PyObject *args) {
    const char *filename;

    if (!PyArg_ParseTuple(args, "s", &filename))
        return NULL;
    std::string filenamestr = filename;
    try {
        dccl::DynamicProtobufManager::load_from_proto_file(filenamestr);
    } catch (dccl::Exception &e) {
        PyErr_SetString(DcclException, e.what());
        return NULL;
    } catch (...) {
        PyErr_SetString(DcclException, "unexpected exception");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyMethodDef DcclMethods[] = {
    {"loadProtoFile", (PyCFunction)dccl_loadProtoFile, METH_VARARGS,
     "Load the types in a specific protobuf file (.proto).  The path *MUST* be absolute."},
    {"addProtoIncludePath", (PyCFunction)dccl_addProtoIncludePath, METH_VARARGS,
     "Adds a path to a collection of protobuf files (.proto)."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};


#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef DcclModule = {
        PyModuleDef_HEAD_INIT,
        "_dccl",
        "DCCL Bindings - C++ Module.",
        -1,
        DcclMethods,
        NULL,
        NULL,
        NULL,
        NULL
};

#define INITERROR return NULL

PyMODINIT_FUNC
PyInit__dccl(void)

#else
#define INITERROR return

extern "C"
void
init_dccl(void)
#endif
{

  dccl_CodecType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&dccl_CodecType) < 0) {
    INITERROR;
  }

#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&DcclModule);
#else
    PyObject *module = Py_InitModule3("_dccl", DcclMethods, "DCCL Bindings - C++ Module.");
#endif

    if (module == NULL) {
        INITERROR;
    }

    Py_INCREF(&dccl_CodecType);
    PyModule_AddObject(module, "Codec", (PyObject *)&dccl_CodecType);

    // Register a Python DCCL Exception
    DcclException = PyErr_NewException("dccl.DcclException", NULL, NULL);
    Py_INCREF(DcclException);
    PyModule_AddObject(module, "DcclException", DcclException);

    DcclOutOfRangeException = PyErr_NewException("dccl.OutOfRangeException", NULL, NULL);
    Py_INCREF(DcclOutOfRangeException);
    PyModule_AddObject(module, "DcclOutOfRangeException", DcclOutOfRangeException);

    // We're always going to need dynamic support to use this from Python, so enable it with DCCL
    // and get a reference to the default Symbol Database to facilitate type lookups.
    dccl::DynamicProtobufManager::enable_compilation();
    PyObject* GPBSymbolDBModule = PyImport_ImportModule("google.protobuf.symbol_database");
    if (GPBSymbolDBModule == NULL) {
        Py_DECREF(module);
        INITERROR;
    }

    GPBSymbolDB = PyObject_CallMethod(GPBSymbolDBModule, "Default", NULL);
    Py_DECREF(GPBSymbolDBModule);
    if (GPBSymbolDB == NULL) {
        Py_DECREF(module);
        INITERROR;
    }


#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}

