#include <math.h>
#include <ctype.h>
#include "evolet.h"

extern float effectiveness;
extern unsigned long predtotal;

/***************************************************************/

QueueCodelet::QueueCodelet(EvolSystemPtr sys, WorkspaceRef &ref) :
  Codelet(UrgeRange * sys->Credibility()), location(ref) {
  type = CQueueCodelet;
  system = sys;

  TrackLink::MemStore(trackid, &location, CONST_FLAG);
  TrackLink::MemStore(trackid, system.GetSystemPP(), FALSE);
}

QueueCodelet::QueueCodelet(FILE *fp, WorkspaceRef &ref) :
  Codelet(fp), location(ref) {

  fread(system.GetSystemPP(), sizeof(Workspace *), 1, fp);

  TrackLink::MemStore(trackid, &location, CONST_FLAG);
  TrackLink::MemStore(trackid, system.GetSystemPP(), FALSE);  
}

QueueCodelet::~QueueCodelet() {
}

/* Setup or add to queue on a given element */
void QueueCodelet::Execute() {
  /* kill such sections as have used up life */
  EvolSystemPtr survive = system->CheckLife();
  if (!survive.GetSystem())
    return;

  /* if this is first time for this element */
  WorkspaceElt &elt(location.GetElement());
  verbize(-5, "debug", "Adding new system %ld to queue on %ld\n",
	  survive.GetSystem(), &elt);
  if (elt.AddQueue(survive)) {
    location.GetWorkspace()->GetCoderack().
      AddCodelet(new EvolaiCodelet(location, elt.QueueUrgency()));
  }
}

const char *QueueCodelet::Class() const {
  return "QueueCodelet";
}

int QueueCodelet::AssertValid() {
  AIObject *obj;

  verbize(-2, "assert", "QueueCodelet::AssertValid\n");

  obj = dynamic_cast<AIObject*>(&location);
  aiassert(obj && obj->type == CWorkspaceRef, "valid location");

  obj = dynamic_cast<AIObject*>(system.GetSystem());
  aiassert(obj && system->AssertValid(), "valid system");

  return (type == CQueueCodelet);
}

int QueueCodelet::WriteObject(FILE *fp) {
  WorkspaceRef *wr = &location;

  size_t result = fwrite(&wr, sizeof(WorkspaceRef *), 1, fp);
  result = Codelet::WriteObject(fp) + result;
  return fwrite(system.GetSystemPP(), sizeof(EvolSystem *), 1, fp) + result;
}

/***************************************************************/

EvolaiCodelet::EvolaiCodelet(WorkspaceRef &ref, urgetype urge) :
  Codelet(urge), location(ref) {
  type = CEvolaiCodelet;

  TrackLink::MemStore(trackid, &location, CONST_FLAG);
}

EvolaiCodelet::EvolaiCodelet(FILE *fp, WorkspaceRef &ref) :
  Codelet(fp), location(ref) {

  TrackLink::MemStore(trackid, &location, CONST_FLAG);
}

void EvolaiCodelet::Execute() {
  WorkspaceElt &elt(location.GetElement());
  EvolSystemPtr system = elt.RemoveQueue();
  if (system.GetSystem()) {
    verbize(-8, "debug", "Got System %ld:%d\n", system.GetSystem(),
	    system->type);
    location.GetWorkspace()->GetCoderack().
      AddCodelet(new MoveSystemCodelet(location, system,
				       system->execute(elt.GetValue())));
  }
}

const char *EvolaiCodelet::Class() const {
  return "EvolaiCodelet";
}

int EvolaiCodelet::AssertValid() {
  AIObject *obj;

  verbize(-2, "assert", "EvolaiCodelet::AssertValid\n");

  obj = dynamic_cast<AIObject*>(&location);
  aiassert(obj && obj->type == CWorkspaceRef, "valid location");

  return (type == CEvolaiCodelet);
}

int EvolaiCodelet::WriteObject(FILE *fp) {
  WorkspaceRef *wr = &location;

  size_t result = fwrite(&wr, sizeof(WorkspaceRef *), 1, fp);
  return Codelet::WriteObject(fp) + result;
}

/***************************************************************/

MoveSystemCodelet::MoveSystemCodelet(WorkspaceRef &curr, EvolSystemPtr sys,
				     char pred) :
  Codelet(UrgeRange * sys->Credibility()), currloc(curr) {
  type = CMoveSystemCodelet;
  verbize(-6, "debug", "Creating MoveSystemCodelet %ld with %ld (%f)\n", this,
	  sys.GetSystem(), sys->Credibility());
  system = sys;
  prediction = pred;

  TrackLink::MemStore(trackid, &currloc, CONST_FLAG);
  TrackLink::MemStore(trackid, system.GetSystemPP(), FALSE);
}

