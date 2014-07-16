#include "copyright.h"
/*============================================================================*/
/*! \file utils.c
 *  \brief A variety of useful utility functions.
 *
 * PURPOSE: A variety of useful utility functions.
 *
 * CONTAINS PUBLIC FUNCTIONS:
 * - ath_strdup()     - not supplied by fancy ANSI C, but ok in C89
 * - ath_gcd()        - computes greatest common divisor by Euler's method
 * - ath_big_endian() - run-time detection of endianism of the host cpu
 * - ath_bswap()      - fast byte swapping routine
 * - ath_error()      - fatal error routine
 * - minmax1()        - fast Min/Max for a 1d array using registers
 * - minmax2()        - fast Min/Max for a 2d array using registers
 * - minmax3()        - fast Min/Max for a 3d array using registers
 *============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "defs.h"
#include "athena.h"
#include "prototypes.h"
#include "globals.h"

/*----------------------------------------------------------------------------*/
/*! \fn char *ath_strdup(const char *in)
 *  \brief This is really strdup(), but strdup is not available in
 *   ANSI  (-pendantic or -ansi will leave it undefined in gcc)
 *   much like allocate.
 */

char *ath_strdup(const char *in)
{
  char *out = (char *)malloc((1+strlen(in))*sizeof(char));
  if(out == NULL) {
    ath_perr(-1,"ath_strdup: failed to alloc %d\n",(int)(1+strlen(in)));
    return NULL; /* malloc failed */
  }
  return strcpy(out,in);
}

/*----------------------------------------------------------------------------*/
/*! \fn int ath_gcd(int a, int b)
 *  \brief Calculate the Greatest Common Divisor by Euler's method
 */

int ath_gcd(int a, int b)
{
  int c;
  if(b>a) {c=a; a=b; b=c;}
  while((c=a%b)) {a=b; b=c;}
  return b;
}

/*----------------------------------------------------------------------------*/
/*! \fn int ath_big_endian(void)
 *  \brief Return 1 if the machine is big endian (e.g. Sun, PowerPC)
 * return 0 if not (e.g. Intel)
 */

int ath_big_endian(void)
{
  short int n = 1;
  char *ep = (char *)&n;

  return (*ep == 0); /* Returns 1 on a big endian machine */
}

/*----------------------------------------------------------------------------*/
/*! \fn void ath_bswap(void *vdat, int len, int cnt)
 *  \brief Swap bytes, code stolen from NEMO
 */

void ath_bswap(void *vdat, int len, int cnt)
{
  char tmp, *dat = (char *) vdat;
  int k;

  if (len==1)
    return;
  else if (len==2)
    while (cnt--) {
      tmp = dat[0];  dat[0] = dat[1];  dat[1] = tmp;
      dat += 2;
    }
  else if (len==4)
    while (cnt--) {
      tmp = dat[0];  dat[0] = dat[3];  dat[3] = tmp;
      tmp = dat[1];  dat[1] = dat[2];  dat[2] = tmp;
      dat += 4;
    }
  else if (len==8)
    while (cnt--) {
      tmp = dat[0];  dat[0] = dat[7];  dat[7] = tmp;
      tmp = dat[1];  dat[1] = dat[6];  dat[6] = tmp;
      tmp = dat[2];  dat[2] = dat[5];  dat[5] = tmp;
      tmp = dat[3];  dat[3] = dat[4];  dat[4] = tmp;
      dat += 8;
    }
  else {  /* the general SLOOOOOOOOOW case */
    for(k=0; k<len/2; k++) {
      tmp = dat[k];
      dat[k] = dat[len-1-k];
      dat[len-1-k] = tmp;
    }
  }
}

/*----------------------------------------------------------------------------*/
/*! \fn void ath_error(char *fmt, ...)
 *  \brief Terminate execution and output error message
 *  Uses variable-length argument lists provided in <stdarg.h>
 */

void ath_error(char *fmt, ...)
{
  va_list ap;
  FILE *atherr = atherr_fp();

  fprintf(atherr,"### Fatal error: ");   /* prefix */
  va_start(ap, fmt);              /* ap starts with string 'fmt' */
  vfprintf(atherr, fmt, ap);      /* print out on atherr */
  fflush(atherr);                 /* flush it NOW */
  va_end(ap);                     /* end varargs */

#ifdef MPI_PARALLEL
  MPI_Abort(MPI_COMM_WORLD, 1);
#endif

  exit(EXIT_FAILURE);
}

/*----------------------------------------------------------------------------*/
/*! \fn void minmax1(Real *data, int nx1, Real *dmino, Real *dmaxo)
 *  \brief Return the min and max of a 1D array using registers
 *  Works on data of type float, not Real.
 */

void minmax1(Real *data, int nx1, Real *dmino, Real *dmaxo)
{
  int i;
  register Real dmin, dmax;

  dmin = dmax = data[0];
  for (i=0; i<nx1; i++) {
    dmin = MIN(dmin,data[i]);
    dmax = MAX(dmax,data[i]);
  }
  *dmino = dmin;
  *dmaxo = dmax;
}

/*! \fn void minmax2(Real **data, int nx2, int nx1, Real *dmino, Real *dmaxo)
 *  \brief Return the min and max of a 2D array using registers
 *  Works on data of type float, not Real.
 */
void minmax2(Real **data, int nx2, int nx1, Real *dmino, Real *dmaxo)
{
  int i,j;
  register Real dmin, dmax;

  dmin = dmax = data[0][0];
  for (j=0; j<nx2; j++) {
    for (i=0; i<nx1; i++) {
      dmin = MIN(dmin,data[j][i]);
      dmax = MAX(dmax,data[j][i]);
    }
  }
  *dmino = dmin;
  *dmaxo = dmax;
}

/*! \fn void minmax3(Real ***data, int nx3, int nx2, int nx1, Real *dmino,
 *                   Real *dmaxo)
 *  \brief Return the min and max of a 3D array using registers
 *  Works on data of type float, not Real.
 */
void minmax3(Real ***data, int nx3, int nx2, int nx1, Real *dmino, Real *dmaxo)
{
  int i,j,k;
  register Real dmin, dmax;

  dmin = dmax = data[0][0][0];
  for (k=0; k<nx3; k++) {
    for (j=0; j<nx2; j++) {
      for (i=0; i<nx1; i++) {
        dmin = MIN(dmin,data[k][j][i]);
        dmax = MAX(dmax,data[k][j][i]);
      }
    }
  }
  *dmino = dmin;
  *dmaxo = dmax;
}

/*----------------------------------------------------------------------------*/
/*! \fn  void do_nothing_bc(GridS *pG)
 *
 *  \brief DOES ABSOLUTELY NOTHING!  THUS, WHATEVER THE BOUNDARY ARE SET TO
 *  INITIALLY, THEY REMAIN FOR ALL TIME.
 */
void do_nothing_bc(GridS *pG __attribute((unused)))
{
}

/*============================================================================
 * ERROR-ANALYSIS FUNCTIONS
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*! \fn Real compute_div_b(GridS *pG)
 *  \brief COMPUTE THE DIVERGENCE OF THE MAGNETIC FIELD USING FACE-CENTERED
 *  FIELDS OVER THE ENTIRE ACTIVE GRID.  RETURNS THE MAXIMUM OF |DIV B|.
 */
Real compute_div_b(GridS *pG)
{
#ifdef MHD
  int i,j,k,is,ie,js,je,ks,ke;
  Real x1,x2,x3,divB,maxdivB=0.0;
  Real lsf=1.0,rsf=1.0,dx2=pG->dx2;

  is = pG->is; ie = pG->ie;
  js = pG->js; je = pG->je;
  ks = pG->ks; ke = pG->ke;

  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je; j++) {
      for (i=is; i<=ie; i++) {
        cc_pos(pG,i,j,k,&x1,&x2,&x3);
#ifdef CYLINDRICAL
        rsf = (x1+0.5*pG->dx1)/x1;  lsf = (x1-0.5*pG->dx1)/x1;
        dx2 = x1*pG->dx2;
#endif
        divB = (rsf*pG->B1i[k][j][i+1] - lsf*pG->B1i[k][j][i])/pG->dx1;
        if (je > js)
          divB += (pG->B2i[k][j+1][i] - pG->B2i[k][j][i])/dx2;
        if (ke > ks)
          divB += (pG->B3i[k+1][j][i] - pG->B3i[k][j][i])/pG->dx3;

        maxdivB = MAX(maxdivB,fabs(divB));
      }
    }
  }

  return maxdivB;

#else
  fprintf(stderr,"[compute_div_b]: This only works for MHD!\n");
  exit(EXIT_FAILURE);
  return 0.0;
#endif /* MHD */
}

/*----------------------------------------------------------------------------*/
/*! \fn void compute_l1_error(const char *problem, const MeshS *pM,
 *                            const ConsS ***RootSoln, const int errortype)
 *  \brief COMPUTE THE L1-ERRORS IN ALL VARIABLES AT THE CURRENT
 *  (USUALLY THE FINAL)
 *  TIMESTEP USING THE INITIAL SOLUTION.
 *
 *  THIS MEANS THAT THE SOLUTION MUST
 *  EITHER BE STATIC (STEADY-STATE) OR MUST HAVE COMPLETED A FULL PERIOD OF
 *  ROTATION, ETC.  FOR THE ERRORTYPE FLAG, 0 MEANS ABSOLUTE ERROR, AND
 *  1 MEANS AVERAGE ERROR PER GRID CELL.
 */
void compute_l1_error(const char *problem, const MeshS *pM,
                      const ConsS ***RootSoln, const int errortype)
{
  DomainS *pD=&(pM->Domain[0][0]);
  GridS   *pG=pM->Domain[0][0].Grid;
  int i=0,j=0,k=0;
#if (NSCALARS > 0)
  int n;
#endif
  int is,ie,js,je,ks,ke;
  Real rms_error=0.0;
  Real dVol,totVol;
  ConsS error,total_error;
  FILE *fp;
  char *fname, fnamestr[256];
  int Nx1,Nx2,Nx3;
#if defined MPI_PARALLEL
  double err[8+NSCALARS], tot_err[8+NSCALARS];
  int mpi_err;
#endif
#ifdef CYLINDRICAL
  Real x1,x2,x3;
#endif

/* Clear out the total_error struct */
  memset(&total_error,0.0,sizeof(ConsS));
  if (pG == NULL) return;

/* compute L1 error in each variable, and rms total error */

  is = pG->is; ie = pG->ie;
  js = pG->js; je = pG->je;
  ks = pG->ks; ke = pG->ke;

  for (k=ks; k<=ke; k++) {
    for (j=js; j<=je; j++) {
      memset(&error,0.0,sizeof(ConsS));
      for (i=is; i<=ie; i++) {
        dVol = 1.0;
        if (pG->dx1 > 0.0) dVol *= pG->dx1;
        if (pG->dx2 > 0.0) dVol *= pG->dx2;
        if (pG->dx3 > 0.0) dVol *= pG->dx3;
#ifdef CYLINDRICAL
        cc_pos(pG,i,j,k,&x1,&x2,&x3);
        dVol *= x1;
#endif

/* Sum local L1 error for each grid cell I own */
        error.d   += dVol*fabs(pG->U[k][j][i].d   - RootSoln[k][j][i].d );
        error.M1  += dVol*fabs(pG->U[k][j][i].M1  - RootSoln[k][j][i].M1);
        error.M2  += dVol*fabs(pG->U[k][j][i].M2  - RootSoln[k][j][i].M2);
        error.M3  += dVol*fabs(pG->U[k][j][i].M3  - RootSoln[k][j][i].M3);
#ifdef MHD
        error.B1c += dVol*fabs(pG->U[k][j][i].B1c - RootSoln[k][j][i].B1c);
        error.B2c += dVol*fabs(pG->U[k][j][i].B2c - RootSoln[k][j][i].B2c);
        error.B3c += dVol*fabs(pG->U[k][j][i].B3c - RootSoln[k][j][i].B3c);
#endif /* MHD */
#ifndef ISOTHERMAL
        error.E   += dVol*fabs(pG->U[k][j][i].E   - RootSoln[k][j][i].E );
#endif /* ISOTHERMAL */
#if (NSCALARS > 0)
        for (n=0; n<NSCALARS; n++)
          error.s[n] += dVol*fabs(pG->U[k][j][i].s[n] - RootSoln[k][j][i].s[n]);
#endif
      }

/* total_error is sum of local L1 error */
      total_error.d += error.d;
      total_error.M1 += error.M1;
      total_error.M2 += error.M2;
      total_error.M3 += error.M3;
#ifdef MHD
      total_error.B1c += error.B1c;
      total_error.B2c += error.B2c;
      total_error.B3c += error.B3c;
#endif /* MHD */
#ifndef ISOTHERMAL
      total_error.E += error.E;
#endif /* ISOTHERMAL */
#if (NSCALARS > 0)
      for (n=0; n<NSCALARS; n++) total_error.s[n] += error.s[n];
#endif
    }
  }

#ifdef MPI_PARALLEL
/* Now we have to use an All_Reduce to get the total error over all the MPI
 * grids.  Begin by copying the error into the err[] array */

  err[0] = total_error.d;
  err[1] = total_error.M1;
  err[2] = total_error.M2;
  err[3] = total_error.M3;
#ifdef MHD
  err[4] = total_error.B1c;
  err[5] = total_error.B2c;
  err[6] = total_error.B3c;
#endif /* MHD */
#ifndef ISOTHERMAL
  err[7] = total_error.E;
#endif /* ISOTHERMAL */
#if (NSCALARS > 0)
  for (n=0; n<NSCALARS; n++) err[8+n] = total_error.s[n];
#endif

/* Sum up the Computed Error */
  mpi_err = MPI_Reduce(err, tot_err, (8+NSCALARS),
                       MPI_DOUBLE, MPI_SUM, 0, pD->Comm_Domain);
  if(mpi_err)
    ath_error("[compute_l1_error]: MPI_Reduce call returned error = %d\n",
              mpi_err);

/* If I'm the parent, copy the sum back to the total_error variable */
  if(pD->DomNumber == 0){ /* I'm the parent */
    total_error.d   = tot_err[0];
    total_error.M1  = tot_err[1];
    total_error.M2  = tot_err[2];
    total_error.M3  = tot_err[3];
#ifdef MHD
    total_error.B1c = tot_err[4];
    total_error.B2c = tot_err[5];
    total_error.B3c = tot_err[6];
#endif /* MHD */
#ifndef ISOTHERMAL
    total_error.E   = tot_err[7];
#endif /* ISOTHERMAL */
#if (NSCALARS > 0)
    for (n=0; n<NSCALARS; n++) total_error.s[n] = err[8+n];
#endif

  }
  else return; /* The child grids do not do any of the following code */
#endif /* MPI_PARALLEL */

/* Compute total number of grid cells */
  Nx1 = pD->Nx[0];
  Nx2 = pD->Nx[1];
  Nx3 = pD->Nx[2];

  totVol = 1.0;
  if (errortype == 1) {
    if (pD->MaxX[0] > pD->MinX[0]) totVol *= pD->MaxX[0] - pD->MinX[0];
    if (pD->MaxX[1] > pD->MinX[1]) totVol *= pD->MaxX[1] - pD->MinX[1];
    if (pD->MaxX[2] > pD->MinX[2]) totVol *= pD->MaxX[2] - pD->MinX[2];
#ifdef CYLINDRICAL
    totVol *= 0.5*(pD->MinX[0] + pD->MaxX[0]);
#endif
  }


/* Compute RMS error over all variables, and print out */

  rms_error = SQR(total_error.d) + SQR(total_error.M1) + SQR(total_error.M2)
    + SQR(total_error.M3);
#ifdef MHD
  rms_error += SQR(total_error.B1c) + SQR(total_error.B2c)
    + SQR(total_error.B3c);
#endif /* MHD */
#ifndef ISOTHERMAL
  rms_error += SQR(total_error.E);
#endif /* ISOTHERMAL */

  rms_error = sqrt(rms_error)/totVol;

/* Print error to file "BLAH-errors.#.dat"  */
  sprintf(fnamestr,"%s-errors",problem);
  fname = ath_fname(NULL,fnamestr,NULL,NULL,1,0,NULL,"dat");

/* The file exists -- reopen the file in append mode */
  if((fp=fopen(fname,"r")) != NULL){
    if((fp = freopen(fname,"a",fp)) == NULL){
      ath_error("[compute_l1_error]: Unable to reopen file.\n");
      free(fname);
      return;
    }
  }
/* The file does not exist -- open the file in write mode */
  else{
    if((fp = fopen(fname,"w")) == NULL){
      ath_error("[compute_l1_error]: Unable to open file.\n");
      free(fname);
      return;
    }
/* Now write out some header information */
    fprintf(fp,"# Nx1  Nx2  Nx3  RMS-Error  d  M1  M2  M3");
#ifndef ISOTHERMAL
    fprintf(fp,"  E");
#endif /* ISOTHERMAL */
#ifdef MHD
    fprintf(fp,"  B1c  B2c  B3c");
#endif /* MHD */
#if (NSCALARS > 0)
    for (n=0; n<NSCALARS; n++) {
      fprintf(fp,"  S[ %d ]",n);
    }
#endif
    fprintf(fp,"\n#\n");
  }

  fprintf(fp,"%d  %d  %d  %e",Nx1,Nx2,Nx3,rms_error);

  fprintf(fp,"  %e  %e  %e  %e",
          (total_error.d /totVol),
          (total_error.M1/totVol),
          (total_error.M2/totVol),
          (total_error.M3/totVol));

#ifndef ISOTHERMAL
  fprintf(fp,"  %e",total_error.E/totVol);
#endif /* ISOTHERMAL */

#ifdef MHD
  fprintf(fp,"  %e  %e  %e",
          (total_error.B1c/totVol),
          (total_error.B2c/totVol),
          (total_error.B3c/totVol));
#endif /* MHD */
#if (NSCALARS > 0)
  for (n=0; n<NSCALARS; n++) {
    fprintf(fp,"  %e",total_error.s[n]/totVol);
  }
#endif

  fprintf(fp,"\n");

  fclose(fp);
  free(fname);

  return;
}

