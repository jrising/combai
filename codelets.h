#ifndef CODELETS_H
#define CODELETS_H

class Codelet;

#include <time.h>
#include "base.h"
#include "coderack.h"
#include "workspace.h"

#define TYPEDOC_UDIV 1000.
#define TYPEDOC_UINT .25

#define EXP_BOND_MAX .75

#define PRIV_FLAG 0x01

/*  codelet base class  */
class Codelet : public AIObject {
public:
  Codelet(urgetype urge);
  Codelet(FILE *fp);
  Codelet();

  urgetype getUrgency();
  urgetype getComplacency();
  int getFlags();
  virtual void Execute() = NULL;
  virtual const char *Class() const = NULL;
  virtual int AssertValid() = NULL;

  virtual int WriteObject(FILE *fp);

protected:
  urgetype urgency;

  int flags;
};

class ReadKeyboardCodelet : public Codelet {
public:
  ReadKeyboardCodelet(WorkspaceRef *last, Workspace *ws, urgetype urge);
  ReadKeyboardCodelet(WorkspaceRef *last, Workspace *ws, urgetype urge,
		       clock_t startt);
  ReadKeyboardCodelet(FILE *fp);

  virtual void Execute();
  virtual const char *Class() const;
  virtual int AssertValid();

  virtual int WriteObject(FILE *fp);

  static clock_t reference_tdiff;

private:
  int kbhit();
  int getch();

  WorkspaceRef *lastelt;
  Workspace *workspace;
  clock_t initial;
  urgetype lasturge;
};

#define FILESAVED_SIZE 128

class TypeDocumentCodelet : public Codelet {
public:
  TypeDocumentCodelet(char *filename, Coderack *coderack);
  TypeDocumentCodelet(FILE *fp, Coderack *coderack, urgetype urge);
  TypeDocumentCodelet(char *filename, FILE *fp, Coderack *coderack,
		      urgetype urge);
  TypeDocumentCodelet(FILE *fp);

  virtual void Execute();
  virtual const char *Class() const;
  virtual int AssertValid();

  virtual int WriteObject(FILE *fp);

private:
  FILE *fp;
  Coderack *coderack;
  char filesaved[128];
};

#endif
