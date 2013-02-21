#include <stdlib.h>
#include <string.h>

#include "libisp.h"

#include "defs.h"
#include "irc.h"
#include "util.h"
#include "conf.h"

extern data_t *the_global_env;

size_t fmt_data_rec(const data_t *d, char *out, int print_parens) {
	data_t *head, *tail;
	size_t s = 0;

	if(!d) {
		if(out) sprintf(out, "%s()", out);
		s += 2;
	}
	else if(d == the_global_env) {
		if(out) sprintf(out, "%s<env>", out);
		s += 5;
	} else {
		switch(d->type) {
			case prim_procedure: 
				if(out) sprintf(out, "%s<proc>", out);
				s += 6; 
				break;
			case integer: 
				if(out)	sprintf(out, "%s%d", out, d->val.integer); 
				s += 10; 
				break;
			case decimal:
				if(out)	sprintf(out, "%s%g", out, d->val.decimal); 
				s += 16; 
				break;
			case symbol:
				if(out) sprintf(out, "%s%s", out, d->val.symbol); 
				s += strlen(d->val.symbol); 
				break;
			case string: 
				if(out) sprintf(out, "%s\"%s\"", out, d->val.string);
				s += 2 + strlen(d->val.string);
				break;
			case pair:
				if(is_compound_procedure(d)) {
					if(out) sprintf(out, "%s<proc>", out);
					s += 6;
					break;
				}

				if(print_parens) {
					if(out) sprintf(out, "%s(", out);
					s++;
				}

				head = car(d);
				tail = cdr(d);

				if(tail) {
					s += fmt_data_rec(head, out, 1);
					if(tail->type != pair) {
						if(out) sprintf(out, "%s . ", out);
						s += 3 + fmt_data_rec(tail, out, 1);
					} else {
						if(out) sprintf(out, "%s ", out);
						s += 1 + fmt_data_rec(tail, out, 0);
					}
				} else {
					s += fmt_data_rec(head, out, 1);
				}

				if(print_parens) {
					if(out) sprintf(out, "%s)", out);
					s++;
				}
		}
	}
	return s;
}

static char *fmt_data(const data_t *d) {
	char *out;
	size_t len;

	len = fmt_data_rec(d, NULL, 1);
	if((out = malloc(len + 1)) == NULL) {
		return NULL;
	}
	memset(out, 0, len + 1);
	fmt_data_rec(d, out, 1);

	return out;
}

static void runexp(char *exp, info_t *in) {
	data_t *exp_list, *ret;
	size_t readto, reclaimed;
	int error;
	char *retchar;

	do {
		exp_list = read_exp(exp, &readto, &error);
		
		if(error) {
			irc_privmsg(to_sender(in), "Syntax Error: '%s'", exp);
		} else {
			ret = eval_thread(exp_list, the_global_env);
			retchar = fmt_data(ret);
			if(retchar) {
				irc_privmsg(to_sender(in), "YHBT: %s", retchar);
				free(retchar);
			} else {
				irc_privmsg(to_sender(in), "ERROR: fmt_data() returned NULL.");
			}
		}

		exp += readto;
		
		if(reclaimed = run_gc(GC_LOWMEM))
			irc_privmsg(to_sender(in), "GC: %d bytes reclaimed.", reclaimed);
	} while(strlen(exp) && !error);
}

int reply(info_t *in) {
	char *exp;

	if(in->cmd == cmd_privmsg && !tail_cmd(&in->tail, "eval")) {
		exp = in->tail;
		if(exp)
			runexp(exp, in);
		else
			irc_privmsg(to_sender(in), "ERROR: Need an expression to evaluate.");
	}
	return 0;
}

int init(void) {
	char *p;
	int i;

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

	mem_verbosity = MEM_SILENT;
	setup_environment();
}

void destroy(void) {
	cleanup_lisp();
}

PLUGIN_DEF(
	init,
	destroy,
	reply, 
	NULL,
	NULL, 
	PLUGIN_NODEP, 
	PLUGIN_HELP("Conjure the spirits of the computer with your spells."))
