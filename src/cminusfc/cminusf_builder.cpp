#include "cminusf_builder.hpp"

#define CONST_FP(num) ConstantFP::get((float)num, module.get())
#define CONST_INT(num) ConstantInt::get(num, module.get())

// types
Type *VOID_T;
Type *INT1_T;
Type *INT32_T;
Type *INT32PTR_T;
Type *FLOAT_T;
Type *FLOATPTR_T;

/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */

Value* CminusfBuilder::visit(ASTProgram &node) {
    VOID_T = module->get_void_type();
    INT1_T = module->get_int1_type();
    INT32_T = module->get_int32_type();
    INT32PTR_T = module->get_int32_ptr_type();
    FLOAT_T = module->get_float_type();
    FLOATPTR_T = module->get_float_ptr_type();

    Value *ret_val = nullptr;
    for (auto &decl : node.declarations) {
        ret_val = decl->accept(*this);
    }
    return ret_val;
}

Value* CminusfBuilder::visit(ASTNum &node) {
    if (node.type == TYPE_INT)
        return CONST_INT(node.i_val);
    return CONST_FP(node.f_val);
}

Value* CminusfBuilder::visit(ASTVarDeclaration &node) {
    Type *var_type = node.type == TYPE_INT ? static_cast<Type *>(INT32_T)
                                           : static_cast<Type *>(FLOAT_T);
    if (node.num != nullptr)
        var_type = module->get_array_type(var_type, node.num->i_val);

    if (scope.in_global()) {
        auto *var = GlobalVariable::create(
            node.id, module.get(), var_type, false,
            ConstantZero::get(var_type, module.get()));
        scope.push(node.id, var);
    } else {
        auto *current_bb = builder->get_insert_block();
        builder->set_insert_point(context.func->get_entry_block());
        auto *var = builder->create_alloca_begin(var_type);
        builder->set_insert_point(current_bb);
        scope.push(node.id, var);
    }
    return nullptr;
}

Value* CminusfBuilder::visit(ASTFunDeclaration &node) {
    FunctionType *fun_type;
    Type *ret_type;
    std::vector<Type *> param_types;
    if (node.type == TYPE_INT)
        ret_type = INT32_T;
    else if (node.type == TYPE_FLOAT)
        ret_type = FLOAT_T;
    else
        ret_type = VOID_T;

    for (auto &param : node.params) {
        Type *param_type = param->type == TYPE_INT ? INT32_T : FLOAT_T;
        if (param->isarray)
            param_type = param->type == TYPE_INT ? INT32PTR_T : FLOATPTR_T;
        param_types.push_back(param_type);
    }

    fun_type = FunctionType::get(ret_type, param_types);
    auto func = Function::create(fun_type, node.id, module.get());
    scope.push(node.id, func);
    context.func = func;
    auto funBB = BasicBlock::create(module.get(), "entry", func);
    builder->set_insert_point(funBB);
    scope.enter();
    std::vector<Value *> args;
    for (auto &arg : func->get_args()) {
        args.push_back(&arg);
    }
    for (unsigned int i = 0; i < node.params.size(); ++i) {
        context.param = args[i];
        node.params[i]->accept(*this);
    }
    node.compound_stmt->accept(*this);
    if (not builder->get_insert_block()->is_terminated()) 
    {
        if (context.func->get_return_type()->is_void_type())
            builder->create_void_ret();
        else if (context.func->get_return_type()->is_float_type())
            builder->create_ret(CONST_FP(0.));
        else
            builder->create_ret(CONST_INT(0));
    }
    scope.exit();
    return nullptr;
}

Value* CminusfBuilder::visit(ASTParam &node) {
    auto *param_addr = builder->create_alloca_begin(context.param->get_type());
    builder->create_store(context.param, param_addr);
    scope.push(node.id, param_addr);
    context.param = nullptr;
    return nullptr;
}

