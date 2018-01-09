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

#ifndef ___PYLZMA_BCJ2DECOBJ__H___
#define ___PYLZMA_BCJDECOBJ__H___

#include <Python.h>

#include "../sdk/C/Bcj2.h"

typedef struct {
    PyObject_HEAD
    CBcj2Dec state;
    SizeT max_length;
    SizeT total_out;
    unsigned char *unconsumed_tail;
    SizeT unconsumed_length;
    int needs_init;
} CBcjDecompressionObject;

extern PyTypeObject CBcjDecompressionObject_Type;

#define BcjDecompressionObject_Check(v)   ((v)->ob_type == &CBcjDecompressionObject_Type)

#endif
