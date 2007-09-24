/*
  Copyright (c) 2007 David Ellinghaus <dellinghaus@zbh.uni-hamburg.de>
  Copyright (c) 2007 Center for Bioinformatics, University of Hamburg
  See LICENSE file or http://genometools.org/license.html for license details.
*/

#include <string.h>

#include "libgtcore/minmax.h"
#include "libgtmatch/divmodmul.h"
#include "libgtmatch/chardef.h"

#include "repeattypes.h"
#include "myxdrop.h"

#define MINUSINFINITYINT ((int)integermin)
#define ACCESSTOFRONT(D,K) ((unsigned int) (D) * (D) + (D) + (K))

/*
 The following function shows the matrix of the calculated fronts.
 */
/* CAUTION: fronts, that run over the matrix boundaries are not shown in
   the printed matrix.
 */
int showmatrix ( ArrayMyfrontvalue * fronts,
  int distance,
  unsigned char * useq,
  unsigned char * vseq,
  int ulen,
  int vlen)
{
  int i, j, k, d = distance + 1, filled = 0;
  unsigned int l;
  int integermax, integermin;

  integermax = MAX (ulen, vlen), integermin = -integermax;

  printf( "ACHTUNG: Fronten, die ueber die boundaries gehen,"
                  " erscheinen nicht in Matrix.\n");
  printf("matrix:\n");
  printf("        ");
  printf("%-3c ", vseq[0]);
  /* print vseq */
  for (i = 1; i < vlen; i++)
  {
    printf("%-3c ", vseq[i]);
  }

  for (i = 0; i <= ulen; i++)
  {
    printf("\n");
    /* print useq */
    if (i != 0)
    {
      printf("%-3c ", useq[i - 1]);
    }
    else
    {
      printf("    ");
    }
    for (j = 0; j <= vlen; j++)
    {
      d = distance + 1;
      k = i - j;
      for (l = 0; l < fronts->nextfreeMyfrontvalue; l++)
      {
        if (MINUSINFINITYINT == fronts->spaceMyfrontvalue[l].dptabrow)
        {
          continue;
        }
        if (i == fronts->spaceMyfrontvalue[l].dptabrow)
        {
          for (d = 0; d <= distance; d++)
          {
            if ((k < -d) || (k > d))
            {
              continue;
            }
            if (l == ACCESSTOFRONT (d, i - j))
            {
              printf("%-3d ", d);
              l = fronts->nextfreeMyfrontvalue;
              filled++;
              break;
            }
          }
        }
      }
      if (d > distance)
      {
        printf("U   ");
      }
    }
  }

  printf("\n%.2f percent of matrix filled\n",
           filled * 100.00 / ((ulen + 1) * (vlen + 1)));

  return 0;
}

/* If showmatrix funktion = "True", then
    (A)->nextfree##TYPE = POS+1; must be set:

 ** The other case, saves more space

#define STOREINARRAYFRONTS(A,POS,TYPE,VAL)\
        CHECKARRAYSPACEMULTI(A,TYPE,POS+1)\
        (A)->space##TYPE[POS] = VAL;\
        (A)->nextfree##TYPE = POS+1;
*/

/*
 POS+1 necessary, since at POS = 0 and nextfree##TYPE = 0
 nothing will be allocated in addition!
 */
#define STOREINARRAYFRONTS(A,POS,TYPE,VAL)\
        CHECKARRAYSPACEMULTI(A,TYPE,POS+1)\
        (A)->space##TYPE[POS] = VAL;

/*
 The following macro swaps two values.
 */
#define SWAPMAYBE\
        if (m < n)\
        {\
          r = m;\
          m = n;\
          n = r;\
        }
/*
 The following macro determines the greatest common divisor.
 */
#define GGT\
        do\
        {\
          r = m % n;\
          m = n;\
          n = r;\
        } while (r != 0)

/*
 The following function calculates the distance from the given scores.
 */
