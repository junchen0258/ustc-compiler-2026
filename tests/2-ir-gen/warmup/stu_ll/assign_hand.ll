define i32 @main() {
entry:
  %a = alloca [10 x i32]
  %a0 = getelementptr [10 x i32], [10 x i32]* %a, i64 0, i64 0
  store i32 10, i32* %a0
  %a1 = getelementptr [10 x i32], [10 x i32]* %a, i64 0, i64 1
  %a0_again = getelementptr [10 x i32], [10 x i32]* %a, i64 0, i64 0
  %a0_value = load i32, i32* %a0_again
  %product = mul i32 %a0_value, 2
  store i32 %product, i32* %a1
  %a1_again = getelementptr [10 x i32], [10 x i32]* %a, i64 0, i64 1
  %result = load i32, i32* %a1_again
  ret i32 %result
}