/*============================================================================
 * ROOT-FINDING FUNCTIONS
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*! \fn int sign_change(Real (*func)(const Real,const Real), const Real a0,
 *                      const Real b0, const Real x, Real *a, Real *b)
 *  \brief SEARCH FOR A SIGN CHANGE.
 *
 *  THIS FUNCTION PARTITIONS THE INTERVAL (a0,b0) INTO
 *  2^k EQUALLY SPACED GRID POINTS, EVALUATES THE FUNCTION f AT THOSE POINTS,
 *  AND THEN SEARCHES FOR A SIGN CHANGE IN f BETWEEN ADJACENT GRID POINTS.  THE
 *  FIRST SUCH INTERVAL FOUND, (a,b), IS RETURNED.
 */
int sign_change(Real (*func)(const Real,const Real), const Real a0,
                const Real b0, const Real x, Real *a, Real *b) {
  const int kmax=20;
  int k, n, i;
  Real delta, fk, fkp1;

  for (k=1; k<=kmax; k++) {
    n = pow(2,k);
    delta = (b0-a0)/(n-1);
    *a = a0;
    fk = func(x,*a);
    for (i=1; i<n; i++) {
      *b = *a + delta;
      fkp1 = func(x,*b);
      if (fkp1*fk < 0)
        return 1;
      *a = *b;
      fk = fkp1;
    }
  }
/*   ath_error("[sign_change]: No sign change was detected in (%f,%f) for x=%f!\n",a0,b0,x); */
  return 0;
}


/*----------------------------------------------------------------------------*/
/*! \fn int bisection(Real (*func)(const Real,const Real), const Real a0,
 *                    const Real b0, const Real x, Real *root)
 *  \brief THIS FUNCTION IMPLEMENTS THE BISECTION METHOD FOR ROOT FINDING.
 */
int bisection(Real (*func)(const Real,const Real), const Real a0, const Real b0,
              const Real x, Real *root)
{
  const Real tol = 1.0E-10;
  const int maxiter = 400;
  Real a=a0, b=b0, c, fa, fb, fc;
  int i;

  fa = func(x,a);
  fb = func(x,b);
  if (fabs(fa) < tol) {
    *root = a;
    return 1;
  }
  if (fabs(fb) < tol) {
    *root = b;
    return 1;
  }
/* printf("fa = %f, fb = %f\n", fa, fb); */

  for (i = 0; i < maxiter; i++) {
    c = 0.5*(a+b);
/* printf("x = %f, a = %f, b = %f, c = %f\n", x,a,b,c); */
#ifdef MYDEBUG
    printf("c = %f\n", c);
#endif
    if (fabs((b-a)/c) < tol) {
#ifdef MYDEBUG
      printf("Bisection converged within tolerance of %f!\n", eps);
#endif
      *root = c;
      return 1;
    }
    fc = func(x,c);
    if (fa*fc < 0) {
      b = c;
      fb = fc;
    }
    else if (fc*fb < 0) {
      a = c;
      fa = fc;
    }
    else if (fc == 0) {
      *root = c;
      return 1;
    }
    else {
      ath_error("[bisection]:  There is no single root in (%f,%f) for x = %13.10f!!\n", a, b,x);
      *root = c;
      return 0;
    }
  }

  ath_error("[bisection]:  Bisection did not converge in %d iterations for x = %13.10f!!\n", maxiter,x);
  *root = c;
  return 0;
}


/*============================================================================
 * QUADRATURE FUNCTIONS
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*! \fn Real trapzd(Real (*func)(Real), const Real a, const Real b, const int n,
 *                  const Real s)
 *  \brief THIS ROUTINE COMPUTES THE nTH STAGE OF REFINEMENT OF AN EXTENDED
 *  TRAPEZOIDAL RULE.
 *
 * func IS INPUT AS A POINTER TO THE FUNCTION TO BE INTEGRATED BETWEEN
 * LIMITS a AND b, ALSO INPUT. WHEN CALLED WITH n=1, THE ROUTINE RETURNS THE
 * CRUDEST ESTIMATE OF \int_a^b f(R) R dR.  SUBSEQUENT CALLS WITH n=2,3,...
 * (IN THAT SEQUENTIAL ORDER) WILL IMPROVE THE ACCURACY BY ADDING 2n-2
 * ADDITIONAL INTERIOR POINTS.
 * ADAPTED FROM NUMERICAL RECIPES BY AARON SKINNER
 */
Real trapzd(Real (*func)(Real), const Real a, const Real b, const int n,
            const Real s)
{
  Real x,tnm,sum,dx;
  int it,j;

  if (n == 1) {
    return 0.5*(b-a)*(func(a)+func(b));
  }
  else {
    for (it=1,j=1; j<n-1; j++) it <<= 1;  /* it = 2^(n-2) */
    tnm = it;
    dx = (b-a)/tnm;  /* THIS IS THE SPACING OF THE POINTS TO BE ADDED. */
    x = a + 0.5*dx;
    for (sum=0.0,j=1; j<=it; j++,x+=dx) sum += func(x);
    return 0.5*(s+(b-a)*sum/tnm);  /* THIS REPLACES s BY ITS REFINED VALUE. */
  }
}

/*----------------------------------------------------------------------------*/
/*! \fn Real qsimp(Real (*func)(Real), const Real a, const Real b)
 *  \brief RETURNS THE INTEGRAL OF THE FUNCTION func FROM a TO b.
 *
 * THE PARAMETER EPS
 * CAN BE SET TO THE DESIRED FRACTIONAL ACCURACY AND JMAX SO THAT 2^(JMAX-1)
 * IS THE MAXIMUM ALLOWED NUMBER OF STEPS.  INTEGRATION IS PERFORMED BY
 * SIMPSON'S RULE.
 * ADAPTED FROM NUMERICAL RECIPES BY AARON SKINNER
 */

#define EPS 1.0e-8
#define JMAX 20

Real qsimp(Real (*func)(Real), const Real a, const Real b)
{
  int j;
  Real s,st,ost,os;

  ost = os = -1.0e30;
  for (j=1; j<=JMAX; j++) {
    st = trapzd(func,a,b,j,ost);
    s = (4.0*st-ost)/3.0;  /* EQUIVALENT TO SIMPSON'S RULE */
    if (j > 5)  /* AVOID SPURIOUS EARLY CONVERGENCE. */
      if (fabs(s-os) < EPS*fabs(os) || (s == 0.0 && os == 0.0)) return s;
    os=s;
    ost=st;
  }

  ath_error("[qsimp]:  Too many steps!\n");
  return 0.0;
}


/*----------------------------------------------------------------------------*/
/* FUNCTION avg1d,avg2d,avg3d
 *
 * RETURNS THE INTEGRAL OF A USER-SUPPLIED FUNCTION func OVER THE ONE-, TWO-,
 * OR THREE-DIMENSIONAL GRID CELL (i,j,k).  INTEGRATION IS PERFORMED USING qsimp.
 * ADAPTED FROM NUMERICAL RECIPES BY AARON SKINNER
 */
static Real xsav,ysav,zsav,xmin,xmax,ymin,ymax,zmin,zmax;
static Real (*nrfunc)(Real,Real,Real);

/*! \fn Real avg1d(Real (*func)(Real, Real, Real), const GridS *pG,
 *                 const int i, const int j, const int k)
 *  \brief RETURNS THE INTEGRAL OF A USER-SUPPLIED FUNCTION func OVER THE ONE-
 * DIMENSIONAL GRID CELL (i,j,k).
 *
 * INTEGRATION IS PERFORMED USING qsimp.
 * ADAPTED FROM NUMERICAL RECIPES BY AARON SKINNER
 */
Real avg1d(Real (*func)(Real, Real, Real), const GridS *pG,
           const int i, const int j, const int k)
{
  Real x1,x2,x3,dvol=pG->dx1;
  Real fx(Real x);

  nrfunc=func;
  cc_pos(pG,i,j,k,&x1,&x2,&x3);
  xmin = x1 - 0.5*pG->dx1;  xmax = x1 + 0.5*pG->dx1;

  ysav = x2;
  zsav = x3;
#ifdef CYLINDRICAL
  dvol *= x1;
#endif

  return qsimp(fx,xmin,xmax)/dvol;
}

/*! \fn Real avg2d(Real (*func)(Real, Real, Real), const GridS *pG,
 *                 const int i, const int j, const int k)
 *  \brief RETURNS THE INTEGRAL OF A USER-SUPPLIED FUNCTION func OVER THE TWO-
 * DIMENSIONAL GRID CELL (i,j,k).
 *
 * INTEGRATION IS PERFORMED USING qsimp.
 * ADAPTED FROM NUMERICAL RECIPES BY AARON SKINNER
 */
Real avg2d(Real (*func)(Real, Real, Real), const GridS *pG,
           const int i, const int j, const int k)
{
  Real x1,x2,x3,dvol=pG->dx1*pG->dx2;
  Real fy(Real y);

  nrfunc=func;
  cc_pos(pG,i,j,k,&x1,&x2,&x3);
  xmin = x1 - 0.5*pG->dx1;  xmax = x1 + 0.5*pG->dx1;
  ymin = x2 - 0.5*pG->dx2;  ymax = x2 + 0.5*pG->dx2;

  zsav = x3;
#ifdef CYLINDRICAL
  dvol *= x1;
#endif

  return qsimp(fy,ymin,ymax)/dvol;
}

/*! \fn Real avg3d(Real (*func)(Real, Real, Real), const GridS *pG,
 *                 const int i, const int j, const int k)
 *  \brief RETURNS THE INTEGRAL OF A USER-SUPPLIED FUNCTION func OVER THE
 * THREE-DIMENSIONAL GRID CELL (i,j,k).
 *
 * INTEGRATION IS PERFORMED USING qsimp.
 * ADAPTED FROM NUMERICAL RECIPES BY AARON SKINNER
 */
Real avg3d(Real (*func)(Real, Real, Real), const GridS *pG,
           const int i, const int j, const int k)
{
  Real x1,x2,x3,dvol=pG->dx1*pG->dx2*pG->dx3;
  Real fz(Real z);

  nrfunc=func;
  cc_pos(pG,i,j,k,&x1,&x2,&x3);
  xmin = x1 - 0.5*pG->dx1;  xmax = x1 + 0.5*pG->dx1;
  ymin = x2 - 0.5*pG->dx2;  ymax = x2 + 0.5*pG->dx2;
  zmin = x3 - 0.5*pG->dx3;  zmax = x3 + 0.5*pG->dx3;

#ifdef CYLINDRICAL
  dvol *= x1;
#endif

  return qsimp(fz,zmin,zmax)/dvol;
}

Real avgXZ(Real (*func)(Real, Real, Real), const GridS *pG, const int i, const int j, const int k) {
  Real x1,x2,x3;

  Real fXZ(Real z);

  nrfunc=func;
  cc_pos(pG,i,j,k,&x1,&x2,&x3);
  xmin = x1 - 0.5*pG->dx1;  xmax = x1 + 0.5*pG->dx1;
  zmin = x3 - 0.5*pG->dx3;  zmax = x3 + 0.5*pG->dx3;

  ysav = x2;
  return qsimp(fXZ,zmin,zmax)/(x1*pG->dx1*pG->dx3);

}

Real fz(Real z)
{
  Real fy(Real y);

  zsav = z;
  return qsimp(fy,ymin,ymax);
}

Real fy(Real y)
{
  Real fx(Real x);

  ysav = y;
  return qsimp(fx,xmin,xmax);
}

Real fx(Real x)
{
#ifdef CYLINDRICAL
  return x*nrfunc(x,ysav,zsav);
#else
  return nrfunc(x,ysav,zsav);
#endif
}

Real fXZ(Real z) {
  Real fx(Real x);

  zsav = z;
  return qsimp(fx,xmin,xmax);

}

/*----------------------------------------------------------------------------*/
/* FUNCTION vecpot2b1i,vecpot2b2i,vecpot2b3i
 *
 * THESE FUNCTIONS COMPUTE MAGNETIC FIELD COMPONENTS FROM COMPONENTS OF A
 * SPECIFIED VECTOR POTENTIAL USING STOKES' THEOREM AND SIMPSON'S QUADRATURE.
 * NOTE:  THIS IS ONLY GUARANTEED TO WORK IF THE POTENTIAL IS OF CLASS C^1.
 * WRITTEN BY AARON SKINNER.
 */

static Real (*a1func)(Real,Real,Real);
static Real (*a2func)(Real,Real,Real);
static Real (*a3func)(Real,Real,Real);

/*! \fn Real vecpot2b1i(Real (*A2)(Real,Real,Real), Real (*A3)(Real,Real,Real),
 *                const GridS *pG, const int i, const int j, const int k)
 *  \brief Compute B-field components from a vector potential.
 *
 * THESE FUNCTIONS COMPUTE MAGNETIC FIELD COMPONENTS FROM COMPONENTS OF A
 * SPECIFIED VECTOR POTENTIAL USING STOKES' THEOREM AND SIMPSON'S QUADRATURE.
 * NOTE:  THIS IS ONLY GUARANTEED TO WORK IF THE POTENTIAL IS OF CLASS C^1.
 * WRITTEN BY AARON SKINNER.
 */
Real vecpot2b1i(Real (*A2)(Real,Real,Real), Real (*A3)(Real,Real,Real),
                const GridS *pG, const int i, const int j, const int k)
{
  Real x1,x2,x3,b1i=0.0,lsf=1.0,rsf=1.0,dx2=pG->dx2;
  Real f2(Real y);
  Real f3(Real z);

  a2func = A2;
  a3func = A3;
  cc_pos(pG,i,j,k,&x1,&x2,&x3);
  xmin = x1 - 0.5*pG->dx1;  xmax = x1 + 0.5*pG->dx1;
  ymin = x2 - 0.5*pG->dx2;  ymax = x2 + 0.5*pG->dx2;
  zmin = x3 - 0.5*pG->dx3;  zmax = x3 + 0.5*pG->dx3;

  xsav = xmin;
#ifdef CYLINDRICAL
  lsf = xmin;  rsf = xmin;
  dx2 = xmin*pG->dx2;
#endif

  if (A2 != NULL) {
    if (ymin == ymax)
      b1i += rsf*A2(xmin,ymin,zmin) - lsf*A2(xmin,ymin,zmax);
    else {
      zsav = zmin;
      b1i += rsf*qsimp(f2,ymin,ymax);
      zsav = zmax;
      b1i -= lsf*qsimp(f2,ymin,ymax);
    }
  }
  if (A3 != NULL) {
    if (zmin == zmax)
      b1i += A3(xmin,ymax,zmin) - A3(xmin,ymin,zmin);
    else {
      ysav = ymax;
      b1i += qsimp(f3,zmin,zmax);
      ysav = ymin;
      b1i -= qsimp(f3,zmin,zmax);
    }
  }

  if (pG->dx2 > 0.0) b1i /= dx2;
  if (pG->dx3 > 0.0) b1i /= pG->dx3;

  return b1i;
}

/*! \fn Real vecpot2b2i(Real (*A1)(Real,Real,Real), Real (*A3)(Real,Real,Real),
 *                const GridS *pG, const int i, const int j, const int k)
 *  \brief Compute B-field components from a vector potential.
 *
 * THESE FUNCTIONS COMPUTE MAGNETIC FIELD COMPONENTS FROM COMPONENTS OF A
 * SPECIFIED VECTOR POTENTIAL USING STOKES' THEOREM AND SIMPSON'S QUADRATURE.
 * NOTE:  THIS IS ONLY GUARANTEED TO WORK IF THE POTENTIAL IS OF CLASS C^1.
 * WRITTEN BY AARON SKINNER.
 */
