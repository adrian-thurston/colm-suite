/*
 * Copyright 2007-2018 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _COLM_BYTECODE_H
#define _COLM_BYTECODE_H

#include <colm/pdarun.h>
#include <colm/type.h>
#include <colm/tree.h>

#ifdef __cplusplus
extern "C" {
#endif

#if SIZEOF_LONG != 4 && SIZEOF_LONG != 8
	#error "SIZEOF_LONG contained an unexpected value"
#endif

typedef unsigned long ulong;
typedef unsigned char uchar;

enum INSTRCT_BYTECODE {
	IN_NONE,
	IN_LOAD_INT,
	IN_LOAD_STR,
	IN_LOAD_NIL,
	IN_LOAD_TRUE,
	IN_LOAD_FALSE,
	IN_LOAD_TREE,
	IN_LOAD_WORD,

	IN_ADD_INT,
	IN_SUB_INT,
	IN_MULT_INT,
	IN_DIV_INT,

	IN_TST_EQL_VAL,
	IN_TST_EQL_TREE,
	IN_TST_NOT_EQL_TREE,
	IN_TST_NOT_EQL_VAL,
	IN_TST_LESS_VAL,
	IN_TST_LESS_TREE,
	IN_TST_GRTR_VAL,
	IN_TST_GRTR_TREE,
	IN_TST_LESS_EQL_VAL,
	IN_TST_LESS_EQL_TREE,
	IN_TST_GRTR_EQL_VAL,
	IN_TST_GRTR_EQL_TREE,
	IN_TST_LOGICAL_AND,
	IN_TST_LOGICAL_OR,

	IN_TST_NZ_TREE,

	IN_LOAD_RETVAL,

	IN_STASH_ARG,
	IN_PREP_ARGS,
	IN_CLEAR_ARGS,

	IN_GEN_ITER_FROM_REF,
	IN_GEN_ITER_DESTROY,
	IN_GEN_ITER_UNWIND,
	IN_GEN_ITER_GET_CUR_R,
	IN_GEN_VITER_GET_CUR_R,
	IN_LIST_ITER_ADVANCE,
	IN_REV_LIST_ITER_ADVANCE,
	IN_MAP_ITER_ADVANCE,

	IN_NOT_VAL,
	IN_NOT_TREE,

	IN_JMP,
	IN_JMP_FALSE_TREE,
	IN_JMP_TRUE_TREE,
	IN_JMP_FALSE_VAL,
	IN_JMP_TRUE_VAL,

	IN_STR_LENGTH,
	IN_CONCAT_STR,
	IN_TREE_TRIM,

	IN_POP_TREE,
	IN_POP_N_WORDS,
	IN_POP_VAL,
	IN_DUP_VAL,
	IN_DUP_TREE,

	IN_REJECT,
	IN_MATCH,
	IN_PROD_NUM,
	IN_CONSTRUCT,
	IN_CONS_OBJECT,
	IN_CONS_GENERIC,
	IN_TREE_CAST,

	IN_GET_LOCAL_R,
	IN_GET_LOCAL_WC,
	IN_SET_LOCAL_WC,

	IN_GET_LOCAL_REF_R,
	IN_GET_LOCAL_REF_WC,
	IN_SET_LOCAL_REF_WC,

	IN_SAVE_RET,

	IN_GET_FIELD_TREE_R,
	IN_GET_FIELD_TREE_WC,
	IN_GET_FIELD_TREE_WV,
	IN_GET_FIELD_TREE_BKT,

	IN_SET_FIELD_TREE_WV,
	IN_SET_FIELD_TREE_WC,
	IN_SET_FIELD_TREE_BKT,
	IN_SET_FIELD_TREE_LEAVE_WC,

	IN_GET_FIELD_VAL_R,
	IN_SET_FIELD_VAL_WC,

	IN_GET_MATCH_LENGTH_R,
	IN_GET_MATCH_TEXT_R,

	IN_GET_TOKEN_DATA_R,
	IN_SET_TOKEN_DATA_WC,
	IN_SET_TOKEN_DATA_WV,
	IN_SET_TOKEN_DATA_BKT,

	IN_GET_TOKEN_FILE_R,
	IN_GET_TOKEN_LINE_R,
	IN_GET_TOKEN_POS_R,
	IN_GET_TOKEN_COL_R,

	IN_INIT_RHS_EL,
	IN_INIT_LHS_EL,
	IN_INIT_CAPTURES,
	IN_STORE_LHS_EL,
	IN_RESTORE_LHS,

	IN_TRITER_FROM_REF,
	IN_TRITER_ADVANCE,
	IN_TRITER_WIG_ADVANCE,
	IN_TRITER_NEXT_CHILD,
	IN_TRITER_GET_CUR_R,
	IN_TRITER_GET_CUR_WC,
	IN_TRITER_SET_CUR_WC,
	IN_TRITER_UNWIND,
	IN_TRITER_DESTROY,
	IN_TRITER_NEXT_REPEAT,
	IN_TRITER_PREV_REPEAT,

	IN_REV_TRITER_FROM_REF,
	IN_REV_TRITER_DESTROY,
	IN_REV_TRITER_UNWIND,
	IN_REV_TRITER_PREV_CHILD,

	IN_UITER_DESTROY,
	IN_UITER_UNWIND,
	IN_UITER_CREATE_WV,
	IN_UITER_CREATE_WC,
	IN_UITER_ADVANCE,
	IN_UITER_GET_CUR_R,
	IN_UITER_GET_CUR_WC,
	IN_UITER_SET_CUR_WC,

	IN_TREE_SEARCH,

	IN_LOAD_GLOBAL_R,
	IN_LOAD_GLOBAL_WV,
	IN_LOAD_GLOBAL_WC,
	IN_LOAD_GLOBAL_BKT,

	IN_PTR_ACCESS_WV,
	IN_PTR_ACCESS_BKT,

	IN_REF_FROM_LOCAL,
	IN_REF_FROM_REF,
	IN_REF_FROM_QUAL_REF,
	IN_RHS_REF_FROM_QUAL_REF,
	IN_REF_FROM_BACK,
	IN_TRITER_REF_FROM_CUR,
	IN_UITER_REF_FROM_CUR,

	IN_GET_MAP_EL_MEM_R,

	IN_MAP_LENGTH,

	IN_LIST_LENGTH,

	IN_GET_LIST_MEM_R,
	IN_GET_LIST_MEM_WC,
	IN_GET_LIST_MEM_WV,
	IN_GET_LIST_MEM_BKT,

	IN_GET_VLIST_MEM_R,
	IN_GET_VLIST_MEM_WC,
	IN_GET_VLIST_MEM_WV,
	IN_GET_VLIST_MEM_BKT,

	IN_CONS_REDUCER,
	IN_READ_REDUCE,

	IN_DONE,

	IN_GET_LIST_EL_MEM_R,

	IN_GET_MAP_MEM_R,
	IN_GET_MAP_MEM_WV,
	IN_GET_MAP_MEM_WC,
	IN_GET_MAP_MEM_BKT,

	IN_TREE_TO_STR_XML,
	IN_TREE_TO_STR_XML_AC,
	IN_TREE_TO_STR_POSTFIX,

	IN_HOST,

	IN_CALL_WC,
	IN_CALL_WV,
	IN_RET,
	IN_YIELD,
	IN_HALT,

	IN_INT_TO_STR,
	IN_TREE_TO_STR,
	IN_TREE_TO_STR_TRIM,
	IN_TREE_TO_STR_TRIM_A,

	IN_CREATE_TOKEN,
	IN_MAKE_TOKEN,
	IN_MAKE_TREE,
	IN_CONSTRUCT_TERM,

	IN_INPUT_PULL_WV,
	IN_INPUT_PULL_WC,
	IN_INPUT_PULL_BKT,

	IN_INPUT_CLOSE_WC,
	IN_INPUT_AUTO_TRIM_WC,
	IN_IINPUT_AUTO_TRIM_WC,

	IN_PARSE_FRAG_W,
	IN_PARSE_INIT_BKT,
	IN_PARSE_FRAG_BKT,

	IN_PRINT_TREE,

	IN_SEND_NOTHING,
	IN_SEND_TEXT_W,
	IN_SEND_TEXT_BKT,

	IN_SEND_TREE_W,
	IN_SEND_TREE_BKT,

	IN_SEND_STREAM_W,
	IN_SEND_STREAM_BKT,

	IN_SEND_EOF_W,
	IN_SEND_EOF_BKT,

	IN_REDUCE_COMMIT,

	IN_PCR_RET,
	IN_PCR_END_DECK,

	IN_OPEN_FILE,

	IN_GET_CONST,

	IN_TO_UPPER,
	IN_TO_LOWER,

	IN_LOAD_INPUT_R,
	IN_LOAD_INPUT_WV,
	IN_LOAD_INPUT_WC,
	IN_LOAD_INPUT_BKT,

	IN_INPUT_PUSH_WV,
	IN_INPUT_PUSH_BKT,
	IN_INPUT_PUSH_IGNORE_WV,

	IN_INPUT_PUSH_STREAM_WV,
	IN_INPUT_PUSH_STREAM_BKT,

	IN_LOAD_CONTEXT_R,
	IN_LOAD_CONTEXT_WV,
	IN_LOAD_CONTEXT_WC,
	IN_LOAD_CONTEXT_BKT,

	IN_SET_PARSER_CONTEXT,
	IN_SET_PARSER_INPUT,

	IN_GET_RHS_VAL_R,
	IN_GET_RHS_VAL_WC,
	IN_GET_RHS_VAL_WV,
	IN_GET_RHS_VAL_BKT,
	IN_SET_RHS_VAL_WC,
	IN_SET_RHS_VAL_WV,
	IN_SET_RHS_VAL_BKT,

	IN_GET_PARSER_MEM_R,

	IN_GET_STREAM_MEM_R,

	IN_GET_PARSER_STREAM,

	IN_GET_ERROR,
	IN_SET_ERROR,

	IN_SYSTEM,

	IN_GET_STRUCT_R,
	IN_GET_STRUCT_WC,
	IN_GET_STRUCT_WV,
	IN_GET_STRUCT_BKT,
	IN_SET_STRUCT_WC,
	IN_SET_STRUCT_WV,
	IN_SET_STRUCT_BKT,
	IN_GET_STRUCT_VAL_R,
	IN_SET_STRUCT_VAL_WV,
	IN_SET_STRUCT_VAL_WC,
	IN_SET_STRUCT_VAL_BKT,
	IN_NEW_STRUCT,

	IN_GET_LOCAL_VAL_R,
	IN_SET_LOCAL_VAL_WC,

	IN_NEW_STREAM,
	IN_GET_COLLECT_STRING,

	IN_FN,
};

/*
 * Const things to get.
 */
