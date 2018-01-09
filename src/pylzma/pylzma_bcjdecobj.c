/*
 * Python Bindings for LZMA
 *
 * Copyright (c) 2004-2015 by Joachim Bauch, mail@joachim-bauch.de
 * 7-Zip Copyright (C) 1999-2010 Igor Pavlov
 * LZMA SDK Copyright (C) 1999-2010 Igor Pavlov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * $Id$
 *
 */

#include <Python.h>

#include "pylzma.h"
#include "pylzma_bcjdecobj.h"

static int
pylzma_bcjdecobj_init(CBcjDecompressionObject *self, PyObject *args, PyObject *kwargs)
{
    PY_LONG_LONG max_length = -1;
    
    // possible keywords for this function
    static char *kwlist[] = {"maxlength", NULL};
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|L", kwlist, &max_length))
        return -1;
    
    if (max_length == 0 || max_length < -1) {
        PyErr_SetString(PyExc_ValueError, "the decompressed size must be greater than zero");
        return -1;
    }
    
    self->unconsumed_tail = NULL;
    self->unconsumed_length = 0;
    self->max_length = max_length;
    self->total_out = 0;
    self->needs_init = 1;
    
    return 0;
}

static const char
doc_bcjdec_decompress[] = \
    "decompress(data[, bufsize]) -- Returns a string containing the up to bufsize decompressed bytes of the data.\n" \
    "After calling, some of the input data may be available in internal buffers for later processing.";

void hexDump (char *desc, void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        PySys_WriteStdout ("%s:\n", desc);

    if (len == 0) {
        PySys_WriteStdout("  ZERO LENGTH\n");
        return;
    }
    if (len < 0) {
        PySys_WriteStdout("  NEGATIVE LENGTH: %i\n",len);
        return;
    }

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                PySys_WriteStdout ("  %s\n", buff);

            // Output the offset.
            PySys_WriteStdout ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        PySys_WriteStdout (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        PySys_WriteStdout ("   ");
        i++;
    }

    // And print the final ASCII bit.
    PySys_WriteStdout ("  %s\n", buff);
}
static PyObject *
pylzma_bcjdec_decode(CBcjDecompressionObject *self, PyObject *args)
{
    PyObject *result=NULL;
    unsigned char *data;
    Byte *next_in, *next_out;
    PARSE_LENGTH_TYPE length;
    int res;
    PY_LONG_LONG bufsize=BLOCK_SIZE;
    SizeT avail_in;
    SizeT inProcessed, outProcessed;
    SizeT SIZE_TEMP = 20*1024;
    
    if (!PyArg_ParseTuple(args, "s#|L", &data, &length, &bufsize)){
        return NULL;
    }

    if (bufsize <= 0) {
        PyErr_SetString(PyExc_ValueError, "bufsize must be greater than zero");
        return NULL;
    }
    
    if (self->unconsumed_length > 0) {
        self->unconsumed_tail = (unsigned char *) realloc(self->unconsumed_tail, self->unconsumed_length + length);
        next_in = (unsigned char *) self->unconsumed_tail;
        memcpy(next_in + self->unconsumed_length, data, length);
    } else {
        next_in = data;
    }
    
    if (self->needs_init) {
        self->needs_init = 0;
        
        //Are these just temp buffs? gunna treat them like temp buffs
        Bcj2Dec_Init(&self->state);
        self->unconsumed_length = length;
    } else {
        self->unconsumed_length += length;
    }
    avail_in = self->unconsumed_length;
    if (avail_in == 0) {
        // no more bytes to decompress
        return PyBytes_FromString("");
    }
    
    result = PyBytes_FromStringAndSize(NULL, avail_in);
    if (result == NULL) {
        PyErr_NoMemory();
        goto exit;
    }
    
    next_out = (unsigned char *) PyBytes_AS_STRING(result);
    memset(next_out, 0, avail_in);
    self->state.bufs[BCJ2_STREAM_MAIN] = next_in;
    self->state.lims[BCJ2_STREAM_MAIN] = next_in + avail_in;
    self->state.bufs[BCJ2_STREAM_RC] = next_out;
    self->state.lims[BCJ2_STREAM_RC] = next_out + avail_in;
    SIZE_TEMP =  avail_in + (4-(avail_in&3));
    self->state.bufs[BCJ2_STREAM_CALL] = (unsigned char *)malloc(SIZE_TEMP);
    self->state.bufs[BCJ2_STREAM_CALL] = self->state.bufs[BCJ2_STREAM_CALL] + SIZE_TEMP;
    self->state.bufs[BCJ2_STREAM_JUMP] = (unsigned char *)malloc(SIZE_TEMP);
    self->state.bufs[BCJ2_STREAM_JUMP] = self->state.bufs[BCJ2_STREAM_JUMP] + SIZE_TEMP;
    self->state.dest = next_out;
    self->state.destLim = next_out + avail_in;
    //hexDump(NULL, next_in, avail_in);
    //Py_BEGIN_ALLOW_THREADS
    res = Bcj2Dec_Decode(&self->state);
    //Py_END_ALLOW_THREADS
    PySys_WriteStdout("Post Decode %d\n", self->state.state);
    //self->total_out += outProcessed;
    avail_in = self->state.lims[BCJ2_STREAM_RC] - self->state.bufs[BCJ2_STREAM_RC];
    next_in = (Byte *)self->state.bufs[BCJ2_STREAM_RC];
    
    if (res != SZ_OK) {
        PySys_WriteStdout("Res: %d Status: %d\n", res, self->state.state);
        DEC_AND_NULL(result);
        PyErr_SetString(PyExc_ValueError, "data error during decompression");
        goto exit;
    }
    //hexDump(NULL, self->state.bufs[BCJ2_STREAM_CALL], avail_in);

    // Not all of the compressed data could be accomodated in the output buffer
    // of specified size. Return the unconsumed tail in an attribute.
    if (avail_in > 0) {
        if (self->unconsumed_tail == NULL) {
            // data are in "data"
            self->unconsumed_tail = (unsigned char *) malloc(avail_in);
            if (self->unconsumed_tail == NULL) {
                Py_DECREF(result);
                PyErr_NoMemory();
                goto exit;
            }
            memcpy(self->unconsumed_tail, next_in, avail_in);
        } else {
            memmove(self->unconsumed_tail, next_in, avail_in);
            self->unconsumed_tail = (unsigned char *) realloc(self->unconsumed_tail, avail_in);
        }
    } else {
        FREE_AND_NULL(self->unconsumed_tail);
    }
    
    self->unconsumed_length = avail_in;
    _PyBytes_Resize(&result, outProcessed);
    
exit:
    return result;
}