Real vecpot2b2i(Real (*A1)(Real,Real,Real), Real (*A3)(Real,Real,Real),
                const GridS *pG, const int i, const int j, const int k)
{
  Real x1,x2,x3,b2i=0.0;
  Real f1(Real x);
  Real f3(Real z);

  a1func = A1;
  a3func = A3;
  cc_pos(pG,i,j,k,&x1,&x2,&x3);
  xmin = x1 - 0.5*pG->dx1;  xmax = x1 + 0.5*pG->dx1;
  ymin = x2 - 0.5*pG->dx2;  ymax = x2 + 0.5*pG->dx2;
  zmin = x3 - 0.5*pG->dx3;  zmax = x3 + 0.5*pG->dx3;

  ysav = ymin;

  if (A1 != NULL) {
    if (xmin == xmax)
      b2i += A1(xmin,ymin,zmax) - A1(xmin,ymin,zmin);
    else {
      zsav = zmax;
      b2i += qsimp(f1,xmin,xmax);
      zsav = zmin;
      b2i -= qsimp(f1,xmin,xmax);
    }
  }
  if (A3 != NULL) {
    if (zmin == zmax)
      b2i += A3(xmin,ymin,zmin) - A3(xmax,ymin,zmin);
    else {
      xsav = xmin;
      b2i += qsimp(f3,zmin,zmax);
      xsav = xmax;
      b2i -= qsimp(f3,zmin,zmax);
    }
  }

  if (pG->dx1 > 0.0) b2i /= pG->dx1;
  if (pG->dx3 > 0.0) b2i /= pG->dx3;

  return b2i;
}

/*! \fn Real vecpot2b3i(Real (*A1)(Real,Real,Real), Real (*A2)(Real,Real,Real),
 *                const GridS *pG, const int i, const int j, const int k)
 *  \brief Compute B-field components from a vector potential.
 *
 * THESE FUNCTIONS COMPUTE MAGNETIC FIELD COMPONENTS FROM COMPONENTS OF A
 * SPECIFIED VECTOR POTENTIAL USING STOKES' THEOREM AND SIMPSON'S QUADRATURE.
 * NOTE:  THIS IS ONLY GUARANTEED TO WORK IF THE POTENTIAL IS OF CLASS C^1.
 * WRITTEN BY AARON SKINNER.
 */
Real vecpot2b3i(Real (*A1)(Real,Real,Real), Real (*A2)(Real,Real,Real),
                const GridS *pG, const int i, const int j, const int k)
{
  Real x1,x2,x3,b3i=0.0,lsf=1.0,rsf=1.0,dx2=pG->dx2;
  Real f1(Real x);
  Real f2(Real y);

  a1func = A1;
  a2func = A2;
  cc_pos(pG,i,j,k,&x1,&x2,&x3);
  xmin = x1 - 0.5*pG->dx1;  xmax = x1 + 0.5*pG->dx1;
  ymin = x2 - 0.5*pG->dx2;  ymax = x2 + 0.5*pG->dx2;
  zmin = x3 - 0.5*pG->dx3;  zmax = x3 + 0.5*pG->dx3;

  zsav = zmin;
#ifdef CYLINDRICAL
  rsf = xmax;  lsf = xmin;
  dx2 = x1*pG->dx2;
#endif

  if (A1 != NULL) {
    if (xmin == xmax)
      b3i += A1(xmin,ymin,zmin) - A1(xmin,ymax,zmin);
    else {
      ysav = ymin;
      b3i += qsimp(f1,xmin,xmax);
      ysav = ymax;
      b3i -= qsimp(f1,xmin,xmax);
    }
  }
  if (A2 != NULL) {
    if (ymin == ymax)
      b3i += rsf*A2(xmax,ymin,zmin) - lsf*A2(xmin,ymin,zmin);
    else {
      xsav = xmax;
      b3i += rsf*qsimp(f2,ymin,ymax);
      xsav = xmin;
      b3i -= lsf*qsimp(f2,ymin,ymax);
    }
  }

  if (pG->dx1 > 0.0) b3i /= pG->dx1;
  if (pG->dx2 > 0.0) b3i /= dx2;

  return b3i;
}

Real f1(Real x)
{
  return a1func(x,ysav,zsav);
}

Real f2(Real y)
{
  return a2func(xsav,y,zsav);
}

Real f3(Real z)
{
  return a3func(xsav,ysav,z);
}


#if defined(PARTICLES) || defined(CHEMISTRY)
/*----------------------------------------------------------------------------*/
/*! \fn void ludcmp(Real **a, int n, int *indx, Real *d)
 *  \brief LU decomposition from Numerical Recipes
 *
 * Using Crout's method with partial pivoting
 * a is the input matrix, and is returned with LU decomposition readily made,
 * n is the matrix size, indx records the history of row permutation,
 * whereas d =1(-1) for even(odd) number of permutations.
 */
void ludcmp(Real **a, int n, int *indx, Real *d)
{
  int i,imax,j,k;
  Real big,dum,sum,temp;
  Real *rowscale;  /* the implicit scaling of each row */

  rowscale = (Real*)calloc_1d_array(n, sizeof(Real));
  *d=1.0;  /* No row interchanges yet */

  for (i=0;i<n;i++)
    { /* Loop over rows to get the implicit scaling information */
      big=0.0;
      for (j=0;j<n;j++)
        if ((temp=fabs(a[i][j])) > big) big=temp;
      if (big == 0.0) ath_error("[LUdecomp]:Input matrix is singular!");
      rowscale[i]=1.0/big;  /* Save the scaling */
    }

  for (j=0;j<n;j++) { /* Loop over columns of Crout's method */
/* Calculate the upper block */
    for (i=0;i<j;i++) {
      sum=a[i][j];
      for (k=0;k<i;k++) sum -= a[i][k]*a[k][j];
      a[i][j]=sum;
    }
/* Calculate the lower block (first step) */
    big=0.0;
    for (i=j;i<n;i++) {
      sum=a[i][j];
      for (k=0;k<j;k++)
        sum -= a[i][k]*a[k][j];
      a[i][j]=sum;
/* search for the largest pivot element */
      if ( (dum=rowscale[i]*fabs(sum)) >= big) {
        big=dum;
        imax=i;
      }
    }
/* row interchange */
    if (j != imax) {
      for (k=0;k<n;k++) {
        dum=a[imax][k];
        a[imax][k]=a[j][k];
        a[j][k]=dum;
      }
      *d = -(*d);
      rowscale[imax]=rowscale[j];
    }
    indx[j]=imax; /* record row interchange history */
/* Calculate the lower block (second step) */
    if (a[j][j] == 0.0) a[j][j]=TINY_NUMBER;
    dum=1.0/(a[j][j]);
    for (i=j+1;i<n;i++) a[i][j] *= dum;
  }
  free(rowscale);
}

/*----------------------------------------------------------------------------*/
/*! \fn void lubksb(Real **a, int n, int *indx, Real b[])
 *  \brief Backward substitution (from numerical recipies)
 *
 * a is the input matrix done with LU decomposition, n is the matrix size
 * indx id the history of row permutation
 * b is the vector on the right (AX=b), and is returned with the solution
 */
void lubksb(Real **a, int n, int *indx, Real b[])
{
  int i,ii=-1,ip,j;
  Real sum;
/* Solve L*y=b */
  for (i=0;i<n;i++) {
    ip=indx[i];
    sum=b[ip];
    b[ip]=b[i];
    if (ii>=0)
      for (j=ii;j<=i-1;j++) sum -= a[i][j]*b[j];
    else if (sum) ii=i;
    b[i]=sum;
  }
/* Solve U*x=y */
  for (i=n-1;i>=0;i--) {
    sum=b[i];
    for (j=i+1;j<n;j++) sum -= a[i][j]*b[j];
    b[i]=sum/a[i][i];
  }
}

/*----------------------------------------------------------------------------*/
/*! \fn void InverseMatrix(Real **a, int n, Real **b)
 *  \brief Inverse matrix solver
 *
 * a: input matrix; n: matrix size, b: return matrix
 * Note: the input matrix will be DESTROYED
 */
void InverseMatrix(Real **a, int n, Real **b)
{
  int i,j,*indx;
  Real *col,d;

  indx = (int*)calloc_1d_array(n, sizeof(int));
  col = (Real*)calloc_1d_array(n, sizeof(Real));

  ludcmp(a,n,indx,&d);

  for (j=0; j<n; j++) {
    for (i=0; i<n; i++) col[i]=0.0;
    col[j]=1.0;
    lubksb(a, n, indx, col);
    for (i=0; i<n; i++)    b[i][j] = col[i];
  }

  return;
}

/*----------------------------------------------------------------------------*/
/*! \fn void MatrixMult(Real **a, Real **b, int m, int n, int l, Real **c)
 *  \brief Matrix multiplication: a(m*n) * b(n*l) = c(m*l) */
void MatrixMult(Real **a, Real **b, int m, int n, int l, Real **c)
{
  int i, j, k;
  for (i=0; i<m; i++)
    for (j=0; j<l; j++)
      {
        c[i][j] = 0.0;
        for (k=0; k<n; k++) c[i][j] += a[i][k] * b[k][j];
      }
}
#endif /* PARTICLES or CHEMISTRY */






#if defined (RADIATION_HYDRO) || defined (RADIATION_MHD)
/*----------------------------------------------------------------------------*/
/*
 *   Input Arguments:
 *     W = Primitive variable
 *    The effective sound speed is calculated as conserved variable formula
 */

Real eff_sound(const Prim1DS W, Real dt, int flag)
{
/* All rad hydro components of W should be updated before this function is called */
/* flag is used to decide whether 1D or multi-D. *
 * In 1D, we use effective sound speed. *
 * In multi-D, we use adiabatic sound speed *
 */

  Real aeff, temperature, SPP, Alpha;
  Real SVV, beta;
/*      Real dSigma[2*NOPACITY],dSigmadP[NOPACITY];
 */
  Real velocity_x, velocity_y, velocity_z, velocity;
  int i;
  if(flag == 1){
/*
  for(i=0; i<2*NOPACITY; i++)
  dSigma[i] = 0.0;
*/

    temperature = W.P / (W.d * R_ideal);
    velocity_x = W.Vx;
    velocity_y = W.Vy;
    velocity_z = W.Vz;

    velocity = velocity_x * velocity_x + velocity_y * velocity_y + velocity_z * velocity_z;

/*      if(Opacity != NULL) Opacity(W.d, temperature, NULL, dSigma);

        dSigmadP[0] =  dSigma[4] / (W.d * R_ideal);
        dSigmadP[1] =  dSigma[5] / (W.d * R_ideal);
        dSigmadP[2] =  dSigma[6] / (W.d * R_ideal);
        dSigmadP[3] =  dSigma[7] / (W.d * R_ideal);
*/

/*      SPP = -4.0 * (Gamma - 1.0) * Prat * Crat * W.Sigma[2] * temperature * temperature * temperature / (W.d * R_ideal)
        -(Gamma - 1.0) * Prat * Crat * (dSigmadP[2]  * pow(temperature,4.0) - dSigmadP[3] * W.Er)
        -(Gamma - 1.0) * Prat * 2.0 * dSigmadP[1] * (
        velocity_x * (W.Fr1 - ((1.0 + W.Edd_11) * velocity_x + W.Edd_21 * velocity_y + W.Edd_31 * velocity_z) * W.Er/Crat)
        +  velocity_y * (W.Fr2 - (W.Edd_21 * velocity_x + (1.0 + W.Edd_22) * velocity_y + W.Edd_32 * velocity_z) * W.Er/Crat)
        +  velocity_z * (W.Fr3 - (W.Edd_31 * velocity_x + W.Edd_32 * velocity_y + (1.0 + W.Edd_33) * velocity_z) * W.Er/Crat)
        );
*/

    SPP = -4.0 * (Gamma - 1.0) * Prat * Crat * W.Sigma[2] * temperature * temperature * temperature * (1.0 - velocity/(Crat * Crat)) / (W.d * R_ideal);


    if(fabs(SPP * dt * 0.5) > 0.001)
      Alpha = (exp(SPP * dt * 0.5) - 1.0)/(SPP * dt * 0.5);
    else
      Alpha = 1.0 + 0.25 * SPP * dt;

/* In case SPP * dt  is small, use expansion expression */

/* In case velocity is close to speed of light or very large optical depth.
 * It is important to include momentum stiff source term
 */

/*      velocity = sqrt(velocity_x * velocity_x + velocity_y * velocity_y + velocity_z * velocity_z);
 */

/* Eddington tensor is assumed 1/3 here for simplicity */

    SVV = -Prat * (W.Sigma[0] + W.Sigma[1]) * (1.0 + 1.0/3.0) * W.Er / (W.d * Crat);

    if(fabs(SVV * dt * 0.5) > 0.001)
      beta = (exp(SVV * dt * 0.5) - 1.0)/(SVV * dt * 0.5);
    else
      beta = 1.0 + 0.25 * SVV * dt;

/* In case SPP * dt  is small, use expansion expression */



    aeff = beta * ((Gamma - 1.0) * Alpha + 1.0) * W.P / W.d;

    aeff = sqrt(aeff);

  }
  else{

    aeff = sqrt(Gamma * W.P / W.d);

  }
  return aeff;
}

/* This is used to limit the time step to make modified Godunov step stable */
/* This function is not used right now. Only left here for future reference */
Real eff_sound_thick(const Prim1DS W, Real dt)
{

  Real aeff1, aeff2, SFFr, Edd;
  Real alpha, DetT, DetTrho, DetTE, temperature;
  Real root1, root2;
  Real coefa, coefb, coefc, coefd, coefe, coefh, coefr;

  SFFr = -Crat * (W.Sigma[0] + W.Sigma[1]);

/* Find the maximum Eddington factor to make it safe on either direction*/
  Edd = W.Edd_11;
  if(W.Edd_22 > Edd) Edd = W.Edd_22;
  else if(W.Edd_33 > Edd) Edd = W.Edd_33;

  alpha = (exp(SFFr * dt * 0.5) - 1.0)/(SFFr * dt * 0.5);

  temperature = W.P / (W.d * R_ideal);

  DetT = 1.0 + 4.0 * Prat * pow(temperature, 3.0) * (Gamma - 1.0) / (W.d * R_ideal);

  DetTrho = -W.P / (W.d * W.d * R_ideal * DetT);

  DetTE = (Gamma - 1.0) / (W.d * R_ideal * DetT);

  coefa = 4.0 * Prat * pow(temperature, 3.0) * (Edd + 1.0 - Gamma) * DetTrho;

  coefb = Gamma - 1.0 + 4.0 * Prat * pow(temperature, 3.0) * (Edd + 1.0 - Gamma) * DetTE;

  coefr = -Prat / Crat;

  coefc = Gamma * W.P * alpha / ((Gamma - 1.0) * W.d);

  coefd = Prat * Crat * alpha;

  coefe = 4.0 * Crat * Edd * pow(temperature, 3.0) * DetTrho;

  coefh = 4.0 * Crat * Edd * pow(temperature, 3.0) * DetTE;

  root1 = coefa + coefb * coefc + coefd * coefh + coefe * coefr + coefc * coefh * coefr;
  root2 = 4.0 * coefd * (coefb * coefe - coefa * coefh) + pow(root1 ,2.0);
  if(root2 > 0.0) root2 = sqrt(root2);

  aeff1 = (root1 - root2) / 2.0;
  aeff2 = (root1 + root2) / 2.0;

/* times a safe factor 1.2 when Prat is small */
  if(aeff1 > 0.0) aeff1 = sqrt(aeff1);
  else  aeff1 = 1.2 * sqrt(Gamma * W.P / W.d);
  if(aeff2 > 0.0) aeff2 = sqrt(aeff2);
  else  aeff2 = 1.2 * sqrt(Gamma * W.P / W.d);

  if(aeff2 > aeff1) return aeff2;
  else  return aeff1;

}

