/*
 * ShadowWorkspace - interacts like a workspace, but with all of the processing
 * and data occuring on another machine.
 */

#ifndef SHADOWS_H
#define SHADOWS_H

#include "workspace.h"
#include "shadrpc.h"

class ShadowWorkspace;
class ShadowWorkspaceRef;

class ShadowWorkspace : public Workspace {
public:
  ShadowWorkspace(CNIndex maxid, Workspace *higher, Workspace *lower,
		  unsigned prity, unsigned maxsz, int site);
  ShadowWorkspace(Workspace *ws, CNIndex maxid, Workspace *higher,
		  Workspace *lower, unsigned prity, unsigned maxsz, int site);
  ShadowWorkspace(FILE *fp);
  ~ShadowWorkspace();

  /* serious move must change all references */
  virtual WorkspaceRef &DataShift(CNIndex id, Workspace *newws,
				  WorkspaceElt &repl);
  virtual WorkspaceRef &DataShift(CNIndex id, Workspace *newws,
				  WorkspaceRef &repl);
  virtual WorkspaceRef &DataSwitch(WorkspaceRef &here, WorkspaceRef &repl);

  virtual WorkspaceElt *GetElement(CNIndex id);
  virtual void SetElement(CNIndex id, const WorkspaceElt &elt);

  virtual WorkspaceRef &SalientElement(CNIndex i);

  virtual int WriteObject(FILE *fp);

private:
  virtual WorkspaceRef &RoomAddElement(WorkspaceElt &elt);
  virtual WorkspaceRef &RoomAddElement(WorkspaceRef &ref);
  virtual void SetSalient(CNIndex i, WorkspaceRef &ref);

  int site;
  Workspace *ws;
  int created;
}

/* returns data from array of server-local references to catch changes */
class ShadowWorkspaceRef : public WorkspaceRef {
public:
  ShadowWorkspaceRef(ShadowWorkspace *lkup, CNIndex loc);
  ShadowWorkspaceRef(FILE *fp);
  ~ShadowWorkspaceRef();

  virtual Workspace *GetWorkspace();
  virtual CNIndex GetLocation();

  virtual int WriteObject(FILE *fp);

private:
  int site;
  CNIndex redir;
}

#define aicall(clnt, proc, xdrin, adrin, xdrout, adrout, time, str) \
  if (clnt_call(clnt, proc, xrdin, (caddr_t) adrin, \
                xdrout, (caddr_t) adrout, time) != RPC_SUCCESS) { \
    verbize(2, "error", "%s\n", clnt_sperror(clnt, str)); \
    return; \
  }\

#define aifreeres(clnt, xdrout, adrout, str) \
  if (!clnt_freeres(clnt, xdrout, (caddr_t) adrout)) { \
    verbize(2, "error", "%s\n", clnt_sperror(clnt, str)); \
    return; \
  }

void add_clnt(char *host, int site);
CLIENT *clnt_find(int site);
void remove_clnts();

typedef struct clnt_list_st {
  CLIENT *clnt;
  int site;
  struct clnt_list_st *next;
} clnt_list;

static clnt_list *clnt_root;

#endif
