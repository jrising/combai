#ifndef CHECKER_H
#define CHECKER_H

#include "codelets.h"

#define CHKWRK_URGE .1
#define CHKCDR_URGE .1
#define CHKMEM_URGE .1

class CheckWorkspace : public Codelet {
public:
  CheckWorkspace(Workspace &ws);
  CheckWorkspace(FILE *fp, Workspace &ws);

  virtual void Execute();
  virtual const char *Class() const;
  virtual int AssertValid();

  virtual int WriteObject(FILE *fp);

private:
  Workspace &workspace;
};

class CheckCoderack : public Codelet {
public:
  CheckCoderack(Coderack &rack);
  CheckCoderack(FILE *fp, Coderack &cr);

  virtual void Execute();
  virtual const char *Class() const;
  virtual int AssertValid();

  virtual int WriteObject(FILE *fp);

private:
  Coderack &coderack;
};

class CheckMemory : public Codelet {
public:
  CheckMemory(Coderack &rack);
  CheckMemory(FILE *fp, Coderack &cr);

  virtual void Execute();
  virtual const char *Class() const;
  virtual int AssertValid();

  virtual int WriteObject(FILE *fp);

private:
  Coderack &coderack;
};

#endif
