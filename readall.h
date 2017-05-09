#ifndef READALL_H
#define READALL_H

#include <stdio.h>
#include "workspace.h"

struct BasePointers {
  MemoryWorkspace *basews;
};

struct BasePointers ReadAllObjects(FILE *fp);

#endif
