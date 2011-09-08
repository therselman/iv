#ifndef IV_LV5_JSVAL_H_
#define IV_LV5_JSVAL_H_
#include "detail/array.h"
#include "dtoa.h"
#include "lv5/error_check.h"
#include "lv5/jsval_fwd.h"
#include "lv5/error.h"
#include "lv5/jsobject.h"
#include "lv5/context_utils.h"
#include "lv5/jsbooleanobject.h"
#include "lv5/jsnumberobject.h"
#include "lv5/jsstringobject.h"
namespace iv {
namespace lv5 {

JSString* JSVal::TypeOf(Context* ctx) const {
  if (IsObject()) {
    if (object()->IsCallable()) {
      return JSString::NewAsciiString(ctx, "function");
    } else {
      return JSString::NewAsciiString(ctx, "object");
    }
  } else if (IsNumber()) {
    return JSString::NewAsciiString(ctx, "number");
  } else if (IsString()) {
    return JSString::NewAsciiString(ctx, "string");
  } else if (IsBoolean()) {
    return JSString::NewAsciiString(ctx, "boolean");
  } else if (IsNull()) {
    return JSString::NewAsciiString(ctx, "object");
  } else {
    assert(IsUndefined());
    return JSString::NewAsciiString(ctx, "undefined");
  }
}

JSObject* JSVal::GetPrimitiveProto(Context* ctx) const {
  assert(IsPrimitive());
  if (IsString()) {
    return context::GetClassSlot(ctx, Class::String).prototype;
  } else if (IsNumber()) {
    return context::GetClassSlot(ctx, Class::Number).prototype;
  } else {  // IsBoolean()
    return context::GetClassSlot(ctx, Class::Boolean).prototype;
  }
}

JSObject* JSVal::ToObject(Context* ctx, Error* e) const {
  if (IsObject()) {
    return object();
  } else if (IsNumber()) {
    return JSNumberObject::New(ctx, number());
  } else if (IsString()) {
    return JSStringObject::New(ctx, string());
  } else if (IsBoolean()) {
    return JSBooleanObject::New(ctx, boolean());
  } else if (IsNull()) {
    e->Report(Error::Type, "null has no properties");
    return NULL;
  } else {
    assert(IsUndefined());
    e->Report(Error::Type, "undefined has no properties");
    return NULL;
  }
}

JSString* JSVal::ToString(Context* ctx, Error* e) const {
  if (IsString()) {
    return string();
  } else if (IsNumber()) {
    std::array<char, 80> buffer;
    const char* const str =
        core::DoubleToCString(number(), buffer.data(), buffer.size());
    return JSString::NewAsciiString(ctx, str);
  } else if (IsBoolean()) {
    return JSString::NewAsciiString(ctx, (boolean() ? "true" : "false"));
  } else if (IsNull()) {
    return JSString::NewAsciiString(ctx, "null");
  } else if (IsUndefined()) {
    return JSString::NewAsciiString(ctx, "undefined");
  } else {
    assert(IsObject());
    JSVal prim = object()->DefaultValue(ctx, Hint::STRING,
                                        IV_LV5_ERROR_WITH(e, NULL));
    return prim.ToString(ctx, e);
  }
}

double JSVal::ToNumber(Context* ctx, Error* e) const {
  if (IsNumber()) {
    return number();
  } else if (IsString()) {
    return core::StringToDouble(*string()->GetFiber(), false);
  } else if (IsBoolean()) {
    return boolean() ? 1 : +0;
  } else if (IsNull()) {
    return +0;
  } else if (IsUndefined()) {
    return core::kNaN;
  } else {
    assert(IsObject());
    JSVal prim = object()->DefaultValue(ctx, Hint::NUMBER,
                                        IV_LV5_ERROR_WITH(e, 0.0));
    return prim.ToNumber(ctx, e);
  }
}

bool JSVal::ToBoolean(Error* e) const {
  if (IsNumber()) {
    const double num = number();
    return num != 0 && !core::IsNaN(num);
  } else if (IsString()) {
    return !string()->empty();
  } else if (IsNull()) {
    return false;
  } else if (IsUndefined()) {
    return false;
  } else if (IsBoolean()) {
    return boolean();
  } else {
    assert(!IsEmpty());
    return true;
  }
}

JSVal JSVal::ToPrimitive(Context* ctx, Hint::Object hint, Error* e) const {
  if (IsObject()) {
    return object()->DefaultValue(ctx, hint, e);
  } else {
    assert(!IsEnvironment() && !IsReference() && !IsEmpty());
    return *this;
  }
}

int32_t JSVal::ToInt32(Context* ctx, Error* e) const {
  if (IsInt32()) {
    return int32();
  } else {
    return core::DoubleToInt32(ToNumber(ctx, e));
  }
}

uint32_t JSVal::ToUInt32(Context* ctx, Error* e) const {
  if (IsInt32() && int32() >= 0) {
    return static_cast<uint32_t>(int32());
  } else {
    return core::DoubleToUInt32(ToNumber(ctx, e));
  }
}

uint32_t JSVal::GetUInt32() const {
  assert(IsNumber());
  uint32_t val = 0;  // make gcc happy
  GetUInt32(&val);
  return val;
}

bool JSVal::GetUInt32(uint32_t* result) const {
  if (IsInt32() && int32() >= 0) {
    *result = static_cast<uint32_t>(int32());
    return true;
  } else if (IsNumber()) {
    const double val = number();
    const uint32_t res = static_cast<uint32_t>(val);
    if (val == res) {
      *result = res;
      return true;
    }
  }
  return false;
}

bool JSVal::IsCallable() const {
  return IsObject() && object()->IsCallable();
}

void JSVal::CheckObjectCoercible(Error* e) const {
  assert(!IsEnvironment() && !IsReference() && !IsEmpty());
  if (IsNull()) {
    e->Report(Error::Type, "null has no properties");
  } else if (IsUndefined()) {
    e->Report(Error::Type, "undefined has no properties");
  }
}

} }  // namespace iv::lv5
#endif  // IV_LV5_JSVAL_H_
