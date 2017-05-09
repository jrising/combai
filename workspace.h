#ifndef WORKSPACE_H
#define WORKSPACE_H

/* All workspaces must keep track of up to N element references
   ranked vaguely by their salience and additional N element info
   records. */

typedef unsigned long CNIndex;
bool_t xdr_cnindex(XDR *xdrs, CNIndex *id);

typedef float BondStrength;
bool_t xdr_bondstrength(XDR *xdrs, BondStrength *id);

typedef unsigned char Value;
bool_t xdr_value(XDR *xdrs, Value *val);

#define SUFF_PULL 256

class Workspace;
class MemoryWorkspace;
class BigEndianWorkspace;
class LittleEndianWorkspace;
class WorkspaceBond;
class WorkspaceElt;
class WorkspaceRef;

#include "system.h"
#include "coderack.h"

#define nullwsref (Workspace *) NULL
#define STRMOD_FACTOR .1
#define MAX_MEMWSSIZE 65536

typedef enum {
  UndefBond, DataBond, EvolaiBond
} BondType;

/* High level, implementation-near-free workspace class */
class Workspace : public AIObject {
public:
  Workspace(CNIndex maxid, Workspace *higher, Workspace *lower,
	    unsigned prity, unsigned maxsz);
  Workspace(FILE *fp);
  ~Workspace();

  WorkspaceRef &AddElement(WorkspaceElt &elt);
  WorkspaceRef &AddElement(WorkspaceRef &ref); /* doesn't create new reference */

  /* serious move must change all references */
  virtual WorkspaceRef &DataShift(CNIndex id, Workspace *newws,
				   WorkspaceElt &repl) = NULL;
  virtual WorkspaceRef &DataShift(CNIndex id, Workspace *newws,
				   WorkspaceRef &repl) = NULL;
  virtual WorkspaceRef &DataSwitch(WorkspaceRef &here, WorkspaceRef &repl)
    = NULL;

  virtual WorkspaceElt *GetElement(CNIndex id) = NULL;
  virtual void SetElement(CNIndex id, const WorkspaceElt &elt) = NULL;

  virtual WorkspaceRef &SalientElement(CNIndex i) = NULL;
  void SalientSwitch(CNIndex id, WorkspaceRef &ref, CNIndex oldid,
		     Workspace *oldws);
  /* Move salient item from to my index id (and
     move my item to where it is) */

  Workspace *GetHigherWorkspace();
  Workspace *GetLowerWorkspace();
  void SetHigherWorkspace(Workspace *higher);
  void SetLowerWorkspace(Workspace *lower);

  CNIndex GetCurrentIndex();
  CNIndex GetMaxIndex();
  unsigned GetPriority();

  Coderack &GetCoderack();

  virtual int WriteObject(FILE *fp);

  static bool_t xdr_proc(XDR *xdrs, Workspace *ws);

protected:
  CNIndex currindex;

private:
  virtual WorkspaceRef &RoomAddElement(WorkspaceElt &elt) = NULL;
  virtual WorkspaceRef &RoomAddElement(WorkspaceRef &ref) = NULL;
  virtual void SetSalient(CNIndex i, WorkspaceRef &ref) = NULL;

  Workspace *higherws;
  Workspace *lowerws;
  CNIndex maxindex;
  unsigned priority;

  Coderack *theCoderack;
};

/* Workspace information stored in RAM */
class MemoryWorkspace : public Workspace {
public:
  MemoryWorkspace(CNIndex maxid, Workspace *higher, Workspace *lower,
		  unsigned prity, unsigned maxsz);
  MemoryWorkspace(FILE *fp);
  ~MemoryWorkspace();

  virtual WorkspaceRef &DataShift(CNIndex id, Workspace *newws,
				  WorkspaceElt &repl);
  virtual WorkspaceRef &DataShift(CNIndex id, Workspace *newws,
				  WorkspaceRef &repl);
  virtual WorkspaceRef &DataSwitch(WorkspaceRef &here, WorkspaceRef &repl);

  virtual WorkspaceElt *GetElement(CNIndex id);
  virtual void SetElement(CNIndex id, const WorkspaceElt &elt);

  virtual WorkspaceRef &SalientElement(CNIndex i);

  virtual int WriteObject(FILE *fp);

protected:
  virtual WorkspaceRef &RoomAddElement(WorkspaceElt &elt);
  virtual WorkspaceRef &RoomAddElement(WorkspaceRef &ref);
  virtual void SetSalient(CNIndex i, WorkspaceRef &ref);

  WorkspaceElt **data;
  WorkspaceRef **refs;
};

/* Workspace information stored in big endian file */
/*class BigEndianWorkspace : public Workspace {
  };*/

