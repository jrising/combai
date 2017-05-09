#include "readall.h"
#include "base.h"
#include "memtrack.h"
#include "workspace.h"
#include "system.h"
#include "codelets.h"
#include "evolet.h"
#include "checker.h"
#include "textshow.h"

struct BasePointers ReadAllObjects(FILE *fp) {
  PointerMapLink *root = NULL;
  PointerMapLink *prev;
  void *oldptr;
  void *newptr;
  void *cntdptr;
  classtype type;
  struct BasePointers adnl;

  printf("Test: %ld\n", fp);

  // Read in all objects
  while (fread(&oldptr, sizeof(void *), 1, fp)) {
    fread(&type, sizeof(classtype), 1, fp);

    verbize(-3, "", "Reading object %ld, of type %d\n", oldptr, type);

    switch (type) {
    case CInvalidClass: {
      unsigned long size;
      fread(&size, sizeof(unsigned long), 1, fp);
      newptr = aialloc(size, "reading array", 1, -1);
      root = new PointerMapLink(oldptr, newptr, root);
      break;
    }
    case CMemoryWorkspace: {
      root = new PointerMapLink(oldptr, new MemoryWorkspace(fp), root);
      break;
    }
    case CEvolSystemBasic: {
      root = new PointerMapLink(oldptr, new EvolSystemBasic(fp), root);
      break;
    }
    case CEvolSystemCombo: {
      root = new PointerMapLink(oldptr, new EvolSystemCombo(fp), root);
      break;
    }
    case CCoderack: {
      root = new PointerMapLink(oldptr, new Coderack(fp), root);
      break;
    }
    case CCoderackBranch: {
      root = new PointerMapLink(oldptr, new CoderackBranch(fp), root);
      break;
    }
    case CCoderackRoot: {
      root = new PointerMapLink(oldptr, new CoderackRoot(fp), root);
      break;
    }
    case CCoderackLeaf: {
      root = new PointerMapLink(oldptr, new CoderackLeaf(fp), root);
      break;
    }
    case CReadKeyboardCodelet: {
      root = new PointerMapLink(oldptr, new ReadKeyboardCodelet(fp), root);
      break;
    }
    case CQueueCodelet: {
      fread(&cntdptr, sizeof(void *), 1, fp);
      WorkspaceRef *ref = (WorkspaceRef *) root->FindNewPointer(cntdptr);
      root = new PointerMapLink(oldptr, new QueueCodelet(fp, *ref), root);
      break;
    }
    case CEvolaiCodelet: {
      fread(&cntdptr, sizeof(void *), 1, fp);
      printf("A\n");
      WorkspaceRef *ref = (WorkspaceRef *) root->FindNewPointer(cntdptr);
      printf("Here\n");
      root = new PointerMapLink(oldptr, new EvolaiCodelet(fp, *ref), root);
      break;
    }
    case CMoveSystemCodelet: {
      fread(&cntdptr, sizeof(void *), 1, fp);
      WorkspaceRef *ref = (WorkspaceRef *) root->FindNewPointer(cntdptr);
      root = new PointerMapLink(oldptr, new MoveSystemCodelet(fp, *ref), root);
      break;
    }
    case CJumpSystemCodelet: {
      fread(&cntdptr, sizeof(void *), 1, fp);
      WorkspaceRef *ref = (WorkspaceRef *) root->FindNewPointer(cntdptr);
      root = new PointerMapLink(oldptr, new JumpSystemCodelet(fp, *ref), root);
      break;
    }
    case CRepeatedCodelet: {
      fread(&cntdptr, sizeof(void *), 1, fp);
      WorkspaceRef *ref = (WorkspaceRef *) root->FindNewPointer(cntdptr);
      root = new PointerMapLink(oldptr, new RepeatedCodelet(fp, *ref), root);
      break;
    }
    case CCheckWorkspace: {
      fread(&cntdptr, sizeof(void *), 1, fp);
      Workspace *ws = (Workspace *) root->FindNewPointer(cntdptr);
      root = new PointerMapLink(oldptr, new CheckWorkspace(fp, *ws), root);
      break;
    }
    case CCheckCoderack: {
      fread(&cntdptr, sizeof(void *), 1, fp);
      Coderack *cr = (Coderack *) root->FindNewPointer(cntdptr);
      root = new PointerMapLink(oldptr, new CheckCoderack(fp, *cr), root);
      break;
    }
    case CWorkspaceRef: {
      root = new PointerMapLink(oldptr, new WorkspaceRef(fp), root);
      break;      
    }
    case CWorkspaceElt: {
      fread(&cntdptr, sizeof(void *), 1, fp);
      WorkspaceRef *ref1 = (WorkspaceRef *) root->FindNewPointer(cntdptr);
      fread(&cntdptr, sizeof(void *), 1, fp);
      WorkspaceRef *ref2 = (WorkspaceRef *) root->FindNewPointer(cntdptr);
      root = new PointerMapLink(oldptr, new WorkspaceElt(fp, *ref1, *ref2),
				root);
      break;
    }
    case CWorkspaceBond: {
      fread(&cntdptr, sizeof(void *), 1, fp);
      WorkspaceRef *ref1 = (WorkspaceRef *) root->FindNewPointer(cntdptr);
      fread(&cntdptr, sizeof(void *), 1, fp);
      WorkspaceRef *ref2 = (WorkspaceRef *) root->FindNewPointer(cntdptr);
      root = new PointerMapLink(oldptr, new WorkspaceBond(fp, *ref1, *ref2),
				root);
      break;
    }
    case CCheckMemory: {
      fread(&cntdptr, sizeof(void *), 1, fp);
      Coderack *cr = (Coderack *) root->FindNewPointer(cntdptr);
      root = new PointerMapLink(oldptr, new CheckMemory(fp, *cr), root);
    }
    case CTextShowWorkspace: {
      fread(&cntdptr, sizeof(void *), 1, fp);
      Workspace *ws = (Workspace *) root->FindNewPointer(cntdptr);
      root = new PointerMapLink(oldptr, new TextShowWorkspace(fp, *ws), root);
      break;
    }
    case CTextShowElement: {
      fread(&cntdptr, sizeof(void *), 1, fp);
      WorkspaceRef *ref = (WorkspaceRef *) root->FindNewPointer(cntdptr);
      root = new PointerMapLink(oldptr, new TextShowElement(fp, *ref), root);
      break;
    }
    }
  }
    
  // Add pointers on the root
  while (fread(&oldptr, sizeof(void *), 1, fp)) {
    char flag;
    if (!oldptr)
      break;
    fread(&flag, sizeof(char), 1, fp);
    TrackLink::MemStore(TrackLink::root, &oldptr, flag);
  }

  // Read additional pointers
  fread(&adnl, sizeof(struct BasePointers), 1, fp);
  adnl.basews = (MemoryWorkspace *) root->FindNewPointer(adnl.basews);

  // Fix Pointers
  TrackLink::FixPointers(root);

  // Delete pointer map
  while (root) {
    prev = root;
    root = root->next;
    delete prev;
  }

  return adnl;
}