#define CONST_STDIN           0x10
#define CONST_STDOUT          0x11
#define CONST_STDERR          0x12
#define CONST_ARG             0x13



/*
 * IN_FN instructions.
 */
enum FN_FUNCS {
	FN_NONE,
	FN_STOP,

	FN_STR_ATOI,
	FN_STR_ATOO,
	FN_STR_UORD8,
	FN_STR_SORD8,
	FN_STR_UORD16,
	FN_STR_SORD16,
	FN_STR_UORD32,
	FN_STR_SORD32,
	FN_STR_PREFIX,
	FN_STR_SUFFIX,
	FN_SPRINTF,
	FN_LOAD_ARGV,
	FN_LOAD_ARG0,
	FN_INIT_STDS,


	FN_LIST_PUSH_TAIL_WV,
	FN_LIST_PUSH_TAIL_WC,
	FN_LIST_PUSH_TAIL_BKT,
	FN_LIST_POP_TAIL_WV,
	FN_LIST_POP_TAIL_WC,
	FN_LIST_POP_TAIL_BKT,
	FN_LIST_PUSH_HEAD_WV,
	FN_LIST_PUSH_HEAD_WC,
	FN_LIST_PUSH_HEAD_BKT,
	FN_LIST_POP_HEAD_WV,
	FN_LIST_POP_HEAD_WC,
	FN_LIST_POP_HEAD_BKT,

