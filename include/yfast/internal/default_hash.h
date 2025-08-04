#ifndef _YFAST_INTERNAL_DEFAULT_HASH_H
#define _YFAST_INTERNAL_DEFAULT_HASH_H

#ifdef YFAST_WITHOUT_HOPSCOTCH_MAP
#include <unordered_map>
#else
#include <tsl/hopscotch_map.h>
#endif

namespace yfast::internal {

#ifdef YFAST_WITHOUT_HOPSCOTCH_MAP
template <typename Key, typename Value>
using DefaultHash = std::unordered_map<Key, Value>;
#else
template <typename Key, typename Value>
using DefaultHash = tsl::hopscotch_map<Key, Value>;
#endif

}

#endif
