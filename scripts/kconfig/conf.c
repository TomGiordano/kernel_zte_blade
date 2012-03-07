/*
 * Copyright (C) 2002 Roman Zippel <zippel@linux-m68k.org>
 * Released under the terms of the GNU GPL v2.0.
 */

#include <locale.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

#define LKC_DIRECT_LINK
#include "lkc.h"

static void conf(struct menu *menu);
static void check_conf(struct menu *menu);

enum {
	ask_all,
	ask_new,
	ask_silent,
	set_default,
	set_yes,
	set_mod,
	set_no,
	set_random
} input_mode = ask_all;
char *defconfig_file;

static int indent = 1;
static int valid_stdin = 1;
static int sync_kconfig;
static int conf_cnt;
static char line[128];
static struct menu *rootEntry;

static void print_help(struct menu *menu)
{
	struct gstr help = str_new();

	menu_get_ext_help(menu, &help);

	printf("\n%s\n", str_get(&help));
	str_free(&help);
}

static void strip(char *str)
{
	char *p = str;
	int l;

	while ((isspace(*p)))
		p++;
	l = strlen(p);
	if (p != str)
		memmove(str, p, l + 1);
	if (!l)
		return;
	p = str + l - 1;
	while ((isspace(*p)))
		*p-- = 0;
}

static void check_stdin(void)
{
	if (!valid_stdin) {
		printf(_("aborted!\n\n"));
		printf(_("Console input/output is redirected. "));
		printf(_("Run 'make oldconfig' to update configuration.\n\n"));
		exit(1);
	}
}

static int conf_askvalue(struct symbol *sym, const char *def)
{
	enum symbol_type type = sym_get_type(sym);

	if (!sym_has_value(sym))
		printf(_("(NEW) "));

	line[0] = '\n';
	line[1] = 0;

	if (!sym_is_changable(sym)) {
		printf("%s\n", def);
		line[0] = '\n';
		line[1] = 0;
		return 0;
	}

	switch (input_mode) {
	case ask_new:
	case ask_silent:
		if (sym_has_value(sym)) {
			printf("%s\n", def);
			return 0;
		}
		check_stdin();
	case ask_all:
		fflush(stdout);
		xfgets(line, 128, stdin);
		return 1;
	default:
		break;
	}

	switch (type) {
	case S_INT:
	case S_HEX:
	case S_STRING:
		printf("%s\n", def);
		return 1;
	default:
		;
	}
	printf("%s", line);
	return 1;
}

static int conf_string(struct menu *menu)
{
	struct symbol *sym = menu->sym;
	const char *def;

	while (1) {
		printf("%*s%s ", indent - 1, "", _(menu->prompt->text));
		printf("(%s) ", sym->name);
		def = sym_get_string_value(sym);
		if (sym_get_string_value(sym))
			printf("[%s] ", def);
		if (!conf_askvalue(sym, def))
			return 0;
		switch (line[0]) {
		case '\n':
			break;
		case '?':
			/* print help */
			if (line[1] == '\n') {
				print_help(menu);
				def = NULL;
				break;
			}
		default:
			line[strlen(line)-1] = 0;
			def = line;
		}
		if (def && sym_set_string_value(sym, def))
			return 0;
	}
}

static int conf_sym(struct menu *menu)
{
	struct symbol *sym = menu->sym;
	int type;
	tristate oldval, newval;

	while (1) {
		printf("%*s%s ", indent - 1, "", _(menu->prompt->text));
		if (sym->name)
			printf("(%s) ", sym->name);
		type = sym_get_type(sym);
		putchar('[');
		oldval = sym_get_tristate_value(sym);
		switch (oldval) {
		case no:
			putchar('N');
			break;
		case mod:
			putchar('M');
			break;
		case yes:
			putchar('Y');
			break;
		}
		if (oldval != no && sym_tristate_within_range(sym, no))
			printf("/n");
		if (oldval != mod && sym_tristate_within_range(sym, mod))
			printf("/m");
		if (oldval != yes && sym_tristate_within_range(sym, yes))
			printf("/y");
		if (menu_has_help(menu))
			printf("/?");
		printf("] ");
		if (!conf_askvalue(sym, sym_get_string_value(sym)))
			return 0;
		strip(line);

		switch (line[0]) {
		case 'n':
		case 'N':
			newval = no;
			if (!line[1] || !strcmp(&line[1], "o"))
				break;
			continue;
		case 'm':
		case 'M':
			newval = mod;
			if (!line[1])
				break;
			continue;
		case 'y':
		case 'Y':
			newval = yes;
			if (!line[1] || !strcmp(&line[1], "es"))
				break;
			continue;
		case 0:
			newval = oldval;
			break;
		case '?':
			goto help;
		default:
			continue;
		}
		if (sym_set_tristate_value(sym, newval))
			return 0;
help:
		print_help(menu);
	}
}

