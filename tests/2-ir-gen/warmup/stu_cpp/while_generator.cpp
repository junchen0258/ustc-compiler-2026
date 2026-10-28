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
    auto aAlloca = builder->create_alloca(Int32Type);
    auto iAlloca = builder->create_alloca(Int32Type);
    builder->create_store(CONST_INT(10), aAlloca);
    builder->create_store(CONST_INT(0), iAlloca);
    auto condBB = BasicBlock::create(module, "while_cond", mainFun);
    auto bodyBB = BasicBlock::create(module, "while_body", mainFun);
    auto endBB = BasicBlock::create(module, "while_end", mainFun);
    builder->create_br(condBB);
    builder->set_insert_point(condBB);
    auto iCondLoad = builder->create_load(iAlloca);
    auto condition = builder->create_icmp_lt(iCondLoad, CONST_INT(10));
    builder->create_cond_br(condition, bodyBB, endBB);
    builder->set_insert_point(bodyBB);
    auto iLoad = builder->create_load(iAlloca);
    auto iNext = builder->create_iadd(iLoad, CONST_INT(1));
    builder->create_store(iNext, iAlloca);
    auto aLoad = builder->create_load(aAlloca);
    auto iCurrent = builder->create_load(iAlloca);
    auto aNext = builder->create_iadd(aLoad, iCurrent);
    builder->create_store(aNext, aAlloca);
    builder->create_br(condBB);
    builder->set_insert_point(endBB);
    auto result = builder->create_load(aAlloca);
    builder->create_ret(result);
    std::cout << module->print();
    delete builder;
    delete module;
    return 0;
}