static PyMethodDef
pylzma_bcjdecobj_methods[] = {
    {"decompress", (PyCFunction)pylzma_bcjdec_decode, METH_VARARGS, (char *)&doc_bcjdec_decompress},
    //{"flush",      (PyCFunction)pylzma_decomp_flush,      METH_NOARGS,  (char *)&doc_decomp_flush},
    //{"reset",      (PyCFunction)pylzma_decomp_reset,      METH_VARARGS | METH_KEYWORDS, (char *)&doc_decomp_reset},
    {NULL},
};

static void
pylzma_bcjdec_dealloc(CBcjDecompressionObject *self)
{
    FREE_AND_NULL(self->state.bufs[BCJ2_STREAM_CALL])
    FREE_AND_NULL(self->state.bufs[BCJ2_STREAM_JUMP])
    FREE_AND_NULL(self->unconsumed_tail);
    Py_TYPE(self)->tp_free((PyObject*) self);
}

PyTypeObject
CBcjDecompressionObject_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pylzma.bcjdecobj",                  /* char *tp_name; */
    sizeof(CBcjDecompressionObject),        /* int tp_basicsize; */
    0,                                   /* int tp_itemsize;       // not used much */
    (destructor)pylzma_bcjdec_dealloc,   /* destructor tp_dealloc; */
    NULL,                                /* printfunc  tp_print;   */
    NULL,                                /* getattrfunc  tp_getattr; // __getattr__ */
    NULL,                                /* setattrfunc  tp_setattr;  // __setattr__ */
    NULL,                                /* cmpfunc  tp_compare;  // __cmp__ */
    NULL,                                /* reprfunc  tp_repr;    // __repr__ */
    NULL,                                /* PyNumberMethods *tp_as_number; */
    NULL,                                /* PySequenceMethods *tp_as_sequence; */
    NULL,                                /* PyMappingMethods *tp_as_mapping; */
    NULL,                                /* hashfunc tp_hash;     // __hash__ */
    NULL,                                /* ternaryfunc tp_call;  // __call__ */
    NULL,                                /* reprfunc tp_str;      // __str__ */
    0,                                   /* tp_getattro*/
    0,                                   /* tp_setattro*/
    0,                                   /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,                  /*tp_flags*/
    "Decompression class",               /* tp_doc */
    0,                                   /* tp_traverse */
    0,                                   /* tp_clear */
    0,                                   /* tp_richcompare */
    0,                                   /* tp_weaklistoffset */
    0,                                   /* tp_iter */
    0,                                   /* tp_iternext */
    pylzma_bcjdecobj_methods,            /* tp_methods */
    0,                                   /* tp_members */
    0,                                   /* tp_getset */
    0,                                   /* tp_base */
    0,                                   /* tp_dict */
    0,                                   /* tp_descr_get */
    0,                                   /* tp_descr_set */
    0,                                   /* tp_dictoffset */
    (initproc)pylzma_bcjdecobj_init,     /* tp_init */
    0,                                   /* tp_alloc */
    0,                                   /* tp_new */
};
