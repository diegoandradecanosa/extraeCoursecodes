/*

                 Copyright (c) 2010.
      Lawrence Livermore National Security, LLC.
Produced at the Lawrence Livermore National Laboratory.
                  LLNL-CODE-461231
                All rights reserved.

This file is part of LULESH, Version 1.0.
Please also read this link -- http://www.opensource.org/licenses/index.php

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the disclaimer below.

   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the disclaimer (as noted below)
     in the documentation and/or other materials provided with the
     distribution.

   * Neither the name of the LLNS/LLNL nor the names of its contributors
     may be used to endorse or promote products derived from this software
     without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LAWRENCE LIVERMORE NATIONAL SECURITY, LLC,
THE U.S. DEPARTMENT OF ENERGY OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


Additional BSD Notice

1. This notice is required to be provided under our contract with the U.S.
   Department of Energy (DOE). This work was produced at Lawrence Livermore
   National Laboratory under Contract No. DE-AC52-07NA27344 with the DOE.

2. Neither the United States Government nor Lawrence Livermore National
   Security, LLC nor any of their employees, makes any warranty, express
   or implied, or assumes any liability or responsibility for the accuracy,
   completeness, or usefulness of any information, apparatus, product, or
   process disclosed, or represents that its use would not infringe
   privately-owned rights.

3. Also, reference herein to any specific commercial products, process, or
   services by trade name, trademark, manufacturer or otherwise does not
   necessarily constitute or imply its endorsement, recommendation, or
   favoring by the United States Government or Lawrence Livermore National
   Security, LLC. The views and opinions of authors expressed herein do not
   necessarily state or reflect those of the United States Government or
   Lawrence Livermore National Security, LLC, and shall not be used for
   advertising or product endorsement purposes.

*/

#include <climits>
#include <vector>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <omp.h>

#define LULESH_SHOW_PROGRESS 1
#define VIZ_MESH 0

enum { VolumeError = -1, QStopError = -2 } ;

/****************************************************/
/* Allow flexibility for arithmetic representations */
/****************************************************/

/* Could also support fixed point and interval arithmetic types */
typedef float        real4 ;
typedef double       real8 ;
typedef long double  real10 ;  /* 10 bytes on x86 */

typedef int    Index_t ; /* array subscript and loop index */
typedef real8  Real_t ;  /* floating point representation */
typedef int    Int_t ;   /* integer representation */

inline real4  SQRT(real4  arg) { return sqrtf(arg) ; }
inline real8  SQRT(real8  arg) { return sqrt(arg) ; }
inline real10 SQRT(real10 arg) { return sqrtl(arg) ; }

inline real4  CBRT(real4  arg) { return cbrtf(arg) ; }
inline real8  CBRT(real8  arg) { return cbrt(arg) ; }
inline real10 CBRT(real10 arg) { return cbrtl(arg) ; }

inline real4  FABS(real4  arg) { return fabsf(arg) ; }
inline real8  FABS(real8  arg) { return fabs(arg) ; }
inline real10 FABS(real10 arg) { return fabsl(arg) ; }


#define MAX(a, b) ( ((a) > (b)) ? (a) : (b))

/*********************************/
/* Data structure implementation */
/*********************************/

/* might want to add access methods so that memory can be */
/* better managed, as in luleshFT */

struct Domain {
   /* Elem-centered */

   Index_t *matElemlist ;  /* material indexset */
#if 1
   Index_t *nodelist ;     /* elemToNode connectivity */
#else
   Index_t *nodelist0 ;    /* elemToNode connectivity */
   Index_t *nodelist1 ;
   Index_t *nodelist2 ;
   Index_t *nodelist3 ;
   Index_t *nodelist4 ;
   Index_t *nodelist5 ;
   Index_t *nodelist6 ;
   Index_t *nodelist7 ;
#endif

   Index_t *lxim ;         /* elem connectivity through face */
   Index_t *lxip ;
   Index_t *letam ;
   Index_t *letap ;
   Index_t *lzetam ;
   Index_t *lzetap ;

   Int_t *elemBC ;         /* elem face symm/free-surface flag */

   Real_t *e ;             /* energy */

   Real_t *p ;             /* pressure */

   Real_t *q ;             /* q */
   Real_t *ql ;            /* linear term for q */
   Real_t *qq ;            /* quadratic term for q */

   Real_t *v ;             /* relative volume */

   Real_t *volo ;          /* reference volume */
   Real_t *delv ;          /* m_vnew - m_v */
   Real_t *vdov ;          /* volume derivative over volume */

   Real_t *arealg ;        /* elem characteristic length */

   Real_t *ss ;            /* "sound speed" */

   Real_t *elemMass ;      /* mass */

   /* Elem temporaries */

   Real_t *vnew ;          /* new relative volume -- temporary */

   Real_t *delv_xi ;       /* velocity gradient -- temporary */
   Real_t *delv_eta ;
   Real_t *delv_zeta ;

   Real_t *delx_xi ;       /* position gradient -- temporary */
   Real_t *delx_eta ;
   Real_t *delx_zeta ;

   Real_t *dxx ;          /* principal strains -- temporary */
   Real_t *dyy ;
   Real_t *dzz ;

   /* Node-centered */

   Real_t *x ;             /* coordinates */
   Real_t *y ;
   Real_t *z ;

   Real_t *xd ;            /* velocities */
   Real_t *yd ;
   Real_t *zd ;

   Real_t *xdd ;           /* accelerations */
   Real_t *ydd ;
   Real_t *zdd ;

   Real_t *fx ;            /* forces */
   Real_t *fy ;
   Real_t *fz ;

   Real_t *nodalMass ;     /* mass */

   /* Communication Work space */

   Real_t *commDataSend ;
   Real_t *commDataRecv ;

   /* Maximum number of block neighbors */
   MPI_Request recvRequest[26] ; /* 6 faces + 12 edges + 8 corners */
   MPI_Request sendRequest[26] ; /* 6 faces + 12 edges + 8 corners */


   /* Boundary nodesets */

   Index_t *symmX ;        /* Nodes on X symmetry plane */
   Index_t *symmY ;        /* Nodes on Y symmetry plane */
   Index_t *symmZ ;        /* Nodes on Z symmetry plane */

   /* OMP hack */
   Index_t *nodeElemCount ;
   Index_t *nodeElemStart ;
   Index_t *nodeElemCornerList ;

   /* Parameters */

   Real_t  dtfixed ;           /* fixed time increment */
   Real_t  time ;              /* current time */
   Real_t  deltatime ;         /* variable time increment */
   Real_t  deltatimemultlb ;
   Real_t  deltatimemultub ;
   Real_t  stoptime ;          /* end time for simulation */

   Real_t  u_cut ;             /* velocity tolerance */
   Real_t  hgcoef ;            /* hourglass control */
   Real_t  qstop ;             /* excessive q indicator */
   Real_t  monoq_max_slope ;
   Real_t  monoq_limiter_mult ;
   Real_t  e_cut ;             /* energy tolerance */
   Real_t  p_cut ;             /* pressure tolerance */
   Real_t  ss4o3 ;
   Real_t  q_cut ;             /* q tolerance */
   Real_t  v_cut ;             /* relative volume tolerance */
   Real_t  qlc_monoq ;         /* linear term coef for q */
   Real_t  qqc_monoq ;         /* quadratic term coef for q */
   Real_t  qqc ;
   Real_t  eosvmax ;
   Real_t  eosvmin ;
   Real_t  pmin ;              /* pressure floor */
   Real_t  emin ;              /* energy floor */
   Real_t  dvovmax ;           /* maximum allowable volume change */
   Real_t  refdens ;           /* reference density */

   Real_t  dtcourant ;         /* courant constraint */
   Real_t  dthydro ;           /* volume change constraint */
   Real_t  dtmax ;             /* maximum allowable time increment */

   Int_t   cycle ;             /* iteration count for simulation */

   Int_t   numProcs ;

   Index_t colLoc ;
   Index_t rowLoc ;
   Index_t planeLoc ;
   Index_t tp ;

   Index_t sizeX ;
   Index_t sizeY ;
   Index_t sizeZ ;
   Index_t maxPlaneSize ;
   Index_t maxEdgeSize ;
   Index_t numElem ;

   Index_t numNode ;
} ;


template <typename T>
T *Allocate(size_t size)
{
   return static_cast<T *>(malloc(sizeof(T)*size)) ;
}

template <typename T>
void Release(T **ptr)
{
   if (*ptr != NULL) {
      free(*ptr) ;
      *ptr = NULL ;
   }
}


/* Stuff needed for boundary conditions */
/* 2 BCs on each of 6 hexahedral faces (12 bits) */
#define XI_M        0x00007
#define XI_M_SYMM   0x00001
#define XI_M_FREE   0x00002
#define XI_M_COMM   0x00004

#define XI_P        0x00038
#define XI_P_SYMM   0x00008
#define XI_P_FREE   0x00010
#define XI_P_COMM   0x00020

#define ETA_M       0x001c0
#define ETA_M_SYMM  0x00040
#define ETA_M_FREE  0x00080
#define ETA_M_COMM  0x00100

#define ETA_P       0x00e00
#define ETA_P_SYMM  0x00200
#define ETA_P_FREE  0x00400
#define ETA_P_COMM  0x00800

#define ZETA_M      0x07000
#define ZETA_M_SYMM 0x01000
#define ZETA_M_FREE 0x02000
#define ZETA_M_COMM 0x04000

#define ZETA_P      0x38000
#define ZETA_P_SYMM 0x08000
#define ZETA_P_FREE 0x10000
#define ZETA_P_COMM 0x20000

/* Assume 128 byte coherence */
/* Assume Real_t is an "integral power of 2" bytes wide */
#define CACHE_COHERENCE_PAD_REAL (128 / sizeof(Real_t))

#define CACHE_ALIGN_REAL(n) \
   (((n) + (CACHE_COHERENCE_PAD_REAL - 1)) & ~(CACHE_COHERENCE_PAD_REAL-1))

/******************************************/

/* Comm Routines */

#define MAX_FIELDS_PER_MPI_COMM 6

#define ALLOW_UNPACKED_PLANE false
#define ALLOW_UNPACKED_ROW   false
#define ALLOW_UNPACKED_COL   false

#define MSG_COMM_SBN      1024
#define MSG_SYNC_POS_VEL  2048
#define MSG_MONOQ         3072

/*
   define one of these three symbols:

   SEDOV_SYNC_POS_VEL_NONE
   SEDOV_SYNC_POS_VEL_EARLY
   SEDOV_SYNC_POS_VEL_LATE
*/

#define SEDOV_SYNC_POS_VEL_EARLY 1

/*
   There are coherence issues for packing and unpacking message
   buffers.  Ideally, you would like a lot of threads to 
   cooperate in the assembly/dissassembly of each message.
   To do that, each thread should really be operating in a
   different coherence zone.

   Let's assume we have three fields, f1 through f3, defined on
   a 61x61x61 cube.  If we want to send the block boundary
   information for each field to each neighbor processor across
   each cube face, then we have three cases for the
   memory layout/coherence of data on each of the six cube
   boundaries:

      (a) Two of the faces will be in contiguous memory blocks
      (b) Two of the faces will be comprised of pencils of
          contiguous memory.
      (c) Two of the faces will have large strides between
          every value living on the face.

   How do you pack and unpack this data in buffers to
   simultaneous achieve the best memory efficiency and
   the most thread independence?

   Do do you pack field f1 through f3 tighly to reduce message
   size?  Do you align each field on a cache coherence boundary
   within the message so that threads can pack and unpack each
   field independently?  For case (b), do you align each
   boundary pencil of each field separately?  This increases
   the message size, but could improve cache coherence so
   each pencil could be processed independently by a separate
   thread with no conflicts.

   Also, memory access for case (c) would best be done without
   going through the cache (the stride is so large it just causes
   a lot of useless cache evictions).  Is it worth creating
   a special case version of the packing algorithm that uses
   non-coherent load/store opcodes?
*/

/*
    Currently, all message traffic occurs at once.
    We could spread message traffic out like this: 

    CommRecv(domain) ;
    forall(domain->views()-attr("chunk & boundary")) {
       ... do work in parallel ...
    }
    CommSend(domain) ;
    forall(domain->views()-attr("chunk & ~boundary")) {
       ... do work in parallel ...
    }
    CommSBN() ;

    or the CommSend() could function as a semaphore
    for even finer granularity.  When the last chunk
    on a boundary marks the boundary as complete, the
    send could happen immediately:

    CommRecv(domain) ;
    forall(domain->views()-attr("chunk & boundary")) {
       ... do work in parallel ...
       CommSend(domain) ;
    }
    forall(domain->views()-attr("chunk & ~boundary")) {
       ... do work in parallel ...
    }
    CommSBN() ;

*/
    
/* doRecv flag only works with regular block structure */
void CommRecv(Domain *domain, int msgType, Index_t xferFields,
              Index_t dx, Index_t dy, Index_t dz, bool doRecv, bool planeOnly) {

   if (domain->numProcs == 1) return ;

   /* post recieve buffers for all incoming messages */
   int myRank ;
   Index_t maxPlaneComm = xferFields * domain->maxPlaneSize ;
   Index_t maxEdgeComm  = xferFields * domain->maxEdgeSize ;
   Index_t pmsg = 0 ; /* plane comm msg */
   Index_t emsg = 0 ; /* edge comm msg */
   Index_t cmsg = 0 ; /* corner comm msg */
   MPI_Datatype baseType = ((sizeof(Real_t) == 4) ? MPI_FLOAT : MPI_DOUBLE) ;
   bool rowMin, rowMax, colMin, colMax, planeMin, planeMax ;

   /* assume communication to 6 neighbors by default */
   rowMin = rowMax = colMin = colMax = planeMin = planeMax = true ;

   if (domain->rowLoc == 0) {
      rowMin = false ;
   }
   if (domain->rowLoc == (domain->tp-1)) {
      rowMax = false ;
   }
   if (domain->colLoc == 0) {
      colMin = false ;
   }
   if (domain->colLoc == (domain->tp-1)) {
      colMax = false ;
   }
   if (domain->planeLoc == 0) {
      planeMin = false ;
   }
   if (domain->planeLoc == (domain->tp-1)) {
      planeMax = false ;
   }

   for (Index_t i=0; i<26; ++i) {
      domain->recvRequest[i] = MPI_REQUEST_NULL ;
   }

   MPI_Comm_rank(MPI_COMM_WORLD, &myRank) ;

   /* post receives */

   /* receive data from neighboring domain faces */
   if (planeMin && doRecv) {
      /* contiguous memory */
      int fromProc = myRank - domain->tp*domain->tp ;
      int recvCount = dx * dy * xferFields ;
      MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm],
                recvCount, baseType, fromProc, msgType,
                MPI_COMM_WORLD, &domain->recvRequest[pmsg]) ;
      ++pmsg ;
   }
   if (planeMax) {
      /* contiguous memory */
      int fromProc = myRank + domain->tp*domain->tp ;
      int recvCount = dx * dy * xferFields ;
      MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm],
                recvCount, baseType, fromProc, msgType,
                MPI_COMM_WORLD, &domain->recvRequest[pmsg]) ;
      ++pmsg ;
   }
   if (rowMin && doRecv) {
      /* semi-contiguous memory */
      int fromProc = myRank - domain->tp ;
      int recvCount = dx * dz * xferFields ;
      MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm],
                recvCount, baseType, fromProc, msgType,
                MPI_COMM_WORLD, &domain->recvRequest[pmsg]) ;
      ++pmsg ;
   }
   if (rowMax) {
      /* semi-contiguous memory */
      int fromProc = myRank + domain->tp ;
      int recvCount = dx * dz * xferFields ;
      MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm],
                recvCount, baseType, fromProc, msgType,
                MPI_COMM_WORLD, &domain->recvRequest[pmsg]) ;
      ++pmsg ;
   }
   if (colMin && doRecv) {
      /* scattered memory */
      int fromProc = myRank - 1 ;
      int recvCount = dy * dz * xferFields ;
      MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm],
                recvCount, baseType, fromProc, msgType,
                MPI_COMM_WORLD, &domain->recvRequest[pmsg]) ;
      ++pmsg ;
   }
   if (colMax) {
      /* scattered memory */
      int fromProc = myRank + 1 ;
      int recvCount = dy * dz * xferFields ;
      MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm],
                recvCount, baseType, fromProc, msgType,
                MPI_COMM_WORLD, &domain->recvRequest[pmsg]) ;
      ++pmsg ;
   }

   if (!planeOnly) {
      /* receive data from domains connected only by an edge */
      if (rowMin && colMin && doRecv) {
         int fromProc = myRank - domain->tp - 1 ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm],
                   dz * xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (rowMin && planeMin && doRecv) {
         int fromProc = myRank - domain->tp*domain->tp - domain->tp ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm],
                   dx * xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (colMin && planeMin && doRecv) {
         int fromProc = myRank - domain->tp*domain->tp - 1 ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm],
                   dy * xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (rowMax && colMax) {
         int fromProc = myRank + domain->tp + 1 ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm],
                   dz * xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (rowMax && planeMax) {
         int fromProc = myRank + domain->tp*domain->tp + domain->tp ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm],
                   dx * xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (colMax && planeMax) {
         int fromProc = myRank + domain->tp*domain->tp + 1 ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm],
                   dy * xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (rowMax && colMin) {
         int fromProc = myRank + domain->tp - 1 ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm],
                   dz * xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (rowMin && planeMax) {
         int fromProc = myRank + domain->tp*domain->tp - domain->tp ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm],
                   dx * xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (colMin && planeMax) {
         int fromProc = myRank + domain->tp*domain->tp - 1 ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm],
                   dy * xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (rowMin && colMax && doRecv) {
         int fromProc = myRank - domain->tp + 1 ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm],
                   dz * xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (rowMax && planeMin && doRecv) {
         int fromProc = myRank - domain->tp*domain->tp + domain->tp ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm],
                   dx * xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (colMax && planeMin && doRecv) {
         int fromProc = myRank - domain->tp*domain->tp + 1 ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm],
                   dy * xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      /* receive data from domains connected only by a corner */
      if (rowMin && colMin && planeMin && doRecv) {
         /* corner at domain logical coord (0, 0, 0) */
         int fromProc = myRank - domain->tp*domain->tp - domain->tp - 1 ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm +
                                         cmsg * CACHE_COHERENCE_PAD_REAL],
                   xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg+cmsg]) ;
         ++cmsg ;
      }
      if (rowMin && colMin && planeMax) {
         /* corner at domain logical coord (0, 0, 1) */
         int fromProc = myRank + domain->tp*domain->tp - domain->tp - 1 ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm +
                                         cmsg * CACHE_COHERENCE_PAD_REAL],
                   xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg+cmsg]) ;
         ++cmsg ;
      }
      if (rowMin && colMax && planeMin && doRecv) {
         /* corner at domain logical coord (1, 0, 0) */
         int fromProc = myRank - domain->tp*domain->tp - domain->tp + 1 ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm +
                                         cmsg * CACHE_COHERENCE_PAD_REAL],
                   xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg+cmsg]) ;
         ++cmsg ;
      }
      if (rowMin && colMax && planeMax) {
         /* corner at domain logical coord (1, 0, 1) */
         int fromProc = myRank + domain->tp*domain->tp - domain->tp + 1 ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm +
                                         cmsg * CACHE_COHERENCE_PAD_REAL],
                   xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg+cmsg]) ;
         ++cmsg ;
      }
      if (rowMax && colMin && planeMin && doRecv) {
         /* corner at domain logical coord (0, 1, 0) */
         int fromProc = myRank - domain->tp*domain->tp + domain->tp - 1 ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm +
                                         cmsg * CACHE_COHERENCE_PAD_REAL],
                   xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg+cmsg]) ;
         ++cmsg ;
      }
      if (rowMax && colMin && planeMax) {
         /* corner at domain logical coord (0, 1, 1) */
         int fromProc = myRank + domain->tp*domain->tp + domain->tp - 1 ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm +
                                         cmsg * CACHE_COHERENCE_PAD_REAL],
                   xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg+cmsg]) ;
         ++cmsg ;
      }
      if (rowMax && colMax && planeMin && doRecv) {
         /* corner at domain logical coord (1, 1, 0) */
         int fromProc = myRank - domain->tp*domain->tp + domain->tp + 1 ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm +
                                         cmsg * CACHE_COHERENCE_PAD_REAL],
                   xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg+cmsg]) ;
         ++cmsg ;
      }
      if (rowMax && colMax && planeMax) {
         /* corner at domain logical coord (1, 1, 1) */
         int fromProc = myRank + domain->tp*domain->tp + domain->tp + 1 ;
         MPI_Irecv(&domain->commDataRecv[pmsg * maxPlaneComm +
                                         emsg * maxEdgeComm +
                                         cmsg * CACHE_COHERENCE_PAD_REAL],
                   xferFields, baseType, fromProc, msgType,
                   MPI_COMM_WORLD, &domain->recvRequest[pmsg+emsg+cmsg]) ;
         ++cmsg ;
      }
   }
}

