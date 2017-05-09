#include "coderack.h"
#include <stdlib.h>

Coderack::Coderack(unsigned long maxsz) :
  AIObject(CCoderack), maxsize(maxsz) {
  root = new CoderackRoot();
  head = root;
  size = 0;

  TrackLink::MemStore(trackid, &root, FALSE);
  TrackLink::MemStore(trackid, &head, FALSE);
}

Coderack::Coderack(FILE *fp) :
  AIObject(fp) {
  fread(&root, sizeof(CoderackRoot *), 1, fp);
  fread(&head, sizeof(CoderackRoot *), 1, fp);
  fread(&size, sizeof(unsigned long), 1, fp);
  fread(&maxsize, sizeof(unsigned long), 1, fp);

  TrackLink::MemStore(trackid, &root, FALSE);
  TrackLink::MemStore(trackid, &head, FALSE);
}

Coderack::~Coderack() {
  delete root;
}

void Coderack::AddCodelet(Codelet *cdlet) {
  urgetype urge = cdlet->getUrgency();

  verbize(-3, "debug", "Adding Codelet %ld: %s\n", cdlet, cdlet->Class());

  if (urge <= 0.) {
    delete cdlet;
    return;
  }

  if (size >= maxsize)
    RemoveCodelet();
  head = head->AddBranch(new CoderackLeaf(cdlet, urge));
  size++;
}

void Coderack::RemoveCodelet() {
  CoderackLeaf *todel = root->SelectRandomLeaf(frand());
  if (todel->getCodelet()->getFlags() & PRIV_FLAG)
    RemoveCodelet();  // don't remove this one

  head = todel->Remove()->getRoot(); /* In case select's root is head */

  delete todel;
  size--;
}

void Coderack::ExecuteCodelet() {
  verbize(-8, "debug", "Beginning ExecuteCodelet\n");
  
  CoderackLeaf *torun = root->SelectWeightLeaf(frand() * root->getSummed());

  if (!torun) {
    recalcTotalUrgency();
    return;
  }

  verbize(-3, "", "Executing codelet %ld: %s\n", torun->getCodelet(),
	  torun->getCodelet()->Class());

  torun->getCodelet()->Execute();

  head = torun->Remove()->getRoot(); /* In case select's root is head */
  delete torun;
  size--;
}

urgesumtype Coderack::getTotalUrgency() {
  return root->getSummed();
}

void Coderack::recalcTotalUrgency() {
  urgesumtype old = root->getSummed();
  root->recalcSummed();
}

void Coderack::print() {
  printf("Root is %ld, Head is %ld; Size is %ld of %ld.\n", root, head,
	 size, maxsize);
  root->print();
}

unsigned long Coderack::getSize() {
  return size;
}

unsigned long Coderack::getMaxSize() {
  return maxsize;
}

int Coderack::AssertValid() {
  AIObject *obj;

  /* Check all the way through the tree to make sure everything is good */
  /*   Check that root and head are valid */
  obj = dynamic_cast<AIObject*>(root);
  aiassert(obj && obj->type == CCoderackRoot, "checking validity of root");

  obj = dynamic_cast<AIObject*>(head);
  aiassert(obj && (obj->type == CCoderackRoot ||
		   obj->type == CCoderackBranch),
	   "checking validity of root\n");

  /*   Now ask root to validate all of its children, on down */
  aiassert(root->AssertValidChildren(head) == -1, "finding head under root");

  return (type == CCoderack);
}

int Coderack::WriteObject(FILE *fp) {
  size_t result = AIObject::WriteObject(fp);
  result = fwrite(&root, sizeof(CoderackRoot *), 1, fp) + result;
  result = fwrite(&head, sizeof(CoderackRoot *), 1, fp) + result;
  result = fwrite(&size, sizeof(unsigned long), 1, fp) + result;
  return fwrite(&maxsize, sizeof(unsigned long), 1, fp) + result;
}

/**********************/

CoderackNode::CoderackNode() :
  AIObject(CCoderackNode) {
  TrackLink::MemStore(trackid, &root, FALSE);
}

CoderackNode::CoderackNode(FILE *fp) :
  AIObject(fp) {
  fread(&summed, sizeof(urgesumtype), 1, fp);
  fread(&root, sizeof(CoderackNode *), 1, fp);

  TrackLink::MemStore(trackid, &root, FALSE);
}

urgesumtype CoderackNode::getSummed() {
  return summed;
}

void CoderackNode::updateSummed(urgetype delta) {
  summed += delta;
  if (root)
    root->updateSummed(delta);
}

void CoderackNode::setRoot(CoderackNode *rt) {
  root = rt;
}

CoderackNode *CoderackNode::getRoot() {
  return root;
}

