#include <math.h>
#include <sys/types.h>
#include "system.h"
#include "base.h"

#define	ULONG_MAX	4294967295UL	/* max of "unsigned long int" */

#define checkdat() if (datl >= length || datl < 0) { endc = 2; break; }
#define getdata(datl) ((datl < dnalen) ? dna[datl] : pool[datl - dnalen])

unsigned long EvolSystem::systotal = 0;
unsigned long EvolSystem::agetotal = 0;

double EvolSystem::GetAverageAge() {
  return (double) agetotal / (double) systotal;
}

/*****************************************************************************/

EvolSystem::EvolSystem() :
  AIObject(CEvolSystem) {
  score = 0;
  totalinst = 0;
  predictions = 0;
  credfact = INIT_CRED;
  refcount = 0;
  age = 0;
  systotal++;
}

EvolSystem::EvolSystem(FILE *fp) :
  AIObject(fp) {
  fread(&score, sizeof(unsigned long), 1, fp);
  fread(&predictions, sizeof(unsigned long), 1, fp);
  fread(&totalinst, sizeof(unsigned long), 1, fp);
  fread(&age, sizeof(unsigned long), 1, fp);
  fread(&credfact, sizeof(float), 1, fp);
  fread(&refcount, sizeof(unsigned), 1, fp);
  agetotal += age;
  systotal++;
}

EvolSystem::~EvolSystem() {
  verbize(-5, "debug", "Destroying System %ld\n", this);
  agetotal -= age;
  systotal--;
  type = CDeletedClass;
}

float EvolSystem::Credibility() {
  if (predictions && score)
    return credfact * ((float) score) / ((float) predictions);
  if (predictions)
    return credfact / predictions;
  return credfact;
}

void EvolSystem::Weaken() {
  credfact *= .9;
}

void EvolSystem::Weaken(Confidence cred) {
  if (cred > 1.) {
    verbize(2, "error", "Credibility %f > 1.!\n", cred);
    exit(-2);
  }
  credfact *= sqrt(1. - cred);
}

void EvolSystem::Strengthen() {
  credfact *= 1.11;
  if (credfact > 1.)
    credfact = 1.;
}

void EvolSystem::Strengthen(Confidence cred) {
  if (cred > 1.) {
    verbize(2, "error", "Credibility %f > 1.!\n", cred);
    exit(-2);
  }
  credfact *= sqrt(1. + cred);
  if (credfact > 1.)
    credfact = 1.;
}

void EvolSystem::AddReference() {
  refcount++;
  verbize(-6, "refcount", "Increasing reference for %ld to %d\n", this,
	  refcount);
}

void EvolSystem::RemoveReference() {
  refcount--;
  verbize(-6, "refcount", "Decreasing reference for %ld to %d\n", this,
	  refcount);
  if (refcount <= 0)
    delete this;
}

int EvolSystem::WriteObject(FILE *fp) {
  size_t result = AIObject::WriteObject(fp);
  result = fwrite(&score, sizeof(unsigned long), 1, fp) + result;
  result = fwrite(&predictions, sizeof(unsigned long), 1, fp) + result;
  result = fwrite(&totalinst, sizeof(unsigned long), 1, fp) + result;
  result = fwrite(&age, sizeof(unsigned long), 1, fp) + result;
  result = fwrite(&credfact, sizeof(float), 1, fp) + result;
  return fwrite(&refcount, sizeof(unsigned), 1, fp) + result;
}

/*****************************************************************************/

EvolSystemBasic::EvolSystemBasic() {
  verbize(-5, "debug", "Creating new basic System %ld\n", this);

  type = CEvolSystemBasic;
  pristine = NULL;

  dna = NULL;
  pool = NULL;
  dnalen = length = 0;
  poolsize = 0;

  dnal = datl = 0;
  side = 0;

  TrackLink::MemStore(trackid, pristine.GetSystemPP(), FALSE);
  TrackLink::MemStore(trackid, &dna, FALSE);
  TrackLink::MemStore(trackid, &pool, FALSE);
}

EvolSystemBasic::EvolSystemBasic(FILE *fp) :
  EvolSystem(fp) {
  fread(pristine.GetSystemPP(), sizeof(EvolSystemBasic *), 1, fp);
  fread(&dnalen, sizeof(unsigned), 1, fp);
  fread(dna, sizeof(unsigned char), dnalen, fp);
  fread(&poolsize, sizeof(unsigned), 1, fp);
  fread(pool, sizeof(unsigned char), poolsize, fp);
  fread(&length, sizeof(unsigned), 1, fp);
  fread(&dnal, sizeof(long), 1, fp);
  fread(&datl, sizeof(long), 1, fp);
  fread(&side, sizeof(char), 1, fp);

  TrackLink::MemStore(trackid, pristine.GetSystemPP(), FALSE);
  TrackLink::MemStore(trackid, &dna, FALSE);
  TrackLink::MemStore(trackid, &pool, FALSE);
}

