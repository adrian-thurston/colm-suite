/*
 * Copyright 2016-2018 Adrian Thurston <thurston@colm.net>
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

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <colm/program.h>
#include <colm/struct.h>

#include "internal.h"
#include "bytecode.h"

struct colm_tree *colm_get_global( program_t *prg, long pos )
{
	return colm_struct_get_field( prg->global, tree_t*, pos );
}

void colm_struct_add( program_t *prg, struct colm_struct *item )
{
	if ( prg->heap.head == 0 ) {
		prg->heap.head = prg->heap.tail = item;
		item->prev = item->next = 0;
	}
	else {
		item->prev = prg->heap.tail;
		item->next = 0;
		prg->heap.tail->next = item;
		prg->heap.tail = item;
	}
}

/* Takes a struct off the program's heap list, the inverse of
 * colm_struct_add(). */
static void heap_unlink( program_t *prg, struct colm_struct *s )
{
	if ( s->prev == 0 )
		prg->heap.head = s->next;
	else
		s->prev->next = s->next;

	if ( s->next == 0 )
		prg->heap.tail = s->prev;
	else
		s->next->prev = s->prev;

	s->prev = s->next = 0;
}

struct colm_struct *colm_struct_new_size( program_t *prg, int size )
{
	size_t memsize = sizeof(struct colm_struct) + ( sizeof(tree_t*) * size );
	struct colm_struct *item = (struct colm_struct*) malloc( memsize );
	memset( item, 0, memsize );

	colm_struct_add( prg, item );
	return item;
}

struct colm_struct *colm_struct_new( program_t *prg, int id )
{
	struct colm_struct *s = colm_struct_new_size( prg, prg->rtd->sel_info[id - prg->rtd->num_lang_els].size );
	s->id = id;
	return s;
}

/* A list's or a map's element, made as colm_struct_new() makes a struct but
 * kept off the heap list. Only its list or map holds it, and frees it, so it
 * isn't counted: its refs stays 0. */
struct colm_struct *colm_struct_new_el( program_t *prg, int id )
{
	size_t memsize = sizeof(struct colm_struct) + ( sizeof(tree_t*) *
			prg->rtd->sel_info[id - prg->rtd->num_lang_els].size );
	struct colm_struct *item = (struct colm_struct*) malloc( memsize );
	memset( item, 0, memsize );
	item->id = id;
	return item;
}

static struct struct_el_info *colm_sel_info( program_t *prg, int id )
{
	return &prg->rtd->sel_info[id - prg->rtd->num_lang_els];
}

void colm_struct_upref( program_t *prg, struct colm_struct *s )
{
	if ( s != 0 ) {
		assert( s->id >= prg->rtd->first_struct_el_id );
		s->refs += 1;
	}
}

/* Step 1 of freeing a struct: its weak references are set to nil, so that
 * none reads it from here on. Nothing makes a weak reference yet. #670's
 * colm_weak_clear() (weak-reference-design.md) replaces this when weak
 * references are built. */
static void weak_clear( program_t *prg, struct colm_struct *s )
{
}

/* Releases a strong reference to s. The last one frees it: s goes on the list
 * of structs to free, and the outermost downref frees what is on the list,
 * one at a time, each releasing what it holds. */
void colm_struct_downref( program_t *prg, tree_t **sp, struct colm_struct *s )
{
	/* At exit colm_clear_heap() frees every struct in turn, and may have
	 * freed s already. */
	if ( s == 0 || prg->clearing_heap )
		return;

	assert( s->id >= prg->rtd->first_struct_el_id );
	assert( s->refs > 0 );
	s->refs -= 1;
	if ( s->refs > 0 )
		return;

	weak_clear( prg, s );
	heap_unlink( prg, s );
	s->next = prg->free_structs;
	prg->free_structs = s;

	/* A teardown that takes another struct to 0 puts it on the list, and
	 * this loop frees it, so freeing never recurses from struct to struct. */
	if ( !prg->freeing_structs ) {
		prg->freeing_structs = 1;
		while ( prg->free_structs != 0 ) {
			struct colm_struct *f = prg->free_structs;
			prg->free_structs = f->next;
			colm_struct_delete( prg, sp, f );
		}
		prg->freeing_structs = 0;
	}
}

