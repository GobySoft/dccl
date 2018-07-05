#include <Python.h>
#include "structmember.h"

#include <dccl.h>
#include <google/protobuf/message.h>

#include <string>

namespace gp = google::protobuf;

static PyObject *GPBSymbolDB;
static PyObject *DcclException;

typedef struct {
    PyObject_HEAD
    dccl::Codec *codec;
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
    char *ch_full_name = PyString_AsString(py_full_name);
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
    } catch (dccl::Exception &e) {
        PyErr_SetString(DcclException, "Could not convert to a known DCCL protobuf type.");
        return 0;
    }
    
    // Now that we have the C++ type, serialize the python data, and populate the C++ object.
    PyObject *result = PyObject_CallMethod(pyMsg, "SerializeToString", NULL);
    if (!result || !PyString_Check(result)) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to Serialize python protobuf message.");
        delete msg;
        return 0;
    }
    msg->ParseFromArray(PyString_AsString(result), PyString_Size(result));

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
    PyObject_CallMethod(msg, "ParseFromString", "s#", encoded.c_str(), encoded.size());
    return msg;
}

// new, dealloc, initializers...
static PyObject *Codec_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    return type->tp_alloc(type, 0);
}

static void Codec_dealloc(Codec* self) {
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
    } catch (dccl::Exception &e) {
        PyErr_SetString(DcclException, e.what());
    }

    return 0;
}

// Get the ID for an encoded message
static PyObject *Codec_id(Codec *self, PyObject *args) {
    const char *bytes;
    int bytes_len;
    unsigned id;

    if (!PyArg_ParseTuple(args, "s#", &bytes, &bytes_len))
        return NULL;
    try {
        id = self->codec->id(std::string(bytes, bytes_len));
    } catch (dccl::Exception &e) {
        PyErr_SetString(DcclException, e.what());
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
    }
    delete msg;
    return Py_BuildValue("s#", bytes.c_str(), bytes.size());
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
    }
    delete msg;
    return Py_BuildValue("I", size);
}

static PyObject *Codec_decode(Codec *self, PyObject *args) {
    const char *bytes;
    int size = 0;
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
    {"load", (PyCFunction)Codec_load, METH_VARARGS,
     "load(type_name)\n\nEnsure that type_name is registered for use with DCCL."},
    {"load_library", (PyCFunction)Codec_load_library, METH_VARARGS,
     "load_library(path)\n\nLoad any codecs present in the given shared library name."}, /* Could make support ctypes handles as well... */
    {NULL}  /* Sentinel */
};

static PyMemberDef Codec_members[] = {
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


extern "C" {
// Initialize the Python Module
PyMODINIT_FUNC init_dccl(void) {
    PyObject *m;
    
    dccl_CodecType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dccl_CodecType) < 0)
        return;

    m = Py_InitModule3("_dccl", DcclMethods, "DCCL Bindings - C++ Module.");
    if (m == NULL)
        return;

    Py_INCREF(&dccl_CodecType);
    PyModule_AddObject(m, "Codec", (PyObject *)&dccl_CodecType);

    // Register a Python DCCL Exception    
    DcclException = PyErr_NewException("dccl.DcclException", NULL, NULL);
    Py_INCREF(DcclException);
    PyModule_AddObject(m, "DcclException", DcclException);
    
    // We're always going to need dynamic support to use this from Python, so enable it with DCCL
    // and get a reference to the default Symbol Database to facilitate type lookups.
    dccl::DynamicProtobufManager::enable_compilation();
    PyObject* GPBSymbolDBModule = PyImport_ImportModule("google.protobuf.symbol_database");
    if (GPBSymbolDBModule == NULL)
        return;
        
    GPBSymbolDB = PyObject_CallMethod(GPBSymbolDBModule, "Default", NULL);
    Py_DECREF(GPBSymbolDBModule);
    if (GPBSymbolDB == NULL)
        return;
}

}
