/*
 * Created by Geoff Crewe.
 *
 * Copyright 2011 MIT Haystack Observatory 
 *  
 * This file is part of mark6.
 *
 * mark6 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mark6 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mark6.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

/*
 * $Id: replay_parser.c 592 2010-08-09 15:54:42Z gbc $
 *
 * REPLAY specific commanding and information formatting.
 */

#include <stdio.h>
#include <string.h>

#include "bmr_util.h"
#include "bmr_server.h"
#include "load_task.h"
#include "push_task.h"

#include "replay_server.h"

#define REPLAY_EXEC_HELP    "\
The command must be something like:\n\
  load:<cmd>    which configures the LOAD task\n\
  push:<cmd>    which configures the PUSH task\n\
  server:<cmd>  which configures the server\n\
Use \"help\" as the <cmd> for details on each subsystem.\n\
"

/*
 * This dumps the current working state configuration to a file
 * The intent here is that the file of commands may be reloaded.
 */
static int replay_dump_to_file(char *file)
{
    FILE    *fp;
    if (!(file = skip_leading_space(file))) return(0);
    nuke_trailing_space(file);
    if (!(fp = fopen(file, "w"))) return(perror("replay_dump_to_file"),0);
    load_dump(fp, get_load_state());
    push_dump(fp, get_push_state());
    bmr_server_dump(fp, get_bmr_server_state());
    fputs("##\n## eof\n##\n", fp);
    fclose(fp);
    return(0);
}

/*
 * Executes the cached commands.
 *
 * This is really just a matter of a top-level branch into the
 * more specific parsers to handle the different pieces.
 *
 * The while loop is broken if any of the config commands has
 * an error.
 */
int replay_exec_cache(BMRServer *bs, LoadState *ls, PushState *ps)
{
    static char	buf[BMR_MAX_MESSAGE];
    char    *cmd, *cached_cmds = bmr_cached_cmds_ptr();
    int	    cnt = 0, rv = 0;

    if (!cached_cmds) return(0);
    if (bs->verb>1) bmr_show_cached_cmds();

    for (cmd = strtok(cached_cmds, "\n"); cmd; cmd = strtok(0, "\n")) {
	if (bs->verb>0) fprintf(stderr, "CMD%d: '%s'\n", cnt++, cmd);
	if (!(cmd = skip_leading_space(cmd))) continue;
	if        (!strncmp(cmd, "server", 6)) {
	    if (bmr_server_config(cmd+6, bs, buf)) break;
	} else if (!strncmp(cmd, "load", 4)) {
	    if (load_config(cmd+4, ls, buf)) break;
	} else if (!strncmp(cmd, "push", 4)) {
	    if (push_config(cmd+4, ps, buf)) break;
	} else {
	    rv = snprintf(buf, BMR_MAX_MESSAGE,
		"Ignoring '%s'\n" REPLAY_EXEC_HELP, cmd);
	}
	fputs(buf, stderr);
    }
    if (cmd) return(fprintf(stderr, "%s Execution halted.\n", buf));

    bmr_cache_free();
    return(rv);
}

/*
 * This just resets the GRAB and PUSH components,
 * and tacitly assumes that the server remains sane.
 */
static int replay_exec_reset(char *resp)
{
    BMRServer	*bsp = get_bmr_server_state();
    LoadState	*lsp = get_load_state();
    PushState	*psp = get_push_state();
    int		errs = replay_exec_cache(bsp, lsp, psp);
    if (errs) return(snprintf(resp, BMR_MAX_MESSAGE, "Exec (%d)\n", errs));
    errs += set_load_state(lsp);
    if (errs) return(snprintf(resp, BMR_MAX_MESSAGE, "Load (%d)\n", errs));
    errs += set_push_state(psp);
    if (errs) return(snprintf(resp, BMR_MAX_MESSAGE, "Push (%d)\n", errs));
    return(snprintf(resp, BMR_MAX_MESSAGE, "LOAD + PUSH updated\n"));
}

/*
 * Provide help on request/response
 */
static int replay_help(char *resp)
{
    int nb = snprintf(resp, BMR_MAX_MESSAGE,
	"bmr_replay commands:\n"
	"  quit         exits the replay server\n"
	"  help         provides this help\n"
	"  help:server  provides help on server configuration\n"
	"  help:load    provides help on load configuration\n"
	"  help:push    provides help on push configuration\n"
	"\n"
	"  cfg:<cmd>    caches a (re)configuration command\n"
	"               <cmd> is (server|load|push):<cmd>\n"
	"  exec:<file>  load and cache these commands from some file\n"
	"  dump:<file>  saves working configuration to file\n"
	"  reset        applies the caches commands\n"
	"\n"
	"  show         shows internal state information\n"
	"  info         shows state and additional data\n"
	"  counters     shows various packet/byte counters\n"
	"  rates        shows load / push rates\n"
	"\n"
    );
    return(nb);
}

/*
 * A handler that responds to client requests.
 *
 * The length of the response is returned and the response
 * overwrites the request (at most BMR_MAX_MESSAGE bytes).
 *
 * A zero return implies no response.
 * A negative return should provoke a terminal error.
 * A positive return is the number of characters in the response.
 */
static int replay_response(char *req, char *resp)
{
    int	    nb;

    *resp = 0;
    if (!(req = skip_leading_space(req))) return(0);

    if        (!strncmp(req, "quit", 4)) {
	return(-1);
    } else if (!strncmp(req, "help:server", 11)) {
	nb = bmr_server_help(resp);
    } else if (!strncmp(req, "help:load", 9)) {
	nb = load_help(resp);
    } else if (!strncmp(req, "help:push", 9)) {
	nb = push_help(resp);
    } else if (!strncmp(req, "help", 4)) {
	nb = replay_help(resp);

    } else if (!strncmp(req, "cfg:", 4)) {
	nb = bmr_parse_cache_cmd(req+4) ? -2 : 0;
    } else if (!strncmp(req, "exec:", 5)) {
	nb = bmr_parse_cache_file(req+5) ? -3 : 0;
    } else if (!strncmp(req, "dump:", 5)) {
	nb = replay_dump_to_file(req+5);
    } else if (!strncmp(req, "reset", 4)) {
	nb = replay_exec_reset(resp);

    } else if (!strncmp(req, "show", 4)) {
	nb = replay_resp_show(resp);
    } else if (!strncmp(req, "info", 4)) {
	nb = replay_resp_info(resp);
    } else if (!strncmp(req, "counters", 8)) {
	nb = replay_resp_counters(resp);
    } else if (!strncmp(req, "rates", 5)) {
	nb = replay_resp_rates(resp);

    } else {
	nb = snprintf(resp, BMR_MAX_MESSAGE, "What is: %s", req);
    }

    return(nb);
}

/*
 * Different servers have different parsers, but one per customer.
 */
int (* const bmr_response)(char *request, char *response) = &replay_response;

/*
 * eof
 */