void CommSend(Domain *domain, int msgType,
              Index_t xferFields, Real_t **fieldData,
              Index_t dx, Index_t dy, Index_t dz, bool doSend, bool planeOnly)
{

   if (domain->numProcs == 1) return ;

   /* post recieve buffers for all incoming messages */
   int myRank ;
   Index_t maxPlaneComm = xferFields * domain->maxPlaneSize ;
   Index_t maxEdgeComm  = xferFields * domain->maxEdgeSize ;
   Index_t pmsg = 0 ; /* plane comm msg */
   Index_t emsg = 0 ; /* edge comm msg */
   Index_t cmsg = 0 ; /* corner comm msg */
   MPI_Datatype baseType = ((sizeof(Real_t) == 4) ? MPI_FLOAT : MPI_DOUBLE) ;
   MPI_Status status[26] ;
   Real_t *destAddr ;
   bool rowMin, rowMax, colMin, colMax, planeMin, planeMax ;
   bool packable ;
   /* assume communication to 6 neighbors by default */
   rowMin = rowMax = colMin = colMax = planeMin = planeMax = true ;
   if (domain->rowLoc == 0) {
      rowMin = false ;
   }
   if (domain->rowLoc == (domain->tp-1)) {
      rowMax = false ;
   }
   if (domain->colLoc == 0) {
      colMin = false ;
   }
   if (domain->colLoc == (domain->tp-1)) {
      colMax = false ;
   }
   if (domain->planeLoc == 0) {
      planeMin = false ;
   }
   if (domain->planeLoc == (domain->tp-1)) {
      planeMax = false ;
   }

   packable = true ;
   for (Index_t i=0; i<xferFields-2; ++i) {
     if((fieldData[i+1] - fieldData[i]) != (fieldData[i+2] - fieldData[i+1])) {
        packable = false ;
        break ;
     }
   }
   for (Index_t i=0; i<26; ++i) {
      domain->sendRequest[i] = MPI_REQUEST_NULL ;
   }

   MPI_Comm_rank(MPI_COMM_WORLD, &myRank) ;

   /* post sends */

   if (planeMin | planeMax) {
      /* ASSUMING ONE DOMAIN PER RANK, CONSTANT BLOCK SIZE HERE */
      static MPI_Datatype msgTypePlane ;
      static bool packPlane ;
      int sendCount = dx * dy ;

      if (msgTypePlane == 0) {
         /* Create an MPI_struct for field data */
         if (ALLOW_UNPACKED_PLANE && packable) {

            MPI_Type_vector(xferFields, sendCount,
                            (fieldData[1] - fieldData[0]),
                            baseType, &msgTypePlane) ;
            MPI_Type_commit(&msgTypePlane) ;
            packPlane = false ;
         }
         else {
            msgTypePlane = baseType ;
            packPlane = true ;
         }
      }

      if (planeMin) {
         /* contiguous memory */
         if (packPlane) {
            destAddr = &domain->commDataSend[pmsg * maxPlaneComm] ;
            for (Index_t fi=0 ; fi<xferFields; ++fi) {
               Real_t *srcAddr = fieldData[fi] ;
               memcpy(destAddr, srcAddr, sendCount*sizeof(Real_t)) ;
               destAddr += sendCount ;
            }
            destAddr -= xferFields*sendCount ;
         }
         else {
            destAddr = fieldData[0] ;
         }

         MPI_Isend(destAddr, (packPlane ? xferFields*sendCount : 1),
                   msgTypePlane, myRank - domain->tp*domain->tp, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg]) ;
         ++pmsg ;
      }
      if (planeMax && doSend) {
         /* contiguous memory */
         Index_t offset = dx*dy*(dz - 1) ;
         if (packPlane) {
            destAddr = &domain->commDataSend[pmsg * maxPlaneComm] ;
            for (Index_t fi=0 ; fi<xferFields; ++fi) {
               Real_t *srcAddr = &fieldData[fi][offset] ;
               memcpy(destAddr, srcAddr, sendCount*sizeof(Real_t)) ;
               destAddr += sendCount ;
            }
            destAddr -= xferFields*sendCount ;
         }
         else {
            destAddr = &fieldData[0][offset] ;
         }

         MPI_Isend(destAddr, (packPlane ? xferFields*sendCount : 1),
                   msgTypePlane, myRank + domain->tp*domain->tp, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg]) ;
         ++pmsg ;
      }
   }
   if (rowMin | rowMax) {
      /* ASSUMING ONE DOMAIN PER RANK, CONSTANT BLOCK SIZE HERE */
      static MPI_Datatype msgTypeRow ;
      static bool packRow ;
      int sendCount = dx * dz ;

      if (msgTypeRow == 0) {
         /* Create an MPI_struct for field data */
         if (ALLOW_UNPACKED_ROW && packable) {

            static MPI_Datatype msgTypePencil ;

            /* dz pencils per plane */
            MPI_Type_vector(dz, dx, dx * dy, baseType, &msgTypePencil) ;
            MPI_Type_commit(&msgTypePencil) ;
            
            MPI_Type_vector(xferFields, 1, (fieldData[1] - fieldData[0]),
                            msgTypePencil, &msgTypeRow) ;
            MPI_Type_commit(&msgTypeRow) ;
            packRow = false ;
         }
         else {
            msgTypeRow = baseType ;
            packRow = true ;
         }
      }

      if (rowMin) {
         /* contiguous memory */
         if (packRow) {
            destAddr = &domain->commDataSend[pmsg * maxPlaneComm] ;
            for (Index_t fi=0; fi<xferFields; ++fi) {
               Real_t *srcAddr = fieldData[fi] ;
               for (Index_t i=0; i<dz; ++i) {
                  memcpy(&destAddr[i*dx], &srcAddr[i*dx*dy],
                         dx*sizeof(Real_t)) ;
               }
               destAddr += sendCount ;
            }
            destAddr -= xferFields*sendCount ;
         }
         else {
            destAddr = fieldData[0] ;
         }

         MPI_Isend(destAddr, (packRow ? xferFields*sendCount : 1),
                   msgTypeRow, myRank - domain->tp, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg]) ;
         ++pmsg ;
      }
      if (rowMax && doSend) {
         /* contiguous memory */
         Index_t offset = dx*(dy - 1) ;
         if (packRow) {
            destAddr = &domain->commDataSend[pmsg * maxPlaneComm] ;
            for (Index_t fi=0; fi<xferFields; ++fi) {
               Real_t *srcAddr = &fieldData[fi][offset] ;
               for (Index_t i=0; i<dz; ++i) {
                  memcpy(&destAddr[i*dx], &srcAddr[i*dx*dy],
                         dx*sizeof(Real_t)) ;
               }
               destAddr += sendCount ;
            }
            destAddr -= xferFields*sendCount ;
         }
         else {
            destAddr = &fieldData[0][offset] ;
         }

         MPI_Isend(destAddr, (packRow ? xferFields*sendCount : 1),
                   msgTypeRow, myRank + domain->tp, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg]) ;
         ++pmsg ;
      }
   }
   if (colMin | colMax) {
      /* ASSUMING ONE DOMAIN PER RANK, CONSTANT BLOCK SIZE HERE */
      static MPI_Datatype msgTypeCol ;
      static bool packCol ;
      int sendCount = dy * dz ;

      if (msgTypeCol == 0) {
         /* Create an MPI_struct for field data */
         if (ALLOW_UNPACKED_COL && packable) {

            static MPI_Datatype msgTypePoint ;
            static MPI_Datatype msgTypePencil ;

            /* dy points per pencil */
            MPI_Type_vector(dy, 1, dx, baseType, &msgTypePoint) ;
            MPI_Type_commit(&msgTypePoint) ;
           
            /* dz pencils per plane */
            MPI_Type_vector(dz, 1, dx*dy, msgTypePoint, &msgTypePencil) ;
            MPI_Type_commit(&msgTypePencil) ;

            MPI_Type_vector(xferFields, 1, (fieldData[1] - fieldData[0]),
                            msgTypePencil, &msgTypeCol) ;
            MPI_Type_commit(&msgTypeCol) ;
            packCol = false ;
         }
         else {
            msgTypeCol = baseType ;
            packCol = true ;
         }
      }

      if (colMin) {
         /* contiguous memory */
         if (packCol) {
            destAddr = &domain->commDataSend[pmsg * maxPlaneComm] ;
            for (Index_t fi=0; fi<xferFields; ++fi) {
               for (Index_t i=0; i<dz; ++i) {
                  Real_t *srcAddr = &fieldData[fi][i*dx*dy] ;
                  for (Index_t j=0; j<dy; ++j) {
                     destAddr[i*dy + j] = srcAddr[j*dx] ;
                  }
               }
               destAddr += sendCount ;
            }
            destAddr -= xferFields*sendCount ;
         }
         else {
            destAddr = fieldData[0] ;
         }

         MPI_Isend(destAddr, (packCol ? xferFields*sendCount : 1),
                   msgTypeCol, myRank - 1, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg]) ;
         ++pmsg ;
      }
      if (colMax && doSend) {
         /* contiguous memory */
         Index_t offset = dx - 1 ;
         if (packCol) {
            destAddr = &domain->commDataSend[pmsg * maxPlaneComm] ;
            for (Index_t fi=0; fi<xferFields; ++fi) {
               for (Index_t i=0; i<dz; ++i) {
                  Real_t *srcAddr = &fieldData[fi][i*dx*dy + offset] ;
                  for (Index_t j=0; j<dy; ++j) {
                     destAddr[i*dy + j] = srcAddr[j*dx] ;
                  }
               }
               destAddr += sendCount ;
            }
            destAddr -= xferFields*sendCount ;
         }
         else {
            destAddr = &fieldData[0][offset] ;
         }

         MPI_Isend(destAddr,
                   (packCol ? xferFields*sendCount : 1),
                   msgTypeCol, myRank + 1, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg]) ;
         ++pmsg ;
      }
   }

   if (!planeOnly) {
      if (rowMin && colMin) {
         int toProc = myRank - domain->tp - 1 ;
         destAddr = &domain->commDataSend[pmsg * maxPlaneComm +
                                          emsg * maxEdgeComm] ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            Real_t *srcAddr = fieldData[fi] ;
            for (Index_t i=0; i<dz; ++i) {
               destAddr[i] = srcAddr[i*dx*dy] ;
            }
            destAddr += dz ;
         }
         destAddr -= xferFields*dz ;
         MPI_Isend(destAddr, xferFields*dz, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (rowMin && planeMin) {
         int toProc = myRank - domain->tp*domain->tp - domain->tp ;
         destAddr = &domain->commDataSend[pmsg * maxPlaneComm +
                                          emsg * maxEdgeComm] ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            Real_t *srcAddr = fieldData[fi] ;
            for (Index_t i=0; i<dx; ++i) {
               destAddr[i] = srcAddr[i] ;
            }
            destAddr += dx ;
         }
         destAddr -= xferFields*dx ;
         MPI_Isend(destAddr, xferFields*dx, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (colMin && planeMin) {
         int toProc = myRank - domain->tp*domain->tp - 1 ;
         destAddr = &domain->commDataSend[pmsg * maxPlaneComm +
                                          emsg * maxEdgeComm] ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            Real_t *srcAddr = fieldData[fi] ;
            for (Index_t i=0; i<dy; ++i) {
               destAddr[i] = srcAddr[i*dx] ;
            }
            destAddr += dy ;
         }
         destAddr -= xferFields*dy ;
         MPI_Isend(destAddr, xferFields*dy, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (rowMax && colMax && doSend) {
         int toProc = myRank + domain->tp + 1 ;
         destAddr = &domain->commDataSend[pmsg * maxPlaneComm +
                                          emsg * maxEdgeComm] ;
         Index_t offset = dx*dy - 1 ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            Real_t *srcAddr = &fieldData[fi][offset] ;
            for (Index_t i=0; i<dz; ++i) {
               destAddr[i] = srcAddr[i*dx*dy] ;
            }
            destAddr += dz ;
         }
         destAddr -= xferFields*dz ;
         MPI_Isend(destAddr, xferFields*dz, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (rowMax && planeMax && doSend) {
         int toProc = myRank + domain->tp*domain->tp + domain->tp ;
         destAddr = &domain->commDataSend[pmsg * maxPlaneComm +
                                          emsg * maxEdgeComm] ;
         Index_t offset = dx*(dy-1) + dx*dy*(dz-1) ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            Real_t *srcAddr = &fieldData[fi][offset] ;
            for (Index_t i=0; i<dx; ++i) {
              destAddr[i] = srcAddr[i] ;
            }
            destAddr += dx ;
         }
         destAddr -= xferFields*dx ;
         MPI_Isend(destAddr, xferFields*dx, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (colMax && planeMax && doSend) {
         int toProc = myRank + domain->tp*domain->tp + 1 ;
         destAddr = &domain->commDataSend[pmsg * maxPlaneComm +
                                          emsg * maxEdgeComm] ;
         Index_t offset = dx*dy*(dz-1) + dx - 1 ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            Real_t *srcAddr = &fieldData[fi][offset] ;
            for (Index_t i=0; i<dy; ++i) {
               destAddr[i] = srcAddr[i*dx] ;
            }
            destAddr += dy ;
         }
         destAddr -= xferFields*dy ;
         MPI_Isend(destAddr, xferFields*dy, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (rowMax && colMin && doSend) {
         int toProc = myRank + domain->tp - 1 ;
         destAddr = &domain->commDataSend[pmsg * maxPlaneComm +
                                          emsg * maxEdgeComm] ;
         Index_t offset = dx*(dy-1) ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            Real_t *srcAddr = &fieldData[fi][offset] ;
            for (Index_t i=0; i<dz; ++i) {
               destAddr[i] = srcAddr[i*dx*dy] ;
            }
            destAddr += dz ;
         }
         destAddr -= xferFields*dz ;
         MPI_Isend(destAddr, xferFields*dz, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (rowMin && planeMax && doSend) {
         int toProc = myRank + domain->tp*domain->tp - domain->tp ;
         destAddr = &domain->commDataSend[pmsg * maxPlaneComm +
                                          emsg * maxEdgeComm] ;
         Index_t offset = dx*dy*(dz-1) ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            Real_t *srcAddr = &fieldData[fi][offset] ;
            for (Index_t i=0; i<dx; ++i) {
               destAddr[i] = srcAddr[i] ;
            }
            destAddr += dx ;
         }
         destAddr -= xferFields*dx ;
         MPI_Isend(destAddr, xferFields*dx, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (colMin && planeMax && doSend) {
         int toProc = myRank + domain->tp*domain->tp - 1 ;
         destAddr = &domain->commDataSend[pmsg * maxPlaneComm +
                                          emsg * maxEdgeComm] ;
         Index_t offset = dx*dy*(dz-1) ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            Real_t *srcAddr = &fieldData[fi][offset] ;
            for (Index_t i=0; i<dy; ++i) {
               destAddr[i] = srcAddr[i*dx] ;
            }
            destAddr += dy ;
         }
         destAddr -= xferFields*dy ;
         MPI_Isend(destAddr, xferFields*dy, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (rowMin && colMax) {
         int toProc = myRank - domain->tp + 1 ;
         destAddr = &domain->commDataSend[pmsg * maxPlaneComm +
                                          emsg * maxEdgeComm] ;
         Index_t offset = dx - 1 ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            Real_t *srcAddr = &fieldData[fi][offset] ;
            for (Index_t i=0; i<dz; ++i) {
               destAddr[i] = srcAddr[i*dx*dy] ;
            }
            destAddr += dz ;
         }
         destAddr -= xferFields*dz ;
         MPI_Isend(destAddr, xferFields*dz, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (rowMax && planeMin) {
         int toProc = myRank - domain->tp*domain->tp + domain->tp ;
         destAddr = &domain->commDataSend[pmsg * maxPlaneComm +
                                          emsg * maxEdgeComm] ;
         Index_t offset = dx*(dy - 1) ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            Real_t *srcAddr = &fieldData[fi][offset] ;
            for (Index_t i=0; i<dx; ++i) {
               destAddr[i] = srcAddr[i] ;
            }
            destAddr += dx ;
         }
         destAddr -= xferFields*dx ;
         MPI_Isend(destAddr, xferFields*dx, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg]) ;
         ++emsg ;
      }

      if (colMax && planeMin) {
         int toProc = myRank - domain->tp*domain->tp + 1 ;
         destAddr = &domain->commDataSend[pmsg * maxPlaneComm +
                                          emsg * maxEdgeComm] ;
         Index_t offset = dx - 1 ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            Real_t *srcAddr = &fieldData[fi][offset] ;
            for (Index_t i=0; i<dy; ++i) {
               destAddr[i] = srcAddr[i*dx] ;
            }
            destAddr += dy ;
         }
         destAddr -= xferFields*dy ;
         MPI_Isend(destAddr, xferFields*dy, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg]) ;
         ++emsg ;
      }


      if (rowMin && colMin && planeMin) {
         /* corner at domain logical coord (0, 0, 0) */
         int toProc = myRank - domain->tp*domain->tp - domain->tp - 1 ;
         Real_t *comBuf = &domain->commDataSend[pmsg * maxPlaneComm +
                                                emsg * maxEdgeComm +
                                      cmsg * CACHE_COHERENCE_PAD_REAL] ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            comBuf[fi] = fieldData[fi][0] ;
         }
         MPI_Isend(comBuf, xferFields, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg+cmsg]) ;
         ++cmsg ;
      }
      if (rowMin && colMin && planeMax && doSend) {
         /* corner at domain logical coord (0, 0, 1) */
         int toProc = myRank + domain->tp*domain->tp - domain->tp - 1 ;
         Real_t *comBuf = &domain->commDataSend[pmsg * maxPlaneComm +
                                                emsg * maxEdgeComm +
                                         cmsg * CACHE_COHERENCE_PAD_REAL] ;
         Index_t idx = dx*dy*(dz - 1) ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            comBuf[fi] = fieldData[fi][idx] ;
         }
         MPI_Isend(comBuf, xferFields, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg+cmsg]) ;
         ++cmsg ;
      }
      if (rowMin && colMax && planeMin) {
         /* corner at domain logical coord (1, 0, 0) */
         int toProc = myRank - domain->tp*domain->tp - domain->tp + 1 ;
         Real_t *comBuf = &domain->commDataSend[pmsg * maxPlaneComm +
                                                emsg * maxEdgeComm +
                                         cmsg * CACHE_COHERENCE_PAD_REAL] ;
         Index_t idx = dx - 1 ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            comBuf[fi] = fieldData[fi][idx] ;
         }
         MPI_Isend(comBuf, xferFields, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg+cmsg]) ;
         ++cmsg ;
      }
      if (rowMin && colMax && planeMax && doSend) {
         /* corner at domain logical coord (1, 0, 1) */
         int toProc = myRank + domain->tp*domain->tp - domain->tp + 1 ;
         Real_t *comBuf = &domain->commDataSend[pmsg * maxPlaneComm +
                                                emsg * maxEdgeComm +
                                         cmsg * CACHE_COHERENCE_PAD_REAL] ;
         Index_t idx = dx*dy*(dz - 1) + (dx - 1) ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            comBuf[fi] = fieldData[fi][idx] ;
         }
         MPI_Isend(comBuf, xferFields, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg+cmsg]) ;
         ++cmsg ;
      }
      if (rowMax && colMin && planeMin) {
         /* corner at domain logical coord (0, 1, 0) */
         int toProc = myRank - domain->tp*domain->tp + domain->tp - 1 ;
         Real_t *comBuf = &domain->commDataSend[pmsg * maxPlaneComm +
                                                emsg * maxEdgeComm +
                                         cmsg * CACHE_COHERENCE_PAD_REAL] ;
         Index_t idx = dx*(dy - 1) ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            comBuf[fi] = fieldData[fi][idx] ;
         }
         MPI_Isend(comBuf, xferFields, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg+cmsg]) ;
         ++cmsg ;
      }
      if (rowMax && colMin && planeMax && doSend) {
         /* corner at domain logical coord (0, 1, 1) */
         int toProc = myRank + domain->tp*domain->tp + domain->tp - 1 ;
         Real_t *comBuf = &domain->commDataSend[pmsg * maxPlaneComm +
                                                emsg * maxEdgeComm +
                                         cmsg * CACHE_COHERENCE_PAD_REAL] ;
         Index_t idx = dx*dy*(dz - 1) + dx*(dy - 1) ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            comBuf[fi] = fieldData[fi][idx] ;
         }
         MPI_Isend(comBuf, xferFields, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg+cmsg]) ;
         ++cmsg ;
      }
      if (rowMax && colMax && planeMin) {
         /* corner at domain logical coord (1, 1, 0) */
         int toProc = myRank - domain->tp*domain->tp + domain->tp + 1 ;
         Real_t *comBuf = &domain->commDataSend[pmsg * maxPlaneComm +
                                                emsg * maxEdgeComm +
                                         cmsg * CACHE_COHERENCE_PAD_REAL] ;
         Index_t idx = dx*dy - 1 ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            comBuf[fi] = fieldData[fi][idx] ;
         }
         MPI_Isend(comBuf, xferFields, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg+cmsg]) ;
         ++cmsg ;
      }
      if (rowMax && colMax && planeMax && doSend) {
         /* corner at domain logical coord (1, 1, 1) */
         int toProc = myRank + domain->tp*domain->tp + domain->tp + 1 ;
         Real_t *comBuf = &domain->commDataSend[pmsg * maxPlaneComm +
                                                emsg * maxEdgeComm +
                                         cmsg * CACHE_COHERENCE_PAD_REAL] ;
         Index_t idx = dx*dy*dz - 1 ;
         for (Index_t fi=0; fi<xferFields; ++fi) {
            comBuf[fi] = fieldData[fi][idx] ;
         }
         MPI_Isend(comBuf, xferFields, baseType, toProc, msgType,
                   MPI_COMM_WORLD, &domain->sendRequest[pmsg+emsg+cmsg]) ;
         ++cmsg ;
      }
   }

   MPI_Waitall(26, domain->sendRequest, status) ;
}

void CommSBN(Domain *domain, int xferFields, Real_t **fieldData) {

   if (domain->numProcs == 1) return ;

   /* summation order should be from smallest value to largest */
   /* or we could try out kahan summation! */

   int myRank ;
   Index_t maxPlaneComm = xferFields * domain->maxPlaneSize ;
   Index_t maxEdgeComm  = xferFields * domain->maxEdgeSize ;
   Index_t pmsg = 0 ; /* plane comm msg */
   Index_t emsg = 0 ; /* edge comm msg */
   Index_t cmsg = 0 ; /* corner comm msg */
   Index_t dx = domain->sizeX + 1 ;
   Index_t dy = domain->sizeY + 1 ;
   Index_t dz = domain->sizeZ + 1 ;
   MPI_Status status ;
   Real_t *srcAddr ;
   Index_t rowMin, rowMax, colMin, colMax, planeMin, planeMax ;
   /* assume communication to 6 neighbors by default */
   rowMin = rowMax = colMin = colMax = planeMin = planeMax = 1 ;
   if (domain->rowLoc == 0) {
      rowMin = 0 ;
   }
   if (domain->rowLoc == (domain->tp-1)) {
      rowMax = 0 ;
   }
   if (domain->colLoc == 0) {
      colMin = 0 ;
   }
   if (domain->colLoc == (domain->tp-1)) {
      colMax = 0 ;
   }
   if (domain->planeLoc == 0) {
      planeMin = 0 ;
   }
   if (domain->planeLoc == (domain->tp-1)) {
      planeMax = 0 ;
   }

   MPI_Comm_rank(MPI_COMM_WORLD, &myRank) ;

   if (planeMin | planeMax) {
      /* ASSUMING ONE DOMAIN PER RANK, CONSTANT BLOCK SIZE HERE */
      Index_t opCount = dx * dy ;

      if (planeMin) {
         /* contiguous memory */
         srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm] ;
         MPI_Wait(&domain->recvRequest[pmsg], &status) ;
         for (Index_t fi=0 ; fi<xferFields; ++fi) {
            Real_t *destAddr = fieldData[fi] ;
            for (Index_t i=0; i<opCount; ++i) {
               destAddr[i] += srcAddr[i] ;
            }
            srcAddr += opCount ;
         }
         ++pmsg ;
      }
      if (planeMax) {
         /* contiguous memory */
         Index_t offset = dx*dy*(dz - 1) ;
         srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm] ;
         MPI_Wait(&domain->recvRequest[pmsg], &status) ;
         for (Index_t fi=0 ; fi<xferFields; ++fi) {
            Real_t *destAddr = &fieldData[fi][offset] ;
            for (Index_t i=0; i<opCount; ++i) {
               destAddr[i] += srcAddr[i] ;
            }
            srcAddr += opCount ;
         }
         ++pmsg ;
      }
   }

   if (rowMin | rowMax) {
      /* ASSUMING ONE DOMAIN PER RANK, CONSTANT BLOCK SIZE HERE */
      Index_t opCount = dx * dz ;

      if (rowMin) {
         /* contiguous memory */
         srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm] ;
         MPI_Wait(&domain->recvRequest[pmsg], &status) ;
         for (Index_t fi=0 ; fi<xferFields; ++fi) {
            for (Index_t i=0; i<dz; ++i) {
               Real_t *destAddr = &fieldData[fi][i*dx*dy] ;
               for (Index_t j=0; j<dx; ++j) {
                  destAddr[j] += srcAddr[i*dx + j] ;
               }
            }
            srcAddr += opCount ;
         }
         ++pmsg ;
      }
      if (rowMax) {
         /* contiguous memory */
         Index_t offset = dx*(dy - 1) ;
         srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm] ;
         MPI_Wait(&domain->recvRequest[pmsg], &status) ;
         for (Index_t fi=0 ; fi<xferFields; ++fi) {
            for (Index_t i=0; i<dz; ++i) {
               Real_t *destAddr = &fieldData[fi][offset + i*dx*dy] ;
               for (Index_t j=0; j<dx; ++j) {
                  destAddr[j] += srcAddr[i*dx + j] ;
               }
            }
            srcAddr += opCount ;
         }
         ++pmsg ;
      }
   }
   if (colMin | colMax) {
      /* ASSUMING ONE DOMAIN PER RANK, CONSTANT BLOCK SIZE HERE */
      Index_t opCount = dy * dz ;

      if (colMin) {
         /* contiguous memory */
         srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm] ;
         MPI_Wait(&domain->recvRequest[pmsg], &status) ;
         for (Index_t fi=0 ; fi<xferFields; ++fi) {
            for (Index_t i=0; i<dz; ++i) {
               Real_t *destAddr = &fieldData[fi][i*dx*dy] ;
               for (Index_t j=0; j<dy; ++j) {
                  destAddr[j*dx] += srcAddr[i*dy + j] ;
               }
            }
            srcAddr += opCount ;
         }
         ++pmsg ;
      }
      if (colMax) {
         /* contiguous memory */
         Index_t offset = dx - 1 ;
         srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm] ;
         MPI_Wait(&domain->recvRequest[pmsg], &status) ;
         for (Index_t fi=0 ; fi<xferFields; ++fi) {
            for (Index_t i=0; i<dz; ++i) {
               Real_t *destAddr = &fieldData[fi][offset + i*dx*dy] ;
               for (Index_t j=0; j<dy; ++j) {
                  destAddr[j*dx] += srcAddr[i*dy + j] ;
               }
            }
            srcAddr += opCount ;
         }
         ++pmsg ;
      }
   }

   if (rowMin & colMin) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = fieldData[fi] ;
         for (Index_t i=0; i<dz; ++i) {
            destAddr[i*dx*dy] += srcAddr[i] ;
         }
         srcAddr += dz ;
      }
      ++emsg ;
   }

   if (rowMin & planeMin) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = fieldData[fi] ;
         for (Index_t i=0; i<dx; ++i) {
            destAddr[i] += srcAddr[i] ;
         }
         srcAddr += dx ;
      }
      ++emsg ;
   }

   if (colMin & planeMin) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = fieldData[fi] ;
         for (Index_t i=0; i<dy; ++i) {
            destAddr[i*dx] += srcAddr[i] ;
         }
         srcAddr += dy ;
      }
      ++emsg ;
   }

   if (rowMax & colMax) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      Index_t offset = dx*dy - 1 ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = &fieldData[fi][offset] ;
         for (Index_t i=0; i<dz; ++i) {
            destAddr[i*dx*dy] += srcAddr[i] ;
         }
         srcAddr += dz ;
      }
      ++emsg ;
   }

   if (rowMax & planeMax) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      Index_t offset = dx*(dy-1) + dx*dy*(dz-1) ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = &fieldData[fi][offset] ;
         for (Index_t i=0; i<dx; ++i) {
            destAddr[i] += srcAddr[i] ;
         }
         srcAddr += dx ;
      }
      ++emsg ;
   }

   if (colMax & planeMax) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      Index_t offset = dx*dy*(dz-1) + dx - 1 ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = &fieldData[fi][offset] ;
         for (Index_t i=0; i<dy; ++i) {
            destAddr[i*dx] += srcAddr[i] ;
         }
         srcAddr += dy ;
      }
      ++emsg ;
   }

   if (rowMax & colMin) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      Index_t offset = dx*(dy-1) ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = &fieldData[fi][offset] ;
         for (Index_t i=0; i<dz; ++i) {
            destAddr[i*dx*dy] += srcAddr[i] ;
         }
         srcAddr += dz ;
      }
      ++emsg ;
   }

   if (rowMin & planeMax) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      Index_t offset = dx*dy*(dz-1) ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = &fieldData[fi][offset] ;
         for (Index_t i=0; i<dx; ++i) {
            destAddr[i] += srcAddr[i] ;
         }
         srcAddr += dx ;
      }
      ++emsg ;
   }

   if (colMin & planeMax) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      Index_t offset = dx*dy*(dz-1) ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = &fieldData[fi][offset] ;
         for (Index_t i=0; i<dy; ++i) {
            destAddr[i*dx] += srcAddr[i] ;
         }
         srcAddr += dy ;
      }
      ++emsg ;
   }

   if (rowMin & colMax) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      Index_t offset = dx - 1 ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = &fieldData[fi][offset] ;
         for (Index_t i=0; i<dz; ++i) {
            destAddr[i*dx*dy] += srcAddr[i] ;
         }
         srcAddr += dz ;
      }
      ++emsg ;
   }

   if (rowMax & planeMin) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      Index_t offset = dx*(dy - 1) ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = &fieldData[fi][offset] ;
         for (Index_t i=0; i<dx; ++i) {
            destAddr[i] += srcAddr[i] ;
         }
         srcAddr += dx ;
      }
      ++emsg ;
   }

   if (colMax & planeMin) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      Index_t offset = dx - 1 ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = &fieldData[fi][offset] ;
         for (Index_t i=0; i<dy; ++i) {
            destAddr[i*dx] += srcAddr[i] ;
         }
         srcAddr += dy ;
      }
      ++emsg ;
   }


   if (rowMin & colMin & planeMin) {
      /* corner at domain logical coord (0, 0, 0) */
      Real_t *comBuf = &domain->commDataRecv[pmsg * maxPlaneComm +
                                             emsg * maxEdgeComm +
                                      cmsg * CACHE_COHERENCE_PAD_REAL] ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg+cmsg], &status) ;
      for (Index_t fi=0; fi<xferFields; ++fi) {
         fieldData[fi][0] += comBuf[fi] ;
      }
      ++cmsg ;
   }
   if (rowMin & colMin & planeMax) {
      /* corner at domain logical coord (0, 0, 1) */
      Real_t *comBuf = &domain->commDataRecv[pmsg * maxPlaneComm +
                                             emsg * maxEdgeComm +
                                      cmsg * CACHE_COHERENCE_PAD_REAL] ;
      Index_t idx = dx*dy*(dz - 1) ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg+cmsg], &status) ;
      for (Index_t fi=0; fi<xferFields; ++fi) {
         fieldData[fi][idx] += comBuf[fi] ;
      }
      ++cmsg ;
   }
   if (rowMin & colMax & planeMin) {
      /* corner at domain logical coord (1, 0, 0) */
      Real_t *comBuf = &domain->commDataRecv[pmsg * maxPlaneComm +
                                             emsg * maxEdgeComm +
                                      cmsg * CACHE_COHERENCE_PAD_REAL] ;
      Index_t idx = dx - 1 ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg+cmsg], &status) ;
      for (Index_t fi=0; fi<xferFields; ++fi) {
         fieldData[fi][idx] += comBuf[fi] ;
      }
      ++cmsg ;
   }
   if (rowMin & colMax & planeMax) {
      /* corner at domain logical coord (1, 0, 1) */
      Real_t *comBuf = &domain->commDataRecv[pmsg * maxPlaneComm +
                                             emsg * maxEdgeComm +
                                      cmsg * CACHE_COHERENCE_PAD_REAL] ;
      Index_t idx = dx*dy*(dz - 1) + (dx - 1) ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg+cmsg], &status) ;
      for (Index_t fi=0; fi<xferFields; ++fi) {
         fieldData[fi][idx] += comBuf[fi] ;
      }
      ++cmsg ;
   }
   if (rowMax & colMin & planeMin) {
      /* corner at domain logical coord (0, 1, 0) */
      Real_t *comBuf = &domain->commDataRecv[pmsg * maxPlaneComm +
                                             emsg * maxEdgeComm +
                                      cmsg * CACHE_COHERENCE_PAD_REAL] ;
      Index_t idx = dx*(dy - 1) ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg+cmsg], &status) ;
      for (Index_t fi=0; fi<xferFields; ++fi) {
         fieldData[fi][idx] += comBuf[fi] ;
      }
      ++cmsg ;
   }
   if (rowMax & colMin & planeMax) {
      /* corner at domain logical coord (0, 1, 1) */
      Real_t *comBuf = &domain->commDataRecv[pmsg * maxPlaneComm +
                                             emsg * maxEdgeComm +
                                      cmsg * CACHE_COHERENCE_PAD_REAL] ;
      Index_t idx = dx*dy*(dz - 1) + dx*(dy - 1) ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg+cmsg], &status) ;
      for (Index_t fi=0; fi<xferFields; ++fi) {
         fieldData[fi][idx] += comBuf[fi] ;
      }
      ++cmsg ;
   }
   if (rowMax & colMax & planeMin) {
      /* corner at domain logical coord (1, 1, 0) */
      Real_t *comBuf = &domain->commDataRecv[pmsg * maxPlaneComm +
                                             emsg * maxEdgeComm +
                                      cmsg * CACHE_COHERENCE_PAD_REAL] ;
      Index_t idx = dx*dy - 1 ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg+cmsg], &status) ;
      for (Index_t fi=0; fi<xferFields; ++fi) {
         fieldData[fi][idx] += comBuf[fi] ;
      }
      ++cmsg ;
   }
   if (rowMax & colMax & planeMax) {
      /* corner at domain logical coord (1, 1, 1) */
      Real_t *comBuf = &domain->commDataRecv[pmsg * maxPlaneComm +
                                             emsg * maxEdgeComm +
                                      cmsg * CACHE_COHERENCE_PAD_REAL] ;
      Index_t idx = dx*dy*dz - 1 ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg+cmsg], &status) ;
      for (Index_t fi=0; fi<xferFields; ++fi) {
         fieldData[fi][idx] += comBuf[fi] ;
      }
      ++cmsg ;
   }
}

