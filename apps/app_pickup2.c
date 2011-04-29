/*
 * Asterisk -- A telephony toolkit for Linux.
 *
 * Pickup, channel independent call pickup
 *
 * Copyright (C) 2005-2007, Thorsten Knabe <ast@thorsten-knabe.de>
 * 
 * Copyright (C) 2004, Junghanns.NET GmbH
 *
 * Klaus-Peter Junghanns <kpj@junghanns.net>
 *
 * Copyright (C) 2004, Florian Overkamp <florian@obsimref.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 */

/*** MODULEINFO
	<defaultenabled>yes</defaultenabled>
 ***/

#include "asterisk.h"

ASTERISK_FILE_VERSION(__FILE__, "$Revision: 2 $")

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include "asterisk/lock.h"
#include "asterisk/file.h"
#include "asterisk/logger.h"
#include "asterisk/channel.h"
#include "asterisk/pbx.h"
#include "asterisk/module.h"
#include "asterisk/musiconhold.h"
#include "asterisk/features.h"
#include "asterisk/options.h"

static char *app = "PickUp2";
static char *synopsis = "PickUp ringing channel.";
static char *descrip = 
"  PickUp2(Technology/resource[&Technology2/resource2&...][|<option>]):\n"
"Matches the list of prefixes in the parameter list against channels in\n"
"state RINGING. If a match is found the channel is picked up and\n"
"PICKUP_CHANNEL is set to the picked up channel name. If no matching\n"
"channel is found PICKUP_CHANNEL is empty.\n"
"Possible options:\n"
"B: match on channel name of bridged channel.\n";

static char *app2 = "PickDown2";
static char *synopsis2 = "Hangup ringing channel.";
static char *descrip2 = 
"  PickDown2(Technology/resource[&Technology2/resource2&...][|<option>]):\n"
"Matches the list of prefixes in the parameter list against channels in\n"
"state RINGING. If a match is found the channel is hung up and\n"
"PICKDOWN_CHANNEL is set to the hung up channel name. If no matching\n"
"channel is found PICKDOWN_CHANNEL is empty.\n"
"Possible options:\n"
"B: match on channel name of bridged channel.\n";

static char *app3 = "Steal2";
static char *synopsis3 = "Steal a connected channel.";

static char *descrip3 = 
"  Steal2(Technology/resource[&Technology2/resource2&...][|<option>]):\n"
"Matches the list of prefixes in the parameter list against channels in\n"
"state UP. If a match is found the channel is stolen and\n"
"STEAL_CHANNEL is set to the stolen channel name. If no matching\n"
"channel is found STEAL_CHANNEL is empty.\n"
"Possible options:\n"
"B: match on channel name of bridged channel.\n";

/* Find channel matching given pattern and state, skipping our own channel.
 * Returns locked channel, which has to be unlocked using ast_mutex_unlock().
 * Returns NULL when no matching channel is found.
 */
static struct ast_channel *find_matching_channel(struct ast_channel *chan,
	void *pattern, int chanstate)
{
	struct ast_channel *cur;
	char *pat = "";
	char *next_pat = NULL;
	char *option = "";
	struct ast_channel *bridged;

	/* copy original pattern or use empty pattern if no pattern has been given*/
	if (pattern) {
		pat = alloca(strlen(pattern) + 1);
		strcpy(pat, pattern);
	}
	for (option = pat; *option; option++) {
		if (*option == '|') {
			*option = '\0';
			option++;
			break;
		}
	}
	ast_verbose(VERBOSE_PREFIX_4 
		"find_matching_channel: pattern='%s' option='%s' state=%d\n",
		(char *)pat, option, chanstate);

	/* match on bridged channel name */
	if (!strcmp("B", option)) {
		/* Iterate over each part of the pattern */
		while (pat) {
			/* find pattern for next iteration, 
			 * terminate current pattern */
			for (next_pat = pat; 
				*next_pat && *next_pat != '&'; next_pat++);
			if (*next_pat == '&') {
				*next_pat = 0;
				next_pat++;
			} else
				next_pat = NULL;
			/* Iterate over all channels */
			cur = ast_channel_walk_locked(NULL);
			while (cur) {
				bridged = ast_bridged_channel(cur);
				if (bridged) {
					ast_verbose(VERBOSE_PREFIX_4 
						"find_matching_channel: trying channel='%s' bridged='%s' "
						"state=%d pattern='%s'\n",
						cur->name, bridged->name, cur->_state, pat);
					if ((cur != chan) && (bridged != chan) &&
						(cur->_state == chanstate) &&
						!strncmp(pat, bridged->name, strlen(pat))) {
						ast_verbose(VERBOSE_PREFIX_4
								"find_matching_channel: "
								"found channel='%s' bridged='%s'\n",
								cur->name, bridged->name);
						return(cur);
					}
				}
				ast_mutex_unlock(&cur->lock);
				cur = ast_channel_walk_locked(cur);
			}
			pat = next_pat;
		}
	} else {
		/* Iterate over each part of the pattern */
		while (pat) {
			/* find pattern for next iteration, 
			 * terminate current pattern */
			for (next_pat = pat; 
				*next_pat && *next_pat != '&'; next_pat++);
			if (*next_pat == '&') {
				*next_pat = 0;
				next_pat++;
			} else
				next_pat = NULL;
			/* Iterate over all channels */
			cur = ast_channel_walk_locked(NULL);
			while (cur) {
				ast_verbose(VERBOSE_PREFIX_4 
					"find_matching_channel: trying channel='%s' "
					"state=%d pattern='%s'\n",
					cur->name, cur->_state, pat);
				if ((cur != chan) && 
					(cur->_state == chanstate) &&
					!strncmp(pat, cur->name, strlen(pat))) {
					ast_verbose(VERBOSE_PREFIX_4
							"find_matching_channel: "
							"found channel='%s'\n",
							cur->name);
					return(cur);
				}
				ast_mutex_unlock(&cur->lock);
				cur = ast_channel_walk_locked(cur);
			}
			pat = next_pat;
		}
	}
	return(NULL);
}

