#include "workspace.h"

/* :: Helper Type Functions :: */

bool_t xdr_cnindex(XDR *xdrs, CNIndex *id) {
  if (!xdr_u_long(xdrs, &id))
    return FALSE;
  return TRUE;
}

bool_t xdr_bondstrength(XDR *xdrs, BondStrength *id) {
  if (!xdr_float(xdrs, &id))
    return FALSE;
  return TRUE;
}

bool_t xdr_value(XDR *xdrs, Value *val) {
  if (!xdr_u_char(xdrs, &val))
    return FALSE;
  return TRUE;
}

/* :: Workspace Functions :: */

Workspace::Workspace(CNIndex maxid, Workspace *lower, Workspace *higher,
		     unsigned prity, unsigned maxsz) :
  AIObject(CWorkspace) {
  lowerws = lower;
  higherws = higher;
  currindex = 0;

  aiassert(maxid > 0, "creating workspace");

  maxindex = maxid;
  priority = prity;

  theCoderack = new Coderack(maxsz);

  TrackLink::MemStore(trackid, &higherws, FALSE);
  TrackLink::MemStore(trackid, &lowerws, FALSE);
  TrackLink::MemStore(trackid, &theCoderack, FALSE);
}

Workspace::Workspace(FILE *fp) :
  AIObject(fp) {
  verbize(-4, "debug", "Creating Workspace from file...\n");

  fread(&currindex, sizeof(CNIndex), 1, fp);
  fread(&higherws, sizeof(Workspace *), 1, fp);
  fread(&lowerws, sizeof(Workspace *), 1, fp);
  fread(&maxindex, sizeof(CNIndex), 1, fp);
  fread(&priority, sizeof(unsigned), 1, fp);
  fread(&theCoderack, sizeof(Coderack *), 1, fp);

  TrackLink::MemStore(trackid, &higherws, FALSE);
  TrackLink::MemStore(trackid, &lowerws, FALSE);
  TrackLink::MemStore(trackid, &theCoderack, FALSE);
}

Workspace::~Workspace() {
  delete theCoderack;
}

/* Add with random salience rank */
WorkspaceRef &Workspace::AddElement(WorkspaceElt &elt) {
  if (currindex == maxindex) {
    CNIndex replace = irand(maxindex);
    return DataShift(replace, lowerws, elt);
  } else {
    WorkspaceRef &tmp(RoomAddElement(elt));
    currindex++;
    return tmp;
  }
}

WorkspaceRef &Workspace::AddElement(WorkspaceRef &ref) {
  if (currindex == maxindex) {
    CNIndex replace = irand(maxindex);
    return DataShift(replace, lowerws, ref);
  } else {
    WorkspaceRef &tmp(RoomAddElement(ref));
    currindex++;
    return tmp;
  }
}

/* Put WorkspaceRef ref into salience spot id of this workspace */
/* Move the current WorkspaceRef with that salience to ref's old spot */
void Workspace::SalientSwitch(CNIndex id, WorkspaceRef &ref, CNIndex oldid,
			      Workspace *oldws) {
  WorkspaceRef &old = SalientElement(id);
  SetSalient(id, ref);
  oldws->SetSalient(oldid, old);
}

Workspace *Workspace::GetHigherWorkspace() {
  return higherws;
}

Workspace *Workspace::GetLowerWorkspace() {
  return lowerws;
}

void Workspace::SetHigherWorkspace(Workspace *higher) {
  higherws = higher;
}

void Workspace::SetLowerWorkspace(Workspace *lower) {
  lowerws = lower;
}

CNIndex Workspace::GetCurrentIndex() {
  return currindex;
}

CNIndex Workspace::GetMaxIndex() {
  return maxindex;
}

unsigned Workspace::GetPriority() {
  return priority;
}

Coderack &Workspace::GetCoderack() {
  return *theCoderack;
}

int Workspace::WriteObject(FILE *fp) {
  size_t result = AIObject::WriteObject(fp);
  result = fwrite(&currindex, sizeof(CNIndex), 1, fp) + result;
  result = fwrite(&higherws, sizeof(Workspace *), 1, fp) + result;
  result = fwrite(&lowerws, sizeof(Workspace *), 1, fp) + result;
  result = fwrite(&maxindex, sizeof(CNIndex), 1, fp) + result;
  result = fwrite(&priority, sizeof(unsigned), 1, fp) + result;
  return fwrite(&theCoderack, sizeof(Coderack *), 1, fp) + result;
}

