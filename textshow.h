#ifndef TEXTSHOW_H
#define TEXTSHOW_H

#include "codelets.h"

#define TEXTWRK_URGE .1
#define TEXTELT_URGE .1

static unsigned char bondrep[8] = {'-', '<', '+', 'K', '*', '%', '#', '@'};

class TextShowWorkspace : public Codelet {
public:
  TextShowWorkspace(Workspace &ws);
  TextShowWorkspace(FILE *fp, Workspace &ws);

  virtual void Execute();
  virtual const char *Class() const;
  virtual int AssertValid();

  virtual int WriteObject(FILE *fp);

private:
  Workspace &workspace;
};

class TextShowElement : public Codelet {
public:
  TextShowElement(WorkspaceRef &ref);
  TextShowElement(FILE *fp, WorkspaceRef &ref);

  virtual void Execute();
  virtual const char *Class() const;
  virtual int AssertValid();

  virtual int WriteObject(FILE *fp);

private:
  WorkspaceRef &chosen;
};

#endif
