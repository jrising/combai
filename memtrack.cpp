#include "base.h"
#include "memtrack.h"
#include <exception>

TrackLink *TrackLink::table[TABLE_BINS];
TrackLink *TrackLink::root = NULL;

void TrackLink::MemInitialize() {
  for (int i = 0; i < TABLE_BINS; i++)
    table[i] = NULL;
  root = new TrackLink(NULL);
}

void TrackLink::MemDestroy() {
  TrackLink *curr, *prev;

  for (unsigned bin = 0; bin < TABLE_BINS; bin++) {
    curr = table[bin];
    while (curr) {
      prev = curr;
      curr = curr->next;
      delete prev;
    }
  }
  delete root;
}

TrackLink *TrackLink::MemRegister(void *ptr, unsigned long flags) {
  if (!ptr)
    return NULL;

  try {
    unsigned bin = HashPointer(ptr);
    TrackLink *newlink = new TrackLink(ptr);
    newlink->flag = flags;

    //printf("Registering %ld\n", ptr);

    // install into table
    newlink->next = table[bin];
    if (table[bin])
      table[bin]->prev = newlink;
    table[bin] = newlink;

    return newlink;
  } catch (std::exception &e) {
    fprintf(stderr, "Error in memory tracking system: MemRegister: %s\n", e.what());
  }
}

void TrackLink::MemForget(TrackLink *link) {
  if (!link)
    return;

  try {
    unsigned bin = HashPointer(link->ptr);

    //printf("Forgetting %ld\n", link->ptr);

    if (link->prev)
      link->prev->next = link->next;
    else
      table[bin] = link->next;
    if (link->next)
      link->next->prev = link->prev;
    delete link;
  } catch (std::exception &e) {
    fprintf(stderr, "Error in memory tracking system: MemForget: %s\n", e.what());
  }
}

void TrackLink::MemStore(TrackLink *container, void *contained, char flag) {
  if (!container || !contained)
    return;

  try {
    PointerLink *newlink = new PointerLink(contained, flag);

    /*if (flag & CONST_FLAG)
      printf("Stored %ld (const) into %ld\n", contained, container->ptr);
    else
      printf("Stored %ld = %ld into %ld\n", contained, *((void **) contained),
      container->ptr);*/

    // install into list
    newlink->next = container->list;
    container->list = newlink;
  } catch (std::exception &e) {
    fprintf(stderr, "Error in memory tracking system: MemStore: %s\n", e.what());
  }
}  

void TrackLink::MemRemove(TrackLink *container, void *contained) {
  if (!container || !contained)
    return;

  try {
    // Find it
    PointerLink *last = NULL, *curr = container->list;
    while (curr) {
      if (curr->ptr == contained) {
	if (last == NULL)
	  container->list = curr->next;
	else
	  last->next = curr->next;
	break;
      }
      last = curr;
      curr = curr->next;
    }

    if (curr)
      delete curr;
    else
      throw "Contained not found";
  } catch (std::exception &e) {
    fprintf(stderr, "Error in memory tracking system: MemForget: %s\n", e.what());
  } catch (char *expn) {
    fprintf(stderr, "Error in memory tracking system: MemForget: %s\n", expn);
  }
}

void *TrackLink::MemMarkCheck() {
  try {
    // Reset all markings
    for (unsigned bin = 0; bin < TABLE_BINS; bin++) {
      for (TrackLink *curr = table[bin]; curr; curr = curr->next) {
	//printf("Clearing %ld\n", curr->ptr);
	curr->flag &= ~((unsigned long) MARKED_FLAG);
      }
    }

    root->flag &= ~((unsigned long) MARKED_FLAG);
    MemMarkReachableFrom(root);

    // Find any memory that isn't reachable
    for (unsigned bin = 0; bin < TABLE_BINS; bin++) {
      for (TrackLink *curr = table[bin]; curr; curr = curr->next)
	if (!(curr->flag & MARKED_FLAG))
	  return curr->ptr;
    }

    return NULL;
  } catch (std::exception &e) {
    fprintf(stderr, "Error in memory tracking system: MemMarkCheck: %s\n", e.what());
  }
}

