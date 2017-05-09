#ifndef SHADRPC_H
#define SHADRPC_H

#include <rpc/rpc.h>
#include <rpc/types.h>
#include <netconfig.h>
#include <time.h>
#include "workspace.h"

#define OPSTR_MAXSIZE 8
#define IMEXP_MAXSIZE 2147483647
#define BITPK_MAXSIZE 32
#define FFTWS_MAXSIZE 256

#define CAILIB_PROG  ((u_long) 0x12345678)
#define CAILIB_VERS  ((u_long) 0x00010000)
#define NETTYPE      "netpath"
#define TIMEOUT_TOTAL 30

// RPC Function Codes

// ShadowWorkspace structure functions
#define RPCSWS_NEW 0x01
#define RPCSWS_DELETE 0x04
#define RPCSWS_DATASH1 0x08
#define RPCSWS_DATASH2 0x09

static struct timeval time_out = {TIMEOUT_TOTAL, 0};

// Argument Structures
struct sws_new_args {
  CNIndex maxid;
  Workspace *higher;
  Workspace *lower;
  unsigned prity;
  unsigned maxsz;
}

struct sws_datash1_args {
  CNIndex id;
  Workspace *newws;
  WorkspaceElt &repl;
}

struct sws_datash2_args {
  CNIndex id;
  Workspace *newws;
  WorkspaceRef *repl;
}

bool_t xdr_new_args(XDR *xdrs, struct sws_new_args *args);
bool_t xdr_datash1_args(XDR *xdrs, struct sws_datash1 *args);
bool_t xdr_datash2_args(XDR *xdrs, struct sws_datash2 *args);

#endif