static bool_t Workspace::xdr_proc(XDR *xdrs, Workspace *ws) {
  if (!AIObject::xdr_proc(xdrs, ws))
    return FALSE;
  if (!xdr_cnindex(xdrs, ws->currindex))
    return FALSE;
  if (!xdr_ptr(xdrs, ws->higherws))
    return FALSE;
  if (!xdr_ptr(xdrs, ws->lowerws))
    return FALSE;
  if (!xdr_cnindex(xdrs, ws->maxindex))
    return FALSE;
  if (!xdr_u_int(xdrs, ws->priority))
    return FALSE;
  if (!Coderack::xdr_ptr(xdrs, ws->theCoderack))
    return FALSE;
  return TRUE;
}

/* :: MemoryWorkspace Functions :: */

MemoryWorkspace::MemoryWorkspace(CNIndex maxid, Workspace *higher,
                                 Workspace *lower, unsigned prity,
				 unsigned maxsz) :
  Workspace(maxid, higher, lower, prity, maxsz) {
  type = CMemoryWorkspace;
  data = (WorkspaceElt **) aialloc(sizeof(WorkspaceElt *) * GetMaxIndex(),
				   "MemoryWorkspace data array", 1, -1);
  refs = (WorkspaceRef **) aialloc(sizeof(WorkspaceRef *) * GetMaxIndex(),
				   "MemoryWorkspace data array", 1, -1);
  for (CNIndex i = 0; i < GetMaxIndex(); i++) {
    refs[i] = NULL;
    data[i] = NULL;
  }

  TrackLink::MemStore(trackid, &data, FALSE);
  TrackLink::MemStore(trackid, &refs, FALSE);
}

MemoryWorkspace::MemoryWorkspace(FILE *fp) :
  Workspace(fp) {
  verbize(-4, "debug", "Creating MemoryWorkspace from file\n");

  data = (WorkspaceElt **) aialloc(sizeof(WorkspaceElt *) * GetMaxIndex(),
				   "MemoryWorkspace data array", 1, -1);
  refs = (WorkspaceRef **) aialloc(sizeof(WorkspaceRef *) * GetMaxIndex(),
				   "MemoryWorkspace data array", 1, -1);
  fread(data, sizeof(WorkspaceElt *), GetCurrentIndex(), fp);
  fread(refs, sizeof(WorkspaceRef *), GetCurrentIndex(), fp);

  TrackLink::MemStore(trackid, &data, FALSE);
  TrackLink::MemStore(trackid, &refs, FALSE);
}

MemoryWorkspace::~MemoryWorkspace() {
  for (CNIndex i = 0; i < GetMaxIndex(); i++) {
    if (refs[i])
      delete refs[i];
    if (data[i])
      delete data[i];
  }
  delete[] data;
  delete[] refs;
}

/* actually move storage of data and change all references
   (but don't move references) */
/* need to also make a reference to the element elt */
WorkspaceRef &MemoryWorkspace::DataShift(CNIndex id, Workspace *newws,
					  WorkspaceElt &elt) {
  /* Move data into appropriate spots, update references */
  /* AddElement(WorkspaceRef) updates references for that element */
  newws->AddElement(data[id]->GetSalientLoc());

  data[id] = &elt;
  refs[id] = new WorkspaceRef(this, currindex);

  return *refs[id];
}

/* actually move storage of data and change all references
   (but don't move references) */
WorkspaceRef &MemoryWorkspace::DataShift(CNIndex id, Workspace *newws,
					  WorkspaceRef &repl) {
  /* Move data into appropriate spots, update references */
  /* AddElement(WorkspaceRef) updates references for that element */
  newws->AddElement(data[id]->GetSalientLoc());

  data[id] = repl.GetWorkspace()->GetElement(repl.GetLocation());
  repl.lookup = this;
  repl.location = id;
  return repl;
}

/* "proper" data switch: put each element where the other was */
WorkspaceRef &MemoryWorkspace::DataSwitch(WorkspaceRef &here,
					  WorkspaceRef &repl) {
  repl.GetWorkspace()->SetElement(repl.GetLocation(),
				  *here.GetWorkspace()->
				  GetElement(here.GetLocation()));
  CNIndex id = here.GetLocation();
  here.lookup = repl.GetWorkspace();
  here.location = repl.GetLocation();

  data[id] = repl.GetWorkspace()->GetElement(repl.GetLocation());
  repl.lookup = this;
  repl.location = id;
  return repl;
}

