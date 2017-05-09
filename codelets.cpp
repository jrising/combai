#include <stdio.h>
#include <termios.h>
#include <time.h>
#include <string.h>

#include "codelets.h"
#include "system.h"
#include "evolet.h"

int recentchar = 0;
int beendone = 0;

/*****************************************************************************/

Codelet::Codelet(urgetype urge) :
  AIObject(CCodelet) {
  urgency = urge;
  flags = 0;
}

Codelet::Codelet(FILE *fp) :
  AIObject(fp) {
  fread(&urgency, sizeof(urgetype), 1, fp);
  fread(&flags, sizeof(int), 1, fp);
}

Codelet::Codelet() :
  AIObject(CCodelet) {
  urgency = 0.;
  flags = 0;
}

urgetype Codelet::getUrgency() {
  return urgency;
}

urgetype Codelet::getComplacency() {
  return UrgeRange - urgency;
}

int Codelet::getFlags() {
  return flags;
}

int Codelet::WriteObject(FILE *fp) {
  size_t result = AIObject::WriteObject(fp);
  result = fwrite(&urgency, sizeof(urgetype), 1, fp) + result;
  return fwrite(&flags, sizeof(int), 1, fp) + result;
}

/*****************************************************************************/

clock_t ReadKeyboardCodelet::reference_tdiff = CLOCKS_PER_SEC;  /* maximum */

ReadKeyboardCodelet::ReadKeyboardCodelet(WorkspaceRef *last, Workspace *ws,
					   urgetype urge) :
  Codelet(urge) {
  type = CReadKeyboardCodelet;
  lastelt = last;
  workspace = ws;
  initial = clock();
  lasturge = urge;

  flags |= PRIV_FLAG;

  TrackLink::MemStore(trackid, &lastelt, FALSE);
  TrackLink::MemStore(trackid, &workspace, FALSE);
}

ReadKeyboardCodelet::ReadKeyboardCodelet(WorkspaceRef *last, Workspace *ws,
					   urgetype urge, time_t startt) :
  Codelet(urge) {
  type = CReadKeyboardCodelet;
  lastelt = last;
  workspace = ws;
  initial = startt;
  lasturge = urge;

  flags |= PRIV_FLAG;

  TrackLink::MemStore(trackid, &lastelt, FALSE);
  TrackLink::MemStore(trackid, &workspace, FALSE);
}

ReadKeyboardCodelet::ReadKeyboardCodelet(FILE *fp) :
  Codelet(fp) {
  fread(&reference_tdiff, sizeof(clock_t), 1, fp);
  fread(&lastelt, sizeof(WorkspaceRef *), 1, fp);
  fread(&workspace, sizeof(Workspace *), 1, fp);
  fread(&initial, sizeof(clock_t), 1, fp);
  fread(&lasturge, sizeof(urgetype), 1, fp);

  TrackLink::MemStore(trackid, &lastelt, FALSE);
  TrackLink::MemStore(trackid, &workspace, FALSE);
}

void ReadKeyboardCodelet::Execute() {
  verbize(-5, "debug", "Executing ReadKeybaord Codelet.\n");

  if (recentchar || kbhit()) {
    int input = (recentchar ? recentchar : getch());
    clock_t tdiff = clock() - initial;
    float strength = lastelt ?
      (.5 - (abs(input - lastelt->BorrowElement().GetValue()) / (4. * 255.))) :
      .5;
    float bondstr = strength * ((float) reference_tdiff) / (float) tdiff;

    if (recentchar) {
      recentchar = 0;
      bondstr = strength;
    }

    WorkspaceElt newelt(input); /* create an element */
    WorkspaceRef &newref(workspace->AddElement(newelt)); /* add to workspace */
    WorkspaceElt &newelt2(newref.GetElement()); // get element as added
    if (lastelt) {
      if (bondstr >= EXP_BOND_MAX) {
	if (bondstr > 1.)
	  bondstr = 1.;
	reference_tdiff /= 2;
      } else if (bondstr <= 1. - EXP_BOND_MAX)
	reference_tdiff *= 2;

      lastelt->GetElement().
	AddBond(new WorkspaceBond(*lastelt, newref, bondstr, DataBond));
    }

    verbize(-5, "debug", "Added Element from keystroke: %c\n", input);
    
    /* now, search for data again */
    workspace->GetCoderack().
      AddCodelet(new ReadKeyboardCodelet(&newref, workspace,
					 (urgetype)
					 ((workspace->GetCoderack().
					   getTotalUrgency() - lasturge)
					  / 4. + .5)));
					  
    /* and start a codelet on this */
    if (lastelt) {
      EvolSystemBasic *newsys;
      workspace->GetCoderack().
	AddCodelet(new QueueCodelet(newsys = new EvolSystemBasic(), newref));
      newsys->mutate();
    } else
      workspace->GetCoderack().
	AddCodelet(new RepeatedCodelet(newref));
  } else
    workspace->GetCoderack().
      AddCodelet(new ReadKeyboardCodelet(lastelt, workspace, lasturge,
					  initial));
}

const char *ReadKeyboardCodelet::Class() const {
  return "ReadKeyboard";
}

int ReadKeyboardCodelet::AssertValid() {
  AIObject *obj;

  verbize(-2, "assert", "ReadKeyboardCodelet::AssertValid\n");

  if (lastelt) {
    obj = dynamic_cast<AIObject*>(lastelt);
    aiassert(obj && obj->type == CWorkspaceRef, "valid lastelt");
  }

  obj = dynamic_cast<AIObject*>(workspace);
  aiassert(obj && (obj->type == CMemoryWorkspace ||
		   obj->type == CBigEndianWorkspace ||
		   obj->type == CLittleEndianWorkspace), "valid workspace");

  return (type == CReadKeyboardCodelet);
}

