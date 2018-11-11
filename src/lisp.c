#include <stdlib.h>
#include <string.h>

#include "libisp.h"

#include "defs.h"
#include "irc.h"
#include "util.h"
#include "conf.h"

#define THREAD_TIMEOUT	2
#define MAX_CTX			8
#define MAX_NAME		16

typedef struct ctx_list_t {
	char *name;
	lisp_ctx_t *context;
	struct ctx_list_t *next;
	struct ctx_list_t *last;
} ctx_list_t;

static lisp_ctx_t *global_ctx;
static int n_ctx = 0;
static ctx_list_t *ctx_list = NULL;
static ctx_list_t *ctx_list_head = NULL;

int is_compound_procedure(const lisp_data_t *exp);

static ctx_list_t *new_node(const char *name,
							const size_t mem_lim_soft, 
							const size_t mem_lim_hard) {
	ctx_list_t *newnode;

	if((newnode = malloc(sizeof(ctx_list_t))) == NULL)
		return NULL;

	if((newnode->name = malloc(MAX_NAME + 1)) == NULL) {
		free(newnode);
		return NULL;
	}

	strncpy(newnode->name, name, MAX_NAME);
	if((newnode->context = lisp_make_context(mem_lim_soft, mem_lim_hard, 
										LISP_GC_SILENT, THREAD_TIMEOUT)) == NULL) {
		free(newnode->name);
		free(newnode);
		return NULL;
	}

	lisp_setup_env(newnode->context);
	newnode->next = NULL;
	return newnode;						
}

static int create_ctx(const char *name, 
		const size_t mem_lim_soft, const size_t mem_lim_hard) {
	ctx_list_t *newnode = new_node(name, mem_lim_soft, mem_lim_hard);

	if(newnode == NULL)
		return 0;

	if(ctx_list == NULL) {
		ctx_list = newnode;
		newnode->last = NULL;
	} else {
		ctx_list_head->next = newnode;
		newnode->last = ctx_list_head;
	}
	ctx_list_head = newnode;

	n_ctx++;
	return 1;
}

static void del_node(ctx_list_t *node) {
	if(node == ctx_list_head)
		ctx_list_head = node->last;

	if(node == ctx_list) {
		ctx_list = node->next;
		if(ctx_list)
			ctx_list->last = NULL;
	} else if(node->last) {
		node->last->next = node->next;
	}

	if(node->next) {
		node->next->last = node->last;
	}

	free(node->name);
	lisp_destroy_context(node->context);
	free(node);
}

static char *list_ctx_names(void) {
	ctx_list_t *currnode = ctx_list;
	size_t pos = 0, len;
	char *out;

	if(ctx_list == NULL)
		return NULL;

	if((out = malloc((MAX_NAME + 1) * MAX_CTX)) == NULL)
		return NULL;

	while(currnode) {
		len = strlen(currnode->name);
		if(len > MAX_NAME) len = MAX_NAME;
		strncpy(out + pos, currnode->name, len);
		pos += len;
		out[pos++] = ' ';
		currnode = currnode->next;
	}
	out[pos - 1] = '\0';

	return out;
}

static int del_ctx(const char *name) {
	ctx_list_t *currnode = ctx_list;
	ctx_list_t *nextnode;

	while(currnode) {
		nextnode = currnode->next;
		if(!strncmp(currnode->name, name, MAX_NAME)) {
			del_node(currnode);
			n_ctx--;
			return 1;
		}
		currnode = nextnode;
	}
	return 0;
}

static lisp_ctx_t *lookup_ctx(const char *name) {
	ctx_list_t *currnode = ctx_list;

	while(currnode) {
		if(!strncmp(currnode->name, name, MAX_NAME))
			return currnode->context;
		currnode = currnode->next;
	}
	return NULL;
}