void CommSyncPosVel(Domain *domain) {

   if (domain->numProcs == 1) return ;

   int myRank ;
   bool doRecv = false ;
   Index_t xferFields = 6 ; /* x, y, z, xd, yd, zd */
   Real_t *fieldData[6] ;
   Index_t maxPlaneComm = xferFields * domain->maxPlaneSize ;
   Index_t maxEdgeComm  = xferFields * domain->maxEdgeSize ;
   Index_t pmsg = 0 ; /* plane comm msg */
   Index_t emsg = 0 ; /* edge comm msg */
   Index_t cmsg = 0 ; /* corner comm msg */
   Index_t dx = domain->sizeX + 1 ;
   Index_t dy = domain->sizeY + 1 ;
   Index_t dz = domain->sizeZ + 1 ;
   MPI_Status status ;
   Real_t *srcAddr ;
   bool rowMin, rowMax, colMin, colMax, planeMin, planeMax ;
   /* assume communication to 6 neighbors by default */
   rowMin = rowMax = colMin = colMax = planeMin = planeMax = true ;
   if (domain->rowLoc == 0) {
      rowMin = false ;
   }
   if (domain->rowLoc == (domain->tp-1)) {
      rowMax = false ;
   }
   if (domain->colLoc == 0) {
      colMin = false ;
   }
   if (domain->colLoc == (domain->tp-1)) {
      colMax = false ;
   }
   if (domain->planeLoc == 0) {
      planeMin = false ;
   }
   if (domain->planeLoc == (domain->tp-1)) {
      planeMax = false ;
   }

   fieldData[0] = domain->x ;
   fieldData[1] = domain->y ;
   fieldData[2] = domain->z ;
   fieldData[3] = domain->xd ;
   fieldData[4] = domain->yd ;
   fieldData[5] = domain->zd ;

   MPI_Comm_rank(MPI_COMM_WORLD, &myRank) ;

   if (planeMin | planeMax) {
      /* ASSUMING ONE DOMAIN PER RANK, CONSTANT BLOCK SIZE HERE */
      Index_t opCount = dx * dy ;

      if (planeMin && doRecv) {
         /* contiguous memory */
         srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm] ;
         MPI_Wait(&domain->recvRequest[pmsg], &status) ;
         for (Index_t fi=0 ; fi<xferFields; ++fi) {
            Real_t *destAddr = fieldData[fi] ;
            for (Index_t i=0; i<opCount; ++i) {
               destAddr[i] = srcAddr[i] ;
            }
            srcAddr += opCount ;
         }
         ++pmsg ;
      }
      if (planeMax) {
         /* contiguous memory */
         Index_t offset = dx*dy*(dz - 1) ;
         srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm] ;
         MPI_Wait(&domain->recvRequest[pmsg], &status) ;
         for (Index_t fi=0 ; fi<xferFields; ++fi) {
            Real_t *destAddr = &fieldData[fi][offset] ;
            for (Index_t i=0; i<opCount; ++i) {
               destAddr[i] = srcAddr[i] ;
            }
            srcAddr += opCount ;
         }
         ++pmsg ;
      }
   }

   if (rowMin | rowMax) {
      /* ASSUMING ONE DOMAIN PER RANK, CONSTANT BLOCK SIZE HERE */
      Index_t opCount = dx * dz ;

      if (rowMin && doRecv) {
         /* contiguous memory */
         srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm] ;
         MPI_Wait(&domain->recvRequest[pmsg], &status) ;
         for (Index_t fi=0 ; fi<xferFields; ++fi) {
            for (Index_t i=0; i<dz; ++i) {
               Real_t *destAddr = &fieldData[fi][i*dx*dy] ;
               for (Index_t j=0; j<dx; ++j) {
                  destAddr[j] = srcAddr[i*dx + j] ;
               }
            }
            srcAddr += opCount ;
         }
         ++pmsg ;
      }
      if (rowMax) {
         /* contiguous memory */
         Index_t offset = dx*(dy - 1) ;
         srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm] ;
         MPI_Wait(&domain->recvRequest[pmsg], &status) ;
         for (Index_t fi=0 ; fi<xferFields; ++fi) {
            for (Index_t i=0; i<dz; ++i) {
               Real_t *destAddr = &fieldData[fi][offset + i*dx*dy] ;
               for (Index_t j=0; j<dx; ++j) {
                  destAddr[j] = srcAddr[i*dx + j] ;
               }
            }
            srcAddr += opCount ;
         }
         ++pmsg ;
      }
   }
   if (colMin | colMax) {
      /* ASSUMING ONE DOMAIN PER RANK, CONSTANT BLOCK SIZE HERE */
      Index_t opCount = dy * dz ;

      if (colMin && doRecv) {
         /* contiguous memory */
         srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm] ;
         MPI_Wait(&domain->recvRequest[pmsg], &status) ;
         for (Index_t fi=0 ; fi<xferFields; ++fi) {
            for (Index_t i=0; i<dz; ++i) {
               Real_t *destAddr = &fieldData[fi][i*dx*dy] ;
               for (Index_t j=0; j<dy; ++j) {
                  destAddr[j*dx] = srcAddr[i*dy + j] ;
               }
            }
            srcAddr += opCount ;
         }
         ++pmsg ;
      }
      if (colMax) {
         /* contiguous memory */
         Index_t offset = dx - 1 ;
         srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm] ;
         MPI_Wait(&domain->recvRequest[pmsg], &status) ;
         for (Index_t fi=0 ; fi<xferFields; ++fi) {
            for (Index_t i=0; i<dz; ++i) {
               Real_t *destAddr = &fieldData[fi][offset + i*dx*dy] ;
               for (Index_t j=0; j<dy; ++j) {
                  destAddr[j*dx] = srcAddr[i*dy + j] ;
               }
            }
            srcAddr += opCount ;
         }
         ++pmsg ;
      }
   }

   if (rowMin && colMin && doRecv) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = fieldData[fi] ;
         for (Index_t i=0; i<dz; ++i) {
            destAddr[i*dx*dy] = srcAddr[i] ;
         }
         srcAddr += dz ;
      }
      ++emsg ;
   }

   if (rowMin && planeMin && doRecv) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = fieldData[fi] ;
         for (Index_t i=0; i<dx; ++i) {
            destAddr[i] = srcAddr[i] ;
         }
         srcAddr += dx ;
      }
      ++emsg ;
   }

   if (colMin && planeMin && doRecv) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = fieldData[fi] ;
         for (Index_t i=0; i<dy; ++i) {
            destAddr[i*dx] = srcAddr[i] ;
         }
         srcAddr += dy ;
      }
      ++emsg ;
   }

   if (rowMax && colMax) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      Index_t offset = dx*dy - 1 ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = &fieldData[fi][offset] ;
         for (Index_t i=0; i<dz; ++i) {
            destAddr[i*dx*dy] = srcAddr[i] ;
         }
         srcAddr += dz ;
      }
      ++emsg ;
   }

   if (rowMax && planeMax) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      Index_t offset = dx*(dy-1) + dx*dy*(dz-1) ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = &fieldData[fi][offset] ;
         for (Index_t i=0; i<dx; ++i) {
            destAddr[i] = srcAddr[i] ;
         }
         srcAddr += dx ;
      }
      ++emsg ;
   }

   if (colMax && planeMax) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      Index_t offset = dx*dy*(dz-1) + dx - 1 ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = &fieldData[fi][offset] ;
         for (Index_t i=0; i<dy; ++i) {
            destAddr[i*dx] = srcAddr[i] ;
         }
         srcAddr += dy ;
      }
      ++emsg ;
   }

   if (rowMax && colMin) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      Index_t offset = dx*(dy-1) ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = &fieldData[fi][offset] ;
         for (Index_t i=0; i<dz; ++i) {
            destAddr[i*dx*dy] = srcAddr[i] ;
         }
         srcAddr += dz ;
      }
      ++emsg ;
   }

   if (rowMin && planeMax) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      Index_t offset = dx*dy*(dz-1) ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = &fieldData[fi][offset] ;
         for (Index_t i=0; i<dx; ++i) {
            destAddr[i] = srcAddr[i] ;
         }
         srcAddr += dx ;
      }
      ++emsg ;
   }

   if (colMin && planeMax) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      Index_t offset = dx*dy*(dz-1) ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = &fieldData[fi][offset] ;
         for (Index_t i=0; i<dy; ++i) {
            destAddr[i*dx] = srcAddr[i] ;
         }
         srcAddr += dy ;
      }
      ++emsg ;
   }

   if (rowMin && colMax && doRecv) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      Index_t offset = dx - 1 ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = &fieldData[fi][offset] ;
         for (Index_t i=0; i<dz; ++i) {
            destAddr[i*dx*dy] = srcAddr[i] ;
         }
         srcAddr += dz ;
      }
      ++emsg ;
   }

   if (rowMax && planeMin && doRecv) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      Index_t offset = dx*(dy - 1) ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = &fieldData[fi][offset] ;
         for (Index_t i=0; i<dx; ++i) {
            destAddr[i] = srcAddr[i] ;
         }
         srcAddr += dx ;
      }
      ++emsg ;
   }

   if (colMax && planeMin && doRecv) {
      srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm +
                                       emsg * maxEdgeComm] ;
      Index_t offset = dx - 1 ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg], &status) ;
      for (Index_t fi=0 ; fi<xferFields; ++fi) {
         Real_t *destAddr = &fieldData[fi][offset] ;
         for (Index_t i=0; i<dy; ++i) {
            destAddr[i*dx] = srcAddr[i] ;
         }
         srcAddr += dy ;
      }
      ++emsg ;
   }


   if (rowMin && colMin && planeMin && doRecv) {
      /* corner at domain logical coord (0, 0, 0) */
      Real_t *comBuf = &domain->commDataRecv[pmsg * maxPlaneComm +
                                             emsg * maxEdgeComm +
                                      cmsg * CACHE_COHERENCE_PAD_REAL] ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg+cmsg], &status) ;
      for (Index_t fi=0; fi<xferFields; ++fi) {
         fieldData[fi][0] = comBuf[fi] ;
      }
      ++cmsg ;
   }
   if (rowMin && colMin && planeMax) {
      /* corner at domain logical coord (0, 0, 1) */
      Real_t *comBuf = &domain->commDataRecv[pmsg * maxPlaneComm +
                                             emsg * maxEdgeComm +
                                      cmsg * CACHE_COHERENCE_PAD_REAL] ;
      Index_t idx = dx*dy*(dz - 1) ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg+cmsg], &status) ;
      for (Index_t fi=0; fi<xferFields; ++fi) {
         fieldData[fi][idx] = comBuf[fi] ;
      }
      ++cmsg ;
   }
   if (rowMin && colMax && planeMin && doRecv) {
      /* corner at domain logical coord (1, 0, 0) */
      Real_t *comBuf = &domain->commDataRecv[pmsg * maxPlaneComm +
                                             emsg * maxEdgeComm +
                                      cmsg * CACHE_COHERENCE_PAD_REAL] ;
      Index_t idx = dx - 1 ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg+cmsg], &status) ;
      for (Index_t fi=0; fi<xferFields; ++fi) {
         fieldData[fi][idx] = comBuf[fi] ;
      }
      ++cmsg ;
   }
   if (rowMin && colMax && planeMax) {
      /* corner at domain logical coord (1, 0, 1) */
      Real_t *comBuf = &domain->commDataRecv[pmsg * maxPlaneComm +
                                             emsg * maxEdgeComm +
                                      cmsg * CACHE_COHERENCE_PAD_REAL] ;
      Index_t idx = dx*dy*(dz - 1) + (dx - 1) ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg+cmsg], &status) ;
      for (Index_t fi=0; fi<xferFields; ++fi) {
         fieldData[fi][idx] = comBuf[fi] ;
      }
      ++cmsg ;
   }
   if (rowMax && colMin && planeMin && doRecv) {
      /* corner at domain logical coord (0, 1, 0) */
      Real_t *comBuf = &domain->commDataRecv[pmsg * maxPlaneComm +
                                             emsg * maxEdgeComm +
                                      cmsg * CACHE_COHERENCE_PAD_REAL] ;
      Index_t idx = dx*(dy - 1) ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg+cmsg], &status) ;
      for (Index_t fi=0; fi<xferFields; ++fi) {
         fieldData[fi][idx] = comBuf[fi] ;
      }
      ++cmsg ;
   }
   if (rowMax && colMin && planeMax) {
      /* corner at domain logical coord (0, 1, 1) */
      Real_t *comBuf = &domain->commDataRecv[pmsg * maxPlaneComm +
                                             emsg * maxEdgeComm +
                                      cmsg * CACHE_COHERENCE_PAD_REAL] ;
      Index_t idx = dx*dy*(dz - 1) + dx*(dy - 1) ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg+cmsg], &status) ;
      for (Index_t fi=0; fi<xferFields; ++fi) {
         fieldData[fi][idx] = comBuf[fi] ;
      }
      ++cmsg ;
   }
   if (rowMax && colMax && planeMin && doRecv) {
      /* corner at domain logical coord (1, 1, 0) */
      Real_t *comBuf = &domain->commDataRecv[pmsg * maxPlaneComm +
                                             emsg * maxEdgeComm +
                                      cmsg * CACHE_COHERENCE_PAD_REAL] ;
      Index_t idx = dx*dy - 1 ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg+cmsg], &status) ;
      for (Index_t fi=0; fi<xferFields; ++fi) {
         fieldData[fi][idx] = comBuf[fi] ;
      }
      ++cmsg ;
   }
   if (rowMax && colMax && planeMax) {
      /* corner at domain logical coord (1, 1, 1) */
      Real_t *comBuf = &domain->commDataRecv[pmsg * maxPlaneComm +
                                             emsg * maxEdgeComm +
                                      cmsg * CACHE_COHERENCE_PAD_REAL] ;
      Index_t idx = dx*dy*dz - 1 ;
      MPI_Wait(&domain->recvRequest[pmsg+emsg+cmsg], &status) ;
      for (Index_t fi=0; fi<xferFields; ++fi) {
         fieldData[fi][idx] = comBuf[fi] ;
      }
      ++cmsg ;
   }
}

void CommMonoQ(Domain *domain)
{
   if (domain->numProcs == 1) return ;

   int myRank ;
   Index_t xferFields = 3 ; /* delv_xi, delv_eta, delv_zeta */
   Real_t *fieldData[3] ;
   Index_t maxPlaneComm = xferFields * domain->maxPlaneSize ;
   Index_t pmsg = 0 ; /* plane comm msg */
   Index_t dx = domain->sizeX ;
   Index_t dy = domain->sizeY ;
   Index_t dz = domain->sizeZ ;
   MPI_Status status ;
   Real_t *srcAddr ;
   bool rowMin, rowMax, colMin, colMax, planeMin, planeMax ;
   /* assume communication to 6 neighbors by default */
   rowMin = rowMax = colMin = colMax = planeMin = planeMax = true ;
   if (domain->rowLoc == 0) {
      rowMin = false ;
   }
   if (domain->rowLoc == (domain->tp-1)) {
      rowMax = false ;
   }
   if (domain->colLoc == 0) {
      colMin = false ;
   }
   if (domain->colLoc == (domain->tp-1)) {
      colMax = false ;
   }
   if (domain->planeLoc == 0) {
      planeMin = false ;
   }
   if (domain->planeLoc == (domain->tp-1)) {
      planeMax = false ;
   }

   /* point into ghost data area */
   fieldData[0] = domain->delv_xi + domain->numElem ;
   fieldData[1] = domain->delv_eta + domain->numElem ;
   fieldData[2] = domain->delv_zeta + domain->numElem ;

   MPI_Comm_rank(MPI_COMM_WORLD, &myRank) ;

   if (planeMin | planeMax) {
      /* ASSUMING ONE DOMAIN PER RANK, CONSTANT BLOCK SIZE HERE */
      Index_t opCount = dx * dy ;

      if (planeMin) {
         /* contiguous memory */
         srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm] ;
         MPI_Wait(&domain->recvRequest[pmsg], &status) ;
         for (Index_t fi=0 ; fi<xferFields; ++fi) {
            Real_t *destAddr = fieldData[fi] ;
            for (Index_t i=0; i<opCount; ++i) {
               destAddr[i] = srcAddr[i] ;
            }
            srcAddr += opCount ;
            fieldData[fi] += opCount ;
         }
         ++pmsg ;
      }
      if (planeMax) {
         /* contiguous memory */
         srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm] ;
         MPI_Wait(&domain->recvRequest[pmsg], &status) ;
         for (Index_t fi=0 ; fi<xferFields; ++fi) {
            Real_t *destAddr = fieldData[fi] ;
            for (Index_t i=0; i<opCount; ++i) {
               destAddr[i] = srcAddr[i] ;
            }
            srcAddr += opCount ;
            fieldData[fi] += opCount ;
         }
         ++pmsg ;
      }
   }

   if (rowMin | rowMax) {
      /* ASSUMING ONE DOMAIN PER RANK, CONSTANT BLOCK SIZE HERE */
      Index_t opCount = dx * dz ;

      if (rowMin) {
         /* contiguous memory */
         srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm] ;
         MPI_Wait(&domain->recvRequest[pmsg], &status) ;
         for (Index_t fi=0 ; fi<xferFields; ++fi) {
            Real_t *destAddr = fieldData[fi] ;
            for (Index_t i=0; i<opCount; ++i) {
               destAddr[i] = srcAddr[i] ;
            }
            srcAddr += opCount ;
            fieldData[fi] += opCount ;
         }
         ++pmsg ;
      }
      if (rowMax) {
         /* contiguous memory */
         srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm] ;
         MPI_Wait(&domain->recvRequest[pmsg], &status) ;
         for (Index_t fi=0 ; fi<xferFields; ++fi) {
            Real_t *destAddr = fieldData[fi] ;
            for (Index_t i=0; i<opCount; ++i) {
               destAddr[i] = srcAddr[i] ;
            }
            srcAddr += opCount ;
            fieldData[fi] += opCount ;
         }
         ++pmsg ;
      }
   }
   if (colMin | colMax) {
      /* ASSUMING ONE DOMAIN PER RANK, CONSTANT BLOCK SIZE HERE */
      Index_t opCount = dy * dz ;

      if (colMin) {
         /* contiguous memory */
         srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm] ;
         MPI_Wait(&domain->recvRequest[pmsg], &status) ;
         for (Index_t fi=0 ; fi<xferFields; ++fi) {
            Real_t *destAddr = fieldData[fi] ;
            for (Index_t i=0; i<opCount; ++i) {
               destAddr[i] = srcAddr[i] ;
            }
            srcAddr += opCount ;
            fieldData[fi] += opCount ;
         }
         ++pmsg ;
      }
      if (colMax) {
         /* contiguous memory */
         srcAddr = &domain->commDataRecv[pmsg * maxPlaneComm] ;
         MPI_Wait(&domain->recvRequest[pmsg], &status) ;
         for (Index_t fi=0 ; fi<xferFields; ++fi) {
            Real_t *destAddr = fieldData[fi] ;
            for (Index_t i=0; i<opCount; ++i) {
               destAddr[i] = srcAddr[i] ;
            }
            srcAddr += opCount ;
         }
         ++pmsg ;
      }
   }
}

/******************************************/

/* Work Routines */

static inline
void TimeIncrement(Domain *domain)
{
   Real_t targetdt = domain->stoptime - domain->time ;

   if ((domain->dtfixed <= Real_t(0.0)) && (domain->cycle != Int_t(0))) {
      Real_t ratio ;
      Real_t olddt = domain->deltatime ;

      /* This will require a reduction in parallel */
      Real_t gnewdt = Real_t(1.0e+20) ;
      Real_t newdt ;
      if (domain->dtcourant < gnewdt) {
         gnewdt = domain->dtcourant / Real_t(2.0) ;
      }
      if (domain->dthydro < gnewdt) {
         gnewdt = domain->dthydro * Real_t(2.0) / Real_t(3.0) ;
      }

      MPI_Allreduce(&gnewdt, &newdt, 1,
                    ((sizeof(Real_t) == 4) ? MPI_FLOAT : MPI_DOUBLE),
                    MPI_MIN, MPI_COMM_WORLD) ;

      ratio = newdt / olddt ;
      if (ratio >= Real_t(1.0)) {
         if (ratio < domain->deltatimemultlb) {
            newdt = olddt ;
         }
         else if (ratio > domain->deltatimemultub) {
            newdt = olddt*domain->deltatimemultub ;
         }
      }

      if (newdt > domain->dtmax) {
         newdt = domain->dtmax ;
      }
      domain->deltatime = newdt ;
   }

   /* TRY TO PREVENT VERY SMALL SCALING ON THE NEXT CYCLE */
   if ((targetdt > domain->deltatime) &&
       (targetdt < (Real_t(4.0) * domain->deltatime / Real_t(3.0))) ) {
      targetdt = Real_t(2.0) * domain->deltatime / Real_t(3.0) ;
   }

   if (targetdt < domain->deltatime) {
      domain->deltatime = targetdt ;
   }

   domain->time += domain->deltatime ;

   ++domain->cycle ;
}