MoveSystemCodelet::MoveSystemCodelet(FILE *fp, WorkspaceRef &ref) :
  Codelet(fp), currloc(ref) {
  fread(system.GetSystemPP(), sizeof(EvolSystem *), 1, fp);
  fread(&prediction, sizeof(Value), 1, fp);

  TrackLink::MemStore(trackid, &currloc, CONST_FLAG);
  TrackLink::MemStore(trackid, system.GetSystemPP(), FALSE);
}

MoveSystemCodelet::~MoveSystemCodelet() {
}

void MoveSystemCodelet::Execute() {
  WorkspaceBond *dir = currloc.BorrowElement().SelectWeightBond();
  EvolSystemPtr child;

  if (!dir) { // nothing to do yet
    if (isprint(prediction)) {
      verbize(-2, "status", "Input Prediction (%ld): %c\n", currloc.GetLocation(), prediction);
      printf("(%c)", prediction);
      fflush(NULL);
    } else
      verbize(-2, "status", "Input Prediction (%ld): %d\n", currloc.GetLocation(), prediction);
    currloc.GetWorkspace()->GetCoderack().
      AddCodelet(new MoveSystemCodelet(currloc, system, prediction));
  } else if (prediction == dir->To().BorrowElement().GetValue()) {
    verbize(-6, "debug", "Executing correct prediction\n");
    effectiveness += 1.;
    predtotal++;
    dir->Strengthen(system->Credibility());
    system->PredSuccess();
    /* move along */
    currloc.GetWorkspace()->GetCoderack().
      AddCodelet(new QueueCodelet(system, dir->To()));
    /* reproduce the system */
    child = system->reproduce(REPRO_PRISTINE);
    if (child.GetSystem()) {
      child->mutate();
      currloc.GetWorkspace()->GetCoderack().
	AddCodelet(new QueueCodelet(child, currloc));
    }
  } else {
    verbize(-7, "debug", "Executing Else Case (%ld, %ld)\n", dir,
	    system.GetSystem());
    dir->Weaken(system->Credibility());
    if (ProbToBool(sqrt(system->Credibility()))) { /* do we know what doing? */
      effectiveness -= .5;
      predtotal++;
      system->Weaken(Confidence(sqr(dir->GetStrength()))); /* don't week much for re-move */
      currloc.GetWorkspace()->GetCoderack().
	AddCodelet(new MoveSystemCodelet(currloc, system, prediction));
    } else if (ProbToBool(system->Credibility())) {
      verbize(-6, "debug", "Executing excused for jump\n");
      effectiveness -= .1;
      predtotal++;
      system->Weaken(Confidence(dir->GetStrength())); /* weaken move for jump */
      currloc.GetWorkspace()->GetCoderack().
	AddCodelet(new JumpSystemCodelet(dir->To(), system, prediction));
    } else {
      verbize(-6, "debug", "Executing unexcused failure\n");
      effectiveness -= 1.;
      predtotal++;
      EvolSystemComboPtr combo =
	dynamic_cast<EvolSystemCombo *>(system.GetSystem());

      system->PredFailure();

      if (combo.GetSystem()) {
	/* break up the system if it's a combo */
	currloc.GetWorkspace()->GetCoderack().
	  AddCodelet(new MoveSystemCodelet(currloc, combo->GetAbove(),
					   prediction));
	currloc.GetWorkspace()->GetCoderack().
	  AddCodelet(new MoveSystemCodelet(currloc, combo->GetBelow(),
					   prediction));
      } else
	currloc.GetWorkspace()->GetCoderack().
	  AddCodelet(new QueueCodelet(system, dir->To()));
    }
  }
}

const char *MoveSystemCodelet::Class() const {
  return "MoveSystemCodelet";
}

int MoveSystemCodelet::AssertValid() {
  AIObject *obj;

  verbize(-2, "assert", "MoveSystemCodelet::AssertValid\n");

  obj = dynamic_cast<AIObject*>(&currloc);
  aiassert(obj && obj->type == CWorkspaceRef, "valid currloc");

  obj = dynamic_cast<AIObject*>(system.GetSystem());
  aiassert(obj && system->AssertValid(), "valid system");

  return (type == CMoveSystemCodelet);
}

int MoveSystemCodelet::WriteObject(FILE *fp) {
  WorkspaceRef *wr = &currloc;

  size_t result = fwrite(&wr, sizeof(WorkspaceRef *), 1, fp);
  result = Codelet::WriteObject(fp) + result;
  result = fwrite(system.GetSystemPP(), sizeof(EvolSystem *), 1, fp) + result;
  return fwrite(&prediction, sizeof(Value), 1, fp) + result;
}

/***************************************************************/