Value* CminusfBuilder::visit(ASTCompoundStmt &node) {
    bool is_function_body = context.compound_stmt_depth == 0;
    if (!is_function_body)
        scope.enter();
    ++context.compound_stmt_depth;

    for (auto &decl : node.local_declarations) {
        decl->accept(*this);
    }

    for (auto &stmt : node.statement_list) {
        stmt->accept(*this);
        if (builder->get_insert_block()->is_terminated())
            break;
    }

    --context.compound_stmt_depth;
    if (!is_function_body)
        scope.exit();
    return nullptr;
}

Value* CminusfBuilder::visit(ASTExpressionStmt &node) {
    if (node.expression != nullptr)
        return node.expression->accept(*this);
    return nullptr;
}

Value* CminusfBuilder::visit(ASTSelectionStmt &node) {
    auto *cond = node.expression->accept(*this);
    if (cond->get_type()->is_float_type())
        cond = builder->create_fcmp_ne(cond, CONST_FP(0.));
    else
        cond = builder->create_icmp_ne(cond, CONST_INT(0));

    auto *then_bb = BasicBlock::create(
        module.get(), "if_then_" + std::to_string(context.block_id++),
        context.func);
    auto *end_bb = BasicBlock::create(
        module.get(), "if_end_" + std::to_string(context.block_id++),
        context.func);

    if (node.else_statement != nullptr) {
        auto *else_bb = BasicBlock::create(
            module.get(), "if_else_" + std::to_string(context.block_id++),
            context.func);
        builder->create_cond_br(cond, then_bb, else_bb);

        builder->set_insert_point(then_bb);
        node.if_statement->accept(*this);
        if (!builder->get_insert_block()->is_terminated())
            builder->create_br(end_bb);

        builder->set_insert_point(else_bb);
        node.else_statement->accept(*this);
        if (!builder->get_insert_block()->is_terminated())
            builder->create_br(end_bb);
    } else {
        builder->create_cond_br(cond, then_bb, end_bb);
        builder->set_insert_point(then_bb);
        node.if_statement->accept(*this);
        if (!builder->get_insert_block()->is_terminated())
            builder->create_br(end_bb);
    }

    builder->set_insert_point(end_bb);
    return nullptr;
}

Value* CminusfBuilder::visit(ASTIterationStmt &node) {
    auto *cond_bb = BasicBlock::create(
        module.get(), "while_cond_" + std::to_string(context.block_id++),
        context.func);
    auto *body_bb = BasicBlock::create(
        module.get(), "while_body_" + std::to_string(context.block_id++),
        context.func);
    auto *end_bb = BasicBlock::create(
        module.get(), "while_end_" + std::to_string(context.block_id++),
        context.func);

    builder->create_br(cond_bb);
    builder->set_insert_point(cond_bb);
    auto *cond = node.expression->accept(*this);
    if (cond->get_type()->is_float_type())
        cond = builder->create_fcmp_ne(cond, CONST_FP(0.));
    else
        cond = builder->create_icmp_ne(cond, CONST_INT(0));
    builder->create_cond_br(cond, body_bb, end_bb);

    builder->set_insert_point(body_bb);
    node.statement->accept(*this);
    if (!builder->get_insert_block()->is_terminated())
        builder->create_br(cond_bb);

    builder->set_insert_point(end_bb);
    return nullptr;
}

Value* CminusfBuilder::visit(ASTReturnStmt &node) {
    if (node.expression == nullptr) {
        builder->create_void_ret();
        return nullptr;
    } else {
        auto *ret_val = node.expression->accept(*this);
        auto *ret_type = context.func->get_return_type();
        if (ret_val->get_type()->is_float_type() &&
            ret_type->is_integer_type())
            ret_val = builder->create_fptosi(ret_val, ret_type);
        else if (ret_val->get_type()->is_integer_type() &&
                 ret_type->is_float_type())
            ret_val = builder->create_sitofp(ret_val, ret_type);
        builder->create_ret(ret_val);
    }
    return nullptr;
}

