#include "shadrpc.h"

bool_t xdr_new_args(XDR *xdrs, struct sws_new_args *args) {
  if (!xdr_cnindex(xdrs, &args->id))
    return FALSE;
  if (!Workspace::xdr_ptr(xdrs, &args->higher))
    return FALSE;
  if (!Workspace::xdr_ptr(xdrs, &args->lower))
    return FALSE;
  if (!xdr_u_int(xdrs, &args->prity))
    return FALSE;
  if (!xdr_u_int(xdrs, &args->maxsz))
    return FALSE;
  return TRUE;
}

bool_t xdr_datash1_args(XDR *xdrs, struct sws_datash1_args *args) {
  if (!xdr_cnindex(xdrs, &args->id))
    return FALSE;
  if (!Workspace::xdr_ptr(xdrs, &args->newws))
    return FALSE;
  if (!xdr_reference(xdrs, &args->repl, sizeof(WorkspaceElt), xdr_wselt))
    return FALSE;
  return TRUE;
}
