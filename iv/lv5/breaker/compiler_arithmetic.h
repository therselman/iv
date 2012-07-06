#ifndef IV_LV5_BREAKER_COMPILER_ARITHMETIC_H_
#define IV_LV5_BREAKER_COMPILER_ARITHMETIC_H_
namespace iv {
namespace lv5 {
namespace breaker {

// opcode | (dst | lhs | rhs)
inline void Compiler::EmitBINARY_MULTIPLY(const Instruction* instr) {
  const int16_t dst = Reg(instr[1].i16[0]);
  const int16_t lhs = Reg(instr[1].i16[1]);
  const int16_t rhs = Reg(instr[1].i16[2]);

  const TypeEntry lhs_type_entry = type_record_.Get(lhs);
  const TypeEntry rhs_type_entry = type_record_.Get(rhs);
  const TypeEntry dst_type_entry =
      TypeEntry::Multiply(lhs_type_entry, rhs_type_entry);

  type_record_.Put(dst, dst_type_entry);

  // dst is constant
  if (dst_type_entry.IsConstant()) {
    EmitConstantDest(dst_type_entry, dst);
    return;
  }

  // lhs or rhs is not int32_t
  if (lhs_type_entry.type().IsNotInt32() || rhs_type_entry.type().IsNotInt32()) {
    LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
    asm_->mov(asm_->rdi, asm_->r14);
    asm_->Call(&stub::BINARY_MULTIPLY);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    set_last_used_candidate(dst);
    return;
  }

  const Assembler::LocalLabelScope scope(asm_);

  if (lhs_type_entry.IsConstantInt32()) {
    const int32_t lhs_value = lhs_type_entry.constant().int32();
    LoadVR(asm_->rax, rhs);
    Int32Guard(rhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->imul(asm_->eax, asm_->eax, lhs_value);
    asm_->jo(".ARITHMETIC_OVERFLOW");
  } else if (rhs_type_entry.IsConstantInt32()) {
    const int32_t rhs_value = rhs_type_entry.constant().int32();
    LoadVR(asm_->rax, lhs);
    Int32Guard(lhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->imul(asm_->eax, asm_->eax, rhs_value);
    asm_->jo(".ARITHMETIC_OVERFLOW");
  } else {
    LoadVRs(asm_->rax, lhs, asm_->rdx, rhs);
    Int32Guard(lhs, asm_->rax, asm_->rcx, ".ARITHMETIC_GENERIC");
    Int32Guard(rhs, asm_->rdx, asm_->rcx, ".ARITHMETIC_GENERIC");
    asm_->imul(asm_->eax, asm_->edx);
    asm_->jo(".ARITHMETIC_OVERFLOW");
  }
  // boxing
  asm_->or(asm_->rax, asm_->r15);
  asm_->jmp(".ARITHMETIC_EXIT");

  kill_last_used_candidate();

  // lhs and rhs is always int32 (but overflow)
  asm_->L(".ARITHMETIC_OVERFLOW");
  LoadVR(asm_->rax, lhs);
  asm_->cvtsi2sd(asm_->xmm0, asm_->eax);
  asm_->cvtsi2sd(asm_->xmm1, asm_->edx);
  asm_->mulsd(asm_->xmm0, asm_->xmm1);
  asm_->movq(asm_->rax, asm_->xmm0);
  ConvertNotNaNDoubleToJSVal(asm_->rax, asm_->rcx);
  asm_->jmp(".ARITHMETIC_EXIT");

  asm_->L(".ARITHMETIC_GENERIC");
  LoadVRs(asm_->rsi, lhs, asm_->rdx, rhs);
  asm_->mov(asm_->rdi, asm_->r14);
  asm_->Call(&stub::BINARY_MULTIPLY);

  asm_->L(".ARITHMETIC_EXIT");
  asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  set_last_used_candidate(dst);
}

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_COMPILER_ARITHMETIC_H_