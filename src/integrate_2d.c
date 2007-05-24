#include "copyright.h"
/*==============================================================================
 * FILE: integrate_2d.c
 *
 * PURPOSE: Updates the input Grid structure pointed to by *pGrid by one 
 *   timestep using directionally unsplit CTU method of Colella (1990).  The
 *   variables updated are:
 *      U.[d,M1,M2,M3,E,B1c,B2c,B3c,s] -- where U is of type Gas
 *      B1i, B2i -- interface magnetic field
 *   Also adds gravitational source terms, self-gravity, and H-correction
 *   of Sanders et al.
 *
 * REFERENCES:
 *   P. Colella, "Multidimensional upwind methods for hyperbolic conservation
 *   laws", JCP, 87, 171 (1990)
 *
 *   T. Gardiner & J.M. Stone, "An unsplit Godunov method for ideal MHD via
 *   constrained transport", JCP, 205, 509 (2005)
 *
 *   R. Sanders, E. Morano, & M.-C. Druguet, "Multidimensinal dissipation for
 *   upwind schemes: stability and applications to gas dynamics", JCP, 145, 511
 *   (1998)
 *
 * CONTAINS PUBLIC FUNCTIONS: 
 *   integrate_2d()
 *   integrate_init_2d()
 *   integrate_destruct_2d()
 *============================================================================*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "athena.h"
#include "globals.h"
#include "prototypes.h"

/* The L/R states of conserved variables and fluxes at each cell face */
static Cons1D **Ul_x1Face=NULL, **Ur_x1Face=NULL;
static Cons1D **Ul_x2Face=NULL, **Ur_x2Face=NULL;
static Cons1D **x1Flux=NULL, **x2Flux=NULL;

/* The interface magnetic fields and emfs */
static Real **B1_x1Face=NULL, **B2_x2Face=NULL;
#ifdef MHD
static Real **emf3=NULL, **emf3_cc=NULL;
#endif /* MHD */

/* 1D scratch vectors used by lr_states and flux functions */
static Real *Bxc=NULL, *Bxi=NULL;
static Prim1D *W=NULL, *Wl=NULL, *Wr=NULL;
static Cons1D *U1d=NULL, *Ul=NULL, *Ur=NULL;

/* density at t^{n+1/2} needed by both MHD and to make gravity 2nd order */
static Real **dhalf = NULL;

/* variables needed for H-correction of Sanders et al (1998) */
extern Real etah;
#ifdef H_CORRECTION
static Real **eta1=NULL, **eta2=NULL;
#endif

/*==============================================================================
 * PRIVATE FUNCTION PROTOTYPES: 
 *   integrate_emf3_corner() - the upwind CT method in Gardiner & Stone (2005) 
 *============================================================================*/

#ifdef MHD
static void integrate_emf3_corner(Grid *pGrid);
#endif

/*=========================== PUBLIC FUNCTIONS ===============================*/
/*----------------------------------------------------------------------------*/
/* integrate_2d:  CTU integrator in 2D  */

