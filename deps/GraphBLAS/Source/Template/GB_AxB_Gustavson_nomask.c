//------------------------------------------------------------------------------
// GB_AxB_Gustavson_nomask:  C=A*B using Gustavson method, precomputed pattern
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2018, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

// This file is #include'd in GB_AxB_Gustavson.c, and Template/GB_AxB.c, the
// latter of which expands into Generated/GB_AxB__* for all built-in semirings.

// The pattern of C has already been computed in the symbolic phase of
// GB_AxB_Gustavson.  This is Gustavson's method, extended to handle
// hypersparse matrices and arbitrary semirings.

{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    ASSERT (NOT_ALIASED_3 (C, M, A, B)) ;
    ASSERT (C->vdim == B->vdim) ;
    ASSERT (C->vlen == A->vlen) ;
    ASSERT (A->vdim == B->vlen) ;

    //--------------------------------------------------------------------------
    // get A and B
    //--------------------------------------------------------------------------

    const int64_t *restrict Ap = A->p ;
    const int64_t *restrict Ai = A->i ;
    const int64_t *restrict Bi = B->i ;

    #ifdef HYPER
    const int64_t *restrict Ah = A->h ;
    int64_t anvec = A->nvec ;
    #endif

    //--------------------------------------------------------------------------
    // get C (pattern already constructed)
    //--------------------------------------------------------------------------

    const int64_t *restrict Ci = C->i ;
    ASSERT (C->nvec <= B->nvec) ;

    // C->p and C->h have already been computed in the symbolic phase
    ASSERT (C->magic = MAGIC) ;

    //--------------------------------------------------------------------------
    // C=A*B using the Gustavson's saxpy-based method; precomputed pattern of C
    //--------------------------------------------------------------------------

    #ifdef HYPER
    for_each_vector2 (B, C)
    #else
    const int64_t *restrict Bp = B->p ;
    const int64_t *restrict Cp = C->p ;
    int64_t n = C->vdim ;
    for (int64_t j = 0 ; j < n ; j++)
    #endif
    {

        //----------------------------------------------------------------------
        // get C(:,j) and skip if empty
        //----------------------------------------------------------------------

        #ifdef HYPER
        int64_t GBI2_initj (Iter, j, pB_start, pB_end, pC_start, pC_end) ;
        #else
        int64_t pB_start = Bp [j] ;
        int64_t pB_end   = Bp [j+1] ;
        int64_t pC_start = Cp [j] ;
        int64_t pC_end   = Cp [j+1] ;
        #endif

        if (pC_end == pC_start) continue ;
        int64_t bjnz = pB_end - pB_start ;
        ASSERT (bjnz > 0) ;

        //----------------------------------------------------------------------
        // clear w
        //----------------------------------------------------------------------

        for (int64_t pC = pC_start ; pC < pC_end ; pC++)
        { 
            // w [Ci [pC]] = identity ;
            COPY_SCALAR_TO_ARRAY (w, Ci [pC], IDENTITY, zsize) ;
        }

        #ifdef HYPER
        // trim Ah on right
        int64_t pleft = 0 ;
        int64_t pright = anvec-1 ;
        if (A_is_hyper && bjnz > 2)
        { 
            // trim Ah [0..pright] to remove any entries past the last B(:,j)
            GB_bracket_right (Bi [pB_end-1], Ah, 0, &pright) ;
        }
        #endif

        //----------------------------------------------------------------------
        // C(:,j) = A * B(:,j)
        //----------------------------------------------------------------------

        for (int64_t pB = pB_start ; pB < pB_end ; pB++)
        {

            //------------------------------------------------------------------
            // get B(k,j) and A(:,k)
            //------------------------------------------------------------------

            // get the pattern of B(k,j)
            int64_t k = Bi [pB] ;

            // find A(:,k), reusing pleft since Bi [...] is sorted
            int64_t pA_start, pA_end ;
            #ifdef HYPER
            GB_lookup (A_is_hyper, Ah, Ap, &pleft, pright, k,
                &pA_start, &pA_end) ;
            #else
            pA_start = Ap [k] ;
            pA_end   = Ap [k+1] ;
            #endif

            if (pA_start == pA_end) continue ;

            // get the value of B(k,j)
            COPY_ARRAY_TO_SCALAR (bkj, Bx, pB, bsize) ;

            //------------------------------------------------------------------
            // w += A(:,k) * B(k,j)
            //------------------------------------------------------------------

            for (int64_t pA = pA_start ; pA < pA_end ; pA++)
            { 
                // w [i] += A(i,k) * B(k,j)
                int64_t i = Ai [pA] ;
                MULTADD_NOMASK ;
            }
        }

        //----------------------------------------------------------------------
        // gather C(:,j) from w
        //----------------------------------------------------------------------

        for (int64_t pC = pC_start ; pC < pC_end ; pC++)
        { 
            // Cx [pC] = w [Ci [pC]] ;
            COPY_ARRAY_TO_ARRAY (Cx, pC, w, Ci [pC], zsize) ;
        }
    }

    ASSERT (info == GrB_SUCCESS) ;
}