int CoderackNode::WriteObject(FILE *fp) {
  size_t result = AIObject::WriteObject(fp);
  result = fwrite(&summed, sizeof(urgesumtype), 1, fp) + result;
  return fwrite(&root, sizeof(CoderackNode *), 1, fp) + result;
}

/******************************/

CoderackBranch::CoderackBranch(CoderackNode *rt) {
  type = CCoderackBranch;
  root = rt;
  summed = 0;
  childcnt = 0;
}

CoderackBranch::CoderackBranch(FILE *fp) :
  CoderackNode(fp) {
  fread(&childcnt, sizeof(unsigned), 1, fp);
  fread(child, sizeof(CoderackNode *), childcnt, fp);
}

CoderackBranch::~CoderackBranch() {
  for (int i = 0; i < childcnt; i++)
    delete child[i];
}

/* DOESN'T MATTER where it goes, just as long as all branches are equal */
/* Sets the branch's root to the branch it is added to */
/* Returns pointer to node that branch actually added to */
CoderackNode *CoderackBranch::AddBranch(CoderackNode *branch) {
  if (childcnt == 3) {
    CoderackBranch *spawn = new CoderackBranch((CoderackNode *) NULL);
    spawn->AddBranch(child[2]);
    spawn->AddBranch(branch);
    root->AddBranch(spawn);
    updateSummed(-child[2]->getSummed());
    childcnt = 2;
    TrackLink::MemRemove(trackid, &child[2]);
    return spawn;
  } else {
    child[childcnt++] = branch;
    TrackLink::MemStore(trackid, &child[childcnt - 1], FALSE);
    branch->setRoot(this);
    updateSummed(branch->getSummed());
    return this;
  }
}

/* Returns any child that is left */
CoderackNode *CoderackBranch::LeafRemoved(CoderackNode *branch) {
  int i, j;

  verbize(-9, "debug", "LeafRemoved on Branch %ld\n", this);

  if (childcnt == 3) {
    /* Just remove from list */
    for (i = 0; i < 3 && child[i] != branch; i++);
    for (j = i + 1; j < 3; j++)
      child[j - 1] = child[j];
    childcnt--;
    TrackLink::MemRemove(trackid, &child[childcnt]);
    updateSummed(-branch->getSummed());
    return child[0];
  } else {
    /* Recombine */
    CoderackNode *merger = root->LeafRemoved(this);
    CoderackNode *left = (child[0] == branch) ? child[1] : child[0];
    merger->AddBranch(left);
    childcnt = 0;
    delete this;
    return left;
  }
}

CoderackLeaf *CoderackBranch::SelectRandomLeaf(float select) {
  int seld = (int) (select * (float) childcnt);
  float news = (select * (float) childcnt) - (float) seld;
  return child[(int) (select * ((float) childcnt))]->SelectRandomLeaf(news);
}

CoderackLeaf *CoderackBranch::SelectWeightLeaf(urgesumtype select) {
  int i;
  verbize(-8, "debug", "In SWL for branch %ld: %f\n", this, select);
  for (i = 0; i < childcnt && select > child[i]->getSummed(); i++)
    select -= child[i]->getSummed();
  if (i == childcnt)
    return NULL;
  verbize(-9, "debug", "Child Found: %f: %d, %ld\n", select, i, child[i]);
  return child[i]->SelectWeightLeaf(select);
}

void CoderackBranch::print() {
  printf("%ld (%f): %ld; ", this, summed, root);
  for (int i = 0; i < childcnt; i++)
    printf("%ld ", child[i]);
  printf("\n");
  for (int i = 0; i < childcnt; i++)
    child[i]->print();
}

void CoderackBranch::recalcSummed() {
  urgesumtype newsum = 0.;
  for (int i = 0; i < childcnt; i++) {
    child[i]->recalcSummed();
    newsum += child[i]->getSummed();
  }
  summed = newsum;
}

int CoderackBranch::AssertValidChildren(CoderackNode *head) {
  AIObject *obj;
  int foundhead = 0;

  aiassert(childcnt == 3 || childcnt == 2, "checking size of branch");

  for (int i = 0; i < childcnt; i++) {
    obj = dynamic_cast<AIObject*>(child[i]);
    aiassert(obj && (obj->type == CCoderackLeaf ||
		     obj->type == CCoderackBranch),
	     "checking nature of children");
    foundhead |= child[i]->AssertValidChildren(head);
  }

  if (this == head)
    return -1;
  else
    return foundhead;
}

int CoderackBranch::WriteObject(FILE *fp) {
  size_t result = CoderackNode::WriteObject(fp);
  result = fwrite(&childcnt, sizeof(unsigned), 1, fp) + result;
  return fwrite(child, sizeof(CoderackNode *), childcnt, fp) + result;
}

/**********************/

CoderackRoot::CoderackRoot() {
  type = CCoderackRoot;
  summed = 0;
  child = this;
  root = NULL;

  TrackLink::MemStore(trackid, &child, FALSE);
}

