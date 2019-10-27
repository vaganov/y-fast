cdef extern from 'xfast.h' namespace 'yfast':
    cdef cppclass xfast[Key, Value, Hash = *]:
        cppclass Leaf:
            Key key
            Value value
        xfast()
        Leaf* insert(Key, const Value&)
        Leaf* find(Key)
        Leaf* pred(Key)
        Leaf* succ(Key)
