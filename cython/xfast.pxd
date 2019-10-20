cdef extern from 'xfast.h' namespace 'yfast':
    cdef cppclass xfast[_Key, _Value, _Hash]:
        cppclass Leaf:
            _Key key
            _Value value
        xfast()
        void insert(_Key, _Value&)
        Leaf* find(_Key)
        Leaf* pred(_Key)
        Leaf* succ(_Key)