	FN_MAP_FIND,
	FN_MAP_INSERT_WV,
	FN_MAP_INSERT_WC,
	FN_MAP_INSERT_BKT,
	FN_MAP_DETACH_WV,
	FN_MAP_DETACH_WC,
	FN_MAP_DETACH_BKT,

	FN_VMAP_FIND,
	FN_VMAP_INSERT_WC,
	FN_VMAP_INSERT_WV,
	FN_VMAP_INSERT_BKT,
	FN_VMAP_REMOVE_WC,
	FN_VMAP_REMOVE_WV,
	FN_VMAP_REMOVE_BKT,

	FN_VLIST_PUSH_TAIL_WV,
	FN_VLIST_PUSH_TAIL_WC,
	FN_VLIST_PUSH_TAIL_BKT,
	FN_VLIST_POP_TAIL_WV,
	FN_VLIST_POP_TAIL_WC,
	FN_VLIST_POP_TAIL_BKT,
	FN_VLIST_PUSH_HEAD_WV,
	FN_VLIST_PUSH_HEAD_WC,
	FN_VLIST_PUSH_HEAD_BKT,
	FN_VLIST_POP_HEAD_WV,
	FN_VLIST_POP_HEAD_WC,
	FN_VLIST_POP_HEAD_BKT,
	FN_EXIT,
	FN_EXIT_HARD,
	FN_PREFIX,
	FN_SUFFIX,
};

