#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <exception>

/* Usage: artintel [-h] [-v] [-q] [-i <file>] [-o <file>]
   [-V topic] [-Q topic] [-d <datafile>]*/

extern int errno;

#include "base.h"
#include "codelets.h"
#include "workspace.h"
#include "checker.h"
#include "textshow.h"
#include "readall.h"

#define VERBIZE_FILE "/tmp/output.txt"
#define VERBIZE_BUFF 4194304
#define VERBIZE_FILE_X "/tmp/output2.txt"
#define OUTPUT_COUNT 100

float effectiveness = 0;
unsigned long predtotal = 0;
int whichfile = 0;        // only used if VERBIZE_BUFF and VERBIZE_FILE_X

int main(int argc, char *argv[]) {
  int c;
  extern char *optarg;
  extern int optind;
  char initd = FALSE;

  unsigned long ocount = OUTPUT_COUNT;
  MemoryWorkspace *baseWorkspace;

  verbize(1, "", "Initializing...\n");

  TrackLink::MemInitialize();

  while ((c = getopt(argc, argv, "hvqV:Q:d:")) != EOF)
    switch (c) {
    case 'v':
      verbize(-2, "", "Verbosity increased to %d.\n",
	      change_verbosity("", 1));
      break;
    case 'q':
      verbize(-2, "", "Verbosity decreased to %d.\n",
	      change_verbosity("", -1));
      break;
    case 'V':
      verbize(-2, "", "Verbosity increased to %d.\n",
	      change_verbosity(optarg, 1));
      break;
    case 'Q':
      verbize(-2, "", "Verbosity decreased to %d.\n",
	      change_verbosity(optarg, -1));
      break;
    case 'd': {
      FILE *fp = fopen(optarg, "rb");
      struct BasePointers adnl;

      adnl = ReadAllObjects(fp);
      fclose(fp);
      baseWorkspace = adnl.basews;
      initd = TRUE;
    }
    case '?':
      verbize(3, "",
	      "Usage: %s [-q] [-v] [-h] [-i <file>] [-o <file>]\n",
	      argv[0]);
      exit(BADARG_ERROR);
    case 'h':
      verbize(3, "",
	      "Usage: %s [-q] [-v] [-h] [-i <file>] [-o <file>]\n",
	      argv[0]);
      break;
    }

  if (!initd) {
    baseWorkspace = new MemoryWorkspace(65536, nullwsref, nullwsref, 1, 65536);
    TrackLink::MemStore(TrackLink::root, &baseWorkspace, FALSE);
    baseWorkspace->GetCoderack().
      AddCodelet(new ReadKeyboardCodelet(NULL, baseWorkspace, UrgeRange / 4));
    baseWorkspace->GetCoderack().
      AddCodelet(new CheckWorkspace(*baseWorkspace));
    baseWorkspace->GetCoderack().
      AddCodelet(new TextShowWorkspace(*baseWorkspace));
    baseWorkspace->GetCoderack().
      AddCodelet(new CheckCoderack(baseWorkspace->GetCoderack()));
    baseWorkspace->GetCoderack().
      AddCodelet(new CheckMemory(baseWorkspace->GetCoderack()));
    baseWorkspace->GetCoderack().
      AddCodelet(new TypeDocumentCodelet("input.txt",
					 &baseWorkspace->GetCoderack()));
    initd = TRUE;
  }

  verbize(1, "", "Processing...\n");

  while (1) {
    try {
      baseWorkspace->GetCoderack().ExecuteCodelet();
    } catch (std::exception &e) {
      verbize(2, "error", "Exception: %s", e.what());
    }
    if (!(--ocount)) {
      // Status Line
      verbize(1, "status", "Workspace: %ld; Coderack: %ld (%f) => (%f for %ld) :: %f\n",
	      baseWorkspace->GetCurrentIndex(),
	      baseWorkspace->GetCoderack().getSize(),
	      baseWorkspace->GetCoderack().getTotalUrgency(),
	      effectiveness / (double) predtotal, predtotal, EvolSystem::GetAverageAge());
      ocount = OUTPUT_COUNT;
      // Output Test
      struct BasePointers adnl;
      adnl.basews = baseWorkspace;

      FILE *fp = fopen("allobjs.dat", "w");
      TrackLink::WriteAllObjects(fp, &adnl, sizeof(struct BasePointers));
    }
  }

  struct verblist *last = &verbosity;
  struct verblist *curr = verbosity.next;
  while (curr) {
    last = curr;
    curr = curr->next;
    aifree(last);
  }
    
  TrackLink::MemDestroy();
}

