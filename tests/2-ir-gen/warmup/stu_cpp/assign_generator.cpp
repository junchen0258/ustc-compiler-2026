#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "IRBuilder.hpp"
#include "Module.hpp"
#include "Type.hpp"

#include <iostream>

#define CONST_INT(num) ConstantInt::get(num, module)

int main() {
    auto module = new Module();
    auto builder = new IRBuilder(nullptr, module);
    Type *Int32Type = module->get_int32_type();
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}), "main", module);
    auto bb = BasicBlock::create(module, "entry", mainFun);
    builder->set_insert_point(bb);
    auto *arrayType = ArrayType::get(Int32Type, 10);
    auto aAlloca = builder->create_alloca(arrayType);
    auto a0GEP = builder->create_gep(aAlloca, {CONST_INT(0), CONST_INT(0)});
    builder->create_store(CONST_INT(10), a0GEP);
    auto a1GEP = builder->create_gep(aAlloca, {CONST_INT(0), CONST_INT(1)});
    a0GEP = builder->create_gep(aAlloca, {CONST_INT(0), CONST_INT(0)});
    auto a0Load = builder->create_load(a0GEP);
    auto product = builder->create_imul(a0Load, CONST_INT(2));
    builder->create_store(product, a1GEP);
    a1GEP = builder->create_gep(aAlloca, {CONST_INT(0), CONST_INT(1)});
    auto result = builder->create_load(a1GEP);
    builder->create_ret(result);
    std::cout << module->print();
    delete builder;
    delete module;
    return 0;
}
