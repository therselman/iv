#ifndef IV_LV5_INTERPRETER_H_
#define IV_LV5_INTERPRETER_H_
#include "ast.h"
#include "ast_visitor.h"
#include "noncopyable.h"
#include "jsast.h"
#include "arguments.h"
#include "utils.h"
#include "jsval.h"
#include "symbol.h"

namespace iv {
namespace lv5 {

class Context;
class Error;
class JSEnv;
class JSDeclEnv;
class JSObjectEnv;
class JSCodeFunction;
class JSReference;

class Interpreter : private core::Noncopyable<Interpreter>::type,
                    public AstVisitor {
 public:
  enum CompareKind {
    CMP_TRUE,
    CMP_FALSE,
    CMP_UNDEFINED,
    CMP_ERROR
  };
  Interpreter();
  ~Interpreter();
  void Run(const FunctionLiteral* global, bool is_eval);

  Context* context() const {
    return ctx_;
  }
  void set_context(Context* context) {
    ctx_ = context;
  }
  void CallCode(JSCodeFunction* code, const Arguments& args,
                Error* error);

  static JSDeclEnv* NewDeclarativeEnvironment(Context* ctx, JSEnv* env);
  static JSObjectEnv* NewObjectEnvironment(Context* ctx,
                                           JSObject* val, JSEnv* env);

  class ContextSwitcher : private core::Noncopyable<ContextSwitcher>::type {
   public:
    ContextSwitcher(Context* ctx,
                    JSEnv* lex,
                    JSEnv* var,
                    const JSVal& binding,
                    bool strict);
    ~ContextSwitcher();
   private:
    JSEnv* prev_lex_;
    JSEnv* prev_var_;
    JSVal prev_binding_;
    bool prev_strict_;
    Context* ctx_;
  };

  class LexicalEnvSwitcher
      : private core::Noncopyable<LexicalEnvSwitcher>::type {
   public:
    LexicalEnvSwitcher(Context* context, JSEnv* env);
    ~LexicalEnvSwitcher();
   private:
    Context* ctx_;
    JSEnv* old_;
  };

  class StrictSwitcher : private core::Noncopyable<StrictSwitcher>::type {
   public:
    StrictSwitcher(Context* ctx, bool strict);
    ~StrictSwitcher();
   private:
    Context* ctx_;
    bool prev_;
  };

 private:
  void Visit(const Block* block);
  void Visit(const FunctionStatement* func);
  void Visit(const FunctionDeclaration* func);
  void Visit(const VariableStatement* var);
  void Visit(const EmptyStatement* stmt);
  void Visit(const IfStatement* stmt);
  void Visit(const DoWhileStatement* stmt);
  void Visit(const WhileStatement* stmt);
  void Visit(const ForStatement* stmt);
  void Visit(const ForInStatement* stmt);
  void Visit(const ContinueStatement* stmt);
  void Visit(const BreakStatement* stmt);
  void Visit(const ReturnStatement* stmt);
  void Visit(const WithStatement* stmt);
  void Visit(const LabelledStatement* stmt);
  void Visit(const SwitchStatement* stmt);
  void Visit(const ThrowStatement* stmt);
  void Visit(const TryStatement* stmt);
  void Visit(const DebuggerStatement* stmt);
  void Visit(const ExpressionStatement* stmt);
  void Visit(const Assignment* assign);
  void Visit(const BinaryOperation* binary);
  void Visit(const ConditionalExpression* cond);
  void Visit(const UnaryOperation* unary);
  void Visit(const PostfixExpression* postfix);
  void Visit(const StringLiteral* literal);
  void Visit(const NumberLiteral* literal);
  void Visit(const Identifier* literal);
  void Visit(const ThisLiteral* literal);
  void Visit(const NullLiteral* lit);
  void Visit(const TrueLiteral* lit);
  void Visit(const FalseLiteral* lit);
  void Visit(const RegExpLiteral* literal);
  void Visit(const ArrayLiteral* literal);
  void Visit(const ObjectLiteral* literal);
  void Visit(const FunctionLiteral* literal);
  void Visit(const IdentifierAccess* prop);
  void Visit(const IndexAccess* prop);
  void Visit(const FunctionCall* call);
  void Visit(const ConstructorCall* call);

  void Visit(const Declaration* dummy);
  void Visit(const CaseClause* dummy);

  bool InCurrentLabelSet(const BreakableStatement* stmt);

  // section 11.8.5
  template<bool LeftFirst>
  CompareKind Compare(const JSVal& lhs, const JSVal& rhs, Error* error) {
    JSVal px;
    JSVal py;
    if (LeftFirst) {
      px = lhs.ToPrimitive(ctx_, Hint::NUMBER, error);
      if (*error) {
        return CMP_ERROR;
      }
      py = rhs.ToPrimitive(ctx_, Hint::NUMBER, error);
      if (*error) {
        return CMP_ERROR;
      }
    } else {
      py = rhs.ToPrimitive(ctx_, Hint::NUMBER, error);
      if (*error) {
        return CMP_ERROR;
      }
      px = lhs.ToPrimitive(ctx_, Hint::NUMBER, error);
      if (*error) {
        return CMP_ERROR;
      }
    }
    if (px.IsString() && py.IsString()) {
      // step 4
      return (*(px.string()) < *(py.string())) ? CMP_TRUE : CMP_FALSE;
    } else {
      const double nx = px.ToNumber(ctx_, error);
      if (*error) {
        return CMP_ERROR;
      }
      const double ny = py.ToNumber(ctx_, error);
      if (*error) {
        return CMP_ERROR;
      }
      if (std::isnan(nx) || std::isnan(ny)) {
        return CMP_UNDEFINED;
      }
      if (nx == ny) {
        if (std::signbit(nx) != std::signbit(ny)) {
          return CMP_FALSE;
        }
        return CMP_FALSE;
      }
      if (nx == std::numeric_limits<double>::infinity()) {
        return CMP_FALSE;
      }
      if (ny == std::numeric_limits<double>::infinity()) {
        return CMP_TRUE;
      }
      if (ny == (-std::numeric_limits<double>::infinity())) {
        return CMP_FALSE;
      }
      if (nx == (-std::numeric_limits<double>::infinity())) {
        return CMP_TRUE;
      }
      return (nx < ny) ? CMP_TRUE : CMP_FALSE;
    }
  }

  JSVal GetValue(const JSVal& val, Error* error);
  void PutValue(const JSVal& val, const JSVal& w, Error* error);
  JSReference* GetIdentifierReference(JSEnv* lex, Symbol name, bool strict);

  Context* ctx_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_INTERPRETER_H_
