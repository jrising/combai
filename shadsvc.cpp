#include "shadsvc.h"

WorkspaceServer::WorkspaceServer() {
  int transpnum;

  transpnum = svc_create(Dispatch, CAILIB_PROG, CAILIB_VERS, NETTYPE);
  aiassert(transpnum, "Server Startup\n");

  svc_run();
}

static void WorkspaceServer::Dispatch(struct svc_req *rqstp, SVCXPTR *transp) {
  switch (rqstp->rq_proc) {
  case NULLPROC:  // Do Nothing!
    svc_sendreply(transp, xdr_void, NULL);
    return;

    // Workspace Services
  case RPCSWS_NEW: WorkspaceNew(transp); break;
  case RPC_SWS_DELETE: WorkspaceDelete(transp); break;

  default:
    aiassert(0, "Unrecognized function to WorkspaceServer\n");
    return;
  }
}

static void WorkspaceNew(SVCXPTR *transp) {
  struct sws_new_args args;
  Workspace *ws;

  cec_svc_getargs(transp, xdr_new_args, &args, "workspace new");
  ws = new MemoryWorkspace(args.maxid, args.higher, args.lower, args.prity,
			   args.maxsz);
  cec_svc_sendreply(transp, Workspace::xdr_ptr, &ws, "workspace new");
  cec_svc_freeargs(transp, xdr_new_args, &args, "workspace new");
}

static void WorkspaceDelete(SVCXPTR *transp) {
  Workspace *ws;

  cec_svc_getargs(transp, Workspace::xdr_ptr, &ws, "workspace delete");
  delete ws;
  cec_svc_sendreply(transp, xdr_void, NULL, "workspace delete");
  cec_svc_freeargs(transp, Workspace::xdr_ptr, &ws, "workspace delete");
}