EvolSystemBasic::~EvolSystemBasic() {
  verbize(-5, "debug", "killing of basic: %ld\n", this);
  if (dna)
    aifree(dna);
  if (pool)
    aifree(pool);
  type = CDeletedClass;
}

void EvolSystemBasic::mutate() {
  unsigned newlen = dnalen;

  verbize(-6, "debug", "Mutating %ld (%d)\n", this, dnalen);

  /* Extend or Shrink dna array */
  if (random(2) || dnalen == 0)
    while (!newlen || !random(sqrt(newlen)))
      newlen++;  /* make dna longer */
  else
    while (!random(sqrt(newlen)) && newlen > 1)
      newlen--;  /* make dna shorter */

  /* Allocate new dna array */
  if (newlen != dnalen)
    dna = (unsigned char *) airealloc(dna, newlen, "mutate dna array", 1, -2);

  /* Copy dna, introducing errors */
  for (unsigned base = 0; base < min(dnalen, newlen); base++) {
    if (!random(sqrt(dnalen)))
      dna[base] = random(256);
  }

  /* If dna is longer, fill with random bytes */
  for (unsigned base = dnalen; base < newlen; base++)
    dna[base] = random(256);

  length += newlen - dnalen;
  dnalen = newlen;
}

EvolSystemPtr EvolSystemBasic::reproduce() {
  verbize(-5, "debug", "Reproducing basic-noprob- %ld\n", this);

  if (score > 0 || !pristine.GetSystem()) {
    if (pristine.GetSystem())
      score--;
    return new EvolSystemBasic(this);
  }
  return NULL;
}

EvolSystemPtr EvolSystemBasic::reproduce(float prob) {
  verbize(-5, "debug", "Reproducing basic %ld\n", this);

  if (score > 0 || !pristine.GetSystem()) {
    if (frand() < prob) {
      if (pristine.GetSystem()) {
	score--;
	verbize(-6, "debug", "Reproducing off pristine %ld\n", pristine.GetSystem());
	return pristine->reproduce();
      } else
	return NULL;
    } else
      return reproduce();
  }

  return NULL;
}

char EvolSystemBasic::execute(char input) {
  verbize(-5, "debug", "Executing system %ld\n", this);
  char test = dnexec(upexec(input));
  return test;
}

void EvolSystemBasic::PredFailure() {
  if (predictions < ULONG_MAX)
    predictions++;
  age++;
  agetotal++;
}

void EvolSystemBasic::PredSuccess() {
  if (predictions < ULONG_MAX) {
    score++;
    predictions++;
  }
  age++;
  agetotal++;
}

EvolSystemPtr EvolSystemBasic::CheckLife() {
  if (predictions > SYSTEM_MAX_LIFE) {
    verbize(-7, "debug", "RR: Basic CheckLife %ld\n", this);
    return NULL;
  } else
    return this;
}

EvolSystemBasic::EvolSystemBasic(unsigned char *newdna, unsigned newlen) {
  pristine = NULL;
  type = CEvolSystemBasic;

  dna = (unsigned char *) aialloc(newlen, "making offspring", 1, -1);

  for (int base = 0; base < newlen; base++)
    dna[base] = newdna[base];
  pool = NULL;
  poolsize = 0;
  length = dnalen = newlen;

  dnal = datl = side = 0;

  TrackLink::MemStore(trackid, pristine.GetSystemPP(), FALSE);
  TrackLink::MemStore(trackid, &dna, FALSE);
  TrackLink::MemStore(trackid, &pool, FALSE);
}

EvolSystemBasic::EvolSystemBasic(EvolSystemBasicPtr prist) {
  verbize(-6, "debug", "EvolSystemBasic off prist %ld\n", prist.GetSystem());

  type = CEvolSystemBasic;

  //EvolSystemBasic(prist->dna, prist->length);
  dna = (unsigned char *) aialloc(prist->length, "making offspring", 1, -1);

  for (int base = 0; base < prist->length; base++)
    dna[base] = (prist->dna)[base];
  pool = NULL;
  poolsize = 0;
  length = dnalen = prist->length;

  pristine = prist;

  dnal = datl = side = 0;

  TrackLink::MemStore(trackid, pristine.GetSystemPP(), FALSE);
  TrackLink::MemStore(trackid, &dna, FALSE);
  TrackLink::MemStore(trackid, &pool, FALSE);
}