WorkspaceElt *MemoryWorkspace::GetElement(CNIndex id) {
  return data[id];
}

void MemoryWorkspace::SetElement(CNIndex id, const WorkspaceElt &elt) {
  verbize(-5, "debug", "Setting Element at %ld\n", id);
  if (data[id])
    delete data[id];
  verbize(-8, "debug", "Deleted old at %ld\n", id);
  data[id] = new WorkspaceElt(elt);
}

WorkspaceRef &MemoryWorkspace::SalientElement(CNIndex i) {
  return *refs[i];
}

void MemoryWorkspace::SetSalient(CNIndex i, WorkspaceRef &ref) {
  refs[i] = &ref;
}

WorkspaceRef &MemoryWorkspace::RoomAddElement(WorkspaceElt &elt) {
  verbize(-4, "debug", "Adding element to ID %ld\n", currindex);
  refs[currindex] = new WorkspaceRef(this, currindex);
  data[currindex] = new WorkspaceElt(elt, *refs[currindex],
				     *(new WorkspaceRef(this, currindex)));
  TrackLink::MemStore(trackid, &data[currindex], FALSE);
  TrackLink::MemStore(trackid, &refs[currindex], FALSE);

  return *refs[currindex];
}

WorkspaceRef &MemoryWorkspace::RoomAddElement(WorkspaceRef &ref) {
  data[currindex] = ref.StealElement();
  TrackLink::MemStore(trackid, &data[currindex], FALSE);

  ref.lookup = this;
  ref.location = currindex;
  return ref;
}

int MemoryWorkspace::WriteObject(FILE *fp) {
  size_t result = Workspace::WriteObject(fp);
  result = fwrite(data, sizeof(WorkspaceElt *), GetCurrentIndex(), fp)
    + result;
  return fwrite(refs, sizeof(WorkspaceRef *), GetCurrentIndex(), fp) + result;
}

static bool_t MemoryWorkspace::xdr_proc(XDR *xdrs, MemoryWorkspace *ws) {
  if (!Workspace::xdr_proc(xdrs, ws))
    return FALSE;
  if (!xdr_array(xdrs, &ws->data, &ws->maxindex, MAX_MEMWSSIZE,
		 sizeof(WorkspaceElt *), WorkspaceElt::xdr_ptr))
    return FALSE;
  if (!xdr_array(xdrs, &ws->data, &ws->maxindex, MAX_MEMWSSIZE,
		 sizeof(WorkspaceRef *), WorkspaceRef::xdr_ptr))
    return FALSE;
  return TRUE;
}

/* :: WorkspaceElt Functions :: */

WorkspaceElt::WorkspaceElt(WorkspaceRef &ref) :
  AIObject(CWorkspaceElt),
  reference(ref.BorrowElement().reference),
  salientloc(ref.BorrowElement().salientloc) {
  verbize(-5, "debug", "Creating WorkspaceElt [%ld] (R%ld)...\n", this, &ref);
  const WorkspaceElt &elt(ref.BorrowElement());
  value = elt.value;
  totalstr = elt.totalstr;
  bondcount = elt.bondcount;
  if (elt.root)
    root = new WorkspaceBond(*elt.root);
  else
    root = NULL;

  if (elt.squeue.GetSystem())
    squeue = elt.squeue;
  else
    squeue = NULL;

  allocated = 0;

  uppref = NULL;
  dnpref = NULL;
  uppull = 0;
  dnpull = 0;

  TrackLink::MemStore(trackid, &reference, CONST_FLAG);
  TrackLink::MemStore(trackid, &salientloc, CONST_FLAG);
  TrackLink::MemStore(trackid, squeue.GetSystemPP(), FALSE);
  TrackLink::MemStore(trackid, &root, FALSE);
  TrackLink::MemStore(trackid, &uppref, FALSE);
  TrackLink::MemStore(trackid, &dnpref, FALSE);
}