/* Well behaved fopen */
FILE *aifopen(const char *filename, const char *mode, const char *purpose,
	      int fatal, int vi) {
  FILE *fp;

  if (!strcmp(filename, "-")) {
    if (strchr(mode, 'r')) {
      fp = stdin;
      verbize(-1 + vi, "debug", "Using stdin for %s\n", purpose);
    }
    else if (strchr(mode, 'w')) {
      fp = stdout;
      verbize(-1 + vi, "debug", "Using stdout for %s\n", purpose);
    }
    else {
      verbize(2 + vi + fatal, "debug",
	      "Invalid mode (%s) openning standard stream for %s",
	      mode, purpose);
      if (fatal)
	exit(FOPEN_ERROR);
      fp = NULL;
    }
  } else {
    fp = fopen(filename, mode);
    if (!fp) {
      verbize(2 + vi + fatal, "debug", "Error opening %s for %s: %s",
	      optarg, purpose, strerror(errno));
      if (fatal)
	exit(FOPEN_ERROR);
    } else
      verbize(-2 + vi, "debug", "Opened %s for %s\n", optarg, purpose);
  }

  return fp;
}

void verbize(int verbity, char *topic, char *format, ...) {
  va_list ap;
  int verbtot = verbosity.level;
  struct verblist *curr;
  FILE *fp;
#if VERBIZE_BUFF
  char buff[VERBIZE_BUFF];
  int c, i = 0;
#endif

  for (curr = verbosity.next; curr; curr = curr->next)
    if (!strcasecmp(curr->topic, topic))
      verbtot += curr->level;

  if (verbity + verbtot >= 0) {
    va_start(ap, format);
    if (VERBIZE_FILE) {
      if (!whichfile)
	fp = fopen(VERBIZE_FILE, "a+");
      else
	fp = fopen(VERBIZE_FILE_X, "a+");
      fseek(fp, 0, SEEK_END);
      vfprintf(fp, format, ap);
      if (VERBIZE_BUFF && !VERBIZE_FILE_X) {
	if (ftell(fp) > VERBIZE_BUFF) {
	  fseek(fp, -VERBIZE_BUFF, SEEK_END);
	  while ((c = fgetc(fp)) != EOF)
	    buff[i++] = c;
	  fp = freopen(VERBIZE_FILE, "w", fp);
	  for (i = 0; i < VERBIZE_BUFF; i++)
	    fputc(buff[i], fp);
	}
      } else if (VERBIZE_BUFF && VERBIZE_FILE_X) {
	if (ftell(fp) > VERBIZE_BUFF) {
	  whichfile = !whichfile;
	  if (!whichfile)
	    fp = freopen(VERBIZE_FILE, "w", fp);
	  else
	    fp = freopen(VERBIZE_FILE_X, "w", fp);
	}
      }
      fclose(fp);
    } else
      vprintf(format, ap);
    va_end(ap);
  }
}

void *aialloc(size_t size, char *purpose, int fatal, int vi) {
  void *ptr = malloc(size);

  if (!ptr) {
    verbize(vi + fatal, "debug", "Failed to allocate %d bytes for %s: %s\n",
	    size, purpose, strerror(errno));
    if (fatal)
      exit(MEMORY_ERROR);
  } else
    verbize(-2 + vi, "debug", "Allocated %d bytes for %s\n", size, purpose);

  TrackLink::MemRegister(ptr, size << TL_FLAG_BITS);

  return ptr;
}

void *airealloc(void *ptr, size_t size, char *purpose, int fatal, int vi) {
  void *oldptr = ptr;
  ptr = realloc(ptr, size);

  if (!ptr) {
    verbize(vi + fatal, "debug", "Failed to allocate %d bytes for %s: %s\n",
	    size, purpose, strerror(errno));
    if (fatal)
      exit(MEMORY_ERROR);
  } else
    verbize(-2 + vi, "debug", "Allocated %d bytes for %s\n", size, purpose);

  TrackLink::MemForget(TrackLink::MemFindLink(oldptr));
  TrackLink::MemRegister(ptr, size << TL_FLAG_BITS);

  return ptr;
}

void aifree(void *ptr) {
  TrackLink::MemForget(TrackLink::MemFindLink(ptr));
  free(ptr);
}

void aiassert(int abool, char *purpose) {
  if (!abool) {
    verbize(ASSERT_VI, "debug", "Assertion for %s failed!\n", purpose);
    exit(ASSERT_ERROR);
  }
}

int ProbToBool(float prob) {
  if (prob > .5)
    return (frand() > exp(10. * (.5 - prob)));
  else
    return (frand() < exp(10. * (prob - .5)));
}

int change_verbosity(char *topic, int change) {
  struct verblist *curr;

  for (curr = &verbosity; curr; curr = curr->next)
    if (!strcasecmp(topic, curr->topic)) {
      curr->level += change;
      return curr->level;
    }

  curr = verbosity.next;
  verbosity.next = (struct verblist *) aialloc(sizeof(struct verblist),
					       "Creating verbosity element",
					       0, -1);
  if (verbosity.next) {
    strcpy(verbosity.next->topic, topic);
    verbosity.next->level = change;
    verbosity.next->next = curr;
  } else
    verbosity.next = curr;

  return change;
}