void colm_struct_delete( program_t *prg, tree_t **sp, struct colm_struct *el )
{
	if ( el->id == prg->rtd->struct_inbuilt_id ||
			el->id == prg->rtd->struct_input_id ||
			el->id == prg->rtd->struct_stream_id ) {
		colm_destructor_t destructor = ((struct colm_inbuilt*)el)->destructor;
		if ( destructor != 0 )
			(*destructor)( prg, sp, el );
	}
	else {
		int tree_i;
		struct struct_el_info *sel = colm_sel_info( prg, el->id );
		for ( tree_i = 0; tree_i < sel->trees_len; tree_i++ ) {
			tree_t *tree = colm_struct_get_field( el, tree_t*, sel->trees[tree_i] );
			colm_tree_downref( prg, sp, tree );
		}

		int struct_i;
		for ( struct_i = 0; struct_i < sel->structs_len; struct_i++ ) {
			struct colm_struct *s = colm_struct_get_field( el,
					struct colm_struct*, sel->structs[struct_i] );
			colm_struct_downref( prg, sp, s );
		}
	}
	free( el );
}

static void colm_parser_destroy( program_t *prg, tree_t **sp, struct colm_struct *s )
{
	struct colm_parser *parser = (struct colm_parser*) s;

	/* The context, taken before its PDA run goes. */
	struct colm_struct *context = parser->pda_run->context;

	/* Free the PDA run. */
	colm_pda_clear( prg, sp, parser->pda_run );
	free( parser->pda_run );

	/* Free the result. */
	colm_tree_downref( prg, sp, parser->result );

	/* Release the context and the input. Nothing counts either until #668
	 * counts a parser's context and input, so until then this runs only at
	 * exit, where the downref does nothing. */
	colm_struct_downref( prg, sp, context );
	colm_struct_downref( prg, sp, (struct colm_struct*)parser->input );
}

parser_t *colm_parser_new( program_t *prg, struct generic_info *gi, int stop_id, int reducer )
{
	struct pda_run *pda_run = malloc( sizeof(struct pda_run) );

	/* Start off the parsing process. */
	colm_pda_init( prg, pda_run, prg->rtd->pda_tables, 
			gi->parser_id, stop_id, 0, 0, reducer );
	
	size_t memsize = sizeof(struct colm_parser);
	struct colm_parser *parser = (struct colm_parser*) malloc( memsize );
	memset( parser, 0, memsize );
	colm_struct_add( prg, (struct colm_struct*) parser );

	parser->id = prg->rtd->struct_inbuilt_id;
	parser->destructor = &colm_parser_destroy;
	parser->pda_run = pda_run;

	return parser;
}

/* Frees the map's elements, in key order, with what each holds. */
static void colm_map_destroy( struct colm_program *prg, tree_t **sp, struct colm_struct *s )
{
	map_t *map = (map_t*)s;
	long gen_id = map->generic_info - prg->rtd->generic_info;
	map_el_t *map_el = map->head;
	while ( map_el != 0 ) {
		map_el_t *next = map_el->next;
		colm_struct_delete( prg, sp, colm_generic_el_container( prg, map_el, gen_id ) );
		map_el = next;
	}
}

map_t *colm_map_new( struct colm_program *prg )
{
	size_t memsize = sizeof(struct colm_map);
	struct colm_map *map = (struct colm_map*) malloc( memsize );
	memset( map, 0, memsize );
	colm_struct_add( prg, (struct colm_struct *)map );
	map->id = prg->rtd->struct_inbuilt_id;
	map->destructor = &colm_map_destroy;
	return map;
}

struct_t *colm_construct_generic( program_t *prg, long generic_id, int stop_id )
{
	struct generic_info *generic_info = &prg->rtd->generic_info[generic_id];
	struct_t *new_generic = 0;
	switch ( generic_info->type ) {
		case GEN_MAP: {
			map_t *map = colm_map_new( prg );
			map->generic_info = generic_info;
			new_generic = (struct_t*) map;
			break;
		}
		case GEN_LIST: {
			list_t *list = colm_list_new( prg );
			list->generic_info = generic_info;
			new_generic = (struct_t*) list;
			break;
		}
		case GEN_PARSER: {
			parser_t *parser = colm_parser_new( prg, generic_info, stop_id, 0 );
			parser->input = colm_input_new( prg );
			new_generic = (struct_t*) parser;
			break;
		}
	}

	return new_generic;
}

struct_t *colm_construct_reducer( program_t *prg, long generic_id, int reducer_id )
{
	struct generic_info *generic_info = &prg->rtd->generic_info[generic_id];
	struct_t *new_generic = 0;

	parser_t *parser = colm_parser_new( prg, generic_info, 0, reducer_id );
	parser->input = colm_input_new( prg );
	new_generic = (struct_t*) parser;

	return new_generic;
}