void integrate_2d(Grid *pGrid)
{
  Real dtodx1 = pGrid->dt/pGrid->dx1, dtodx2 = pGrid->dt/pGrid->dx2;
  Real hdt = 0.5*pGrid->dt;
  int is = pGrid->is, ie = pGrid->ie;
  int js = pGrid->js, je = pGrid->je;
  int ks = pGrid->ks;
  int i,il,iu;
  int j,jl,ju;
#ifdef MHD
  Real MHD_src,dbx,dby,B1,B2,B3,V3;
  Real d, M1, M2, B1c, B2c;
#endif
#ifdef H_CORRECTION
  Real cfr,cfl,ur,ul;
#endif
#if (NSCALARS > 0)
  int n;
#endif
  Real qa,pb,x1,x2,x3,phicl,phicr,phifc,phil,phir,phic;

  il = is - 2;
  iu = ie + 2;

  jl = js - 2;
  ju = je + 2;

/*--- Step 1a ------------------------------------------------------------------
 * Load 1D vector of conserved variables;  U1d = (d, M1, M2, M3, E, B2c, B3c)
 */

  for (j=jl; j<=ju; j++) {
    for (i=is-nghost; i<=ie+nghost; i++) {
      U1d[i].d  = pGrid->U[ks][j][i].d;
      U1d[i].Mx = pGrid->U[ks][j][i].M1;
      U1d[i].My = pGrid->U[ks][j][i].M2;
      U1d[i].Mz = pGrid->U[ks][j][i].M3;
#ifndef ISOTHERMAL
      U1d[i].E  = pGrid->U[ks][j][i].E;
#endif /* ISOTHERMAL */
#ifdef MHD
      U1d[i].By = pGrid->U[ks][j][i].B2c;
      U1d[i].Bz = pGrid->U[ks][j][i].B3c;
      Bxc[i] = pGrid->U[ks][j][i].B1c;
      Bxi[i] = pGrid->B1i[ks][j][i];
      B1_x1Face[j][i] = pGrid->B1i[ks][j][i];
#endif /* MHD */
#if (NSCALARS > 0)
      for (n=0; n<NSCALARS; n++) U1d[i].s[n] = pGrid->U[ks][j][i].s[n];
#endif
    }

/*--- Step 1b ------------------------------------------------------------------
 * Compute L and R states at X1-interfaces.
 */

    for (i=is-nghost; i<=ie+nghost; i++) {
      pb = Cons1D_to_Prim1D(&U1d[i],&W[i],&Bxc[i]);
    }
    lr_states(W,Bxc,pGrid->dt,dtodx1,is-1,ie+1,Wl,Wr);

/*--- Step 1c ------------------------------------------------------------------
 * Add "MHD source terms" for 0.5*dt
 */

#ifdef MHD
    for (i=is-1; i<=iu; i++) {
      MHD_src = (pGrid->U[ks][j][i-1].M2/pGrid->U[ks][j][i-1].d)*
               (pGrid->B1i[ks][j][i] - pGrid->B1i[ks][j][i-1])/pGrid->dx1;
      Wl[i].By += hdt*MHD_src;

      MHD_src = (pGrid->U[ks][j][i].M2/pGrid->U[ks][j][i].d)*
               (pGrid->B1i[ks][j][i+1] - pGrid->B1i[ks][j][i])/pGrid->dx1;
      Wr[i].By += hdt*MHD_src;
    }
#endif

/*--- Step 1d ------------------------------------------------------------------
 * Add gravitational source terms from static potential for 0.5*dt to L/R states
 */

    if (StaticGravPot != NULL){
      for (i=is-1; i<=iu; i++) {

/* Calculate the potential at i [phi-center-right],i-1 [phi-center-left],
 * and i-1/2 [phi-face] */
        cc_pos(pGrid,i,j,ks,&x1,&x2,&x3);
        phicr = (*StaticGravPot)( x1                ,x2,x3);
        phicl = (*StaticGravPot)((x1-    pGrid->dx1),x2,x3);
        phifc = (*StaticGravPot)((x1-0.5*pGrid->dx1),x2,x3);

/* Apply gravitational source terms to velocity using gradient of potential.
 * for (dt/2).   S_{V} = -Grad(Phi) */
        Wl[i].Vx -= dtodx1*(phifc - phicl);
        Wr[i].Vx -= dtodx1*(phicr - phifc);
      }
    }

/*--- Step 1e ------------------------------------------------------------------
 * Compute 1D fluxes in x1-direction, storing into 2D array
 */

    for (i=is-1; i<=iu; i++) {
      pb = Prim1D_to_Cons1D(&Ul_x1Face[j][i],&Wl[i],&Bxi[i]);
      pb = Prim1D_to_Cons1D(&Ur_x1Face[j][i],&Wr[i],&Bxi[i]);
    }

    for (i=is-1; i<=iu; i++) {
      GET_FLUXES(B1_x1Face[j][i],Ul_x1Face[j][i],Ur_x1Face[j][i],&x1Flux[j][i]);
    }
  }

/*--- Step 2a ------------------------------------------------------------------
 * Load 1D vector of conserved variables;  U1d = (d, M2, M3, M1, E, B3c, B1c)
 */

  for (i=il; i<=iu; i++) {
    for (j=js-nghost; j<=je+nghost; j++) {
      U1d[j].d  = pGrid->U[ks][j][i].d;
      U1d[j].Mx = pGrid->U[ks][j][i].M2;
      U1d[j].My = pGrid->U[ks][j][i].M3;
      U1d[j].Mz = pGrid->U[ks][j][i].M1;
#ifndef ISOTHERMAL
      U1d[j].E  = pGrid->U[ks][j][i].E;
#endif /* ISOTHERMAL */
#ifdef MHD
      U1d[j].By = pGrid->U[ks][j][i].B3c;
      U1d[j].Bz = pGrid->U[ks][j][i].B1c;
      Bxc[j] = pGrid->U[ks][j][i].B2c;
      Bxi[j] = pGrid->B2i[ks][j][i];
      B2_x2Face[j][i] = pGrid->B2i[ks][j][i];
#endif /* MHD */
#if (NSCALARS > 0)
      for (n=0; n<NSCALARS; n++) U1d[j].s[n] = pGrid->U[ks][j][i].s[n];
#endif
    }

/*--- Step 2b ------------------------------------------------------------------
 * Compute L and R states at X2-interfaces.
 */

    for (j=js-nghost; j<=je+nghost; j++) {
      pb = Cons1D_to_Prim1D(&U1d[j],&W[j],&Bxc[j]);
    }
    lr_states(W,Bxc,pGrid->dt,dtodx2,js-1,je+1,Wl,Wr);

/*--- Step 2c ------------------------------------------------------------------
 * Add "MHD source terms"
 */

#ifdef MHD
    for (j=js-1; j<=ju; j++) {
      MHD_src = (pGrid->U[ks][j-1][i].M1/pGrid->U[ks][j-1][i].d)*
        (pGrid->B2i[ks][j][i] - pGrid->B2i[ks][j-1][i])/pGrid->dx2;
      Wl[j].Bz += hdt*MHD_src;

      MHD_src = (pGrid->U[ks][j][i].M1/pGrid->U[ks][j][i].d)*
        (pGrid->B2i[ks][j+1][i] - pGrid->B2i[ks][j][i])/pGrid->dx2;
      Wr[j].Bz += hdt*MHD_src;
    }
#endif

/*--- Step 2d ------------------------------------------------------------------
 * Add gravitational source terms from static potential for 0.5*dt to L/R states
 */
  
    if (StaticGravPot != NULL){
      for (j=js-1; j<=ju; j++) {

/* Calculate the potential at j [phi-center-right],j-1 [phi-center-left],
 * and j-1/2 [phi-face] */
        cc_pos(pGrid,i,j,ks,&x1,&x2,&x3);
        phicr = (*StaticGravPot)(x1, x2                ,x3);
        phicl = (*StaticGravPot)(x1,(x2-    pGrid->dx2),x3);
        phifc = (*StaticGravPot)(x1,(x2-0.5*pGrid->dx2),x3);

/* Apply gravitational source terms to velocity using gradient of potential.
 * for (dt/2).   S_{V} = -Grad(Phi) */
        Wl[j].Vx -= dtodx2*(phifc - phicl);
        Wr[j].Vx -= dtodx2*(phicr - phifc);
      }
    }

    for (j=js-1; j<=ju; j++) {
      pb = Prim1D_to_Cons1D(&Ul_x2Face[j][i],&Wl[j],&Bxi[j]);
      pb = Prim1D_to_Cons1D(&Ur_x2Face[j][i],&Wr[j],&Bxi[j]);
    }
  }

/*--- Step 2e ------------------------------------------------------------------
 * Compute 1D fluxes in x2-direction, storing into 2D array
 */

  for (j=js-1; j<=ju; j++) {
    for (i=il; i<=iu; i++) {
      GET_FLUXES(B2_x2Face[j][i],Ul_x2Face[j][i],Ur_x2Face[j][i],&x2Flux[j][i]);
    }
  }

/*--- Step 3 ------------------------------------------------------------------
 * Calculate the cell centered value of emf_3 at t^{n}
 */

#ifdef MHD
  for (j=jl; j<=ju; j++) {
    for (i=il; i<=iu; i++) {
      emf3_cc[j][i] =
	(pGrid->U[ks][j][i].B1c*pGrid->U[ks][j][i].M2 -
	 pGrid->U[ks][j][i].B2c*pGrid->U[ks][j][i].M1 )/pGrid->U[ks][j][i].d;
    }
  }

/*--- Step 4 ------------------------------------------------------------------
 * Integrate emf3 to the grid cell corners and then update the 
 * interface magnetic fields using CT for a half time step.
 */

  integrate_emf3_corner(pGrid);

  for (j=js-1; j<=je+1; j++) {
    for (i=is-1; i<=ie+1; i++) {
      B1_x1Face[j][i] -= 0.5*dtodx2*(emf3[j+1][i  ] - emf3[j][i]);
      B2_x2Face[j][i] += 0.5*dtodx1*(emf3[j  ][i+1] - emf3[j][i]);
    }
    B1_x1Face[j][iu] -= 0.5*dtodx2*(emf3[j+1][iu] - emf3[j][iu]);
  }
  for (i=is-1; i<=ie+1; i++) {
    B2_x2Face[ju][i] += 0.5*dtodx1*(emf3[ju][i+1] - emf3[ju][i]);
  }
#endif

/*--- Step 5a ------------------------------------------------------------------
 * Correct the L/R states at x1-interfaces using transverse flux-gradients in
 * the x2-direction for 0.5*dt using x2-fluxes computed in Step 2e.
 * Since the fluxes come from an x2-sweep, (x,y,z) on RHS -> (z,x,y) on LHS */

  qa = 0.5*dtodx2;
  for (j=js-1; j<=je+1; j++) {
    for (i=is-1; i<=iu; i++) {
      Ul_x1Face[j][i].d  -= qa*(x2Flux[j+1][i-1].d  - x2Flux[j][i-1].d );
      Ul_x1Face[j][i].Mx -= qa*(x2Flux[j+1][i-1].Mz - x2Flux[j][i-1].Mz);
      Ul_x1Face[j][i].My -= qa*(x2Flux[j+1][i-1].Mx - x2Flux[j][i-1].Mx);
      Ul_x1Face[j][i].Mz -= qa*(x2Flux[j+1][i-1].My - x2Flux[j][i-1].My);
#ifndef ISOTHERMAL
      Ul_x1Face[j][i].E  -= qa*(x2Flux[j+1][i-1].E  - x2Flux[j][i-1].E );
#endif /* ISOTHERMAL */
#ifdef MHD
      Ul_x1Face[j][i].Bz -= qa*(x2Flux[j+1][i-1].By - x2Flux[j][i-1].By);
#endif

      Ur_x1Face[j][i].d  -= qa*(x2Flux[j+1][i  ].d  - x2Flux[j][i  ].d );
      Ur_x1Face[j][i].Mx -= qa*(x2Flux[j+1][i  ].Mz - x2Flux[j][i  ].Mz);
      Ur_x1Face[j][i].My -= qa*(x2Flux[j+1][i  ].Mx - x2Flux[j][i  ].Mx);
      Ur_x1Face[j][i].Mz -= qa*(x2Flux[j+1][i  ].My - x2Flux[j][i  ].My);
#ifndef ISOTHERMAL
      Ur_x1Face[j][i].E  -= qa*(x2Flux[j+1][i  ].E  - x2Flux[j][i  ].E );
#endif /* ISOTHERMAL */
#ifdef MHD
      Ur_x1Face[j][i].Bz -= qa*(x2Flux[j+1][i  ].By - x2Flux[j][i  ].By);
#endif
#if (NSCALARS > 0)
      for (n=0; n<NSCALARS; n++) {
        Ul_x1Face[j][i].s[n] -=qa*(x2Flux[j+1][i-1].s[n] - x2Flux[j][i-1].s[n]);
        Ur_x1Face[j][i].s[n] -=qa*(x2Flux[j+1][i  ].s[n] - x2Flux[j][i  ].s[n]);
      }
#endif
    }
  }

/*--- Step 5b ------------------------------------------------------------------
 * Add the "MHD source terms" to the x2 (conservative) flux gradient.
 */

#ifdef MHD
  qa = 0.5*dtodx1;
  for (j=js-1; j<=je+1; j++) {
    for (i=is-1; i<=iu; i++) {
      dbx = pGrid->B1i[ks][j][i] - pGrid->B1i[ks][j][i-1];
      B1 = pGrid->U[ks][j][i-1].B1c;
      B2 = pGrid->U[ks][j][i-1].B2c;
      B3 = pGrid->U[ks][j][i-1].B3c;
      V3 = pGrid->U[ks][j][i-1].M3/pGrid->U[ks][j][i-1].d;

      Ul_x1Face[j][i].Mx += qa*B1*dbx;
      Ul_x1Face[j][i].My += qa*B2*dbx;
      Ul_x1Face[j][i].Mz += qa*B3*dbx;
      Ul_x1Face[j][i].Bz += qa*V3*dbx;
#ifndef ISOTHERMAL
      Ul_x1Face[j][i].E  += qa*B3*V3*dbx;
#endif /* ISOTHERMAL */

      dbx = pGrid->B1i[ks][j][i+1] - pGrid->B1i[ks][j][i];
      B1 = pGrid->U[ks][j][i].B1c;
      B2 = pGrid->U[ks][j][i].B2c;
      B3 = pGrid->U[ks][j][i].B3c;
      V3 = pGrid->U[ks][j][i].M3/pGrid->U[ks][j][i].d;

      Ur_x1Face[j][i].Mx += qa*B1*dbx;
      Ur_x1Face[j][i].My += qa*B2*dbx;
      Ur_x1Face[j][i].Mz += qa*B3*dbx;
      Ur_x1Face[j][i].Bz += qa*V3*dbx;
#ifndef ISOTHERMAL
      Ur_x1Face[j][i].E  += qa*B3*V3*dbx;
#endif /* ISOTHERMAL */
    }
  }
#endif /* MHD */

/*--- Step 5c ------------------------------------------------------------------
 * Add gravitational source terms in x2-direction to transverse flux-gradient
 * used to correct L/R states on x1-faces.
 *    S_{M} = -(\rho) Grad(Phi);   S_{E} = -(\rho v) Grad{Phi}
 */

  if (StaticGravPot != NULL){
    qa = 0.5*dtodx2;
    for (j=js-1; j<=je+1; j++) {
      for (i=is-1; i<=iu; i++) {
        cc_pos(pGrid,i,j,ks,&x1,&x2,&x3);
        phic = (*StaticGravPot)(x1, x2                ,x3);
        phir = (*StaticGravPot)(x1,(x2+0.5*pGrid->dx2),x3);
        phil = (*StaticGravPot)(x1,(x2-0.5*pGrid->dx2),x3);

        Ur_x1Face[j][i].My -= qa*(phir-phil)*pGrid->U[ks][j][i].d;
#ifndef ISOTHERMAL
        Ur_x1Face[j][i].E -= qa*(x2Flux[j  ][i  ].d*(phic - phil) +
                                 x2Flux[j+1][i  ].d*(phir - phic));
#endif

        phic = (*StaticGravPot)((x1-pGrid->dx1), x2                ,x3);
        phir = (*StaticGravPot)((x1-pGrid->dx1),(x2+0.5*pGrid->dx2),x3);
        phil = (*StaticGravPot)((x1-pGrid->dx1),(x2-0.5*pGrid->dx2),x3);
        
        Ul_x1Face[j][i].My -= qa*(phir-phil)*pGrid->U[ks][j][i-1].d;
#ifndef ISOTHERMAL
        Ul_x1Face[j][i].E -= qa*(x2Flux[j  ][i-1].d*(phic - phil) +
                                 x2Flux[j+1][i-1].d*(phir - phic));
#endif
      }
    }
  }

/*--- Step 6a ------------------------------------------------------------------
 * Correct the L/R states at x2-interfaces using transverse flux-gradients in
 * the x1-direction for 0.5*dt using x1-fluxes computed in Step 1e.
 * Since the fluxes come from an x1-sweep, (x,y,z) on RHS -> (y,z,x) on LHS */

  qa = 0.5*dtodx1;
  for (j=js-1; j<=ju; j++) {
    for (i=is-1; i<=ie+1; i++) {
      Ul_x2Face[j][i].d  -= qa*(x1Flux[j-1][i+1].d  - x1Flux[j-1][i].d );
      Ul_x2Face[j][i].Mx -= qa*(x1Flux[j-1][i+1].My - x1Flux[j-1][i].My);
      Ul_x2Face[j][i].My -= qa*(x1Flux[j-1][i+1].Mz - x1Flux[j-1][i].Mz);
      Ul_x2Face[j][i].Mz -= qa*(x1Flux[j-1][i+1].Mx - x1Flux[j-1][i].Mx);
#ifndef ISOTHERMAL
      Ul_x2Face[j][i].E  -= qa*(x1Flux[j-1][i+1].E  - x1Flux[j-1][i].E );
#endif /* ISOTHERMAL */
#ifdef MHD
      Ul_x2Face[j][i].By -= qa*(x1Flux[j-1][i+1].Bz - x1Flux[j-1][i].Bz);
#endif

      Ur_x2Face[j][i].d  -= qa*(x1Flux[j  ][i+1].d  - x1Flux[j  ][i].d );
      Ur_x2Face[j][i].Mx -= qa*(x1Flux[j  ][i+1].My - x1Flux[j  ][i].My);
      Ur_x2Face[j][i].My -= qa*(x1Flux[j  ][i+1].Mz - x1Flux[j  ][i].Mz);
      Ur_x2Face[j][i].Mz -= qa*(x1Flux[j  ][i+1].Mx - x1Flux[j  ][i].Mx);
#ifndef ISOTHERMAL
      Ur_x2Face[j][i].E  -= qa*(x1Flux[j  ][i+1].E  - x1Flux[j  ][i].E );
#endif /* ISOTHERMAL */
#ifdef MHD
      Ur_x2Face[j][i].By -= qa*(x1Flux[j][i+1].Bz - x1Flux[j][i].Bz);
#endif
#if (NSCALARS > 0)
      for (n=0; n<NSCALARS; n++) {
        Ul_x2Face[j][i].s[n] -=qa*(x1Flux[j-1][i+1].s[n] - x1Flux[j-1][i].s[n]);
        Ur_x2Face[j][i].s[n] -=qa*(x1Flux[j  ][i+1].s[n] - x1Flux[j  ][i].s[n]);
      }
#endif
    }
  }

/*--- Step 6b ------------------------------------------------------------------
 * Add the "MHD source terms" to the x1 (conservative) flux gradient.
 */

#ifdef MHD
  qa = 0.5*dtodx2;
  for (j=js-1; j<=ju; j++) {
    for (i=is-1; i<=ie+1; i++) {
      dby = pGrid->B2i[ks][j][i] - pGrid->B2i[ks][j-1][i];
      B1 = pGrid->U[ks][j-1][i].B1c;
      B2 = pGrid->U[ks][j-1][i].B2c;
      B3 = pGrid->U[ks][j-1][i].B3c;
      V3 = pGrid->U[ks][j-1][i].M3/pGrid->U[ks][j-1][i].d;

      Ul_x2Face[j][i].Mz += qa*B1*dby;
      Ul_x2Face[j][i].Mx += qa*B2*dby;
      Ul_x2Face[j][i].My += qa*B3*dby;
      Ul_x2Face[j][i].By += qa*V3*dby;
#ifndef ISOTHERMAL
      Ul_x2Face[j][i].E  += qa*B3*V3*dby;
#endif /* ISOTHERMAL */

      dby = pGrid->B2i[ks][j+1][i] - pGrid->B2i[ks][j][i];
      B1 = pGrid->U[ks][j][i].B1c;
      B2 = pGrid->U[ks][j][i].B2c;
      B3 = pGrid->U[ks][j][i].B3c;
      V3 = pGrid->U[ks][j][i].M3/pGrid->U[ks][j][i].d;

      Ur_x2Face[j][i].Mz += qa*B1*dby;
      Ur_x2Face[j][i].Mx += qa*B2*dby;
      Ur_x2Face[j][i].My += qa*B3*dby;
      Ur_x2Face[j][i].By += qa*V3*dby;
#ifndef ISOTHERMAL
      Ur_x2Face[j][i].E  += qa*B3*V3*dby;
#endif /* ISOTHERMAL */
    }
  }
#endif /* MHD */

/*--- Step 6c ------------------------------------------------------------------
 * Add gravitational source terms in x1-direction to transverse flux-gradient
 * used to correct L/R states on x2-faces.
 */

  if (StaticGravPot != NULL){
    qa = 0.5*dtodx1;
    for (j=js-1; j<=ju; j++) {
      for (i=is-1; i<=ie+1; i++) {
        cc_pos(pGrid,i,j,ks,&x1,&x2,&x3);
        phic = (*StaticGravPot)((x1               ),x2,x3);
        phir = (*StaticGravPot)((x1+0.5*pGrid->dx1),x2,x3);
        phil = (*StaticGravPot)((x1-0.5*pGrid->dx1),x2,x3);

        Ur_x2Face[j][i].Mz -= qa*(phir-phil)*pGrid->U[ks][j][i].d;
#ifndef ISOTHERMAL
        Ur_x2Face[j][i].E -= qa*(x1Flux[j  ][i  ].d*(phic - phil) +
                                 x1Flux[j  ][i+1].d*(phir - phic));
#endif

        phic = (*StaticGravPot)((x1               ),(x2-pGrid->dx2),x3);
        phir = (*StaticGravPot)((x1+0.5*pGrid->dx1),(x2-pGrid->dx2),x3);
        phil = (*StaticGravPot)((x1-0.5*pGrid->dx1),(x2-pGrid->dx2),x3);

        Ul_x2Face[j][i].Mz -= qa*(phir-phil)*pGrid->U[ks][j-1][i].d;
#ifndef ISOTHERMAL
        Ul_x2Face[j][i].E -= qa*(x1Flux[j-1][i  ].d*(phic - phil) +
                                 x1Flux[j-1][i+1].d*(phir - phic));
#endif
      }
    }
  }

/*--- Step 7 ------------------------------------------------------------------
 * Calculate the cell centered value of emf_3 at t^{n+1/2}, needed by CT 
 * algorithm to integrate emf to corner in step 10
 */

  if (dhalf != NULL){
    for (j=js-1; j<=je+1; j++) {
      for (i=is-1; i<=ie+1; i++) {
        dhalf[j][i] = pGrid->U[ks][j][i].d
          - 0.5*dtodx1*(x1Flux[j  ][i+1].d - x1Flux[j][i].d)
          - 0.5*dtodx2*(x2Flux[j+1][i  ].d - x2Flux[j][i].d);
      }
    }
  }

#ifdef MHD
  for (j=js-1; j<=je+1; j++) {
    for (i=is-1; i<=ie+1; i++) {
      cc_pos(pGrid,i,j,ks,&x1,&x2,&x3);

      d  = dhalf[j][i];

      M1 = pGrid->U[ks][j][i].M1
        - 0.5*dtodx1*(x1Flux[j][i+1].Mx - x1Flux[j][i].Mx)
        - 0.5*dtodx2*(x2Flux[j+1][i].Mz - x2Flux[j][i].Mz);
      if (StaticGravPot != NULL){
        phir = (*StaticGravPot)((x1+0.5*pGrid->dx1),x2,x3);
        phil = (*StaticGravPot)((x1-0.5*pGrid->dx1),x2,x3);
        M1 -= 0.5*dtodx1*(phir-phil)*pGrid->U[ks][j][i].d;
      }

      M2 = pGrid->U[ks][j][i].M2
        - 0.5*dtodx1*(x1Flux[j][i+1].My - x1Flux[j][i].My)
        - 0.5*dtodx2*(x2Flux[j+1][i].Mx - x2Flux[j][i].Mx);
      if (StaticGravPot != NULL){
        phir = (*StaticGravPot)(x1,(x2+0.5*pGrid->dx2),x3);
        phil = (*StaticGravPot)(x1,(x2-0.5*pGrid->dx2),x3);
        M2 -= 0.5*dtodx2*(phir-phil)*pGrid->U[ks][j][i].d;
      }

      B1c = 0.5*(B1_x1Face[j][i] + B1_x1Face[j][i+1]);

      B2c = 0.5*(B2_x2Face[j][i] + B2_x2Face[j+1][i]);

      emf3_cc[j][i] = (B1c*M2 - B2c*M1)/d;
    }
  }
#endif /* MHD */

/*--- Step 8a ------------------------------------------------------------------
 * Compute maximum wavespeeds in multidimensions (eta in eq. 10 from Sanders et
 *  al. (1998)) for H-correction
 */

#ifdef H_CORRECTION
  for (j=js-1; j<=je+1; j++) {
    for (i=is-1; i<=iu; i++) {
      cfr = cfast(&(Ur_x1Face[j][i]), &(B1_x1Face[j][i]));
      cfl = cfast(&(Ul_x1Face[j][i]), &(B1_x1Face[j][i]));
      ur = Ur_x1Face[j][i].Mx/Ur_x1Face[j][i].d;
      ul = Ul_x1Face[j][i].Mx/Ul_x1Face[j][i].d;
      eta1[j][i] = 0.5*(fabs(ur - ul) + fabs(cfr - cfl));
    }
  }

  for (j=js-1; j<=ju; j++) {
    for (i=is-1; i<=ie+1; i++) {
      cfr = cfast(&(Ur_x2Face[j][i]), &(B2_x2Face[j][i]));
      cfl = cfast(&(Ul_x2Face[j][i]), &(B2_x2Face[j][i]));
      ur = Ur_x2Face[j][i].Mx/Ur_x2Face[j][i].d;
      ul = Ul_x2Face[j][i].Mx/Ul_x2Face[j][i].d;
      eta2[j][i] = 0.5*(fabs(ur - ul) + fabs(cfr - cfl));
    }
  }
#endif /* H_CORRECTION */

/*--- Step 8b ------------------------------------------------------------------
 * Compute x1-fluxes from corrected L/R states.
 */

  for (j=js-1; j<=je+1; j++) {
    for (i=is; i<=ie+1; i++) {
#ifdef H_CORRECTION
      etah = MAX(eta2[j][i-1],eta2[j][i]);
      etah = MAX(etah,eta2[j+1][i-1]);
      etah = MAX(etah,eta2[j+1][i  ]);
      etah = MAX(etah,eta1[j  ][i  ]);
#endif /* H_CORRECTION */
      GET_FLUXES(B1_x1Face[j][i],Ul_x1Face[j][i],Ur_x1Face[j][i],&x1Flux[j][i]);
    }
  }

/*--- Step 8c ------------------------------------------------------------------
 * Compute x2-fluxes from corrected L/R states.
 */

  for (j=js; j<=je+1; j++) {
    for (i=is-1; i<=ie+1; i++) {
#ifdef H_CORRECTION
      etah = MAX(eta1[j-1][i],eta1[j][i]);
      etah = MAX(etah,eta1[j-1][i+1]);
      etah = MAX(etah,eta1[j  ][i+1]);
      etah = MAX(etah,eta2[j  ][i  ]);
#endif /* H_CORRECTION */
      GET_FLUXES(B2_x2Face[j][i],Ul_x2Face[j][i],Ur_x2Face[j][i],&x2Flux[j][i]);
    }
  }

/*--- Step 9 -------------------------------------------------------------------
 * Integrate emf3^{n+1/2} to the grid cell corners and then update the 
 * interface magnetic fields using CT for a full time step.
 */

#ifdef MHD
  integrate_emf3_corner(pGrid);

  for (j=js; j<=je; j++) {
    for (i=is; i<=ie; i++) {
      pGrid->B1i[ks][j][i] -= dtodx2*(emf3[j+1][i  ] - emf3[j][i]);
      pGrid->B2i[ks][j][i] += dtodx1*(emf3[j  ][i+1] - emf3[j][i]);
    }
    pGrid->B1i[ks][j][ie+1] -= dtodx2*(emf3[j+1][ie+1] - emf3[j][ie+1]);
  }
  for (i=is; i<=ie; i++) {
    pGrid->B2i[ks][je+1][i] += dtodx1*(emf3[je+1][i+1] - emf3[je+1][i]);
  }
#endif


/*--- Step 10 ------------------------------------------------------------------
 * Add the gravitational source terms at second order.  To improve conservation
 * of total energy, we average the energy source term computed at cell faces.
 *    S_{M} = -(\rho)^{n+1/2} Grad(Phi);   S_{E} = -(\rho v)^{n+1/2} Grad{Phi}
 */

  if (StaticGravPot != NULL){
    for (j=js; j<=je; j++) {
      for (i=is; i<=ie; i++) {
        cc_pos(pGrid,i,j,ks,&x1,&x2,&x3);
        phic = (*StaticGravPot)((x1               ),x2,x3);
        phir = (*StaticGravPot)((x1+0.5*pGrid->dx1),x2,x3);
        phil = (*StaticGravPot)((x1-0.5*pGrid->dx1),x2,x3);

        pGrid->U[ks][j][i].M1 -= dtodx1*dhalf[j][i]*(phir-phil);

#ifndef ISOTHERMAL
        pGrid->U[ks][j][i].E -= dtodx1*(x1Flux[j][i  ].d*(phic - phil) +
                                        x1Flux[j][i+1].d*(phir - phic));
#endif
        phir = (*StaticGravPot)(x1,(x2+0.5*pGrid->dx2),x3);
        phil = (*StaticGravPot)(x1,(x2-0.5*pGrid->dx2),x3);

        pGrid->U[ks][j][i].M2 -= dtodx2*dhalf[j][i]*(phir-phil);

#ifndef ISOTHERMAL
        pGrid->U[ks][j][i].E -= dtodx2*(x2Flux[j  ][i].d*(phic - phil) +
                                        x2Flux[j+1][i].d*(phir - phic));
#endif
      }
    }
  }

/*--- Step 11a -----------------------------------------------------------------
 * Update cell-centered variables in pGrid using x1-fluxes
 */

  for (j=js; j<=je; j++) {
    for (i=is; i<=ie; i++) {
      pGrid->U[ks][j][i].d  -= dtodx1*(x1Flux[j][i+1].d  - x1Flux[j][i].d );
      pGrid->U[ks][j][i].M1 -= dtodx1*(x1Flux[j][i+1].Mx - x1Flux[j][i].Mx);
      pGrid->U[ks][j][i].M2 -= dtodx1*(x1Flux[j][i+1].My - x1Flux[j][i].My);
      pGrid->U[ks][j][i].M3 -= dtodx1*(x1Flux[j][i+1].Mz - x1Flux[j][i].Mz);
#ifndef ISOTHERMAL
      pGrid->U[ks][j][i].E  -= dtodx1*(x1Flux[j][i+1].E  - x1Flux[j][i].E );
#endif /* ISOTHERMAL */
#ifdef MHD
      pGrid->U[ks][j][i].B2c -= dtodx1*(x1Flux[j][i+1].By - x1Flux[j][i].By);
      pGrid->U[ks][j][i].B3c -= dtodx1*(x1Flux[j][i+1].Bz - x1Flux[j][i].Bz);
#endif /* MHD */
#if (NSCALARS > 0)
      for (n=0; n<NSCALARS; n++)
        pGrid->U[ks][j][i].s[n] -= dtodx1*(x1Flux[j][i+1].s[n] 
                                         - x1Flux[j][i  ].s[n]);
#endif

    }
  }

/*--- Step 11b -----------------------------------------------------------------
 * Update cell-centered variables in pGrid using x2-fluxes
 */

  for (j=js; j<=je; j++) {
    for (i=is; i<=ie; i++) {
      pGrid->U[ks][j][i].d  -= dtodx2*(x2Flux[j+1][i].d  - x2Flux[j][i].d );
      pGrid->U[ks][j][i].M1 -= dtodx2*(x2Flux[j+1][i].Mz - x2Flux[j][i].Mz);
      pGrid->U[ks][j][i].M2 -= dtodx2*(x2Flux[j+1][i].Mx - x2Flux[j][i].Mx);
      pGrid->U[ks][j][i].M3 -= dtodx2*(x2Flux[j+1][i].My - x2Flux[j][i].My);
#ifndef ISOTHERMAL
      pGrid->U[ks][j][i].E  -= dtodx2*(x2Flux[j+1][i].E  - x2Flux[j][i].E );
#endif /* ISOTHERMAL */
#ifdef MHD
      pGrid->U[ks][j][i].B3c -= dtodx2*(x2Flux[j+1][i].By - x2Flux[j][i].By);
      pGrid->U[ks][j][i].B1c -= dtodx2*(x2Flux[j+1][i].Bz - x2Flux[j][i].Bz);
#endif /* MHD */
#if (NSCALARS > 0)
      for (n=0; n<NSCALARS; n++)
        pGrid->U[ks][j][i].s[n] -= dtodx2*(x2Flux[j+1][i].s[n] 
                                         - x2Flux[j  ][i].s[n]);
#endif
    }
  }

/*--- Step 13 ------------------------------------------------------------------
 * LAST STEP!
 * Set cell centered magnetic fields to average of updated face centered fields.
 */

#ifdef MHD
  for (j=js; j<=je; j++) {
    for (i=is; i<=ie; i++) {

      pGrid->U[ks][j][i].B1c =0.5*(pGrid->B1i[ks][j][i]+pGrid->B1i[ks][j][i+1]);
      pGrid->U[ks][j][i].B2c =0.5*(pGrid->B2i[ks][j][i]+pGrid->B2i[ks][j+1][i]);
/* Set the 3-interface magnetic field equal to the cell center field. */
      pGrid->B3i[ks][j][i] = pGrid->U[ks][j][i].B3c;
    }
  }
#endif /* MHD */

  return;
}


