#include <Python.h>
#include "structmember.h"

#include <dccl.h>
#include <google/protobuf/message.h>

#include <string>

namespace gp = google::protobuf;

static PyObject *GPBMessageModule;
static PyObject *DcclException;

typedef struct {
    PyObject_HEAD
    dccl::Codec *codec;
} Codec;

static void Codec_dealloc(Codec* self) {
    if (self->codec) { delete self->codec; }
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *Codec_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    return type->tp_alloc(type, 0);
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

static PyObject *Codec_id(Codec *self, PyObject *args) {
    const char *bytes;
    int bytes_len;
    unsigned id;

    if (!PyArg_ParseTuple(args, "s#", &bytes, &bytes_len))
        return NULL;
    id = self->codec->id(std::string(bytes, bytes_len));
    return Py_BuildValue("I", id);
}

static int python_pbmsg_to_cpp_pbmsg(PyObject *pyMsg, gp::Message **cppMsg) {
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
    char *full_name = PyString_AsString(py_full_name);
    Py_DECREF(py_full_name);
    if (!full_name) {
        PyErr_SetString(PyExc_TypeError, "Message full_name was not a string.");
        return 0;
    }

    gp::Message *msg;
    try {
        msg = dccl::DynamicProtobufManager::new_protobuf_message<gp::Message*>(std::string(full_name));
    } catch (dccl::Exception &e) {
        PyErr_SetString(DcclException, "Could not convert to a known DCCL protobuf type - was the type loaded with a call to dccl.loadProtoFile()?");
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

static PyObject *Codec_encode(Codec *self, PyObject *args) {
    std::string bytes;
    gp::Message *msg = NULL;
    int header_only = 0;
    
    if (!PyArg_ParseTuple(args, "O&|i", &python_pbmsg_to_cpp_pbmsg, &msg, &header_only))
        return NULL;
    
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

static PyObject *Codec_load(Codec *self, PyObject *args) {
    const char *type_name = NULL;
    
    if (!PyArg_ParseTuple(args, "s", &type_name))
        return NULL;
        
    const gp::Descriptor* desc = dccl::DynamicProtobufManager::find_descriptor(std::string(type_name));
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

static PyMethodDef Codec_methods[] = {
    {"id", (PyCFunction)Codec_id, METH_VARARGS, "Return the ID for a string or message."},
    {"encode", (PyCFunction)Codec_encode, METH_VARARGS, "Encode a DCCL Message."},
    {"load", (PyCFunction)Codec_load, METH_VARARGS, "Load a DCCL type by name."},
    {NULL}  /* Sentinel */
};

static PyMemberDef Codec_members[] = {
    {NULL}  /* Sentinel */
};

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
    "Codec objects",           /* tp_doc */
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

PyMODINIT_FUNC init_dccl(void) {
    PyObject *m;
    
    dccl_CodecType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dccl_CodecType) < 0)
        return;

    m = Py_InitModule3("_dccl", DcclMethods, "DCCL Bindings");
    if (m == NULL)
        return;

    Py_INCREF(&dccl_CodecType);
    PyModule_AddObject(m, "Codec", (PyObject *)&dccl_CodecType);
    
    DcclException = PyErr_NewException("dccl.DcclException", NULL, NULL);
    Py_INCREF(DcclException);
    PyModule_AddObject(m, "DcclException", DcclException);
    
    // We're always going to need dynamic support to use this from Python...
    dccl::DynamicProtobufManager::enable_compilation();
    
    GPBMessageModule = PyImport_ImportModule("google.protobuf.message");
    if (GPBMessageModule == NULL)
        return;
}

}
