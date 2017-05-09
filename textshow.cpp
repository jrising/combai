#include "textshow.h"

TextShowWorkspace::TextShowWorkspace(Workspace &ws) :
  Codelet(TEXTWRK_URGE), workspace(ws) {
  type = CTextShowWorkspace;
  flags |= PRIV_FLAG;
  TrackLink::MemStore(trackid, &workspace, CONST_FLAG);
}

TextShowWorkspace::TextShowWorkspace(FILE *fp, Workspace &ws) :
  Codelet(fp), workspace(ws) {
  TrackLink::MemStore(trackid, &workspace, CONST_FLAG);
}

void TextShowWorkspace::Execute() {
  /* Randomly select an element */
  WorkspaceRef &ref = workspace.SalientElement(irand(workspace.GetCurrentIndex()));

  /* Put codelet to describe element into coderack */
  workspace.GetCoderack().AddCodelet(new TextShowElement(ref));
}

const char *TextShowWorkspace::Class() const {
  return "TextShowWorkspace";
}

int TextShowWorkspace::AssertValid() {
  AIObject *obj;

  verbize(-2, "assert", "TextShowWorkspace::AssertValid\n");

  obj = dynamic_cast<AIObject*>(&workspace);
  aiassert(obj && (obj->type == CMemoryWorkspace ||
		   obj->type == CBigEndianWorkspace ||
		   obj->type == CLittleEndianWorkspace), "valid workspace");

  return (type == CTextShowWorkspace);
}

int TextShowWorkspace::WriteObject(FILE *fp) {
  Workspace *ws = &workspace;

  size_t result = fwrite(&ws, sizeof(Workspace *), 1, fp);
  return Codelet::WriteObject(fp) + result;
}

TextShowElement::TextShowElement(WorkspaceRef &ref) :
  Codelet(TEXTELT_URGE), chosen(ref) {
  type = CTextShowElement;
  flags |= PRIV_FLAG;
  TrackLink::MemStore(trackid, &chosen, CONST_FLAG);
}

TextShowElement::TextShowElement(FILE *fp, WorkspaceRef &ref) :
  Codelet(fp), chosen(ref) {
  TrackLink::MemStore(trackid, &chosen, CONST_FLAG);
}

void TextShowElement::Execute() {
  WorkspaceRef *curr = &chosen;
  char buff1[81], buff2[81];
  int i;

  /* Output two lines of data */
  for (i = 0; i < 78; i += 2) {
    WorkspaceElt &elt = curr->GetElement();
    if (elt.GetValue() == '\n')
      buff1[i] = '\\';
    else
      buff1[i] = elt.GetValue();
    if (elt.GetBondCount() <= 0) {
      i++;
      break;
    }
    if (elt.GetBondCount() >= 8)
      buff1[i+1] = bondrep[7];
    else
      buff1[i+1] = bondrep[elt.GetBondCount() - 1];
    sprintf(buff2 + i, "%2d", elt.QueueSystemCount());

    WorkspaceBond *next = elt.SelectRandomBond();
    if (next)
      curr = &next->To();
    else
      break;
  }

  buff1[i] = '\0';
  buff2[i] = '\0';

  verbize(-1, "diagi", "%s\n", buff1);
  verbize(-1, "diagi", "%s\n", buff2);

  /* Put back into coderack */
  chosen.GetWorkspace()->GetCoderack().AddCodelet(new TextShowWorkspace(*chosen.GetWorkspace()));
}

const char *TextShowElement::Class() const {
  return "TextShowElement";
}

int TextShowElement::AssertValid() {
  AIObject *obj;

  verbize(-2, "assert", "TextShowElement::AssertValid\n");

  obj = dynamic_cast<AIObject*>(&chosen);
  aiassert(obj && obj->type == CWorkspaceRef, "valid workspace reference");

  return (type == CTextShowElement);
}

int TextShowElement::WriteObject(FILE *fp) {
  WorkspaceRef *ref = &chosen;

  size_t result = fwrite(&ref, sizeof(WorkspaceRef *), 1, fp);
  return Codelet::WriteObject(fp) + result;
}
