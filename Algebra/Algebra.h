#ifndef NITCBASE_ALGEBRA_H
#define NITCBASE_ALGEBRA_H

#include "../Cache/OpenRelTable.h"
#include "../Schema/Schema.h"
#include "../define/constants.h"

class Algebra {
 public:
  // Insert
  static int insert(char relName[ATTR_SIZE], int numberOfAttributes, char record[][ATTR_SIZE]);

  // Select unified function that handles both with and without INTO.
  // If targetRel is nullptr, results are displayed; otherwise inserted into targetRel.
  static int select(char srcRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE], char *targetRel = nullptr);

  // Project all (Copy)
  static int project(char srcRel[ATTR_SIZE], char *targetRel);

  // Project unified for both with and without INTO.
  static int project(char srcRel[ATTR_SIZE], int tar_nAttrs, char tar_Attrs[][ATTR_SIZE], char *targetRel = nullptr);

  // Join
  static int join(char srcRelOne[ATTR_SIZE], char srcRelTwo[ATTR_SIZE], char targetRel[ATTR_SIZE],
                  char attrOne[ATTR_SIZE], char attrTwo[ATTR_SIZE]);
};

#endif  // NITCBASE_ALGEBRA_H