static int conf_choice(struct menu *menu)
{
	struct symbol *sym, *def_sym;
	struct menu *child;
	int type;
	bool is_new;

	sym = menu->sym;
	type = sym_get_type(sym);
	is_new = !sym_has_value(sym);
	if (sym_is_changable(sym)) {
		conf_sym(menu);
		sym_calc_value(sym);
		switch (sym_get_tristate_value(sym)) {
		case no:
			return 1;
		case mod:
			return 0;
		case yes:
			break;
		}
	} else {
		switch (sym_get_tristate_value(sym)) {
		case no:
			return 1;
		case mod:
			printf("%*s%s\n", indent - 1, "", _(menu_get_prompt(menu)));
			return 0;
		case yes:
			break;
		}
	}

	while (1) {
		int cnt, def;

		printf("%*s%s\n", indent - 1, "", _(menu_get_prompt(menu)));
		def_sym = sym_get_choice_value(sym);
		cnt = def = 0;
		line[0] = 0;
		for (child = menu->list; child; child = child->next) {
			if (!menu_is_visible(child))
				continue;
			if (!child->sym) {
				printf("%*c %s\n", indent, '*', _(menu_get_prompt(child)));
				continue;
			}
			cnt++;
			if (child->sym == def_sym) {
				def = cnt;
				printf("%*c", indent, '>');
			} else
				printf("%*c", indent, ' ');
			printf(" %d. %s", cnt, _(menu_get_prompt(child)));
			if (child->sym->name)
				printf(" (%s)", child->sym->name);
			if (!sym_has_value(child->sym))
				printf(_(" (NEW)"));
			printf("\n");
		}
		printf(_("%*schoice"), indent - 1, "");
		if (cnt == 1) {
			printf("[1]: 1\n");
			goto conf_childs;
		}
		printf("[1-%d", cnt);
		if (menu_has_help(menu))
			printf("?");
		printf("]: ");
		switch (input_mode) {
		case ask_new:
		case ask_silent:
			if (!is_new) {
				cnt = def;
				printf("%d\n", cnt);
				break;
			}
			check_stdin();
		case ask_all:
			fflush(stdout);
			xfgets(line, 128, stdin);
			strip(line);
			if (line[0] == '?') {
				print_help(menu);
				continue;
			}
			if (!line[0])
				cnt = def;
			else if (isdigit(line[0]))
				cnt = atoi(line);
			else
				continue;
			break;
		default:
			break;
		}

	conf_childs:
		for (child = menu->list; child; child = child->next) {
			if (!child->sym || !menu_is_visible(child))
				continue;
			if (!--cnt)
				break;
		}
		if (!child)
			continue;
		if (line[strlen(line) - 1] == '?') {
			print_help(child);
			continue;
		}
		sym_set_choice_value(sym, child->sym);
		for (child = child->list; child; child = child->next) {
			indent += 2;
			conf(child);
			indent -= 2;
		}
		return 1;
	}
}

static void conf(struct menu *menu)
{
	struct symbol *sym;
	struct property *prop;
	struct menu *child;

	if (!menu_is_visible(menu))
		return;

	sym = menu->sym;
	prop = menu->prompt;
	if (prop) {
		const char *prompt;

		switch (prop->type) {
		case P_MENU:
			if (input_mode == ask_silent && rootEntry != menu) {
				check_conf(menu);
				return;
			}
		case P_COMMENT:
			prompt = menu_get_prompt(menu);
			if (prompt)
				printf("%*c\n%*c %s\n%*c\n",
					indent, '*',
					indent, '*', _(prompt),
					indent, '*');
		default:
			;
		}
	}

	if (!sym)
		goto conf_childs;

	if (sym_is_choice(sym)) {
		conf_choice(menu);
		if (sym->curr.tri != mod)
			return;
		goto conf_childs;
	}

	switch (sym->type) {
	case S_INT:
	case S_HEX:
	case S_STRING:
		conf_string(menu);
		break;
	default:
		conf_sym(menu);
		break;
	}

conf_childs:
	if (sym)
		indent += 2;
	for (child = menu->list; child; child = child->next)
		conf(child);
	if (sym)
		indent -= 2;
}

static void check_conf(struct menu *menu)
{
	struct symbol *sym;
	struct menu *child;

	if (!menu_is_visible(menu))
		return;

	sym = menu->sym;
	if (sym && !sym_has_value(sym)) {
		if (sym_is_changable(sym) ||
		    (sym_is_choice(sym) && sym_get_tristate_value(sym) == yes)) {
			if (!conf_cnt++)
				printf(_("*\n* Restart config...\n*\n"));
			rootEntry = menu_get_parent_menu(menu);
			conf(rootEntry);
		}
	}

	for (child = menu->list; child; child = child->next)
		check_conf(child);
}

