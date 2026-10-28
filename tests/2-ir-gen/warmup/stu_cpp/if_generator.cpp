#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "IRBuilder.hpp"
#include "Module.hpp"
#include "Type.hpp"

#include <iostream>

#define CONST_INT(num) ConstantInt::get(num, module)
#define CONST_FP(num) ConstantFP::get(num, module)

int main() {
    auto module = new Module();
    auto builder = new IRBuilder(nullptr, module);
    Type *Int32Type = module->get_int32_type();
    Type *FloatType = module->get_float_type();
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}), "main", module);
    auto bb = BasicBlock::create(module, "entry", mainFun);
    builder->set_insert_point(bb);
    auto aAlloca = builder->create_alloca(FloatType);
    builder->create_store(CONST_FP(5.555), aAlloca);
    auto aLoad = builder->create_load(aAlloca);
    auto condition = builder->create_fcmp_gt(aLoad, CONST_FP(1.0));
    auto trueBB = BasicBlock::create(module, "if_true", mainFun);
    auto falseBB = BasicBlock::create(module, "if_false", mainFun);
    builder->create_cond_br(condition, trueBB, falseBB);
    builder->set_insert_point(trueBB);
    builder->create_ret(CONST_INT(233));
    builder->set_insert_point(falseBB);
    builder->create_ret(CONST_INT(0));
    std::cout << module->print();
    delete builder;
    delete module;
    return 0;
}
