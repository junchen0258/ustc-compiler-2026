define i32 @main() {
entry:
  %a = alloca i32
  %i = alloca i32
  store i32 10, i32* %a
  store i32 0, i32* %i
  br label %while_cond

while_cond:
  %i_cond = load i32, i32* %i
  %condition = icmp slt i32 %i_cond, 10
  br i1 %condition, label %while_body, label %while_end

while_body:
  %i_old = load i32, i32* %i
  %i_next = add i32 %i_old, 1
  store i32 %i_next, i32* %i
  %a_old = load i32, i32* %a
  %i_current = load i32, i32* %i
  %a_next = add i32 %a_old, %i_current
  store i32 %a_next, i32* %a
  br label %while_cond

while_end:
  %result = load i32, i32* %a
  ret i32 %result
}