/* function to calculate derivative of source function over conserved variables */
void dSource(const Cons1DS U, const Real Bx, Real *SEE, Real *SErho, Real *SEmx, Real *SEmy, Real *SEmz, const Real x1)
{
/* NOTE that SEmy and SEmz can be NULL, which depends on dimension of the problem */
/* In FARGO, the independent variables are perturbed quantities.
 * But in source terms, especially the co-moving flux, should include background shearing */

/* Opacity is: Sigma[0]=Sigma_sF, Sigma[1] = Sigma_aF, Sigma[2] = Sigma_aP, Sigma[3] = Sigma_aE */
  Real pressure, temperature, velocity_x, velocity_y, velocity_z, velocity_fargo;
  Real Fr0x, Fr0y, Fr0z;

/*
  Real dSigma[2*NOPACITY];

  Real dSigmaE[NOPACITY], dSigmarho[NOPACITY], dSigmavx[NOPACITY], dSigmavy[NOPACITY], dSigmavz[NOPACITY];
*/
  Real Sigma[NOPACITY];

  int i,m;
/*      for(i=0; i<2*NOPACITY; i++)
        dSigma[i] = 0.0;
*/
  for(m=0;m<NOPACITY;m++){
    Sigma[m] = U.Sigma[m];
  }


  pressure = (U.E - 0.5 * (U.Mx * U.Mx + U.My * U.My + U.Mz * U.Mz) / U.d ) * (Gamma - 1.0);
#ifdef  RADIATION_MHD
  pressure -= (Gamma - 1.0) * 0.5 * (Bx * Bx + U.By * U.By + U.Bz * U.Bz);
#endif
/* capture negative pressure */
  if(pressure > TINY_NUMBER){
/* Should include magnetic energy for MHD */
    temperature = pressure / (U.d * R_ideal);
    velocity_x = U.Mx / U.d;
    velocity_y = U.My / U.d;
    velocity_z = U.Mz / U.d;
    velocity_fargo = velocity_y;

#ifdef FARGO
    velocity_fargo = velocity_y - qshear * Omega_0 * x1;

#endif


    Fr0x = U.Fr1 - ((1.0 + U.Edd_11) * velocity_x + U.Edd_21 * velocity_fargo + U.Edd_31 * velocity_z) * U.Er/Crat;
    Fr0y = U.Fr2 - (U.Edd_21 * velocity_x + (1.0 + U.Edd_22) * velocity_fargo + U.Edd_32 * velocity_z) * U.Er/Crat;
    Fr0z = U.Fr3 - (U.Edd_31 * velocity_x + U.Edd_32 * velocity_fargo + (1.0 + U.Edd_33) * velocity_z) * U.Er/Crat;


/*
  if(Opacity != NULL) Opacity(U.d, temperature, NULL, dSigma);

  for(m=0;m<NOPACITY;m++){
  dSigmaE[m] = dSigma[4+m] * (Gamma - 1.0)/(U.d * R_ideal);

  dSigmavx[m] = -dSigma[4+m] * velocity_x * (Gamma - 1.0) /(U.d * R_ideal);
  dSigmavy[m] = -dSigma[4+m] * velocity_y * (Gamma - 1.0) /(U.d * R_ideal);
  dSigmavz[m] = -dSigma[4+m] * velocity_z * (Gamma - 1.0) /(U.d * R_ideal);

  dSigmarho[m] = dSigma[m] - dSigma[4+m] * (Gamma - 1.0) * (U.E - (velocity_x * velocity_x + velocity_y * velocity_y + velocity_z * velocity_z) * U.d)/(U.d * U.d * R_ideal);
  #ifdef RADIATION_MHD
  dSigmarho[m] += dSigma[4+m] * 0.5 * (Gamma - 1.0) * (Bx * Bx + U.By * U.By + U.Bz * U.Bz) / (U.d * U.d * R_ideal);
  #endif

  }
*/
/* We keep another v/c term here */
/* When opacity depends on density and temperature, this may cause trouble */
/*       *SEE = 4.0 * Sigma[2] * temperature * temperature * temperature * (Gamma - 1.0)/ (U.d * R_ideal)
         + (dSigmaE[2] * pow(temperature, 4.0) - dSigmaE[3] * U.Er)
         + (dSigmaE[1] - dSigmaE[0]) * (
         velocity_x * (U.Fr1 - ((1.0 + U.Edd_11) * velocity_x + U.Edd_21 * velocity_fargo + U.Edd_31 * velocity_z) * U.Er/Crat)
         +  velocity_fargo * (U.Fr2 - (U.Edd_21 * velocity_x + (1.0 + U.Edd_22) * velocity_fargo + U.Edd_32 * velocity_z) * U.Er/Crat)
         +  velocity_z * (U.Fr3 - (U.Edd_31 * velocity_x + U.Edd_32 * velocity_fargo + (1.0 + U.Edd_33) * velocity_z) * U.Er/Crat)
         )/Crat;
*/
/* If *SEE < 0, the code will be unstable */
    *SEE = 4.0 * Sigma[2] * temperature * temperature * temperature * (Gamma - 1.0)/ (U.d * R_ideal);
    if(!Erflag)
      *SEE = 0.0;


    *SErho = 4.0 * Sigma[2] * temperature * temperature * temperature * (Gamma - 1.0) * (-U.E/U.d + velocity_x * velocity_x + velocity_y * velocity_y + velocity_z * velocity_z)/ (U.d * R_ideal);
/*
  + (dSigmarho[2] * pow(temperature, 4.0) - dSigmarho[3] * U.Er)
  + ((dSigmarho[1] - dSigmarho[0]) - (Sigma[1] - Sigma[0]) / U.d) * (
  velocity_x * (U.Fr1 - ((1.0 + U.Edd_11) * velocity_x + U.Edd_21 * velocity_fargo + U.Edd_31 * velocity_z) * U.Er/Crat)
  +  velocity_fargo * (U.Fr2 - (U.Edd_21 * velocity_x + (1.0 + U.Edd_22) * velocity_fargo + U.Edd_32 * velocity_z) * U.Er/Crat)
  +  velocity_z * (U.Fr3 - (U.Edd_31 * velocity_x + U.Edd_32 * velocity_fargo + (1.0 + U.Edd_33) * velocity_z) * U.Er/Crat)
  )/Crat;
*/
#ifdef RADIATION_MHD
    *SErho += 4.0 * Sigma[2] * temperature * temperature * temperature * (Gamma - 1.0) * 0.5 * (Bx * Bx + U.By * U.By + U.Bz * U.Bz)/(U.d * U.d * R_ideal);
#endif

    *SEmx = -4.0 * Sigma[2] * temperature * temperature * temperature * (Gamma - 1.0) * velocity_x / (U.d * R_ideal)
      + (Sigma[1] - Sigma[0]) * Fr0x / (Crat * U.d);

/*            + (dSigmavx[2] * pow(temperature, 4.0) - dSigmavx[3] * U.Er);
 */
    if(SEmy != NULL)
      *SEmy = -4.0 * Sigma[2] * temperature * temperature * temperature * (Gamma - 1.0) * velocity_y / (U.d * R_ideal)
        + (Sigma[1] - Sigma[0]) * Fr0y / (Crat * U.d);

/*            + (dSigmavy[2] * pow(temperature, 4.0) - dSigmavy[3] * U.Er);
 */
    if(SEmz != NULL)
      *SEmz = -4.0 * Sigma[2] * temperature * temperature * temperature * (Gamma - 1.0) * velocity_z / (U.d * R_ideal)
        + (Sigma[1] - Sigma[0]) * Fr0z / (Crat * U.d);

/*            + (dSigmavz[2] * pow(temperature, 4.0) - dSigmavz[3] * U.Er);
 */
  }
  else{

    *SEE = 0.0;
    *SErho = 0.0;
    *SEmx = 0.0;
    if(SEmy != NULL)
      *SEmy = 0.0;
    if(SEmz != NULL)
      *SEmz = 0.0;

  }


  return;


}




/* Function to calculate the eddington tensor.
 *  Only used when radiation_transfer module is defined
 * Only work for 1 frequency now
 */

#ifdef FLD

void Eddington_FUN (DomainS *pD)
{
  GridS *pG=(pD->Grid);
  int i, j, k, DIM;
  int is, ie, js, je, ks, ke;


  DIM = 0;
  is = pG->is;
  ie = pG->ie;
  js = pG->js;
  je = pG->je;
  ks = pG->ks;
  ke = pG->ke;

  for (i=0; i<3; i++) if(pG->Nx[i] > 1) ++DIM;

/* Note that in pRG, is = 1, ie = is + Nx *
 * that is, for radiation_transfer, there is only one ghost zone
 * for hydro ,there is 4 ghost zone
 */

  Real dErdx, dErdy, dErdz, dx, dy, dz, divEr, limiter, sigmat, Er, Eddf, FLDR;
  dx = pG->dx1;
  dy = pG->dx2;
  dz = pG->dx3;

  for(k=ks; k<=ke; k++) {
    for(j=js; j<=je; j++) {
      for(i=is; i<=ie; i++) {
        dErdx = (pG->U[k][j][i+1].Er - pG->U[k][j][i-1].Er) / (2.0 * dx);
        dErdy = (pG->U[k][j+1][i].Er - pG->U[k][j-1][i].Er) / (2.0 * dy);
        if(DIM == 3){
          dErdz = (pG->U[k+1][j][i].Er - pG->U[k-1][j][i].Er) / (2.0 * dz);
        }
        else{
          dErdz = 0.0;
        }

        Er = pG->U[k][j][i].Er;

        divEr = sqrt(dErdx * dErdx + dErdy * dErdy + dErdz * dErdz);
/* scale to unit vector */
        if(divEr > TINY_NUMBER){
          dErdx /= divEr;
          dErdy /= divEr;
          dErdz /= divEr;
        }

        sigmat = pG->U[k][j][i].Sigma[0] + pG->U[k][j][i].Sigma[1];

        FLD_limiter(divEr, Er, sigmat, &(limiter));
        FLDR = divEr/(sigmat*Er);
        Eddf = limiter + limiter * limiter * FLDR * FLDR;


        pG->U[k][j][i].Edd_11 = 0.5 * (1.0 - Eddf) + 0.5 * (3.0 * Eddf - 1.0) * dErdx * dErdx;
        pG->U[k][j][i].Edd_21 = 0.5 * (3.0 * Eddf - 1.0) * dErdx * dErdy;
        pG->U[k][j][i].Edd_22 = 0.5 * (1.0 - Eddf) + 0.5 * (3.0 * Eddf - 1.0) * dErdy * dErdy;
        if(DIM == 3){
          pG->U[k][j][i].Edd_31 = 0.5 * (3.0 * Eddf - 1.0) * dErdx * dErdz;
          pG->U[k][j][i].Edd_32 = 0.5 * (3.0 * Eddf - 1.0) * dErdy * dErdz;
          pG->U[k][j][i].Edd_33 = 0.5 * (1.0 - Eddf) + 0.5 * (3.0 * Eddf - 1.0) * dErdz * dErdz;
        }
      }
    }
  }
  return;
}

#else


#ifdef RADIATION_TRANSFER
/* Function to calculate the eddington tensor.
 *  Only used when radiation_transfer module is defined
 * Only work for 1 frequency now
 */

void Eddington_FUN (DomainS *pD)
{
  RadGridS *pRG=(pD->RadGrid);
  GridS *pG=(pD->Grid);
  int i, j, k, DIM;
  int is, ie, js, je, ks, ke;
  int ri, rj, rk;
  int ioff, joff, koff;
  int ifr = 0;
  Real J, Hrt = 0.0;

  DIM = 0;
  is = pG->is;
  ie = pG->ie;
  js = pG->js;
  je = pG->je;
  ks = pG->ks;
  ke = pG->ke;

/* Note that in pRG, is = 1, ie = is + Nx *
 * that is, for radiation_transfer, there is only one ghost zone
 * for hydro ,there is 4 ghost zone
 */

  for (i=0; i<3; i++) if(pG->Nx[i] > 1) ++DIM;
  ioff = 1 - nghost;
  if(DIM > 1) {
    joff = 1 - nghost;
    if (DIM == 3)
      koff = 1 - nghost;
    else
      koff = 0;
  } else{
    joff = 0;
    koff = 0;
  }

  for(k=ks; k<=ke; k++) {
    rk = k + koff;
    for(j=js; j<=je; j++) {
      rj = j + joff;
      for(i=is; i<=ie; i++) {
        ri = i + ioff;

#ifdef RAY_TRACING
        Hrt = pRG->H[ifr][rk][rj][ri];
#endif
        J = pRG->R[ifr][rk][rj][ri].J + Hrt;

        if(fabs(J) < TINY_NUMBER)
          ath_error("[Eddington_FUN]: Zeroth momentum of specific intensity is zero at i: %d  j:  %d  k:  %d\n",i,j,k);

        if(DIM == 1)
          pG->U[k][j][i].Edd_11 = (pRG->R[ifr][rk][rj][ri].K[0] + Hrt)/J;
        else if(DIM == 2){
          pG->U[k][j][i].Edd_11 = (pRG->R[ifr][rk][rj][ri].K[0] + Hrt)/J;
          pG->U[k][j][i].Edd_21 = pRG->R[ifr][rk][rj][ri].K[1]/J;
          pG->U[k][j][i].Edd_22 = pRG->R[ifr][rk][rj][ri].K[2]/J;
        }
        else if(DIM == 3){
          pG->U[k][j][i].Edd_11 = (pRG->R[ifr][rk][rj][ri].K[0] + Hrt)/J;
          pG->U[k][j][i].Edd_21 = pRG->R[ifr][rk][rj][ri].K[1]/J;
          pG->U[k][j][i].Edd_22 = pRG->R[ifr][rk][rj][ri].K[2]/J;
          pG->U[k][j][i].Edd_31 = pRG->R[ifr][rk][rj][ri].K[3]/J;
          pG->U[k][j][i].Edd_32 = pRG->R[ifr][rk][rj][ri].K[4]/J;
          pG->U[k][j][i].Edd_33 = pRG->R[ifr][rk][rj][ri].K[5]/J;
        }
        else
          ath_error("Dimension is not right!\n");
      }
    }
  }
  return;
}


void Eddington_FUN_new(const GridS *pG, const RadGridS *pRG)
{

  int i, j, k, DIM;
  int is, ie, js, je, ks, ke;
  int ri, rj, rk;
  Real fx, fy, fz, fmag, frat, chi;
  Real d1, d2;
  DIM = 0;
  is = pG->is;
  ie = pG->ie;
  js = pG->js;
  je = pG->je;
  ks = pG->ks;
  ke = pG->ke;

/* Note that in pRG, is = 1, ie = is + Nx *
 * that is, for radiation_transfer, there is only one ghost zone
 * for hydro ,there is 4 ghost zone
 */


  for (i=0; i<3; i++) if(pG->Nx[i] > 1) ++DIM;

  for(k=ks; k<=ke; k++)
    for(j=js; j<=je; j++)
      for(i=is; i<=ie; i++){
        if(DIM == 1){
          rj = j;
          rk = k;
          ri = i -nghost + 1;
        }
        else if(DIM == 2){
          rk = k;
          ri = i - nghost + 1;
          rj = j - nghost + 1;
        }
        else{
          ri = i - nghost + 1;
          rj = j - nghost + 1;
          rk = k - nghost + 1;
        }

        fmag = pG->U[k][j][i].Fr1 * pG->U[k][j][i].Fr1;
        if(DIM == 2)
          fmag += pG->U[k][j][i].Fr2 * pG->U[k][j][i].Fr2;
        if(DIM == 3)
          fmag += pG->U[k][j][i].Fr3 * pG->U[k][j][i].Fr3;
        fmag = sqrt(fmag);
        fx = pG->U[k][j][i].Fr1 / fmag;
        if(DIM == 2)
          fy = pG->U[k][j][i].Fr2 / fmag;
        if(DIM == 3)
          fz = pG->U[k][j][i].Fr3 / fmag;
        frat = fmag / pG->U[k][j][i].Er;
/*if (is == is) printf(" frat: %g %g %g\n",frat,fmag, pG->U[k][j][i].Er);
 */
        if (frat > 1.0) {
          printf(" frat > 1: %g %g %g\n",frat,fmag, pG->U[k][j][i].Er);
          frat = 1.0;
        }
        chi = (3.0 + 4.0 * frat * frat) / (5.0 + 2.0 * sqrt(4.0 - 3.0 * frat * frat));
        d1 = 0.5 - 0.5 * chi;
        d2 = 1.5 * chi - 0.5;

        if(DIM == 1)
          pG->U[k][j][i].Edd_11 = chi;
        else if(DIM == 2){
          pG->U[k][j][i].Edd_11 = d1 + fx * fx * d2;
          pG->U[k][j][i].Edd_21 =      fx * fy * d2;
          pG->U[k][j][i].Edd_22 = d1 + fy * fy * d2;
        }
        else if(DIM == 3){
          pG->U[k][j][i].Edd_11 = d1 + fx * fx * d2;
          pG->U[k][j][i].Edd_21 =      fx * fy * d2;
          pG->U[k][j][i].Edd_22 = d1 + fy * fy * d2;
          pG->U[k][j][i].Edd_31 =      fx * fy * d2;
          pG->U[k][j][i].Edd_32 =      fy * fz * d2;
          pG->U[k][j][i].Edd_33 = d1 + fz * fz * d2;
        }
        else
          ath_error("Dimension is not right!\n");
      }

}



#endif



/* end radiation_transfer*/


#endif /* FLD */