#define TRIM_DEFAULT 0x01
#define TRIM_YES     0x02
#define TRIM_NO      0x03

/* Types of Generics. */
enum GEN {
	GEN_PARSER   = 0x14,
	GEN_LIST     = 0x15,
	GEN_MAP      = 0x16
};

/* Known language element ids. */
enum LEL_ID {
	LEL_ID_PTR        = 1,
	LEL_ID_STR        = 2,
	LEL_ID_IGNORE     = 3
};

/*
 * Flags
 */

/* A tree that has been generated by a termDup. */
#define PF_TERM_DUP            0x0001

/* Has been processed by the commit function. All children have also been
 * processed. */
#define PF_COMMITTED           0x0002

/* Created by a token generation action, not made from the input. */
#define PF_ARTIFICIAL          0x0004

/* Named node from a pattern or constructor. */
#define PF_NAMED               0x0008

/* There is reverse code associated with this tree node. */
#define PF_HAS_RCODE           0x0010

#define PF_RIGHT_IGNORE        0x0020

#define PF_LEFT_IL_ATTACHED    0x0400
#define PF_RIGHT_IL_ATTACHED   0x0800

#define AF_LEFT_IGNORE   0x0100
#define AF_RIGHT_IGNORE  0x0200

#define AF_SUPPRESS_LEFT  0x4000
#define AF_SUPPRESS_RIGHT 0x8000

/*
 * Call stack.
 */

/* Number of spots in the frame, after the args. */
#define FR_AA 5

/* Positions relative to the frame pointer. */
#define FR_CA  4    /* call args */
#define FR_RV  3    /* return value */
#define FR_RI  2    /* return instruction */
#define FR_RFP 1    /* return frame pointer */
#define FR_RFD 0    /* return frame id. */

/*
 * Calling Convention:
 *   a1
 *   a2
 *   a3
 *   ...
 *   return value      FR_RV
 *   return instr      FR_RI
 *   return frame ptr  FR_RFP
 *   return frame id   FR_RFD
 */

/*
 * User iterator call stack.
 * Adds an iframe pointer, removes the return value.
 */

/* Number of spots in the frame, after the args. */
#define IFR_AA  5

/* Positions relative to the frame pointer. */
#define IFR_RIN 2    /* return instruction */
#define IFR_RIF 1    /* return iframe pointer */
#define IFR_RFR 0    /* return frame pointer */

#define vm_push_type(type, i) \
	( ( sp == prg->sb_beg ? (sp = vm_bs_add(prg, sp, 1)) : 0 ), (*((type*)(--sp)) = (i)) )

#define vm_pushn(n) \
	( ( (sp-(n)) < prg->sb_beg ? (sp = vm_bs_add(prg, sp, n)) : 0 ), (sp -= (n)) )

#define vm_pop_type(type) \
	({ type r = *((type*)sp); (sp+1) >= prg->sb_end ? (sp = vm_bs_pop(prg, sp, 1)) : (sp += 1); r; })

#define vm_push_tree(i)   vm_push_type(tree_t*, i)
#define vm_push_input(i)  vm_push_type(input_t*, i)
#define vm_push_stream(i) vm_push_type(stream_t*, i)
#define vm_push_struct(i) vm_push_type(struct_t*, i)
#define vm_push_parser(i) vm_push_type(parser_t*, i)
#define vm_push_value(i)  vm_push_type(value_t, i)
#define vm_push_string(i) vm_push_type(str_t*, i)
#define vm_push_kid(i)    vm_push_type(kid_t*, i)
#define vm_push_ref(i)    vm_push_type(ref_t*, i)
#define vm_push_string(i) vm_push_type(str_t*, i)
#define vm_push_ptree(i)  vm_push_type(parse_tree_t*, i)

#define vm_pop_tree()   vm_pop_type(tree_t*)
#define vm_pop_input()  vm_pop_type(input_t*)
#define vm_pop_stream() vm_pop_type(stream_t*)
#define vm_pop_struct() vm_pop_type(struct_t*)
#define vm_pop_parser() vm_pop_type(parser_t*)
#define vm_pop_list()   vm_pop_type(list_t*)
#define vm_pop_map()    vm_pop_type(map_t*)
#define vm_pop_value()  vm_pop_type(value_t)
#define vm_pop_string() vm_pop_type(str_t*)
#define vm_pop_kid()    vm_pop_type(kid_t*)
#define vm_pop_ref()    vm_pop_type(ref_t*)
#define vm_pop_ptree()  vm_pop_type(parse_tree_t*)