char EvolSystemBasic::upexec(char input) {
  verbize(-7, "debug", "Upexec on basic %ld\n", this);
  return dnexec(input);
}

char EvolSystemBasic::dnexec(char input) {
  verbize(-7, "debug", "Dnexec on basic %ld\n", this);

  /* Meta-AI structure */
  long inst = 0; /* instruction count */

  int curr;  /* current instruction */

  int endc;  /* flag for completion of prediction */

  long base;
  unsigned char temp;
  long tmpl;

  /* AI Variables */
  unsigned char accu;       /* Accumulator */

  /* Begin Prediction for input file */
  accu = input;

  /* Predict the next character */
  for (; inst < MAX_PER_CHAR; inst++, totalinst++) {

    /* Get the next instruction */
    if (dnal >= length) {
      dnal = side = 0;
      curr = getdata(0) % 16;
    } else {
      if (!side)
	curr = getdata(dnal) % 16;
      else {
	curr = getdata(dnal) >> 4;
	dnal++;
      }
      side = (side + 1) % 2;
    }

    /* Execute instruction */
    switch (curr) {
    case (0):   /* Test conditional-jump */
      if (accu) {
	checkdat();
	tmpl = dnal;
	dnal = datl;
	datl = tmpl;
      }
      break;
    case (1):   /* Read */
      checkdat();
      accu = getdata(datl);
      datl++;
      break;
    case (2):   /* Write */
      if (datl == length)
	length++;
      checkdat();
      if (datl < dnalen)
	dna[datl++] = accu;
      else {
	if (poolsize < length - dnalen) {
	  pool = (unsigned char *) airealloc(pool, poolsize = 2 * poolsize + 1,
					     "Allocating a pool", 1, -3);
	}
	pool[datl++ - dnalen] = accu;
	datl++;
      }
      break;
    case (3):   /* I/O with accumulator */
      endc = 1;
      break;
    case (4):  /* Set DAT to accu */
      datl = accu;
      break;
    case (5):  /* Set accu to first byte of DAT */
      accu = datl % 256;
      break;
    case (6):  /* Move DAT by a byte, down */
      datl >>= 8;
      break;
    case (7):  /* Move DAT by a byte, up */
      datl <<= 8;
      break;
    case (8):  /* Change Data pointer */
      datl += accu;
      break;
    case (9): /* Add to accumulator */
      checkdat();
      accu += getdata(datl);
      break;
    case (10): /* Bitwise AND with accumulator */
      checkdat();
      accu &= getdata(datl);
      break;
    case (11): /* Bitwise OR with accumulator */
      checkdat();
      accu |= getdata(datl);
      break;
    case (12): /* Bitwise XOR with accumulator */
      checkdat();
      accu ^= getdata(datl);
      break;
    case (13): /* Bitwise NOT of accumulator */
      accu = ~accu;
      break;
    case (14): /* Bitwise shift left */
      accu <<= 1;
      break;
    case (15): /* Bitwise shift right */
      accu >>= 1;
      break;
    case (EOF): /* End of DNA */
      break;
    }
  }

  if (--endc || inst == MAX_PER_CHAR) {
    if (predictions < ULONG_MAX / 2 - 1)
      predictions = predictions * 2 + 1; /* will quickly kill */
    if (score > 0)
      score--;
  }

  return accu;
}

int EvolSystemBasic::AssertValid() {
  AIObject *obj;
  verbize(-2, "assert", "EvolSystemBasic::AssertValid: %ld\n", this);

  if (pristine.GetSystem()) {
    obj = dynamic_cast<AIObject*>(pristine.GetSystem());
    aiassert(obj && (obj->type == CEvolSystemBasic ||
		     obj->type == CEvolSystemCombo), "valid pristine");
  }

  return (type == CEvolSystemBasic);
}

int EvolSystemBasic::WriteObject(FILE *fp) {
  size_t result = EvolSystem::WriteObject(fp);
  result = fwrite(pristine.GetSystemPP(), sizeof(EvolSystemBasic *), 1, fp)
    + result;
  result = fwrite(&dnalen, sizeof(unsigned), 1, fp) + result;
  result = fwrite(dna, sizeof(unsigned char), dnalen, fp) + result;
  result = fwrite(&poolsize, sizeof(unsigned), 1, fp) + result;
  result = fwrite(pool, sizeof(unsigned char), poolsize, fp) + result;
  result = fwrite(&length, sizeof(unsigned), 1, fp) + result;
  result = fwrite(&dnal, sizeof(long), 1, fp) + result;
  result = fwrite(&datl, sizeof(long), 1, fp) + result;
  return fwrite(&side, sizeof(char), 1, fp) + result;
}