/* Extrapolation function adopted from Numerical Receipes */
void polint(Real xa[], Real ya[], int n, Real x, Real *y, Real *dy)
{
  int i,m,ns=1;
  Real den,dif,dift,ho,hp,w;
  Real *c,*d;

  dif=fabs(x-xa[1]);
  c=(Real*)calloc_1d_array(n+1,sizeof(Real));
  d=(Real*)calloc_1d_array(n+1,sizeof(Real));
  for (i=1;i<=n;i++) {
    if ( (dift=fabs(x-xa[i])) < dif) {
      ns=i;
      dif=dift;
    }
    c[i]=ya[i];
    d[i]=ya[i];
  }
  *y=ya[ns--];
  for (m=1;m<n;m++) {
    for (i=1;i<=n-m;i++) {
      ho=xa[i]-x;
      hp=xa[i+m]-x;
      w=c[i+1]-d[i];
      if ( (den=ho-hp) == 0.0) ath_error("Error in routine polint");
      den=w/den;
      d[i]=hp*den;
      c[i]=ho*den;
    }
    *y += (*dy=(2*ns < (n-m) ? c[ns+1] : d[ns--]));
  }
  free_1d_array(d);
  free_1d_array(c);

}



/* Function to calculate  for source term T^4 - Er */

void GetTguess(MeshS *pM)
{



  GridS *pG;
  int i, j, k;
  int ie, is;
  int je, js;
  int ke, ks;
  int jl, ju, kl, ku;

  Real pressure, Sigma_aP, Sigma_aE, Ern, ETsource, Det, Erguess, Tguess, temperature, TEr, Ersum;
  Real sign1, sign2, coef1, coef2, coef3;

  Real dt, Terr, Ererr;

  int nl, nd;

  for (nl=0; nl<(pM->NLevels); nl++){
    for (nd=0; nd<(pM->DomainsPerLevel[nl]); nd++){
      if (pM->Domain[nl][nd].Grid != NULL){
        pG = pM->Domain[nl][nd].Grid;

        ie = pG->ie;
        is = pG->is;
        je = pG->je;
        js = pG->js;
        ke = pG->ke;
        ks = pG->ks;


        if (pG->Nx[1] > 1) {
          ju = pG->je + nghost;
          jl = pG->js - nghost;
        }
        else {
          ju = pG->je;
          jl = pG->js;
        }

        if (pG->Nx[2] > 1) {
          ku = pG->ke + nghost;
          kl = pG->ks - nghost;
        }
        else {
          ku = pG->ke;
          kl = pG->ks;
        }


        dt = pG->dt;

        for(k=kl; k<=ku; k++)
          for(j=jl; j<=ju; j++)
            for(i=is-nghost; i<=ie+nghost; i++)
              {

                pressure = (pG->U[k][j][i].E - (0.5 * pG->U[k][j][i].M1 * pG->U[k][j][i].M1 + 0.5 * pG->U[k][j][i].M2 * pG->U[k][j][i].M2
                                                + 0.5 * pG->U[k][j][i].M3 * pG->U[k][j][i].M3) / pG->U[k][j][i].d ) * (Gamma - 1.0);

#ifdef RADIATION_MHD
                pressure -= 0.5 * (pG->U[k][j][i].B1c * pG->U[k][j][i].B1c + pG->U[k][j][i].B2c * pG->U[k][j][i].B2c + pG->U[k][j][i].B3c * pG->U[k][j][i].B3c) * (Gamma - 1.0);
#endif

                temperature = pressure / (pG->U[k][j][i].d * R_ideal);



                Sigma_aP = pG->U[k][j][i].Sigma[2];
                Sigma_aE = pG->U[k][j][i].Sigma[3];
                Ern =  pG->U[k][j][i].Er;

                if((Sigma_aP < TINY_NUMBER) || (Sigma_aE < TINY_NUMBER)){
                  pG->Tguess[k][j][i] = pow(temperature, 4.0);
                  pG->Ersource[k][j][i] = 0.0;
                }
                else if(fabs(Ern - pow(temperature, 4.0)) < TINY_NUMBER){

                  pG->Tguess[k][j][i] = Ern;
                  pG->Ersource[k][j][i] = 0.0;
                }
                else if(pressure < TINY_NUMBER || pressure != pressure){
                  pG->Tguess[k][j][i] = Ern;
                  pG->Ersource[k][j][i] = 0.0;

                }
                else{

/*

                  ETsource = Crat * (Sigma_aP * pow(temperature,4.0) - Sigma_aE * Ern);

                  Det = 1.0 + 4.0 * (Gamma - 1.0) * dt * Prat * Crat * Sigma_aP * pow(temperature,3.0) / ( pG->U[k][j][i].d * R_ideal) + dt * Crat * Sigma_aE;
                  Erguess = Ern + dt * ReduceC * ETsource / Det;

                  Tguess = temperature - (Erguess -  pG->U[k][j][i].Er) * Prat * (Gamma - 1.0)/( ReduceC * pG->U[k][j][i].d * R_ideal);


                  Ererr = Ern + dt * 0.5 * (ETsource + ReduceC * Crat * (Sigma_aP * pow(Tguess,4.0) - Sigma_aE * Erguess)) - Erguess;
                  Terr = temperature - 0.5 * dt * (Gamma - 1.0) * Prat * (ETsource + Crat * (Sigma_aP * pow(Tguess,4.0) - Sigma_aE * Erguess))/( pG->U[k][j][i].d * R_ideal) - Tguess;

                  Det =  1.0 + 4.0 * (Gamma - 1.0) * dt * Prat * Crat * Sigma_aP * pow(Tguess,3.0) / ( pG->U[k][j][i].d * R_ideal) + dt * Crat * Sigma_aE;
                  Ern =  (1.0 + 4.0 * (Gamma - 1.0) * dt * Prat * Crat * Sigma_aP * pow(Tguess,3.0) / ( pG->U[k][j][i].d * R_ideal)) * Ererr / Det
                    + 4.0 * ReduceC * Crat * Sigma_aP * pow(Tguess,3.0) * dt * Terr / Det;
                  Erguess += Ern;

                  Tguess = temperature - (Erguess -  pG->U[k][j][i].Er) * Prat * (Gamma - 1.0)/( ReduceC * pG->U[k][j][i].d * R_ideal);

                  sign1 =  pG->U[k][j][i].Er - pow(temperature,4.0);
                  sign2 = Erguess - pow(Tguess, 4.0);

*/



                    if( pG->U[k][j][i].Er < 0.0) pG->U[k][j][i].Er = 0.0;

                    ThermalRelaxation(temperature, pG->U[k][j][i].Er, pG->U[k][j][i].d, Sigma_aP, Sigma_aE, pG->dt, &Tguess, &Erguess);
                



/*
  pG->Tguess[k][j][i] = Erguess;
*/
                  pG->Tguess[k][j][i] = pow(Tguess, 4.0);


/*      pG->Tguess[k][j][i] = pow(temperature, 4.0);
 */
              /* The Ersource = dt * ReduceC * Crat * sigma_a(T^4-E_r) */
 
                  pG->Ersource[k][j][i] = Erguess - pG->U[k][j][i].Er;

                }


              }

      }/* end if Grid != NULL */
    }/* End loop the domains at level nl */
  } /* end loop each level */
}



/* Function to get the thermal equilibrium radiation
 * energy density and gas temperature *
 * Input: density, thermal + radiation energy density , Er in last step *
 * Output: equilibrium temperature
 */
Real EquState(const Real density, const Real sum, const Real Er0)
{





  Real temperature, TEr, Tguess;
  Real coef1, coef2, coef3;


  coef1 = Prat;
  coef2 =  density * R_ideal / (Gamma - 1.0);
  coef3 = -sum;

  temperature = (sum - Prat * Er0) / (density * R_ideal);
  if(temperature < 0.0) temperature = 0.0;
  TEr = pow(Er0, 0.25);

  if(temperature > TEr){
    Tguess = rtsafe(Tequilibrium, TEr * (1.0 - 0.01), temperature * (1.0 + 0.01), 1.e-12, coef1, coef2, coef3,0.0);

  }
  else{
    Tguess = rtsafe(Tequilibrium, temperature * (1.0 - 0.01), TEr * (1.0 + 0.01), 1.e-12, coef1, coef2, coef3,0.0);
  }

  return Tguess;
}




/* Function to calculate matrix coefficient */