size_t fmt_data_rec(const lisp_data_t *d, char *out, int print_parens, 
					lisp_ctx_t *context) {
	lisp_data_t *head, *tail;
	size_t s = 0;

	if(!d) {
		if(out) sprintf(out, "%s()", out);
		s += 2;
	}
	else if(d == context->the_global_environment) {
		if(out) sprintf(out, "%s<env>", out);
		s += 5;
	} else {
		switch(d->type) {
			case lisp_type_prim: 
				if(out) sprintf(out, "%s<proc>", out);
				s += 6; 
				break;
			case lisp_type_integer: 
				if(out)	sprintf(out, "%s%d", out, d->integer); 
				s += 10; 
				break;
			case lisp_type_decimal:
				if(out)	sprintf(out, "%s%g", out, d->decimal); 
				s += 16; 
				break;
			case lisp_type_symbol:
				if(out) sprintf(out, "%s%s", out, d->symbol); 
				s += strlen(d->symbol); 
				break;
			case lisp_type_string: 
				if(out) sprintf(out, "%s\"%s\"", out, d->string);
				s += 2 + strlen(d->string);
				break;
			case lisp_type_error:
				if(out) sprintf(out, "%sERROR: \"%s\"", out, d->error);
				s += 9 + strlen(d->error);
				break;
			case lisp_type_pair:
				if(is_compound_procedure(d)) {
					if(out) sprintf(out, "%s<proc>", out);
					s += 6;
					break;
				}

				if(print_parens) {
					if(out) sprintf(out, "%s(", out);
					s++;
				}

				head = lisp_car(d);
				tail = lisp_cdr(d);

				if(tail) {
					s += fmt_data_rec(head, out, 1, context);
					if(tail->type != lisp_type_pair) {
						if(out) sprintf(out, "%s . ", out);
						s += 3 + fmt_data_rec(tail, out, 1, context);
					} else {
						if(out) sprintf(out, "%s ", out);
						s += 1 + fmt_data_rec(tail, out, 0, context);
					}
				} else {
					s += fmt_data_rec(head, out, 1, context);
				}

				if(print_parens) {
					if(out) sprintf(out, "%s)", out);
					s++;
				}
		}
	}
	return s;
}

static char *fmt_data(const lisp_data_t *d, lisp_ctx_t *context) {
	char *out;
	size_t len;

	len = fmt_data_rec(d, NULL, 1, context);
	if((out = malloc(len + 1)) == NULL) {
		return NULL;
	}
	memset(out, 0, len + 1);
	fmt_data_rec(d, out, 1, context);

	return out;
}

static void runexp(char *exp, info_t *in, lisp_ctx_t *context) {
	lisp_data_t *exp_list, *ret;
	size_t readto, reclaimed;
	int error;
	char *retchar;

	do {
		exp_list = lisp_read(exp, &readto, &error, context);
		
		if(error) {
			irc_privmsg(to_sender(in), "Syntax Error: '%s'", exp);
		} else {
			ret = lisp_eval(exp_list, context);
			retchar = fmt_data(ret, context);
			if(retchar) {
				irc_privmsg(to_sender(in), "YHBT: %s", retchar);
				free(retchar);
			} else {
				irc_privmsg(to_sender(in), "ERROR: fmt_data() returned NULL.");
			}
		}

		exp += readto;
		
		if(reclaimed = lisp_gc(LISP_GC_LOWMEM, context))
			irc_privmsg(to_sender(in), "GC: %d bytes reclaimed.", reclaimed);
	} while(strlen(exp) && !error);
}