unsigned EvolSystemBasic::GetTotalSysCount() {
  return 1;
}

EvolSystemCombo::EvolSystemCombo(EvolSystemPtr top, EvolSystemPtr bot) {
  type = CEvolSystemCombo;

  above = top;
  below = bot;

  if (above.GetSystem() == below.GetSystem()) {
    verbize(2, "error", "Got the impossible condition for %ld.\n",
	    above.GetSystem());
    exit(-3);
  }    

  TrackLink::MemStore(trackid, above.GetSystemPP(), FALSE);
  TrackLink::MemStore(trackid, below.GetSystemPP(), FALSE);
}

EvolSystemCombo::EvolSystemCombo(FILE *fp) :
  EvolSystem(fp) {
  fread(above.GetSystemPP(), sizeof(EvolSystem *), 1, fp);
  fread(below.GetSystemPP(), sizeof(EvolSystem *), 1, fp);

  TrackLink::MemStore(trackid, above.GetSystemPP(), FALSE);
  TrackLink::MemStore(trackid, below.GetSystemPP(), FALSE);
}

EvolSystemCombo::~EvolSystemCombo() {
  verbize(-5, "debug", "killing of combo: %ld\n", this);
  // debugy code (to make sure there aren't renegate references
  above = NULL;
  below = NULL;
  type = CDeletedClass;
}

void EvolSystemCombo::mutate() {
  verbize(-6, "debug", "Mutating combo %ld [%ld, %ld]\n", this,
	  above.GetSystem(), below.GetSystem());
  above->mutate();
  below->mutate();
}

EvolSystemPtr EvolSystemCombo::reproduce() {
  EvolSystemPtr newab, newbe;
  EvolSystemPtr combo;

  verbize(-5, "debug", "Reproducing combo (no prob) %ld [%ld, %ld]\n",
	  this, above.GetSystem(), below.GetSystem());

  if (score > 0) {
    score--;
    newab = above->reproduce();
    newbe = below->reproduce();
    if (newab.GetSystem() && newbe.GetSystem())
      return new EvolSystemCombo(newab, newbe);
    if (newab.GetSystem())
      return newab;
    if (newbe.GetSystem())
      return newbe;
  }
  return NULL;
}

EvolSystemPtr EvolSystemCombo::reproduce(float prob) {
  EvolSystemPtr newab, newbe;
  EvolSystemPtr combo;

  verbize(-5, "debug", "Reproducing combo %ld [%ld, %ld]\n",
	  this, above.GetSystem(), below.GetSystem());

  if (score > 0) {
    score--;
    newab = above->reproduce(prob);
    newbe = below->reproduce(prob);
    if (newab.GetSystem() && newbe.GetSystem())
      return new EvolSystemCombo(newab, newbe);
    if (newab.GetSystem())
      return newab;
    if (newbe.GetSystem())
      return newbe;
  }
  return NULL;
}

char EvolSystemCombo::execute(char input) {
  verbize(-5, "debug", "Executing combo %ld start: %ld:%d, %ld:%d\n", this, above.GetSystem(), above->type, below.GetSystem(), below->type);
  char test = dnexec(upexec(input));
  verbize(-5, "debug", "Executing combo %ld end\n", this);
  return test;
}

void EvolSystemCombo::PredFailure() {
  above->PredFailure();
  below->PredFailure();
  if (predictions < ULONG_MAX)
    predictions++;
  age++;
  agetotal++;
}

void EvolSystemCombo::PredSuccess() {
  if (predictions < ULONG_MAX) {
    score++;
    predictions++;
  }
  above->PredSuccess();
  below->PredSuccess();
  age++;
  agetotal++;
}

EvolSystemPtr EvolSystemCombo::CheckLife() {
  verbize(-7, "debug", "Beginning CheckLife-combo %ld (%ld:%d, %ld:%d)\n",
	  this, above.GetSystem(), above->type, below.GetSystem(), below->type);

  EvolSystemPtr newabove = above->CheckLife();
  EvolSystemPtr newbelow = below->CheckLife();
  EvolSystemPtr newcombo;

  if ((newabove.GetSystem() == above.GetSystem()) &&
      (newbelow.GetSystem() == below.GetSystem()))
    return this;
  else if (newabove.GetSystem() && newbelow.GetSystem())
    return new EvolSystemCombo(newabove, newbelow);
  else if (newabove.GetSystem())
    return newabove;
  else if (newbelow.GetSystem())
    return newbelow;
  return NULL;
}

