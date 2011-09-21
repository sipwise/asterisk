/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 2007, Digium, Inc.
 *
 * Russell Bryant <russell@digium.com> 
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*! \file
 *
 * \brief Manually controlled blinky lights
 *
 * \author Russell Bryant <russell@digium.com> 
 *
 * \ingroup functions
 *
 * \note Props go out to Ahrimanes in #asterisk for requesting this at 4:30 AM
 *       when I couldn't sleep.  :)
 */

#include "asterisk.h"

ASTERISK_FILE_VERSION(__FILE__, "$Revision: 4 $")

#include <stdlib.h>

#include "asterisk/module.h"
#include "asterisk/channel.h"
#include "asterisk/pbx.h"
#include "asterisk/utils.h"
#include "asterisk/linkedlists.h"
#include "asterisk/devicestate.h"
#include "asterisk/cli.h"
#include "asterisk/astdb.h"

static const char astdb_family[] = "CustomDevstate";

static const char *ast_devstate_str(int state)
{
	const char *res = "UNKNOWN";

	switch (state) {
	case AST_DEVICE_UNKNOWN:
		break;
	case AST_DEVICE_NOT_INUSE:
		res = "NOT_INUSE";
		break;
	case AST_DEVICE_INUSE:
		res = "INUSE";
		break;
	case AST_DEVICE_BUSY:
		res = "BUSY";
		break;
	case AST_DEVICE_INVALID:
		res = "INVALID";
		break;
	case AST_DEVICE_UNAVAILABLE:
		res = "UNAVAILABLE";
		break;
	case AST_DEVICE_RINGING:
		res = "RINGING";
		break;
	case AST_DEVICE_RINGINUSE:
		res = "RINGINUSE";
		break;
	case AST_DEVICE_ONHOLD:
		res = "ONHOLD";
		break;
	}

	return res;
}

static int ast_devstate_val(const char *val)
{
	if (!strcasecmp(val, "NOT_INUSE"))
		return AST_DEVICE_NOT_INUSE;
	else if (!strcasecmp(val, "INUSE"))
		return AST_DEVICE_INUSE;
	else if (!strcasecmp(val, "BUSY"))
		return AST_DEVICE_BUSY;
	else if (!strcasecmp(val, "INVALID"))
		return AST_DEVICE_INVALID;
	else if (!strcasecmp(val, "UNAVAILABLE"))
		return AST_DEVICE_UNAVAILABLE;
	else if (!strcasecmp(val, "RINGING"))
		return AST_DEVICE_RINGING;
	else if (!strcasecmp(val, "RINGINUSE"))
		return AST_DEVICE_RINGINUSE;
	else if (!strcasecmp(val, "ONHOLD"))
		return AST_DEVICE_ONHOLD;

	return AST_DEVICE_UNKNOWN;
}

static int devstate_read(struct ast_channel *chan, char *cmd, char *data, 
	char *buf, size_t len)
{
	ast_copy_string(buf, ast_devstate_str(ast_device_state(data)), len);

	return 0;
}

static int devstate_write(struct ast_channel *chan, char *function,
	char *data, const char *value)
{
	size_t len = strlen("Custom:");

	if (strncasecmp(data, "Custom:", len)) {
		ast_log(LOG_WARNING, "The DEVSTATE function can only be used to set 'Custom:' device state!\n");
		return -1;
	}
	data += len;
	if (ast_strlen_zero(data)) {
		ast_log(LOG_WARNING, "DEVSTATE function called with no custom device name!\n");
		return -1;
	}

	ast_db_put(astdb_family, data, (char *) value);

	ast_device_state_changed("Custom:%s", data);

	return 0;
}

static int custom_devstate_callback(const char *data)
{
	char buf[256] = "";

	ast_db_get(astdb_family, data, buf, sizeof(buf));

	return ast_devstate_val(buf);
}

static int cli_funcdevstate_list(int fd, int argc, char *argv[])
{
	struct ast_db_entry *db_entry, *db_tree;

	if (argc != 2)
		return RESULT_SHOWUSAGE;

	ast_cli(fd, "\n"
	        "---------------------------------------------------------------------\n"
	        "--- Custom Device States --------------------------------------------\n"
	        "---------------------------------------------------------------------\n"
	        "---\n");

	db_entry = db_tree = ast_db_gettree(astdb_family, NULL);
	for (; db_entry; db_entry = db_entry->next) {
		const char *dev_name = strrchr(db_entry->key, '/') + 1;
		if (dev_name <= (const char *) 1)
			continue;
		ast_cli(fd, "--- name: 'custom:%s'  state: '%s'\n"
		               "---\n", dev_name, db_entry->data);
	}
	ast_db_freetree(db_tree);
	db_tree = NULL;

	ast_cli(fd,
	        "---------------------------------------------------------------------\n"
	        "---------------------------------------------------------------------\n"
	        "\n");

	return RESULT_SUCCESS;
}

static struct ast_cli_entry cli_funcdevstate[] = {
	{ { "funcdevstate", "list", }, cli_funcdevstate_list, NULL, NULL },
};

static struct ast_custom_function devstate_function = {
	.name = "DEVSTATE",
	.synopsis = "Get or Set a device state",
	.syntax = "DEVSTATE(device)",
	.desc =
	"  The DEVSTATE function can be used to retrieve the device state from any\n"
	"device state provider.  For example:\n"
	"   NoOp(SIP/mypeer has state ${DEVSTATE(SIP/mypeer)})\n"
	"   NoOp(Conference number 1234 has state ${DEVSTATE(MeetMe:1234)})\n"
	"\n"
	"  The DEVSTATE function can also be used to set custom device state from\n"
	"the dialplan.  The \"Custom:\" prefix must be used.  For example:\n"
	"  Set(DEVSTATE(Custom:lamp1)=BUSY)\n"
	"  Set(DEVSTATE(Custom:lamp2)=NOT_INUSE)\n"
	"You can subscribe to the status of a custom device state using a hint in\n"
	"the dialplan:\n"
	"  exten => 1234,hint,Custom:lamp1\n"
	"\n"
	"  The possible values for both uses of this function are:\n"
	"UNKNOWN | NOT_INUSE | INUSE | BUSY | INVALID | UNAVAILABLE | RINGING\n"
	"RINGINUSE | ONHOLD\n",
	.read = devstate_read,
	.write = devstate_write,
};

static int unload_module(void)
{
	int res = 0;

	res |= ast_custom_function_unregister(&devstate_function);
	ast_devstate_prov_del("Custom");
	ast_cli_unregister_multiple(cli_funcdevstate, ARRAY_LEN(cli_funcdevstate));

	return res;
}

static int load_module(void)
{
	int res = 0;

	res |= ast_custom_function_register(&devstate_function);
	res |= ast_devstate_prov_add("Custom", custom_devstate_callback);
	ast_cli_register_multiple(cli_funcdevstate, ARRAY_LEN(cli_funcdevstate));

	return res;
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "Gets or sets a device state in the dialplan");