static int pickup_channel(struct ast_channel *chan, void *pattern)
{
	int ret = 0;
	struct ast_module_user *u;
	struct ast_channel *cur;
	u = ast_module_user_add(chan);
	cur = find_matching_channel(chan, pattern, AST_STATE_RINGING);
	if (cur) {
		ast_verbose(VERBOSE_PREFIX_3 
			"Channel %s picked up ringing channel %s\n",
			chan->name, cur->name);
		pbx_builtin_setvar_helper(chan, "PICKUP_CHANNEL", cur->name);
		if (chan->_state != AST_STATE_UP) {
			ast_answer(chan);
		}
		if (ast_channel_masquerade(cur, chan)) {
			ast_log(LOG_ERROR, "unable to masquerade\n");
			ret = -1;
		}
		ast_mutex_unlock(&cur->lock);
		ast_mutex_unlock(&chan->lock);
	} else {
		pbx_builtin_setvar_helper(chan, "PICKUP_CHANNEL", "");
	}
	ast_module_user_remove(u);
	return(ret);
}

static int pickdown_channel(struct ast_channel *chan, void *pattern)
{
	int ret = 0;
	struct ast_module_user *u;
	struct ast_channel *cur;
	u = ast_module_user_add(chan);
	cur = find_matching_channel(chan, pattern, AST_STATE_RINGING);
	if (cur) {
                ast_verbose(VERBOSE_PREFIX_3 
			"Channel %s hung up ringing channel %s\n",
			chan->name, cur->name);
		pbx_builtin_setvar_helper(chan, "PICKDOWN_CHANNEL", cur->name);
		ast_softhangup_nolock(cur, AST_SOFTHANGUP_DEV);
		ast_mutex_unlock(&cur->lock);
	} else {
		pbx_builtin_setvar_helper(chan, "PICKDOWN_CHANNEL", "");
	}
	ast_module_user_remove(u);
	return(ret);
}

static int steal_channel(struct ast_channel *chan, void *pattern)
{
	int ret = 0;
	struct ast_module_user *u;
	struct ast_channel *cur;
	u = ast_module_user_add(chan);
	cur = find_matching_channel(chan, pattern, AST_STATE_UP);
	if (cur) {
		ast_verbose(VERBOSE_PREFIX_3 
			"Channel %s stole channel %s\n",
			chan->name, cur->name);
		pbx_builtin_setvar_helper(chan, "STEAL_CHANNEL", cur->name);
		if (chan->_state != AST_STATE_UP) {
			ast_answer(chan);
		}
		if (cur->_bridge) {
			if (!ast_mutex_lock(&cur->_bridge->lock)) {
				ast_moh_stop(cur->_bridge);
				ast_mutex_unlock(&cur->_bridge->lock);
			}
		}
			
		if (ast_channel_masquerade(cur, chan)) {
			ast_log(LOG_ERROR, "unable to masquerade\n");
			ret = -1;
		}
		ast_mutex_unlock(&cur->lock);
		ast_mutex_unlock(&chan->lock);
	} else {
		pbx_builtin_setvar_helper(chan, "STEAL_CHANNEL", "");
	}
	ast_module_user_remove(u);
	return(ret);
}

static int unload_module(void)
{
  	ast_module_user_hangup_all();
	ast_unregister_application(app3);
	ast_unregister_application(app2);
	return ast_unregister_application(app);
}

static int load_module(void)
{
	ast_register_application(app3, steal_channel, synopsis3, descrip3);
	ast_register_application(app2, pickdown_channel, synopsis2, descrip2);
	return ast_register_application(app, pickup_channel, synopsis, descrip);
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, 
    "PickUp2, PickDown2, Steal2 Application");
