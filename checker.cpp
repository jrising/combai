#include "checker.h"

CheckWorkspace::CheckWorkspace(Workspace &ws) :
  Codelet(CHKWRK_URGE), workspace(ws) {
  type = CCheckWorkspace;
  flags |= PRIV_FLAG;
  TrackLink::MemStore(trackid, &workspace, CONST_FLAG);
}

CheckWorkspace::CheckWorkspace(FILE *fp, Workspace &ws) :
  Codelet(fp), workspace(ws) {
  TrackLink::MemStore(trackid, &workspace, CONST_FLAG);
}

void CheckWorkspace::Execute() {
  /* check if higherws and lowerws are valid */
  Workspace *otherws;
  AIObject *obj;

  obj = dynamic_cast<AIObject*>(&workspace);
  aiassert(obj && (obj->type == CMemoryWorkspace ||
		   obj->type == CBigEndianWorkspace ||
		   obj->type == CLittleEndianWorkspace),
	   "checking validity of workspace");

  if (otherws = workspace.GetHigherWorkspace()) {
    obj = dynamic_cast<AIObject*>(otherws);
    aiassert(obj && (obj->type == CMemoryWorkspace ||
		     obj->type == CBigEndianWorkspace ||
		     obj->type == CLittleEndianWorkspace),
	     "checking validity of higherws");
  }
  if (otherws = workspace.GetLowerWorkspace()) {
    obj = dynamic_cast<AIObject*>(otherws);
    aiassert(obj && (obj->type == CMemoryWorkspace ||
		     obj->type == CBigEndianWorkspace ||
		     obj->type == CLittleEndianWorkspace),
	     "checking validity of lowerws");
  }

  verbize(-8, "debug", "Note: %ld <> %ld\n",
	  workspace.GetCurrentIndex(), workspace.GetMaxIndex());
  aiassert(workspace.GetCurrentIndex() <= workspace.GetMaxIndex(),
	   "checking validity of indices");

  /* Put back into coderack */
  workspace.GetCoderack().AddCodelet(new CheckWorkspace(workspace));
}

const char *CheckWorkspace::Class() const {
  return "CheckWorkspace";
}

int CheckWorkspace::AssertValid() {
  AIObject *obj;

  verbize(-2, "assert", "CheckWorkspace::AssertValid\n");

  obj = dynamic_cast<AIObject*>(&workspace);
  aiassert(obj && (obj->type == CMemoryWorkspace ||
		   obj->type == CBigEndianWorkspace ||
		   obj->type == CLittleEndianWorkspace), "valid workspace");

  return (type == CCheckWorkspace);
}

int CheckWorkspace::WriteObject(FILE *fp) {
  Workspace *ws = &workspace;

  size_t result = fwrite(&ws, sizeof(Workspace *), 1, fp);
  return Codelet::WriteObject(fp) + result;
}

CheckCoderack::CheckCoderack(Coderack &rack) :
  Codelet(CHKCDR_URGE), coderack(rack) {
  type = CCheckCoderack;
  flags |= PRIV_FLAG;
  TrackLink::MemStore(trackid, &coderack, CONST_FLAG);
}

CheckCoderack::CheckCoderack(FILE *fp, Coderack &cr) :
  Codelet(fp), coderack(cr) {
  TrackLink::MemStore(trackid, &coderack, CONST_FLAG);
}

void CheckCoderack::Execute() {
  AIObject *obj;
  urgesumtype totalurge;

  obj = dynamic_cast<AIObject*>(&coderack);
  aiassert(obj && obj->type == CCoderack, "checking validity of coderack");

  aiassert(coderack.getTotalUrgency() > 0., "checking positive urgency");

  /* Checking that total urgency is about right (fixing, error if too bad) */
  totalurge = coderack.getTotalUrgency();
  coderack.recalcTotalUrgency();
  aiassert(totalurge  - coderack.getTotalUrgency() < coderack.getMaxSize(),
	   "checking urgency isn't too badly calculated");

  aiassert(coderack.AssertValid(), "valid coderack");

  /* Put back into coderack */
  coderack.AddCodelet(new CheckCoderack(coderack));
}

const char *CheckCoderack::Class() const {
  return "CheckCoderack";
}

int CheckCoderack::AssertValid() {
  AIObject *obj;

  verbize(-2, "assert", "CheckCoderack::AssertValid\n");

  obj = dynamic_cast<AIObject*>(&coderack);
  aiassert(obj && obj->type == CCoderack, "valid coderack");

  return (type == CCheckCoderack);
}

int CheckCoderack::WriteObject(FILE *fp) {
  Coderack *cr = &coderack;

  size_t result = fwrite(&cr, sizeof(Coderack *), 1, fp);
  return Codelet::WriteObject(fp) + result;
}

CheckMemory::CheckMemory(Coderack &rack) :
  Codelet(CHKMEM_URGE), coderack(rack) {
  type = CCheckMemory;
  flags |= PRIV_FLAG;
  TrackLink::MemStore(trackid, &coderack, CONST_FLAG);
}

CheckMemory::CheckMemory(FILE *fp, Coderack &cr) :
  Codelet(fp), coderack(cr) {
  TrackLink::MemStore(trackid, &coderack, CONST_FLAG);
}

void CheckMemory::Execute() {
  void *result = TrackLink::MemMarkCheck();

  verbize(-3, "debug", "CheckMemory returned %ld\n", result);

  aiassert(!result, "memory leaklessness");

  /* Put back into coderack */
  coderack.AddCodelet(new CheckMemory(coderack));
}

const char *CheckMemory::Class() const {
  return "CheckMemory";
}

int CheckMemory::AssertValid() {
  AIObject *obj;

  verbize(-2, "assert", "CheckMemory::AssertValid\n");

  obj = dynamic_cast<AIObject*>(&coderack);
  aiassert(obj && obj->type == CCoderack, "valid coderack");

  return (type == CCheckMemory);
}

int CheckMemory::WriteObject(FILE *fp) {
  Coderack *cr = &coderack;

  size_t result = fwrite(&cr, sizeof(Coderack *), 1, fp);
  return Codelet::WriteObject(fp) + result;
}
