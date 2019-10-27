# cython: language_level=3
# distutils: language=c++

from xfast cimport xfast

from libcpp.string cimport string

from cython.operator cimport dereference

ctypedef int Key
ctypedef string Value

def test_insert_zero():
    cdef xfast[Key, Value] trie
    cdef Key key = 0
    cdef Value value = b'zero'
    leaf = trie.insert(key, value)
    assert(leaf is not NULL)
    assert(dereference(leaf).value == value)
    leaf = trie.find(key)
    assert(leaf is not NULL)
    assert(dereference(leaf).value == value)
    leaf = trie.pred(key)
    assert(leaf is not NULL)
    assert(dereference(leaf).value == value)
    leaf = trie.succ(key)
    assert(leaf is not NULL)
    assert(dereference(leaf).value == value)

def test_insert_zero_and_one():
    cdef xfast[Key, Value] trie
    cdef Key key0 = 0
    cdef Value value0 = b'zero'
    leaf0 = trie.insert(key0, value0)
    cdef key1 = 1
    cdef Value value1 = b'one'
    leaf1 = trie.insert(key1, value1)
    assert(leaf0 is not NULL)
    assert(dereference(leaf0).value == value0)
    leaf0 = trie.find(key0)
    assert(leaf0 is not NULL)
    assert(dereference(leaf0).value == value0)
    leaf0 = trie.pred(key0)
    assert(leaf0 is not NULL)
    assert(dereference(leaf0).value == value0)
    leaf0 = trie.succ(key0)
    assert(leaf0 is not NULL)
    assert(dereference(leaf0).value == value0)
    assert(leaf1 is not NULL)
    assert(dereference(leaf1).value == value1)
    leaf1 = trie.find(key1)
    assert(leaf1 is not NULL)
    assert(dereference(leaf1).value == value1)
    leaf1 = trie.pred(key1)
    assert(leaf1 is not NULL)
    assert(dereference(leaf1).value == value1)
    leaf1 = trie.succ(key1)
    assert(leaf1 is not NULL)
    assert(dereference(leaf1).value == value1)

def test_pred_succ_one():
    cdef xfast[Key, Value] trie
    cdef Key key0 = 0
    cdef Value value0 = b'zero'
    trie.insert(key0, value0)
    cdef Key key2 = 2
    cdef Value value2 = b'two'
    trie.insert(key2, value2)
    cdef Key key1 = 1
    leaf0 = trie.pred(key1)
    assert(leaf0 is not NULL)
    assert(dereference(leaf0).value == value0)
    leaf2 = trie.succ(key1)
    assert(leaf2 is not NULL)
    assert(dereference(leaf2).value == value2)
