#ifndef CODERACK_H
#define CODERACK_H

#include <stdio.h>

typedef float urgetype;
typedef double urgesumtype;

class Coderack;
class CoderackNode;
class CoderackBranch;
class CoderackRoot;
class CoderackLeaf;

#include "base.h"
#include "codelets.h"

/*  coderack structure  */
class Coderack : public AIObject {
public:
  Coderack(unsigned long maxsz);
  Coderack(FILE *fp);
  ~Coderack();

  void RemoveCodelet();
  void AddCodelet(Codelet *codelet);  // -codelet
  void ExecuteCodelet();
  urgesumtype getTotalUrgency();
  void recalcTotalUrgency();
  void print();
  unsigned long getSize();  // the current size
  unsigned long getMaxSize();

  int AssertValid();

  virtual int WriteObject(FILE *fp);

private:
  CoderackRoot *root;
  CoderackNode *head;
  unsigned long size;
  unsigned long maxsize;
};

class CoderackNode : public AIObject {
public:
  CoderackNode();
  CoderackNode(FILE *fp);

  virtual CoderackNode *AddBranch(CoderackNode *branch) = NULL;
  virtual CoderackNode *LeafRemoved(CoderackNode *branch) = NULL;
  virtual CoderackLeaf *SelectRandomLeaf(float select) = NULL;
  virtual CoderackLeaf *SelectWeightLeaf(urgesumtype select) = NULL;
  virtual void print() = NULL;

  void updateSummed(urgetype delta);
  urgesumtype getSummed();
  virtual void recalcSummed() = NULL;
  void setRoot(CoderackNode *rt);
  CoderackNode *getRoot();

  /* 1 if children are valid, 0 if a child is invalid, -1 if head found */
  virtual int AssertValidChildren(CoderackNode *head) = NULL;

  virtual int WriteObject(FILE *fp);

protected:
  urgesumtype summed;
  CoderackNode *root;
};

class CoderackBranch : public CoderackNode {
public:
  CoderackBranch(CoderackNode *root);
  CoderackBranch(FILE *fp);
  ~CoderackBranch();

  virtual CoderackNode *AddBranch(CoderackNode *branch);
  virtual CoderackNode *LeafRemoved(CoderackNode *branch);
  virtual CoderackLeaf *SelectRandomLeaf(float select);
  virtual CoderackLeaf *SelectWeightLeaf(urgesumtype select);
  virtual void print();

  virtual void recalcSummed();

  virtual int AssertValidChildren(CoderackNode *head);

  virtual int WriteObject(FILE *fp);

private:
  CoderackNode *child[3];
  unsigned childcnt;
};

class CoderackRoot : public CoderackNode {
public:
  CoderackRoot();
  CoderackRoot(FILE *fp);
  ~CoderackRoot();

  virtual CoderackNode *AddBranch(CoderackNode *branch);
  virtual CoderackNode *LeafRemoved(CoderackNode *branch);
  virtual CoderackLeaf *SelectRandomLeaf(float select);
  virtual CoderackLeaf *SelectWeightLeaf(urgesumtype select);
  virtual void print();

  virtual void recalcSummed();

  virtual int AssertValidChildren(CoderackNode *head);

  virtual int WriteObject(FILE *fp);

private:
  CoderackNode *child;
};

class CoderackLeaf : public CoderackNode {
public:
  CoderackLeaf(Codelet *cdlet, urgetype urge);
  CoderackLeaf(FILE *fp);
  ~CoderackLeaf();

  virtual CoderackNode *AddBranch(CoderackNode *branch);
  virtual CoderackNode *LeafRemoved(CoderackNode *branch);
  virtual CoderackLeaf *SelectRandomLeaf(float select);
  virtual CoderackLeaf *SelectWeightLeaf(urgesumtype select);
  virtual void print();

  virtual void recalcSummed();

  CoderackNode *Remove();

  Codelet *getCodelet();

  virtual int AssertValidChildren(CoderackNode *head);

  virtual int WriteObject(FILE *fp);

private:
  Codelet *codelet;
};

#endif

/* For testing purposes */

/*class Codelet {
public:
  Codelet(float urge) {
    urgency = urge;
    totalurge += urge;
    printf("%ld (%f) is created!\n", this, urgency);
  }

  ~Codelet() {
    totalurge -= urgency;
  }

  void execute() {
    printf("%ld (%f) is executed!\n", this, urgency);
  }

  static float totalurge;

private:
  float urgency;
  };*/