int ReadKeyboardCodelet::WriteObject(FILE *fp) {
  size_t result = Codelet::WriteObject(fp);
  result = fwrite(&reference_tdiff, sizeof(clock_t), 1, fp) + result;
  result = fwrite(&lastelt, sizeof(WorkspaceRef *), 1, fp) + result;
  result = fwrite(&workspace, sizeof(Workspace *), 1, fp) + result;
  result = fwrite(&initial, sizeof(clock_t), 1, fp) + result;
  return fwrite(&lasturge, sizeof(urgetype), 1, fp) + result;
}

int ReadKeyboardCodelet::kbhit() {
  struct termios term, oterm;
  int fd = 0;
  int c = 0;
  
  /* get the terminal settings */
  tcgetattr(fd, &oterm);

  /* get a copy of the settings, which we modify */
  memcpy(&term, &oterm, sizeof(term));

  /* put the terminal in non-canonical mode, any 
     reads timeout after 0.0 seconds or when a 
     single character is read */
  term.c_lflag = term.c_lflag & (!ICANON);
  term.c_cc[VMIN] = 0;
  term.c_cc[VTIME] = 0;
  tcsetattr(fd, TCSANOW, &term);

  /* get input - timeout after 0.0 seconds or 
     when one character is read. If timed out
     getchar() returns -1, otherwise it returns
     the character */
  c=getchar();

  /* reset the terminal to original state */
  tcsetattr(fd, TCSANOW, &oterm);

  /* if we retrieved a character, put it back on
     the input stream */
  if (c != -1)
    ungetc(c, stdin);

  /* return 1 if the keyboard was hit, or 0 if it
     was not hit */
  return ((c!=-1)?1:0);
}

int ReadKeyboardCodelet::getch() {
  int c, i, fd=0;
  struct termios term, oterm;
  
  /* get the terminal settings */
  tcgetattr(fd, &oterm);
  
  /* get a copy of the settings, which we modify */
  memcpy(&term, &oterm, sizeof(term));
  
  /* put the terminal in non-canonical mode, any 
     reads will wait until a character has been
     pressed. This function will not time out */
  term.c_lflag = term.c_lflag & (!ICANON);
  term.c_cc[VMIN] = 1;
  term.c_cc[VTIME] = 0;
  tcsetattr(fd, TCSANOW, &term);

  /* get a character. c is the character */
  c=getchar();

  /* reset the terminal to its original state */
  tcsetattr(fd, TCSANOW, &oterm);

  /* return the charcter */
  return c;
}

/*****************************************************************************/

TypeDocumentCodelet::TypeDocumentCodelet(char *filename, Coderack *cd) :
  Codelet(TYPEDOC_UINT) {
  fp = fopen(filename, "r");
  coderack = cd;
  
  // Try to find restarting problem
  if (beendone)
    exit(-1);
  beendone = 1;

  flags |= PRIV_FLAG;
  strcpy(filesaved, filename);

  TrackLink::MemStore(trackid, &coderack, FALSE);
}

TypeDocumentCodelet::TypeDocumentCodelet(FILE *newfp, Coderack *cd,
					 urgetype urge) :
  Codelet(urge) {
  fp = newfp;
  coderack = cd;
  flags |= PRIV_FLAG;
  filesaved[0] = '\0';

  TrackLink::MemStore(trackid, &coderack, FALSE);
}

TypeDocumentCodelet::TypeDocumentCodelet(char *filename, FILE *newfp,
					 Coderack *cd, urgetype urge) :
  Codelet(urge) {
  fp = newfp;
  coderack = cd;
  flags |= PRIV_FLAG;

  strcpy(filesaved, filename);

  TrackLink::MemStore(trackid, &coderack, FALSE);
}

TypeDocumentCodelet::TypeDocumentCodelet(FILE *ffp) :
  Codelet(ffp) {
  fread(&fp, sizeof(FILE *), 1, ffp);
  fread(&coderack, sizeof(Coderack *), 1, ffp);
  fread(filesaved, sizeof(char), FILESAVED_SIZE, ffp);

  TrackLink::MemStore(trackid, &coderack, FALSE);
}

void TypeDocumentCodelet::Execute() {
  int c;

  if (recentchar) {
    // Put back on Coderack
    coderack->AddCodelet(new TypeDocumentCodelet(filesaved, fp, coderack,
						 getUrgency()));
    return;
  }

  c = fgetc(fp);

  if (c != EOF) {
    recentchar = c;
    //ungetc(c, stdin);

    // Put back on Coderack
    coderack->AddCodelet(new TypeDocumentCodelet(filesaved, fp, coderack,
						 TYPEDOC_UINT));
						 //(urgetype)
						 //((coderack->getTotalUrgency() -
						 //  getUrgency())
						 // / TYPEDOC_UDIV + TYPEDOC_UINT)));

    printf("%c", c);
    fflush(NULL);
  }
}

const char *TypeDocumentCodelet::Class() const {
  return "TypeDocumentCodelet";
}

int TypeDocumentCodelet::AssertValid() {
  return TRUE;
}

int TypeDocumentCodelet::WriteObject(FILE *ffp) {
  size_t result = Codelet::WriteObject(ffp);
  result = fwrite(&fp, sizeof(FILE *), 1, ffp) + result;
  result = fwrite(&coderack, sizeof(Coderack *), 1, ffp) + result;
  return fwrite(filesaved, sizeof(char), FILESAVED_SIZE, ffp) + result;
}