/* Workspace information stored in little endian file */
/*class LittleEndianWorkspace : public Workspace {
  };*/

class WorkspaceBond : public AIObject {
public:
  WorkspaceBond(WorkspaceRef &fromelt, WorkspaceRef &toelt,
		BondStrength str, BondType type);
  WorkspaceBond(WorkspaceBond &copy);
  WorkspaceBond(FILE *fp, WorkspaceRef &from, WorkspaceRef &to);
  ~WorkspaceBond();

  WorkspaceRef &To();

  void SetNextBond(WorkspaceBond *bond);
  WorkspaceBond *GetNextBond();

  BondStrength GetStrength();
  void SetStrength(BondStrength str);
  void Strengthen(Confidence cred);
  void Weaken(Confidence cred);

  BondType GetType();
  void SetType(BondType newtype);

  virtual int WriteObject(FILE *fp);

  static bool_t xdr_proc(XDR *xdrs, WorkspaceBond *bond);

private:
  void UpdateTotalStr(BondStrength diff);

  WorkspaceRef &fromelement;
  WorkspaceRef &toelement;
  BondStrength strength;
  BondType type;
  WorkspaceBond *next;
};

/* The data for a single element, needed for modifying the element */
class WorkspaceElt : public AIObject {
  friend void WorkspaceBond::UpdateTotalStr(BondStrength diff);
public:
  WorkspaceElt(WorkspaceRef &ref);
  WorkspaceElt(const WorkspaceElt &copy);  // data
  WorkspaceElt(const WorkspaceElt &copy, WorkspaceRef &ref);  // data, ref.
  WorkspaceElt(const WorkspaceElt &copy, WorkspaceRef &ref,
	       WorkspaceRef &loc);  // data, ref., salient loc
  WorkspaceElt(Value val);
  WorkspaceElt(FILE *fp, WorkspaceRef &refloc, WorkspaceRef &salloc);
  ~WorkspaceElt(); /* free bond memeory */

  Value GetValue() const;
  WorkspaceRef &GetSalientLoc() const;

  void AddBond(WorkspaceRef &toelt, BondStrength str, BondType type);
  void AddBond(WorkspaceBond *bond);  // -bond
  WorkspaceBond *SelectRandomBond() const;
  WorkspaceBond *SelectWeightBond() const;
  BondStrength GetTotalStr() const;
  unsigned GetBondCount() const;

  void Pull(Workspace *puller);

  int AddQueue(EvolSystemPtr newsys);
  EvolSystemPtr RemoveQueue();
  unsigned QueueSystemCount();
  urgetype QueueUrgency() const;

  void Commit() const;

  virtual int WriteObject(FILE *fp);

  static bool_t xdr_proc(XDR *xdrs, WorkspaceElt *elt);

protected:
  WorkspaceRef &reference;
  WorkspaceRef &salientloc;

  int allocated;

  Value value;

  EvolSystemPtr squeue;

  WorkspaceBond *root;
  BondStrength totalstr;
  unsigned bondcount;

  Workspace *uppref;
  Workspace *dnpref;
  unsigned char uppull;  /* how much pulled toward prefered up workspace */
  unsigned char dnpull;  /* how much pulled toward prefered down workspace */
};

/* A reference to the data of an element-- should only store these */
class WorkspaceRef : public AIObject {
  friend WorkspaceRef &MemoryWorkspace::RoomAddElement(WorkspaceRef &ref);
  friend WorkspaceRef &MemoryWorkspace::RoomAddElement(WorkspaceElt &elt);
  friend WorkspaceRef &MemoryWorkspace::DataShift(CNIndex id,
						   Workspace *newws,
						   WorkspaceRef &repl);
  friend WorkspaceRef &MemoryWorkspace::DataSwitch(WorkspaceRef &here,
						   WorkspaceRef &repl);
public:
  WorkspaceRef(Workspace *lkup, CNIndex loc);
  WorkspaceRef(FILE *fp);
  ~WorkspaceRef();

  WorkspaceElt &GetElement();  
  WorkspaceElt &GetElement(Workspace *puller);
  WorkspaceElt *StealElement(); // +
  WorkspaceElt *StealElement(Workspace *puller); // +
  const WorkspaceElt &BorrowElement();

  int operator==(WorkspaceRef &right);
  int operator!=(WorkspaceRef &right);

  virtual Workspace *GetWorkspace();
  virtual CNIndex GetLocation();

  virtual int WriteObject(FILE *fp);

  static bool_t xdr_proc(XDR *xdrs, WorkspaceRef *ref);

private:
  Workspace *lookup;
  CNIndex location;
};

#endif
