#pragma once
#ifndef __VSOP87_H
#define __VSOP87_H

#include <stdint.h>
#include <stdbool.h>

#include "misc.h"

void earth_coord(TDateTime date, double *l, double *b, double *r);

#endif /* __VSOP87_H */