WorkspaceElt::WorkspaceElt(const WorkspaceElt &copy) :
  AIObject(CWorkspaceElt), reference(copy.reference),
  salientloc(copy.salientloc) {
  verbize(-5, "debug", "Creating WorkspaceElt [%ld] (E%ld)...\n", this, &copy);
  value = copy.value;
  totalstr = copy.totalstr;
  bondcount = copy.bondcount;
  if (copy.root)
    root = new WorkspaceBond(*copy.root);
  else
    root = NULL;

  if (copy.squeue.GetSystem())
    squeue = copy.squeue;
  else
    squeue = NULL;

  allocated = 0;

  uppref = NULL;
  dnpref = NULL;
  uppull = 0;
  dnpull = 0;

  TrackLink::MemStore(trackid, &reference, CONST_FLAG);
  TrackLink::MemStore(trackid, &salientloc, CONST_FLAG);
  TrackLink::MemStore(trackid, squeue.GetSystemPP(), FALSE);
  TrackLink::MemStore(trackid, &root, FALSE);
  TrackLink::MemStore(trackid, &uppref, FALSE);
  TrackLink::MemStore(trackid, &dnpref, FALSE);
}

WorkspaceElt::WorkspaceElt(const WorkspaceElt &copy, WorkspaceRef &ref) :
  AIObject(CWorkspaceElt), reference(ref), salientloc(copy.salientloc) {
  verbize(-5, "debug", "Creating WorkspaceElt [%ld] (K%ld)...\n", this, &copy);
  value = copy.value;
  totalstr = copy.totalstr;
  bondcount = copy.bondcount;
  if (copy.root)
    root = new WorkspaceBond(*copy.root);
  else
    root = NULL;

  if (copy.squeue.GetSystem())
    squeue = copy.squeue;
  else
    squeue = NULL;

  allocated = 0;

  uppref = NULL;
  dnpref = NULL;
  uppull = 0;
  dnpull = 0;

  TrackLink::MemStore(trackid, &reference, CONST_FLAG);
  TrackLink::MemStore(trackid, &salientloc, CONST_FLAG);
  TrackLink::MemStore(trackid, squeue.GetSystemPP(), FALSE);
  TrackLink::MemStore(trackid, &root, FALSE);
  TrackLink::MemStore(trackid, &uppref, FALSE);
  TrackLink::MemStore(trackid, &dnpref, FALSE);
}

WorkspaceElt::WorkspaceElt(const WorkspaceElt &copy, WorkspaceRef &ref,
			   WorkspaceRef &loc) :
  AIObject(CWorkspaceElt), reference(ref), salientloc(loc) {
  verbize(-5, "debug", "Creating WorkspaceElt [%ld] (K%ld)...\n", this, &copy);
  value = copy.value;
  totalstr = copy.totalstr;
  bondcount = copy.bondcount;
  if (copy.root)
    root = new WorkspaceBond(*copy.root);
  else
    root = NULL;

  if (copy.squeue.GetSystem())
    squeue = copy.squeue;
  else
    squeue = NULL;

  allocated = 0;

  uppref = NULL;
  dnpref = NULL;
  uppull = 0;
  dnpull = 0;

  TrackLink::MemStore(trackid, &reference, CONST_FLAG);
  TrackLink::MemStore(trackid, &salientloc, CONST_FLAG);
  TrackLink::MemStore(trackid, squeue.GetSystemPP(), FALSE);
  TrackLink::MemStore(trackid, &root, FALSE);
  TrackLink::MemStore(trackid, &uppref, FALSE);
  TrackLink::MemStore(trackid, &dnpref, FALSE);
}

WorkspaceElt::WorkspaceElt(Value val) :
  AIObject(CWorkspaceElt),
  reference(*(new WorkspaceRef(NULL, 0))),
  salientloc(*(new WorkspaceRef(NULL, 0))) {
  verbize(-5, "debug", "Creating WorkspaceElt [%ld] (V%d)...\n", this, val);
  value = val;
  root = NULL;
  totalstr = 0;
  bondcount = 0;
  squeue = NULL;

  allocated = 1;

  uppref = NULL;
  dnpref = NULL;
  uppull = 0;
  dnpull = 0;

  TrackLink::MemStore(trackid, &reference, CONST_FLAG);
  TrackLink::MemStore(trackid, &salientloc, CONST_FLAG);
  TrackLink::MemStore(trackid, squeue.GetSystemPP(), FALSE);
  TrackLink::MemStore(trackid, &root, FALSE);
  TrackLink::MemStore(trackid, &uppref, FALSE);
  TrackLink::MemStore(trackid, &dnpref, FALSE);
}