Value* CminusfBuilder::visit(ASTVar &node) {
    auto *addr = scope.find(node.id);
    if (node.expression != nullptr) {
        auto *index = node.expression->accept(*this);
        if (index->get_type()->is_float_type())
            index = builder->create_fptosi(index, INT32_T);

        auto *is_negative = builder->create_icmp_lt(index, CONST_INT(0));
        auto *error_bb = BasicBlock::create(
            module.get(), "idx_error_" + std::to_string(context.block_id++),
            context.func);
        auto *valid_bb = BasicBlock::create(
            module.get(), "idx_valid_" + std::to_string(context.block_id++),
            context.func);
        builder->create_cond_br(is_negative, error_bb, valid_bb);

        builder->set_insert_point(error_bb);
        builder->create_call(scope.find("neg_idx_except"),
                             std::vector<Value *>{});
        builder->create_br(valid_bb);
        builder->set_insert_point(valid_bb);

        auto *var_type = addr->get_type()->get_pointer_element_type();
        if (var_type->is_array_type())
            addr = builder->create_gep(addr, {CONST_INT(0), index});
        else if (var_type->is_pointer_type()) {
            addr = builder->create_load(addr);
            addr = builder->create_gep(addr, {index});
        } else
            addr = builder->create_gep(addr, {index});
    } else {
        auto *var_type = addr->get_type()->get_pointer_element_type();
        if (var_type->is_array_type())
            return builder->create_gep(addr, {CONST_INT(0), CONST_INT(0)});
        if (var_type->is_pointer_type())
            return builder->create_load(addr);
    }
    return builder->create_load(addr);
}

Value* CminusfBuilder::visit(ASTAssignExpression &node) {
    auto *addr = scope.find(node.var->id);
    if (node.var->expression != nullptr) {
        auto *index = node.var->expression->accept(*this);
        if (index->get_type()->is_float_type())
            index = builder->create_fptosi(index, INT32_T);

        auto *is_negative = builder->create_icmp_lt(index, CONST_INT(0));
        auto *error_bb = BasicBlock::create(
            module.get(), "idx_error_" + std::to_string(context.block_id++),
            context.func);
        auto *valid_bb = BasicBlock::create(
            module.get(), "idx_valid_" + std::to_string(context.block_id++),
            context.func);
        builder->create_cond_br(is_negative, error_bb, valid_bb);

        builder->set_insert_point(error_bb);
        builder->create_call(scope.find("neg_idx_except"),
                             std::vector<Value *>{});
        builder->create_br(valid_bb);
        builder->set_insert_point(valid_bb);

        auto *var_type = addr->get_type()->get_pointer_element_type();
        if (var_type->is_array_type())
            addr = builder->create_gep(addr, {CONST_INT(0), index});
        else if (var_type->is_pointer_type()) {
            addr = builder->create_load(addr);
            addr = builder->create_gep(addr, {index});
        } else
            addr = builder->create_gep(addr, {index});
    }

    auto *value = node.expression->accept(*this);
    auto *target_type = addr->get_type()->get_pointer_element_type();
    if (value->get_type()->is_float_type() && target_type->is_integer_type())
        value = builder->create_fptosi(value, target_type);
    else if (value->get_type()->is_integer_type() &&
             target_type->is_float_type())
        value = builder->create_sitofp(value, target_type);
    builder->create_store(value, addr);
    return value;
}

