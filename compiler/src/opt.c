#include "opt.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    int known;
    long value;
} ConstVal;

typedef struct {
    IROp op;
    BinOp bin_op;
    int lhs;
    int rhs;
    int lhs_ver;
    int rhs_ver;
    long imm;
    int dst;
    const char *name;
} ExprEntry;

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

static int is_commutative(BinOp op) {
    switch (op) {
        case BIN_ADD:
        case BIN_MUL:
        case BIN_EQ:
        case BIN_NEQ:
        case BIN_LOGAND:
        case BIN_LOGOR:
            return 1;
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
                    if (inst->bin_op == BIN_MUL) {
                        int lhs_known = (inst->lhs >= 0 && inst->lhs <= max && vals[inst->lhs].known);
                        int rhs_known = (inst->rhs >= 0 && inst->rhs <= max && vals[inst->rhs].known);
                        if (lhs_known || rhs_known) {
                            int other = lhs_known ? inst->rhs : inst->lhs;
                            long c = lhs_known ? vals[inst->lhs].value : vals[inst->rhs].value;
                            if (c == 0) {
                                inst->op = IR_CONST;
                                inst->imm = 0;
                                inst->lhs = 0;
                                inst->rhs = 0;
                                vals[inst->dst].known = 1;
                                vals[inst->dst].value = 0;
                                break;
                            }
                            if (c == 1) {
                                inst->op = IR_MOV;
                                inst->lhs = other;
                                inst->rhs = 0;
                                if (other >= 0 && other <= max && vals[other].known) {
                                    vals[inst->dst].known = 1;
                                    vals[inst->dst].value = vals[other].value;
                                } else {
                                    vals[inst->dst].known = 0;
                                }
                                break;
                            }
                            if (c == 2) {
                                inst->bin_op = BIN_ADD;
                                inst->lhs = other;
                                inst->rhs = other;
                                vals[inst->dst].known = 0;
                                break;
                            }
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

static void clear_exprs(ExprEntry *entries, size_t *count) {
    *count = 0;
    (void)entries;
}

static int expr_match(const ExprEntry *e, IROp op, BinOp bin_op, int lhs, int rhs, int lhs_ver, int rhs_ver, long imm, const char *name) {
    if (e->op != op || e->bin_op != bin_op || e->lhs != lhs || e->rhs != rhs ||
        e->lhs_ver != lhs_ver || e->rhs_ver != rhs_ver || e->imm != imm) {
        if (op != IR_ADDR && e->op == op && e->bin_op == bin_op && e->imm == imm &&
            e->lhs_ver == lhs_ver && e->rhs_ver == rhs_ver) {
            return 1;
        }
        return 0;
    }
    if (op == IR_ADDR) {
        if (!e->name || !name) return 0;
        return strcmp(e->name, name) == 0;
    }
    return 1;
}

static void cse_range(IRInst *insts, size_t start, size_t end) {
    int max = max_temp_in_range(insts, start, end);
    if (max < 0) return;
    int *vn = (int *)calloc((size_t)max + 1, sizeof(int));
    if (!vn) return;
    int next_vn = 1;
    ExprEntry *entries = (ExprEntry *)calloc(end - start, sizeof(ExprEntry));
    if (!entries) { free(vn); return; }
    size_t entry_count = 0;

    for (size_t i = start; i < end; i++) {
        IRInst *inst = &insts[i];
        switch (inst->op) {
            case IR_LABEL:
            case IR_JMP:
            case IR_BZ:
            case IR_RET:
            case IR_EXIT:
                clear_exprs(entries, &entry_count);
                break;
            case IR_STORE:
            case IR_STORE8:
            case IR_CALL:
            case IR_WRITE:
                clear_exprs(entries, &entry_count);
                break;
            case IR_ADDR: {
                int found = -1;
                for (size_t e = 0; e < entry_count; e++) {
                    if (expr_match(&entries[e], IR_ADDR, 0, 0, 0, 0, 0, 0, inst->name)) { found = (int)e; break; }
                }
                if (found >= 0) {
                    inst->op = IR_MOV;
                    inst->lhs = entries[found].dst;
                    inst->rhs = 0;
                    if (inst->dst >= 0 && inst->dst <= max) vn[inst->dst] = vn[entries[found].dst];
                } else {
                    if (inst->dst >= 0 && inst->dst <= max) vn[inst->dst] = next_vn++;
                    if (entry_count < (end - start)) {
                        entries[entry_count].op = IR_ADDR;
                        entries[entry_count].bin_op = 0;
                        entries[entry_count].lhs = 0;
                        entries[entry_count].rhs = 0;
                        entries[entry_count].lhs_ver = 0;
                        entries[entry_count].rhs_ver = 0;
                        entries[entry_count].imm = 0;
                        entries[entry_count].dst = inst->dst;
                        entries[entry_count].name = inst->name;
                        entry_count++;
                    }
                }
                break;
            }
            case IR_LOAD:
            case IR_LOAD8: {
                int lhs = inst->lhs;
                int lhs_ver = (lhs >= 0 && lhs <= max) ? vn[lhs] : 0;
                int found = -1;
                for (size_t e = 0; e < entry_count; e++) {
                    if (expr_match(&entries[e], inst->op, 0, lhs, 0, lhs_ver, 0, 0, NULL)) { found = (int)e; break; }
                }
                if (found >= 0) {
                    inst->op = IR_MOV;
                    inst->lhs = entries[found].dst;
                    inst->rhs = 0;
                    if (inst->dst >= 0 && inst->dst <= max) vn[inst->dst] = vn[entries[found].dst];
                } else if (entry_count < (end - start)) {
                    entries[entry_count].op = inst->op;
                    entries[entry_count].bin_op = 0;
                    entries[entry_count].lhs = lhs;
                    entries[entry_count].rhs = 0;
                    entries[entry_count].lhs_ver = lhs_ver;
                    entries[entry_count].rhs_ver = 0;
                    entries[entry_count].imm = 0;
                    entries[entry_count].dst = inst->dst;
                    entries[entry_count].name = NULL;
                    entry_count++;
                }
                if (inst->dst >= 0 && inst->dst <= max && inst->op != IR_MOV) vn[inst->dst] = next_vn++;
                break;
            }
            case IR_BIN: {
                int lhs = inst->lhs;
                int rhs = inst->rhs;
                int lhs_ver = (lhs >= 0 && lhs <= max) ? vn[lhs] : 0;
                int rhs_ver = (rhs >= 0 && rhs <= max) ? vn[rhs] : 0;
                if (is_commutative(inst->bin_op)) {
                    if (lhs > rhs) {
                        int tmp = lhs; lhs = rhs; rhs = tmp;
                        tmp = lhs_ver; lhs_ver = rhs_ver; rhs_ver = tmp;
                    }
                }
                int found = -1;
                for (size_t e = 0; e < entry_count; e++) {
                    if (expr_match(&entries[e], IR_BIN, inst->bin_op, lhs, rhs, lhs_ver, rhs_ver, 0, NULL)) { found = (int)e; break; }
                }
                if (found >= 0) {
                    inst->op = IR_MOV;
                    inst->lhs = entries[found].dst;
                    inst->rhs = 0;
                    if (inst->dst >= 0 && inst->dst <= max) vn[inst->dst] = vn[entries[found].dst];
                } else if (entry_count < (end - start)) {
                    entries[entry_count].op = IR_BIN;
                    entries[entry_count].bin_op = inst->bin_op;
                    entries[entry_count].lhs = lhs;
                    entries[entry_count].rhs = rhs;
                    entries[entry_count].lhs_ver = lhs_ver;
                    entries[entry_count].rhs_ver = rhs_ver;
                    entries[entry_count].imm = 0;
                    entries[entry_count].dst = inst->dst;
                    entries[entry_count].name = NULL;
                    entry_count++;
                }
                if (inst->dst >= 0 && inst->dst <= max && inst->op != IR_MOV) vn[inst->dst] = next_vn++;
                break;
            }
            default:
                if (inst->op == IR_CONST && inst->dst >= 0 && inst->dst <= max) {
                    vn[inst->dst] = next_vn++;
                } else if (inst->op == IR_MOV && inst->dst >= 0 && inst->dst <= max) {
                    if (inst->lhs >= 0 && inst->lhs <= max) vn[inst->dst] = vn[inst->lhs];
                    else vn[inst->dst] = next_vn++;
                } else if (inst_has_dst(inst) && inst->dst >= 0 && inst->dst <= max) {
                    vn[inst->dst] = next_vn++;
                }
                break;
        }
    }

    free(entries);
    free(vn);
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
            cse_range(ir->insts, start, end);

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