WorkspaceElt::WorkspaceElt(FILE *fp, WorkspaceRef &refloc,
			   WorkspaceRef &salloc) :
  AIObject(fp), reference(refloc), salientloc(salloc) {
  fread(&allocated, sizeof(int), 1, fp);
  fread(&value, sizeof(Value), 1, fp);
  fread(squeue.GetSystemPP(), sizeof(EvolSystem *), 1, fp);
  fread(&root, sizeof(WorkspaceBond *), 1, fp);
  fread(&totalstr, sizeof(BondStrength), 1, fp);
  fread(&bondcount, sizeof(unsigned), 1, fp);
  fread(&uppref, sizeof(Workspace *), 1, fp);
  fread(&dnpref, sizeof(Workspace *), 1, fp);
  fread(&uppull, sizeof(unsigned char), 1, fp);
  fread(&dnpull, sizeof(unsigned char), 1, fp);

  TrackLink::MemStore(trackid, &reference, CONST_FLAG);
  TrackLink::MemStore(trackid, &salientloc, CONST_FLAG);
  TrackLink::MemStore(trackid, squeue.GetSystemPP(), FALSE);
  TrackLink::MemStore(trackid, &root, FALSE);
  TrackLink::MemStore(trackid, &uppref, FALSE);
  TrackLink::MemStore(trackid, &dnpref, FALSE);
}

WorkspaceElt::~WorkspaceElt() {
  verbize(-6, "debug", "Deleting WorkspaceElt %ld (%ld, %d)...\n",
	  this, root, allocated);

  if (root)
    delete root;

  if (allocated) {
    delete &reference;
    delete &salientloc;
  }
}

Value WorkspaceElt::GetValue() const {
  return value;
}

WorkspaceRef &WorkspaceElt::GetSalientLoc() const {
  return salientloc;
}

void WorkspaceElt::AddBond(WorkspaceRef &toelt, BondStrength str,
			   BondType type) {
  if (str > 0.) {
    WorkspaceBond *bond = new WorkspaceBond(reference, toelt, str, type);
    AddBond(bond);
  }
}

void WorkspaceElt::AddBond(WorkspaceBond *bond) {
  if (bond->GetStrength() > 0.) {
    verbize(-5, "debug", "Adding new bond: %ld\n", bond);
    bond->SetNextBond(root);
    root = bond;
    bondcount++;
  } else
    delete bond;
}

WorkspaceBond *WorkspaceElt::SelectRandomBond() const {
  unsigned choice = irand(bondcount);
  WorkspaceBond *curr = root;

  while (choice && curr)
    curr = curr->GetNextBond();

  return curr;
}

WorkspaceBond *WorkspaceElt::SelectWeightBond() const {
  double choice = frand() * totalstr;
  WorkspaceBond *curr = root;

  verbize(-6, "debug", "Finding a bond %ld: %f\n", curr, choice);

  if (!curr)
    return NULL;

  while (choice > curr->GetStrength()) {
    verbize(-7, "debug", "Bond check %ld => %f\n", curr, curr->GetStrength());
    choice -= curr->GetStrength();
    curr = curr->GetNextBond();
  }

  return curr;
}

BondStrength WorkspaceElt::GetTotalStr() const {
  return totalstr;
}

unsigned WorkspaceElt::GetBondCount() const {
  return bondcount;
}

void WorkspaceElt::Pull(Workspace *puller) {
  Workspace *currws = reference.GetWorkspace();
  unsigned currpy = currws->GetPriority();
  unsigned pullpy = puller->GetPriority();

  if (currws == puller) { /* no need to move */
    if (uppull)
      uppull--;
    if (dnpull)
      dnpull--;
  } else if (pullpy > currpy) { /* encourage upward movement */
    if (!uppull) {
      uppref = puller;
      uppull++;
    } else if (uppref == puller)
      uppull++;
    else
      uppull--;
      
    if (dnpull)
      dnpull--;

    /* if sufficient pull, move! */
    if (uppull >= SUFF_PULL)
      uppref->DataSwitch(uppref->SalientElement(irand(uppref->GetMaxIndex())),
			 reference);
  } else { /* encourage downward movement */
    if (!dnpull) {
      dnpref = puller;
      dnpull++;
    } else if (dnpref == puller)
      dnpull++;
    else
      dnpull--;
      
    if (uppull)
      uppull--;

    /* if sufficient pull, move! */
    if (dnpull >= SUFF_PULL)
      dnpref->DataSwitch(dnpref->SalientElement(irand(dnpref->GetMaxIndex())),
			 reference);
  }
}

