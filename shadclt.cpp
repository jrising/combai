#include "shadclt.h"

ShadowWorkspace::ShadowWorkspace(CNIndex maxid, Workspace *higher,
				 Workspace *lower, unsigned prity,
				 unsigned maxsz, int argsite) :
  Workspace(maxid, higher, lower, prity, maxsz) {
  struct sws_new_args args;
  Workspace *remote;

  type = CShadowWorkspace;

  args.maxid = maxid;
  args.higher = higher;
  args.lower = lower;
  args.prity = prity;
  args.maxsz = maxsz;

  aicall(clnt_find(argsite), RPCSWS_NEW, xdr_new_args, &args,
	 Workspace::xdr_ptr, &remote, time_out, "workspace new");

  created = TRUE;
  ws = remote;
  site = argsite;
}

ShadowWorkspace::ShadowWorkspace(Workspace *remote, CNIndex maxid,
				 Workspace *higher, Workspace *lower,
				 unsigned prity, unsigned maxsz, int argsite) :
  Workspace(maxid, higher, lower, prity, maxsz) {
  type = CShadowWorkspace;

  created = FALSE;
  ws = remote;
  site = argsite;
}

ShadowWorkspace::ShadowWorkspace(FILE *fp) :
  Workspace(fp) {
  verbize(-4, "debug", "Creating ShadowWorkspace from file\n");

  fread(&site, sizeof(int), 1, fp);
  fread(&ws, sizeof(Workspace *), 1, fp);
  fread(&created, sizeof(int), 1, fp);
}

ShadowWorkspace::~ShadowWorkspace() {
  if (created) {
    aicall(clnt_find(site), RPCSWS_DELETE, Workspace::xdr_ptr, &ws,
	   xdr_void, NULL, time_out, "workspace delete");
  }
}

WorkspaceRef &ShadowWorkspace::DataShift(CNIndex id, Workspace *newws,
					 WorkspaceElt &repl) {
  struct sws_datash1_args args;
  

  args.id = id;
  args.newws = newws;
  args.repl = &repl;

  aicall(clnt_find(argsite), RPCSWS_DATASH1, xdr_datash1_args, &args,
	 WorkspaceRef::xdr_proc, &remote, time_out, "workspace datashift");
  
  return 
}
