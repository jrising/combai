#ifndef BASE_H
#define BASE_H

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <rpc/xdr.h>
#include "memtrack.h"

#define UNSPEC_ERROR -1
#define FOPEN_ERROR -2
#define BADARG_ERROR -3
#define MEMORY_ERROR -4
#define ASSERT_ERROR -5

typedef enum {CInvalidClass, CDeletedClass, CUninitializedClass,
	      CWorkspace, CMemoryWorkspace, CBigEndianWorkspace,
	      CLittleEndianWorkspace, CEvolSystem, CEvolSystemBasic,
	      CEvolSystemCombo, CCoderack, CCoderackNode, CCoderackBranch,
	      CCoderackRoot, CCoderackLeaf, CCodelet, CReadKeyboardCodelet,
	      CQueueCodelet, CEvolaiCodelet, CMoveSystemCodelet,
	      CJumpSystemCodelet, CRepeatedCodelet, CCheckWorkspace,
	      CCheckCoderack, CWorkspaceRef, CWorkspaceElt, CWorkspaceBond,
	      CCheckMemory, CTextShowWorkspace, CTextShowElement}
classtype;

/* AIObject class from which everything inherits */
class AIObject {
public:
  AIObject(classtype t) {
    type = t;
    trackid = TrackLink::MemRegister(this, AIOBJ_FLAG);
  }

  AIObject(FILE *fp) {
    fread(&type, sizeof(classtype), 1, fp);
    trackid = TrackLink::MemRegister(this, AIOBJ_FLAG);
  }

  virtual ~AIObject() {
    type = CDeletedClass;
    TrackLink::MemForget(trackid);
  }

  virtual int WriteObject(FILE *fp) {
    return fwrite(&type, sizeof(classtype), 1, fp);
  }

  static bool_t xdr_ptr(XDR *xdrs, AIObject **ptr) {
    if (!xdr_int(xdrs, (int *) ptr))
      return FALSE;
    return TRUE;
  }

  static bool_t xdr_proc(XDR *xdrs, AIObject *obj) {
    if (!xdr_enum(xdrs, (enum_t *) &obj->type))
      return FALSE;
    if (!xdr_int(xdrs, (int *) &obj->trackid))
      return FALSE;
    return TRUE;
  }

  classtype type;
  TrackLink *trackid;
};

/* Codelet Handling */
#include "coderack.h"

#define UrgeRange 1.0

/* Useful functions */
#define random(rng) ((int) (((float) rand()) * ((float) rng) / ((float) RAND_MAX)))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define sqr(a) ((a)*(a))
#define frand() (((double) rand()) / (double) RAND_MAX)
#define irand(n) (rand() % n)

#define ASSERT_VI 5

FILE *aifopen(const char *filename, const char *mode, const char *purpose,
	      int fatal, int vi);
void *aialloc(size_t size, char *purpose, int fatal, int vi);
void *airealloc(void *ptr, size_t size, char *purpose, int fatal, int vi);
void aiassert(int abool, char *purpose);
void aifree(void *ptr);
int ProbToBool(float prob);

struct verblist {
  char topic[32];
  int level;
  struct verblist *next;
};

static struct verblist verbosity = {"", 0, NULL};

void verbize(int verbity, char *topic, char *format, ...);
int change_verbosity(char *topic, int change);

#define TRUE 1
#define FALSE 0

#endif
