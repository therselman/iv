#ifndef IV_DETAIL_TYPE_TRAITS_H_
#define IV_DETAIL_TYPE_TRAITS_H_
#include "platform.h"

#if defined(IV_COMPILER_MSVC) || defined(__GXX_EXPERIMENTAL_CXX0X__)
#include <type_traits>

#if defined(IV_COMPILER_MSVC) && !defined(IV_COMPILER_MSVC_10)
namespace std { using namespace tr1; }
#endif

#else

#include <tr1/type_traits>
namespace std { using namespace tr1; }

#endif



#endif  // IV_DETAIL_TYPE_TRAITS_H_
