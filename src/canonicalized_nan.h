#ifndef _IV_CANONICALIZED_NAN_H_
#define _IV_CANONICALIZED_NAN_H_
#include "detail/cinttypes.h"
namespace iv {
namespace core {
namespace detail {

union Trans64 {
  uint64_t bits_;
  double double_;
};

// 111111111111000000000000000000000000000000000000000000000000000
static const Trans64 kNaNTrans = { UINT64_C(0x7FF8000000000000) };

}  // namespace detail

static const double kNaN = detail::kNaNTrans.double_;

} }  // namespace iv::core
#endif  // _IV_CANONICALIZED_NAN_H_
