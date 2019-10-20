# cython: language_level=3
# distutils: language = c++

from xfast cimport xfast

from libcpp.string cimport string
from libcpp.unordered_map cimport unordered_map

from cython.operator cimport dereference

ctypedef int Key
ctypedef string Value
ctypedef unordered_map[Key, void*] Hash

def test_insert_zero():
    cdef xfast[Key, Value, Hash] trie
    cdef Key key = 0
    cdef Value value = b'zero'
    trie.insert(key, value)
    leaf = trie.find(key)
    assert(leaf is not NULL)
    assert(dereference(leaf).value == value)