/*----------------------------------------------------------------------------*/
/* integrate_destruct_2d:  Free temporary integration arrays */

void integrate_destruct_2d(void)
{
#ifdef MHD
  if (emf3    != NULL) free_2d_array(emf3);
  if (emf3_cc != NULL) free_2d_array(emf3_cc);
#endif /* MHD */
#ifdef H_CORRECTION
  if (eta1 != NULL) free_2d_array(eta1);
  if (eta2 != NULL) free_2d_array(eta2);
#endif /* H_CORRECTION */
  if (Bxc != NULL) free(Bxc);
  if (Bxi != NULL) free(Bxi);
  if (B1_x1Face != NULL) free_2d_array(B1_x1Face);
  if (B2_x2Face != NULL) free_2d_array(B2_x2Face);

  if (U1d      != NULL) free(U1d);
  if (Ul       != NULL) free(Ul);
  if (Ur       != NULL) free(Ur);
  if (W        != NULL) free(W);
  if (Wl       != NULL) free(Wl);
  if (Wr       != NULL) free(Wr);

  if (Ul_x1Face != NULL) free_2d_array(Ul_x1Face);
  if (Ur_x1Face != NULL) free_2d_array(Ur_x1Face);
  if (Ul_x2Face != NULL) free_2d_array(Ul_x2Face);
  if (Ur_x2Face != NULL) free_2d_array(Ur_x2Face);
  if (x1Flux    != NULL) free_2d_array(x1Flux);
  if (x2Flux    != NULL) free_2d_array(x2Flux);
  if (dhalf     != NULL) free_2d_array(dhalf);

  return;
}