static inline
void InitStressTermsForElems(Real_t *p, Real_t *q,
                             Real_t *sigxx, Real_t *sigyy, Real_t *sigzz,
                             Index_t numElem)
{
   //
   // pull in the stresses appropriate to the hydro integration
   //

#pragma omp parallel for firstprivate(numElem)
   for (Index_t i = 0 ; i < numElem ; ++i){
      sigxx[i] = sigyy[i] = sigzz[i] =  - p[i] - q[i] ;
   }
}

static inline
void CalcElemShapeFunctionDerivatives( Real_t const x[],
                                       Real_t const y[],
                                       Real_t const z[],
                                       Real_t b[][8],
                                       Real_t* const volume )
{
  const Real_t x0 = x[0] ;   const Real_t x1 = x[1] ;
  const Real_t x2 = x[2] ;   const Real_t x3 = x[3] ;
  const Real_t x4 = x[4] ;   const Real_t x5 = x[5] ;
  const Real_t x6 = x[6] ;   const Real_t x7 = x[7] ;

  const Real_t y0 = y[0] ;   const Real_t y1 = y[1] ;
  const Real_t y2 = y[2] ;   const Real_t y3 = y[3] ;
  const Real_t y4 = y[4] ;   const Real_t y5 = y[5] ;
  const Real_t y6 = y[6] ;   const Real_t y7 = y[7] ;

  const Real_t z0 = z[0] ;   const Real_t z1 = z[1] ;
  const Real_t z2 = z[2] ;   const Real_t z3 = z[3] ;
  const Real_t z4 = z[4] ;   const Real_t z5 = z[5] ;
  const Real_t z6 = z[6] ;   const Real_t z7 = z[7] ;

  Real_t fjxxi, fjxet, fjxze;
  Real_t fjyxi, fjyet, fjyze;
  Real_t fjzxi, fjzet, fjzze;
  Real_t cjxxi, cjxet, cjxze;
  Real_t cjyxi, cjyet, cjyze;
  Real_t cjzxi, cjzet, cjzze;

  fjxxi = .125 * ( (x6-x0) + (x5-x3) - (x7-x1) - (x4-x2) );
  fjxet = .125 * ( (x6-x0) - (x5-x3) + (x7-x1) - (x4-x2) );
  fjxze = .125 * ( (x6-x0) + (x5-x3) + (x7-x1) + (x4-x2) );

  fjyxi = .125 * ( (y6-y0) + (y5-y3) - (y7-y1) - (y4-y2) );
  fjyet = .125 * ( (y6-y0) - (y5-y3) + (y7-y1) - (y4-y2) );
  fjyze = .125 * ( (y6-y0) + (y5-y3) + (y7-y1) + (y4-y2) );

  fjzxi = .125 * ( (z6-z0) + (z5-z3) - (z7-z1) - (z4-z2) );
  fjzet = .125 * ( (z6-z0) - (z5-z3) + (z7-z1) - (z4-z2) );
  fjzze = .125 * ( (z6-z0) + (z5-z3) + (z7-z1) + (z4-z2) );

  /* compute cofactors */
  cjxxi =    (fjyet * fjzze) - (fjzet * fjyze);
  cjxet =  - (fjyxi * fjzze) + (fjzxi * fjyze);
  cjxze =    (fjyxi * fjzet) - (fjzxi * fjyet);

  cjyxi =  - (fjxet * fjzze) + (fjzet * fjxze);
  cjyet =    (fjxxi * fjzze) - (fjzxi * fjxze);
  cjyze =  - (fjxxi * fjzet) + (fjzxi * fjxet);

  cjzxi =    (fjxet * fjyze) - (fjyet * fjxze);
  cjzet =  - (fjxxi * fjyze) + (fjyxi * fjxze);
  cjzze =    (fjxxi * fjyet) - (fjyxi * fjxet);

  /* calculate partials :
     this need only be done for l = 0,1,2,3   since , by symmetry ,
     (6,7,4,5) = - (0,1,2,3) .
  */
  b[0][0] =   -  cjxxi  -  cjxet  -  cjxze;
  b[0][1] =      cjxxi  -  cjxet  -  cjxze;
  b[0][2] =      cjxxi  +  cjxet  -  cjxze;
  b[0][3] =   -  cjxxi  +  cjxet  -  cjxze;
  b[0][4] = -b[0][2];
  b[0][5] = -b[0][3];
  b[0][6] = -b[0][0];
  b[0][7] = -b[0][1];

  b[1][0] =   -  cjyxi  -  cjyet  -  cjyze;
  b[1][1] =      cjyxi  -  cjyet  -  cjyze;
  b[1][2] =      cjyxi  +  cjyet  -  cjyze;
  b[1][3] =   -  cjyxi  +  cjyet  -  cjyze;
  b[1][4] = -b[1][2];
  b[1][5] = -b[1][3];
  b[1][6] = -b[1][0];
  b[1][7] = -b[1][1];

  b[2][0] =   -  cjzxi  -  cjzet  -  cjzze;
  b[2][1] =      cjzxi  -  cjzet  -  cjzze;
  b[2][2] =      cjzxi  +  cjzet  -  cjzze;
  b[2][3] =   -  cjzxi  +  cjzet  -  cjzze;
  b[2][4] = -b[2][2];
  b[2][5] = -b[2][3];
  b[2][6] = -b[2][0];
  b[2][7] = -b[2][1];

  /* calculate jacobian determinant (volume) */
  *volume = Real_t(8.) * ( fjxet * cjxet + fjyet * cjyet + fjzet * cjzet);
}

static inline
void SumElemFaceNormal(Real_t *normalX0, Real_t *normalY0, Real_t *normalZ0,
                       Real_t *normalX1, Real_t *normalY1, Real_t *normalZ1,
                       Real_t *normalX2, Real_t *normalY2, Real_t *normalZ2,
                       Real_t *normalX3, Real_t *normalY3, Real_t *normalZ3,
                       const Real_t x0, const Real_t y0, const Real_t z0,
                       const Real_t x1, const Real_t y1, const Real_t z1,
                       const Real_t x2, const Real_t y2, const Real_t z2,
                       const Real_t x3, const Real_t y3, const Real_t z3)
{
   Real_t bisectX0 = Real_t(0.5) * (x3 + x2 - x1 - x0);
   Real_t bisectY0 = Real_t(0.5) * (y3 + y2 - y1 - y0);
   Real_t bisectZ0 = Real_t(0.5) * (z3 + z2 - z1 - z0);
   Real_t bisectX1 = Real_t(0.5) * (x2 + x1 - x3 - x0);
   Real_t bisectY1 = Real_t(0.5) * (y2 + y1 - y3 - y0);
   Real_t bisectZ1 = Real_t(0.5) * (z2 + z1 - z3 - z0);
   Real_t areaX = Real_t(0.25) * (bisectY0 * bisectZ1 - bisectZ0 * bisectY1);
   Real_t areaY = Real_t(0.25) * (bisectZ0 * bisectX1 - bisectX0 * bisectZ1);
   Real_t areaZ = Real_t(0.25) * (bisectX0 * bisectY1 - bisectY0 * bisectX1);

   *normalX0 += areaX;
   *normalX1 += areaX;
   *normalX2 += areaX;
   *normalX3 += areaX;

   *normalY0 += areaY;
   *normalY1 += areaY;
   *normalY2 += areaY;
   *normalY3 += areaY;

   *normalZ0 += areaZ;
   *normalZ1 += areaZ;
   *normalZ2 += areaZ;
   *normalZ3 += areaZ;
}

static inline
void CalcElemNodeNormals(Real_t pfx[8],
                         Real_t pfy[8],
                         Real_t pfz[8],
                         const Real_t x[8],
                         const Real_t y[8],
                         const Real_t z[8])
{
   for (Index_t i = 0 ; i < 8 ; ++i) {
      pfx[i] = Real_t(0.0);
      pfy[i] = Real_t(0.0);
      pfz[i] = Real_t(0.0);
   }
   /* evaluate face one: nodes 0, 1, 2, 3 */
   SumElemFaceNormal(&pfx[0], &pfy[0], &pfz[0],
                  &pfx[1], &pfy[1], &pfz[1],
                  &pfx[2], &pfy[2], &pfz[2],
                  &pfx[3], &pfy[3], &pfz[3],
                  x[0], y[0], z[0], x[1], y[1], z[1],
                  x[2], y[2], z[2], x[3], y[3], z[3]);
   /* evaluate face two: nodes 0, 4, 5, 1 */
   SumElemFaceNormal(&pfx[0], &pfy[0], &pfz[0],
                  &pfx[4], &pfy[4], &pfz[4],
                  &pfx[5], &pfy[5], &pfz[5],
                  &pfx[1], &pfy[1], &pfz[1],
                  x[0], y[0], z[0], x[4], y[4], z[4],
                  x[5], y[5], z[5], x[1], y[1], z[1]);
   /* evaluate face three: nodes 1, 5, 6, 2 */
   SumElemFaceNormal(&pfx[1], &pfy[1], &pfz[1],
                  &pfx[5], &pfy[5], &pfz[5],
                  &pfx[6], &pfy[6], &pfz[6],
                  &pfx[2], &pfy[2], &pfz[2],
                  x[1], y[1], z[1], x[5], y[5], z[5],
                  x[6], y[6], z[6], x[2], y[2], z[2]);
   /* evaluate face four: nodes 2, 6, 7, 3 */
   SumElemFaceNormal(&pfx[2], &pfy[2], &pfz[2],
                  &pfx[6], &pfy[6], &pfz[6],
                  &pfx[7], &pfy[7], &pfz[7],
                  &pfx[3], &pfy[3], &pfz[3],
                  x[2], y[2], z[2], x[6], y[6], z[6],
                  x[7], y[7], z[7], x[3], y[3], z[3]);
   /* evaluate face five: nodes 3, 7, 4, 0 */
   SumElemFaceNormal(&pfx[3], &pfy[3], &pfz[3],
                  &pfx[7], &pfy[7], &pfz[7],
                  &pfx[4], &pfy[4], &pfz[4],
                  &pfx[0], &pfy[0], &pfz[0],
                  x[3], y[3], z[3], x[7], y[7], z[7],
                  x[4], y[4], z[4], x[0], y[0], z[0]);
   /* evaluate face six: nodes 4, 7, 6, 5 */
   SumElemFaceNormal(&pfx[4], &pfy[4], &pfz[4],
                  &pfx[7], &pfy[7], &pfz[7],
                  &pfx[6], &pfy[6], &pfz[6],
                  &pfx[5], &pfy[5], &pfz[5],
                  x[4], y[4], z[4], x[7], y[7], z[7],
                  x[6], y[6], z[6], x[5], y[5], z[5]);
}

static inline
void SumElemStressesToNodeForces( const Real_t B[][8],
                                  const Real_t stress_xx,
                                  const Real_t stress_yy,
                                  const Real_t stress_zz,
                                  Real_t fx[], Real_t fy[], Real_t fz[] )
{
  Real_t pfx0 = B[0][0] ;   Real_t pfx1 = B[0][1] ;
  Real_t pfx2 = B[0][2] ;   Real_t pfx3 = B[0][3] ;
  Real_t pfx4 = B[0][4] ;   Real_t pfx5 = B[0][5] ;
  Real_t pfx6 = B[0][6] ;   Real_t pfx7 = B[0][7] ;

  Real_t pfy0 = B[1][0] ;   Real_t pfy1 = B[1][1] ;
  Real_t pfy2 = B[1][2] ;   Real_t pfy3 = B[1][3] ;
  Real_t pfy4 = B[1][4] ;   Real_t pfy5 = B[1][5] ;
  Real_t pfy6 = B[1][6] ;   Real_t pfy7 = B[1][7] ;

  Real_t pfz0 = B[2][0] ;   Real_t pfz1 = B[2][1] ;
  Real_t pfz2 = B[2][2] ;   Real_t pfz3 = B[2][3] ;
  Real_t pfz4 = B[2][4] ;   Real_t pfz5 = B[2][5] ;
  Real_t pfz6 = B[2][6] ;   Real_t pfz7 = B[2][7] ;

  fx[0] = -( stress_xx * pfx0 );
  fx[1] = -( stress_xx * pfx1 );
  fx[2] = -( stress_xx * pfx2 );
  fx[3] = -( stress_xx * pfx3 );
  fx[4] = -( stress_xx * pfx4 );
  fx[5] = -( stress_xx * pfx5 );
  fx[6] = -( stress_xx * pfx6 );
  fx[7] = -( stress_xx * pfx7 );

  fy[0] = -( stress_yy * pfy0  );
  fy[1] = -( stress_yy * pfy1  );
  fy[2] = -( stress_yy * pfy2  );
  fy[3] = -( stress_yy * pfy3  );
  fy[4] = -( stress_yy * pfy4  );
  fy[5] = -( stress_yy * pfy5  );
  fy[6] = -( stress_yy * pfy6  );
  fy[7] = -( stress_yy * pfy7  );

  fz[0] = -( stress_zz * pfz0 );
  fz[1] = -( stress_zz * pfz1 );
  fz[2] = -( stress_zz * pfz2 );
  fz[3] = -( stress_zz * pfz3 );
  fz[4] = -( stress_zz * pfz4 );
  fz[5] = -( stress_zz * pfz5 );
  fz[6] = -( stress_zz * pfz6 );
  fz[7] = -( stress_zz * pfz7 );
}

static inline
void IntegrateStressForElems( Index_t *nodelist,
                              Real_t *x,  Real_t *y,  Real_t *z,
                              Real_t *fx, Real_t *fy, Real_t *fz,
                              Index_t *nodeElemCount,
                              Index_t *nodeElemStart,
                              Index_t *nodeElemCornerList,
                              Real_t *sigxx, Real_t *sigyy, Real_t *sigzz,
                              Real_t *determ, Index_t numElem, Index_t numNode)
{
   Index_t numElem8 = numElem * 8 ;
   Real_t *fx_elem = Allocate<Real_t>(numElem8) ;
   Real_t *fy_elem = Allocate<Real_t>(numElem8) ;
   Real_t *fz_elem = Allocate<Real_t>(numElem8) ;


  // loop over all elements

#pragma omp parallel for firstprivate(numElem)
  for( Index_t k=0 ; k<numElem ; ++k )
  {
  Real_t B[3][8] ;// shape function derivatives
  Real_t x_local[8] ;
  Real_t y_local[8] ;
  Real_t z_local[8] ;
    const Index_t* const elemNodes = &nodelist[8*k];

    // get nodal coordinates from global arrays and copy into local arrays.
    for( Index_t lnode=0 ; lnode<8 ; ++lnode )
    {
      Index_t gnode = elemNodes[lnode];
      x_local[lnode] = x[gnode];
      y_local[lnode] = y[gnode];
      z_local[lnode] = z[gnode];
    }

    /* Volume calculation involves extra work for numerical consistency. */
    CalcElemShapeFunctionDerivatives(x_local, y_local, z_local,
                                         B, &determ[k]);

    CalcElemNodeNormals( B[0] , B[1], B[2],
                          x_local, y_local, z_local );

    SumElemStressesToNodeForces( B, sigxx[k], sigyy[k], sigzz[k],
                                 &fx_elem[k*8], &fy_elem[k*8], &fz_elem[k*8] ) ;
  }

  {
#pragma omp parallel for firstprivate(numNode)
     for( Index_t gnode=0 ; gnode<numNode ; ++gnode )
     {
        Index_t count = nodeElemCount[gnode] ;
        Index_t start = nodeElemStart[gnode] ;
        Real_t fx_tmp = Real_t(0.0) ;
        Real_t fy_tmp = Real_t(0.0) ;
        Real_t fz_tmp = Real_t(0.0) ;
        for (Index_t i=0 ; i < count ; ++i) {
           Index_t elem = nodeElemCornerList[start+i] ;
           fx_tmp += fx_elem[elem] ;
           fy_tmp += fy_elem[elem] ;
           fz_tmp += fz_elem[elem] ;
        }
        fx[gnode] = fx_tmp ;
        fy[gnode] = fy_tmp ;
        fz[gnode] = fz_tmp ;
     }
  }

  Release(&fz_elem) ;
  Release(&fy_elem) ;
  Release(&fx_elem) ;
}

static inline
void CollectDomainNodesToElemNodes(Real_t *x, Real_t *y, Real_t *z,
                                   const Index_t* elemToNode,
                                   Real_t elemX[8],
                                   Real_t elemY[8],
                                   Real_t elemZ[8])
{
   Index_t nd0i = elemToNode[0] ;
   Index_t nd1i = elemToNode[1] ;
   Index_t nd2i = elemToNode[2] ;
   Index_t nd3i = elemToNode[3] ;
   Index_t nd4i = elemToNode[4] ;
   Index_t nd5i = elemToNode[5] ;
   Index_t nd6i = elemToNode[6] ;
   Index_t nd7i = elemToNode[7] ;

   elemX[0] = x[nd0i];
   elemX[1] = x[nd1i];
   elemX[2] = x[nd2i];
   elemX[3] = x[nd3i];
   elemX[4] = x[nd4i];
   elemX[5] = x[nd5i];
   elemX[6] = x[nd6i];
   elemX[7] = x[nd7i];

   elemY[0] = y[nd0i];
   elemY[1] = y[nd1i];
   elemY[2] = y[nd2i];
   elemY[3] = y[nd3i];
   elemY[4] = y[nd4i];
   elemY[5] = y[nd5i];
   elemY[6] = y[nd6i];
   elemY[7] = y[nd7i];

   elemZ[0] = z[nd0i];
   elemZ[1] = z[nd1i];
   elemZ[2] = z[nd2i];
   elemZ[3] = z[nd3i];
   elemZ[4] = z[nd4i];
   elemZ[5] = z[nd5i];
   elemZ[6] = z[nd6i];
   elemZ[7] = z[nd7i];

}

static inline
void VoluDer(const Real_t x0, const Real_t x1, const Real_t x2,
             const Real_t x3, const Real_t x4, const Real_t x5,
             const Real_t y0, const Real_t y1, const Real_t y2,
             const Real_t y3, const Real_t y4, const Real_t y5,
             const Real_t z0, const Real_t z1, const Real_t z2,
             const Real_t z3, const Real_t z4, const Real_t z5,
             Real_t* dvdx, Real_t* dvdy, Real_t* dvdz)
{
   const Real_t twelfth = Real_t(1.0) / Real_t(12.0) ;

   *dvdx =
      (y1 + y2) * (z0 + z1) - (y0 + y1) * (z1 + z2) +
      (y0 + y4) * (z3 + z4) - (y3 + y4) * (z0 + z4) -
      (y2 + y5) * (z3 + z5) + (y3 + y5) * (z2 + z5);
   *dvdy =
      - (x1 + x2) * (z0 + z1) + (x0 + x1) * (z1 + z2) -
      (x0 + x4) * (z3 + z4) + (x3 + x4) * (z0 + z4) +
      (x2 + x5) * (z3 + z5) - (x3 + x5) * (z2 + z5);

   *dvdz =
      - (y1 + y2) * (x0 + x1) + (y0 + y1) * (x1 + x2) -
      (y0 + y4) * (x3 + x4) + (y3 + y4) * (x0 + x4) +
      (y2 + y5) * (x3 + x5) - (y3 + y5) * (x2 + x5);

   *dvdx *= twelfth;
   *dvdy *= twelfth;
   *dvdz *= twelfth;
}

static inline
void CalcElemVolumeDerivative(Real_t dvdx[8],
                              Real_t dvdy[8],
                              Real_t dvdz[8],
                              const Real_t x[8],
                              const Real_t y[8],
                              const Real_t z[8])
{
   VoluDer(x[1], x[2], x[3], x[4], x[5], x[7],
           y[1], y[2], y[3], y[4], y[5], y[7],
           z[1], z[2], z[3], z[4], z[5], z[7],
           &dvdx[0], &dvdy[0], &dvdz[0]);
   VoluDer(x[0], x[1], x[2], x[7], x[4], x[6],
           y[0], y[1], y[2], y[7], y[4], y[6],
           z[0], z[1], z[2], z[7], z[4], z[6],
           &dvdx[3], &dvdy[3], &dvdz[3]);
   VoluDer(x[3], x[0], x[1], x[6], x[7], x[5],
           y[3], y[0], y[1], y[6], y[7], y[5],
           z[3], z[0], z[1], z[6], z[7], z[5],
           &dvdx[2], &dvdy[2], &dvdz[2]);
   VoluDer(x[2], x[3], x[0], x[5], x[6], x[4],
           y[2], y[3], y[0], y[5], y[6], y[4],
           z[2], z[3], z[0], z[5], z[6], z[4],
           &dvdx[1], &dvdy[1], &dvdz[1]);
   VoluDer(x[7], x[6], x[5], x[0], x[3], x[1],
           y[7], y[6], y[5], y[0], y[3], y[1],
           z[7], z[6], z[5], z[0], z[3], z[1],
           &dvdx[4], &dvdy[4], &dvdz[4]);
   VoluDer(x[4], x[7], x[6], x[1], x[0], x[2],
           y[4], y[7], y[6], y[1], y[0], y[2],
           z[4], z[7], z[6], z[1], z[0], z[2],
           &dvdx[5], &dvdy[5], &dvdz[5]);
   VoluDer(x[5], x[4], x[7], x[2], x[1], x[3],
           y[5], y[4], y[7], y[2], y[1], y[3],
           z[5], z[4], z[7], z[2], z[1], z[3],
           &dvdx[6], &dvdy[6], &dvdz[6]);
   VoluDer(x[6], x[5], x[4], x[3], x[2], x[0],
           y[6], y[5], y[4], y[3], y[2], y[0],
           z[6], z[5], z[4], z[3], z[2], z[0],
           &dvdx[7], &dvdy[7], &dvdz[7]);
}

