/*
   Unix SMB/CIFS implementation.
   RPC pipe client

   Copyright (C) 2013      Guenther Deschner <gd@samba.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "includes.h"
#include "rpcclient.h"
#include "../librpc/gen_ndr/ndr_winspool.h"
#include "libsmb/libsmb.h"
#include "auth/gensec/gensec.h"
#include "auth/credentials/credentials.h"

/****************************************************************************
****************************************************************************/

static WERROR cmd_iremotewinspool_async_open_printer(struct rpc_pipe_client *cli,
						     TALLOC_CTX *mem_ctx,
						     int argc, const char **argv)
{
	NTSTATUS status;
	struct policy_handle hnd;
	struct spoolss_DevmodeContainer devmode_ctr;
	struct spoolss_UserLevelCtr client_info_ctr;
	struct spoolss_UserLevel1 level1;
	uint32_t access_mask = PRINTER_ALL_ACCESS;
	struct dcerpc_binding_handle *b = cli->binding_handle;
	struct GUID uuid;
	struct winspool_AsyncOpenPrinter r;
	struct cli_credentials *creds = gensec_get_credentials(cli->auth->auth_ctx);

	if (argc < 2) {
		printf("Usage: %s <printername> [access_mask]\n", argv[0]);
		return WERR_OK;
	}

	if (argc >= 3) {
		sscanf(argv[2], "%x", &access_mask);
	}

	status = GUID_from_string(IREMOTEWINSPOOL_OBJECT_GUID, &uuid);
	if (!NT_STATUS_IS_OK(status)) {
		return WERR_NOT_ENOUGH_MEMORY;
	}

	ZERO_STRUCT(devmode_ctr);

	level1.size	= 40;
	level1.client	= talloc_asprintf(mem_ctx, "\\\\%s", lp_netbios_name());
	W_ERROR_HAVE_NO_MEMORY(level1.client);
	level1.user	= cli_credentials_get_username(creds);
	level1.build	= 1381;
	level1.major	= 3;
	level1.minor	= 0;
	level1.processor = PROCESSOR_ARCHITECTURE_AMD64;

	client_info_ctr.level = 1;
	client_info_ctr.user_info.level1 = &level1;

	r.in.pPrinterName	= argv[1];
	r.in.pDatatype		= "RAW";
	r.in.pDevModeContainer	= &devmode_ctr;
	r.in.AccessRequired	= access_mask;
	r.in.pClientInfo	= &client_info_ctr;
	r.out.pHandle		= &hnd;

	/* Open the printer handle */

	status = dcerpc_binding_handle_call(b,
					    &uuid,
					    &ndr_table_iremotewinspool,
					    NDR_WINSPOOL_ASYNCOPENPRINTER,
					    mem_ctx,
					    &r);
	if (!NT_STATUS_IS_OK(status)) {
		return ntstatus_to_werror(status);
	}
	if (!W_ERROR_IS_OK(r.out.result)) {
		return r.out.result;
	}

	printf("Printer %s opened successfully\n", argv[1]);

	return WERR_OK;
}

/* List of commands exported by this module */
struct cmd_set iremotewinspool_commands[] = {

	{ "IRemoteWinspool"  },

	{ "winspool_AsyncOpenPrinter", RPC_RTYPE_WERROR, NULL,
		cmd_iremotewinspool_async_open_printer,
		&ndr_table_iremotewinspool,
		NULL, "Open printer handle", "" },

	{ NULL }
};