int WorkspaceElt::AddQueue(EvolSystemPtr newsys) {
  if (squeue.GetSystem()) {
    EvolSystemPtr saved = squeue;

    squeue = new EvolSystemCombo(squeue, newsys);
    verbize(-6, "debug", "Creating for queue on %ld new combo %ld\n", this,
	    squeue.GetSystem());
    return 0;
  } else {
    squeue = newsys;
    verbize(-6, "debug", "Creating queue initially on %ld with %ld\n", this,
	    squeue.GetSystem());
    return 1;
  }
}

EvolSystemPtr WorkspaceElt::RemoveQueue() {
  verbize(-7, "debug", "Removing Queue on %ld to produce sys %ld\n", this,
	  squeue.GetSystem());
  EvolSystemPtr saved = squeue;
  squeue = NULL;
  return saved;
}

unsigned WorkspaceElt::QueueSystemCount() {
  if (squeue.GetSystem())
    return squeue->GetTotalSysCount();
  else
    return 0;
}

/* currently based solely on salience */
urgetype WorkspaceElt::QueueUrgency() const {
  verbize(-8, "debug", "Getting the queue urgency for %ld\n", this);
  return UrgeRange * .5 *
    (1. - (((float) salientloc.GetLocation()) /
	   (float) salientloc.GetWorkspace()->GetMaxIndex()));
}

void WorkspaceElt::Commit() const {
  verbize(-5, "debug", "Commiting element %ld to workspace %ld\n",
	  &reference, reference.GetWorkspace());
  if (this == reference.GetWorkspace()->GetElement(reference.GetLocation())) {
    verbize(-7, "debug", "Trivial Commit\n");
    return; // Nothing to do
  }
  reference.GetWorkspace()->SetElement(reference.GetLocation(), *this);
  verbize(-7, "debug", "Successful commit\n");
}

int WorkspaceElt::WriteObject(FILE *fp) {
  WorkspaceRef *wr1 = &reference;
  WorkspaceRef *wr2 = &salientloc;

  size_t result = fwrite(&wr1, sizeof(WorkspaceRef *), 1, fp);
  result = fwrite(&wr1, sizeof(WorkspaceRef *), 1, fp) + result;
  result = AIObject::WriteObject(fp) + result;
  result = fwrite(&allocated, sizeof(int), 1, fp) + result;
  result = fwrite(&value, sizeof(Value), 1, fp) + result;
  result = fwrite(squeue.GetSystemPP(), sizeof(EvolSystem *), 1, fp) + result;
  result = fwrite(&root, sizeof(WorkspaceBond *), 1, fp) + result;
  result = fwrite(&totalstr, sizeof(BondStrength), 1, fp) + result;
  result = fwrite(&bondcount, sizeof(unsigned), 1, fp) + result;
  result = fwrite(&uppref, sizeof(Workspace *), 1, fp) + result;
  result = fwrite(&dnpref, sizeof(Workspace *), 1, fp) + result;
  result = fwrite(&uppull, sizeof(unsigned char), 1, fp) + result;
  return fwrite(&dnpull, sizeof(unsigned char), 1, fp) + result;
}

static bool_t WorkspaceElt::xdr_proc(XDR *xdrs, WorkspaceElt *elt) {
  if (!xdr_reference(xdrs, &elt->reference, sizeof(WorkspaceRef),
		     WorkspaceRef::xdr_proc))
    return FALSE;
  if (!xdr_reference(xdrs, &elt->salientloc, sizeof(WorkspaceRef),
		     WorkspaceRef::xdr_proc))
    return FALSE;
  if (!xdr_int(xdrs, &elt->allocated))
    return FALSE;
  if (!xdr_value(xdrs, &elt->value))
    return FALSE;
  if (!EvolSystemPtr::xdr_proc(xdrs, &elt->squeue))
    return FALSE;
  if (!WorkspaceBond::xdr_proc(xdrs, &elt->root))
    return FALSE;
  if (!xdr_bondstrength(xdrs, &elt->totalstr))
    return FALSE;
  if (!xdr_u_int(xdrs, &elt->bondcount))
    return FALSE;
  if (!Workspace::xdr_ptr(xdrs, &elt->uppref))
    return FALSE;
  if (!Workspace::xdr_ptr(xdrs, &elt->dnpref))
    return FALSE;
  if (!xdr_u_char(xdrs, &elt->uppull))
    return FALSE;
  if (!xdr_u_char(xdrs, &elt->dnpull))
    return FALSE;
  return TRUE;
}