static inline
void CalcElemFBHourglassForce(Real_t *xd, Real_t *yd, Real_t *zd,  Real_t *hourgam0,
                              Real_t *hourgam1, Real_t *hourgam2, Real_t *hourgam3,
                              Real_t *hourgam4, Real_t *hourgam5, Real_t *hourgam6,
                              Real_t *hourgam7, Real_t coefficient,
                              Real_t *hgfx, Real_t *hgfy, Real_t *hgfz )
{
   Index_t i00=0;
   Index_t i01=1;
   Index_t i02=2;
   Index_t i03=3;

   Real_t h00 =
      hourgam0[i00] * xd[0] + hourgam1[i00] * xd[1] +
      hourgam2[i00] * xd[2] + hourgam3[i00] * xd[3] +
      hourgam4[i00] * xd[4] + hourgam5[i00] * xd[5] +
      hourgam6[i00] * xd[6] + hourgam7[i00] * xd[7];

   Real_t h01 =
      hourgam0[i01] * xd[0] + hourgam1[i01] * xd[1] +
      hourgam2[i01] * xd[2] + hourgam3[i01] * xd[3] +
      hourgam4[i01] * xd[4] + hourgam5[i01] * xd[5] +
      hourgam6[i01] * xd[6] + hourgam7[i01] * xd[7];

   Real_t h02 =
      hourgam0[i02] * xd[0] + hourgam1[i02] * xd[1]+
      hourgam2[i02] * xd[2] + hourgam3[i02] * xd[3]+
      hourgam4[i02] * xd[4] + hourgam5[i02] * xd[5]+
      hourgam6[i02] * xd[6] + hourgam7[i02] * xd[7];

   Real_t h03 =
      hourgam0[i03] * xd[0] + hourgam1[i03] * xd[1] +
      hourgam2[i03] * xd[2] + hourgam3[i03] * xd[3] +
      hourgam4[i03] * xd[4] + hourgam5[i03] * xd[5] +
      hourgam6[i03] * xd[6] + hourgam7[i03] * xd[7];

   hgfx[0] = coefficient *
      (hourgam0[i00] * h00 + hourgam0[i01] * h01 +
       hourgam0[i02] * h02 + hourgam0[i03] * h03);

   hgfx[1] = coefficient *
      (hourgam1[i00] * h00 + hourgam1[i01] * h01 +
       hourgam1[i02] * h02 + hourgam1[i03] * h03);

   hgfx[2] = coefficient *
      (hourgam2[i00] * h00 + hourgam2[i01] * h01 +
       hourgam2[i02] * h02 + hourgam2[i03] * h03);

   hgfx[3] = coefficient *
      (hourgam3[i00] * h00 + hourgam3[i01] * h01 +
       hourgam3[i02] * h02 + hourgam3[i03] * h03);

   hgfx[4] = coefficient *
      (hourgam4[i00] * h00 + hourgam4[i01] * h01 +
       hourgam4[i02] * h02 + hourgam4[i03] * h03);

   hgfx[5] = coefficient *
      (hourgam5[i00] * h00 + hourgam5[i01] * h01 +
       hourgam5[i02] * h02 + hourgam5[i03] * h03);

   hgfx[6] = coefficient *
      (hourgam6[i00] * h00 + hourgam6[i01] * h01 +
       hourgam6[i02] * h02 + hourgam6[i03] * h03);

   hgfx[7] = coefficient *
      (hourgam7[i00] * h00 + hourgam7[i01] * h01 +
       hourgam7[i02] * h02 + hourgam7[i03] * h03);

   h00 =
      hourgam0[i00] * yd[0] + hourgam1[i00] * yd[1] +
      hourgam2[i00] * yd[2] + hourgam3[i00] * yd[3] +
      hourgam4[i00] * yd[4] + hourgam5[i00] * yd[5] +
      hourgam6[i00] * yd[6] + hourgam7[i00] * yd[7];

   h01 =
      hourgam0[i01] * yd[0] + hourgam1[i01] * yd[1] +
      hourgam2[i01] * yd[2] + hourgam3[i01] * yd[3] +
      hourgam4[i01] * yd[4] + hourgam5[i01] * yd[5] +
      hourgam6[i01] * yd[6] + hourgam7[i01] * yd[7];

   h02 =
      hourgam0[i02] * yd[0] + hourgam1[i02] * yd[1]+
      hourgam2[i02] * yd[2] + hourgam3[i02] * yd[3]+
      hourgam4[i02] * yd[4] + hourgam5[i02] * yd[5]+
      hourgam6[i02] * yd[6] + hourgam7[i02] * yd[7];

   h03 =
      hourgam0[i03] * yd[0] + hourgam1[i03] * yd[1] +
      hourgam2[i03] * yd[2] + hourgam3[i03] * yd[3] +
      hourgam4[i03] * yd[4] + hourgam5[i03] * yd[5] +
      hourgam6[i03] * yd[6] + hourgam7[i03] * yd[7];


   hgfy[0] = coefficient *
      (hourgam0[i00] * h00 + hourgam0[i01] * h01 +
       hourgam0[i02] * h02 + hourgam0[i03] * h03);

   hgfy[1] = coefficient *
      (hourgam1[i00] * h00 + hourgam1[i01] * h01 +
       hourgam1[i02] * h02 + hourgam1[i03] * h03);

   hgfy[2] = coefficient *
      (hourgam2[i00] * h00 + hourgam2[i01] * h01 +
       hourgam2[i02] * h02 + hourgam2[i03] * h03);

   hgfy[3] = coefficient *
      (hourgam3[i00] * h00 + hourgam3[i01] * h01 +
       hourgam3[i02] * h02 + hourgam3[i03] * h03);

   hgfy[4] = coefficient *
      (hourgam4[i00] * h00 + hourgam4[i01] * h01 +
       hourgam4[i02] * h02 + hourgam4[i03] * h03);

   hgfy[5] = coefficient *
      (hourgam5[i00] * h00 + hourgam5[i01] * h01 +
       hourgam5[i02] * h02 + hourgam5[i03] * h03);

   hgfy[6] = coefficient *
      (hourgam6[i00] * h00 + hourgam6[i01] * h01 +
       hourgam6[i02] * h02 + hourgam6[i03] * h03);

   hgfy[7] = coefficient *
      (hourgam7[i00] * h00 + hourgam7[i01] * h01 +
       hourgam7[i02] * h02 + hourgam7[i03] * h03);

   h00 =
      hourgam0[i00] * zd[0] + hourgam1[i00] * zd[1] +
      hourgam2[i00] * zd[2] + hourgam3[i00] * zd[3] +
      hourgam4[i00] * zd[4] + hourgam5[i00] * zd[5] +
      hourgam6[i00] * zd[6] + hourgam7[i00] * zd[7];

   h01 =
      hourgam0[i01] * zd[0] + hourgam1[i01] * zd[1] +
      hourgam2[i01] * zd[2] + hourgam3[i01] * zd[3] +
      hourgam4[i01] * zd[4] + hourgam5[i01] * zd[5] +
      hourgam6[i01] * zd[6] + hourgam7[i01] * zd[7];

   h02 =
      hourgam0[i02] * zd[0] + hourgam1[i02] * zd[1]+
      hourgam2[i02] * zd[2] + hourgam3[i02] * zd[3]+
      hourgam4[i02] * zd[4] + hourgam5[i02] * zd[5]+
      hourgam6[i02] * zd[6] + hourgam7[i02] * zd[7];

   h03 =
      hourgam0[i03] * zd[0] + hourgam1[i03] * zd[1] +
      hourgam2[i03] * zd[2] + hourgam3[i03] * zd[3] +
      hourgam4[i03] * zd[4] + hourgam5[i03] * zd[5] +
      hourgam6[i03] * zd[6] + hourgam7[i03] * zd[7];


   hgfz[0] = coefficient *
      (hourgam0[i00] * h00 + hourgam0[i01] * h01 +
       hourgam0[i02] * h02 + hourgam0[i03] * h03);

   hgfz[1] = coefficient *
      (hourgam1[i00] * h00 + hourgam1[i01] * h01 +
       hourgam1[i02] * h02 + hourgam1[i03] * h03);

   hgfz[2] = coefficient *
      (hourgam2[i00] * h00 + hourgam2[i01] * h01 +
       hourgam2[i02] * h02 + hourgam2[i03] * h03);

   hgfz[3] = coefficient *
      (hourgam3[i00] * h00 + hourgam3[i01] * h01 +
       hourgam3[i02] * h02 + hourgam3[i03] * h03);

   hgfz[4] = coefficient *
      (hourgam4[i00] * h00 + hourgam4[i01] * h01 +
       hourgam4[i02] * h02 + hourgam4[i03] * h03);

   hgfz[5] = coefficient *
      (hourgam5[i00] * h00 + hourgam5[i01] * h01 +
       hourgam5[i02] * h02 + hourgam5[i03] * h03);

   hgfz[6] = coefficient *
      (hourgam6[i00] * h00 + hourgam6[i01] * h01 +
       hourgam6[i02] * h02 + hourgam6[i03] * h03);

   hgfz[7] = coefficient *
      (hourgam7[i00] * h00 + hourgam7[i01] * h01 +
       hourgam7[i02] * h02 + hourgam7[i03] * h03);
}

static inline
void CalcFBHourglassForceForElems( Index_t *nodelist,
                                   Real_t *ss, Real_t *elemMass,
                                   Real_t *xd, Real_t *yd, Real_t *zd,
                                   Real_t *fx, Real_t *fy, Real_t *fz,
                                   Index_t *nodeElemCount,
                                   Index_t *nodeElemStart,
                                   Index_t *nodeElemCornerList,
                                   Real_t *determ,
                                   Real_t *x8n, Real_t *y8n, Real_t *z8n,
                                   Real_t *dvdx, Real_t *dvdy, Real_t *dvdz,
                                   Real_t hourg, Index_t numElem,
                                   Index_t numNode)
{
   /*************************************************
    *
    *     FUNCTION: Calculates the Flanagan-Belytschko anti-hourglass
    *               force.
    *
    *************************************************/

   Index_t numElem8 = numElem * 8 ;
   Real_t *fx_elem = Allocate<Real_t>(numElem8) ;
   Real_t *fy_elem = Allocate<Real_t>(numElem8) ;
   Real_t *fz_elem = Allocate<Real_t>(numElem8) ;

   Real_t  gamma[4][8];

   gamma[0][0] = Real_t( 1.);
   gamma[0][1] = Real_t( 1.);
   gamma[0][2] = Real_t(-1.);
   gamma[0][3] = Real_t(-1.);
   gamma[0][4] = Real_t(-1.);
   gamma[0][5] = Real_t(-1.);
   gamma[0][6] = Real_t( 1.);
   gamma[0][7] = Real_t( 1.);
   gamma[1][0] = Real_t( 1.);
   gamma[1][1] = Real_t(-1.);
   gamma[1][2] = Real_t(-1.);
   gamma[1][3] = Real_t( 1.);
   gamma[1][4] = Real_t(-1.);
   gamma[1][5] = Real_t( 1.);
   gamma[1][6] = Real_t( 1.);
   gamma[1][7] = Real_t(-1.);
   gamma[2][0] = Real_t( 1.);
   gamma[2][1] = Real_t(-1.);
   gamma[2][2] = Real_t( 1.);
   gamma[2][3] = Real_t(-1.);
   gamma[2][4] = Real_t( 1.);
   gamma[2][5] = Real_t(-1.);
   gamma[2][6] = Real_t( 1.);
   gamma[2][7] = Real_t(-1.);
   gamma[3][0] = Real_t(-1.);
   gamma[3][1] = Real_t( 1.);
   gamma[3][2] = Real_t(-1.);
   gamma[3][3] = Real_t( 1.);
   gamma[3][4] = Real_t( 1.);
   gamma[3][5] = Real_t(-1.);
   gamma[3][6] = Real_t( 1.);
   gamma[3][7] = Real_t(-1.);

/*************************************************/
/*    compute the hourglass modes */


#pragma omp parallel for firstprivate(numElem, hourg)
   for(Index_t i2=0;i2<numElem;++i2){
      Real_t *fx_local, *fy_local, *fz_local ;
      Real_t hgfx[8], hgfy[8], hgfz[8] ;

      Real_t coefficient;

      Real_t hourgam0[4], hourgam1[4], hourgam2[4], hourgam3[4] ;
      Real_t hourgam4[4], hourgam5[4], hourgam6[4], hourgam7[4];
      Real_t xd1[8], yd1[8], zd1[8] ;

      const Index_t *elemToNode = &nodelist[8*i2];
      Index_t i3=8*i2;
      Real_t volinv=Real_t(1.0)/determ[i2];
      Real_t ss1, mass1, volume13 ;
      for(Index_t i1=0;i1<4;++i1){

         Real_t hourmodx =
            x8n[i3] * gamma[i1][0] + x8n[i3+1] * gamma[i1][1] +
            x8n[i3+2] * gamma[i1][2] + x8n[i3+3] * gamma[i1][3] +
            x8n[i3+4] * gamma[i1][4] + x8n[i3+5] * gamma[i1][5] +
            x8n[i3+6] * gamma[i1][6] + x8n[i3+7] * gamma[i1][7];

         Real_t hourmody =
            y8n[i3] * gamma[i1][0] + y8n[i3+1] * gamma[i1][1] +
            y8n[i3+2] * gamma[i1][2] + y8n[i3+3] * gamma[i1][3] +
            y8n[i3+4] * gamma[i1][4] + y8n[i3+5] * gamma[i1][5] +
            y8n[i3+6] * gamma[i1][6] + y8n[i3+7] * gamma[i1][7];

         Real_t hourmodz =
            z8n[i3] * gamma[i1][0] + z8n[i3+1] * gamma[i1][1] +
            z8n[i3+2] * gamma[i1][2] + z8n[i3+3] * gamma[i1][3] +
            z8n[i3+4] * gamma[i1][4] + z8n[i3+5] * gamma[i1][5] +
            z8n[i3+6] * gamma[i1][6] + z8n[i3+7] * gamma[i1][7];

         hourgam0[i1] = gamma[i1][0] -  volinv*(dvdx[i3  ] * hourmodx +
                                                  dvdy[i3  ] * hourmody +
                                                  dvdz[i3  ] * hourmodz );

         hourgam1[i1] = gamma[i1][1] -  volinv*(dvdx[i3+1] * hourmodx +
                                                  dvdy[i3+1] * hourmody +
                                                  dvdz[i3+1] * hourmodz );

         hourgam2[i1] = gamma[i1][2] -  volinv*(dvdx[i3+2] * hourmodx +
                                                  dvdy[i3+2] * hourmody +
                                                  dvdz[i3+2] * hourmodz );

         hourgam3[i1] = gamma[i1][3] -  volinv*(dvdx[i3+3] * hourmodx +
                                                  dvdy[i3+3] * hourmody +
                                                  dvdz[i3+3] * hourmodz );

         hourgam4[i1] = gamma[i1][4] -  volinv*(dvdx[i3+4] * hourmodx +
                                                  dvdy[i3+4] * hourmody +
                                                  dvdz[i3+4] * hourmodz );

         hourgam5[i1] = gamma[i1][5] -  volinv*(dvdx[i3+5] * hourmodx +
                                                  dvdy[i3+5] * hourmody +
                                                  dvdz[i3+5] * hourmodz );

         hourgam6[i1] = gamma[i1][6] -  volinv*(dvdx[i3+6] * hourmodx +
                                                  dvdy[i3+6] * hourmody +
                                                  dvdz[i3+6] * hourmodz );

         hourgam7[i1] = gamma[i1][7] -  volinv*(dvdx[i3+7] * hourmodx +
                                                  dvdy[i3+7] * hourmody +
                                                  dvdz[i3+7] * hourmodz );

      }

      /* compute forces */
      /* store forces into h arrays (force arrays) */

      ss1=ss[i2];
      mass1=elemMass[i2];
      volume13=CBRT(determ[i2]);

      Index_t n0si2 = elemToNode[0];
      Index_t n1si2 = elemToNode[1];
      Index_t n2si2 = elemToNode[2];
      Index_t n3si2 = elemToNode[3];
      Index_t n4si2 = elemToNode[4];
      Index_t n5si2 = elemToNode[5];
      Index_t n6si2 = elemToNode[6];
      Index_t n7si2 = elemToNode[7];

      xd1[0] = xd[n0si2];
      xd1[1] = xd[n1si2];
      xd1[2] = xd[n2si2];
      xd1[3] = xd[n3si2];
      xd1[4] = xd[n4si2];
      xd1[5] = xd[n5si2];
      xd1[6] = xd[n6si2];
      xd1[7] = xd[n7si2];

      yd1[0] = yd[n0si2];
      yd1[1] = yd[n1si2];
      yd1[2] = yd[n2si2];
      yd1[3] = yd[n3si2];
      yd1[4] = yd[n4si2];
      yd1[5] = yd[n5si2];
      yd1[6] = yd[n6si2];
      yd1[7] = yd[n7si2];

      zd1[0] = zd[n0si2];
      zd1[1] = zd[n1si2];
      zd1[2] = zd[n2si2];
      zd1[3] = zd[n3si2];
      zd1[4] = zd[n4si2];
      zd1[5] = zd[n5si2];
      zd1[6] = zd[n6si2];
      zd1[7] = zd[n7si2];

      coefficient = - hourg * Real_t(0.01) * ss1 * mass1 / volume13;

      CalcElemFBHourglassForce(xd1,yd1,zd1,
                      hourgam0,hourgam1,hourgam2,hourgam3,
                      hourgam4,hourgam5,hourgam6,hourgam7,
                      coefficient, hgfx, hgfy, hgfz);

      fx_local = &fx_elem[i3] ;
      fx_local[0] = hgfx[0];
      fx_local[1] = hgfx[1];
      fx_local[2] = hgfx[2];
      fx_local[3] = hgfx[3];
      fx_local[4] = hgfx[4];
      fx_local[5] = hgfx[5];
      fx_local[6] = hgfx[6];
      fx_local[7] = hgfx[7];

      fy_local = &fy_elem[i3] ;
      fy_local[0] = hgfy[0];
      fy_local[1] = hgfy[1];
      fy_local[2] = hgfy[2];
      fy_local[3] = hgfy[3];
      fy_local[4] = hgfy[4];
      fy_local[5] = hgfy[5];
      fy_local[6] = hgfy[6];
      fy_local[7] = hgfy[7];

      fz_local = &fz_elem[i3] ;
      fz_local[0] = hgfz[0];
      fz_local[1] = hgfz[1];
      fz_local[2] = hgfz[2];
      fz_local[3] = hgfz[3];
      fz_local[4] = hgfz[4];
      fz_local[5] = hgfz[5];
      fz_local[6] = hgfz[6];
      fz_local[7] = hgfz[7];
   }

  {
#pragma omp parallel for firstprivate(numNode)
     for( Index_t gnode=0 ; gnode<numNode ; ++gnode )
     {
        Index_t count = nodeElemCount[gnode] ;
        Index_t start = nodeElemStart[gnode] ;
        Real_t fx_tmp = Real_t(0.0) ;
        Real_t fy_tmp = Real_t(0.0) ;
        Real_t fz_tmp = Real_t(0.0) ;
        for (Index_t i=0 ; i < count ; ++i) {
           Index_t elem = nodeElemCornerList[start+i] ;
           fx_tmp += fx_elem[elem] ;
           fy_tmp += fy_elem[elem] ;
           fz_tmp += fz_elem[elem] ;
        }
        fx[gnode] += fx_tmp ;
        fy[gnode] += fy_tmp ;
        fz[gnode] += fz_tmp ;
     }
  }

  Release(&fz_elem) ;
  Release(&fy_elem) ;
  Release(&fx_elem) ;

}

static inline
void CalcHourglassControlForElems(Domain *domain,
                                  Real_t determ[], Real_t hgcoef)
{
   Index_t numElem = domain->numElem ;
   Index_t numElem8 = numElem * 8 ;
   Real_t *dvdx = Allocate<Real_t>(numElem8) ;
   Real_t *dvdy = Allocate<Real_t>(numElem8) ;
   Real_t *dvdz = Allocate<Real_t>(numElem8) ;
   Real_t *x8n  = Allocate<Real_t>(numElem8) ;
   Real_t *y8n  = Allocate<Real_t>(numElem8) ;
   Real_t *z8n  = Allocate<Real_t>(numElem8) ;

   /* start loop over elements */
#pragma omp parallel for firstprivate(numElem)
   for (Index_t i=0 ; i<numElem ; ++i){
      Real_t  x1[8],  y1[8],  z1[8] ;
      Real_t pfx[8], pfy[8], pfz[8] ;

      Index_t* elemToNode = &domain->nodelist[8*i];
      CollectDomainNodesToElemNodes(domain->x, domain->y, domain->z,
                                    elemToNode, x1, y1, z1);

      CalcElemVolumeDerivative(pfx, pfy, pfz, x1, y1, z1);

      /* load into temporary storage for FB Hour Glass control */
      for(Index_t ii=0;ii<8;++ii){
         Index_t jj=8*i+ii;

         dvdx[jj] = pfx[ii];
         dvdy[jj] = pfy[ii];
         dvdz[jj] = pfz[ii];
         x8n[jj]  = x1[ii];
         y8n[jj]  = y1[ii];
         z8n[jj]  = z1[ii];
      }

      determ[i] = domain->volo[i] * domain->v[i];

      /* Do a check for negative volumes */
      if ( domain->v[i] <= Real_t(0.0) ) {
         MPI_Abort(MPI_COMM_WORLD, VolumeError) ;
      }
   }

   if ( hgcoef > Real_t(0.) ) {
      CalcFBHourglassForceForElems( domain->nodelist,
                                    domain->ss, domain->elemMass,
                                    domain->xd, domain->yd, domain->zd,
                                    domain->fx, domain->fy, domain->fz,
                                    domain->nodeElemCount,
                                    domain->nodeElemStart, 
                                    domain->nodeElemCornerList,
                                    determ, x8n, y8n, z8n, dvdx, dvdy, dvdz,
                                    hgcoef, numElem, domain->numNode) ;
   }

   Release(&z8n) ;
   Release(&y8n) ;
   Release(&x8n) ;
   Release(&dvdz) ;
   Release(&dvdy) ;
   Release(&dvdx) ;

   return ;
}

static inline
void CalcVolumeForceForElems(Domain *domain)
{
   Index_t numElem = domain->numElem ;
   if (numElem != 0) {
      Real_t  hgcoef = domain->hgcoef ;
      Real_t *sigxx  = Allocate<Real_t>(numElem) ;
      Real_t *sigyy  = Allocate<Real_t>(numElem) ;
      Real_t *sigzz  = Allocate<Real_t>(numElem) ;
      Real_t *determ = Allocate<Real_t>(numElem) ;

      /* Sum contributions to total stress tensor */
      InitStressTermsForElems(domain->p, domain->q,
                              sigxx, sigyy, sigzz, numElem);

      // call elemlib stress integration loop to produce nodal forces from
      // material stresses.
      IntegrateStressForElems( domain->nodelist,
                               domain->x, domain->y, domain->z,
                               domain->fx, domain->fy, domain->fz,
                               domain->nodeElemCount,
                               domain->nodeElemStart,
                               domain->nodeElemCornerList,
                               sigxx, sigyy, sigzz, determ, numElem,
                               domain->numNode) ;

      // check for negative element volume
#pragma omp parallel for firstprivate(numElem)
      for ( Index_t k=0 ; k<numElem ; ++k ) {
         if (determ[k] <= Real_t(0.0)) {
            MPI_Abort(MPI_COMM_WORLD, VolumeError) ;
         }
      }

      CalcHourglassControlForElems(domain, determ, hgcoef) ;

      Release(&determ) ;
      Release(&sigzz) ;
      Release(&sigyy) ;
      Release(&sigxx) ;
   }
}

static inline void CalcForceForNodes(Domain *domain)
{
  Real_t *fieldData[3] ;
  Index_t numNode = domain->numNode ;

  CommRecv(domain, MSG_COMM_SBN, 3,
           domain->sizeX + 1, domain->sizeY + 1, domain->sizeZ + 1,
           true, false) ;

#pragma omp parallel for firstprivate(numNode)
  for (Index_t i=0; i<numNode; ++i) {
     domain->fx[i] = Real_t(0.0) ;
     domain->fy[i] = Real_t(0.0) ;
     domain->fz[i] = Real_t(0.0) ;
  }

  /* Calcforce calls partial, force, hourq */
  CalcVolumeForceForElems(domain) ;

  fieldData[0] = domain->fx ;
  fieldData[1] = domain->fy ;
  fieldData[2] = domain->fz ;
  CommSend(domain, MSG_COMM_SBN, 3, fieldData,
           domain->sizeX + 1, domain->sizeY + 1, domain->sizeZ +  1,
           true, false) ;
  CommSBN(domain, 3, fieldData) ;
}

static inline
void CalcAccelerationForNodes(Real_t *xdd, Real_t *ydd, Real_t *zdd,
                              Real_t *fx, Real_t *fy, Real_t *fz,
                              Real_t *nodalMass, Index_t numNode)
{
#pragma omp parallel for firstprivate(numNode)
   for (Index_t i = 0; i < numNode; ++i) {
      xdd[i] = fx[i] / nodalMass[i];
      ydd[i] = fy[i] / nodalMass[i];
      zdd[i] = fz[i] / nodalMass[i];
   }
}

static inline
void ApplyAccelerationBoundaryConditionsForNodes(Real_t *xdd, Real_t *ydd,
                                                 Real_t *zdd, Index_t *symmX,
                                                 Index_t *symmY,
                                                 Index_t *symmZ, Index_t size)
{
  Index_t numNodeBC = (size+1)*(size+1) ;

#pragma omp parallel
  {
    if (symmX != 0) {
#pragma omp for nowait firstprivate(numNodeBC)
       for(Index_t i=0 ; i<numNodeBC ; ++i)
          xdd[symmX[i]] = Real_t(0.0) ;
    }

    if (symmY != 0) {
#pragma omp for nowait firstprivate(numNodeBC)
       for(Index_t i=0 ; i<numNodeBC ; ++i)
          ydd[symmY[i]] = Real_t(0.0) ;
    }

    if (symmZ != 0) {
#pragma omp for nowait firstprivate(numNodeBC)
       for(Index_t i=0 ; i<numNodeBC ; ++i)
          zdd[symmZ[i]] = Real_t(0.0) ;
    }
  }
}

static inline
void CalcVelocityForNodes(Real_t *xd,  Real_t *yd,  Real_t *zd,
                          Real_t *xdd, Real_t *ydd, Real_t *zdd,
                          const Real_t dt, const Real_t u_cut,
                          Index_t numNode)
{

#pragma omp parallel for firstprivate(numNode)
   for ( Index_t i = 0 ; i < numNode ; ++i )
   {
     Real_t xdtmp, ydtmp, zdtmp ;

     xdtmp = xd[i] + xdd[i] * dt ;
     if( FABS(xdtmp) < u_cut ) xdtmp = Real_t(0.0);
     xd[i] = xdtmp ;

     ydtmp = yd[i] + ydd[i] * dt ;
     if( FABS(ydtmp) < u_cut ) ydtmp = Real_t(0.0);
     yd[i] = ydtmp ;

     zdtmp = zd[i] + zdd[i] * dt ;
     if( FABS(zdtmp) < u_cut ) zdtmp = Real_t(0.0);
     zd[i] = zdtmp ;
   }
}

static inline
void CalcPositionForNodes(Real_t *x,  Real_t *y,  Real_t *z,
                          Real_t *xd, Real_t *yd, Real_t *zd,
                          const Real_t dt, Index_t numNode)
{
#pragma omp parallel for firstprivate(numNode)
   for ( Index_t i = 0 ; i < numNode ; ++i )
   {
     x[i] += xd[i] * dt ;
     y[i] += yd[i] * dt ;
     z[i] += zd[i] * dt ;
   }
}

static inline
void LagrangeNodal(Domain *domain)
{
#ifdef SEDOV_SYNC_POS_VEL_EARLY
   Real_t *fieldData[6] ;
#endif

  const Real_t delt = domain->deltatime ;
  Real_t u_cut = domain->u_cut ;

  /* time of boundary condition evaluation is beginning of step for force and
   * acceleration boundary conditions. */
  CalcForceForNodes(domain);

#ifdef SEDOV_SYNC_POS_VEL_EARLY
   CommRecv(domain, MSG_SYNC_POS_VEL, 6,
            domain->sizeX + 1, domain->sizeY + 1, domain->sizeZ + 1,
            false, false) ;
#endif

  CalcAccelerationForNodes(domain->xdd, domain->ydd, domain->zdd,
                           domain->fx, domain->fy, domain->fz,
                           domain->nodalMass, domain->numNode);

  ApplyAccelerationBoundaryConditionsForNodes(domain->xdd, domain->ydd,
                                              domain->zdd, domain->symmX,
                                              domain->symmY, domain->symmZ,
                                              domain->sizeX);

  CalcVelocityForNodes( domain->xd,  domain->yd,  domain->zd,
                        domain->xdd, domain->ydd, domain->zdd,
                        delt, u_cut, domain->numNode) ;

  CalcPositionForNodes( domain->x,  domain->y,  domain->z,
                        domain->xd, domain->yd, domain->zd,
                        delt, domain->numNode );
#ifdef SEDOV_SYNC_POS_VEL_EARLY
   fieldData[0] = domain->x ;
   fieldData[1] = domain->y ;
   fieldData[2] = domain->z ;
   fieldData[3] = domain->xd ;
   fieldData[4] = domain->yd ;
   fieldData[5] = domain->zd ;
   
   CommSend(domain, MSG_SYNC_POS_VEL, 6, fieldData,
            domain->sizeX + 1, domain->sizeY + 1, domain->sizeZ + 1,
            false, false) ;
   CommSyncPosVel(domain) ;
#endif

  return;
}