Value* CminusfBuilder::visit(ASTSimpleExpression &node) {
    auto *lhs = node.additive_expression_l->accept(*this);
    if (node.additive_expression_r == nullptr)
        return lhs;

    auto *rhs = node.additive_expression_r->accept(*this);
    if (lhs->get_type()->is_float_type() || rhs->get_type()->is_float_type()) {
        if (lhs->get_type()->is_integer_type())
            lhs = builder->create_sitofp(lhs, FLOAT_T);
        if (rhs->get_type()->is_integer_type())
            rhs = builder->create_sitofp(rhs, FLOAT_T);

        switch (node.op) {
        case OP_LE:
            lhs = builder->create_fcmp_le(lhs, rhs);
            break;
        case OP_LT:
            lhs = builder->create_fcmp_lt(lhs, rhs);
            break;
        case OP_GT:
            lhs = builder->create_fcmp_gt(lhs, rhs);
            break;
        case OP_GE:
            lhs = builder->create_fcmp_ge(lhs, rhs);
            break;
        case OP_EQ:
            lhs = builder->create_fcmp_eq(lhs, rhs);
            break;
        case OP_NEQ:
            lhs = builder->create_fcmp_ne(lhs, rhs);
            break;
        }
    } else {
        switch (node.op) {
        case OP_LE:
            lhs = builder->create_icmp_le(lhs, rhs);
            break;
        case OP_LT:
            lhs = builder->create_icmp_lt(lhs, rhs);
            break;
        case OP_GT:
            lhs = builder->create_icmp_gt(lhs, rhs);
            break;
        case OP_GE:
            lhs = builder->create_icmp_ge(lhs, rhs);
            break;
        case OP_EQ:
            lhs = builder->create_icmp_eq(lhs, rhs);
            break;
        case OP_NEQ:
            lhs = builder->create_icmp_ne(lhs, rhs);
            break;
        }
    }
    return builder->create_zext(lhs, INT32_T);
}

Value* CminusfBuilder::visit(ASTAdditiveExpression &node) {
    if (node.additive_expression == nullptr)
        return node.term->accept(*this);

    auto *lhs = node.additive_expression->accept(*this);
    auto *rhs = node.term->accept(*this);
    if (lhs->get_type()->is_float_type() || rhs->get_type()->is_float_type()) {
        if (lhs->get_type()->is_integer_type())
            lhs = builder->create_sitofp(lhs, FLOAT_T);
        if (rhs->get_type()->is_integer_type())
            rhs = builder->create_sitofp(rhs, FLOAT_T);
        if (node.op == OP_PLUS)
            return builder->create_fadd(lhs, rhs);
        return builder->create_fsub(lhs, rhs);
    }

    if (node.op == OP_PLUS)
        return builder->create_iadd(lhs, rhs);
    return builder->create_isub(lhs, rhs);
}

Value* CminusfBuilder::visit(ASTTerm &node) {
    if (node.term == nullptr)
        return node.factor->accept(*this);

    auto *lhs = node.term->accept(*this);
    auto *rhs = node.factor->accept(*this);
    if (lhs->get_type()->is_float_type() || rhs->get_type()->is_float_type()) {
        if (lhs->get_type()->is_integer_type())
            lhs = builder->create_sitofp(lhs, FLOAT_T);
        if (rhs->get_type()->is_integer_type())
            rhs = builder->create_sitofp(rhs, FLOAT_T);
        if (node.op == OP_MUL)
            return builder->create_fmul(lhs, rhs);
        return builder->create_fdiv(lhs, rhs);
    }

    if (node.op == OP_MUL)
        return builder->create_imul(lhs, rhs);
    return builder->create_isdiv(lhs, rhs);
}

Value* CminusfBuilder::visit(ASTCall &node) {
    auto *func = static_cast<Function *>(scope.find(node.id));
    auto *func_type = func->get_function_type();
    std::vector<Value *> args;
    unsigned int i = 0;
    for (auto &arg : node.args) {
        auto *arg_val = arg->accept(*this);
        if (i < func_type->get_num_of_args()) {
            auto *arg_type = func_type->get_param_type(i);
            if (arg_val->get_type()->is_float_type() &&
                arg_type->is_integer_type())
                arg_val = builder->create_fptosi(arg_val, arg_type);
            else if (arg_val->get_type()->is_integer_type() &&
                     arg_type->is_float_type())
                arg_val = builder->create_sitofp(arg_val, arg_type);
        }
        args.push_back(arg_val);
        ++i;
    }
    return builder->create_call(func, args);
}