void matrix_coef(const MatrixS *pMat, const GridS *pG, const int DIM, const int i, const int j, const int k, const Real qom, Real *theta, Real *phi, Real *psi, Real *varphi){
/* Notice that pMat->rho actually stores the background E_r at time step n */

/* subtract the advection part from the matrix coefficients */
/*====================================================================
 * Notice that in the matrix, we transform Eulerian flux to co-moving flux
 * Advection part is seperated from diffusion part in order to
 * reduce the numerical diffusion in optical thick regime
 * There is a flag to decide whether seperate it or not
 *
 *===================================================================*/

/* Temporary variables to setup the matrix */
  Real hdtodx1, hdtodx2, hdtodx3, dt, dx, dy, dz, dl;
  Real vx, vy, vz, vxi0, vxi1, vxj0, vxj1, vxk0, vxk1, vyi0, vyi1, vyj0, vyj1, vyk0, vyk1, vzi0, vzi1, vzj0, vzj1, vzk0, vzk1;
  Real vFxFull, vFyFull, vFzFull, vFxi0Full, vFxi1Full, vFyj0Full, vFyj1Full, vFzk0Full, vFzk1Full; /* This always include background shearing */
  Real vFxi0, vFxi1, vFyj0, vFyj1, vFzk0, vFzk1;
/* Actual advection flux used at the cell interface, some of them will be zero */
  Real f11, f22, f33, f11i0, f11i1, f22j0, f22j1, f33k0, f33k1;
  Real f21, f31, f32, f21j0, f21j1, f21i0, f21i1, f31k0, f31k1, f31i0, f31i1, f32j0, f32j1, f32k0, f32k1;
  Real Sigma_aF, Sigma_aP, Sigma_aE, Sigma_sF;
  Real Ci0, Ci1, Cj0, Cj1, Ck0, Ck1;
  Real alphai0, alphaj0, alphak0, alphai, alphaj, alphak; /* For mininum velocity */
  Real alphai1max, alphaj1max, alphak1max, alphaimax, alphajmax, alphakmax; /* for maximum velocity */
/* alpha? are for Sigma_aF + Sigma_sF */
  int m;
  Real Sigma[4];
  Real direction;
  Real x1, x2, x3, vshear, vsheari0, vsheari1;
  vshear = 0.0;
  if(pMat != NULL){
    hdtodx1 = 0.5 * pMat->dt/pMat->dx1;
    hdtodx2 = 0.5 * pMat->dt/pMat->dx2;
    hdtodx3 = 0.5 * pMat->dt/pMat->dx3;


    dx = pMat->dx1;
    dy = pMat->dx2;
    dz = pMat->dx3;
    dt = pMat->dt;
#ifdef SHEARING_BOX
#ifdef FARGO
    x1 =  pMat->MinX[0] + ((Real)(i - pMat->is) + 0.5)*dx;
    vshear = qshear * Omega_0 * x1;
#endif
#endif
    vx = pMat->Ugas[k][j][i].V1;
    vxi0 = pMat->Ugas[k][j][i-1].V1;
    vxi1 = pMat->Ugas[k][j][i+1].V1;
    vy = pMat->Ugas[k][j][i].V2;
    vyi0 = pMat->Ugas[k][j][i-1].V2;
    vyi1 = pMat->Ugas[k][j][i+1].V2;
    if(DIM > 1){
      vxj0 = pMat->Ugas[k][j-1][i].V1;
      vxj1 = pMat->Ugas[k][j+1][i].V1;
      vyj0 = pMat->Ugas[k][j-1][i].V2;
      vyj1 = pMat->Ugas[k][j+1][i].V2;
    }
    vz = pMat->Ugas[k][j][i].V3;
    if(DIM > 2){
      vzi0 = pMat->Ugas[k][j][i-1].V3;
      vzi1 = pMat->Ugas[k][j][i+1].V3;
      vzj0 = pMat->Ugas[k][j-1][i].V3;
      vzj1 = pMat->Ugas[k][j+1][i].V3;
      vzk0 = pMat->Ugas[k-1][j][i].V3;
      vzk1 = pMat->Ugas[k+1][j][i].V3;

      vxk0 = pMat->Ugas[k-1][j][i].V1;
      vxk1 = pMat->Ugas[k+1][j][i].V1;
      vyk0 = pMat->Ugas[k-1][j][i].V2;
      vyk1 = pMat->Ugas[k+1][j][i].V2;
    }


    f11i0 = pMat->Ugas[k][j][i-1].Edd_11;
    f21i0 = pMat->Ugas[k][j][i-1].Edd_21;
    f31i0 = pMat->Ugas[k][j][i-1].Edd_31;
    f11 = pMat->Ugas[k][j][i].Edd_11;
    f22 = pMat->Ugas[k][j][i].Edd_22;
    f33 = pMat->Ugas[k][j][i].Edd_33;
    f21 = pMat->Ugas[k][j][i].Edd_21;
    f31 = pMat->Ugas[k][j][i].Edd_31;
    f32 = pMat->Ugas[k][j][i].Edd_32;
    f11i1 = pMat->Ugas[k][j][i+1].Edd_11;
    f21i1 = pMat->Ugas[k][j][i+1].Edd_21;
    f31i1 = pMat->Ugas[k][j][i+1].Edd_31;

    if(DIM > 1){
      f32j0 = pMat->Ugas[k][j-1][i].Edd_32;
      f22j0 = pMat->Ugas[k][j-1][i].Edd_22;
      f21j0 = pMat->Ugas[k][j-1][i].Edd_21;

      f21j1 = pMat->Ugas[k][j+1][i].Edd_21;
      f22j1 = pMat->Ugas[k][j+1][i].Edd_22;
      f32j1 = pMat->Ugas[k][j+1][i].Edd_32;
    }

    if(DIM > 2){
      f33k0 = pMat->Ugas[k-1][j][i].Edd_33;
      f32k0 = pMat->Ugas[k-1][j][i].Edd_32;
      f31k0 = pMat->Ugas[k-1][j][i].Edd_31;

      f31k1 = pMat->Ugas[k+1][j][i].Edd_31;
      f32k1 = pMat->Ugas[k+1][j][i].Edd_32;
      f33k1 = pMat->Ugas[k+1][j][i].Edd_33;
    }

    Sigma_sF = pMat->Ugas[k][j][i].Sigma[0];
    Sigma_aF = pMat->Ugas[k][j][i].Sigma[1];
    Sigma_aP = pMat->Ugas[k][j][i].Sigma[2];
    Sigma_aE = pMat->Ugas[k][j][i].Sigma[3];


  }
  else if(pG != NULL){
    hdtodx1 = 0.5 * pG->dt/pG->dx1;
    hdtodx2 = 0.5 * pG->dt/pG->dx2;
    hdtodx3 = 0.5 * pG->dt/pG->dx3;
    dx = pG->dx1;
    dy = pG->dx2;
    dz = pG->dx3;
    dt = pG->dt;
    vx = pG->U[k][j][i].M1 / pG->U[k][j][i].d;
    vxi0 = pG->U[k][j][i-1].M1 / pG->U[k][j][i-1].d;
    vxi1 = pG->U[k][j][i+1].M1 / pG->U[k][j][i+1].d;

    vy = pG->U[k][j][i].M2 / pG->U[k][j][i].d;

    if(DIM > 1){
      vxj0 = pG->U[k][j-1][i].M1 / pG->U[k][j-1][i].d;
      vxj1 = pG->U[k][j+1][i].M1 / pG->U[k][j+1][i].d;

      vyi0 = pG->U[k][j][i-1].M2 / pG->U[k][j][i-1].d;
      vyi1 = pG->U[k][j][i+1].M2 / pG->U[k][j][i+1].d;

      vyj0 = pG->U[k][j-1][i].M2 / pG->U[k][j-1][i].d;
      vyj1 = pG->U[k][j+1][i].M2 / pG->U[k][j+1][i].d;


    }

    vz = pG->U[k][j][i].M3 / pG->U[k][j][i].d;

    if(DIM > 2){
      vzi0 = pG->U[k][j][i-1].M3 / pG->U[k][j][i-1].d;
      vzi1 = pG->U[k][j][i+1].M3 / pG->U[k][j][i+1].d;
      vzj0 = pG->U[k][j-1][i].M3 / pG->U[k][j-1][i].d;
      vzj1 = pG->U[k][j+1][i].M3 / pG->U[k][j+1][i].d;
      vzk0 = pG->U[k-1][j][i].M3 / pG->U[k-1][j][i].d;
      vzk1 = pG->U[k+1][j][i].M3 / pG->U[k+1][j][i].d;

      vxk0 = pG->U[k-1][j][i].M1 / pG->U[k-1][j][i].d;
      vxk1 = pG->U[k+1][j][i].M1 / pG->U[k+1][j][i].d;

      vyk0 = pG->U[k-1][j][i].M2 / pG->U[k-1][j][i].d;
      vyk1 = pG->U[k+1][j][i].M2 / pG->U[k+1][j][i].d;
    }


#ifdef SHEARING_BOX
#ifdef FARGO
/* vshear is qom * x1 */
    cc_pos(pG,i,j,k,&x1,&x2,&x3);
    vshear   = qom * x1;
    cc_pos(pG,i-1,j,k,&x1,&x2,&x3);
    vsheari0 = qom * x1;
    cc_pos(pG,i+1,j,k,&x1,&x2,&x3);
    vsheari1 = qom * x1;

    vy   -= vshear;
    vyj0 -= vshear;
    vyj1 -= vshear;
    vyk0 -= vshear;
    vyk1 -= vshear;

    vyi0 -= vsheari0;
    vyi1 -= vsheari1;
#endif
#endif


    f11i0 = pG->U[k][j][i-1].Edd_11;
    f21i0 = pG->U[k][j][i-1].Edd_21;
    f31i0 = pG->U[k][j][i-1].Edd_31;
    f11 = pG->U[k][j][i].Edd_11;
    f22 = pG->U[k][j][i].Edd_22;
    f33 = pG->U[k][j][i].Edd_33;
    f21 = pG->U[k][j][i].Edd_21;
    f31 = pG->U[k][j][i].Edd_31;
    f32 = pG->U[k][j][i].Edd_32;
    f11i1 = pG->U[k][j][i+1].Edd_11;
    f21i1 = pG->U[k][j][i+1].Edd_21;
    f31i1 = pG->U[k][j][i+1].Edd_31;

    if(DIM > 1){
      f32j0 = pG->U[k][j-1][i].Edd_32;
      f22j0 = pG->U[k][j-1][i].Edd_22;
      f21j0 = pG->U[k][j-1][i].Edd_21;

      f21j1 = pG->U[k][j+1][i].Edd_21;
      f22j1 = pG->U[k][j+1][i].Edd_22;
      f32j1 = pG->U[k][j+1][i].Edd_32;
    }
    if(DIM > 2){
      f33k0 = pG->U[k-1][j][i].Edd_33;
      f32k0 = pG->U[k-1][j][i].Edd_32;
      f31k0 = pG->U[k-1][j][i].Edd_31;

      f31k1 = pG->U[k+1][j][i].Edd_31;
      f32k1 = pG->U[k+1][j][i].Edd_32;
      f33k1 = pG->U[k+1][j][i].Edd_33;
    }

    Sigma_sF = pG->U[k][j][i].Sigma[0];
    Sigma_aF = pG->U[k][j][i].Sigma[1];
    Sigma_aP = pG->U[k][j][i].Sigma[2];
    Sigma_aE = pG->U[k][j][i].Sigma[3];


  }
  else{

    ath_error("[matrix_coef]: Must provide either pMat or pG pointer!n\n");
  }

/* take the minimum in case resolution is not uniform in each direction */
  dl = dx;
  if((dy < dl) && (DIM > 1))    dl = dy;
  if((dz < dl) && (DIM > 2))    dl = dz;

/* First, calculate vF?Full, this is included no matter RadFargo is used or not */
/* background shearing is always included */
/* seperate vE_r + vP_r part from the Eulerian flux */
  if(DIM == 1){
    vFxFull   = (1.0 + f11) * vx / Crat;
    vFxi0Full   = (1.0 + f11i0) * vxi0 / Crat;
    vFxi1Full   = (1.0 + f11i1) * vxi1 / Crat;
  }
  else if(DIM == 2){
    vFxFull   = ((1.0 + f11) * vx + vy * f21) / Crat;
    vFxi0Full = ((1.0 + f11i0) * vxi0 + vyi0 * f21i0) / Crat;
    vFxi1Full = ((1.0 + f11i1) * vxi1 + vyi1 * f21i1) / Crat;

    vFyFull   = ((1.0 + f22) * vy + vx * f21) / Crat;
    vFyj0Full = ((1.0 + f22j0) * vyj0 + vxj0 * f21j0) / Crat;
    vFyj1Full = ((1.0 + f22j1) * vyj1 + vxj1 * f21j1) / Crat;

  }
  else if(DIM == 3){
    vFxFull   = ((1.0 + f11) * vx + vy * f21 + vz * f31) / Crat;
    vFxi0Full = ((1.0 + f11i0) * vxi0 + vyi0 * f21i0 + vzi0 * f31i0) / Crat;
    vFxi1Full = ((1.0 + f11i1) * vxi1 + vyi1 * f21i1 + vzi1 * f31i1) / Crat;

    vFyFull   = ((1.0 + f22) * vy + vx * f21 + vz * f32) / Crat;
    vFyj0Full = ((1.0 + f22j0) * vyj0 + vxj0 * f21j0 + vzj0 * f32j0) / Crat;
    vFyj1Full = ((1.0 + f22j1) * vyj1 + vxj1 * f21j1 + vzj1 * f32j1) / Crat;

    vFzFull   = ((1.0 + f33) * vz + vx * f31 + vy * f32) / Crat;
    vFzk0Full = ((1.0 + f33k0) * vzk0 + vxk0 * f31k0 + vyk0 * f32k0) / Crat;
    vFzk1Full = ((1.0 + f33k1) * vzk1 + vxk1 * f31k1 + vyk1 * f32k1) / Crat;
  }


/* Set the flag whether split advection term or not */
/* in optical thick regime, split advection to use upwind flux */
/* flag to use upwind flux for advection term */

/* calculate the Div(vP_r) term, which is added as cell centered difference */



  if(DIM == 1){
    vFxi0 = f11i0 * vxi0;
    vFxi1 = f11i1 * vxi1;
  }
  else if(DIM == 2){

    vFxi0 = f11i0 * vxi0 + vyi0 * f21i0;
    vFxi1 = f11i1 * vxi1 + vyi1 * f21i1;


    vFyj0 = f22j0 * vyj0 + vxj0 * f21j0;
    vFyj1 = f22j1 * vyj1 + vxj1 * f21j1;

  }
  else if(DIM == 3){


    vFxi0 = f11i0 * vxi0 + vyi0 * f21i0 + vzi0 * f31i0;
    vFxi1 = f11i1 * vxi1 + vyi1 * f21i1 + vzi1 * f31i1;


    vFyj0 = f22j0 * vyj0 + vxj0 * f21j0 + vzj0 * f32j0;
    vFyj1 = f22j1 * vyj1 + vxj1 * f21j1 + vzj1 * f32j1;


    vFzk0 = f33k0 * vzk0 + vxk0 * f31k0 + vyk0 * f32k0;
    vFzk1 = f33k1 * vzk1 + vxk1 * f31k1 + vyk1 * f32k1;

  }

/*===============================================================================*/


  if(pMat != NULL){
/* use average opacity to calculate flux */
    for(m=0; m<4; m++){
      Sigma[m] = 0.5 * (pMat->Ugas[k][j][i-1].Sigma[m] + pMat->Ugas[k][j][i].Sigma[m]);
    }

    matrix_alpha(direction, Sigma, dt, pMat->Ugas[k][j][i-1].Edd_11, vx, &alphai0, -1, dx);



    matrix_alpha(direction, Sigma, dt, pMat->Ugas[k][j][i].Edd_11, vx, &alphaimax, 1, dx);

    for(m=0; m<4; m++){
      Sigma[m] = 0.5 * (pMat->Ugas[k][j][i].Sigma[m] + pMat->Ugas[k][j][i+1].Sigma[m]);
    }

    matrix_alpha(direction, Sigma, dt, pMat->Ugas[k][j][i].Edd_11, vx, &alphai, -1, dx);



    matrix_alpha(direction, Sigma, dt, pMat->Ugas[k][j][i+1].Edd_11, vx, &alphai1max, 1, dx);


  }
  else if(pG != NULL){
    for(m=0; m<4; m++){
      Sigma[m] = 0.5 * (pG->U[k][j][i-1].Sigma[m] + pG->U[k][j][i].Sigma[m]);
    }

    matrix_alpha(direction, Sigma, dt, pG->U[k][j][i-1].Edd_11, vx, &alphai0, -1, dx);


    matrix_alpha(direction, Sigma, dt, pG->U[k][j][i].Edd_11, vx, &alphaimax, 1, dx);

    for(m=0; m<4; m++){
      Sigma[m] = 0.5 * (pG->U[k][j][i+1].Sigma[m] + pG->U[k][j][i].Sigma[m]);
    }


    matrix_alpha(direction, Sigma, dt, pG->U[k][j][i].Edd_11, vx, &alphai, -1, dx);



    matrix_alpha(direction, Sigma, dt, pG->U[k][j][i+1].Edd_11, vx, &alphai1max, 1, dx);

  }


  if(alphaimax + alphai0 > TINY_NUMBER)
    Ci0 = (alphaimax -  alphai0) / (alphaimax + alphai0);
  else
    Ci0 = 0.0;

  if(alphai1max + alphai > TINY_NUMBER)
    Ci1 = (alphai1max - alphai) / (alphai1max + alphai);
  else
    Ci1 = 0.0;




  if(DIM > 1){
    if(pMat != NULL){
      for(m=0; m<4; m++){
        Sigma[m] = 0.5 * (pMat->Ugas[k][j-1][i].Sigma[m] + pMat->Ugas[k][j][i].Sigma[m]) ;
      }

      matrix_alpha(direction, Sigma, dt, pMat->Ugas[k][j-1][i].Edd_22, vy, &alphaj0, -1, dy);



      matrix_alpha(direction, Sigma, dt, pMat->Ugas[k][j][i].Edd_22, vy, &alphajmax, 1, dy);

      for(m=0; m<4; m++){
        Sigma[m] = 0.5 * (pMat->Ugas[k][j+1][i].Sigma[m] + pMat->Ugas[k][j][i].Sigma[m]);
      }

      matrix_alpha(direction, Sigma, dt, pMat->Ugas[k][j][i].Edd_22, vy, &alphaj, -1, dy);




      matrix_alpha(direction, Sigma, dt, pMat->Ugas[k][j+1][i].Edd_22, vy, &alphaj1max, 1, dy);

    }
    else if(pG != NULL){

      for(m=0; m<4; m++){
        Sigma[m] = 0.5 * (pG->U[k][j-1][i].Sigma[m] + pG->U[k][j][i].Sigma[m]) ;
      }

      matrix_alpha(direction, Sigma, dt, pG->U[k][j-1][i].Edd_22, vy, &alphaj0, -1, dy);



      matrix_alpha(direction, Sigma, dt, pG->U[k][j][i].Edd_22, vy, &alphajmax, 1, dy);

      for(m=0; m<4; m++){
        Sigma[m] = 0.5 * (pG->U[k][j+1][i].Sigma[m] + pG->U[k][j][i].Sigma[m]) ;
      }

      matrix_alpha(direction, Sigma, dt, pG->U[k][j][i].Edd_22, vy, &alphaj, -1, dy);



      matrix_alpha(direction, Sigma, dt, pG->U[k][j+1][i].Edd_22, vy, &alphaj1max, 1, dy);

    }

    if(alphajmax + alphaj0 > TINY_NUMBER)
      Cj0 = (alphajmax -  alphaj0) / (alphajmax + alphaj0);
    else
      Cj0 = 0.0;

    if(alphaj1max + alphaj > TINY_NUMBER)
      Cj1 = (alphaj1max - alphaj) / (alphaj1max + alphaj);
    else
      Cj1 = 0.0;
  }
  if(DIM > 2){

    if(pMat != NULL){
      for(m=0; m<4; m++){
        Sigma[m] = 0.5 * (pMat->Ugas[k-1][j][i].Sigma[m] + pMat->Ugas[k][j][i].Sigma[m]) ;
      }

      matrix_alpha(direction, Sigma, dt, pMat->Ugas[k-1][j][i].Edd_33, vz, &alphak0, -1, dz);


      matrix_alpha(direction, Sigma, dt, pMat->Ugas[k][j][i].Edd_33, vz, &alphakmax, 1, dz);

      for(m=0; m<4; m++){
        Sigma[m] = 0.5 * (pMat->Ugas[k+1][j][i].Sigma[m] + pMat->Ugas[k][j][i].Sigma[m]) ;
      }

      matrix_alpha(direction, Sigma, dt, pMat->Ugas[k][j][i].Edd_33, vz, &alphak, -1, dz);



      matrix_alpha(direction, Sigma, dt, pMat->Ugas[k+1][j][i].Edd_33, vz, &alphak1max, 1, dz);


    }
    else if(pG != NULL){
/* Direction is not used anymore */

      for(m=0; m<4; m++){
        Sigma[m] = 0.5 * (pG->U[k-1][j][i].Sigma[m] + pG->U[k][j][i].Sigma[m]) ;
      }

      matrix_alpha(direction, Sigma, dt, pG->U[k-1][j][i].Edd_33, vz, &alphak0, -1, dz);




      matrix_alpha(direction, Sigma, dt, pG->U[k][j][i].Edd_33, vz, &alphakmax, 1, dz);

      for(m=0; m<4; m++){
        Sigma[m] = 0.5 * (pG->U[k+1][j][i].Sigma[m] + pG->U[k][j][i].Sigma[m]) ;
      }

      matrix_alpha(direction, Sigma, dt, pG->U[k][j][i].Edd_33, vz, &alphak, -1, dz);




      matrix_alpha(direction, Sigma, dt, pG->U[k+1][j][i].Edd_33, vz, &alphak1max, 1, dz);

    }

    if(alphakmax + alphak0 > TINY_NUMBER)
      Ck0 = (alphakmax -  alphak0) / (alphakmax + alphak0);
    else
      Ck0 = 0.0;

    if(alphak1max + alphak > TINY_NUMBER)
      Ck1 = (alphak1max - alphak) / (alphak1max + alphak);
    else
      Ck1 = 0.0;

  }

/*============================================================*/
/* Now construct the matrix coefficient */
/* The radiation work term is always NOT seperated */
/* The matrix actually solves the co-moving flux */
  if(DIM == 1){
/* Assuming the velocity is already the original velocity in case of FARGO */
/* k - 1*/

    theta[0] = -ReduceC * Crat * hdtodx1 * (1.0 + Ci0) * (alphai0 - vFxi0Full) - ReduceC * hdtodx1 * vFxi0;
    theta[1] = -ReduceC * Crat * hdtodx1 * (1.0 + Ci0);
    theta[2] = 1.0 + ReduceC * Crat * hdtodx1 * (1.0 + Ci1) * (alphai - vFxFull)
      + ReduceC * Crat * hdtodx1 * (1.0 - Ci0) * (alphaimax + vFxFull)
      + ReduceC * dt * (Sigma_aF - Sigma_sF) * vx * vFxFull
      + Eratio * ReduceC * Crat * dt * Sigma_aE;
    theta[3] = ReduceC * Crat * hdtodx1 * (Ci0 + Ci1) - ReduceC * dt * (Sigma_aF - Sigma_sF) * vx;
    theta[4] = -ReduceC * Crat * hdtodx1 * (1.0 - Ci1) * (alphai1max + vFxi1Full) + ReduceC * hdtodx1 * vFxi1;
    theta[5] = ReduceC * Crat * hdtodx1 * (1.0 - Ci1);


    phi[0] = -ReduceC * Crat * hdtodx1 * (1.0 + Ci0) * f11i0;
    phi[1] = -ReduceC * Crat * hdtodx1 * (1.0 + Ci0) * alphai0;
    phi[2] = ReduceC * Crat * hdtodx1 * (Ci0 + Ci1) * f11
      - ReduceC * Crat * dt * (Sigma_aF + Sigma_sF) * vFxFull
      + Eratio * ReduceC * dt * Sigma_aE * vx;
    phi[3] = 1.0 + ReduceC * Crat * hdtodx1 * (1.0 + Ci1) * alphai +  ReduceC * Crat * hdtodx1 * (1.0 - Ci0) * alphaimax
      + ReduceC * Crat * dt * (Sigma_aF + Sigma_sF);
    phi[4] =  ReduceC * Crat * hdtodx1 * (1.0 - Ci1) * f11i1;
    phi[5] = -ReduceC * Crat * hdtodx1 * (1.0 - Ci1) * alphai1max;


  }
  else if(DIM == 2){


    theta[0] = -ReduceC * Crat * hdtodx2 * (1.0 + Cj0) * (alphaj0 - vFyj0Full) - ReduceC * hdtodx2 * vFyj0;
    theta[1] = -ReduceC * Crat * hdtodx2 * (1.0 + Cj0);
    theta[2] = -ReduceC * Crat * hdtodx1 * (1.0 + Ci0) * (alphai0 - vFxi0Full) - ReduceC * hdtodx1 * vFxi0;
    theta[3] = -ReduceC * Crat * hdtodx1 * (1.0 + Ci0);
    theta[4] = 1.0  + ReduceC * Crat * hdtodx1 * (1.0 + Ci1) * (alphai - vFxFull)
      + ReduceC * Crat * hdtodx1 * (1.0 - Ci0) * (alphaimax + vFxFull)
      + ReduceC * Crat * hdtodx2 * (1.0 + Cj1) * (alphaj - vFyFull)
      + ReduceC * Crat * hdtodx2 * (1.0 - Cj0) * (alphajmax + vFyFull)
      + ReduceC * dt * (Sigma_aF - Sigma_sF) * (vx * vFxFull + vy * vFyFull)
      + Eratio * ReduceC * Crat * dt * Sigma_aE;
    theta[5] = ReduceC * Crat * hdtodx1 * (Ci0 + Ci1) - ReduceC * dt * (Sigma_aF - Sigma_sF) * vx;
    theta[6] = ReduceC * Crat * hdtodx2 * (Cj0 + Cj1) - ReduceC * dt * (Sigma_aF - Sigma_sF) * vy;
    theta[7] = -ReduceC * Crat * hdtodx1 * (1.0 - Ci1) * (alphai1max + vFxi1Full) + ReduceC * hdtodx1 * vFxi1;
    theta[8] = ReduceC * Crat * hdtodx1 * (1.0 - Ci1);
    theta[9] = -ReduceC * Crat * hdtodx2 * (1.0 - Cj1) * (alphaj1max + vFyj1Full) + ReduceC * hdtodx2 * vFyj1;
    theta[10] = ReduceC * Crat * hdtodx2 * (1.0 - Cj1);

    phi[0] = -ReduceC * Crat * hdtodx2 * (1.0 + Cj0) * f21j0;
    phi[1] = -ReduceC * Crat * hdtodx2 * (1.0 + Cj0) * alphaj0;
    phi[2] = -ReduceC * Crat * hdtodx1 * (1.0 + Ci0) * f11i0;
    phi[3] = -ReduceC * Crat * hdtodx1 * (1.0 + Ci0) * alphai0;
    phi[4] = ReduceC * Crat * hdtodx1 * (Ci0 + Ci1) * f11
      + ReduceC * Crat * hdtodx2 * (Cj0 + Cj1) * f21
      - ReduceC * Crat * dt * (Sigma_aF + Sigma_sF) * vFxFull
      + Eratio * ReduceC * dt * Sigma_aE * vx;
    phi[5] = 1.0 + ReduceC * Crat * hdtodx1 * (1.0 + Ci1) * alphai +  ReduceC * Crat * hdtodx1 * (1.0 - Ci0) * alphaimax
      + ReduceC * Crat * hdtodx2 * (1.0 + Cj1) * alphaj +  ReduceC * Crat * hdtodx2 * (1.0 - Cj0) * alphajmax
      + ReduceC * Crat * dt * (Sigma_aF + Sigma_sF);
    phi[6] =  ReduceC * Crat * hdtodx1 * (1.0 - Ci1) * f11i1;
    phi[7] = -ReduceC * Crat * hdtodx1 * (1.0 - Ci1) * alphai1max;
    phi[8] = ReduceC * Crat * hdtodx2 * (1.0 - Cj1) * f21j1;
    phi[9] = -ReduceC * Crat * hdtodx2 * (1.0 - Cj1) * alphaj1max;


    psi[0] = -ReduceC * Crat * hdtodx2 * (1.0 + Cj0) * f22j0;
    psi[1] = -ReduceC * Crat * hdtodx2 * (1.0 + Cj0) * alphaj0;
    psi[2] = -ReduceC * Crat * hdtodx1 * (1.0 + Ci0) * f21i0;
    psi[3] = -ReduceC * Crat * hdtodx1 * (1.0 + Ci0) * alphai0;
    psi[4] = ReduceC * Crat * hdtodx1 * (Ci0 + Ci1) * f21
      + ReduceC * Crat * hdtodx2 * (Cj0 + Cj1) * f22
      - ReduceC * Crat * dt * (Sigma_aF + Sigma_sF) * vFyFull
      + Eratio * ReduceC * dt * Sigma_aE * vy;
    psi[5] = 1.0 + ReduceC * Crat * hdtodx1 * (1.0 + Ci1) * alphai +  ReduceC * Crat * hdtodx1 * (1.0 - Ci0) * alphaimax
      + ReduceC * Crat * hdtodx2 * (1.0 + Cj1) * alphaj +  ReduceC * Crat * hdtodx2 * (1.0 - Cj0) * alphajmax
      + ReduceC * Crat * dt * (Sigma_aF + Sigma_sF);
    psi[6] =  ReduceC * Crat * hdtodx1 * (1.0 - Ci1) * f21i1;
    psi[7] = -ReduceC * Crat * hdtodx1 * (1.0 - Ci1) * alphai1max;
    psi[8] =  ReduceC * Crat * hdtodx2 * (1.0 - Cj1) * f22j1;
    psi[9] = -ReduceC * Crat * hdtodx2 * (1.0 - Cj1) * alphaj1max;




  }
  else if(DIM == 3){
/* Assuming the velocity is already the original velocity in case of FARGO */
/* k - 1*/


    theta[0] = -ReduceC * Crat * hdtodx3 * (1.0 + Ck0) * (alphak0 - vFzk0Full) - ReduceC * hdtodx3 * vFzk0;
    theta[1] = -ReduceC * Crat * hdtodx3 * (1.0 + Ck0);
    theta[2] = -ReduceC * Crat * hdtodx2 * (1.0 + Cj0) * (alphaj0 - vFyj0Full) - ReduceC * hdtodx2 * vFyj0;
    theta[3] = -ReduceC * Crat * hdtodx2 * (1.0 + Cj0);
    theta[4] = -ReduceC * Crat * hdtodx1 * (1.0 + Ci0) * (alphai0 - vFxi0Full) - ReduceC * hdtodx1 * vFxi0;
    theta[5] = -ReduceC * Crat * hdtodx1 * (1.0 + Ci0);
    theta[6] = 1.0 + ReduceC * Crat * hdtodx1 * (1.0 + Ci1) * (alphai - vFxFull)
      + ReduceC * Crat * hdtodx1 * (1.0 - Ci0) * (alphaimax + vFxFull)
      + ReduceC * Crat * hdtodx2 * (1.0 + Cj1) * (alphaj - vFyFull)
      + ReduceC * Crat * hdtodx2 * (1.0 - Cj0) * (alphajmax + vFyFull)
      + ReduceC * Crat * hdtodx3 * (1.0 + Ck1) * (alphak - vFzFull)
      + ReduceC * Crat * hdtodx3 * (1.0 - Ck0) * (alphakmax + vFzFull)
      + ReduceC * dt * (Sigma_aF - Sigma_sF) * (vx * vFxFull + vy * vFyFull + vz * vFzFull)
      + Eratio * ReduceC * Crat * dt * Sigma_aE;
    theta[7] = ReduceC * Crat * hdtodx1 * (Ci0 + Ci1) - ReduceC * dt * (Sigma_aF - Sigma_sF) * vx;
    theta[8] = ReduceC * Crat * hdtodx2 * (Cj0 + Cj1) - ReduceC * dt * (Sigma_aF - Sigma_sF) * vy;
    theta[9] = ReduceC * Crat * hdtodx3 * (Ck0 + Ck1) - ReduceC * dt * (Sigma_aF - Sigma_sF) * vz;
    theta[10] = -ReduceC * Crat * hdtodx1 * (1.0 - Ci1) * (alphai1max + vFxi1Full) + ReduceC * hdtodx1 * vFxi1;
    theta[11] = ReduceC * Crat * hdtodx1 * (1.0 - Ci1);
    theta[12] = -ReduceC * Crat * hdtodx2 * (1.0 - Cj1) * (alphaj1max + vFyj1Full) + ReduceC * hdtodx2 * vFyj1;
    theta[13] = ReduceC * Crat * hdtodx2 * (1.0 - Cj1);
    theta[14] = -ReduceC * Crat * hdtodx3 * (1.0 - Ck1) * (alphak1max + vFzk1Full) + ReduceC * hdtodx3 * vFzk1;
    theta[15] = ReduceC * Crat * hdtodx3 * (1.0 - Ck1);

    phi[0] = -ReduceC * Crat * hdtodx3 * (1.0 + Ck0) * f31k0;
    phi[1] = -ReduceC * Crat * hdtodx3 * (1.0 + Ck0) * alphak0;
    phi[2] = -ReduceC * Crat * hdtodx2 * (1.0 + Cj0) * f21j0;
    phi[3] = -ReduceC * Crat * hdtodx2 * (1.0 + Cj0) * alphaj0;
    phi[4] = -ReduceC * Crat * hdtodx1 * (1.0 + Ci0) * f11i0;
    phi[5] = -ReduceC * Crat * hdtodx1 * (1.0 + Ci0) * alphai0;
    phi[6] = ReduceC * Crat * hdtodx1 * (Ci0 + Ci1) * f11
      + ReduceC * Crat * hdtodx2 * (Cj0 + Cj1) * f21
      + ReduceC * Crat * hdtodx3 * (Ck0 + Ck1) * f31
      - ReduceC * Crat * dt * (Sigma_aF + Sigma_sF) * vFxFull
      + Eratio * ReduceC * dt * Sigma_aE * vx;
    phi[7] = 1.0 + ReduceC * Crat * hdtodx1 * (1.0 + Ci1) * alphai +  ReduceC * Crat * hdtodx1 * (1.0 - Ci0) * alphaimax
      + ReduceC * Crat * hdtodx2 * (1.0 + Cj1) * alphaj +  ReduceC * Crat * hdtodx2 * (1.0 - Cj0) * alphajmax
      + ReduceC * Crat * hdtodx3 * (1.0 + Ck1) * alphak +  ReduceC * Crat * hdtodx3 * (1.0 - Ck0) * alphakmax
      + ReduceC * Crat * dt * (Sigma_aF + Sigma_sF);
    phi[8] =  ReduceC * Crat * hdtodx1 * (1.0 - Ci1) * f11i1;
    phi[9] = -ReduceC * Crat * hdtodx1 * (1.0 - Ci1) * alphai1max;
    phi[10] = ReduceC * Crat * hdtodx2 * (1.0 - Cj1) * f21j1;
    phi[11] = -ReduceC * Crat * hdtodx2 * (1.0 - Cj1) * alphaj1max;
    phi[12] =  ReduceC * Crat * hdtodx3 * (1.0 - Ck1) * f31k1;
    phi[13] = -ReduceC * Crat * hdtodx3 * (1.0 - Ck1) * alphak1max;

    psi[0] = -ReduceC * Crat * hdtodx3 * (1.0 + Ck0) * f32k0;
    psi[1] = -ReduceC * Crat * hdtodx3 * (1.0 + Ck0) * alphak0;
    psi[2] = -ReduceC * Crat * hdtodx2 * (1.0 + Cj0) * f22j0;
    psi[3] = -ReduceC * Crat * hdtodx2 * (1.0 + Cj0) * alphaj0;
    psi[4] = -ReduceC * Crat * hdtodx1 * (1.0 + Ci0) * f21i0;
    psi[5] = -ReduceC * Crat * hdtodx1 * (1.0 + Ci0) * alphai0;
    psi[6] = ReduceC * Crat * hdtodx1 * (Ci0 + Ci1) * f21
      + ReduceC * Crat * hdtodx2 * (Cj0 + Cj1) * f22
      + ReduceC * Crat * hdtodx3 * (Ck0 + Ck1) * f32
      - ReduceC * Crat * dt * (Sigma_aF + Sigma_sF) * vFyFull
      + Eratio * ReduceC * dt * Sigma_aE * vy;
    psi[7] = 1.0 + ReduceC * Crat * hdtodx1 * (1.0 + Ci1) * alphai +  ReduceC * Crat * hdtodx1 * (1.0 - Ci0) * alphaimax
      + ReduceC * Crat * hdtodx2 * (1.0 + Cj1) * alphaj +  ReduceC * Crat * hdtodx2 * (1.0 - Cj0) * alphajmax
      + ReduceC * Crat * hdtodx3 * (1.0 + Ck1) * alphak +  ReduceC * Crat * hdtodx3 * (1.0 - Ck0) * alphakmax
      + ReduceC * Crat * dt * (Sigma_aF + Sigma_sF);
    psi[8] =  ReduceC * Crat * hdtodx1 * (1.0 - Ci1) * f21i1;
    psi[9] = -ReduceC * Crat * hdtodx1 * (1.0 - Ci1) * alphai1max;
    psi[10] = ReduceC * Crat * hdtodx2 * (1.0 - Cj1) * f22j1;
    psi[11] = -ReduceC * Crat * hdtodx2 * (1.0 - Cj1) * alphaj1max;
    psi[12] = ReduceC * Crat * hdtodx3 * (1.0 - Ck1) * f32k1;
    psi[13] = -ReduceC * Crat * hdtodx3 * (1.0 - Ck1) * alphak1max;

    varphi[0] = -ReduceC * Crat * hdtodx3 * (1.0 + Ck0) * f33k0;
    varphi[1] = -ReduceC * Crat * hdtodx3 * (1.0 + Ck0) * alphak0;
    varphi[2] = -ReduceC * Crat * hdtodx2 * (1.0 + Cj0) * f32j0;
    varphi[3] = -ReduceC * Crat * hdtodx2 * (1.0 + Cj0) * alphaj0;
    varphi[4] = -ReduceC * Crat * hdtodx1 * (1.0 + Ci0) * f31i0;
    varphi[5] = -ReduceC * Crat * hdtodx1 * (1.0 + Ci0) * alphai0;
    varphi[6] = ReduceC * Crat * hdtodx1 * (Ci0 + Ci1) * f31
      + ReduceC * Crat * hdtodx2 * (Cj0 + Cj1) * f32
      + ReduceC * Crat * hdtodx3 * (Ck0 + Ck1) * f33
      - ReduceC * Crat * dt * (Sigma_aF + Sigma_sF) * vFzFull
      + Eratio * ReduceC * dt * Sigma_aE * vz;
    varphi[7] = 1.0 + ReduceC * Crat * hdtodx1 * (1.0 + Ci1) * alphai +  ReduceC * Crat * hdtodx1 * (1.0 - Ci0) * alphaimax
      + ReduceC * Crat * hdtodx2 * (1.0 + Cj1) * alphaj +  ReduceC * Crat * hdtodx2 * (1.0 - Cj0) * alphajmax
      + ReduceC * Crat * hdtodx3 * (1.0 + Ck1) * alphak +  ReduceC * Crat * hdtodx3 * (1.0 - Ck0) * alphakmax
      + ReduceC * Crat * dt * (Sigma_aF + Sigma_sF);
    varphi[8] =  ReduceC * Crat * hdtodx1 * (1.0 - Ci1) * f31i1;
    varphi[9] = -ReduceC * Crat * hdtodx1 * (1.0 - Ci1) * alphai1max;
    varphi[10] = ReduceC * Crat * hdtodx2 * (1.0 - Cj1) * f32j1;
    varphi[11] = -ReduceC * Crat * hdtodx2 * (1.0 - Cj1) * alphaj1max;
    varphi[12] = ReduceC * Crat * hdtodx3 * (1.0 - Ck1) * f33k1;
    varphi[13] = -ReduceC * Crat * hdtodx3 * (1.0 - Ck1) * alphak1max;

  }
  else{
    printf("Wrong Dim in matrix_coef: %d\n",DIM);

  }

}