int main(int ac, char **av)
{
	int opt;
	const char *name;
	struct stat tmpstat;

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	while ((opt = getopt(ac, av, "osdD:nmyrh")) != -1) {
		switch (opt) {
		case 'o':
			input_mode = ask_silent;
			break;
		case 's':
			input_mode = ask_silent;
			sync_kconfig = 1;
			break;
		case 'd':
			input_mode = set_default;
			break;
		case 'D':
			input_mode = set_default;
			defconfig_file = optarg;
			break;
		case 'n':
			input_mode = set_no;
			break;
		case 'm':
			input_mode = set_mod;
			break;
		case 'y':
			input_mode = set_yes;
			break;
		case 'r':
		{
			struct timeval now;
			unsigned int seed;

			/*
			 * Use microseconds derived seed,
			 * compensate for systems where it may be zero
			 */
			gettimeofday(&now, NULL);

			seed = (unsigned int)((now.tv_sec + 1) * (now.tv_usec + 1));
			srand(seed);

			input_mode = set_random;
			break;
		}
		case 'h':
			printf(_("See README for usage info\n"));
			exit(0);
			break;
		default:
			fprintf(stderr, _("See README for usage info\n"));
			exit(1);
		}
	}
	if (ac == optind) {
		printf(_("%s: Kconfig file missing\n"), av[0]);
		exit(1);
	}
	name = av[optind];
	conf_parse(name);
	//zconfdump(stdout);
	if (sync_kconfig) {
		name = conf_get_configname();
		if (stat(name, &tmpstat)) {
			fprintf(stderr, _("***\n"
				"*** You have not yet configured your kernel!\n"
				"*** (missing kernel config file \"%s\")\n"
				"***\n"
				"*** Please run some configurator (e.g. \"make oldconfig\" or\n"
				"*** \"make menuconfig\" or \"make xconfig\").\n"
				"***\n"), name);
			exit(1);
		}
	}

	switch (input_mode) {
	case set_default:
		if (!defconfig_file)
			defconfig_file = conf_get_default_confname();
		if (conf_read(defconfig_file)) {
			printf(_("***\n"
				"*** Can't find default configuration \"%s\"!\n"
				"***\n"), defconfig_file);
			exit(1);
		}
		break;
	case ask_silent:
	case ask_all:
	case ask_new:
		conf_read(NULL);
		break;
	case set_no:
	case set_mod:
	case set_yes:
	case set_random:
		name = getenv("KCONFIG_ALLCONFIG");
		if (name && !stat(name, &tmpstat)) {
			conf_read_simple(name, S_DEF_USER);
			break;
		}
		switch (input_mode) {
		case set_no:	 name = "allno.config"; break;
		case set_mod:	 name = "allmod.config"; break;
		case set_yes:	 name = "allyes.config"; break;
		case set_random: name = "allrandom.config"; break;
		default: break;
		}
		if (!stat(name, &tmpstat))
			conf_read_simple(name, S_DEF_USER);
		else if (!stat("all.config", &tmpstat))
			conf_read_simple("all.config", S_DEF_USER);
		break;
	default:
		break;
	}

	if (sync_kconfig) {
		if (conf_get_changed()) {
			name = getenv("KCONFIG_NOSILENTUPDATE");
			if (name && *name) {
				fprintf(stderr,
					_("\n*** Kernel configuration requires explicit update.\n\n"));
				return 1;
			}
		}
		valid_stdin = isatty(0) && isatty(1) && isatty(2);
	}

	switch (input_mode) {
	case set_no:
		conf_set_all_new_symbols(def_no);
		break;
	case set_yes:
		conf_set_all_new_symbols(def_yes);
		break;
	case set_mod:
		conf_set_all_new_symbols(def_mod);
		break;
	case set_random:
		conf_set_all_new_symbols(def_random);
		break;
	case set_default:
		conf_set_all_new_symbols(def_default);
		break;
	case ask_new:
	case ask_all:
		rootEntry = &rootmenu;
		conf(&rootmenu);
		input_mode = ask_silent;
		/* fall through */
	case ask_silent:
		/* Update until a loop caused no more changes */
		do {
			conf_cnt = 0;
			check_conf(&rootmenu);
		} while (conf_cnt);
		break;
	}

	if (sync_kconfig) {
		/* silentoldconfig is used during the build so we shall update autoconf.
		 * All other commands are only used to generate a config.
		 */
		if (conf_get_changed() && conf_write(NULL)) {
			fprintf(stderr, _("\n*** Error during writing of the kernel configuration.\n\n"));
			exit(1);
		}
		if (conf_write_autoconf()) {
			fprintf(stderr, _("\n*** Error during update of the kernel configuration.\n\n"));
			return 1;
		}
	} else {
		if (conf_write(NULL)) {
			fprintf(stderr, _("\n*** Error during writing of the kernel configuration.\n\n"));
			exit(1);
		}
	}
	return 0;
}
/*
 * Helper function to facilitate fgets() by Jean Sacren.
 */
void xfgets(str, size, in)
  char *str;
  int size;
  FILE *in;
{
  if (fgets(str, size, in) == NULL)
    fprintf(stderr, "\nError in reading or end of file.\n");
}
