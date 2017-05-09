#ifndef SHADSVC_H
#define SHADSVC_H

/*
 * Server communicator -- takes remote commands and interacts with local and
 * remote workspaces
 */

#include "workspace.h"
#include "shadrpc.h"

class WorkspaceServer {
  WorkspaceServer();

  static void Dispatch(struct svc_req *rqstp, SVCXPTR *transp);

  // Workspace Services
  static void WorkspaceNew(SVCXPTR *transp);
  static void WorkspaceDelete(SVCXPTR *transp);
}


#endif