/* :: WorkspaceRef Functions :: */

WorkspaceRef::WorkspaceRef(Workspace *lkup, CNIndex loc) :
  AIObject(CWorkspaceRef) {
  verbize(-5, "debug", "CREATING WORKSPACEREF %ld\n", this);
  lookup = lkup;
  location = loc;

  TrackLink::MemStore(trackid, &lookup, FALSE);
}

WorkspaceRef::WorkspaceRef(FILE *fp) :
  AIObject(fp) {
  fread(&lookup, sizeof(Workspace *), 1, fp);
  fread(&location, sizeof(CNIndex), 1, fp);

  TrackLink::MemStore(trackid, &lookup, FALSE);
}

WorkspaceRef::~WorkspaceRef() {
  verbize(-5, "debug", "DELETING WORKSPACEREF %ld\n", this);
}

WorkspaceElt &WorkspaceRef::GetElement() {
  verbize(-6, "debug", "Getting Element at %ld...\n", GetLocation());
  return *GetWorkspace()->GetElement(GetLocation());
}

WorkspaceElt &WorkspaceRef::GetElement(Workspace *puller) {
  GetWorkspace()->GetElement(GetLocation())->Pull(puller); /* pull toward that workspace */
  return *GetWorkspace()->GetElement(GetLocation());
}

WorkspaceElt *WorkspaceRef::StealElement() {
  verbize(-6, "debug", "Stealing Element...\n");
  return new WorkspaceElt(*(GetWorkspace()->GetElement(GetLocation()))); // give copy
}

WorkspaceElt *WorkspaceRef::StealElement(Workspace *puller) {
  GetWorkspace()->GetElement(GetLocation())->Pull(puller); /* pull toward that workspace */
  return new WorkspaceElt(*(GetWorkspace()->GetElement(GetLocation()))); // give copy
}

const WorkspaceElt &WorkspaceRef::BorrowElement() {
  return *GetWorkspace()->GetElement(GetLocation());
}

int WorkspaceRef::operator==(WorkspaceRef &right) {
  return (this == &right);
}

int WorkspaceRef::operator!=(WorkspaceRef &right) {
  return (this != &right);
}

Workspace *WorkspaceRef::GetWorkspace() {
  return lookup;
}

CNIndex WorkspaceRef::GetLocation() {
  return location;
}

int WorkspaceRef::WriteObject(FILE *fp) {
  size_t result = AIObject::WriteObject(fp);
  result = fwrite(&lookup, sizeof(Workspace *), 1, fp) + result;
  return fwrite(&location, sizeof(CNIndex), 1, fp) + result;
}

static bool_t WorkspaceRef::xdr_proc(XDR *xdrs, WorkspaceRef *ref) {
  if (!Workspace::xdr_ptr(xdrs, &ref->lookup))
    return FALSE;
  if (!xdr_cnindex(xdrs, &ref->location))
    return FALSE;
  return TRUE;
}

/* :: WorkspaceBond Functions :: */

WorkspaceBond::WorkspaceBond(WorkspaceRef &fromelt, WorkspaceRef &toelt, 
			     BondStrength str, BondType ntype) :
  AIObject(CWorkspaceBond), fromelement(fromelt), toelement(toelt) {
  verbize(-5, "debug", "Creating WorkspaceBond [%ld] (XXXX) {%f}...\n", this,
	  str);
  if (str > 1.)
    strength = 1.;
  else
    strength = str;
  type = ntype;
  next = NULL;
  UpdateTotalStr(strength);

  TrackLink::MemStore(trackid, &fromelement, CONST_FLAG);
  TrackLink::MemStore(trackid, &toelement, CONST_FLAG);
  TrackLink::MemStore(trackid, &next, FALSE);
}

