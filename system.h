#ifndef SYSTEM_H
#define SYSTEM_H

class EvolSystem;
class EvolSystemBasic;
class EvolSystemCombo;
class EvolSystemPtr;
class EvolSystemBasicPtr;
class EvolSystemComboPtr;

typedef float Confidence;

#define INIT_CRED .5
#define SYSTEM_MAX_LIFE 65536
#define MAX_PER_CHAR 100   /* Max instructions executed for 1 prediction */

// Pointer classes -- used to keep track of number of references

class EvolSystemPtr {
public:
  // Constructors
  EvolSystemPtr();
  EvolSystemPtr(const EvolSystemPtr &right);
  EvolSystemPtr(EvolSystem *sys);

  // Copying after construction
  virtual EvolSystemPtr operator=(const EvolSystemPtr &right);
  virtual EvolSystemPtr operator=(EvolSystem *sys);

  virtual ~EvolSystemPtr();

  EvolSystem *operator->();
  virtual EvolSystem *GetSystem() const;
  virtual EvolSystem **GetSystemPP();

protected:
  EvolSystem *system;
};

class EvolSystemBasicPtr : public EvolSystemPtr {
public:
  // Constructors
  EvolSystemBasicPtr();
  EvolSystemBasicPtr(EvolSystemBasic *sys);

  // Copying after construction
  virtual EvolSystemBasicPtr operator=(EvolSystemBasic *sys);

  EvolSystemBasic *operator->();
  EvolSystemBasic *GetSystemBasic() const;
};

class EvolSystemComboPtr : public EvolSystemPtr {
public:
  // Constructors
  EvolSystemComboPtr();
  EvolSystemComboPtr(EvolSystemCombo *sys);

  // Copying after construction
  virtual EvolSystemComboPtr operator=(EvolSystemCombo *sys);

  EvolSystemCombo *operator->();
  EvolSystemCombo *GetSystemCombo() const;
};

// Actual classes...............

#include "base.h"

/* General System class (tower or individual) */
class EvolSystem : public AIObject {
public:
  EvolSystem();
  EvolSystem(FILE *fp);
  ~EvolSystem();

  virtual void mutate() = NULL;
  virtual EvolSystemPtr reproduce() = NULL; // +
  virtual EvolSystemPtr reproduce(float prob) = NULL; // +

  virtual char execute(char input) = NULL;

  virtual float Credibility();
  virtual void PredFailure() = NULL;
  virtual void PredSuccess() = NULL;
  virtual void Weaken();
  virtual void Weaken(Confidence cred);
  virtual void Strengthen();
  virtual void Strengthen(Confidence cred);

  virtual EvolSystemPtr CheckLife() = NULL;

  virtual char upexec(char input) = NULL;
  virtual char dnexec(char input) = NULL;

  virtual int AssertValid() = NULL;

  void AddReference();
  void RemoveReference();

  virtual int WriteObject(FILE *fp);

  virtual unsigned GetTotalSysCount() = NULL;

  static double GetAverageAge();

protected:
  unsigned long score;
  unsigned long predictions;
  unsigned long totalinst;
  unsigned long age;

  float credfact;

  unsigned refcount;

  static unsigned long systotal;  // not saved, but inferable
  static unsigned long agetotal;  // not saved! just for testing purposes
};

/* single evolai system */
class EvolSystemBasic : public EvolSystem {
public:
  EvolSystemBasic();
  EvolSystemBasic(FILE *fp);
  ~EvolSystemBasic();

  virtual void mutate();
  virtual EvolSystemPtr reproduce(); // +
  virtual EvolSystemPtr reproduce(float prob); // +

  virtual char execute(char input);

  virtual void PredFailure();
  virtual void PredSuccess();

  virtual EvolSystemPtr CheckLife();

  virtual char upexec(char input);
  virtual char dnexec(char input);

  virtual int AssertValid();

  virtual int WriteObject(FILE *fp);

  virtual unsigned GetTotalSysCount();

protected:
  EvolSystemBasic(unsigned char *newdna, unsigned newlen);
  EvolSystemBasic(EvolSystemBasicPtr prist);

private:
  EvolSystemBasicPtr pristine;

  unsigned char *dna;
  unsigned char *pool;
  unsigned length;
  unsigned dnalen;
  unsigned poolsize;

  long dnal;
  long datl;
  char side;
};

/* tower of evolai systems */
class EvolSystemCombo : public EvolSystem {
public:
  EvolSystemCombo(EvolSystemPtr top, EvolSystemPtr bot);
  EvolSystemCombo(FILE *fp);
  ~EvolSystemCombo();

  virtual void mutate();
  virtual EvolSystemPtr reproduce(); // +
  virtual EvolSystemPtr reproduce(float prob); // +

  virtual char execute(char input);

  virtual void PredFailure();
  virtual void PredSuccess();

  virtual EvolSystemPtr CheckLife();

  EvolSystemPtr GetAbove();
  EvolSystemPtr GetBelow();

  virtual char upexec(char input);
  virtual char dnexec(char input);

  virtual int AssertValid();

  virtual int WriteObject(FILE *fp);

  virtual unsigned GetTotalSysCount();

private:
  EvolSystemPtr above;
  EvolSystemPtr below;
};

#endif
