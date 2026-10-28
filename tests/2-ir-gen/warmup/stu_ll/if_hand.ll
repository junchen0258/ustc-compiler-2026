define i32 @main() {
entry:
  %a = alloca float
  store float 0x40163851E0000000, float* %a
  %a_value = load float, float* %a
  %condition = fcmp ogt float %a_value, 1.000000e+00
  br i1 %condition, label %if_true, label %if_false

if_true:
  ret i32 233

if_false:
  ret i32 0
}

