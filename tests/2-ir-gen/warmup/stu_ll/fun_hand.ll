define i32 @callee(i32 %a) {
entry:
  %a_addr = alloca i32
  store i32 %a, i32* %a_addr
  %a_value = load i32, i32* %a_addr
  %product = mul i32 2, %a_value
  ret i32 %product
}

define i32 @main() {
entry:
  %result = call i32 @callee(i32 110)
  ret i32 %result
}