static inline
Real_t CalcElemVolume( const Real_t x0, const Real_t x1,
               const Real_t x2, const Real_t x3,
               const Real_t x4, const Real_t x5,
               const Real_t x6, const Real_t x7,
               const Real_t y0, const Real_t y1,
               const Real_t y2, const Real_t y3,
               const Real_t y4, const Real_t y5,
               const Real_t y6, const Real_t y7,
               const Real_t z0, const Real_t z1,
               const Real_t z2, const Real_t z3,
               const Real_t z4, const Real_t z5,
               const Real_t z6, const Real_t z7 )
{
  Real_t twelveth = Real_t(1.0)/Real_t(12.0);

  Real_t dx61 = x6 - x1;
  Real_t dy61 = y6 - y1;
  Real_t dz61 = z6 - z1;

  Real_t dx70 = x7 - x0;
  Real_t dy70 = y7 - y0;
  Real_t dz70 = z7 - z0;

  Real_t dx63 = x6 - x3;
  Real_t dy63 = y6 - y3;
  Real_t dz63 = z6 - z3;

  Real_t dx20 = x2 - x0;
  Real_t dy20 = y2 - y0;
  Real_t dz20 = z2 - z0;

  Real_t dx50 = x5 - x0;
  Real_t dy50 = y5 - y0;
  Real_t dz50 = z5 - z0;

  Real_t dx64 = x6 - x4;
  Real_t dy64 = y6 - y4;
  Real_t dz64 = z6 - z4;

  Real_t dx31 = x3 - x1;
  Real_t dy31 = y3 - y1;
  Real_t dz31 = z3 - z1;

  Real_t dx72 = x7 - x2;
  Real_t dy72 = y7 - y2;
  Real_t dz72 = z7 - z2;

  Real_t dx43 = x4 - x3;
  Real_t dy43 = y4 - y3;
  Real_t dz43 = z4 - z3;

  Real_t dx57 = x5 - x7;
  Real_t dy57 = y5 - y7;
  Real_t dz57 = z5 - z7;

  Real_t dx14 = x1 - x4;
  Real_t dy14 = y1 - y4;
  Real_t dz14 = z1 - z4;

  Real_t dx25 = x2 - x5;
  Real_t dy25 = y2 - y5;
  Real_t dz25 = z2 - z5;

#define TRIPLE_PRODUCT(x1, y1, z1, x2, y2, z2, x3, y3, z3) \
   ((x1)*((y2)*(z3) - (z2)*(y3)) + (x2)*((z1)*(y3) - (y1)*(z3)) + (x3)*((y1)*(z2) - (z1)*(y2)))

  Real_t volume =
    TRIPLE_PRODUCT(dx31 + dx72, dx63, dx20,
       dy31 + dy72, dy63, dy20,
       dz31 + dz72, dz63, dz20) +
    TRIPLE_PRODUCT(dx43 + dx57, dx64, dx70,
       dy43 + dy57, dy64, dy70,
       dz43 + dz57, dz64, dz70) +
    TRIPLE_PRODUCT(dx14 + dx25, dx61, dx50,
       dy14 + dy25, dy61, dy50,
       dz14 + dz25, dz61, dz50);

#undef TRIPLE_PRODUCT

  volume *= twelveth;

  return volume ;
}

static inline
Real_t CalcElemVolume( const Real_t x[8], const Real_t y[8], const Real_t z[8] )
{
return CalcElemVolume( x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7],
                       y[0], y[1], y[2], y[3], y[4], y[5], y[6], y[7],
                       z[0], z[1], z[2], z[3], z[4], z[5], z[6], z[7]);
}

static inline
Real_t AreaFace( const Real_t x0, const Real_t x1,
                 const Real_t x2, const Real_t x3,
                 const Real_t y0, const Real_t y1,
                 const Real_t y2, const Real_t y3,
                 const Real_t z0, const Real_t z1,
                 const Real_t z2, const Real_t z3)
{
   Real_t fx = (x2 - x0) - (x3 - x1);
   Real_t fy = (y2 - y0) - (y3 - y1);
   Real_t fz = (z2 - z0) - (z3 - z1);
   Real_t gx = (x2 - x0) + (x3 - x1);
   Real_t gy = (y2 - y0) + (y3 - y1);
   Real_t gz = (z2 - z0) + (z3 - z1);
   Real_t area =
      (fx * fx + fy * fy + fz * fz) *
      (gx * gx + gy * gy + gz * gz) -
      (fx * gx + fy * gy + fz * gz) *
      (fx * gx + fy * gy + fz * gz);
   return area ;
}

static inline
Real_t CalcElemCharacteristicLength( const Real_t x[8],
                                     const Real_t y[8],
                                     const Real_t z[8],
                                     const Real_t volume)
{
   Real_t a, charLength = Real_t(0.0);

   a = AreaFace(x[0],x[1],x[2],x[3],
                y[0],y[1],y[2],y[3],
                z[0],z[1],z[2],z[3]) ;
   charLength = std::max(a,charLength) ;

   a = AreaFace(x[4],x[5],x[6],x[7],
                y[4],y[5],y[6],y[7],
                z[4],z[5],z[6],z[7]) ;
   charLength = std::max(a,charLength) ;

   a = AreaFace(x[0],x[1],x[5],x[4],
                y[0],y[1],y[5],y[4],
                z[0],z[1],z[5],z[4]) ;
   charLength = std::max(a,charLength) ;

   a = AreaFace(x[1],x[2],x[6],x[5],
                y[1],y[2],y[6],y[5],
                z[1],z[2],z[6],z[5]) ;
   charLength = std::max(a,charLength) ;

   a = AreaFace(x[2],x[3],x[7],x[6],
                y[2],y[3],y[7],y[6],
                z[2],z[3],z[7],z[6]) ;
   charLength = std::max(a,charLength) ;

   a = AreaFace(x[3],x[0],x[4],x[7],
                y[3],y[0],y[4],y[7],
                z[3],z[0],z[4],z[7]) ;
   charLength = std::max(a,charLength) ;

   charLength = Real_t(4.0) * volume / SQRT(charLength);

   return charLength;
}

static inline
void CalcElemVelocityGrandient( const Real_t* const xvel,
                                const Real_t* const yvel,
                                const Real_t* const zvel,
                                const Real_t b[][8],
                                const Real_t detJ,
                                Real_t* const d )
{
  const Real_t inv_detJ = Real_t(1.0) / detJ ;
  Real_t dyddx, dxddy, dzddx, dxddz, dzddy, dyddz;
  const Real_t* const pfx = b[0];
  const Real_t* const pfy = b[1];
  const Real_t* const pfz = b[2];

  d[0] = inv_detJ * ( pfx[0] * (xvel[0]-xvel[6])
                     + pfx[1] * (xvel[1]-xvel[7])
                     + pfx[2] * (xvel[2]-xvel[4])
                     + pfx[3] * (xvel[3]-xvel[5]) );

  d[1] = inv_detJ * ( pfy[0] * (yvel[0]-yvel[6])
                     + pfy[1] * (yvel[1]-yvel[7])
                     + pfy[2] * (yvel[2]-yvel[4])
                     + pfy[3] * (yvel[3]-yvel[5]) );

  d[2] = inv_detJ * ( pfz[0] * (zvel[0]-zvel[6])
                     + pfz[1] * (zvel[1]-zvel[7])
                     + pfz[2] * (zvel[2]-zvel[4])
                     + pfz[3] * (zvel[3]-zvel[5]) );

  dyddx  = inv_detJ * ( pfx[0] * (yvel[0]-yvel[6])
                      + pfx[1] * (yvel[1]-yvel[7])
                      + pfx[2] * (yvel[2]-yvel[4])
                      + pfx[3] * (yvel[3]-yvel[5]) );

  dxddy  = inv_detJ * ( pfy[0] * (xvel[0]-xvel[6])
                      + pfy[1] * (xvel[1]-xvel[7])
                      + pfy[2] * (xvel[2]-xvel[4])
                      + pfy[3] * (xvel[3]-xvel[5]) );

  dzddx  = inv_detJ * ( pfx[0] * (zvel[0]-zvel[6])
                      + pfx[1] * (zvel[1]-zvel[7])
                      + pfx[2] * (zvel[2]-zvel[4])
                      + pfx[3] * (zvel[3]-zvel[5]) );

  dxddz  = inv_detJ * ( pfz[0] * (xvel[0]-xvel[6])
                      + pfz[1] * (xvel[1]-xvel[7])
                      + pfz[2] * (xvel[2]-xvel[4])
                      + pfz[3] * (xvel[3]-xvel[5]) );

  dzddy  = inv_detJ * ( pfy[0] * (zvel[0]-zvel[6])
                      + pfy[1] * (zvel[1]-zvel[7])
                      + pfy[2] * (zvel[2]-zvel[4])
                      + pfy[3] * (zvel[3]-zvel[5]) );

  dyddz  = inv_detJ * ( pfz[0] * (yvel[0]-yvel[6])
                      + pfz[1] * (yvel[1]-yvel[7])
                      + pfz[2] * (yvel[2]-yvel[4])
                      + pfz[3] * (yvel[3]-yvel[5]) );
  d[5]  = Real_t( .5) * ( dxddy + dyddx );
  d[4]  = Real_t( .5) * ( dxddz + dzddx );
  d[3]  = Real_t( .5) * ( dzddy + dyddz );
}

static inline
void CalcKinematicsForElems( Index_t *nodelist,
                             Real_t *x,   Real_t *y,   Real_t *z,
                             Real_t *xd,  Real_t *yd,  Real_t *zd,
                             Real_t *dxx, Real_t *dyy, Real_t *dzz,
                             Real_t *v, Real_t *volo,
                             Real_t *vnew, Real_t *delv, Real_t *arealg,
                             Real_t deltaTime, Index_t numElem )
{

  // loop over all elements
#pragma omp parallel for firstprivate(numElem, deltaTime)
  for( Index_t k=0 ; k<numElem ; ++k )
  {
    Real_t B[3][8] ; /** shape function derivatives */
    Real_t D[6] ;
    Real_t x_local[8] ;
    Real_t y_local[8] ;
    Real_t z_local[8] ;
    Real_t xd_local[8] ;
    Real_t yd_local[8] ;
    Real_t zd_local[8] ;
    Real_t detJ = Real_t(0.0) ;

    Real_t volume ;
    Real_t relativeVolume ;
    const Index_t* const elemToNode = &nodelist[8*k] ;

    // get nodal coordinates from global arrays and copy into local arrays.
    for( Index_t lnode=0 ; lnode<8 ; ++lnode )
    {
      Index_t gnode = elemToNode[lnode];
      x_local[lnode] = x[gnode];
      y_local[lnode] = y[gnode];
      z_local[lnode] = z[gnode];
    }

    // volume calculations
    volume = CalcElemVolume(x_local, y_local, z_local );
    relativeVolume = volume / volo[k] ;
    vnew[k] = relativeVolume ;
    delv[k] = relativeVolume - v[k] ;

    // set characteristic length
    arealg[k] = CalcElemCharacteristicLength(x_local, y_local, z_local,
                                             volume);

    // get nodal velocities from global array and copy into local arrays.
    for( Index_t lnode=0 ; lnode<8 ; ++lnode )
    {
      Index_t gnode = elemToNode[lnode];
      xd_local[lnode] = xd[gnode];
      yd_local[lnode] = yd[gnode];
      zd_local[lnode] = zd[gnode];
    }

    Real_t dt2 = Real_t(0.5) * deltaTime;
    for ( Index_t j=0 ; j<8 ; ++j )
    {
       x_local[j] -= dt2 * xd_local[j];
       y_local[j] -= dt2 * yd_local[j];
       z_local[j] -= dt2 * zd_local[j];
    }

    CalcElemShapeFunctionDerivatives( x_local, y_local, z_local,
                                      B, &detJ );

    CalcElemVelocityGrandient( xd_local, yd_local, zd_local,
                               B, detJ, D );

    // put velocity gradient quantities into their global arrays.
    dxx[k] = D[0];
    dyy[k] = D[1];
    dzz[k] = D[2];
  }
}

static inline
void CalcLagrangeElements(Domain *domain)
{
   Index_t numElem = domain->numElem ;
   if (numElem > 0) {
      const Real_t deltatime = domain->deltatime ;

      domain->dxx  = new Real_t[numElem] ; /* principal strains */
      domain->dyy  = new Real_t[numElem] ;
      domain->dzz  = new Real_t[numElem] ;

      CalcKinematicsForElems(domain->nodelist,
                             domain->x, domain->y, domain->z,
                             domain->xd, domain->yd, domain->zd,
                             domain->dxx, domain->dyy, domain->dzz,
                             domain->v, domain->volo,
                             domain->vnew, domain->delv, domain->arealg,
                             deltatime, numElem) ;

      // element loop to do some stuff not included in the elemlib function.
#pragma omp parallel for firstprivate(numElem)
      for ( Index_t k=0 ; k<numElem ; ++k )
      {
        // calc strain rate and apply as constraint (only done in FB element)
        Real_t vdov = domain->dxx[k] + domain->dyy[k] + domain->dzz[k] ;
        Real_t vdovthird = vdov/Real_t(3.0) ;
        
        // make the rate of deformation tensor deviatoric
        domain->vdov[k] = vdov ;
        domain->dxx[k] -= vdovthird ;
        domain->dyy[k] -= vdovthird ;
        domain->dzz[k] -= vdovthird ;

        // See if any volumes are negative, and take appropriate action.
        if (domain->vnew[k] <= Real_t(0.0))
        {
           MPI_Abort(MPI_COMM_WORLD, VolumeError) ;
        }
      }

      delete [] domain->dzz ;
      delete [] domain->dyy ;
      delete [] domain->dxx ;
   }
}

static inline
void CalcMonotonicQGradientsForElems(Real_t *x,  Real_t *y,  Real_t *z,
                                     Real_t *xd, Real_t *yd, Real_t *zd,
                                     Real_t *volo, Real_t *vnew,
                                     Real_t *delv_xi,
                                     Real_t *delv_eta,
                                     Real_t *delv_zeta,
                                     Real_t *delx_xi,
                                     Real_t *delx_eta,
                                     Real_t *delx_zeta,
                                     Index_t *nodelist,
                                     Index_t numElem)
{
#define SUM4(a,b,c,d) (a + b + c + d)

#pragma omp parallel for firstprivate(numElem)
   for (Index_t i = 0 ; i < numElem ; ++i ) {
      const Real_t ptiny = Real_t(1.e-36) ;
      Real_t ax,ay,az ;
      Real_t dxv,dyv,dzv ;

      const Index_t *elemToNode = &nodelist[8*i];
      Index_t n0 = elemToNode[0] ;
      Index_t n1 = elemToNode[1] ;
      Index_t n2 = elemToNode[2] ;
      Index_t n3 = elemToNode[3] ;
      Index_t n4 = elemToNode[4] ;
      Index_t n5 = elemToNode[5] ;
      Index_t n6 = elemToNode[6] ;
      Index_t n7 = elemToNode[7] ;

      Real_t x0 = x[n0] ;
      Real_t x1 = x[n1] ;
      Real_t x2 = x[n2] ;
      Real_t x3 = x[n3] ;
      Real_t x4 = x[n4] ;
      Real_t x5 = x[n5] ;
      Real_t x6 = x[n6] ;
      Real_t x7 = x[n7] ;

      Real_t y0 = y[n0] ;
      Real_t y1 = y[n1] ;
      Real_t y2 = y[n2] ;
      Real_t y3 = y[n3] ;
      Real_t y4 = y[n4] ;
      Real_t y5 = y[n5] ;
      Real_t y6 = y[n6] ;
      Real_t y7 = y[n7] ;

      Real_t z0 = z[n0] ;
      Real_t z1 = z[n1] ;
      Real_t z2 = z[n2] ;
      Real_t z3 = z[n3] ;
      Real_t z4 = z[n4] ;
      Real_t z5 = z[n5] ;
      Real_t z6 = z[n6] ;
      Real_t z7 = z[n7] ;

      Real_t xv0 = xd[n0] ;
      Real_t xv1 = xd[n1] ;
      Real_t xv2 = xd[n2] ;
      Real_t xv3 = xd[n3] ;
      Real_t xv4 = xd[n4] ;
      Real_t xv5 = xd[n5] ;
      Real_t xv6 = xd[n6] ;
      Real_t xv7 = xd[n7] ;

      Real_t yv0 = yd[n0] ;
      Real_t yv1 = yd[n1] ;
      Real_t yv2 = yd[n2] ;
      Real_t yv3 = yd[n3] ;
      Real_t yv4 = yd[n4] ;
      Real_t yv5 = yd[n5] ;
      Real_t yv6 = yd[n6] ;
      Real_t yv7 = yd[n7] ;

      Real_t zv0 = zd[n0] ;
      Real_t zv1 = zd[n1] ;
      Real_t zv2 = zd[n2] ;
      Real_t zv3 = zd[n3] ;
      Real_t zv4 = zd[n4] ;
      Real_t zv5 = zd[n5] ;
      Real_t zv6 = zd[n6] ;
      Real_t zv7 = zd[n7] ;

      Real_t vol = volo[i]*vnew[i] ;
      Real_t norm = Real_t(1.0) / ( vol + ptiny ) ;

      Real_t dxj = Real_t(-0.25)*(SUM4(x0,x1,x5,x4) - SUM4(x3,x2,x6,x7)) ;
      Real_t dyj = Real_t(-0.25)*(SUM4(y0,y1,y5,y4) - SUM4(y3,y2,y6,y7)) ;
      Real_t dzj = Real_t(-0.25)*(SUM4(z0,z1,z5,z4) - SUM4(z3,z2,z6,z7)) ;

      Real_t dxi = Real_t( 0.25)*(SUM4(x1,x2,x6,x5) - SUM4(x0,x3,x7,x4)) ;
      Real_t dyi = Real_t( 0.25)*(SUM4(y1,y2,y6,y5) - SUM4(y0,y3,y7,y4)) ;
      Real_t dzi = Real_t( 0.25)*(SUM4(z1,z2,z6,z5) - SUM4(z0,z3,z7,z4)) ;

      Real_t dxk = Real_t( 0.25)*(SUM4(x4,x5,x6,x7) - SUM4(x0,x1,x2,x3)) ;
      Real_t dyk = Real_t( 0.25)*(SUM4(y4,y5,y6,y7) - SUM4(y0,y1,y2,y3)) ;
      Real_t dzk = Real_t( 0.25)*(SUM4(z4,z5,z6,z7) - SUM4(z0,z1,z2,z3)) ;

      /* find delvk and delxk ( i cross j ) */

      ax = dyi*dzj - dzi*dyj ;
      ay = dzi*dxj - dxi*dzj ;
      az = dxi*dyj - dyi*dxj ;

      delx_zeta[i] = vol / SQRT(ax*ax + ay*ay + az*az + ptiny) ;

      ax *= norm ;
      ay *= norm ;
      az *= norm ;

      dxv = Real_t(0.25)*(SUM4(xv4,xv5,xv6,xv7) - SUM4(xv0,xv1,xv2,xv3)) ;
      dyv = Real_t(0.25)*(SUM4(yv4,yv5,yv6,yv7) - SUM4(yv0,yv1,yv2,yv3)) ;
      dzv = Real_t(0.25)*(SUM4(zv4,zv5,zv6,zv7) - SUM4(zv0,zv1,zv2,zv3)) ;

      delv_zeta[i] = ax*dxv + ay*dyv + az*dzv ;

      /* find delxi and delvi ( j cross k ) */

      ax = dyj*dzk - dzj*dyk ;
      ay = dzj*dxk - dxj*dzk ;
      az = dxj*dyk - dyj*dxk ;

      delx_xi[i] = vol / SQRT(ax*ax + ay*ay + az*az + ptiny) ;

      ax *= norm ;
      ay *= norm ;
      az *= norm ;

      dxv = Real_t(0.25)*(SUM4(xv1,xv2,xv6,xv5) - SUM4(xv0,xv3,xv7,xv4)) ;
      dyv = Real_t(0.25)*(SUM4(yv1,yv2,yv6,yv5) - SUM4(yv0,yv3,yv7,yv4)) ;
      dzv = Real_t(0.25)*(SUM4(zv1,zv2,zv6,zv5) - SUM4(zv0,zv3,zv7,zv4)) ;

      delv_xi[i] = ax*dxv + ay*dyv + az*dzv ;

      /* find delxj and delvj ( k cross i ) */

      ax = dyk*dzi - dzk*dyi ;
      ay = dzk*dxi - dxk*dzi ;
      az = dxk*dyi - dyk*dxi ;

      delx_eta[i] = vol / SQRT(ax*ax + ay*ay + az*az + ptiny) ;

      ax *= norm ;
      ay *= norm ;
      az *= norm ;

      dxv = Real_t(-0.25)*(SUM4(xv0,xv1,xv5,xv4) - SUM4(xv3,xv2,xv6,xv7)) ;
      dyv = Real_t(-0.25)*(SUM4(yv0,yv1,yv5,yv4) - SUM4(yv3,yv2,yv6,yv7)) ;
      dzv = Real_t(-0.25)*(SUM4(zv0,zv1,zv5,zv4) - SUM4(zv3,zv2,zv6,zv7)) ;

      delv_eta[i] = ax*dxv + ay*dyv + az*dzv ;
   }
#undef SUM4
}

static inline
void CalcMonotonicQRegionForElems(
                           Index_t *matElemlist, Index_t *elemBC,
                           Index_t *lxim,   Index_t *lxip,
                           Index_t *letam,  Index_t *letap,
                           Index_t *lzetam, Index_t *lzetap,
                           Real_t *delv_xi,Real_t *delv_eta,Real_t *delv_zeta,
                           Real_t *delx_xi,Real_t *delx_eta,Real_t *delx_zeta,
                           Real_t *vdov, Real_t *volo, Real_t *vnew,
                           Real_t *elemMass, Real_t *qq, Real_t *ql,
                           Real_t qlc_monoq, Real_t qqc_monoq,
                           Real_t monoq_limiter_mult,
                           Real_t monoq_max_slope,
                           Real_t ptiny, Index_t numElem )
{
#pragma omp parallel for firstprivate(numElem, qlc_monoq, qqc_monoq, monoq_limiter_mult, monoq_max_slope, ptiny)
   for ( Index_t ielem = 0 ; ielem < numElem; ++ielem ) {
      Real_t qlin, qquad ;
      Real_t phixi, phieta, phizeta ;
      Index_t i = matElemlist[ielem];
      Int_t bcMask = elemBC[i] ;
      Real_t delvm, delvp ;

      /*  phixi     */
      Real_t norm = Real_t(1.) / ( delv_xi[i] + ptiny ) ;

      switch (bcMask & XI_M) {
         case XI_M_COMM: /* needs comm data */
         case 0:         delvm = delv_xi[lxim[i]] ; break ;
         case XI_M_SYMM: delvm = delv_xi[i] ;       break ;
         case XI_M_FREE: delvm = Real_t(0.0) ;      break ;
         default:        /* ERROR */ ;              break ;
      }
      switch (bcMask & XI_P) {
         case XI_P_COMM: /* needs comm data */
         case 0:         delvp = delv_xi[lxip[i]] ; break ;
         case XI_P_SYMM: delvp = delv_xi[i] ;       break ;
         case XI_P_FREE: delvp = Real_t(0.0) ;      break ;
         default:        /* ERROR */ ;              break ;
      }

      delvm = delvm * norm ;
      delvp = delvp * norm ;

      phixi = Real_t(.5) * ( delvm + delvp ) ;

      delvm *= monoq_limiter_mult ;
      delvp *= monoq_limiter_mult ;

      if ( delvm < phixi ) phixi = delvm ;
      if ( delvp < phixi ) phixi = delvp ;
      if ( phixi < Real_t(0.)) phixi = Real_t(0.) ;
      if ( phixi > monoq_max_slope) phixi = monoq_max_slope;


      /*  phieta     */
      norm = Real_t(1.) / ( delv_eta[i] + ptiny ) ;

      switch (bcMask & ETA_M) {
         case ETA_M_COMM: /* needs comm data */
         case 0:          delvm = delv_eta[letam[i]] ; break ;
         case ETA_M_SYMM: delvm = delv_eta[i] ;        break ;
         case ETA_M_FREE: delvm = Real_t(0.0) ;        break ;
         default:         /* ERROR */ ;                break ;
      }
      switch (bcMask & ETA_P) {
         case ETA_P_COMM: /* needs comm data */
         case 0:          delvp = delv_eta[letap[i]] ; break ;
         case ETA_P_SYMM: delvp = delv_eta[i] ;        break ;
         case ETA_P_FREE: delvp = Real_t(0.0) ;        break ;
         default:         /* ERROR */ ;                break ;
      }

      delvm = delvm * norm ;
      delvp = delvp * norm ;

      phieta = Real_t(.5) * ( delvm + delvp ) ;

      delvm *= monoq_limiter_mult ;
      delvp *= monoq_limiter_mult ;

      if ( delvm  < phieta ) phieta = delvm ;
      if ( delvp  < phieta ) phieta = delvp ;
      if ( phieta < Real_t(0.)) phieta = Real_t(0.) ;
      if ( phieta > monoq_max_slope)  phieta = monoq_max_slope;

      /*  phizeta     */
      norm = Real_t(1.) / ( delv_zeta[i] + ptiny ) ;

      switch (bcMask & ZETA_M) {
         case ZETA_M_COMM: /* needs comm data */
         case 0:           delvm = delv_zeta[lzetam[i]] ; break ;
         case ZETA_M_SYMM: delvm = delv_zeta[i] ;         break ;
         case ZETA_M_FREE: delvm = Real_t(0.0) ;          break ;
         default:          /* ERROR */ ;                  break ;
      }
      switch (bcMask & ZETA_P) {
         case ZETA_P_COMM: /* needs comm data */
         case 0:           delvp = delv_zeta[lzetap[i]] ; break ;
         case ZETA_P_SYMM: delvp = delv_zeta[i] ;         break ;
         case ZETA_P_FREE: delvp = Real_t(0.0) ;          break ;
         default:          /* ERROR */ ;                  break ;
      }

      delvm = delvm * norm ;
      delvp = delvp * norm ;

      phizeta = Real_t(.5) * ( delvm + delvp ) ;

      delvm *= monoq_limiter_mult ;
      delvp *= monoq_limiter_mult ;

      if ( delvm   < phizeta ) phizeta = delvm ;
      if ( delvp   < phizeta ) phizeta = delvp ;
      if ( phizeta < Real_t(0.)) phizeta = Real_t(0.);
      if ( phizeta > monoq_max_slope  ) phizeta = monoq_max_slope;

      /* Remove length scale */

      if ( vdov[i] > Real_t(0.) )  {
         qlin  = Real_t(0.) ;
         qquad = Real_t(0.) ;
      }
      else {
         Real_t delvxxi   = delv_xi[i]   * delx_xi[i]   ;
         Real_t delvxeta  = delv_eta[i]  * delx_eta[i]  ;
         Real_t delvxzeta = delv_zeta[i] * delx_zeta[i] ;

         if ( delvxxi   > Real_t(0.) ) delvxxi   = Real_t(0.) ;
         if ( delvxeta  > Real_t(0.) ) delvxeta  = Real_t(0.) ;
         if ( delvxzeta > Real_t(0.) ) delvxzeta = Real_t(0.) ;

         Real_t rho = elemMass[i] / (volo[i] * vnew[i]) ;

         qlin = -qlc_monoq * rho *
            (  delvxxi   * (Real_t(1.) - phixi) +
               delvxeta  * (Real_t(1.) - phieta) +
               delvxzeta * (Real_t(1.) - phizeta)  ) ;

         qquad = qqc_monoq * rho *
            (  delvxxi*delvxxi     * (Real_t(1.) - phixi*phixi) +
               delvxeta*delvxeta   * (Real_t(1.) - phieta*phieta) +
               delvxzeta*delvxzeta * (Real_t(1.) - phizeta*phizeta)  ) ;
      }

      qq[i] = qquad ;
      ql[i] = qlin  ;
   }
}

