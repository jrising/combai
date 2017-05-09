#ifndef EVOLET_H
#define EVOLET_H

#define REPRO_PRISTINE .5
#define JUMP_STR .25

#include "codelets.h"

/* Predecessor to actual exectuion of a tower of organisms */
class QueueCodelet : public Codelet {
public:
  QueueCodelet(EvolSystemPtr sys, WorkspaceRef &ref); // -system
  QueueCodelet(FILE *fp, WorkspaceRef &ref);
  ~QueueCodelet();

  virtual void Execute();
  virtual const char *Class() const;
  virtual int AssertValid();

  virtual int WriteObject(FILE *fp);

private:
  WorkspaceRef &location;
  EvolSystemPtr system;
};

/* A "tower" of organisms cralling along the data */
class EvolaiCodelet : public Codelet {
public:
  EvolaiCodelet(WorkspaceRef &ref, urgetype urge); // execute the orgs on elt
  EvolaiCodelet(FILE *fp, WorkspaceRef &ref);

  virtual void Execute();
  virtual const char *Class() const;
  virtual int AssertValid();

  virtual int WriteObject(FILE *fp);

private:
  WorkspaceRef &location;
};

/* Data just predicted, move to next location */
class MoveSystemCodelet : public Codelet {
public:
  MoveSystemCodelet(WorkspaceRef &curr, EvolSystemPtr sys, char pred);
  MoveSystemCodelet(FILE *fp, WorkspaceRef &ref);
  ~MoveSystemCodelet();

  virtual void Execute();
  virtual const char *Class() const;
  virtual int AssertValid();

  virtual int WriteObject(FILE *fp);

private:
  WorkspaceRef &currloc;
  EvolSystemPtr system;
  Value prediction;
};

/* Invalid prediction on move, try to save */
class JumpSystemCodelet : public Codelet {
public:
  JumpSystemCodelet(WorkspaceRef &curr, EvolSystemPtr sys, char pred);
  JumpSystemCodelet(FILE *fp, WorkspaceRef &ref);
  ~JumpSystemCodelet();

  virtual void Execute();
  virtual const char *Class() const;
  virtual int AssertValid();

  virtual int WriteObject(FILE *fp);

private:
  WorkspaceRef &currloc;
  EvolSystemPtr system;
  Value prediction;
};

/* Repeated creating at the initial element */
class RepeatedCodelet : public Codelet {
public:
  RepeatedCodelet(WorkspaceRef &ref); // -system
  RepeatedCodelet(FILE *fp, WorkspaceRef &ref);
  ~RepeatedCodelet();

  virtual void Execute();
  virtual const char *Class() const;
  virtual int AssertValid();

  virtual int WriteObject(FILE *fp);

private:
  WorkspaceRef &location;
};

#endif
