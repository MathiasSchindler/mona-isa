#include "opt.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    int known;
    long value;
} ConstVal;

static int max_temp_in_range(const IRInst *insts, size_t start, size_t end) {
    int max = -1;
    for (size_t i = start; i < end; i++) {
        const IRInst *inst = &insts[i];
        if (inst->dst > max) max = inst->dst;
        if (inst->lhs > max) max = inst->lhs;
        if (inst->rhs > max) max = inst->rhs;
        if (inst->op == IR_CALL && inst->args) {
            for (int a = 0; a < inst->argc; a++) if (inst->args[a] > max) max = inst->args[a];
        }
    }
    return max;
}

static void clear_consts(ConstVal *vals, int count) {
    for (int i = 0; i < count; i++) vals[i].known = 0;
}

static int compute_bin(BinOp op, long lhs, long rhs, long *out) {
    switch (op) {
        case BIN_ADD: *out = lhs + rhs; return 1;
        case BIN_SUB: *out = lhs - rhs; return 1;
        case BIN_MUL: *out = lhs * rhs; return 1;
        case BIN_DIV: if (rhs == 0) return 0; *out = lhs / rhs; return 1;
        case BIN_EQ: *out = (lhs == rhs) ? 1 : 0; return 1;
        case BIN_NEQ: *out = (lhs != rhs) ? 1 : 0; return 1;
        case BIN_LT: *out = (lhs < rhs) ? 1 : 0; return 1;
        case BIN_LTE: *out = (lhs <= rhs) ? 1 : 0; return 1;
        case BIN_GT: *out = (lhs > rhs) ? 1 : 0; return 1;
        case BIN_GTE: *out = (lhs >= rhs) ? 1 : 0; return 1;
        case BIN_LOGAND: *out = (lhs != 0 && rhs != 0) ? 1 : 0; return 1;
        case BIN_LOGOR: *out = (lhs != 0 || rhs != 0) ? 1 : 0; return 1;
        default:
            return 0;
    }
}

static void const_fold_range(IRInst *insts, size_t start, size_t end) {
    int max = max_temp_in_range(insts, start, end);
    if (max < 0) return;
    ConstVal *vals = (ConstVal *)calloc((size_t)max + 1, sizeof(ConstVal));
    if (!vals) return;

    for (size_t i = start; i < end; i++) {
        IRInst *inst = &insts[i];
        switch (inst->op) {
            case IR_LABEL:
                clear_consts(vals, max + 1);
                break;
            case IR_JMP:
            case IR_BZ:
            case IR_RET:
                clear_consts(vals, max + 1);
                break;
            case IR_CONST:
                if (inst->dst >= 0 && inst->dst <= max) {
                    vals[inst->dst].known = 1;
                    vals[inst->dst].value = inst->imm;
                }
                break;
            case IR_MOV:
                if (inst->dst >= 0 && inst->dst <= max) {
                    if (inst->lhs >= 0 && inst->lhs <= max && vals[inst->lhs].known) {
                        inst->op = IR_CONST;
                        inst->imm = vals[inst->lhs].value;
                        inst->lhs = 0;
                        if (inst->dst >= 0 && inst->dst <= max) {
                            vals[inst->dst].known = 1;
                            vals[inst->dst].value = inst->imm;
                        }
                    } else {
                        vals[inst->dst].known = 0;
                    }
                }
                break;
            case IR_BIN:
                if (inst->dst >= 0 && inst->dst <= max) {
                    if (inst->lhs >= 0 && inst->lhs <= max && inst->rhs >= 0 && inst->rhs <= max &&
                        vals[inst->lhs].known && vals[inst->rhs].known) {
                        long out = 0;
                        if (compute_bin(inst->bin_op, vals[inst->lhs].value, vals[inst->rhs].value, &out)) {
                            inst->op = IR_CONST;
                            inst->imm = out;
                            inst->lhs = 0;
                            inst->rhs = 0;
                            vals[inst->dst].known = 1;
                            vals[inst->dst].value = out;
                            break;
                        }
                    }
                    vals[inst->dst].known = 0;
                }
                break;
            case IR_ADDR:
            case IR_LOAD:
            case IR_LOAD8:
            case IR_PARAM:
            case IR_CALL:
            case IR_WRITE:
                if (inst->dst >= 0 && inst->dst <= max) vals[inst->dst].known = 0;
                break;
            default:
                break;
        }
    }

    free(vals);
}