static inline
void CalcMonotonicQForElems(Domain *domain)
{  
   //
   // calculate the monotonic q for pure regions
   //
   Index_t numElem = domain->numElem ;
   if (numElem > 0) {
      //
      // initialize parameters
      // 
      const Real_t ptiny = Real_t(1.e-36) ;

      CalcMonotonicQRegionForElems(
                           domain->matElemlist, domain->elemBC,
                           domain->lxim,   domain->lxip,
                           domain->letam,  domain->letap,
                           domain->lzetam, domain->lzetap,
                           domain->delv_xi,domain->delv_eta,domain->delv_zeta,
                           domain->delx_xi,domain->delx_eta,domain->delx_zeta,
                           domain->vdov, domain->volo, domain->vnew,
                           domain->elemMass, domain->qq, domain->ql,
                           domain->qlc_monoq, domain->qqc_monoq,
                           domain->monoq_limiter_mult,
                           domain->monoq_max_slope,
                           ptiny, numElem );
   }
}

static inline
void CalcQForElems(Domain *domain)
{
   //
   // MONOTONIC Q option
   //

   Index_t numElem = domain->numElem ;

   if (numElem != 0) {
      Real_t *fieldData[3] ;
      int allElem = numElem +  /* local elem */
                    2*domain->sizeX*domain->sizeY + /* plane ghosts */
                    2*domain->sizeX*domain->sizeZ + /* row ghosts */
                    2*domain->sizeY*domain->sizeZ ; /* col ghosts */

      domain->delv_xi = new Real_t[allElem] ;   /* velocity gradient */
      domain->delv_eta = new Real_t[allElem] ;
      domain->delv_zeta = new Real_t[allElem] ;

      domain->delx_xi = new Real_t[numElem] ;   /* position gradient */
      domain->delx_eta = new Real_t[numElem] ;
      domain->delx_zeta = new Real_t[numElem] ;

      CommRecv(domain, MSG_MONOQ, 3,
               domain->sizeX, domain->sizeY, domain->sizeZ,
               true, true) ;

      /* Calculate velocity gradients */
      CalcMonotonicQGradientsForElems(domain->x,  domain->y,  domain->z,
                                      domain->xd, domain->yd, domain->zd,
                                      domain->volo, domain->vnew,
                                      domain->delv_xi,
                                      domain->delv_eta,
                                      domain->delv_zeta,
                                      domain->delx_xi,
                                      domain->delx_eta,
                                      domain->delx_zeta,
                                      domain->nodelist, domain->numElem) ;

      /* Transfer veloctiy gradients in the first order elements */
      /* problem->commElements->Transfer(CommElements::monoQ) ; */

      fieldData[0] = domain->delv_xi ;
      fieldData[1] = domain->delv_eta ;
      fieldData[2] = domain->delv_zeta ;

      CommSend(domain, MSG_MONOQ, 3, fieldData,
               domain->sizeX, domain->sizeY, domain->sizeZ,
               true, true) ;
      CommMonoQ(domain) ;

      CalcMonotonicQForElems(domain) ;

      delete [] domain->delx_zeta ;
      delete [] domain->delx_eta ;
      delete [] domain->delx_xi ;

      delete [] domain->delv_zeta ;
      delete [] domain->delv_eta ;
      delete [] domain->delv_xi ;

      /* Don't allow excessive artificial viscosity */
      Real_t qstop = domain->qstop ;
      Index_t idx = -1; 
      for (Index_t i=0; i<numElem; ++i) {
         if ( domain->q[i] > qstop ) {
            idx = i ;
            break ;
         }
      }

      if(idx >= 0) {
         MPI_Abort(MPI_COMM_WORLD, QStopError) ;
      }
   }
}

static inline
void CalcPressureForElems(Real_t* p_new, Real_t* bvc,
                          Real_t* pbvc, Real_t* e_old,
                          Real_t* compression, Real_t *vnewc,
                          Real_t pmin,
                          Real_t p_cut, Real_t eosvmax,
                          Index_t length)
{
#pragma omp parallel for firstprivate(length)
   for (Index_t i = 0; i < length ; ++i) {
      Real_t c1s = Real_t(2.0)/Real_t(3.0) ;
      bvc[i] = c1s * (compression[i] + Real_t(1.));
      pbvc[i] = c1s;
   }

#pragma omp parallel for firstprivate(length, pmin, p_cut, eosvmax)
   for (Index_t i = 0 ; i < length ; ++i){
      p_new[i] = bvc[i] * e_old[i] ;

      if    (FABS(p_new[i]) <  p_cut   )
         p_new[i] = Real_t(0.0) ;

      if    ( vnewc[i] >= eosvmax ) /* impossible condition here? */
         p_new[i] = Real_t(0.0) ;

      if    (p_new[i]       <  pmin)
         p_new[i]   = pmin ;
   }
}

static inline
void CalcEnergyForElems(Real_t* p_new, Real_t* e_new, Real_t* q_new,
                        Real_t* bvc, Real_t* pbvc,
                        Real_t* p_old, Real_t* e_old, Real_t* q_old,
                        Real_t* compression, Real_t* compHalfStep,
                        Real_t* vnewc, Real_t* work, Real_t* delvc, Real_t pmin,
                        Real_t p_cut, Real_t  e_cut, Real_t q_cut, Real_t emin,
                        Real_t* qq_old, Real_t* ql_old,
                        Real_t rho0,
                        Real_t eosvmax,
                        Index_t length)
{
   Real_t *pHalfStep = Allocate<Real_t>(length) ;

#pragma omp parallel for firstprivate(length, emin)
   for (Index_t i = 0 ; i < length ; ++i) {
      e_new[i] = e_old[i] - Real_t(0.5) * delvc[i] * (p_old[i] + q_old[i])
         + Real_t(0.5) * work[i];

      if (e_new[i]  < emin ) {
         e_new[i] = emin ;
      }
   }

   CalcPressureForElems(pHalfStep, bvc, pbvc, e_new, compHalfStep, vnewc,
                   pmin, p_cut, eosvmax, length);

#pragma omp parallel for firstprivate(length, rho0)
   for (Index_t i = 0 ; i < length ; ++i) {
      Real_t vhalf = Real_t(1.) / (Real_t(1.) + compHalfStep[i]) ;

      if ( delvc[i] > Real_t(0.) ) {
         q_new[i] /* = qq_old[i] = ql_old[i] */ = Real_t(0.) ;
      }
      else {
         Real_t ssc = ( pbvc[i] * e_new[i]
                 + vhalf * vhalf * bvc[i] * pHalfStep[i] ) / rho0 ;

         if ( ssc <= Real_t(1.111111e-36) ) {
            ssc = Real_t(.333333e-18) ;
         } else {
            ssc = SQRT(ssc) ;
         }

         q_new[i] = (ssc*ql_old[i] + qq_old[i]) ;
      }

      e_new[i] = e_new[i] + Real_t(0.5) * delvc[i]
         * (  Real_t(3.0)*(p_old[i]     + q_old[i])
              - Real_t(4.0)*(pHalfStep[i] + q_new[i])) ;
   }

#pragma omp parallel for firstprivate(length, emin, e_cut)
   for (Index_t i = 0 ; i < length ; ++i) {

      e_new[i] += Real_t(0.5) * work[i];

      if (FABS(e_new[i]) < e_cut) {
         e_new[i] = Real_t(0.)  ;
      }
      if (     e_new[i]  < emin ) {
         e_new[i] = emin ;
      }
   }

   CalcPressureForElems(p_new, bvc, pbvc, e_new, compression, vnewc,
                   pmin, p_cut, eosvmax, length);

#pragma omp parallel for firstprivate(length, rho0, emin, e_cut)
   for (Index_t i = 0 ; i < length ; ++i){
      const Real_t sixth = Real_t(1.0) / Real_t(6.0) ;
      Real_t q_tilde ;

      if (delvc[i] > Real_t(0.)) {
         q_tilde = Real_t(0.) ;
      }
      else {
         Real_t ssc = ( pbvc[i] * e_new[i]
                 + vnewc[i] * vnewc[i] * bvc[i] * p_new[i] ) / rho0 ;

         if ( ssc <= Real_t(1.111111e-36) ) {
            ssc = Real_t(.333333e-18) ;
         } else {
            ssc = SQRT(ssc) ;
         }

         q_tilde = (ssc*ql_old[i] + qq_old[i]) ;
      }

      e_new[i] = e_new[i] - (  Real_t(7.0)*(p_old[i]     + q_old[i])
                               - Real_t(8.0)*(pHalfStep[i] + q_new[i])
                               + (p_new[i] + q_tilde)) * delvc[i]*sixth ;

      if (FABS(e_new[i]) < e_cut) {
         e_new[i] = Real_t(0.)  ;
      }
      if (     e_new[i]  < emin ) {
         e_new[i] = emin ;
      }
   }

   CalcPressureForElems(p_new, bvc, pbvc, e_new, compression, vnewc,
                   pmin, p_cut, eosvmax, length);

#pragma omp parallel for firstprivate(length, rho0, q_cut)
   for (Index_t i = 0 ; i < length ; ++i){

      if ( delvc[i] <= Real_t(0.) ) {
         Real_t ssc = ( pbvc[i] * e_new[i]
                 + vnewc[i] * vnewc[i] * bvc[i] * p_new[i] ) / rho0 ;

         if ( ssc <= Real_t(1.111111e-36) ) {
            ssc = Real_t(.333333e-18) ;
         } else {
            ssc = SQRT(ssc) ;
         }

         q_new[i] = (ssc*ql_old[i] + qq_old[i]) ;

         if (FABS(q_new[i]) < q_cut) q_new[i] = Real_t(0.) ;
      }
   }

   Release(&pHalfStep) ;

   return ;
}

static inline
void CalcSoundSpeedForElems(Index_t *matElemlist, Real_t *ss,
                            Real_t *vnewc, Real_t rho0, Real_t *enewc,
                            Real_t *pnewc, Real_t *pbvc,
                            Real_t *bvc, Real_t ss4o3, Index_t nz)
{
#pragma omp parallel for firstprivate(nz, rho0, ss4o3)
   for (Index_t i = 0; i < nz ; ++i) {
      Index_t iz = matElemlist[i];
      Real_t ssTmp = (pbvc[i] * enewc[i] + vnewc[i] * vnewc[i] *
                 bvc[i] * pnewc[i]) / rho0;
      if (ssTmp <= Real_t(1.111111e-36)) {
         ssTmp = Real_t(.333333e-18);
      }
      else {
         ssTmp = SQRT(ssTmp);
      }
      ss[iz] = ssTmp ;
   }
}

static inline
void EvalEOSForElems(Domain *domain, Real_t *vnewc, Index_t numElem)
{
   Real_t  e_cut = domain->e_cut ;
   Real_t  p_cut = domain->p_cut ;
   Real_t  ss4o3 = domain->ss4o3 ;
   Real_t  q_cut = domain->q_cut ;

   Real_t eosvmax = domain->eosvmax ;
   Real_t eosvmin = domain->eosvmin ;
   Real_t pmin    = domain->pmin ;
   Real_t emin    = domain->emin ;
   Real_t rho0    = domain->refdens ;

   Real_t *e_old = Allocate<Real_t>(numElem) ;
   Real_t *delvc = Allocate<Real_t>(numElem) ;
   Real_t *p_old = Allocate<Real_t>(numElem) ;
   Real_t *q_old = Allocate<Real_t>(numElem) ;
   Real_t *compression = Allocate<Real_t>(numElem) ;
   Real_t *compHalfStep = Allocate<Real_t>(numElem) ;
   Real_t *qq_old = Allocate<Real_t>(numElem) ;
   Real_t *ql_old = Allocate<Real_t>(numElem) ;
   Real_t *work = Allocate<Real_t>(numElem) ;
   Real_t *p_new = Allocate<Real_t>(numElem) ;
   Real_t *e_new = Allocate<Real_t>(numElem) ;
   Real_t *q_new = Allocate<Real_t>(numElem) ;
   Real_t *bvc = Allocate<Real_t>(numElem) ;
   Real_t *pbvc = Allocate<Real_t>(numElem) ;

   /* compress data, minimal set */

#pragma omp parallel
   {
#pragma omp for nowait firstprivate(numElem)
      for (Index_t i=0; i<numElem; ++i) {
         Index_t zidx = domain->matElemlist[i] ;
         e_old[i] = domain->e[zidx] ;
         delvc[i] = domain->delv[zidx] ;
         p_old[i] = domain->p[zidx] ;
         q_old[i] = domain->q[zidx] ;
         qq_old[i] = domain->qq[zidx] ;
         ql_old[i] = domain->ql[zidx] ;
      }

#pragma omp for nowait firstprivate(numElem)
      for (Index_t i = 0; i < numElem ; ++i) {
         Real_t vchalf ;
         compression[i] = Real_t(1.) / vnewc[i] - Real_t(1.);
         vchalf = vnewc[i] - delvc[i] * Real_t(.5);
         compHalfStep[i] = Real_t(1.) / vchalf - Real_t(1.);
      }

      /* Check for v > eosvmax or v < eosvmin */
      if ( eosvmin != Real_t(0.) ) {
#pragma omp for nowait firstprivate(numElem, eosvmin)
         for(Index_t i=0 ; i<numElem ; ++i) {
            if (vnewc[i] <= eosvmin) { /* impossible due to calling func? */
               compHalfStep[i] = compression[i] ;
            }
         }
      }
      if ( eosvmax != Real_t(0.) ) {
#pragma omp for nowait firstprivate(numElem, eosvmax)
         for(Index_t i=0 ; i<numElem ; ++i) {
            if (vnewc[i] >= eosvmax) { /* impossible due to calling func? */
               p_old[i]        = Real_t(0.) ;
               compression[i]  = Real_t(0.) ;
               compHalfStep[i] = Real_t(0.) ;
            }
         }
      }

#pragma omp for nowait firstprivate(numElem)
      for (Index_t i = 0 ; i < numElem ; ++i) {
         work[i] = Real_t(0.) ; 
      }
   }

   CalcEnergyForElems(p_new, e_new, q_new, bvc, pbvc,
                 p_old, e_old,  q_old, compression, compHalfStep,
                 vnewc, work,  delvc, pmin,
                 p_cut, e_cut, q_cut, emin,
                 qq_old, ql_old, rho0, eosvmax, numElem);


#pragma omp parallel for firstprivate(numElem)
   for (Index_t i=0; i<numElem; ++i) {
      Index_t zidx = domain->matElemlist[i] ;
      domain->p[zidx] = p_new[i] ;
      domain->e[zidx] = e_new[i] ;
      domain->q[zidx] = q_new[i] ;
   }

   CalcSoundSpeedForElems(domain->matElemlist, domain->ss,
             vnewc, rho0, e_new, p_new,
             pbvc, bvc, ss4o3, numElem) ;

   Release(&pbvc) ;
   Release(&bvc) ;
   Release(&q_new) ;
   Release(&e_new) ;
   Release(&p_new) ;
   Release(&work) ;
   Release(&ql_old) ;
   Release(&qq_old) ;
   Release(&compHalfStep) ;
   Release(&compression) ;
   Release(&q_old) ;
   Release(&p_old) ;
   Release(&delvc) ;
   Release(&e_old) ;
}

static inline
void ApplyMaterialPropertiesForElems(Domain *domain)
{
  Index_t numElem = domain->numElem ;

  if (numElem != 0) {
    /* Expose all of the variables needed for material evaluation */
    Real_t eosvmin = domain->eosvmin ;
    Real_t eosvmax = domain->eosvmax ;
    Real_t *vnewc = Allocate<Real_t>(numElem) ;

#pragma omp parallel
    {
#pragma omp for nowait firstprivate(numElem)
       for (Index_t i=0 ; i<numElem ; ++i) {
          Index_t zn = domain->matElemlist[i] ;
          vnewc[i] = domain->vnew[zn] ;
       }

       if (eosvmin != Real_t(0.)) {
#pragma omp for nowait firstprivate(numElem)
          for(Index_t i=0 ; i<numElem ; ++i) {
             if (vnewc[i] < eosvmin)
                vnewc[i] = eosvmin ;
          }
       }

       if (eosvmax != Real_t(0.)) {
#pragma omp for nowait firstprivate(numElem)
          for(Index_t i=0 ; i<numElem ; ++i) {
             if (vnewc[i] > eosvmax)
                vnewc[i] = eosvmax ;
          }
       }

#pragma omp for nowait firstprivate(numElem)
       for (Index_t i=0; i<numElem; ++i) {
          Index_t zn = domain->matElemlist[i] ;
          Real_t vc = domain->v[zn] ;
          if (eosvmin != Real_t(0.)) {
             if (vc < eosvmin)
                vc = eosvmin ;
          }
          if (eosvmax != Real_t(0.)) {
             if (vc > eosvmax)
                vc = eosvmax ;
          }
          if (vc <= 0.) {
             MPI_Abort(MPI_COMM_WORLD, VolumeError) ;
          }
       }
    }

    EvalEOSForElems(domain, vnewc, numElem);

    Release(&vnewc) ;

  }
}

static inline
void UpdateVolumesForElems(Real_t *vnew, Real_t *v,
                           Real_t v_cut, Index_t length)
{
   if (length != 0) {
#pragma omp parallel for firstprivate(length, v_cut)
      for(Index_t i=0 ; i<length ; ++i) {
         Real_t tmpV = vnew[i] ;

         if ( FABS(tmpV - Real_t(1.0)) < v_cut )
            tmpV = Real_t(1.0) ;

         v[i] = tmpV ;
      }
   }

   return ;
}

static inline
void LagrangeElements(Domain *domain, Index_t numElem)
{
  domain->vnew = new Real_t[numElem] ;  /* new relative volume -- temporary */

  CalcLagrangeElements(domain) ;

  /* Calculate Q.  (Monotonic q option requires communication) */
  CalcQForElems(domain) ;

  ApplyMaterialPropertiesForElems(domain) ;

  UpdateVolumesForElems(domain->vnew, domain->v,
                        domain->v_cut, numElem) ;

  delete [] domain->vnew ;
}

static inline
void CalcCourantConstraintForElems(Index_t *matElemlist, Real_t *ss,
                                   Real_t *vdov, Real_t *arealg,
                                   Real_t qqc, Index_t length,
                                   Real_t *dtcourant)
{
   Index_t threads = omp_get_max_threads();

   Index_t courant_elem_per_thread[threads];
   Real_t  dtcourant_per_thread[threads];

#pragma omp parallel firstprivate(length, qqc)
   {
      Real_t   qqc2 = Real_t(64.0) * qqc * qqc ;
      Real_t   dtcourant_tmp = Real_t(1.0e+20) ;
      Index_t  courant_elem  = -1 ;
 
      Index_t thread_num = omp_get_thread_num();

#pragma omp for 
      for (Index_t i = 0 ; i < length ; ++i) {
         Index_t indx = matElemlist[i] ;
         Real_t dtf = ss[indx] * ss[indx] ;

         if ( vdov[indx] < Real_t(0.) ) {
            dtf = dtf
                + qqc2 * arealg[indx] * arealg[indx]
                * vdov[indx] * vdov[indx] ;
         }

         dtf = SQRT(dtf) ;
         dtf = arealg[indx] / dtf ;

         if (vdov[indx] != Real_t(0.)) {
            if ( dtf < dtcourant_tmp ) {
               dtcourant_tmp = dtf ;
               courant_elem  = indx ;
            }
         }
      }

      dtcourant_per_thread[thread_num]    = dtcourant_tmp ;
      courant_elem_per_thread[thread_num] = courant_elem ;
   }

   for (Index_t i = 1; i < threads; ++i) {
      if (dtcourant_per_thread[i] < dtcourant_per_thread[0] ) {
         dtcourant_per_thread[0]    = dtcourant_per_thread[i];
         courant_elem_per_thread[0] = courant_elem_per_thread[i];
      }
   }

   if (courant_elem_per_thread[0] != -1) {
      *dtcourant = dtcourant_per_thread[0] ;
   }

   return ;

}

static inline
void CalcHydroConstraintForElems(Index_t *matElemlist, Real_t *vdov,
                                 Real_t dvovmax, Index_t length,
                                 Real_t *dthydro)
{
   Index_t threads = omp_get_max_threads();

   Real_t  dthydro_per_thread[threads];
   Index_t hydro_elem_per_thread[threads];

#pragma omp parallel firstprivate(length, dvovmax)
   {
      Real_t dthydro_tmp = Real_t(1.0e+20) ;
      Index_t hydro_elem = -1 ;

      Index_t thread_num = omp_get_thread_num();

#pragma omp for
      for (Index_t i = 0 ; i < length ; ++i) {
         Index_t indx = matElemlist[i] ;

         if (vdov[indx] != Real_t(0.)) {
            Real_t dtdvov = dvovmax / (FABS(vdov[indx])+Real_t(1.e-20)) ;

            if ( dthydro_tmp > dtdvov ) {
                  dthydro_tmp = dtdvov ;
                  hydro_elem = indx ;
            }
         }
      }

      dthydro_per_thread[thread_num]    = dthydro_tmp ;
      hydro_elem_per_thread[thread_num] = hydro_elem ;
   }

   for (Index_t i = 1; i < threads; ++i) {
      if(dthydro_per_thread[i] < dthydro_per_thread[0]) {
         dthydro_per_thread[0]    = dthydro_per_thread[i];
         hydro_elem_per_thread[0] =  hydro_elem_per_thread[i];
      }
   }

   if (hydro_elem_per_thread[0] != -1) {
      *dthydro =  dthydro_per_thread[0] ;
   }

   return ;
}

static inline
void CalcTimeConstraintsForElems(Domain *domain) {
   /* evaluate time constraint */
   CalcCourantConstraintForElems(domain->matElemlist, domain->ss,
                                 domain->vdov, domain->arealg,
                                 domain->qqc, domain->numElem,
                                 &domain->dtcourant) ;

   /* check hydro constraint */
   CalcHydroConstraintForElems(domain->matElemlist, domain->vdov,
                               domain->dvovmax, domain->numElem,
                               &domain->dthydro) ;
}

static inline
void LagrangeLeapFrog(Domain *domain)
{
#ifdef SEDOV_SYNC_POS_VEL_LATE
   Real_t *fieldData[6] ;
#endif

   /* calculate nodal forces, accelerations, velocities, positions, with
    * applied boundary conditions and slide surface considerations */
   LagrangeNodal(domain);

#ifdef SEDOV_SYNC_POS_VEL_LATE
#endif

   /* calculate element quantities (i.e. velocity gradient & q), and update
    * material states */
   LagrangeElements(domain, domain->numElem);

#ifdef SEDOV_SYNC_POS_VEL_LATE
   CommRecv(domain, MSG_SYNC_POS_VEL, 6,
            domain->sizeX + 1, domain->sizeY + 1, domain->sizeZ + 1,
            false, false) ;

   fieldData[0] = domain->x ;
   fieldData[1] = domain->y ;
   fieldData[2] = domain->z ;
   fieldData[3] = domain->xd ;
   fieldData[4] = domain->yd ;
   fieldData[5] = domain->zd ;
   
   CommSend(domain, MSG_SYNC_POS_VEL, 6, fieldData,
            domain->sizeX + 1, domain->sizeY + 1, domain->sizeZ + 1,
            false, false) ;
#endif

   CalcTimeConstraintsForElems(domain);

#ifdef SEDOV_SYNC_POS_VEL_LATE
   CommSyncPosVel(domain) ;
#endif

   // LagrangeRelease() ;  Creation/destruction of temps may be important to capture 
}

