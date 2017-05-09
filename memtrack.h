#ifndef MEMTRACK_H
#define MEMTRACK_H

class TrackLink;
class PointerLink;
class PointerMapLink;

#define TABLE_BINS 1024

// flags is the number of bytes allocated for a non-AI-object, shift right by 2
// plus the other flags
#define TL_FLAG_BITS 2
#define MARKED_FLAG 0x01
#define AIOBJ_FLAG 0x02

class TrackLink {
public:

  static void MemInitialize();
  static void MemDestroy();

  static TrackLink *MemRegister(void *ptr, unsigned long flags);
  static void MemForget(TrackLink *link);

  // NULL container => install into ROOT
  static void MemStore(TrackLink *container, void *contained, char flag);
  static void MemRemove(TrackLink *container, void *contained);

  static void *MemMarkCheck();

  static TrackLink *MemFindLink(void *ptr);

  static void WriteAllObjects(FILE *fp, void *adnl, unsigned size);
  static void FixPointers(PointerMapLink *root);

  static TrackLink *root;

private:
  TrackLink(void *ptr);
  ~TrackLink();

  static void MemMarkReachableFrom(TrackLink *track);

  static TrackLink *table[TABLE_BINS];

  void *ptr;
  TrackLink *next;
  TrackLink *prev;
  unsigned long flag;
  PointerLink *list;
};

#define CONST_FLAG 0x01  // if so, not pointer to pointer, just actual pointer

class PointerLink {
public:
  PointerLink(void *contained, char flag);

  char flag;
  void *ptr;
  struct PointerLink *next;
};

class PointerMapLink {
public:
  PointerMapLink(void *oldp, void *newp, PointerMapLink *nxt) {
    oldptr = oldp;
    newptr = newp;
    next = nxt;
  }

  void *FindNewPointer(void *old) {
    if (old == oldptr)
      return newptr;
    if (!oldptr)
      return NULL;
    if (!next)
      printf("Error!  Pointer %ld not found!\n");
    return next->FindNewPointer(old);
  }

  void *oldptr;
  void *newptr;
  PointerMapLink *next;
};

unsigned HashPointer(void *ptr);

#endif