static int inst_has_dst(const IRInst *inst) {
    switch (inst->op) {
        case IR_CONST:
        case IR_BIN:
        case IR_MOV:
        case IR_ADDR:
        case IR_LOAD:
        case IR_LOAD8:
        case IR_PARAM:
        case IR_CALL:
        case IR_WRITE:
            return 1;
        default:
            return 0;
    }
}

static int inst_has_side_effect(const IRInst *inst) {
    switch (inst->op) {
        case IR_STORE:
        case IR_STORE8:
        case IR_CALL:
        case IR_WRITE:
        case IR_RET:
        case IR_EXIT:
        case IR_LOCAL_ALLOC:
        case IR_LABEL:
        case IR_JMP:
        case IR_BZ:
        case IR_FUNC:
        case IR_GLOBAL_INT:
        case IR_GLOBAL_CHAR:
        case IR_GLOBAL_STR:
        case IR_GLOBAL_INT_ARR:
            return 1;
        default:
            return 0;
    }
}

static void mark_used(int *used, int max, int temp) {
    if (temp >= 0 && temp <= max) used[temp] = 1;
}

static void collect_uses(const IRInst *inst, int *used, int max) {
    switch (inst->op) {
        case IR_BIN:
            mark_used(used, max, inst->lhs);
            mark_used(used, max, inst->rhs);
            break;
        case IR_MOV:
        case IR_LOAD:
        case IR_LOAD8:
        case IR_BZ:
        case IR_RET:
        case IR_EXIT:
            mark_used(used, max, inst->lhs);
            break;
        case IR_STORE:
        case IR_STORE8:
            mark_used(used, max, inst->lhs);
            mark_used(used, max, inst->rhs);
            break;
        case IR_CALL:
            if (inst->args) {
                for (int a = 0; a < inst->argc; a++) mark_used(used, max, inst->args[a]);
            }
            break;
        case IR_WRITE:
            mark_used(used, max, inst->lhs);
            break;
        default:
            break;
    }
}

static size_t dce_range(IRInst *insts, size_t start, size_t end, IRInst *out, size_t out_cap) {
    int max = max_temp_in_range(insts, start, end);
    size_t count = end - start;
    unsigned char *keep = (unsigned char *)calloc(count, sizeof(unsigned char));
    int *used = NULL;
    if (max >= 0) used = (int *)calloc((size_t)max + 1, sizeof(int));
    if (!keep || (max >= 0 && !used)) {
        free(keep);
        free(used);
        return 0;
    }

    for (size_t i = count; i > 0; i--) {
        size_t idx = start + i - 1;
        IRInst *inst = &insts[idx];
        int keep_inst = inst_has_side_effect(inst);
        if (!keep_inst && inst_has_dst(inst) && inst->dst >= 0 && inst->dst <= max && used[inst->dst]) {
            keep_inst = 1;
        }
        if (keep_inst) collect_uses(inst, used, max);
        keep[i - 1] = (unsigned char)keep_inst;
    }

    size_t out_count = 0;
    for (size_t i = 0; i < count; i++) {
        IRInst *inst = &insts[start + i];
        if (keep[i]) {
            if (out_count < out_cap) out[out_count++] = *inst;
        } else {
            if (inst->op == IR_CALL) free(inst->args);
            if (inst->op == IR_GLOBAL_STR) free(inst->data);
            if (inst->op == IR_GLOBAL_INT_ARR) free(inst->values);
        }
    }

    free(keep);
    free(used);
    return out_count;
}

void ir_optimize(IRProgram *ir) {
    if (!ir || !ir->insts || ir->count == 0) return;

    IRInst *new_insts = (IRInst *)malloc(ir->count * sizeof(IRInst));
    if (!new_insts) return;

    size_t out_count = 0;
    size_t i = 0;
    while (i < ir->count) {
        IRInst *inst = &ir->insts[i];
        if (inst->op == IR_GLOBAL_INT || inst->op == IR_GLOBAL_STR || inst->op == IR_GLOBAL_INT_ARR) {
            new_insts[out_count++] = *inst;
            i++;
            continue;
        }
        if (inst->op == IR_FUNC) {
            size_t start = i + 1;
            size_t end = start;
            while (end < ir->count && ir->insts[end].op != IR_FUNC) end++;
            const_fold_range(ir->insts, start, end);

            new_insts[out_count++] = *inst;
            size_t kept = dce_range(ir->insts, start, end, new_insts + out_count, ir->count - out_count);
            out_count += kept;
            i = end;
            continue;
        }
        new_insts[out_count++] = *inst;
        i++;
    }

    free(ir->insts);
    ir->insts = new_insts;
    ir->count = out_count;
    ir->cap = out_count;
}