#define vm_pop_ignore() \
	({ (sp+1) >= prg->sb_end ? (sp = vm_bs_pop(prg, sp, 1)) : (sp += 1); })

#define vm_popn(n) \
	({ (sp+(n)) >= prg->sb_end ? (sp = vm_bs_pop(prg, sp, n)) : (sp += (n)); })

#define vm_contiguous(n) \
	( ( (sp-(n)) < prg->sb_beg ? (sp = vm_bs_add(prg, sp, n)) : 0 ) )

#define vm_top() (*sp)
#define vm_ptop() (sp)

#define vm_ssize()       ( prg->sb_total + (prg->sb_end - sp) )

#define vm_local_iframe(o) (exec->iframe_ptr[o])
#define vm_plocal_iframe(o) (&exec->iframe_ptr[o])

void vm_init( struct colm_program * );
tree_t** vm_bs_add( struct colm_program *, tree_t **, int );
tree_t** vm_bs_pop( struct colm_program *, tree_t **, int );
void vm_clear( struct colm_program * );

typedef tree_t *SW;
typedef tree_t **StackPtr;

/* Can't use sizeof() because we have used types that are bigger than the
 * serial representation. */
#define SIZEOF_CODE 1
#define SIZEOF_HALF 2
#define SIZEOF_WORD sizeof(word_t)

typedef struct colm_execution
{
	tree_t **frame_ptr;
	tree_t **iframe_ptr;
	long frame_id;
	tree_t **call_args;

	long rcode_unit_len;

	parser_t *parser;
	long steps;
	long pcr;
	tree_t *ret_val;
	char WV;
} execution_t;

struct colm_execution;

static inline tree_t **vm_get_plocal( struct colm_execution *exec, int o )
{
	if ( o >= FR_AA ) {
		tree_t **call_args = (tree_t**)exec->frame_ptr[FR_CA];
		return &call_args[o - FR_AA];
	}
	else {
		return &exec->frame_ptr[o];
	}
}

static inline tree_t *vm_get_local( struct colm_execution *exec, int o )
{
	if ( o >= FR_AA ) {
		tree_t **call_args = (tree_t**)exec->frame_ptr[FR_CA];
		return call_args[o - FR_AA];
	}
	else {
		return exec->frame_ptr[o];
	}
}

static inline void vm_set_local( struct colm_execution *exec, int o, tree_t* v )
{
	if ( o >= FR_AA ) {
		tree_t **call_args = (tree_t**)exec->frame_ptr[FR_CA];
		call_args[o - FR_AA] = v;
	}
	else {
		exec->frame_ptr[o] = v;
	}
}


long string_length( head_t *str );
const char *string_data( head_t *str );
head_t *init_str_space( long length );
head_t *string_copy( struct colm_program *prg, head_t *head );
void string_free( struct colm_program *prg, head_t *head );
void string_shorten( head_t *tokdata, long newlen );
head_t *concat_str( head_t *s1, head_t *s2 );
word_t str_atoi( head_t *str );
word_t str_atoo( head_t *str );
word_t str_uord16( head_t *head );
word_t str_uord8( head_t *head );
word_t cmp_string( head_t *s1, head_t *s2 );
head_t *string_to_upper( head_t *s );
head_t *string_to_lower( head_t *s );
head_t *string_sprintf( program_t *prg, str_t *format, long integer );

head_t *make_literal( struct colm_program *prg, long litoffset );
head_t *int_to_str( struct colm_program *prg, word_t i );

void colm_execute( struct colm_program *prg, execution_t *exec, code_t *code );

kid_t *alloc_attrs( struct colm_program *prg, long length );
void free_attrs( struct colm_program *prg, kid_t *attrs );
kid_t *get_attr_kid( tree_t *tree, long pos );

tree_t *split_tree( struct colm_program *prg, tree_t *t );

void colm_rcode_downref_all( struct colm_program *prg, tree_t **sp, struct rt_code_vect *cv );
int colm_make_reverse_code( struct pda_run *pda_run );
void colm_transfer_reverse_code( struct pda_run *pda_run, parse_tree_t *tree );

void split_ref( struct colm_program *prg, tree_t ***sp, ref_t *from_ref );

tree_t **colm_execute_code( struct colm_program *prg,
	execution_t *exec, tree_t **sp, code_t *instr );
code_t *colm_pop_reverse_code( struct rt_code_vect *all_rev );

#ifdef __cplusplus
}
#endif

#endif /* _COLM_BYTECODE_H */