CoderackRoot::CoderackRoot(FILE *fp) :
  CoderackNode(fp) {
  fread(&child, sizeof(CoderackNode *), 1, fp);

  TrackLink::MemStore(trackid, &child, FALSE);
}

CoderackRoot::~CoderackRoot() {
  if (child != this)
    delete child;
}

CoderackNode *CoderackRoot::AddBranch(CoderackNode *branch) {
  if (child != this) {
    CoderackBranch *spawn = new CoderackBranch(this);
    summed = 0;
    spawn->AddBranch(child);
    spawn->AddBranch(branch);
    child = spawn;
    return spawn;
  } else {
    child = branch;
    branch->setRoot(this);
    summed = branch->getSummed();
  }
}

CoderackNode *CoderackRoot::LeafRemoved(CoderackNode *branch) {
  verbize(-9, "debug", "LeafRemoved on Root %ld\n", this);

  if (branch == child)
    child = this;
  summed = 0;
  return child;
}

CoderackLeaf *CoderackRoot::SelectRandomLeaf(float select) {
  if (child == this)
    return NULL;
  return child->SelectRandomLeaf(select);
}

CoderackLeaf *CoderackRoot::SelectWeightLeaf(urgesumtype select) {
  verbize(-8, "debug", "In SWL for Root %ld: %f\n", this, select);
  if (child == this)
    return NULL;
  return child->SelectWeightLeaf(select);
}

void CoderackRoot::print() {
  printf("%ld (%f): %ld\n", this, summed, child);
  child->print();
}

void CoderackRoot::recalcSummed() {
  if (child) {
    child->recalcSummed();
    summed = child->getSummed();
  } else
    summed = 0.;
}

int CoderackRoot::AssertValidChildren(CoderackNode *head) {
  AIObject *obj;

  if (child) {
    obj = dynamic_cast<AIObject*>(child);
    aiassert(obj && (obj->type == CCoderackLeaf ||
		     obj->type == CCoderackBranch),
	     "checking nature of children");
    return child->AssertValidChildren(head);
  } else {
    if (this == head)
      return -1;
    else
      return 1;
  }
}

int CoderackRoot::WriteObject(FILE *fp) {
  size_t result = CoderackNode::WriteObject(fp);
  return fwrite(&child, sizeof(CoderackNode *), 1, fp) + result;
}

/*********************/

CoderackLeaf::CoderackLeaf(Codelet *cdlet, urgetype urge) {
  type = CCoderackLeaf;

  verbize(-5, "debug", "Creating new CoderackLeaf %ld with %ld\n", this, cdlet);

  if ((long) cdlet == (long) this) {
    verbize(1, "general", "Codelet pointer equals Coderack leaf pointer!\n");
    exit(UNSPEC_ERROR);
  }
  summed = urge;
  codelet = cdlet;

  TrackLink::MemStore(trackid, &codelet, FALSE);
}

CoderackLeaf::CoderackLeaf(FILE *fp) :
  CoderackNode(fp) {
  fread(&codelet, sizeof(Codelet *), 1, fp);

  TrackLink::MemStore(trackid, &codelet, FALSE);
}

CoderackLeaf::~CoderackLeaf() {
  delete codelet;
}

CoderackNode *CoderackLeaf::AddBranch(CoderackNode *branch) {
  /* ERROR! */
  fprintf(stderr, "ERROR: AddBranch on Leaf\n");
  return NULL;
}

CoderackNode *CoderackLeaf::LeafRemoved(CoderackNode *branch) {
  /* ERROR! */
  fprintf(stderr, "ERROR: LeafRemoved on Leaf\n");
  return NULL;
}

CoderackLeaf *CoderackLeaf::SelectRandomLeaf(float select) {
  return this;
}

CoderackLeaf *CoderackLeaf::SelectWeightLeaf(urgesumtype select) {
  verbize(-8, "debug", "In SWL for Leaf %ld: %f\n", this, select);
  return this;
}

void CoderackLeaf::print() {
  printf("%ld (%f): Leaf\n", this, summed);
}

void CoderackLeaf::recalcSummed() {
  // do nothing
}

CoderackNode *CoderackLeaf::Remove() {
  CoderackNode *left = root->LeafRemoved(this);
  root = NULL;
  return left;
}

Codelet *CoderackLeaf::getCodelet() {
  return codelet;
}

int CoderackLeaf::AssertValidChildren(CoderackNode *head) {
  AIObject *obj;

  obj = dynamic_cast<AIObject*>(codelet);
  aiassert(obj && codelet->AssertValid(), "checking validity of codelet");

  if (this == head)
    return -1;
  else
    return 1;
}

int CoderackLeaf::WriteObject(FILE *fp) {
  size_t result = CoderackNode::WriteObject(fp);
  return fwrite(&codelet, sizeof(Codelet *), 1, fp) + result;
}