JumpSystemCodelet::JumpSystemCodelet(WorkspaceRef &curr, EvolSystemPtr sys,
				     char pred) :
  Codelet(UrgeRange * sys->Credibility()), currloc(curr) {
  type = CJumpSystemCodelet;
  system = sys;
  prediction = pred;

  TrackLink::MemStore(trackid, &currloc, CONST_FLAG);
  TrackLink::MemStore(trackid, system.GetSystemPP(), FALSE);
}

JumpSystemCodelet::JumpSystemCodelet(FILE *fp, WorkspaceRef &ref) :
  Codelet(fp), currloc(ref) {
  fread(system.GetSystemPP(), sizeof(EvolSystem *), 1, fp);
  fread(&prediction, sizeof(Value), 1, fp);

  TrackLink::MemStore(trackid, &currloc, CONST_FLAG);
  TrackLink::MemStore(trackid, system.GetSystemPP(), FALSE);
}

JumpSystemCodelet::~JumpSystemCodelet() {
}

void JumpSystemCodelet::Execute() {
  int spacing = currloc.GetWorkspace()->GetCurrentIndex() / 256 + 1;

  for (unsigned long i = irand(spacing);
       i < currloc.GetWorkspace()->GetCurrentIndex();
       i += irand(spacing) + 1) {
    if (currloc.GetWorkspace()->SalientElement(i).BorrowElement().GetValue() == prediction) {
      if (i == currloc.BorrowElement().GetSalientLoc().GetLocation())
	continue;
      
      WorkspaceElt &elt = currloc.GetElement();

      system->PredSuccess();
      /* Add new bond to this */
      elt.AddBond(currloc.GetWorkspace()->SalientElement(i),
		  JUMP_STR, EvolaiBond);
      elt.Commit();
      /* Continue execution at that location */
      currloc.GetWorkspace()->GetCoderack().
	AddCodelet(new QueueCodelet(system, currloc.GetWorkspace()->SalientElement(i)));
      return;
    }
  }
  /* nothing was found, try again */
  currloc.GetWorkspace()->GetCoderack().
    AddCodelet(new MoveSystemCodelet(currloc, system, prediction));
}

const char *JumpSystemCodelet::Class() const {
  return "JumpSystemCodelet";
}

int JumpSystemCodelet::AssertValid() {
  AIObject *obj;

  verbize(-2, "assert", "JumpSystemCodelet::AssertValid\n");

  obj = dynamic_cast<AIObject*>(&currloc);
  aiassert(obj && obj->type == CWorkspaceRef, "valid currloc");

  obj = dynamic_cast<AIObject*>(system.GetSystem());
  aiassert(obj && system->AssertValid(), "valid system");

  return (type == CJumpSystemCodelet);
}

int JumpSystemCodelet::WriteObject(FILE *fp) {
  WorkspaceRef *wr = &currloc;

  size_t result = fwrite(&wr, sizeof(WorkspaceRef *), 1, fp);
  result = Codelet::WriteObject(fp) + result;
  result = fwrite(system.GetSystemPP(), sizeof(EvolSystem *), 1, fp) + result;
  return fwrite(&prediction, sizeof(Value), 1, fp) + result;
}

/***************************************************************/

RepeatedCodelet::RepeatedCodelet(WorkspaceRef &ref) :
  Codelet(UrgeRange /
	  (float) (ref.GetWorkspace()->GetCurrentIndex() *
		   ref.GetWorkspace()->GetCoderack().getSize())), 
  location(ref) {
  type = CRepeatedCodelet;

  TrackLink::MemStore(trackid, &location, CONST_FLAG);
}

RepeatedCodelet::RepeatedCodelet(FILE *fp, WorkspaceRef &ref) :
  Codelet(fp), location(ref) {

  TrackLink::MemStore(trackid, &location, CONST_FLAG);
}

RepeatedCodelet::~RepeatedCodelet() {
}

/* Setup or add to queue on a given element */
void RepeatedCodelet::Execute() {
  EvolSystemBasicPtr newsys;
  location.GetWorkspace()->GetCoderack().
    AddCodelet(new QueueCodelet(newsys = new EvolSystemBasic(), location));
  newsys->mutate();
  location.GetWorkspace()->GetCoderack().
    AddCodelet(new RepeatedCodelet(location));
}

const char *RepeatedCodelet::Class() const {
  return "RepeatedCodelet";
}

int RepeatedCodelet::AssertValid() {
  AIObject *obj;

  verbize(-2, "assert", "RepeatedCodelet::AssertValid\n");

  obj = dynamic_cast<AIObject*>(&location);
  aiassert(obj && obj->type == CWorkspaceRef, "valid location");

  return (type == CRepeatedCodelet);
}

int RepeatedCodelet::WriteObject(FILE *fp) {
  WorkspaceRef *wr = &location;
  
  size_t result = Codelet::WriteObject(fp);
  return fwrite(&wr, sizeof(WorkspaceRef *), 1, fp) + result;
}
