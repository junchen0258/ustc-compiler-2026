# 2026 实验三：LoongArch64 代码生成

本目录提供实验三的框架、热身练习及自动测试。实验三以完成实验一的解析器和实验二的 IR 生成为前提。

## 需要完成的内容

- `warmup/stu_cpp/`：按照注释完成 6 个热身程序，输入 IR 见 `warmup/ll_cases/`。
- `../../src/codegen/CodeGen.cpp`：补全标有 TODO 的指令翻译和函数调用、返回等逻辑。
- `../../include/codegen/`：汇编输出、寄存器和辅助接口。

热身程序保留了待填空的代码。完成前置实验和热身程序后，在仓库根目录默认构建即可生成六个热身可执行文件，例如：

```bash
cmake -S . -B build
cmake --build build
./build/stu_assign_codegen > assign.s
```

其他热身目标为 `stu_function_codegen`、`stu_icmp_codegen`、`stu_fcmp_codegen`、`stu_float_codegen` 和 `stu_global_codegen`。

## 编译器入口

完成前置实验后，在仓库根目录构建编译器：

```bash
cmake -S . -B build
cmake --build build --target cminusfc
./build/cminusfc -S input.cminus -o output.s
```

省略 `-o` 时，`./build/cminusfc -S input.cminus` 默认生成 `input.s`。

`-emit-ast` 和 `-emit-llvm` 的现有入口保留。后端框架尚未实现的部分会抛出 `not_implemented_error`，需要完成 TODO 后才能运行相关程序。

## 自动测试

需要安装 `loongarch64-unknown-linux-gnu-gcc` 和 `qemu-loongarch64`。在以下目录运行测试脚本：

```bash
cd tests/3-codegen/autogen
./eval_lab3.sh ./testcases debug
# 也可以运行通用测试集
./eval_lab3.sh ../../testcases_general test
```

`debug` 模式额外保存 LLVM IR。脚本生成 LoongArch64 汇编，交叉编译并使用 QEMU 执行，将标准输出和退出码与 `.out` 文件比较。生成文件在 `output/`，详细日志为 `log.txt`。

```bash
./cleanup.sh
```