EvolSystemPtr EvolSystemCombo::GetAbove() {
  return above;
}

EvolSystemPtr EvolSystemCombo::GetBelow() {
  return below;
}

char EvolSystemCombo::upexec(char input) {
  verbize(-7, "debug", "Upexec on combo %ld\n", this);
  return above->upexec(below->upexec(input));
}

char EvolSystemCombo::dnexec(char input) {
  verbize(-7, "debug", "Dnexec on combo %ld\n", this);
  return below->dnexec(above->dnexec(input));
}

int EvolSystemCombo::AssertValid() {
  AIObject *obj;

  verbize(-2, "assert", "EvolSystemCombo::AssertValid: %ld\n", this);

  obj = dynamic_cast<AIObject*>(above.GetSystem());
  aiassert(obj && (obj->type == CEvolSystemBasic ||
		   obj->type == CEvolSystemCombo), "valid above");

  obj = dynamic_cast<AIObject*>(below.GetSystem());
  aiassert(obj && (obj->type == CEvolSystemBasic ||
		   obj->type == CEvolSystemCombo), "valid below");

  return (type == CEvolSystemCombo);
}

int EvolSystemCombo::WriteObject(FILE *fp) {
  size_t result = EvolSystem::WriteObject(fp);
  result = fread(above.GetSystemPP(), sizeof(EvolSystem *), 1, fp) + result;
  return fread(below.GetSystemPP(), sizeof(EvolSystem *), 1, fp) + result;
}

unsigned EvolSystemCombo::GetTotalSysCount() {
  return above->GetTotalSysCount() + below->GetTotalSysCount();
}

/***************************************************************************/

EvolSystemPtr::EvolSystemPtr() {
  system = NULL;
}

EvolSystemPtr::EvolSystemPtr(const EvolSystemPtr &right) {
  system = right.system;
  if (system)
    system->AddReference();
}

EvolSystemPtr::EvolSystemPtr(EvolSystem *sys) {
  system = sys;
  if (system)
    system->AddReference();
}

EvolSystemPtr EvolSystemPtr::operator=(const EvolSystemPtr &right) {
  if (system)
    system->RemoveReference();
  system = right.system;
  if (system)
    system->AddReference();
  return *this;
}

EvolSystemPtr EvolSystemPtr::operator=(EvolSystem *sys) {
  if (system)
    system->RemoveReference();
  system = sys;
  if (system)
    system->AddReference();
  return *this;
}

EvolSystemPtr::~EvolSystemPtr() {
  if (system)
    system->RemoveReference();
}

EvolSystem *EvolSystemPtr::operator->() {
  return system;
}

EvolSystem *EvolSystemPtr::GetSystem() const {
  return system;
}

EvolSystem **EvolSystemPtr::GetSystemPP() {
  return &system;
}

EvolSystemBasicPtr::EvolSystemBasicPtr() {
  system = NULL;
}

EvolSystemBasicPtr::EvolSystemBasicPtr(EvolSystemBasic *sys) {
  system = sys;
  if (system)
    system->AddReference();
}

EvolSystemBasicPtr EvolSystemBasicPtr::operator=(EvolSystemBasic *sys) {
  if (system)
    system->RemoveReference();
  system = sys;
  if (system)
    system->AddReference();
  return *this;
}

EvolSystemBasic *EvolSystemBasicPtr::operator->() {
  return (EvolSystemBasic *) system;
}

EvolSystemBasic *EvolSystemBasicPtr::GetSystemBasic() const {
  return (EvolSystemBasic *) system;
}

EvolSystemComboPtr::EvolSystemComboPtr() {
  system = NULL;
}

EvolSystemComboPtr::EvolSystemComboPtr(EvolSystemCombo *sys) {
  system = sys;
  if (system)
    system->AddReference();
}

EvolSystemComboPtr EvolSystemComboPtr::operator=(EvolSystemCombo *sys) {
  if (system)
    system->RemoveReference();
  system = sys;
  if (system)
    system->AddReference();
  return *this;
}

EvolSystemCombo *EvolSystemComboPtr::operator->() {
  return (EvolSystemCombo *) system;
}

EvolSystemCombo *EvolSystemComboPtr::GetSystemCombo() const {
  return (EvolSystemCombo *) system;
}