/*----------------------------------------------------------------------------*/
/* integrate_init_2d:    Allocate temporary integration arrays */

void integrate_init_2d(int nx1, int nx2)
{
  int nmax;
  int Nx1 = nx1 + 2*nghost;
  int Nx2 = nx2 + 2*nghost;
  nmax = MAX(Nx1,Nx2);

#ifdef MHD
  if ((emf3 = (Real**)calloc_2d_array(Nx2, Nx1, sizeof(Real))) == NULL)
    goto on_error;
  if ((emf3_cc = (Real**)calloc_2d_array(Nx2, Nx1, sizeof(Real))) == NULL)
    goto on_error;
#endif /* MHD */
#ifdef H_CORRECTION
  if ((eta1 = (Real**)calloc_2d_array(Nx2, Nx1, sizeof(Real))) == NULL)
    goto on_error;
  if ((eta2 = (Real**)calloc_2d_array(Nx2, Nx1, sizeof(Real))) == NULL)
    goto on_error;
#endif /* H_CORRECTION */

  if ((Bxc = (Real*)malloc(nmax*sizeof(Real))) == NULL) goto on_error;
  if ((Bxi = (Real*)malloc(nmax*sizeof(Real))) == NULL) goto on_error;

  if ((B1_x1Face = (Real**)calloc_2d_array(Nx2, Nx1, sizeof(Real))) == NULL)
    goto on_error;
  if ((B2_x2Face = (Real**)calloc_2d_array(Nx2, Nx1, sizeof(Real))) == NULL)
    goto on_error;

  if ((U1d =      (Cons1D*)malloc(nmax*sizeof(Cons1D))) == NULL) goto on_error;
  if ((Ul  =      (Cons1D*)malloc(nmax*sizeof(Cons1D))) == NULL) goto on_error;
  if ((Ur  =      (Cons1D*)malloc(nmax*sizeof(Cons1D))) == NULL) goto on_error;
  if ((W  =      (Prim1D*)malloc(nmax*sizeof(Prim1D))) == NULL) goto on_error;
  if ((Wl =      (Prim1D*)malloc(nmax*sizeof(Prim1D))) == NULL) goto on_error;
  if ((Wr =      (Prim1D*)malloc(nmax*sizeof(Prim1D))) == NULL) goto on_error;

  if ((Ul_x1Face = (Cons1D**)calloc_2d_array(Nx2, Nx1, sizeof(Cons1D))) == NULL)
    goto on_error;
  if ((Ur_x1Face = (Cons1D**)calloc_2d_array(Nx2, Nx1, sizeof(Cons1D))) == NULL)
    goto on_error;
  if ((Ul_x2Face = (Cons1D**)calloc_2d_array(Nx2, Nx1, sizeof(Cons1D))) == NULL)
    goto on_error;
  if ((Ur_x2Face = (Cons1D**)calloc_2d_array(Nx2, Nx1, sizeof(Cons1D))) == NULL)
    goto on_error;

  if ((x1Flux    = (Cons1D**)calloc_2d_array(Nx2, Nx1, sizeof(Cons1D))) == NULL)
    goto on_error;
  if ((x2Flux    = (Cons1D**)calloc_2d_array(Nx2, Nx1, sizeof(Cons1D))) == NULL)
    goto on_error;

#if defined MHD
  if ((dhalf = (Real**)calloc_2d_array(Nx2, Nx1, sizeof(Real))) == NULL)
    goto on_error;
#else
  if(StaticGravPot != NULL){
    if ((dhalf = (Real**)calloc_2d_array(Nx2, Nx1, sizeof(Real))) == NULL)
      goto on_error;
  }
#endif

  return;

  on_error:
  integrate_destruct();
  ath_error("[integrate_init]: malloc returned a NULL pointer\n");
}