void TrackLink::WriteAllObjects(FILE *fp, void *adnl, unsigned size) {
  TrackLink *curr;
  PointerLink *todos = NULL;
  PointerLink *dones = NULL;
  int alreadydone, addedtotodo;
  void *ptrtonull = NULL;

  PointerLink plroot(NULL, FALSE);

  //printf("Start WAO\n");

  // Output only those which can already be reconstructed from previous
  for (unsigned bin = 0; bin < TABLE_BINS; bin++) {
    for (curr = table[bin]; curr; curr = curr->next) {
      // Check if this can be reconstructed
      addedtotodo = FALSE;
      for (PointerLink *link = curr->list; link; link = link->next) {
	if (link->flag & CONST_FLAG) {
	  alreadydone = FALSE;
	  for (PointerLink *done = dones; done; done = done->next) {
	    if (((TrackLink *) done->ptr)->ptr == link->ptr) {  // this one has been done
	      alreadydone = TRUE;
	      break;
	    }
	  }
	  if (!alreadydone) {  // nope: add to todo list
	    PointerLink *newlink = new PointerLink(curr, FALSE);
	    newlink->next = todos;
	    todos = newlink;
	    addedtotodo = TRUE;
	    break;
	  }
	}
      }
      if (!addedtotodo) {
	fwrite(&(curr->ptr), sizeof(void *), 1, fp);
	if (curr->flag & AIOBJ_FLAG) {
	  fwrite(&(((AIObject *) curr->ptr)->type), sizeof(classtype), 1, fp);
	  ((AIObject *) curr->ptr)->WriteObject(fp);
	} else {
	  classtype NoClassType = CInvalidClass;
	  unsigned long size = (curr->flag) >> TL_FLAG_BITS;
	  fwrite(&NoClassType, sizeof(classtype), 1, fp);
	  curr->flag &= ~((unsigned long) MARKED_FLAG);
	  fwrite(&size, sizeof(unsigned long), 1, fp);
	  fwrite(curr->ptr, sizeof(char), size, fp);
	}

	PointerLink *newlink = new PointerLink(curr, FALSE);
	newlink->next = dones;
	dones = newlink;
      }
    }
  }

  //printf("Middle WAO\n");

  // Now output all others, cycling until all done
  while (todos) {
    PointerLink *prevlink = &plroot;
    plroot.next = todos;

    for (PointerLink *currlink = todos; currlink; currlink = currlink->next) {
      curr = (TrackLink *) currlink->ptr;

      // Check if this can be reconstructed
      addedtotodo = FALSE;
      for (PointerLink *link = curr->list; link; link = link->next) {
	if (link->flag & CONST_FLAG) {
	  alreadydone = FALSE;
	  for (PointerLink *done = dones; done; done = done->next) {
	    if (((TrackLink *) done->ptr)->ptr == link->ptr) {  // this one has been done
	      alreadydone = TRUE;
	      break;
	    }
	  }
	  if (!alreadydone) {  // nope
	    addedtotodo = TRUE;
	    break;
	  }
	}
      }
      if (!addedtotodo) {
	fwrite(&(curr->ptr), sizeof(void *), 1, fp);
	if (curr->flag & AIOBJ_FLAG) {
	  fwrite(&(((AIObject *) curr->ptr)->type), sizeof(classtype), 1, fp);
	  ((AIObject *) curr->ptr)->WriteObject(fp);
	} else {
	  classtype NoClassType = CInvalidClass;
	  unsigned long size = (curr->flag) >> TL_FLAG_BITS;
	  fwrite(&NoClassType, sizeof(classtype), 1, fp);
	  curr->flag &= ~((unsigned long) MARKED_FLAG);
	  fwrite(&size, sizeof(unsigned long), 1, fp);
	  fwrite(curr->ptr, sizeof(char), size, fp);
	}

	// Move from the todos to the dones
	prevlink->next = currlink->next;
	currlink->next = dones;
	dones = currlink;

	currlink = prevlink;
      } else
	prevlink = currlink;
    }

    todos = plroot.next;
  }

  //printf("Middle WAO\n");

  // Now delete all those extra pointers
  PointerLink *currlink = dones;
  PointerLink *prevlink;
  while (currlink) {
    prevlink = currlink;
    currlink = currlink->next;
    delete prevlink;
  }

  //printf("Final WAO\n");

  // Print out root pointers so that can be reconstructed
  for (PointerLink *link = root->list; link; link = link->next) {
    if ((link->ptr) == NULL)
      continue;
    fwrite(&(link->ptr), sizeof(TrackLink *), 1, fp);
    fwrite(&(link->flag), sizeof(char), 1, fp);
  }
  fwrite(&ptrtonull, sizeof(void *), 1, fp);

  // Also write out requested additional pointers
  fwrite(adnl, size, 1, fp);
}

void TrackLink::FixPointers(PointerMapLink *root) {
  for (unsigned bin = 0; bin < TABLE_BINS; bin++)
    for (TrackLink *curr = table[bin]; curr; curr = curr->next)
      for (PointerLink *link = curr->list; link; link->next)
	if (!(link->flag & CONST_FLAG))
	  *((void **) link->ptr) =
	    root->FindNewPointer(*((void **) link->ptr));
}

void TrackLink::MemMarkReachableFrom(TrackLink *track) {
  if (!track || (track->flag & MARKED_FLAG))
    return;

  track->flag |= MARKED_FLAG;

  // Mark all reachable by track
  for (PointerLink *curr = track->list; curr; curr = curr->next) {
    if (curr->flag & CONST_FLAG)
      MemMarkReachableFrom(MemFindLink(curr->ptr));
    else
      MemMarkReachableFrom(MemFindLink(*((void **) curr->ptr)));
  }
}

TrackLink *TrackLink::MemFindLink(void *ptr) {
  if (!ptr)
    return NULL;

  try {
    unsigned bin = HashPointer(ptr);
    for (TrackLink *curr = table[bin]; curr; curr = curr->next)
      if (curr->ptr == ptr)
	return curr;
    throw "Pointer not found";
  } catch (std::exception &e) {
    fprintf(stderr, "Error in memory tracking system: MemFindLink: %s\n", e.what());
  } catch (char *expn) {
    fprintf(stderr, "Error in memory tracking system: MemFindLink: %s\n", expn);
  }
}

TrackLink::TrackLink(void *ptrarg) {
  ptr = ptrarg;
  next = NULL;
  prev = NULL;
  flag = 0;
  list = NULL;
}

TrackLink::~TrackLink() {
  PointerLink *last = NULL;
  PointerLink *curr = list;
  while (curr) {
    last = curr;
    curr = curr->next;
    delete last;
  }
}

PointerLink::PointerLink(void *contained, char flagarg) {
  flag = flagarg;
  ptr = contained;
  next = NULL;
}

unsigned HashPointer(void *ptr) {
  return ((unsigned long) ptr / 3) % TABLE_BINS;
}