int reply(info_t *in) {
	char *exp, *ctx_name;
	lisp_ctx_t *curr_ctx = global_ctx;

	if(in->cmd == cmd_privmsg) {
		if(!tail_cmd(&in->tail, "eval-new-ctx")) {
			if(n_ctx == MAX_CTX) {
				irc_privmsg(to_sender(in), "ERROR: No more context slots.");
				return 0;
			}
			ctx_name = tail_getp(&in->tail);
			ctx_name[MAX_NAME] = '\0';
			if(!ctx_name) {
				irc_privmsg(to_sender(in), "Usage: %seval-new-ctx <context>", getprefix());
				return 0;
			}
			if(lookup_ctx(ctx_name) != NULL) {
				irc_privmsg(to_sender(in), "ERROR: Context '%s' already exists.", ctx_name);
				return 0;
			}
			if(create_ctx(ctx_name, global_ctx->mem_lim_soft, 
							global_ctx->mem_lim_hard) == 0) {
				irc_privmsg(to_sender(in), "ERROR: create_ctx() failed.");
				return 0;
			}
			irc_privmsg(to_sender(in), "New context :'%s'.", ctx_name);
			return 0;
		} else if(!tail_cmd(&in->tail, "eval-del-ctx")) {
			if(n_ctx == 0) {
				irc_privmsg(to_sender(in), "ERROR: Context not found.");
				return 0;
			}
			ctx_name = tail_getp(&in->tail);
			if(!ctx_name) {
				irc_privmsg(to_sender(in), "Usage: %seval-del-ctx <context>", getprefix());
				return 0;
			}
			if(del_ctx(ctx_name) == 0)
				irc_privmsg(to_sender(in), "ERROR: Context '%s' not found.", ctx_name);
			else
				irc_privmsg(to_sender(in), "Done.", ctx_name);
			
			return 0;
			
		} else if(!tail_cmd(&in->tail, "eval-run-ctx")) {
			ctx_name = tail_getp(&in->tail);
			if(!ctx_name) {
				irc_privmsg(to_sender(in), "Usage: %seval-run-ctx <context> <expression>", getprefix());
				return 0;
			}
			if((curr_ctx = lookup_ctx(ctx_name)) == NULL) {
				irc_privmsg(to_sender(in), "ERROR: Context '%s' not found.", ctx_name);
				return 0;
			}
			exp = in->tail;
			if(exp)
				runexp(exp, in, curr_ctx);
			else
				irc_privmsg(to_sender(in), "ERROR: Need an expression to evaluate.");
			return 0;
		} else if(!tail_cmd(&in->tail, "eval-list-ctx")) {
			ctx_name = list_ctx_names();
			if(ctx_name == NULL) {
				irc_privmsg(to_sender(in), "No contexts defined.");
			} else {
				irc_privmsg(to_sender(in), "Defined contexts: %s.", ctx_name);
				free(ctx_name);
			}
			return 0;	
		} else if(!tail_cmd(&in->tail, "eval")) {
			exp = in->tail;
			if(exp)
				runexp(exp, in, global_ctx);
			else
				irc_privmsg(to_sender(in), "ERROR: Need an expression to evaluate.");
		}
	}
	return 0;
}

int init(void) {
	char *p;
	int i;

	size_t mem_lim_soft, mem_lim_hard;

	p = config_get("lisp.so", "softlimit");
	if(p && (i = atoi(p)) > 0)
		mem_lim_soft = i;
	else
		mem_lim_soft = 786432;

	p = config_get("lisp.so", "hardlimit");
	if(p && (i = atoi(p)) > 0)
		mem_lim_hard = i;
	else
		mem_lim_hard = 1048576;

	global_ctx = lisp_make_context(mem_lim_soft, mem_lim_hard, LISP_GC_SILENT,
									THREAD_TIMEOUT);
	lisp_setup_env(global_ctx);

	return 0;
}

void destroy(void) {
	ctx_list_t *currnode = ctx_list, *nextnode;

	while(currnode) {
		nextnode = currnode->next;
		del_node(currnode);
		currnode = nextnode;
	}
	lisp_destroy_context(global_ctx);
}

PLUGIN_DEF(
	init,
	destroy,
	reply, 
	NULL,
	NULL, 
	PLUGIN_NODEP, 
	PLUGIN_HELP("Conjure the spirits of the computer with your spells."))