void calculatedistancesfromscores(Arbitraryscores *arbitscores,
    Arbitrarydistances *arbitdistances)
{
  unsigned int m, n, r;
  int mat, mis, ins, del;

  /* if mat is odd double all scores */
  if (MOD2((unsigned int)arbitscores->mat))
  {
    mat = arbitscores->mat  * (int)2;
    mis = arbitscores->mis  * (int)2;
    ins = arbitscores->ins  * (int)2;
    del = arbitscores->del  * (int)2;
  }
  else
  {
    mat = arbitscores->mat;
    mis = arbitscores->mis;
    ins = arbitscores->ins;
    del = arbitscores->del;
  }

  m = (unsigned int)(mat - mis);
  n = (unsigned int)(mat/2 - ins);
  SWAPMAYBE
  GGT;

  n = (unsigned int)(mat/2 - del);
  SWAPMAYBE
  GGT;

  arbitscores->gcd = (int) m;

  arbitdistances->mis  = (mat - mis) / arbitscores->gcd;
  arbitdistances->ins  = (mat/2 - ins)  / arbitscores->gcd;
  arbitdistances->del  = (mat/2 - del)  / arbitscores->gcd;

}

/*
 The following function calculates the maximal allowed number of
 generations with all front values equal minus infinity.
 */
void calculateallowedMININFINITYINTgenerations(
   int *allowedMININFINITYINTgenerations,
   Arbitrarydistances *arbitdistances)
{
  *allowedMININFINITYINTgenerations = MAX(arbitdistances->mis,
      			                  arbitdistances->ins);
  *allowedMININFINITYINTgenerations = MAX(*allowedMININFINITYINTgenerations,
      			                  arbitdistances->del);
  (*allowedMININFINITYINTgenerations)--;
}

/*
 The following macro checks for wildcard and separator symbols.
 */
#define COMPARESYMBOLSSEP(I,J)\
        USEQ(a,I);\
        if (a == (unsigned char) SEPARATOR)\
        {\
          ulen = I;\
          break;\
        }\
        VSEQ(b,J);\
        if (b == (unsigned char) SEPARATOR)\
        {\
          vlen = J;\
          break;\
        }\
        if (a != b || a == (unsigned char) WILDCARD)\
        {\
          break;\
        }

#ifdef XDROPDEF_H
  #undef MATCHSCORE
  #undef HALFMATCHSCORE
  #undef MISMATCHSCORE
  #undef S
  #undef SETDBACK
#endif

#define MATCHSCORE arbitscores->mat
#define HALFMATCHSCORE arbitscores->mat/2
#define GCD arbitscores->gcd
#define S(K,D)\
        ((K) * HALFMATCHSCORE - (D) * GCD)
#define SETDBACK(XDROPBELOWSCORE)\
        (XDROPBELOWSCORE + HALFMATCHSCORE) / GCD + 1

#define EVALXDROPARBITSCORES EVALXDROPARBITSCORESRIGHT
#define USEQ(A,I) A = getencodedchar(str_useq,\
                                     useq+(Seqpos)(I),\
                                     Forwardmode)
#define VSEQ(A,J) A = getencodedchar(str_vseq,\
                                     vseq+(Seqpos)(J),\
				     Forwardmode)

#include "myxdrop.gen"

#undef EVALXDROPARBITSCORES
#undef USEQ
#undef VSEQ

/*
  Now we redefine the macros to compute the left to right
  we use macros to abstract from the differences. The
  following 4 macros are defined for the right to left extension beginning
  with the last character of the strings under consideration.
*/

#define EVALXDROPARBITSCORES EVALXDROPARBITSCORESLEFT
#define USEQ(A,I) A = getencodedchar(str_useq,\
                                     useq-(Seqpos)1-(I),\
				     Forwardmode)
#define VSEQ(A,J) A = getencodedchar(str_vseq,\
                                     vseq-(Seqpos)1-(J),\
				     Forwardmode)

#include "myxdrop.gen"