#ifdef FLD



/* Function to calculate matrix coefficient */

void matrix_coef_FLD(const MatrixS *pMat, const int DIM, const int i, const int j, const int k, Real *theta)
{
/* The FLD equation is theta0 Ek-1 + theta1 * Ej-1 + theta2 * Ei-1 + theta3 Ei + theta4 Ei+1 + theta5 Ej+1 + theta6 Ek+1 = Eijk */
/* We always use multigrid to do this */

/* Average the diffusion coefficient */
  Real diffi0, diffi1, diffj0, diffj1, diffk0, diffk1;
  diffi0 = 0.5 * (pMat->Ugas[k][j][i].lambda/(pMat->Ugas[k][j][i].Sigma[0] + pMat->Ugas[k][j][i].Sigma[1])
                  + pMat->Ugas[k][j][i-1].lambda/(pMat->Ugas[k][j][i-1].Sigma[0] + pMat->Ugas[k][j][i-1].Sigma[1]));
  diffi1 = 0.5 * (pMat->Ugas[k][j][i].lambda/(pMat->Ugas[k][j][i].Sigma[0] + pMat->Ugas[k][j][i].Sigma[1])
                  + pMat->Ugas[k][j][i+1].lambda/(pMat->Ugas[k][j][i+1].Sigma[0] + pMat->Ugas[k][j][i+1].Sigma[1]));

  diffj0 = 0.5 * (pMat->Ugas[k][j][i].lambda/(pMat->Ugas[k][j][i].Sigma[0] + pMat->Ugas[k][j][i].Sigma[1])
                  + pMat->Ugas[k][j-1][i].lambda/(pMat->Ugas[k][j-1][i].Sigma[0] + pMat->Ugas[k][j-1][i].Sigma[1]));
  diffj1 = 0.5 * (pMat->Ugas[k][j][i].lambda/(pMat->Ugas[k][j][i].Sigma[0] + pMat->Ugas[k][j][i].Sigma[1])
                  + pMat->Ugas[k][j+1][i].lambda/(pMat->Ugas[k][j+1][i].Sigma[0] + pMat->Ugas[k][j+1][i].Sigma[1]));

  if(DIM == 3){

    diffk0 = 0.5 * (pMat->Ugas[k][j][i].lambda/(pMat->Ugas[k][j][i].Sigma[0] + pMat->Ugas[k][j][i].Sigma[1])
                    + pMat->Ugas[k-1][j][i].lambda/(pMat->Ugas[k-1][j][i].Sigma[0] + pMat->Ugas[k-1][j][i].Sigma[1]));
    diffk1 = 0.5 * (pMat->Ugas[k][j][i].lambda/(pMat->Ugas[k][j][i].Sigma[0] + pMat->Ugas[k][j][i].Sigma[1])
                    + pMat->Ugas[k+1][j][i].lambda/(pMat->Ugas[k+1][j][i].Sigma[0] + pMat->Ugas[k+1][j][i].Sigma[1]));
  }

/* Now the velocity dependent part Div(vEr + vPr) */
  Real hdtodx1, hdtodx2, hdtodx3, dt, dx, dy, dz;
  Real dvxdx, dvydy, dvzdz;

  Real vxi0, vxi1, vxj0, vxj1, vxk0, vxk1, vyi0, vyi1, vyj0, vyj1, vyk0, vyk1, vzi0, vzi1, vzj0, vzj1, vzk0, vzk1;

  Real f11, f22, f33, f11i0, f11i1, f22j0, f22j1, f33k0, f33k1;
  Real f21, f31, f32, f21j0, f21j1, f21i0, f21i1, f31k0, f31k1, f31i0, f31i1, f32j0, f32j1, f32k0, f32k1;

  Real vFxi0, vFxi1, vFyj0, vFyj1, vFzk0, vFzk1;
  Real Sigma_aE;
  Sigma_aE = pMat->Ugas[k][j][i].Sigma[3];

  hdtodx1 = 0.5 * pMat->dt/pMat->dx1;
  hdtodx2 = 0.5 * pMat->dt/pMat->dx2;
  hdtodx3 = 0.5 * pMat->dt/pMat->dx3;


  dx = pMat->dx1;
  dy = pMat->dx2;
  dz = pMat->dx3;
  dt = pMat->dt;

/* Assume the velocity already includes background shearing */

  vxi0 = pMat->Ugas[k][j][i-1].V1;
  vxi1 = pMat->Ugas[k][j][i+1].V1;



  vyi0 = pMat->Ugas[k][j][i-1].V2;
  vyi1 = pMat->Ugas[k][j][i+1].V2;

  vxj0 = pMat->Ugas[k][j-1][i].V1;
  vxj1 = pMat->Ugas[k][j+1][i].V1;

  vyj0 = pMat->Ugas[k][j-1][i].V2;
  vyj1 = pMat->Ugas[k][j+1][i].V2;


  vzi0 = pMat->Ugas[k][j][i-1].V3;
  vzi1 = pMat->Ugas[k][j][i+1].V3;

  vzj0 = pMat->Ugas[k][j-1][i].V3;
  vzj1 = pMat->Ugas[k][j+1][i].V3;

  if(DIM == 3){

    vzk0 = pMat->Ugas[k-1][j][i].V3;
    vzk1 = pMat->Ugas[k+1][j][i].V3;


    vxk0 = pMat->Ugas[k-1][j][i].V1;
    vxk1 = pMat->Ugas[k+1][j][i].V1;

    vyk0 = pMat->Ugas[k-1][j][i].V2;
    vyk1 = pMat->Ugas[k+1][j][i].V2;

  }

  f11i0 = pMat->Ugas[k][j][i-1].Edd_11;
  f21i0 = pMat->Ugas[k][j][i-1].Edd_21;
  f31i0 = pMat->Ugas[k][j][i-1].Edd_31;

  f11 = pMat->Ugas[k][j][i].Edd_11;
  f22 = pMat->Ugas[k][j][i].Edd_22;
  f33 = pMat->Ugas[k][j][i].Edd_33;

  f21 = pMat->Ugas[k][j][i].Edd_21;
  f31 = pMat->Ugas[k][j][i].Edd_31;
  f32 = pMat->Ugas[k][j][i].Edd_32;

  f11i1 = pMat->Ugas[k][j][i+1].Edd_11;
  f21i1 = pMat->Ugas[k][j][i+1].Edd_21;
  f31i1 = pMat->Ugas[k][j][i+1].Edd_31;

  f32j0 = pMat->Ugas[k][j-1][i].Edd_32;
  f22j0 = pMat->Ugas[k][j-1][i].Edd_22;
  f21j0 = pMat->Ugas[k][j-1][i].Edd_21;

  f21j1 = pMat->Ugas[k][j+1][i].Edd_21;
  f22j1 = pMat->Ugas[k][j+1][i].Edd_22;
  f32j1 = pMat->Ugas[k][j+1][i].Edd_32;

  if(DIM == 3){

    f33k0 = pMat->Ugas[k-1][j][i].Edd_33;
    f32k0 = pMat->Ugas[k-1][j][i].Edd_32;
    f31k0 = pMat->Ugas[k-1][j][i].Edd_31;

    f31k1 = pMat->Ugas[k+1][j][i].Edd_31;
    f32k1 = pMat->Ugas[k+1][j][i].Edd_32;
    f33k1 = pMat->Ugas[k+1][j][i].Edd_33;
  }

  if(DIM == 2){
    vFxi0 = f11i0 * vxi0 + vyi0 * f21i0;
    vFxi1 = f11i1 * vxi1 + vyi1 * f21i1;


    vFyj0 = f22j0 * vyj0 + vxj0 * f21j0;
    vFyj1 = f22j1 * vyj1 + vxj1 * f21j1;


  }
  else if(DIM == 3){
    vFxi0 = f11i0 * vxi0 + vyi0 * f21i0 + vzi0 * f31i0;
    vFxi1 = f11i1 * vxi1 + vyi1 * f21i1 + vzi1 * f31i1;


    vFyj0 = f22j0 * vyj0 + vxj0 * f21j0 + vzj0 * f32j0;
    vFyj1 = f22j1 * vyj1 + vxj1 * f21j1 + vzj1 * f32j1;


    vFzk0 = f33k0 * vzk0 + vxk0 * f31k0 + vyk0 * f32k0;
    vFzk1 = f33k1 * vzk1 + vxk1 * f31k1 + vyk1 * f32k1;
  }

  if(DIM == 2){

    theta[0] = -Crat * diffj0 * dt / (dy * dy);
    theta[1] = -Crat * diffi0 * dt / (dx * dx);
    theta[2] = 1.0 + Crat * (diffi0 + diffi1) * dt /(dx * dx) + Crat * (diffj0 + diffj1) * dt / (dy * dy) + Eratio * Crat * dt * Sigma_aE;
    theta[3] = -Crat * diffi1 * dt / (dx * dx);
    theta[4] = -Crat * diffj1 * dt / (dy * dy);

  }
  else if(DIM == 3){

    theta[0] = -Crat * diffk0 * dt / (dz * dz);
    theta[1] = -Crat * diffj0 * dt / (dy * dy);
    theta[2] = -Crat * diffi0 * dt / (dx * dx);
    theta[3] = 1.0 + Crat * (diffi0 + diffi1) * dt /(dx * dx) + Crat * (diffj0 + diffj1) * dt / (dy * dy)
      + Crat * (diffk0 + diffk1) * dt / (dz * dz) + Eratio * Crat * dt * Sigma_aE;
    theta[4] = -Crat * diffi1 * dt / (dx * dx);
    theta[5] = -Crat * diffj1 * dt / (dy * dy);
    theta[6] = -Crat * diffk1 * dt / (dz * dz);
  }
}



