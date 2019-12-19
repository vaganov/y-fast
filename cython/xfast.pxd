from libcpp cimport bool

cdef extern from 'xfast.h' namespace 'yfast':
    cdef cppclass xfast[Key, Value, Hash = *]:
        cppclass Leaf:
            Key key
            Value value
        xfast()
        Leaf* insert(Key, const Value&)
        bool remove(Key)
        Leaf* leftmost()
        Leaf* rightmost()
        Leaf* find(Key)
        Leaf* pred(Key)
        Leaf* succ(Key)