/*=========================== PRIVATE FUNCTIONS ==============================*/

/*----------------------------------------------------------------------------*/
/* integrate_emf3_corner:  */

#ifdef MHD
static void integrate_emf3_corner(Grid *pGrid)
{
  int i,is,ie,j,js,je;
  Real emf_l1, emf_r1, emf_l2, emf_r2;

  is = pGrid->is;   ie = pGrid->ie;
  js = pGrid->js;   je = pGrid->je;

/* NOTE: The x1-Flux of B2 is -E3.  The x2-Flux of B1 is +E3. */
  for (j=js-1; j<=je+2; j++) {
    for (i=is-1; i<=ie+2; i++) {
      if (x1Flux[j-1][i].d > 0.0) {
	emf_l2 = -x1Flux[j-1][i].By
	  + (x2Flux[j][i-1].Bz - emf3_cc[j-1][i-1]);
      }
      else if (x1Flux[j-1][i].d < 0.0) {
	emf_l2 = -x1Flux[j-1][i].By
	  + (x2Flux[j][i].Bz - emf3_cc[j-1][i]);

      } else {
	emf_l2 = -x1Flux[j-1][i].By
	  + 0.5*(x2Flux[j][i-1].Bz - emf3_cc[j-1][i-1] + 
		 x2Flux[j][i  ].Bz - emf3_cc[j-1][i  ] );
      }

      if (x1Flux[j][i].d > 0.0) {
	emf_r2 = -x1Flux[j][i].By 
	  + (x2Flux[j][i-1].Bz - emf3_cc[j][i-1]);
      }
      else if (x1Flux[j][i].d < 0.0) {
	emf_r2 = -x1Flux[j][i].By 
	  + (x2Flux[j][i].Bz - emf3_cc[j][i]);

      } else {
	emf_r2 = -x1Flux[j][i].By 
	  + 0.5*(x2Flux[j][i-1].Bz - emf3_cc[j][i-1] + 
		 x2Flux[j][i  ].Bz - emf3_cc[j][i  ] );
      }

      if (x2Flux[j][i-1].d > 0.0) {
	emf_l1 = x2Flux[j][i-1].Bz
	  + (-x1Flux[j-1][i].By - emf3_cc[j-1][i-1]);
      }
      else if (x2Flux[j][i-1].d < 0.0) {
	emf_l1 = x2Flux[j][i-1].Bz 
          + (-x1Flux[j][i].By - emf3_cc[j][i-1]);
      } else {
	emf_l1 = x2Flux[j][i-1].Bz
	  + 0.5*(-x1Flux[j-1][i].By - emf3_cc[j-1][i-1]
		 -x1Flux[j  ][i].By - emf3_cc[j  ][i-1] );
      }

      if (x2Flux[j][i].d > 0.0) {
	emf_r1 = x2Flux[j][i].Bz
	  + (-x1Flux[j-1][i].By - emf3_cc[j-1][i]);
      }
      else if (x2Flux[j][i].d < 0.0) {
	emf_r1 = x2Flux[j][i].Bz
	  + (-x1Flux[j][i].By - emf3_cc[j][i]);
      } else {
	emf_r1 = x2Flux[j][i].Bz
	  + 0.5*(-x1Flux[j-1][i].By - emf3_cc[j-1][i]
		 -x1Flux[j  ][i].By - emf3_cc[j  ][i] );
      }

      emf3[j][i] = 0.25*(emf_l1 + emf_r1 + emf_l2 + emf_r2);
    }
  }

  return;
}
#endif /* MHD */