Domain *NewDomain(Index_t colLoc, Index_t rowLoc, Index_t planeLoc, Index_t nx, int tp)
{
   Real_t tx, ty, tz ;
   Index_t nidx, zidx, pidx ;
   Index_t ghostIdx[6] ;  /* offsets to ghost locations */
   struct Domain *domain = new Domain ;

   Index_t edgeElems = nx ;
   Index_t edgeNodes = edgeElems+1 ;

   Index_t meshEdgeElems = tp*nx ;

   /****************************/
   /*   Initialize Sedov Mesh  */
   /****************************/

   /* construct a uniform box for this processor */

   domain->colLoc   =   colLoc ;
   domain->rowLoc   =   rowLoc ;
   domain->planeLoc = planeLoc ;
   domain->tp = tp ;
   
   domain->sizeX = edgeElems ;
   domain->sizeY = edgeElems ;
   domain->sizeZ = edgeElems ;
   domain->numElem = edgeElems*edgeElems*edgeElems ;

   domain->numNode = edgeNodes*edgeNodes*edgeNodes ;

   Index_t domElems = domain->numElem ;
   Index_t domNodes = domain->numNode ;

   /* get run options to measure various metrics */

   /* ... */

   /* allocate field memory */

   
   /* Elem-centered */

   domain->matElemlist = new Index_t[domElems] ;  /* material indexset */
   domain->nodelist = new Index_t[8*domElems] ;   /* elemToNode connectivity */

   domain->lxim = new Index_t[domElems] ; /* elem connectivity through face */
   domain->lxip = new Index_t[domElems] ;
   domain->letam = new Index_t[domElems] ;
   domain->letap = new Index_t[domElems] ;
   domain->lzetam = new Index_t[domElems] ;
   domain->lzetap = new Index_t[domElems] ;

   domain->elemBC = new Int_t[domElems] ;  /* elem face symm/free-surface flag */

   domain->e = new Real_t[domElems] ;   /* energy */
   domain->p = new Real_t[domElems] ;   /* pressure */

   domain->q = new Real_t[domElems] ;   /* q */
   domain->ql = new Real_t[domElems] ;  /* linear term for q */
   domain->qq = new Real_t[domElems] ;  /* quadratic term for q */

   domain->v = new Real_t[domElems] ;     /* relative volume */

   domain->volo = new Real_t[domElems] ;  /* reference volume */
   domain->delv = new Real_t[domElems] ;  /* m_vnew - m_v */
   domain->vdov = new Real_t[domElems] ;  /* volume derivative over volume */

   domain->arealg = new Real_t[domElems] ;  /* elem characteristic length */

   domain->ss = new Real_t[domElems] ;      /* "sound speed" */

   domain->elemMass = new Real_t[domElems] ;  /* mass */

   /* Node-centered */

   domain->x = new Real_t[domNodes] ;  /* coordinates */
   domain->y = new Real_t[domNodes] ;
   domain->z = new Real_t[domNodes] ;

   domain->xd = new Real_t[domNodes] ; /* velocities */
   domain->yd = new Real_t[domNodes] ;
   domain->zd = new Real_t[domNodes] ;

   domain->xdd = new Real_t[domNodes] ; /* accelerations */
   domain->ydd = new Real_t[domNodes] ;
   domain->zdd = new Real_t[domNodes] ;

   domain->fx = new Real_t[domNodes] ;  /* forces */
   domain->fy = new Real_t[domNodes] ;
   domain->fz = new Real_t[domNodes] ;

   domain->nodalMass = new Real_t[domNodes] ;  /* mass */

   /* allocate a buffer large enough for nodal ghost data */
   Index_t rowMin, rowMax, colMin, colMax, planeMin, planeMax ;
   Index_t maxEdgeSize = MAX(domain->sizeX, MAX(domain->sizeY, domain->sizeZ))+1 ;
   domain->maxPlaneSize = CACHE_ALIGN_REAL(maxEdgeSize*maxEdgeSize) ;
   domain->maxEdgeSize = CACHE_ALIGN_REAL(maxEdgeSize) ;

   /* assume communication to 6 neighbors by default */
   rowMin = rowMax = colMin = colMax = planeMin = planeMax = 1 ;
   if (rowLoc == 0) {
      rowMin = 0 ;
   }
   if (rowLoc == tp-1) {
      rowMax = 0 ;
   }
   if (colLoc == 0) {
      colMin = 0 ;
   }
   if (colLoc == tp-1) {
      colMax = 0 ;
   }
   if (planeLoc == 0) {
      planeMin = 0 ;
   }
   if (planeLoc == tp-1) {
      planeMax = 0 ;
   }
   /* account for face communication */
   Index_t comBufSize =
      (rowMin + rowMax + colMin + colMax + planeMin + planeMax) *
       domain->maxPlaneSize * MAX_FIELDS_PER_MPI_COMM ;

   /* account for edge communication */
   comBufSize +=
      ((rowMin & colMin) + (rowMin & planeMin) + (colMin & planeMin) +
       (rowMax & colMax) + (rowMax & planeMax) + (colMax & planeMax) +
       (rowMax & colMin) + (rowMin & planeMax) + (colMin & planeMax) +
       (rowMin & colMax) + (rowMax & planeMin) + (colMax & planeMin)) *
       domain->maxEdgeSize * MAX_FIELDS_PER_MPI_COMM ;

   /* account for corner communication */
   /* factor of 16 is so each buffer has its own cache line */
   comBufSize += ((rowMin & colMin & planeMin) +
                  (rowMin & colMin & planeMax) +
                  (rowMin & colMax & planeMin) +
                  (rowMin & colMax & planeMax) +
                  (rowMax & colMin & planeMin) +
                  (rowMax & colMin & planeMax) +
                  (rowMax & colMax & planeMin) +
                  (rowMax & colMax & planeMax)) * CACHE_COHERENCE_PAD_REAL ;

   domain->commDataSend = new Real_t[comBufSize] ;
   domain->commDataRecv = new Real_t[comBufSize] ;
   /* prevent floating point exceptions */
   memset(domain->commDataSend, 0, comBufSize*sizeof(Real_t)) ;
   memset(domain->commDataRecv, 0, comBufSize*sizeof(Real_t)) ;

   /* Boundary nodesets */

   domain->symmX = ((colLoc == 0) ? (new Index_t[edgeNodes*edgeNodes]) : 0) ;
   domain->symmY = ((rowLoc == 0) ? (new Index_t[edgeNodes*edgeNodes]) : 0) ;
   domain->symmZ = ((planeLoc == 0) ? (new Index_t[edgeNodes*edgeNodes]) : 0) ;

   /* Basic Field Initialization */

   for (Index_t i=0; i<domElems; ++i) {
      domain->e[i] = Real_t(0.0) ;
      domain->p[i] = Real_t(0.0) ;
      domain->q[i] = Real_t(0.0) ;
      domain->v[i] = Real_t(1.0) ;
   }

   for (Index_t i=0; i<domNodes; ++i) {
      domain->xd[i] = Real_t(0.0) ;
      domain->yd[i] = Real_t(0.0) ;
      domain->zd[i] = Real_t(0.0) ;
   }

   for (Index_t i=0; i<domNodes; ++i) {
      domain->xdd[i] = Real_t(0.0) ;
      domain->ydd[i] = Real_t(0.0) ;
      domain->zdd[i] = Real_t(0.0) ;
   }

   for (Index_t i=0; i<domNodes; ++i) {
      domain->nodalMass[i] = 0.0 ;
   }
   /* initialize nodal coordinates */

   nidx = 0 ;
   tz = Real_t(1.125)*Real_t(planeLoc*nx)/Real_t(meshEdgeElems) ;
   for (Index_t plane=0; plane<edgeNodes; ++plane) {
      ty = Real_t(1.125)*Real_t(rowLoc*nx)/Real_t(meshEdgeElems) ;
      for (Index_t row=0; row<edgeNodes; ++row) {
         tx = Real_t(1.125)*Real_t(colLoc*nx)/Real_t(meshEdgeElems) ;
         for (Index_t col=0; col<edgeNodes; ++col) {
            domain->x[nidx] = tx ;
            domain->y[nidx] = ty ;
            domain->z[nidx] = tz ;
            ++nidx ;
            // tx += ds ; /* may accumulate roundoff... */
            tx = Real_t(1.125)*Real_t(colLoc*nx+col+1)/Real_t(meshEdgeElems) ;
         }
         // ty += ds ;  /* may accumulate roundoff... */
         ty = Real_t(1.125)*Real_t(rowLoc*nx+row+1)/Real_t(meshEdgeElems) ;
      }
      // tz += ds ;  /* may accumulate roundoff... */
      tz = Real_t(1.125)*Real_t(planeLoc*nx+plane+1)/Real_t(meshEdgeElems) ;
   }


   /* embed hexehedral elements in nodal point lattice */

   nidx = 0 ;
   zidx = 0 ;
   for (Index_t plane=0; plane<edgeElems; ++plane) {
      for (Index_t row=0; row<edgeElems; ++row) {
         for (Index_t col=0; col<edgeElems; ++col) {
            Index_t *localNode = &domain->nodelist[8*zidx] ;
            localNode[0] = nidx                                       ;
            localNode[1] = nidx                                   + 1 ;
            localNode[2] = nidx                       + edgeNodes + 1 ;
            localNode[3] = nidx                       + edgeNodes     ;
            localNode[4] = nidx + edgeNodes*edgeNodes                 ;
            localNode[5] = nidx + edgeNodes*edgeNodes             + 1 ;
            localNode[6] = nidx + edgeNodes*edgeNodes + edgeNodes + 1 ;
            localNode[7] = nidx + edgeNodes*edgeNodes + edgeNodes     ;
            ++zidx ;
            ++nidx ;
         }
         ++nidx ;
      }
      nidx += edgeNodes ;
   }

   {
       // Index_t m;

       /* set up node-centered indexing of elements */
       domain->nodeElemCount = new Index_t[domNodes] ;

       for (Index_t i=0; i<domNodes; ++i) {
          domain->nodeElemCount[i] = 0 ;
       }

       for (Index_t i=0; i<domElems; ++i) {
          Index_t *nl = &domain->nodelist[8*i] ;
          for (Index_t j=0; j < 8; ++j) {
             ++(domain->nodeElemCount[nl[j]] );
          }
       }

       domain->nodeElemStart = new Index_t[domNodes] ;

       domain->nodeElemStart[0] = 0;

       for (Index_t i=1; i < domNodes; ++i) {
          domain->nodeElemStart[i] =
             domain->nodeElemStart[i-1] + domain->nodeElemCount[i-1] ;
       }
       
       domain->nodeElemCornerList =
          new Index_t[domain->nodeElemStart[domNodes-1] +
                      domain->nodeElemCount[domNodes-1] ];

       for (Index_t i=0; i < domNodes; ++i) {
          domain->nodeElemCount[i] = 0;
       }

       for (Index_t i=0; i < domElems; ++i) {
          Index_t *nl = &domain->nodelist[i*8] ;
          for (Index_t j=0; j < 8; ++j) {
             Index_t m = nl[j];
             Index_t k = i*8 + j ;
             Index_t offset = domain->nodeElemStart[m] +
                              domain->nodeElemCount[m] ;
             domain->nodeElemCornerList[offset] = k;
             ++(domain->nodeElemCount[m]) ;
          }
       }

       Index_t clSize = domain->nodeElemStart[domNodes-1] +
                        domain->nodeElemCount[domNodes-1] ;
       for (Index_t i=0; i < clSize; ++i) {
          Index_t clv = domain->nodeElemCornerList[i] ;
          if ((clv < 0) || (clv > domElems*8)) {
               fprintf(stderr,
        "AllocateNodeElemIndexes(): nodeElemCornerList entry out of range!\n");
               exit(1);
          }
      }
   }


   /* Create a material IndexSet (entire domain same material for now) */
   for (Index_t i=0; i<domElems; ++i) {
      domain->matElemlist[i] = i ;
   }
   
   /* initialize material parameters */
   // domain->dtfixed = Real_t(1.0e-7) ;
   domain->dtfixed = Real_t(-1.0e-7) ;
   domain->deltatime = Real_t(1.0e-7) ;
   domain->deltatimemultlb = Real_t(1.1) ;
   domain->deltatimemultub = Real_t(1.2) ;
   // domain->stoptime  = Real_t(1.0e-5) ;
   domain->stoptime  = Real_t(1.0e-2)*Real_t(edgeElems*tp/45.0) ;
   domain->dtcourant = Real_t(1.0e+20) ;
   domain->dthydro   = Real_t(1.0e+20) ;
   domain->dtmax     = Real_t(1.0e-2) ;
   domain->time    = Real_t(0.) ;
   domain->cycle   = 0 ;

   domain->e_cut = Real_t(1.0e-7) ;
   domain->p_cut = Real_t(1.0e-7) ;
   domain->q_cut = Real_t(1.0e-7) ;
   domain->u_cut = Real_t(1.0e-7) ;
   domain->v_cut = Real_t(1.0e-10) ;

   domain->hgcoef      = Real_t(3.0) ;
   domain->ss4o3       = Real_t(4.0)/Real_t(3.0) ;

   domain->qstop              =  Real_t(1.0e+12) ;
   domain->monoq_max_slope    =  Real_t(1.0) ;
   domain->monoq_limiter_mult =  Real_t(2.0) ;
   domain->qlc_monoq          = Real_t(0.5) ;
   domain->qqc_monoq          = Real_t(2.0)/Real_t(3.0) ;
   domain->qqc                = Real_t(2.0) ;

   domain->pmin =  Real_t(0.) ;
   domain->emin = Real_t(-1.0e+15) ;

   domain->dvovmax =  Real_t(0.1) ;

   domain->eosvmax =  Real_t(1.0e+9) ;
   domain->eosvmin =  Real_t(1.0e-9) ;

   domain->refdens =  Real_t(1.0) ;

   /* initialize field data */

   for (Index_t i=0; i<domElems; ++i) {
      Real_t x_local[8], y_local[8], z_local[8] ;
      Index_t *elemToNode = &domain->nodelist[8*i] ;
      for( Index_t lnode=0 ; lnode<8 ; ++lnode )
      {
        Index_t gnode = elemToNode[lnode];
        x_local[lnode] = domain->x[gnode];
        y_local[lnode] = domain->y[gnode];
        z_local[lnode] = domain->z[gnode];
      }

      // volume calculations
      Real_t volume = CalcElemVolume(x_local, y_local, z_local );
      domain->volo[i] = volume ;
      domain->elemMass[i] = volume ;
      for (Index_t j=0; j<8; ++j) {
         Index_t idx = elemToNode[j] ;
         domain->nodalMass[idx] += volume / Real_t(8.0) ;
      }
   }

   /* deposit energy */
   if (rowLoc + colLoc + planeLoc == 0) {
      domain->e[0] = Real_t(3.948746e+7) ;
   }

   /* set up symmetry nodesets */
   nidx = 0 ;
   for (Index_t i=0; i<edgeNodes; ++i) {
      Index_t planeInc = i*edgeNodes*edgeNodes ;
      Index_t rowInc   = i*edgeNodes ;
      for (Index_t j=0; j<edgeNodes; ++j) {
         if (planeLoc == 0) {
            domain->symmZ[nidx] = rowInc   + j ;
         }
         if (rowLoc == 0) {
            domain->symmY[nidx] = planeInc + j ;
         }
         if (colLoc == 0) {
            domain->symmX[nidx] = planeInc + j*edgeNodes ;
         }
         ++nidx ;
      }
   }

   /* set up elemement connectivity information */
   domain->lxim[0] = 0 ;
   for (Index_t i=1; i<domElems; ++i) {
      domain->lxim[i]   = i-1 ;
      domain->lxip[i-1] = i ;
   }
   domain->lxip[domElems-1] = domElems-1 ;

   for (Index_t i=0; i<edgeElems; ++i) {
      domain->letam[i] = i ; 
      domain->letap[domElems-edgeElems+i] = domElems-edgeElems+i ;
   }
   for (Index_t i=edgeElems; i<domElems; ++i) {
      domain->letam[i] = i-edgeElems ;
      domain->letap[i-edgeElems] = i ;
   }

   for (Index_t i=0; i<edgeElems*edgeElems; ++i) {
      domain->lzetam[i] = i ;
      domain->lzetap[domElems-edgeElems*edgeElems+i] = domElems-edgeElems*edgeElems+i ;
   }
   for (Index_t i=edgeElems*edgeElems; i<domElems; ++i) {
      domain->lzetam[i] = i - edgeElems*edgeElems ;
      domain->lzetap[i-edgeElems*edgeElems] = i ;
   }

   /* set up boundary condition information */
   for (Index_t i=0; i<domElems; ++i) {
      domain->elemBC[i] = 0 ;  /* clear BCs by default */
   }

   for (Index_t i=0; i<6; ++i) {
      ghostIdx[i] = INT_MIN ;
   }

   pidx = domElems ;
   if (planeMin != 0) {
      ghostIdx[0] = pidx ;
      pidx += domain->sizeX*domain->sizeY ;
   }

   if (planeMax != 0) {
      ghostIdx[1] = pidx ;
      pidx += domain->sizeX*domain->sizeY ;
   }

   if (rowMin != 0) {
      ghostIdx[2] = pidx ;
      pidx += domain->sizeX*domain->sizeZ ;
   }

   if (rowMax != 0) {
      ghostIdx[3] = pidx ;
      pidx += domain->sizeX*domain->sizeZ ;
   }

   if (colMin != 0) {
      ghostIdx[4] = pidx ;
      pidx += domain->sizeY*domain->sizeZ ;
   }

   if (colMax != 0) {
      ghostIdx[5] = pidx ;
   }

   /* symmetry plane or free surface BCs */
   for (Index_t i=0; i<edgeElems; ++i) {
      Index_t planeInc = i*edgeElems*edgeElems ;
      Index_t rowInc   = i*edgeElems ;
      for (Index_t j=0; j<edgeElems; ++j) {
         if (planeLoc == 0) {
            domain->elemBC[rowInc+j] |= ZETA_M_SYMM ;
         }
         else {
            domain->elemBC[rowInc+j] |= ZETA_M_COMM ;
            domain->lzetam[rowInc+j] = ghostIdx[0] + rowInc + j ;
         }

         if (planeLoc == tp-1) {
            domain->elemBC[rowInc+j+domElems-edgeElems*edgeElems] |=
               ZETA_P_FREE;
         }
         else {
            domain->elemBC[rowInc+j+domElems-edgeElems*edgeElems] |=
               ZETA_P_COMM ;
            domain->lzetap[rowInc+j+domElems-edgeElems*edgeElems] =
               ghostIdx[1] + rowInc + j ;
         }

         if (rowLoc == 0) {
            domain->elemBC[planeInc+j] |= ETA_M_SYMM ;
         }
         else {
            domain->elemBC[planeInc+j] |= ETA_M_COMM ;
            domain->letam[planeInc+j] = ghostIdx[2] + rowInc + j ;
         }

         if (rowLoc == tp-1) {
            domain->elemBC[planeInc+j+edgeElems*edgeElems-edgeElems] |= 
               ETA_P_FREE ;
         }
         else {
            domain->elemBC[planeInc+j+edgeElems*edgeElems-edgeElems] |= 
               ETA_P_COMM ;
            domain->letap[planeInc+j+edgeElems*edgeElems-edgeElems] =
               ghostIdx[3] +  rowInc + j ;
         }

         if (colLoc == 0) {
            domain->elemBC[planeInc+j*edgeElems] |= XI_M_SYMM ;
         }
         else {
            domain->elemBC[planeInc+j*edgeElems] |= XI_M_COMM ;
            domain->lxim[planeInc+j*edgeElems] = ghostIdx[4] + rowInc + j ;
         }

         if (colLoc == tp-1) {
            domain->elemBC[planeInc+j*edgeElems+edgeElems-1] |= XI_P_FREE ;
         }
         else {
            domain->elemBC[planeInc+j*edgeElems+edgeElems-1] |= XI_P_COMM ;
            domain->lxip[planeInc+j*edgeElems+edgeElems-1] =
               ghostIdx[5] + rowInc + j ;
         }
      }
   }

   return domain ;
}


/* serial vs. parallel */
/* fixeddt vs courant (reduction vs no reduction) */
/* slab vs 3d partitioning and processor mapping */
/* X mmap based fault tolerance */

/* ------------------------------------------------- */

/* structured vs unstructured */
/* CPU vs GPU */
/* domain overloading */
/* MPI, OpenMP, MINT (OMP -> CUDA), thread, Cilk */
/*  TBB, CUDA, OpenCL, ESSL, work queue? */
/* hierarchical memory allocation */
/* two-phase memory allocation */
/* private/cached/uncached memory operations */
/* X general fault tolerance capability */
/* pooled temporary allocation */
/* loop fission/fusion  (control equations per loop) */
/* recompute (GPU) vs masked computation (or indexset based) */
/* mapping to network topology (best/worst case mapping) */
/* re-use temporaries */
/* ordered vs. unordered messages (reproducibility) */

/*
   -pm parallel model
                            local parallelism > #cores?
      outer   inner               no                yes
   -------------------------------------------------------------------
       MPI     MPI         domain-per-core    domain-overload-per-core
       MPI     OpenMP
       MPI     Pthread

*/

/* -dx -dy -dz  subdomains along each dimension */
/* -tx -ty -tz  threads in each subdomain along each dimension */
/* -px -py -pz  size of each thread patch along each dimension */

int main(int argc, char *argv[])
{
   Domain *locDom ;
   int myDom ;
   int numProcs ;
   int myRank ;
   int testProcs ;
   MPI_Init(&argc, &argv) ;
   MPI_Comm_size(MPI_COMM_WORLD, &numProcs) ;
   MPI_Comm_rank(MPI_COMM_WORLD, &myRank) ;

   /* Assume cube processor layout for now */
   testProcs = (int) (cbrt(double(numProcs))+0.5) ;
   if (testProcs*testProcs*testProcs != numProcs) {
      printf("num processors must be a cube of an integer (1, 8, 27, ...)\n") ;
      MPI_Abort(MPI_COMM_WORLD, -1) ;
   }
   if (sizeof(Real_t) != 4 && sizeof(Real_t) != 8) {
      printf("MPI operations only support float and double right now...\n");
      MPI_Abort(MPI_COMM_WORLD, -1) ;
   }
   if (MAX_FIELDS_PER_MPI_COMM > CACHE_COHERENCE_PAD_REAL) {
      printf("corner element comm buffers too small.  Fix code.\n") ;
      MPI_Abort(MPI_COMM_WORLD, -1) ;
   }

   Index_t dx = testProcs ;
   Index_t dy = testProcs ;
   Index_t dz = testProcs ;

   Index_t px = 1 ;
   Index_t py = 1 ;
   Index_t pz = 1 ;

   Index_t tx = 1 ;
   Index_t ty = 1 ;
   Index_t tz = 1 ;

   /* assume cube subdomain geometry for now */
   Index_t nx = 45 ;
#if 0
   Index_t ny = 45 ;
   Index_t nz = 45 ;
#endif

#if 0
   int numConcurrencyLevels ;

   struct ConcurrencyInfo {
      /* example hier:  10 nodes, 4 procs, 4 MPI tasks, 4 cores, 4 threads */
      int minLC ;
      int actualLC ;
      int maxLC ;
      /* mapping TBD (i.e. tot mem, node mem, task mem, core mem, thread mem) */
      int minMemory ;
      int actualMemory ;
      int maxMemory ;
   } *cinfo ;

   GetConcurrencyInfo(&numConcurrencyLevels,
                      &cinfo) ;
#endif

   /* temporary test */
   if (dx*dy*dz != numProcs) {
      printf("error -- must have as many domains as procs\n") ;
      MPI_Abort(MPI_COMM_WORLD, -1) ;
   }
   int remainder = dx*dy*dz % numProcs ;
   if (myRank < remainder) {
      myDom = myRank*( 1+ (dx*dy*dz / numProcs)) ;
   }
   else {
      myDom = remainder*( 1+ (dx*dy*dz / numProcs)) +
              (myRank - remainder)*(dx*dy*dz/numProcs) ;
   }

   int col = myDom % dx ;
   int row = (myDom / dx) % dy ;
   int plane = myDom / (dx*dy) ;

#if 0
   /* thread chunk coords */
   int colOffset = col*tx*nx ;
   int rowOffset = row*ty*ny ;
   int planeOffset = plane*tz*nz ;

   int elemOffset = colOffset +
                    rowOffset*dx*tx*nx +
                    planeOffset*dx*tx*nx*dy*ty*ny ;
   locDom = NewDomain(elemOffset,
                         tx*nx, ty*ny, tz*nz,
                         1, dx*tx*nx, dx*tx*nx*dy*ty*ny) ;
#endif
   locDom = NewDomain(col, row, plane, nx, testProcs) ;
   locDom->numProcs = numProcs ;

   CommRecv(locDom, MSG_COMM_SBN, 1,
            locDom->sizeX + 1, locDom->sizeY + 1, locDom->sizeZ + 1,
            true, false) ;
   CommSend(locDom, MSG_COMM_SBN, 1, &locDom->nodalMass,
            locDom->sizeX + 1, locDom->sizeY + 1, locDom->sizeZ +  1,
            true, false) ;
   CommSBN(locDom, 1, &locDom->nodalMass) ;

   /* timestep to solution */
   while(locDom->time < locDom->stoptime) {

      TimeIncrement(locDom) ;
      LagrangeLeapFrog(locDom) ;
#if LULESH_SHOW_PROGRESS
      if (myRank == 0) {
         printf("time = %e, dt=%e\n",
                double(locDom->time), double(locDom->deltatime) ) ;
      }
#endif
   }

  MPI_Finalize() ;

  return 0 ;
}

