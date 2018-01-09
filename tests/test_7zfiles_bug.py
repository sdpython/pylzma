#!/usr/bin/python -u
#
# Python Bindings for LZMA
#
# Copyright (c) 2004-2015 by Joachim Bauch, mail@joachim-bauch.de
# 7-Zip Copyright (C) 1999-2010 Igor Pavlov
# LZMA SDK Copyright (C) 1999-2010 Igor Pavlov
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# $Id$
#
import sys
sys.path.append('..')
from datetime import datetime
import errno
import os
import pylzma
from py7zlib import Archive7z, NoPasswordGivenError, WrongPasswordError, UTC
import sys
import unittest

try:
    sorted
except NameError:
    # Python 2.3 and older
    def sorted(l):
        l = list(l)
        l.sort()
        return l

if sys.version_info[:2] < (3, 0):
    def bytes(s, encoding):
        return s
    
    def unicode_string(s):
        return s.decode('latin-1')
else:
    def unicode_string(s):
        return s

ROOT = os.path.abspath(os.path.split(__file__)[0])

class Test7ZipFilesBug(unittest.TestCase):

    def setUp(self):
        self._open_files = []
        unittest.TestCase.setUp(self)

    def tearDown(self):
        while self._open_files:
            fp = self._open_files.pop()
            fp.close()
        unittest.TestCase.tearDown(self)
        
    def _open_file(self, filename, mode):
        fp = open(filename, mode)
        self._open_files.append(fp)
        return fp        

    def test_bug(self):
        # prevent regression bug #1 reported by mail
        fp = self._open_file(os.path.join(ROOT, 'data', 'donnees.7z'), 'rb')
        archive = Archive7z(fp)
        filenames = list(archive.getnames())
        self.assertEqual(len(filenames), 1)
        cf = archive.getmember(filenames[0])
        self.assertNotEqual(cf, None)
        self.assertTrue(cf.checkcrc())
        data = cf.read()
        self.assertEqual(len(data), cf.size)

def suite():
    suite = unittest.TestSuite()

    test_cases = [
        Test7ZipFilesBug,
    ]

    for tc in test_cases:
        suite.addTest(unittest.makeSuite(tc))

    return suite

if __name__ == '__main__':
    unittest.main(defaultTest='suite')