#endif /* FLD */


/* The reduced factor for radiation subsystem */
/* Eddington tensor is included here */
/* Absolution value of minimum velocity, actual value should include velocity */
void matrix_alpha(const Real direction, const Real *Sigma, const Real dt, const Real Edd, const Real velocity, Real *alpha, int flag, Real dl){

/* Edd is the Eddington tensor, do not include velocity dependence part*/
/* dx is cell size */
/* if flag = 1, for maximum velocity. if flag = -1, for minimum velocity */

  Real Sigma_aF, Sigma_sF, Sigma_aE, tau, Sigma_t, taucell;
  Real reducefactor;


  Sigma_sF = Sigma[0];
  Sigma_aF = Sigma[1];
  Sigma_aE = Sigma[3];
/* reduce the speed for even pure absorption opacity to reduce diffusion */
/* Be careful to use conservative scheme for pure absorption opacity */
  Sigma_t = Sigma_sF + Sigma_aF;
/* just reduce speed to cancel numerical diffusion, even do this for pure absorption opacity */

/*      Sigma_t -= Sigma_aE;
 */

/* use 10 times optical depth per cell */
  taucell = Taufactor * dl * Sigma_t;

/* In optical thin regime, do not include the factor 10, to be consistent with linear analysis */
/*      if(dl * Sigma_t < 0.1)
        taucell = dl * Sigma_t;
*/
/* Use optical depth per cell, so reduced speed of light does not affect tau anymore */
/*
  tau = dt * Crat * Sigma_t;
*/
/*
  if(tau > taucell)
  tau = taucell;
*/

  tau = taucell;

  tau = tau * tau / (2.0 * Edd);

  if(tau > 0.001)
    reducefactor = sqrt(Edd * (1.0 - exp(- tau)) / tau);
  else
    reducefactor = sqrt(Edd * (1.0 - 0.5 * tau));


/*
  reducefactor += (1.0 + Edd) * fabs(velocity) / Crat;
*/

  *alpha = reducefactor;

}

#ifdef FLD

/* The reduced factor for radiation subsystem */
/* Eddington tensor is included here */
/* Absolution value of minimum velocity, actual value should include velocity */
void FLD_limiter(const Real divEr, const Real Er, const Real Sigma, Real *lambda)
{
  Real FLDR, limiter;
  Real beta = 1.e-4;
  FLDR = (fabs(divEr)/Er + beta)/Sigma;
  if(FLDR < 1.e-6)
    limiter = 1.0/3.0 - FLDR * FLDR/45.0;
  else
    limiter = (1.0/tanh(FLDR) - 1.0/FLDR)/FLDR;

  (*lambda) = limiter;

}

#endif /* FLD */

#ifdef MATRIX_MULTIGRID

void vector_product(const Real *v1, const Real *v2, const int dim, Real *result)
{
/* calculate the inner produce of two vectors (v1, v2) */
  int i;
  Real temp;
  temp = 0.0;

  for(i=0; i<dim; i++){
    temp += v1[i] * v2[i];
  }

  *result = temp;

}
#endif

#endif
/* end radiation_hydro or radiation_MHD */

#if defined(RADIATION_HYDRO) || defined(RADIATION_MHD) || defined(FULL_RADIATION_TRANSFER)


/* Function to calculate the equilibrium state due to Compton scattering */
void Tcompton(double T, double coef1, double coef2, double coef3, double coef4, double * fval, double *dfval)
{

/* function is
 *  coef1 * T^8 + coef2 * T^5 + coef3 * T^4 + coef4 == 0 *
 */

  *fval = coef1 * pow(T, 8.0) + coef2 * pow(T,5.0) + coef3 * pow(T, 4.0) + coef4;
  *dfval = 8.0 * coef1 * pow(T, 7.0) + 5.0 * coef2 * pow(T, 4.0) + 4.0 * coef3 * pow(T,3.0);

  return;
}



/* Function to find the equilibrium state */
void Tequilibrium(double T, double coef1, double coef2, double coef3, double coef4, double * fval, double *dfval)
{

/* function is
 *  coef1 * T^4 + coef2 * T + coef3 == 0 *
 */

  *fval = coef1 * pow(T, 4.0) + coef2 * T + coef3;
  *dfval = 4.0 * coef1 * pow(T, 3.0) + coef2;

  return;
}

/* This function solve the equations: *
 * rho R dT/dt  /(gamma-1) = -Prat * Crat * (Sigma_aP T^4 - Sigma_aE * Er);
 * dEr/dt = Crat * (Sigma_aP * T^4 - Sigma_aE * Er)
 * Sigma_aP and Sigma_aE are assumed to be a constant, for first order accurate
 */
 /* The reduced speed factor only appear in radiation equation */

void ThermalRelaxation(const Real Tg0, const Real Er0, const Real density, const Real Sigma_aP, const Real Sigma_aE, const Real dt, Real *Tg, Real *Er)
{

  Real coef1, coef2, coef3, coef4, Ersum, pressure, TEr;
  Real kappaP, kappaE;
  Real Tnew, Ernew;

  if(Prat < TINY_NUMBER){
    if(Tg != NULL)
      *Tg = Tg0;
    if(Er != NULL)
      *Er = Er0;
    return;
  }

  pressure = Tg0 * density * R_ideal;
  Ersum = pressure / (Gamma - 1.0) + Prat * Er0 / ReduceC;
  TEr = pow(Er0, 0.25);

/* Here assume input gas temperature and Er0 is positive */
  if((Tg0 < 0.0) || (Er0 < 0.0))
    ath_error("[ThemralRelaxation]: Negative gas temperature: %e or Radiation energy density: %e!n\n",Tg0,Er0);



  coef1 = dt * Prat * Crat * Sigma_aP;
  coef2 = density * R_ideal * (1.0 + dt * Sigma_aE * Crat * ReduceC) / (Gamma - 1.0);
  coef3 = -pressure / (Gamma - 1.0) - dt * Sigma_aE * Crat * ReduceC * Ersum;
  coef4 = 0.0;

  if(coef1 < 1.e-20){
    Tnew = -coef3 / coef2;
  }
  else{


    if(Tg0 > TEr){
      Tnew = rtsafe(Tequilibrium, TEr * (1.0 - 0.01), Tg0 * (1.0 + 0.01), 1.e-12, coef1, coef2, coef3,coef4);
    }
    else{
      Tnew = rtsafe(Tequilibrium, Tg0 * (1.0 - 0.01), TEr * (1.0 + 0.01), 1.e-12, coef1, coef2, coef3, coef4);
    }
  }

  Ernew = (Ersum - density * R_ideal * Tnew / (Gamma - 1.0)) * ReduceC / Prat;

  if(Tg != NULL)
    *Tg = Tnew;
  if(Er != NULL)
    *Er = Ernew;

  return;


}




/* Newton method to find root, which is taken from numerical recipes */

double rtsafe(void (*funcd)(double, double, double, double, double, double *, double *), double x1, double x2,
              double xacc, double coef1, double coef2, double coef3, double coef4)
{
  int j;
  double df,dx,dxold,f,fh,fl;
  double temp,xh,xl,rts;

  int maxit = 400;

  (*funcd)(x1,coef1, coef2, coef3,coef4, &fl,&df);
  (*funcd)(x2,coef1, coef2, coef3,coef4, &fh,&df);
  if ((fl > 0.0 && fh > 0.0) || (fl < 0.0 && fh < 0.0))
    ath_error("[rtsafe]:Root must be bracketed in rtsafe: Tl: %13.6e Th: %13.6e\n fl: %13.6e\n fh: %13.6e\n",x1, x2, fl, fh);
  if (fl == 0.0) return x1;
  if (fh == 0.0) return x2;
  if (fl < 0.0) {
    xl=x1;
    xh=x2;
  } else {
    xh=x1;
    xl=x2;
  }
  rts=0.5*(x1+x2);
  dxold=fabs(x2-x1);
  dx=dxold;
  (*funcd)(rts,coef1, coef2, coef3,coef4, &f,&df);
  for (j=1;j<=maxit;j++) {
    if ((((rts-xh)*df-f)*((rts-xl)*df-f) > 0.0)
        || (fabs(2.0*f) > fabs(dxold*df))) {
      dxold=dx;
      dx=0.5*(xh-xl);
      rts=xl+dx;
      if (xl == rts) return rts;
    } else {
      dxold=dx;
      dx=f/df;
      temp=rts;
      rts -= dx;
      if (temp == rts) return rts;
    }
    if (fabs(dx) < xacc) return rts;
    (*funcd)(rts,coef1, coef2, coef3,coef4, &f,&df);
    if (f < 0.0)
      xl=rts;
    else
      xh=rts;
  }
  ath_error("[rtsafe]:Maximum number of iterations exceeded in rtsafe: x1: %e x2: %e coef1: %e coef2: %e coef3: %e coef4: %e\n",x1,x2,coef1,coef2,coef3,coef4);

  return 0.0;
}


#endif