WorkspaceBond::WorkspaceBond(WorkspaceBond &copy) :
  AIObject(CWorkspaceBond), fromelement(copy.fromelement),
  toelement(copy.toelement) {
  verbize(-5, "debug", "Creating WorkspaceBond [%ld] (%ld) {%f}...\n", this,
	  copy.next, copy.strength);
  strength = copy.strength;
  if (copy.next)
    next = new WorkspaceBond(*copy.next);
  else
    next = NULL;

  TrackLink::MemStore(trackid, &fromelement, CONST_FLAG);
  TrackLink::MemStore(trackid, &toelement, CONST_FLAG);
  TrackLink::MemStore(trackid, &next, FALSE);
}

WorkspaceBond::WorkspaceBond(FILE *fp, WorkspaceRef &from, WorkspaceRef &to) :
  AIObject(fp), fromelement(from), toelement(to) {
  fread(&strength, sizeof(BondStrength), 1, fp);
  fread(&type, sizeof(BondType), 1, fp);
  fread(&next, sizeof(WorkspaceBond *), 1, fp);

  TrackLink::MemStore(trackid, &fromelement, CONST_FLAG);
  TrackLink::MemStore(trackid, &toelement, CONST_FLAG);
  TrackLink::MemStore(trackid, &next, FALSE);
}  

WorkspaceBond::~WorkspaceBond() {
  verbize(-5, "debug", "Deleting WorkspaceBond [%ld] (%ld)...\n", this, next);
  if (next)
    delete next;
}

WorkspaceRef &WorkspaceBond::To() {
  return toelement;
}

void WorkspaceBond::SetNextBond(WorkspaceBond *bond) {
  next = bond;
}

WorkspaceBond *WorkspaceBond::GetNextBond() {
  return next;
}

BondStrength WorkspaceBond::GetStrength() {
  if (strength > 1.) {
    printf("Ahh!!!, str = %f\n", strength);
    exit(-2);
  }
  return strength;
}

void WorkspaceBond::SetStrength(BondStrength str) {
  if (str > 1.)
    str = 1.;
  UpdateTotalStr(str - strength);
  strength = str;
}

void WorkspaceBond::Strengthen(Confidence cred) {
  SetStrength(GetStrength() * (1.0 + STRMOD_FACTOR * cred));
}

void WorkspaceBond::Weaken(Confidence cred) {
  SetStrength(GetStrength() * (1.0 - STRMOD_FACTOR * cred));
}

void WorkspaceBond::UpdateTotalStr(BondStrength diff) {
  WorkspaceElt &elt(fromelement.GetElement());
  elt.totalstr += diff;
}

BondType WorkspaceBond::GetType() {
  return type;
}

void WorkspaceBond::SetType(BondType newtype) {
  type = newtype;
}

int WorkspaceBond::WriteObject(FILE *fp) {
  WorkspaceRef *wr1 = &fromelement;
  WorkspaceRef *wr2 = &toelement;

  size_t result = fwrite(&wr1, sizeof(WorkspaceRef *), 1, fp);
  result = fwrite(&wr1, sizeof(WorkspaceRef *), 1, fp) + result;
  result = AIObject::WriteObject(fp) + result;
  result = fwrite(&strength, sizeof(BondStrength), 1, fp) + result;
  result = fwrite(&type, sizeof(BondType), 1, fp) + result;
  return fwrite(&next, sizeof(WorkspaceBond *), 1, fp) + result;
}

static bool_t WorkspaceBond::xdr_proc(XDR *xdrs, WorkspaceBond *bond) {
  if (!xdr_reference(xdrs, &bond->fromelement, sizeof(WorkspaceRef),
		     WorkspaceRef::xdr_proc))
    return FALSE;
  if (!xdr_reference(xdrs, &bond->toelement, sizeof(WorkspaceRef),
		     WorkspaceRef::xdr_proc))
    return FALSE;
  if (!xdr_bondstrength(xdrs, &bond->strength))
    return FALSE;
  if (!xdr_enum(xdrs, (enum_t *) &bond->type))
    return FALSE;
  if (!xdr_pointer(xdrs, &bond->next, sizeof(WorkspaceBond),
		   WorkspaceBond::xdr_proc);
    return FALSE;
  return TRUE;
}
