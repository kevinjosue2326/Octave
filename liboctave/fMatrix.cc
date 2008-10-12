// Matrix manipulations.
/*

Copyright (C) 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002,
              2003, 2004, 2005, 2006, 2007 John W. Eaton
Copyright (C) 2008 Jaroslav Hajek

This file is part of Octave.

Octave is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

Octave is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Octave; see the file COPYING.  If not, see
<http://www.gnu.org/licenses/>.

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cfloat>

#include <iostream>
#include <vector>

#include "Array-util.h"
#include "byte-swap.h"
#include "fMatrix.h"
#include "floatDET.h"
#include "floatSCHUR.h"
#include "floatSVD.h"
#include "floatCHOL.h"
#include "f77-fcn.h"
#include "functor.h"
#include "lo-error.h"
#include "lo-ieee.h"
#include "lo-mappers.h"
#include "lo-utils.h"
#include "mx-base.h"
#include "mx-fm-fdm.h"
#include "mx-fdm-fm.h"
#include "mx-inlines.cc"
#include "oct-cmplx.h"
#include "quit.h"

#if defined (HAVE_FFTW3)
#include "oct-fftw.h"
#endif

// Fortran functions we call.

extern "C"
{
  F77_RET_T
  F77_FUNC (xilaenv, XILAENV) (const octave_idx_type&, F77_CONST_CHAR_ARG_DECL,
			       F77_CONST_CHAR_ARG_DECL,
			       const octave_idx_type&, const octave_idx_type&,
			       const octave_idx_type&, const octave_idx_type&,
			       octave_idx_type&
			       F77_CHAR_ARG_LEN_DECL F77_CHAR_ARG_LEN_DECL);

  F77_RET_T
  F77_FUNC (sgebal, SGEBAL) (F77_CONST_CHAR_ARG_DECL,
			     const octave_idx_type&, float*, const octave_idx_type&, octave_idx_type&,
			     octave_idx_type&, float*, octave_idx_type&
			     F77_CHAR_ARG_LEN_DECL);

  F77_RET_T
  F77_FUNC (sgebak, SGEBAK) (F77_CONST_CHAR_ARG_DECL,
			     F77_CONST_CHAR_ARG_DECL,
			     const octave_idx_type&, const octave_idx_type&, const octave_idx_type&, float*,
			     const octave_idx_type&, float*, const octave_idx_type&, octave_idx_type&
			     F77_CHAR_ARG_LEN_DECL
			     F77_CHAR_ARG_LEN_DECL);


  F77_RET_T
  F77_FUNC (sgemm, SGEMM) (F77_CONST_CHAR_ARG_DECL,
			   F77_CONST_CHAR_ARG_DECL,
			   const octave_idx_type&, const octave_idx_type&, const octave_idx_type&,
			   const float&, const float*, const octave_idx_type&,
			   const float*, const octave_idx_type&, const float&,
			   float*, const octave_idx_type&
			   F77_CHAR_ARG_LEN_DECL
			   F77_CHAR_ARG_LEN_DECL);

  F77_RET_T
  F77_FUNC (sgemv, SGEMV) (F77_CONST_CHAR_ARG_DECL,
			   const octave_idx_type&, const octave_idx_type&, const float&,
			   const float*, const octave_idx_type&, const float*,
			   const octave_idx_type&, const float&, float*,
			   const octave_idx_type&
			   F77_CHAR_ARG_LEN_DECL);

  F77_RET_T
  F77_FUNC (xsdot, XSDOT) (const octave_idx_type&, const float*, const octave_idx_type&,
			   const float*, const octave_idx_type&, float&);

  F77_RET_T
  F77_FUNC (ssyrk, SSYRK) (F77_CONST_CHAR_ARG_DECL,
			   F77_CONST_CHAR_ARG_DECL,
			   const octave_idx_type&, const octave_idx_type&, 
			   const float&, const float*, const octave_idx_type&,
			   const float&, float*, const octave_idx_type&
			   F77_CHAR_ARG_LEN_DECL
			   F77_CHAR_ARG_LEN_DECL);

  F77_RET_T
  F77_FUNC (sgetrf, SGETRF) (const octave_idx_type&, const octave_idx_type&, float*, const octave_idx_type&,
		      octave_idx_type*, octave_idx_type&);

  F77_RET_T
  F77_FUNC (sgetrs, SGETRS) (F77_CONST_CHAR_ARG_DECL, const octave_idx_type&, const octave_idx_type&, 
			     const float*, const octave_idx_type&,
			     const octave_idx_type*, float*, const octave_idx_type&, octave_idx_type&
			     F77_CHAR_ARG_LEN_DECL);

  F77_RET_T
  F77_FUNC (sgetri, SGETRI) (const octave_idx_type&, float*, const octave_idx_type&, const octave_idx_type*,
			     float*, const octave_idx_type&, octave_idx_type&);

  F77_RET_T
  F77_FUNC (sgecon, SGECON) (F77_CONST_CHAR_ARG_DECL, const octave_idx_type&, float*, 
			     const octave_idx_type&, const float&, float&, 
			     float*, octave_idx_type*, octave_idx_type&
			     F77_CHAR_ARG_LEN_DECL);

  F77_RET_T
  F77_FUNC (sgelsy, SGELSY) (const octave_idx_type&, const octave_idx_type&, const octave_idx_type&,
			     float*, const octave_idx_type&, float*,
			     const octave_idx_type&, octave_idx_type*, float&, octave_idx_type&,
			     float*, const octave_idx_type&, octave_idx_type&);

  F77_RET_T
  F77_FUNC (sgelsd, SGELSD) (const octave_idx_type&, const octave_idx_type&, const octave_idx_type&,
			     float*, const octave_idx_type&, float*,
			     const octave_idx_type&, float*, float&, octave_idx_type&,
			     float*, const octave_idx_type&, octave_idx_type*,
			     octave_idx_type&);

  F77_RET_T
  F77_FUNC (spotrf, SPOTRF) (F77_CONST_CHAR_ARG_DECL, const octave_idx_type&, 
			     float *, const octave_idx_type&, 
			     octave_idx_type& F77_CHAR_ARG_LEN_DECL);

  F77_RET_T
  F77_FUNC (spocon, SPOCON) (F77_CONST_CHAR_ARG_DECL, const octave_idx_type&, 
			     float*, const octave_idx_type&, const float&,
			     float&, float*, octave_idx_type*,
			     octave_idx_type& F77_CHAR_ARG_LEN_DECL);
  F77_RET_T
  F77_FUNC (spotrs, SPOTRS) (F77_CONST_CHAR_ARG_DECL, const octave_idx_type&, 
			     const octave_idx_type&, const float*, 
			     const octave_idx_type&, float*, 
			     const octave_idx_type&, octave_idx_type&
			     F77_CHAR_ARG_LEN_DECL);

  F77_RET_T
  F77_FUNC (strtri, STRTRI) (F77_CONST_CHAR_ARG_DECL, F77_CONST_CHAR_ARG_DECL, 
			     const octave_idx_type&, const float*, 
			     const octave_idx_type&, octave_idx_type&
			     F77_CHAR_ARG_LEN_DECL
			     F77_CHAR_ARG_LEN_DECL);
  F77_RET_T
  F77_FUNC (strcon, STRCON) (F77_CONST_CHAR_ARG_DECL, F77_CONST_CHAR_ARG_DECL, 
			     F77_CONST_CHAR_ARG_DECL, const octave_idx_type&, 
			     const float*, const octave_idx_type&, float&,
			     float*, octave_idx_type*, octave_idx_type& 
			     F77_CHAR_ARG_LEN_DECL
			     F77_CHAR_ARG_LEN_DECL
			     F77_CHAR_ARG_LEN_DECL);
  F77_RET_T
  F77_FUNC (strtrs, STRTRS) (F77_CONST_CHAR_ARG_DECL, F77_CONST_CHAR_ARG_DECL, 
			     F77_CONST_CHAR_ARG_DECL, const octave_idx_type&, 
			     const octave_idx_type&, const float*, 
			     const octave_idx_type&, float*, 
			     const octave_idx_type&, octave_idx_type&
			     F77_CHAR_ARG_LEN_DECL
			     F77_CHAR_ARG_LEN_DECL
			     F77_CHAR_ARG_LEN_DECL);

  // Note that the original complex fft routines were not written for
  // float complex arguments.  They have been modified by adding an
  // implicit float precision (a-h,o-z) statement at the beginning of
  // each subroutine.

  F77_RET_T
  F77_FUNC (cffti, CFFTI) (const octave_idx_type&, FloatComplex*);

  F77_RET_T
  F77_FUNC (cfftf, CFFTF) (const octave_idx_type&, FloatComplex*, FloatComplex*);

  F77_RET_T
  F77_FUNC (cfftb, CFFTB) (const octave_idx_type&, FloatComplex*, FloatComplex*);

  F77_RET_T
  F77_FUNC (slartg, SLARTG) (const float&, const float&, float&,
			     float&, float&);

  F77_RET_T
  F77_FUNC (strsyl, STRSYL) (F77_CONST_CHAR_ARG_DECL,
			     F77_CONST_CHAR_ARG_DECL,
			     const octave_idx_type&, const octave_idx_type&, const octave_idx_type&,
			     const float*, const octave_idx_type&, const float*,
			     const octave_idx_type&, const float*, const octave_idx_type&,
			     float&, octave_idx_type&
			     F77_CHAR_ARG_LEN_DECL
			     F77_CHAR_ARG_LEN_DECL);

  F77_RET_T
  F77_FUNC (xslange, XSLANGE) (F77_CONST_CHAR_ARG_DECL, const octave_idx_type&,
			       const octave_idx_type&, const float*,
			       const octave_idx_type&, float*, float&
			       F77_CHAR_ARG_LEN_DECL); 
}

// Matrix class.

FloatMatrix::FloatMatrix (const FloatRowVector& rv)
  : MArray2<float> (1, rv.length (), 0.0)
{
  for (octave_idx_type i = 0; i < rv.length (); i++)
    elem (0, i) = rv.elem (i);
}

FloatMatrix::FloatMatrix (const FloatColumnVector& cv)
  : MArray2<float> (cv.length (), 1, 0.0)
{
  for (octave_idx_type i = 0; i < cv.length (); i++)
    elem (i, 0) = cv.elem (i);
}

FloatMatrix::FloatMatrix (const FloatDiagMatrix& a)
  : MArray2<float> (a.rows (), a.cols (), 0.0)
{
  for (octave_idx_type i = 0; i < a.length (); i++)
    elem (i, i) = a.elem (i, i);
}

// FIXME -- could we use a templated mixed-type copy function
// here?

FloatMatrix::FloatMatrix (const boolMatrix& a)
  : MArray2<float> (a.rows (), a.cols ())
{
  for (octave_idx_type i = 0; i < a.rows (); i++)
    for (octave_idx_type j = 0; j < a.cols (); j++)
      elem (i, j) = a.elem (i, j);
}

FloatMatrix::FloatMatrix (const charMatrix& a)
  : MArray2<float> (a.rows (), a.cols ())
{
  for (octave_idx_type i = 0; i < a.rows (); i++)
    for (octave_idx_type j = 0; j < a.cols (); j++)
      elem (i, j) = a.elem (i, j);
}

bool
FloatMatrix::operator == (const FloatMatrix& a) const
{
  if (rows () != a.rows () || cols () != a.cols ())
    return false;

  return mx_inline_equal (data (), a.data (), length ());
}

bool
FloatMatrix::operator != (const FloatMatrix& a) const
{
  return !(*this == a);
}

bool
FloatMatrix::is_symmetric (void) const
{
  if (is_square () && rows () > 0)
    {
      for (octave_idx_type i = 0; i < rows (); i++)
	for (octave_idx_type j = i+1; j < cols (); j++)
	  if (elem (i, j) != elem (j, i))
	    return false;

      return true;
    }

  return false;
}

FloatMatrix&
FloatMatrix::insert (const FloatMatrix& a, octave_idx_type r, octave_idx_type c)
{
  Array2<float>::insert (a, r, c);
  return *this;
}

FloatMatrix&
FloatMatrix::insert (const FloatRowVector& a, octave_idx_type r, octave_idx_type c)
{
  octave_idx_type a_len = a.length ();

  if (r < 0 || r >= rows () || c < 0 || c + a_len > cols ())
    {
      (*current_liboctave_error_handler) ("range error for insert");
      return *this;
    }

  if (a_len > 0)
    {
      make_unique ();

      for (octave_idx_type i = 0; i < a_len; i++)
	xelem (r, c+i) = a.elem (i);
    }

  return *this;
}

FloatMatrix&
FloatMatrix::insert (const FloatColumnVector& a, octave_idx_type r, octave_idx_type c)
{
  octave_idx_type a_len = a.length ();

  if (r < 0 || r + a_len > rows () || c < 0 || c >= cols ())
    {
      (*current_liboctave_error_handler) ("range error for insert");
      return *this;
    }

  if (a_len > 0)
    {
      make_unique ();

      for (octave_idx_type i = 0; i < a_len; i++)
	xelem (r+i, c) = a.elem (i);
    }

  return *this;
}

FloatMatrix&
FloatMatrix::insert (const FloatDiagMatrix& a, octave_idx_type r, octave_idx_type c)
{
  octave_idx_type a_nr = a.rows ();
  octave_idx_type a_nc = a.cols ();

  if (r < 0 || r + a_nr > rows () || c < 0 || c + a_nc > cols ())
    {
      (*current_liboctave_error_handler) ("range error for insert");
      return *this;
    }

  fill (0.0, r, c, r + a_nr - 1, c + a_nc - 1);

  octave_idx_type a_len = a.length ();

  if (a_len > 0)
    {
      make_unique ();

      for (octave_idx_type i = 0; i < a_len; i++)
	xelem (r+i, c+i) = a.elem (i, i);
    }

  return *this;
}

FloatMatrix&
FloatMatrix::fill (float val)
{
  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  if (nr > 0 && nc > 0)
    {
      make_unique ();

      for (octave_idx_type j = 0; j < nc; j++)
	for (octave_idx_type i = 0; i < nr; i++)
	  xelem (i, j) = val;
    }

  return *this;
}

FloatMatrix&
FloatMatrix::fill (float val, octave_idx_type r1, octave_idx_type c1, octave_idx_type r2, octave_idx_type c2)
{
  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  if (r1 < 0 || r2 < 0 || c1 < 0 || c2 < 0
      || r1 >= nr || r2 >= nr || c1 >= nc || c2 >= nc)
    {
      (*current_liboctave_error_handler) ("range error for fill");
      return *this;
    }

  if (r1 > r2) { octave_idx_type tmp = r1; r1 = r2; r2 = tmp; }
  if (c1 > c2) { octave_idx_type tmp = c1; c1 = c2; c2 = tmp; }

  if (r2 >= r1 && c2 >= c1)
    {
      make_unique ();

      for (octave_idx_type j = c1; j <= c2; j++)
	for (octave_idx_type i = r1; i <= r2; i++)
	  xelem (i, j) = val;
    }

  return *this;
}

FloatMatrix
FloatMatrix::append (const FloatMatrix& a) const
{
  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();
  if (nr != a.rows ())
    {
      (*current_liboctave_error_handler) ("row dimension mismatch for append");
      return FloatMatrix ();
    }

  octave_idx_type nc_insert = nc;
  FloatMatrix retval (nr, nc + a.cols ());
  retval.insert (*this, 0, 0);
  retval.insert (a, 0, nc_insert);
  return retval;
}

FloatMatrix
FloatMatrix::append (const FloatRowVector& a) const
{
  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();
  if (nr != 1)
    {
      (*current_liboctave_error_handler) ("row dimension mismatch for append");
      return FloatMatrix ();
    }

  octave_idx_type nc_insert = nc;
  FloatMatrix retval (nr, nc + a.length ());
  retval.insert (*this, 0, 0);
  retval.insert (a, 0, nc_insert);
  return retval;
}

FloatMatrix
FloatMatrix::append (const FloatColumnVector& a) const
{
  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();
  if (nr != a.length ())
    {
      (*current_liboctave_error_handler) ("row dimension mismatch for append");
      return FloatMatrix ();
    }

  octave_idx_type nc_insert = nc;
  FloatMatrix retval (nr, nc + 1);
  retval.insert (*this, 0, 0);
  retval.insert (a, 0, nc_insert);
  return retval;
}

FloatMatrix
FloatMatrix::append (const FloatDiagMatrix& a) const
{
  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();
  if (nr != a.rows ())
    {
      (*current_liboctave_error_handler) ("row dimension mismatch for append");
      return *this;
    }

  octave_idx_type nc_insert = nc;
  FloatMatrix retval (nr, nc + a.cols ());
  retval.insert (*this, 0, 0);
  retval.insert (a, 0, nc_insert);
  return retval;
}

FloatMatrix
FloatMatrix::stack (const FloatMatrix& a) const
{
  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();
  if (nc != a.cols ())
    {
      (*current_liboctave_error_handler)
	("column dimension mismatch for stack");
      return FloatMatrix ();
    }

  octave_idx_type nr_insert = nr;
  FloatMatrix retval (nr + a.rows (), nc);
  retval.insert (*this, 0, 0);
  retval.insert (a, nr_insert, 0);
  return retval;
}

FloatMatrix
FloatMatrix::stack (const FloatRowVector& a) const
{
  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();
  if (nc != a.length ())
    {
      (*current_liboctave_error_handler)
	("column dimension mismatch for stack");
      return FloatMatrix ();
    }

  octave_idx_type nr_insert = nr;
  FloatMatrix retval (nr + 1, nc);
  retval.insert (*this, 0, 0);
  retval.insert (a, nr_insert, 0);
  return retval;
}

FloatMatrix
FloatMatrix::stack (const FloatColumnVector& a) const
{
  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();
  if (nc != 1)
    {
      (*current_liboctave_error_handler)
	("column dimension mismatch for stack");
      return FloatMatrix ();
    }

  octave_idx_type nr_insert = nr;
  FloatMatrix retval (nr + a.length (), nc);
  retval.insert (*this, 0, 0);
  retval.insert (a, nr_insert, 0);
  return retval;
}

FloatMatrix
FloatMatrix::stack (const FloatDiagMatrix& a) const
{
  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();
  if (nc != a.cols ())
    {
      (*current_liboctave_error_handler)
	("column dimension mismatch for stack");
      return FloatMatrix ();
    }

  octave_idx_type nr_insert = nr;
  FloatMatrix retval (nr + a.rows (), nc);
  retval.insert (*this, 0, 0);
  retval.insert (a, nr_insert, 0);
  return retval;
}

FloatMatrix
real (const FloatComplexMatrix& a)
{
  octave_idx_type a_len = a.length ();
  FloatMatrix retval;
  if (a_len > 0)
    retval = FloatMatrix (mx_inline_real_dup (a.data (), a_len),
		     a.rows (), a.cols ());
  return retval;
}

FloatMatrix
imag (const FloatComplexMatrix& a)
{
  octave_idx_type a_len = a.length ();
  FloatMatrix retval;
  if (a_len > 0)
    retval = FloatMatrix (mx_inline_imag_dup (a.data (), a_len),
		     a.rows (), a.cols ());
  return retval;
}

FloatMatrix
FloatMatrix::extract (octave_idx_type r1, octave_idx_type c1, octave_idx_type r2, octave_idx_type c2) const
{
  if (r1 > r2) { octave_idx_type tmp = r1; r1 = r2; r2 = tmp; }
  if (c1 > c2) { octave_idx_type tmp = c1; c1 = c2; c2 = tmp; }

  octave_idx_type new_r = r2 - r1 + 1;
  octave_idx_type new_c = c2 - c1 + 1;

  FloatMatrix result (new_r, new_c);

  for (octave_idx_type j = 0; j < new_c; j++)
    for (octave_idx_type i = 0; i < new_r; i++)
      result.xelem (i, j) = elem (r1+i, c1+j);

  return result;
}

FloatMatrix
FloatMatrix::extract_n (octave_idx_type r1, octave_idx_type c1, octave_idx_type nr, octave_idx_type nc) const
{
  FloatMatrix result (nr, nc);

  for (octave_idx_type j = 0; j < nc; j++)
    for (octave_idx_type i = 0; i < nr; i++)
      result.xelem (i, j) = elem (r1+i, c1+j);

  return result;
}

// extract row or column i.

FloatRowVector
FloatMatrix::row (octave_idx_type i) const
{
  octave_idx_type nc = cols ();
  if (i < 0 || i >= rows ())
    {
      (*current_liboctave_error_handler) ("invalid row selection");
      return FloatRowVector ();
    }

  FloatRowVector retval (nc);
  for (octave_idx_type j = 0; j < nc; j++)
    retval.xelem (j) = elem (i, j);

  return retval;
}

FloatColumnVector
FloatMatrix::column (octave_idx_type i) const
{
  octave_idx_type nr = rows ();
  if (i < 0 || i >= cols ())
    {
      (*current_liboctave_error_handler) ("invalid column selection");
      return FloatColumnVector ();
    }

  FloatColumnVector retval (nr);
  for (octave_idx_type j = 0; j < nr; j++)
    retval.xelem (j) = elem (j, i);

  return retval;
}

FloatMatrix
FloatMatrix::inverse (void) const
{
  octave_idx_type info;
  float rcon;
  MatrixType mattype (*this);
  return inverse (mattype, info, rcon, 0, 0);
}

FloatMatrix
FloatMatrix::inverse (octave_idx_type& info) const
{
  float rcon;
  MatrixType mattype (*this);
  return inverse (mattype, info, rcon, 0, 0);
}

FloatMatrix
FloatMatrix::inverse (octave_idx_type& info, float& rcon, int force,
		 int calc_cond) const
{
  MatrixType mattype (*this);
  return inverse (mattype, info, rcon, force, calc_cond);
}

FloatMatrix
FloatMatrix::inverse (MatrixType& mattype) const
{
  octave_idx_type info;
  float rcon;
  return inverse (mattype, info, rcon, 0, 0);
}

FloatMatrix
FloatMatrix::inverse (MatrixType &mattype, octave_idx_type& info) const
{
  float rcon;
  return inverse (mattype, info, rcon, 0, 0);
}

FloatMatrix
FloatMatrix::tinverse (MatrixType &mattype, octave_idx_type& info, float& rcon, 
		  int force, int calc_cond) const
{
  FloatMatrix retval;

  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  if (nr != nc || nr == 0 || nc == 0)
    (*current_liboctave_error_handler) ("inverse requires square matrix");
  else
    {
      int typ = mattype.type ();
      char uplo = (typ == MatrixType::Lower ? 'L' : 'U');
      char udiag = 'N';
      retval = *this;
      float *tmp_data = retval.fortran_vec ();

      F77_XFCN (strtri, STRTRI, (F77_CONST_CHAR_ARG2 (&uplo, 1),
				 F77_CONST_CHAR_ARG2 (&udiag, 1),
				 nr, tmp_data, nr, info 
				 F77_CHAR_ARG_LEN (1)
				 F77_CHAR_ARG_LEN (1)));

      // Throw-away extra info LAPACK gives so as to not change output.
      rcon = 0.0;
      if (info != 0) 
	info = -1;
      else if (calc_cond) 
	{
	  octave_idx_type dtrcon_info = 0;
	  char job = '1';

	  OCTAVE_LOCAL_BUFFER (float, work, 3 * nr);
	  OCTAVE_LOCAL_BUFFER (octave_idx_type, iwork, nr);

	  F77_XFCN (strcon, STRCON, (F77_CONST_CHAR_ARG2 (&job, 1),
				     F77_CONST_CHAR_ARG2 (&uplo, 1),
				     F77_CONST_CHAR_ARG2 (&udiag, 1),
				     nr, tmp_data, nr, rcon, 
				     work, iwork, dtrcon_info 
				     F77_CHAR_ARG_LEN (1)
				     F77_CHAR_ARG_LEN (1)
				     F77_CHAR_ARG_LEN (1)));

	  if (dtrcon_info != 0) 
	    info = -1;
	}

      if (info == -1 && ! force)
	retval = *this; // Restore matrix contents.
    }

  return retval;
}


FloatMatrix
FloatMatrix::finverse (MatrixType &mattype, octave_idx_type& info, float& rcon, 
		  int force, int calc_cond) const
{
  FloatMatrix retval;

  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  if (nr != nc || nr == 0 || nc == 0)
    (*current_liboctave_error_handler) ("inverse requires square matrix");
  else
    {
      Array<octave_idx_type> ipvt (nr);
      octave_idx_type *pipvt = ipvt.fortran_vec ();

      retval = *this;
      float *tmp_data = retval.fortran_vec ();

      Array<float> z(1);
      octave_idx_type lwork = -1;

      // Query the optimum work array size.
      F77_XFCN (sgetri, SGETRI, (nc, tmp_data, nr, pipvt, 
				 z.fortran_vec (), lwork, info));

      lwork = static_cast<octave_idx_type> (z(0));
      lwork = (lwork < 2 *nc ? 2*nc : lwork);
      z.resize (lwork);
      float *pz = z.fortran_vec ();

      info = 0;

      // Calculate the norm of the matrix, for later use.
      float anorm = 0;
      if (calc_cond) 
	anorm = retval.abs().sum().row(static_cast<octave_idx_type>(0)).max();

      F77_XFCN (sgetrf, SGETRF, (nc, nc, tmp_data, nr, pipvt, info));

      // Throw-away extra info LAPACK gives so as to not change output.
      rcon = 0.0;
      if (info != 0) 
	info = -1;
      else if (calc_cond) 
	{
	  octave_idx_type dgecon_info = 0;

	  // Now calculate the condition number for non-singular matrix.
	  char job = '1';
	  Array<octave_idx_type> iz (nc);
	  octave_idx_type *piz = iz.fortran_vec ();
	  F77_XFCN (sgecon, SGECON, (F77_CONST_CHAR_ARG2 (&job, 1),
				     nc, tmp_data, nr, anorm, 
				     rcon, pz, piz, dgecon_info
				     F77_CHAR_ARG_LEN (1)));

	  if (dgecon_info != 0) 
	    info = -1;
	}

      if (info == -1 && ! force)
	retval = *this; // Restore matrix contents.
      else
	{
	  octave_idx_type dgetri_info = 0;

	  F77_XFCN (sgetri, SGETRI, (nc, tmp_data, nr, pipvt,
				     pz, lwork, dgetri_info));

	  if (dgetri_info != 0) 
	    info = -1;
	}

      if (info != 0)
	mattype.mark_as_rectangular();
    }

  return retval;
}

FloatMatrix
FloatMatrix::inverse (MatrixType &mattype, octave_idx_type& info, float& rcon, 
		 int force, int calc_cond) const
{
  int typ = mattype.type (false);
  FloatMatrix ret;

  if (typ == MatrixType::Unknown)
    typ = mattype.type (*this);

  if (typ == MatrixType::Upper || typ == MatrixType::Lower)
    ret = tinverse (mattype, info, rcon, force, calc_cond);
  else
    {
      if (mattype.is_hermitian ())
	{
	  FloatCHOL chol (*this, info, calc_cond);
	  if (info == 0)
	    {
	      if (calc_cond)
		rcon = chol.rcond ();
	      else
		rcon = 1.0;
	      ret = chol.inverse ();
	    }
	  else
	    mattype.mark_as_unsymmetric ();
	}

      if (!mattype.is_hermitian ())
	ret = finverse(mattype, info, rcon, force, calc_cond);

      if ((mattype.is_hermitian () || calc_cond) && rcon == 0.)
	ret = FloatMatrix (rows (), columns (), octave_Float_Inf);
    }

  return ret;
}

FloatMatrix
FloatMatrix::pseudo_inverse (float tol) const
{
  FloatSVD result (*this, SVD::economy);

  FloatDiagMatrix S = result.singular_values ();
  FloatMatrix U = result.left_singular_matrix ();
  FloatMatrix V = result.right_singular_matrix ();

  FloatColumnVector sigma = S.diag ();

  octave_idx_type r = sigma.length () - 1;
  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  if (tol <= 0.0)
    {
      if (nr > nc)
	tol = nr * sigma.elem (0) * DBL_EPSILON;
      else
	tol = nc * sigma.elem (0) * DBL_EPSILON;
    }

  while (r >= 0 && sigma.elem (r) < tol)
    r--;

  if (r < 0)
    return FloatMatrix (nc, nr, 0.0);
  else
    {
      FloatMatrix Ur = U.extract (0, 0, nr-1, r);
      FloatDiagMatrix D = FloatDiagMatrix (sigma.extract (0, r)) . inverse ();
      FloatMatrix Vr = V.extract (0, 0, nc-1, r);
      return Vr * D * Ur.transpose ();
    }
}

#if defined (HAVE_FFTW3)

FloatComplexMatrix
FloatMatrix::fourier (void) const
{
  size_t nr = rows ();
  size_t nc = cols ();

  FloatComplexMatrix retval (nr, nc);

  size_t npts, nsamples;

  if (nr == 1 || nc == 1)
    {
      npts = nr > nc ? nr : nc;
      nsamples = 1;
    }
  else
    {
      npts = nr;
      nsamples = nc;
    }

  const float *in (fortran_vec ());
  FloatComplex *out (retval.fortran_vec ());

  octave_fftw::fft (in, out, npts, nsamples); 

  return retval;
}

FloatComplexMatrix
FloatMatrix::ifourier (void) const
{
  size_t nr = rows ();
  size_t nc = cols ();

  FloatComplexMatrix retval (nr, nc);

  size_t npts, nsamples;

  if (nr == 1 || nc == 1)
    {
      npts = nr > nc ? nr : nc;
      nsamples = 1;
    }
  else
    {
      npts = nr;
      nsamples = nc;
    }

  FloatComplexMatrix tmp (*this);
  FloatComplex *in (tmp.fortran_vec ());
  FloatComplex *out (retval.fortran_vec ());

  octave_fftw::ifft (in, out, npts, nsamples); 

  return retval;
}

FloatComplexMatrix
FloatMatrix::fourier2d (void) const
{
  dim_vector dv(rows (), cols ());

  const float *in = fortran_vec ();
  FloatComplexMatrix retval (rows (), cols ());
  octave_fftw::fftNd (in, retval.fortran_vec (), 2, dv);

  return retval;
}

FloatComplexMatrix
FloatMatrix::ifourier2d (void) const
{
  dim_vector dv(rows (), cols ());

  FloatComplexMatrix retval (*this);
  FloatComplex *out (retval.fortran_vec ());

  octave_fftw::ifftNd (out, out, 2, dv);

  return retval;
}

#else

FloatComplexMatrix
FloatMatrix::fourier (void) const
{
  FloatComplexMatrix retval;

  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  octave_idx_type npts, nsamples;

  if (nr == 1 || nc == 1)
    {
      npts = nr > nc ? nr : nc;
      nsamples = 1;
    }
  else
    {
      npts = nr;
      nsamples = nc;
    }

  octave_idx_type nn = 4*npts+15;

  Array<FloatComplex> wsave (nn);
  FloatComplex *pwsave = wsave.fortran_vec ();

  retval = FloatComplexMatrix (*this);
  FloatComplex *tmp_data = retval.fortran_vec ();

  F77_FUNC (cffti, CFFTI) (npts, pwsave);

  for (octave_idx_type j = 0; j < nsamples; j++)
    {
      OCTAVE_QUIT;

      F77_FUNC (cfftf, CFFTF) (npts, &tmp_data[npts*j], pwsave);
    }

  return retval;
}

FloatComplexMatrix
FloatMatrix::ifourier (void) const
{
  FloatComplexMatrix retval;

  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  octave_idx_type npts, nsamples;

  if (nr == 1 || nc == 1)
    {
      npts = nr > nc ? nr : nc;
      nsamples = 1;
    }
  else
    {
      npts = nr;
      nsamples = nc;
    }

  octave_idx_type nn = 4*npts+15;

  Array<FloatComplex> wsave (nn);
  FloatComplex *pwsave = wsave.fortran_vec ();

  retval = FloatComplexMatrix (*this);
  FloatComplex *tmp_data = retval.fortran_vec ();

  F77_FUNC (cffti, CFFTI) (npts, pwsave);

  for (octave_idx_type j = 0; j < nsamples; j++)
    {
      OCTAVE_QUIT;

      F77_FUNC (cfftb, CFFTB) (npts, &tmp_data[npts*j], pwsave);
    }

  for (octave_idx_type j = 0; j < npts*nsamples; j++)
    tmp_data[j] = tmp_data[j] / static_cast<float> (npts);

  return retval;
}

FloatComplexMatrix
FloatMatrix::fourier2d (void) const
{
  FloatComplexMatrix retval;

  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  octave_idx_type npts, nsamples;

  if (nr == 1 || nc == 1)
    {
      npts = nr > nc ? nr : nc;
      nsamples = 1;
    }
  else
    {
      npts = nr;
      nsamples = nc;
    }

  octave_idx_type nn = 4*npts+15;

  Array<FloatComplex> wsave (nn);
  FloatComplex *pwsave = wsave.fortran_vec ();

  retval = FloatComplexMatrix (*this);
  FloatComplex *tmp_data = retval.fortran_vec ();

  F77_FUNC (cffti, CFFTI) (npts, pwsave);

  for (octave_idx_type j = 0; j < nsamples; j++)
    {
      OCTAVE_QUIT;

      F77_FUNC (cfftf, CFFTF) (npts, &tmp_data[npts*j], pwsave);
    }

  npts = nc;
  nsamples = nr;
  nn = 4*npts+15;

  wsave.resize (nn);
  pwsave = wsave.fortran_vec ();

  Array<FloatComplex> tmp (npts);
  FloatComplex *prow = tmp.fortran_vec ();

  F77_FUNC (cffti, CFFTI) (npts, pwsave);

  for (octave_idx_type j = 0; j < nsamples; j++)
    {
      OCTAVE_QUIT;

      for (octave_idx_type i = 0; i < npts; i++)
	prow[i] = tmp_data[i*nr + j];

      F77_FUNC (cfftf, CFFTF) (npts, prow, pwsave);

      for (octave_idx_type i = 0; i < npts; i++)
	tmp_data[i*nr + j] = prow[i];
    }

  return retval;
}

FloatComplexMatrix
FloatMatrix::ifourier2d (void) const
{
  FloatComplexMatrix retval;

  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  octave_idx_type npts, nsamples;

  if (nr == 1 || nc == 1)
    {
      npts = nr > nc ? nr : nc;
      nsamples = 1;
    }
  else
    {
      npts = nr;
      nsamples = nc;
    }

  octave_idx_type nn = 4*npts+15;

  Array<FloatComplex> wsave (nn);
  FloatComplex *pwsave = wsave.fortran_vec ();

  retval = FloatComplexMatrix (*this);
  FloatComplex *tmp_data = retval.fortran_vec ();

  F77_FUNC (cffti, CFFTI) (npts, pwsave);

  for (octave_idx_type j = 0; j < nsamples; j++)
    {
      OCTAVE_QUIT;

      F77_FUNC (cfftb, CFFTB) (npts, &tmp_data[npts*j], pwsave);
    }

  for (octave_idx_type j = 0; j < npts*nsamples; j++)
    tmp_data[j] = tmp_data[j] / static_cast<float> (npts);

  npts = nc;
  nsamples = nr;
  nn = 4*npts+15;

  wsave.resize (nn);
  pwsave = wsave.fortran_vec ();

  Array<FloatComplex> tmp (npts);
  FloatComplex *prow = tmp.fortran_vec ();

  F77_FUNC (cffti, CFFTI) (npts, pwsave);

  for (octave_idx_type j = 0; j < nsamples; j++)
    {
      OCTAVE_QUIT;

      for (octave_idx_type i = 0; i < npts; i++)
	prow[i] = tmp_data[i*nr + j];

      F77_FUNC (cfftb, CFFTB) (npts, prow, pwsave);

      for (octave_idx_type i = 0; i < npts; i++)
	tmp_data[i*nr + j] = prow[i] / static_cast<float> (npts);
    }

  return retval;
}

#endif

FloatDET
FloatMatrix::determinant (void) const
{
  octave_idx_type info;
  float rcon;
  return determinant (info, rcon, 0);
}

FloatDET
FloatMatrix::determinant (octave_idx_type& info) const
{
  float rcon;
  return determinant (info, rcon, 0);
}

FloatDET
FloatMatrix::determinant (octave_idx_type& info, float& rcon, int calc_cond) const
{
  FloatDET retval;

  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  if (nr == 0 || nc == 0)
    {
      retval = FloatDET (1.0, 0);
    }
  else
    {
      Array<octave_idx_type> ipvt (nr);
      octave_idx_type *pipvt = ipvt.fortran_vec ();

      FloatMatrix atmp = *this;
      float *tmp_data = atmp.fortran_vec ();

      info = 0;

      // Calculate the norm of the matrix, for later use.
      float anorm = 0;
      if (calc_cond) 
	anorm = atmp.abs().sum().row(static_cast<octave_idx_type>(0)).max();

      F77_XFCN (sgetrf, SGETRF, (nr, nr, tmp_data, nr, pipvt, info));

      // Throw-away extra info LAPACK gives so as to not change output.
      rcon = 0.0;
      if (info != 0) 
	{
	  info = -1;
	  retval = FloatDET ();
	} 
      else 
	{
	  if (calc_cond) 
	    {
	      // Now calc the condition number for non-singular matrix.
	      char job = '1';
	      Array<float> z (4 * nc);
	      float *pz = z.fortran_vec ();
	      Array<octave_idx_type> iz (nc);
	      octave_idx_type *piz = iz.fortran_vec ();

	      F77_XFCN (sgecon, SGECON, (F77_CONST_CHAR_ARG2 (&job, 1),
					 nc, tmp_data, nr, anorm, 
					 rcon, pz, piz, info
					 F77_CHAR_ARG_LEN (1)));
	    }

	  if (info != 0) 
	    {
	      info = -1;
	      retval = FloatDET ();
	    } 
	  else 
	    {
	      float c = 1.0;
	      int e = 0;

	      for (octave_idx_type i = 0; i < nc; i++) 
		{
		  if (ipvt(i) != (i+1))
		    c = -c;

		  c *= atmp(i,i);

		  if (c == 0.0)
		    break;

		  while (fabs (c) < 0.5)
		    {
		      c *= 2.0;
		      e--;
		    }

		  while (fabs (c) >= 2.0)
		    {
		      c /= 2.0;
		      e++;
		    }
		}

	      retval = FloatDET (c, e);
	    }
	}
    }

  return retval;
}

float
FloatMatrix::rcond (void) const
{
  MatrixType mattype (*this);
  return rcond (mattype);
}

float
FloatMatrix::rcond (MatrixType &mattype) const
{
  float rcon;
  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  if (nr != nc)
    (*current_liboctave_error_handler) ("matrix must be square");
  else if (nr == 0 || nc == 0)
    rcon = octave_Inf;
  else
    {
      int typ = mattype.type ();

      if (typ == MatrixType::Unknown)
	typ = mattype.type (*this);

      // Only calculate the condition number for LU/Cholesky
      if (typ == MatrixType::Upper)
	{
	  const float *tmp_data = fortran_vec ();
	  octave_idx_type info = 0;
	  char norm = '1';
	  char uplo = 'U';
	  char dia = 'N';

	  Array<float> z (3 * nc);
	  float *pz = z.fortran_vec ();
	  Array<octave_idx_type> iz (nc);
	  octave_idx_type *piz = iz.fortran_vec ();

	  F77_XFCN (strcon, STRCON, (F77_CONST_CHAR_ARG2 (&norm, 1), 
				     F77_CONST_CHAR_ARG2 (&uplo, 1), 
				     F77_CONST_CHAR_ARG2 (&dia, 1), 
				     nr, tmp_data, nr, rcon,
				     pz, piz, info
				     F77_CHAR_ARG_LEN (1)
				     F77_CHAR_ARG_LEN (1)
				     F77_CHAR_ARG_LEN (1)));

	  if (info != 0) 
	    rcon = 0.0;
	}
      else if  (typ == MatrixType::Permuted_Upper)
	(*current_liboctave_error_handler)
	  ("permuted triangular matrix not implemented");
      else if (typ == MatrixType::Lower)
	{
	  const float *tmp_data = fortran_vec ();
	  octave_idx_type info = 0;
	  char norm = '1';
	  char uplo = 'L';
	  char dia = 'N';

	  Array<float> z (3 * nc);
	  float *pz = z.fortran_vec ();
	  Array<octave_idx_type> iz (nc);
	  octave_idx_type *piz = iz.fortran_vec ();

	  F77_XFCN (strcon, STRCON, (F77_CONST_CHAR_ARG2 (&norm, 1), 
				     F77_CONST_CHAR_ARG2 (&uplo, 1), 
				     F77_CONST_CHAR_ARG2 (&dia, 1), 
				     nr, tmp_data, nr, rcon,
				     pz, piz, info
				     F77_CHAR_ARG_LEN (1)
				     F77_CHAR_ARG_LEN (1)
				     F77_CHAR_ARG_LEN (1)));

	  if (info != 0) 
	    rcon = 0.0;
	}
      else if (typ == MatrixType::Permuted_Lower)
	(*current_liboctave_error_handler)
	  ("permuted triangular matrix not implemented");
      else if (typ == MatrixType::Full || typ == MatrixType::Hermitian)
	{
	  float anorm = -1.0;
	  FloatMatrix atmp = *this;
	  float *tmp_data = atmp.fortran_vec ();

	  if (typ == MatrixType::Hermitian)
	    {
	      octave_idx_type info = 0;
	      char job = 'L';
	      anorm = atmp.abs().sum().
		row(static_cast<octave_idx_type>(0)).max();

	      F77_XFCN (spotrf, SPOTRF, (F77_CONST_CHAR_ARG2 (&job, 1), nr, 
					 tmp_data, nr, info
					 F77_CHAR_ARG_LEN (1)));

	      if (info != 0) 
		{
		  rcon = 0.0;
		  mattype.mark_as_unsymmetric ();
		  typ = MatrixType::Full;
		}
	      else 
		{
		  Array<float> z (3 * nc);
		  float *pz = z.fortran_vec ();
		  Array<octave_idx_type> iz (nc);
		  octave_idx_type *piz = iz.fortran_vec ();

		  F77_XFCN (spocon, SPOCON, (F77_CONST_CHAR_ARG2 (&job, 1),
					     nr, tmp_data, nr, anorm,
					     rcon, pz, piz, info
					     F77_CHAR_ARG_LEN (1)));

		  if (info != 0) 
		    rcon = 0.0;
		}
	    }

	  if (typ == MatrixType::Full)
	    {
	      octave_idx_type info = 0;

	      Array<octave_idx_type> ipvt (nr);
	      octave_idx_type *pipvt = ipvt.fortran_vec ();

	      if(anorm < 0.)
		anorm = atmp.abs().sum().
		  row(static_cast<octave_idx_type>(0)).max();

	      Array<float> z (4 * nc);
	      float *pz = z.fortran_vec ();
	      Array<octave_idx_type> iz (nc);
	      octave_idx_type *piz = iz.fortran_vec ();

	      F77_XFCN (sgetrf, SGETRF, (nr, nr, tmp_data, nr, pipvt, info));

	      if (info != 0) 
		{
		  rcon = 0.0;
		  mattype.mark_as_rectangular ();
		}
	      else 
		{
		  char job = '1';
		  F77_XFCN (sgecon, SGECON, (F77_CONST_CHAR_ARG2 (&job, 1),
					     nc, tmp_data, nr, anorm, 
					     rcon, pz, piz, info
					     F77_CHAR_ARG_LEN (1)));

		  if (info != 0) 
		    rcon = 0.0;
		}
	    }
	}
      else
	rcon = 0.0;
    }

  return rcon;
}

FloatMatrix
FloatMatrix::utsolve (MatrixType &mattype, const FloatMatrix& b, octave_idx_type& info,
		float& rcon, solve_singularity_handler sing_handler,
		bool calc_cond) const
{
  FloatMatrix retval;

  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  if (nr != b.rows ())
    (*current_liboctave_error_handler)
      ("matrix dimension mismatch solution of linear equations");
  else if (nr == 0 || nc == 0 || b.cols () == 0)
    retval = FloatMatrix (nc, b.cols (), 0.0);
  else
    {
      volatile int typ = mattype.type ();

      if (typ == MatrixType::Permuted_Upper ||
	  typ == MatrixType::Upper)
	{
	  octave_idx_type b_nc = b.cols ();
	  rcon = 1.;
	  info = 0;

	  if (typ == MatrixType::Permuted_Upper)
	    {
	      (*current_liboctave_error_handler)
		("permuted triangular matrix not implemented");
	    }
	  else
	    {
	      const float *tmp_data = fortran_vec ();

	      if (calc_cond)
		{
		  char norm = '1';
		  char uplo = 'U';
		  char dia = 'N';

		  Array<float> z (3 * nc);
		  float *pz = z.fortran_vec ();
		  Array<octave_idx_type> iz (nc);
		  octave_idx_type *piz = iz.fortran_vec ();

		  F77_XFCN (strcon, STRCON, (F77_CONST_CHAR_ARG2 (&norm, 1), 
					     F77_CONST_CHAR_ARG2 (&uplo, 1), 
					     F77_CONST_CHAR_ARG2 (&dia, 1), 
					     nr, tmp_data, nr, rcon,
					     pz, piz, info
					     F77_CHAR_ARG_LEN (1)
					     F77_CHAR_ARG_LEN (1)
					     F77_CHAR_ARG_LEN (1)));

		  if (info != 0) 
		    info = -2;

		  volatile float rcond_plus_one = rcon + 1.0;

		  if (rcond_plus_one == 1.0 || xisnan (rcon))
		    {
		      info = -2;

		      if (sing_handler)
			sing_handler (rcon);
		      else
			(*current_liboctave_error_handler)
			  ("matrix singular to machine precision, rcond = %g",
			   rcon);
		    }
		}

	      if (info == 0)
		{
		  retval = b;
		  float *result = retval.fortran_vec ();

		  char uplo = 'U';
		  char trans = 'N';
		  char dia = 'N';

		  F77_XFCN (strtrs, STRTRS, (F77_CONST_CHAR_ARG2 (&uplo, 1), 
					     F77_CONST_CHAR_ARG2 (&trans, 1), 
					     F77_CONST_CHAR_ARG2 (&dia, 1), 
					     nr, b_nc, tmp_data, nr,
					     result, nr, info
					     F77_CHAR_ARG_LEN (1)
					     F77_CHAR_ARG_LEN (1)
					     F77_CHAR_ARG_LEN (1)));
		}
	    }
	}
      else
	(*current_liboctave_error_handler) ("incorrect matrix type");
    }

  return retval;
}

FloatMatrix
FloatMatrix::ltsolve (MatrixType &mattype, const FloatMatrix& b, octave_idx_type& info,
		float& rcon, solve_singularity_handler sing_handler,
		bool calc_cond) const
{
  FloatMatrix retval;

  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  if (nr != b.rows ())
    (*current_liboctave_error_handler)
      ("matrix dimension mismatch solution of linear equations");
  else if (nr == 0 || nc == 0 || b.cols () == 0)
    retval = FloatMatrix (nc, b.cols (), 0.0);
  else
    {
      volatile int typ = mattype.type ();

      if (typ == MatrixType::Permuted_Lower ||
	  typ == MatrixType::Lower)
	{
	  octave_idx_type b_nc = b.cols ();
	  rcon = 1.;
	  info = 0;

	  if (typ == MatrixType::Permuted_Lower)
	    {
	      (*current_liboctave_error_handler)
		("permuted triangular matrix not implemented");
	    }
	  else
	    {
	      const float *tmp_data = fortran_vec ();

	      if (calc_cond)
		{
		  char norm = '1';
		  char uplo = 'L';
		  char dia = 'N';

		  Array<float> z (3 * nc);
		  float *pz = z.fortran_vec ();
		  Array<octave_idx_type> iz (nc);
		  octave_idx_type *piz = iz.fortran_vec ();

		  F77_XFCN (strcon, STRCON, (F77_CONST_CHAR_ARG2 (&norm, 1), 
					     F77_CONST_CHAR_ARG2 (&uplo, 1), 
					     F77_CONST_CHAR_ARG2 (&dia, 1), 
					     nr, tmp_data, nr, rcon,
					     pz, piz, info
					     F77_CHAR_ARG_LEN (1)
					     F77_CHAR_ARG_LEN (1)
					     F77_CHAR_ARG_LEN (1)));

		  if (info != 0) 
		    info = -2;

		  volatile float rcond_plus_one = rcon + 1.0;

		  if (rcond_plus_one == 1.0 || xisnan (rcon))
		    {
		      info = -2;

		      if (sing_handler)
			sing_handler (rcon);
		      else
			(*current_liboctave_error_handler)
			  ("matrix singular to machine precision, rcond = %g",
			   rcon);
		    }
		}

	      if (info == 0)
		{
		  retval = b;
		  float *result = retval.fortran_vec ();

		  char uplo = 'L';
		  char trans = 'N';
		  char dia = 'N';

		  F77_XFCN (strtrs, STRTRS, (F77_CONST_CHAR_ARG2 (&uplo, 1), 
					     F77_CONST_CHAR_ARG2 (&trans, 1), 
					     F77_CONST_CHAR_ARG2 (&dia, 1), 
					     nr, b_nc, tmp_data, nr,
					     result, nr, info
					     F77_CHAR_ARG_LEN (1)
					     F77_CHAR_ARG_LEN (1)
					     F77_CHAR_ARG_LEN (1)));
		}
	    }
	}
      else
	(*current_liboctave_error_handler) ("incorrect matrix type");
    }

  return retval;
}

FloatMatrix
FloatMatrix::fsolve (MatrixType &mattype, const FloatMatrix& b, octave_idx_type& info,
		float& rcon, solve_singularity_handler sing_handler,
		bool calc_cond) const
{
  FloatMatrix retval;

  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  if (nr != nc || nr != b.rows ())
    (*current_liboctave_error_handler)
      ("matrix dimension mismatch solution of linear equations");
  else if (nr == 0 || b.cols () == 0)
    retval = FloatMatrix (nc, b.cols (), 0.0);
  else
    {
      volatile int typ = mattype.type ();
 
     // Calculate the norm of the matrix, for later use.
      float anorm = -1.;

      if (typ == MatrixType::Hermitian)
	{
	  info = 0;
	  char job = 'L';
	  FloatMatrix atmp = *this;
	  float *tmp_data = atmp.fortran_vec ();
	  anorm = atmp.abs().sum().row(static_cast<octave_idx_type>(0)).max();

	  F77_XFCN (spotrf, SPOTRF, (F77_CONST_CHAR_ARG2 (&job, 1), nr, 
				     tmp_data, nr, info
				     F77_CHAR_ARG_LEN (1)));

	  // Throw-away extra info LAPACK gives so as to not change output.
	  rcon = 0.0;
	  if (info != 0) 
	    {
	      info = -2;

	      mattype.mark_as_unsymmetric ();
	      typ = MatrixType::Full;
	    }
	  else 
	    {
	      if (calc_cond)
		{
		  Array<float> z (3 * nc);
		  float *pz = z.fortran_vec ();
		  Array<octave_idx_type> iz (nc);
		  octave_idx_type *piz = iz.fortran_vec ();

		  F77_XFCN (spocon, SPOCON, (F77_CONST_CHAR_ARG2 (&job, 1),
					     nr, tmp_data, nr, anorm,
					     rcon, pz, piz, info
					     F77_CHAR_ARG_LEN (1)));

		  if (info != 0) 
		    info = -2;

		  volatile float rcond_plus_one = rcon + 1.0;

		  if (rcond_plus_one == 1.0 || xisnan (rcon))
		    {
		      info = -2;

		      if (sing_handler)
			sing_handler (rcon);
		      else
			(*current_liboctave_error_handler)
			  ("matrix singular to machine precision, rcond = %g",
			   rcon);
		    }
		}

	      if (info == 0)
		{
		  retval = b;
		  float *result = retval.fortran_vec ();

		  octave_idx_type b_nc = b.cols ();

		  F77_XFCN (spotrs, SPOTRS, (F77_CONST_CHAR_ARG2 (&job, 1),
					     nr, b_nc, tmp_data, nr,
					     result, b.rows(), info
					     F77_CHAR_ARG_LEN (1)));
		}
	      else
		{
		  mattype.mark_as_unsymmetric ();
		  typ = MatrixType::Full;
		}		    
	    }
	}

      if (typ == MatrixType::Full)
	{
	  info = 0;

	  Array<octave_idx_type> ipvt (nr);
	  octave_idx_type *pipvt = ipvt.fortran_vec ();

	  FloatMatrix atmp = *this;
	  float *tmp_data = atmp.fortran_vec ();
	  if(anorm < 0.)
	    anorm = atmp.abs().sum().row(static_cast<octave_idx_type>(0)).max();

	  Array<float> z (4 * nc);
	  float *pz = z.fortran_vec ();
	  Array<octave_idx_type> iz (nc);
	  octave_idx_type *piz = iz.fortran_vec ();

	  F77_XFCN (sgetrf, SGETRF, (nr, nr, tmp_data, nr, pipvt, info));

	  // Throw-away extra info LAPACK gives so as to not change output.
	  rcon = 0.0;
	  if (info != 0) 
	    {
	      info = -2;

	      if (sing_handler)
		sing_handler (rcon);
	      else
		(*current_liboctave_error_handler)
		  ("matrix singular to machine precision");

	      mattype.mark_as_rectangular ();
	    }
	  else 
	    {
	      if (calc_cond)
		{
		  // Now calculate the condition number for 
		  // non-singular matrix.
		  char job = '1';
		  F77_XFCN (sgecon, SGECON, (F77_CONST_CHAR_ARG2 (&job, 1),
					     nc, tmp_data, nr, anorm, 
					     rcon, pz, piz, info
					     F77_CHAR_ARG_LEN (1)));

		  if (info != 0) 
		    info = -2;

		  volatile float rcond_plus_one = rcon + 1.0;

		  if (rcond_plus_one == 1.0 || xisnan (rcon))
		    {
		      info = -2;

		      if (sing_handler)
			sing_handler (rcon);
		      else
			(*current_liboctave_error_handler)
			  ("matrix singular to machine precision, rcond = %g",
			   rcon);
		    }
		}

	      if (info == 0)
		{
		  retval = b;
		  float *result = retval.fortran_vec ();

		  octave_idx_type b_nc = b.cols ();

		  char job = 'N';
		  F77_XFCN (sgetrs, SGETRS, (F77_CONST_CHAR_ARG2 (&job, 1),
					     nr, b_nc, tmp_data, nr,
					     pipvt, result, b.rows(), info
					     F77_CHAR_ARG_LEN (1)));
		}
	      else
		mattype.mark_as_rectangular ();
	    }
	}
      else if (typ != MatrixType::Hermitian)
	(*current_liboctave_error_handler) ("incorrect matrix type");
    }

  return retval;
}

FloatMatrix
FloatMatrix::solve (MatrixType &typ, const FloatMatrix& b) const
{
  octave_idx_type info;
  float rcon;
  return solve (typ, b, info, rcon, 0);
}

FloatMatrix
FloatMatrix::solve (MatrixType &typ, const FloatMatrix& b, octave_idx_type& info, 
	       float& rcon) const
{
  return solve (typ, b, info, rcon, 0);
}

FloatMatrix
FloatMatrix::solve (MatrixType &mattype, const FloatMatrix& b, octave_idx_type& info,
	       float& rcon, solve_singularity_handler sing_handler,
	       bool singular_fallback) const
{
  FloatMatrix retval;
  int typ = mattype.type ();

  if (typ == MatrixType::Unknown)
    typ = mattype.type (*this);

  // Only calculate the condition number for LU/Cholesky
  if (typ == MatrixType::Upper || typ == MatrixType::Permuted_Upper)
    retval = utsolve (mattype, b, info, rcon, sing_handler, false);
  else if (typ == MatrixType::Lower || typ == MatrixType::Permuted_Lower)
    retval = ltsolve (mattype, b, info, rcon, sing_handler, false);
  else if (typ == MatrixType::Full || typ == MatrixType::Hermitian)
    retval = fsolve (mattype, b, info, rcon, sing_handler, true);
  else if (typ != MatrixType::Rectangular)
    {
      (*current_liboctave_error_handler) ("unknown matrix type");
      return FloatMatrix ();
    }

  // Rectangular or one of the above solvers flags a singular matrix
  if (singular_fallback && mattype.type () == MatrixType::Rectangular)
    {
      octave_idx_type rank;
      retval = lssolve (b, info, rank, rcon);
    }

  return retval;
}

FloatComplexMatrix
FloatMatrix::solve (MatrixType &typ, const FloatComplexMatrix& b) const
{
  FloatComplexMatrix tmp (*this);
  return tmp.solve (typ, b);
}

FloatComplexMatrix
FloatMatrix::solve (MatrixType &typ, const FloatComplexMatrix& b, 
  octave_idx_type& info) const
{
  FloatComplexMatrix tmp (*this);
  return tmp.solve (typ, b, info);
}

FloatComplexMatrix
FloatMatrix::solve (MatrixType &typ, const FloatComplexMatrix& b, octave_idx_type& info,
	       float& rcon) const
{
  FloatComplexMatrix tmp (*this);
  return tmp.solve (typ, b, info, rcon);
}

FloatComplexMatrix
FloatMatrix::solve (MatrixType &typ, const FloatComplexMatrix& b, octave_idx_type& info,
	       float& rcon, solve_singularity_handler sing_handler,
	       bool singular_fallback) const
{
  FloatComplexMatrix tmp (*this);
  return tmp.solve (typ, b, info, rcon, sing_handler, singular_fallback);
}

FloatColumnVector
FloatMatrix::solve (MatrixType &typ, const FloatColumnVector& b) const
{
  octave_idx_type info; float rcon;
  return solve (typ, b, info, rcon);
}

FloatColumnVector
FloatMatrix::solve (MatrixType &typ, const FloatColumnVector& b, 
	       octave_idx_type& info) const
{
  float rcon;
  return solve (typ, b, info, rcon);
}

FloatColumnVector
FloatMatrix::solve (MatrixType &typ, const FloatColumnVector& b, octave_idx_type& info,
	       float& rcon) const
{
  return solve (typ, b, info, rcon, 0);
}

FloatColumnVector
FloatMatrix::solve (MatrixType &typ, const FloatColumnVector& b, octave_idx_type& info,
	       float& rcon, solve_singularity_handler sing_handler) const
{
  FloatMatrix tmp (b);
  return solve (typ, tmp, info, rcon, sing_handler).column(static_cast<octave_idx_type> (0));
}

FloatComplexColumnVector
FloatMatrix::solve (MatrixType &typ, const FloatComplexColumnVector& b) const
{
  FloatComplexMatrix tmp (*this);
  return tmp.solve (typ, b);
}

FloatComplexColumnVector
FloatMatrix::solve (MatrixType &typ, const FloatComplexColumnVector& b, 
	       octave_idx_type& info) const
{
  FloatComplexMatrix tmp (*this);
  return tmp.solve (typ, b, info);
}

FloatComplexColumnVector
FloatMatrix::solve (MatrixType &typ, const FloatComplexColumnVector& b, 
	       octave_idx_type& info, float& rcon) const
{
  FloatComplexMatrix tmp (*this);
  return tmp.solve (typ, b, info, rcon);
}

FloatComplexColumnVector
FloatMatrix::solve (MatrixType &typ, const FloatComplexColumnVector& b, 
	       octave_idx_type& info, float& rcon,
	       solve_singularity_handler sing_handler) const
{
  FloatComplexMatrix tmp (*this);
  return tmp.solve(typ, b, info, rcon, sing_handler);
}

FloatMatrix
FloatMatrix::solve (const FloatMatrix& b) const
{
  octave_idx_type info;
  float rcon;
  return solve (b, info, rcon, 0);
}

FloatMatrix
FloatMatrix::solve (const FloatMatrix& b, octave_idx_type& info) const
{
  float rcon;
  return solve (b, info, rcon, 0);
}

FloatMatrix
FloatMatrix::solve (const FloatMatrix& b, octave_idx_type& info, float& rcon) const
{
  return solve (b, info, rcon, 0);
}

FloatMatrix
FloatMatrix::solve (const FloatMatrix& b, octave_idx_type& info,
	       float& rcon, solve_singularity_handler sing_handler) const
{
  MatrixType mattype (*this);
  return solve (mattype, b, info, rcon, sing_handler);
}

FloatComplexMatrix
FloatMatrix::solve (const FloatComplexMatrix& b) const
{
  FloatComplexMatrix tmp (*this);
  return tmp.solve (b);
}

FloatComplexMatrix
FloatMatrix::solve (const FloatComplexMatrix& b, octave_idx_type& info) const
{
  FloatComplexMatrix tmp (*this);
  return tmp.solve (b, info);
}

FloatComplexMatrix
FloatMatrix::solve (const FloatComplexMatrix& b, octave_idx_type& info, float& rcon) const
{
  FloatComplexMatrix tmp (*this);
  return tmp.solve (b, info, rcon);
}

FloatComplexMatrix
FloatMatrix::solve (const FloatComplexMatrix& b, octave_idx_type& info, float& rcon,
	       solve_singularity_handler sing_handler) const
{
  FloatComplexMatrix tmp (*this);
  return tmp.solve (b, info, rcon, sing_handler);
}

FloatColumnVector
FloatMatrix::solve (const FloatColumnVector& b) const
{
  octave_idx_type info; float rcon;
  return solve (b, info, rcon);
}

FloatColumnVector
FloatMatrix::solve (const FloatColumnVector& b, octave_idx_type& info) const
{
  float rcon;
  return solve (b, info, rcon);
}

FloatColumnVector
FloatMatrix::solve (const FloatColumnVector& b, octave_idx_type& info, float& rcon) const
{
  return solve (b, info, rcon, 0);
}

FloatColumnVector
FloatMatrix::solve (const FloatColumnVector& b, octave_idx_type& info, float& rcon,
	       solve_singularity_handler sing_handler) const
{
  MatrixType mattype (*this);
  return solve (mattype, b, info, rcon, sing_handler);
}

FloatComplexColumnVector
FloatMatrix::solve (const FloatComplexColumnVector& b) const
{
  FloatComplexMatrix tmp (*this);
  return tmp.solve (b);
}

FloatComplexColumnVector
FloatMatrix::solve (const FloatComplexColumnVector& b, octave_idx_type& info) const
{
  FloatComplexMatrix tmp (*this);
  return tmp.solve (b, info);
}

FloatComplexColumnVector
FloatMatrix::solve (const FloatComplexColumnVector& b, octave_idx_type& info, float& rcon) const
{
  FloatComplexMatrix tmp (*this);
  return tmp.solve (b, info, rcon);
}

FloatComplexColumnVector
FloatMatrix::solve (const FloatComplexColumnVector& b, octave_idx_type& info, float& rcon,
	       solve_singularity_handler sing_handler) const
{
  FloatComplexMatrix tmp (*this);
  return tmp.solve (b, info, rcon, sing_handler);
}

FloatMatrix
FloatMatrix::lssolve (const FloatMatrix& b) const
{
  octave_idx_type info;
  octave_idx_type rank;
  float rcon;
  return lssolve (b, info, rank, rcon);
}

FloatMatrix
FloatMatrix::lssolve (const FloatMatrix& b, octave_idx_type& info) const
{
  octave_idx_type rank;
  float rcon;
  return lssolve (b, info, rank, rcon);
}

FloatMatrix
FloatMatrix::lssolve (const FloatMatrix& b, octave_idx_type& info,
		 octave_idx_type& rank) const
{
  float rcon;
  return lssolve (b, info, rank, rcon);
}

FloatMatrix
FloatMatrix::lssolve (const FloatMatrix& b, octave_idx_type& info,
		 octave_idx_type& rank, float &rcon) const
{
  FloatMatrix retval;

  octave_idx_type nrhs = b.cols ();

  octave_idx_type m = rows ();
  octave_idx_type n = cols ();

  if (m != b.rows ())
    (*current_liboctave_error_handler)
      ("matrix dimension mismatch solution of linear equations");
  else if (m == 0 || n == 0 || b.cols () == 0)
    retval = FloatMatrix (n, b.cols (), 0.0);
  else
    {
      volatile octave_idx_type minmn = (m < n ? m : n);
      octave_idx_type maxmn = m > n ? m : n;
      rcon = -1.0;
      if (m != n)
	{
	  retval = FloatMatrix (maxmn, nrhs, 0.0);

	  for (octave_idx_type j = 0; j < nrhs; j++)
	    for (octave_idx_type i = 0; i < m; i++)
	      retval.elem (i, j) = b.elem (i, j);
	}
      else
	retval = b;

      FloatMatrix atmp = *this;
      float *tmp_data = atmp.fortran_vec ();

      float *pretval = retval.fortran_vec ();
      Array<float> s (minmn);
      float *ps = s.fortran_vec ();

      // Ask DGELSD what the dimension of WORK should be.
      octave_idx_type lwork = -1;

      Array<float> work (1);

      octave_idx_type smlsiz;
      F77_FUNC (xilaenv, XILAENV) (9, F77_CONST_CHAR_ARG2 ("SGELSD", 6),
				   F77_CONST_CHAR_ARG2 (" ", 1),
				   0, 0, 0, 0, smlsiz
				   F77_CHAR_ARG_LEN (6)
				   F77_CHAR_ARG_LEN (1));

      octave_idx_type mnthr;
      F77_FUNC (xilaenv, XILAENV) (6, F77_CONST_CHAR_ARG2 ("SGELSD", 6),
				   F77_CONST_CHAR_ARG2 (" ", 1),
				   m, n, nrhs, -1, mnthr
				   F77_CHAR_ARG_LEN (6)
				   F77_CHAR_ARG_LEN (1));

      // We compute the size of iwork because DGELSD in older versions
      // of LAPACK does not return it on a query call.
      float dminmn = static_cast<float> (minmn);
      float dsmlsizp1 = static_cast<float> (smlsiz+1);
#if defined (HAVE_LOG2)
      float tmp = log2 (dminmn / dsmlsizp1);
#else
      float tmp = log (dminmn / dsmlsizp1) / log (2.0);
#endif
      octave_idx_type nlvl = static_cast<octave_idx_type> (tmp) + 1;
      if (nlvl < 0)
	nlvl = 0;

      octave_idx_type liwork = 3 * minmn * nlvl + 11 * minmn;
      if (liwork < 1)
	liwork = 1;
      Array<octave_idx_type> iwork (liwork);
      octave_idx_type* piwork = iwork.fortran_vec ();

      F77_XFCN (sgelsd, SGELSD, (m, n, nrhs, tmp_data, m, pretval, maxmn,
				 ps, rcon, rank, work.fortran_vec (),
				 lwork, piwork, info));

      // The workspace query is broken in at least LAPACK 3.0.0
      // through 3.1.1 when n >= mnthr.  The obtuse formula below
      // should provide sufficient workspace for DGELSD to operate
      // efficiently.
      if (n >= mnthr)
	{
	  const octave_idx_type wlalsd
	    = 9*m + 2*m*smlsiz + 8*m*nlvl + m*nrhs + (smlsiz+1)*(smlsiz+1);

	  octave_idx_type addend = m;

	  if (2*m-4 > addend)
	    addend = 2*m-4;

	  if (nrhs > addend)
	    addend = nrhs;

	  if (n-3*m > addend)
	    addend = n-3*m;

	  if (wlalsd > addend)
	    addend = wlalsd;

	  const octave_idx_type lworkaround = 4*m + m*m + addend;

	  if (work(0) < lworkaround)
	    work(0) = lworkaround;
	}
      else if (m >= n)
	{
	  octave_idx_type lworkaround
	    = 12*n + 2*n*smlsiz + 8*n*nlvl + n*nrhs + (smlsiz+1)*(smlsiz+1);

	  if (work(0) < lworkaround)
	    work(0) = lworkaround;
	}

      lwork = static_cast<octave_idx_type> (work(0));
      work.resize (lwork);

      F77_XFCN (sgelsd, SGELSD, (m, n, nrhs, tmp_data, m, pretval,
				 maxmn, ps, rcon, rank,
				 work.fortran_vec (), lwork, 
				 piwork, info));

      if (rank < minmn)
	(*current_liboctave_warning_handler) 
	  ("dgelsd: rank deficient %dx%d matrix, rank = %d", m, n, rank);
      if (s.elem (0) == 0.0)
	rcon = 0.0;
      else
	rcon = s.elem (minmn - 1) / s.elem (0);

      retval.resize (n, nrhs);
    }

  return retval;
}

FloatComplexMatrix
FloatMatrix::lssolve (const FloatComplexMatrix& b) const
{
  FloatComplexMatrix tmp (*this);
  octave_idx_type info;
  octave_idx_type rank;
  float rcon;
  return tmp.lssolve (b, info, rank, rcon);
}

FloatComplexMatrix
FloatMatrix::lssolve (const FloatComplexMatrix& b, octave_idx_type& info) const
{
  FloatComplexMatrix tmp (*this);
  octave_idx_type rank;
  float rcon;
  return tmp.lssolve (b, info, rank, rcon);
}

FloatComplexMatrix
FloatMatrix::lssolve (const FloatComplexMatrix& b, octave_idx_type& info, 
		 octave_idx_type& rank) const
{
  FloatComplexMatrix tmp (*this);
  float rcon;
  return tmp.lssolve (b, info, rank, rcon);
}

FloatComplexMatrix
FloatMatrix::lssolve (const FloatComplexMatrix& b, octave_idx_type& info, 
		 octave_idx_type& rank, float& rcon) const
{
  FloatComplexMatrix tmp (*this);
  return tmp.lssolve (b, info, rank, rcon);
}

FloatColumnVector
FloatMatrix::lssolve (const FloatColumnVector& b) const
{
  octave_idx_type info;
  octave_idx_type rank;
  float rcon;
  return lssolve (b, info, rank, rcon);
}

FloatColumnVector
FloatMatrix::lssolve (const FloatColumnVector& b, octave_idx_type& info) const
{
  octave_idx_type rank;
  float rcon;
  return lssolve (b, info, rank, rcon);
}

FloatColumnVector
FloatMatrix::lssolve (const FloatColumnVector& b, octave_idx_type& info,
		 octave_idx_type& rank) const
{
  float rcon;
  return lssolve (b, info, rank, rcon);
}

FloatColumnVector
FloatMatrix::lssolve (const FloatColumnVector& b, octave_idx_type& info,
		 octave_idx_type& rank, float &rcon) const
{
  FloatColumnVector retval;

  octave_idx_type nrhs = 1;

  octave_idx_type m = rows ();
  octave_idx_type n = cols ();

  if (m != b.length ())
    (*current_liboctave_error_handler)
      ("matrix dimension mismatch solution of linear equations");
  else if (m == 0 || n == 0)
    retval = FloatColumnVector (n, 0.0);
  else
    {
      volatile octave_idx_type minmn = (m < n ? m : n);
      octave_idx_type maxmn = m > n ? m : n;
      rcon = -1.0;
 
      if (m != n)
	{
	  retval = FloatColumnVector (maxmn, 0.0);

	  for (octave_idx_type i = 0; i < m; i++)
	    retval.elem (i) = b.elem (i);
	}
      else
	retval = b;

      FloatMatrix atmp = *this;
      float *tmp_data = atmp.fortran_vec ();

      float *pretval = retval.fortran_vec ();
      Array<float> s (minmn);
      float *ps = s.fortran_vec ();

      // Ask DGELSD what the dimension of WORK should be.
      octave_idx_type lwork = -1;

      Array<float> work (1);

      octave_idx_type smlsiz;
      F77_FUNC (xilaenv, XILAENV) (9, F77_CONST_CHAR_ARG2 ("SGELSD", 6),
				   F77_CONST_CHAR_ARG2 (" ", 1),
				   0, 0, 0, 0, smlsiz
				   F77_CHAR_ARG_LEN (6)
				   F77_CHAR_ARG_LEN (1));

      // We compute the size of iwork because DGELSD in older versions
      // of LAPACK does not return it on a query call.
      float dminmn = static_cast<float> (minmn);
      float dsmlsizp1 = static_cast<float> (smlsiz+1);
#if defined (HAVE_LOG2)
      float tmp = log2 (dminmn / dsmlsizp1);
#else
      float tmp = log (dminmn / dsmlsizp1) / log (2.0);
#endif
      octave_idx_type nlvl = static_cast<octave_idx_type> (tmp) + 1;
      if (nlvl < 0)
	nlvl = 0;

      octave_idx_type liwork = 3 * minmn * nlvl + 11 * minmn;
      if (liwork < 1)
	liwork = 1;
      Array<octave_idx_type> iwork (liwork);
      octave_idx_type* piwork = iwork.fortran_vec ();

      F77_XFCN (sgelsd, SGELSD, (m, n, nrhs, tmp_data, m, pretval, maxmn,
				 ps, rcon, rank, work.fortran_vec (),
				 lwork, piwork, info));

      lwork = static_cast<octave_idx_type> (work(0));
      work.resize (lwork);

      F77_XFCN (sgelsd, SGELSD, (m, n, nrhs, tmp_data, m, pretval,
				 maxmn, ps, rcon, rank,
				 work.fortran_vec (), lwork, 
				 piwork, info));

      if (rank < minmn)
	{
	  if (rank < minmn)
	    (*current_liboctave_warning_handler) 
	      ("dgelsd: rank deficient %dx%d matrix, rank = %d", m, n, rank);
	  if (s.elem (0) == 0.0)
	    rcon = 0.0;
	  else
	    rcon = s.elem (minmn - 1) / s.elem (0);
	}

      retval.resize (n, nrhs);
    }

  return retval;
}

FloatComplexColumnVector
FloatMatrix::lssolve (const FloatComplexColumnVector& b) const
{
  FloatComplexMatrix tmp (*this);
  octave_idx_type info;
  octave_idx_type rank;
  float rcon;
  return tmp.lssolve (b, info, rank, rcon);
}

FloatComplexColumnVector
FloatMatrix::lssolve (const FloatComplexColumnVector& b, octave_idx_type& info) const
{
  FloatComplexMatrix tmp (*this);
  octave_idx_type rank;
  float rcon;
  return tmp.lssolve (b, info, rank, rcon);
}

FloatComplexColumnVector
FloatMatrix::lssolve (const FloatComplexColumnVector& b, octave_idx_type& info, 
		 octave_idx_type& rank) const
{
  FloatComplexMatrix tmp (*this);
  float rcon;
  return tmp.lssolve (b, info, rank, rcon);
}

FloatComplexColumnVector
FloatMatrix::lssolve (const FloatComplexColumnVector& b, octave_idx_type& info, 
		 octave_idx_type& rank, float &rcon) const
{
  FloatComplexMatrix tmp (*this);
  return tmp.lssolve (b, info, rank, rcon);
}

// Constants for matrix exponential calculation.

static float padec [] =
{
  5.0000000000000000e-1,
  1.1666666666666667e-1,
  1.6666666666666667e-2,
  1.6025641025641026e-3,
  1.0683760683760684e-4,
  4.8562548562548563e-6,
  1.3875013875013875e-7,
  1.9270852604185938e-9,
};

static void
solve_singularity_warning (float rcon)
{
  (*current_liboctave_warning_handler) 
    ("singular matrix encountered in expm calculation, rcond = %g",
     rcon);
}

FloatMatrix
FloatMatrix::expm (void) const
{
  FloatMatrix retval;

  FloatMatrix m = *this;

  if (numel () == 1)
    return FloatMatrix (1, 1, exp (m(0)));

  octave_idx_type nc = columns ();

  // Preconditioning step 1: trace normalization to reduce dynamic
  // range of poles, but avoid making stable eigenvalues unstable.

  // trace shift value
  volatile float trshift = 0.0;

  for (octave_idx_type i = 0; i < nc; i++)
    trshift += m.elem (i, i);

  trshift /= nc;

  if (trshift > 0.0)
    {
      for (octave_idx_type i = 0; i < nc; i++)
	m.elem (i, i) -= trshift;
    }

  // Preconditioning step 2: balancing; code follows development
  // in AEPBAL

  float *p_m = m.fortran_vec ();

  octave_idx_type info, ilo, ihi, ilos, ihis;
  Array<float> dpermute (nc);
  Array<float> dscale (nc);

  // permutation first
  char job = 'P';
  F77_XFCN (sgebal, SGEBAL, (F77_CONST_CHAR_ARG2 (&job, 1),
			     nc, p_m, nc, ilo, ihi,
			     dpermute.fortran_vec (), info
			     F77_CHAR_ARG_LEN (1)));

  // then scaling
  job = 'S';
  F77_XFCN (sgebal, SGEBAL, (F77_CONST_CHAR_ARG2 (&job, 1),
			     nc, p_m, nc, ilos, ihis,
			     dscale.fortran_vec (), info
			     F77_CHAR_ARG_LEN (1)));

  // Preconditioning step 3: scaling.
  
  FloatColumnVector work(nc);
  float inf_norm;
  
  F77_XFCN (xslange, XSLANGE, (F77_CONST_CHAR_ARG2 ("I", 1),
			       nc, nc, m.fortran_vec (), nc,
			       work.fortran_vec (), inf_norm
			       F77_CHAR_ARG_LEN (1)));
  
  octave_idx_type sqpow = static_cast<octave_idx_type> (inf_norm > 0.0
		     ? (1.0 + log (inf_norm) / log (2.0))
		     : 0.0);
  
  // Check whether we need to square at all.
  
  if (sqpow < 0)
    sqpow = 0;
  
  if (sqpow > 0)
    {
      if (sqpow > 1023)
	sqpow = 1023;

      float scale_factor = 1.0;
      for (octave_idx_type i = 0; i < sqpow; i++)
	scale_factor *= 2.0;

      m = m / scale_factor;
    }
  
  // npp, dpp: pade' approx polynomial matrices.
  
  FloatMatrix npp (nc, nc, 0.0);
  float *pnpp = npp.fortran_vec ();
  FloatMatrix dpp = npp;
  float *pdpp = dpp.fortran_vec ();
  
  // Now powers a^8 ... a^1.
  
  octave_idx_type minus_one_j = -1;
  for (octave_idx_type j = 7; j >= 0; j--)
    {
      for (octave_idx_type i = 0; i < nc; i++)
	{
	  octave_idx_type k = i * nc + i;
	  pnpp[k] += padec[j];
	  pdpp[k] += minus_one_j * padec[j];
	}      

      npp = m * npp;
      pnpp = npp.fortran_vec ();

      dpp = m * dpp;
      pdpp = dpp.fortran_vec ();

      minus_one_j *= -1;
    }
  
  // Zero power.
  
  dpp = -dpp;
  for (octave_idx_type j = 0; j < nc; j++)
    {
      npp.elem (j, j) += 1.0;
      dpp.elem (j, j) += 1.0;
    }
  
  // Compute pade approximation = inverse (dpp) * npp.

  float rcon;
  retval = dpp.solve (npp, info, rcon, solve_singularity_warning);

  if (info < 0)
    return retval;

  // Reverse preconditioning step 3: repeated squaring.
  
  while (sqpow)
    {
      retval = retval * retval;
      sqpow--;
    }
  
  // Reverse preconditioning step 2: inverse balancing.
  // apply inverse scaling to computed exponential
  for (octave_idx_type i = 0; i < nc; i++)
    for (octave_idx_type j = 0; j < nc; j++)
      retval(i,j) *= dscale(i) / dscale(j);

  OCTAVE_QUIT;

  // construct balancing permutation vector
  Array<octave_idx_type> iperm (nc);
  for (octave_idx_type i = 0; i < nc; i++)
    iperm(i) = i;  // identity permutation

  // trailing permutations must be done in reverse order
  for (octave_idx_type i = nc - 1; i >= ihi; i--)
    {
      octave_idx_type swapidx = static_cast<octave_idx_type> (dpermute(i)) - 1;
      octave_idx_type tmp = iperm(i);
      iperm(i) = iperm(swapidx);
      iperm(swapidx) = tmp;
    }

  // leading permutations in forward order
  for (octave_idx_type i = 0; i < (ilo-1); i++)
    {
      octave_idx_type swapidx = static_cast<octave_idx_type> (dpermute(i)) - 1;
      octave_idx_type tmp = iperm(i);
      iperm(i) = iperm (swapidx);
      iperm(swapidx) = tmp;
    }

  // construct inverse balancing permutation vector
  Array<octave_idx_type> invpvec (nc);
  for (octave_idx_type i = 0; i < nc; i++)
    invpvec(iperm(i)) = i;     // Thanks to R. A. Lippert for this method

  OCTAVE_QUIT;

  FloatMatrix tmpMat = retval;
  for (octave_idx_type i = 0; i < nc; i++)
    for (octave_idx_type j = 0; j < nc; j++)
      retval(i,j) = tmpMat(invpvec(i),invpvec(j));

  // Reverse preconditioning step 1: fix trace normalization. 
  
  if (trshift > 0.0)
    retval = expf (trshift) * retval;

  return retval;
}

FloatMatrix&
FloatMatrix::operator += (const FloatDiagMatrix& a)
{
  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  octave_idx_type a_nr = a.rows ();
  octave_idx_type a_nc = a.cols ();

  if (nr != a_nr || nc != a_nc)
    {
      gripe_nonconformant ("operator +=", nr, nc, a_nr, a_nc);
      return *this;
    }

  for (octave_idx_type i = 0; i < a.length (); i++)
    elem (i, i) += a.elem (i, i);

  return *this;
}

FloatMatrix&
FloatMatrix::operator -= (const FloatDiagMatrix& a)
{
  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  octave_idx_type a_nr = a.rows ();
  octave_idx_type a_nc = a.cols ();

  if (nr != a_nr || nc != a_nc)
    {
      gripe_nonconformant ("operator -=", nr, nc, a_nr, a_nc);
      return *this;
    }

  for (octave_idx_type i = 0; i < a.length (); i++)
    elem (i, i) -= a.elem (i, i);

  return *this;
}

// unary operations

boolMatrix
FloatMatrix::operator ! (void) const
{
  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  boolMatrix b (nr, nc);

  for (octave_idx_type j = 0; j < nc; j++)
    for (octave_idx_type i = 0; i < nr; i++)
      b.elem (i, j) = ! elem (i, j);

  return b;
}

// column vector by row vector -> matrix operations

FloatMatrix
operator * (const FloatColumnVector& v, const FloatRowVector& a)
{
  FloatMatrix retval;

  octave_idx_type len = v.length ();

  if (len != 0)
    {
      octave_idx_type a_len = a.length ();

      retval.resize (len, a_len);
      float *c = retval.fortran_vec ();
	  
      F77_XFCN (sgemm, SGEMM, (F77_CONST_CHAR_ARG2 ("N", 1),
			       F77_CONST_CHAR_ARG2 ("N", 1),
			       len, a_len, 1, 1.0, v.data (), len,
			       a.data (), 1, 0.0, c, len
			       F77_CHAR_ARG_LEN (1)
			       F77_CHAR_ARG_LEN (1)));
    }

  return retval;
}

// other operations.

FloatMatrix
FloatMatrix::map (dmapper fcn) const
{
  return MArray2<float>::map<float> (func_ptr (fcn));
}

FloatComplexMatrix
FloatMatrix::map (cmapper fcn) const
{
  return MArray2<float>::map<FloatComplex> (func_ptr (fcn));
}

boolMatrix
FloatMatrix::map (bmapper fcn) const
{
  return MArray2<float>::map<bool> (func_ptr (fcn));
}

bool
FloatMatrix::any_element_is_negative (bool neg_zero) const
{
  octave_idx_type nel = nelem ();

  if (neg_zero)
    {
      for (octave_idx_type i = 0; i < nel; i++)
	if (lo_ieee_signbit (elem (i)))
	  return true;
    }
  else
    {
      for (octave_idx_type i = 0; i < nel; i++)
	if (elem (i) < 0)
	  return true;
    }

  return false;
}

bool
FloatMatrix::any_element_is_nan (void) const
{
  octave_idx_type nel = nelem ();

  for (octave_idx_type i = 0; i < nel; i++)
    {
      float val = elem (i);
      if (xisnan (val))
	return true;
    }

  return false;
}

bool
FloatMatrix::any_element_is_inf_or_nan (void) const
{
  octave_idx_type nel = nelem ();

  for (octave_idx_type i = 0; i < nel; i++)
    {
      float val = elem (i);
      if (xisinf (val) || xisnan (val))
	return true;
    }

  return false;
}

bool
FloatMatrix::any_element_not_one_or_zero (void) const
{
  octave_idx_type nel = nelem ();

  for (octave_idx_type i = 0; i < nel; i++)
    {
      float val = elem (i);
      if (val != 0 && val != 1)
	return true;
    }

  return false;
}

bool
FloatMatrix::all_elements_are_int_or_inf_or_nan (void) const
{
  octave_idx_type nel = nelem ();

  for (octave_idx_type i = 0; i < nel; i++)
    {
      float val = elem (i);
      if (xisnan (val) || D_NINT (val) == val)
	continue;
      else
	return false;
    }

  return true;
}

// Return nonzero if any element of M is not an integer.  Also extract
// the largest and smallest values and return them in MAX_VAL and MIN_VAL.

bool
FloatMatrix::all_integers (float& max_val, float& min_val) const
{
  octave_idx_type nel = nelem ();

  if (nel > 0)
    {
      max_val = elem (0);
      min_val = elem (0);
    }
  else
    return false;

  for (octave_idx_type i = 0; i < nel; i++)
    {
      float val = elem (i);

      if (val > max_val)
	max_val = val;

      if (val < min_val)
	min_val = val;

      if (D_NINT (val) != val)
	return false;
    }

  return true;
}

bool
FloatMatrix::too_large_for_float (void) const
{
  octave_idx_type nel = nelem ();

  for (octave_idx_type i = 0; i < nel; i++)
    {
      float val = elem (i);

      if (! (xisnan (val) || xisinf (val))
	  && fabs (val) > FLT_MAX)
	return true;
    }

  return false;
}

// FIXME Do these really belong here?  Maybe they should be
// in a base class?

boolMatrix
FloatMatrix::all (int dim) const
{
  MX_ALL_OP (dim);
}

boolMatrix
FloatMatrix::any (int dim) const
{
  MX_ANY_OP (dim);
}

FloatMatrix
FloatMatrix::cumprod (int dim) const
{
  MX_CUMULATIVE_OP (FloatMatrix, float, *=);
}

FloatMatrix
FloatMatrix::cumsum (int dim) const
{
  MX_CUMULATIVE_OP (FloatMatrix, float, +=);
}

FloatMatrix
FloatMatrix::prod (int dim) const
{
  MX_REDUCTION_OP (FloatMatrix, *=, 1.0, 1.0);
}

FloatMatrix
FloatMatrix::sum (int dim) const
{
  MX_REDUCTION_OP (FloatMatrix, +=, 0.0, 0.0);
}

FloatMatrix
FloatMatrix::sumsq (int dim) const
{
#define ROW_EXPR \
  float d = elem (i, j); \
  retval.elem (i, 0) += d * d

#define COL_EXPR \
  float d = elem (i, j); \
  retval.elem (0, j) += d * d

  MX_BASE_REDUCTION_OP (FloatMatrix, ROW_EXPR, COL_EXPR, 0.0, 0.0);

#undef ROW_EXPR
#undef COL_EXPR
}

FloatMatrix
FloatMatrix::abs (void) const
{
  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  FloatMatrix retval (nr, nc);

  for (octave_idx_type j = 0; j < nc; j++)
    for (octave_idx_type i = 0; i < nr; i++)
      retval (i, j) = fabs (elem (i, j));

  return retval;
}

FloatMatrix
FloatMatrix::diag (octave_idx_type k) const
{
  return MArray2<float>::diag (k);
}

FloatColumnVector
FloatMatrix::row_min (void) const
{
  Array<octave_idx_type> dummy_idx;
  return row_min (dummy_idx);
}

FloatColumnVector
FloatMatrix::row_min (Array<octave_idx_type>& idx_arg) const
{
  FloatColumnVector result;

  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  if (nr > 0 && nc > 0)
    {
      result.resize (nr);
      idx_arg.resize (nr);

      for (octave_idx_type i = 0; i < nr; i++)
        {
	  octave_idx_type idx_j;

	  float tmp_min = octave_Float_NaN;

	  for (idx_j = 0; idx_j < nc; idx_j++)
	    {
	      tmp_min = elem (i, idx_j);

	      if (! xisnan (tmp_min))
		break;
	    }

	  for (octave_idx_type j = idx_j+1; j < nc; j++)
	    {
	      float tmp = elem (i, j);

	      if (xisnan (tmp))
		continue;
	      else if (tmp < tmp_min)
		{
		  idx_j = j;
		  tmp_min = tmp;
		}
	    }

	  result.elem (i) = tmp_min;
	  idx_arg.elem (i) = xisnan (tmp_min) ? 0 : idx_j;
        }
    }

  return result;
}

FloatColumnVector
FloatMatrix::row_max (void) const
{
  Array<octave_idx_type> dummy_idx;
  return row_max (dummy_idx);
}

FloatColumnVector
FloatMatrix::row_max (Array<octave_idx_type>& idx_arg) const
{
  FloatColumnVector result;

  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  if (nr > 0 && nc > 0)
    {
      result.resize (nr);
      idx_arg.resize (nr);

      for (octave_idx_type i = 0; i < nr; i++)
        {
	  octave_idx_type idx_j;

	  float tmp_max = octave_Float_NaN;

	  for (idx_j = 0; idx_j < nc; idx_j++)
	    {
	      tmp_max = elem (i, idx_j);

	      if (! xisnan (tmp_max))
		break;
	    }

	  for (octave_idx_type j = idx_j+1; j < nc; j++)
	    {
	      float tmp = elem (i, j);

	      if (xisnan (tmp))
		continue;
	      else if (tmp > tmp_max)
		{
		  idx_j = j;
		  tmp_max = tmp;
		}
	    }

	  result.elem (i) = tmp_max;
	  idx_arg.elem (i) = xisnan (tmp_max) ? 0 : idx_j;
        }
    }

  return result;
}

FloatRowVector
FloatMatrix::column_min (void) const
{
  Array<octave_idx_type> dummy_idx;
  return column_min (dummy_idx);
}

FloatRowVector
FloatMatrix::column_min (Array<octave_idx_type>& idx_arg) const
{
  FloatRowVector result;

  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  if (nr > 0 && nc > 0)
    {
      result.resize (nc);
      idx_arg.resize (nc);

      for (octave_idx_type j = 0; j < nc; j++)
        {
	  octave_idx_type idx_i;

	  float tmp_min = octave_Float_NaN;

	  for (idx_i = 0; idx_i < nr; idx_i++)
	    {
	      tmp_min = elem (idx_i, j);

	      if (! xisnan (tmp_min))
		break;
	    }

	  for (octave_idx_type i = idx_i+1; i < nr; i++)
	    {
	      float tmp = elem (i, j);

	      if (xisnan (tmp))
		continue;
	      else if (tmp < tmp_min)
		{
		  idx_i = i;
		  tmp_min = tmp;
		}
	    }

	  result.elem (j) = tmp_min;
	  idx_arg.elem (j) = xisnan (tmp_min) ? 0 : idx_i;
        }
    }

  return result;
}

FloatRowVector
FloatMatrix::column_max (void) const
{
  Array<octave_idx_type> dummy_idx;
  return column_max (dummy_idx);
}

FloatRowVector
FloatMatrix::column_max (Array<octave_idx_type>& idx_arg) const
{
  FloatRowVector result;

  octave_idx_type nr = rows ();
  octave_idx_type nc = cols ();

  if (nr > 0 && nc > 0)
    {
      result.resize (nc);
      idx_arg.resize (nc);

      for (octave_idx_type j = 0; j < nc; j++)
        {
	  octave_idx_type idx_i;

	  float tmp_max = octave_Float_NaN;

	  for (idx_i = 0; idx_i < nr; idx_i++)
	    {
	      tmp_max = elem (idx_i, j);

	      if (! xisnan (tmp_max))
		break;
	    }

	  for (octave_idx_type i = idx_i+1; i < nr; i++)
	    {
	      float tmp = elem (i, j);

	      if (xisnan (tmp))
		continue;
	      else if (tmp > tmp_max)
		{
		  idx_i = i;
		  tmp_max = tmp;
		}
	    }

	  result.elem (j) = tmp_max;
	  idx_arg.elem (j) = xisnan (tmp_max) ? 0 : idx_i;
        }
    }

  return result;
}

std::ostream&
operator << (std::ostream& os, const FloatMatrix& a)
{
  for (octave_idx_type i = 0; i < a.rows (); i++)
    {
      for (octave_idx_type j = 0; j < a.cols (); j++)
	{
	  os << " ";
	  octave_write_float (os, a.elem (i, j));
	}
      os << "\n";
    }
  return os;
}

std::istream&
operator >> (std::istream& is, FloatMatrix& a)
{
  octave_idx_type nr = a.rows ();
  octave_idx_type nc = a.cols ();

  if (nr < 1 || nc < 1)
    is.clear (std::ios::badbit);
  else
    {
      float tmp;
      for (octave_idx_type i = 0; i < nr; i++)
	for (octave_idx_type j = 0; j < nc; j++)
	  {
	    tmp = octave_read_float (is);
	    if (is)
	      a.elem (i, j) = tmp;
	    else
	      goto done;
	  }
    }

 done:

  return is;
}

FloatMatrix
Givens (float x, float y)
{
  float cc, s, temp_r;

  F77_FUNC (slartg, SLARTG) (x, y, cc, s, temp_r);

  FloatMatrix g (2, 2);

  g.elem (0, 0) = cc;
  g.elem (1, 1) = cc;
  g.elem (0, 1) = s;
  g.elem (1, 0) = -s;

  return g;
}

FloatMatrix
Sylvester (const FloatMatrix& a, const FloatMatrix& b, const FloatMatrix& c)
{
  FloatMatrix retval;

  // FIXME -- need to check that a, b, and c are all the same
  // size.

  // Compute Schur decompositions.

  FloatSCHUR as (a, "U");
  FloatSCHUR bs (b, "U");
  
  // Transform c to new coordinates.

  FloatMatrix ua = as.unitary_matrix ();
  FloatMatrix sch_a = as.schur_matrix ();

  FloatMatrix ub = bs.unitary_matrix ();
  FloatMatrix sch_b = bs.schur_matrix ();
  
  FloatMatrix cx = ua.transpose () * c * ub;
  
  // Solve the sylvester equation, back-transform, and return the
  // solution.

  octave_idx_type a_nr = a.rows ();
  octave_idx_type b_nr = b.rows ();

  float scale;
  octave_idx_type info;

  float *pa = sch_a.fortran_vec ();
  float *pb = sch_b.fortran_vec ();
  float *px = cx.fortran_vec ();

  F77_XFCN (strsyl, STRSYL, (F77_CONST_CHAR_ARG2 ("N", 1),
			     F77_CONST_CHAR_ARG2 ("N", 1),
			     1, a_nr, b_nr, pa, a_nr, pb,
			     b_nr, px, a_nr, scale, info
			     F77_CHAR_ARG_LEN (1)
			     F77_CHAR_ARG_LEN (1)));


  // FIXME -- check info?
  
  retval = -ua*cx*ub.transpose ();

  return retval;
}

// matrix by matrix -> matrix operations

/* Simple Dot Product, Matrix-Vector and Matrix-Matrix Unit tests
%!assert([1 2 3] * [ 4 ; 5 ; 6], 32, 1e-14)
%!assert([1 2 ; 3 4 ] * [5 ; 6], [17 ; 39 ], 1e-14)
%!assert([1 2 ; 3 4 ] * [5 6 ; 7 8], [19 22; 43 50], 1e-14)
*/

/* Test some simple identities
%!shared M, cv, rv
%! M = randn(10,10);
%! cv = randn(10,1);
%! rv = randn(1,10);
%!assert([M*cv,M*cv],M*[cv,cv],1e-14)
%!assert([rv*M;rv*M],[rv;rv]*M,1e-14)
%!assert(2*rv*cv,[rv,rv]*[cv;cv],1e-14)
*/

static const char *
get_blas_trans_arg (bool trans)
{
  static char blas_notrans = 'N', blas_trans = 'T';
  return (trans) ? &blas_trans : &blas_notrans;
}

// the general GEMM operation

FloatMatrix 
xgemm (bool transa, const FloatMatrix& a, bool transb, const FloatMatrix& b)
{
  FloatMatrix retval;

  octave_idx_type a_nr = transa ? a.cols () : a.rows ();
  octave_idx_type a_nc = transa ? a.rows () : a.cols ();

  octave_idx_type b_nr = transb ? b.cols () : b.rows ();
  octave_idx_type b_nc = transb ? b.rows () : b.cols ();

  if (a_nc != b_nr)
    gripe_nonconformant ("operator *", a_nr, a_nc, b_nr, b_nc);
  else
    {
      if (a_nr == 0 || a_nc == 0 || b_nc == 0)
	retval.resize (a_nr, b_nc, 0.0);
      else if (a.data () == b.data () && a_nr == b_nc && transa != transb)
        {
	  octave_idx_type lda = a.rows ();

          retval.resize (a_nr, b_nc);
	  float *c = retval.fortran_vec ();

          const char *ctransa = get_blas_trans_arg (transa);
          F77_XFCN (ssyrk, SSYRK, (F77_CONST_CHAR_ARG2 ("U", 1),
                                   F77_CONST_CHAR_ARG2 (ctransa, 1),
                                   a_nr, a_nc, 1.0,
                                   a.data (), lda, 0.0, c, a_nr
                                   F77_CHAR_ARG_LEN (1)
                                   F77_CHAR_ARG_LEN (1)));
          for (int j = 0; j < a_nr; j++)
            for (int i = 0; i < j; i++)
              retval.xelem (j,i) = retval.xelem (i,j);

        }
      else
	{
	  octave_idx_type lda = a.rows (), tda = a.cols ();
	  octave_idx_type ldb = b.rows (), tdb = b.cols ();

	  retval.resize (a_nr, b_nc);
	  float *c = retval.fortran_vec ();

	  if (b_nc == 1)
	    {
	      if (a_nr == 1)
		F77_FUNC (xsdot, XSDOT) (a_nc, a.data (), 1, b.data (), 1, *c);
	      else
		{
                  const char *ctransa = get_blas_trans_arg (transa);
		  F77_XFCN (sgemv, SGEMV, (F77_CONST_CHAR_ARG2 (ctransa, 1),
					   lda, tda, 1.0,  a.data (), lda,
					   b.data (), 1, 0.0, c, 1
					   F77_CHAR_ARG_LEN (1)));
		}
            }
          else if (a_nr == 1)
            {
              const char *crevtransb = get_blas_trans_arg (! transb);
              F77_XFCN (sgemv, SGEMV, (F77_CONST_CHAR_ARG2 (crevtransb, 1),
                                       ldb, tdb, 1.0,  b.data (), ldb,
                                       a.data (), 1, 0.0, c, 1
                                       F77_CHAR_ARG_LEN (1)));
            }
	  else
	    {
              const char *ctransa = get_blas_trans_arg (transa);
              const char *ctransb = get_blas_trans_arg (transb);
	      F77_XFCN (sgemm, SGEMM, (F77_CONST_CHAR_ARG2 (ctransa, 1),
				       F77_CONST_CHAR_ARG2 (ctransb, 1),
				       a_nr, b_nc, a_nc, 1.0, a.data (),
				       lda, b.data (), ldb, 0.0, c, a_nr
				       F77_CHAR_ARG_LEN (1)
				       F77_CHAR_ARG_LEN (1)));
	    }
	}
    }

  return retval;
}

FloatMatrix
operator * (const FloatMatrix& a, const FloatMatrix& b)
{
  return xgemm (false, a, false, b);
}

// FIXME -- it would be nice to share code among the min/max
// functions below.

#define EMPTY_RETURN_CHECK(T) \
  if (nr == 0 || nc == 0) \
    return T (nr, nc);

FloatMatrix
min (float d, const FloatMatrix& m)
{
  octave_idx_type nr = m.rows ();
  octave_idx_type nc = m.columns ();

  EMPTY_RETURN_CHECK (FloatMatrix);

  FloatMatrix result (nr, nc);

  for (octave_idx_type j = 0; j < nc; j++)
    for (octave_idx_type i = 0; i < nr; i++)
      {
	OCTAVE_QUIT;
	result (i, j) = xmin (d, m (i, j));
      }

  return result;
}

FloatMatrix
min (const FloatMatrix& m, float d)
{
  octave_idx_type nr = m.rows ();
  octave_idx_type nc = m.columns ();

  EMPTY_RETURN_CHECK (FloatMatrix);

  FloatMatrix result (nr, nc);

  for (octave_idx_type j = 0; j < nc; j++)
    for (octave_idx_type i = 0; i < nr; i++)
      {
	OCTAVE_QUIT;
	result (i, j) = xmin (m (i, j), d);
      }

  return result;
}

FloatMatrix
min (const FloatMatrix& a, const FloatMatrix& b)
{
  octave_idx_type nr = a.rows ();
  octave_idx_type nc = a.columns ();

  if (nr != b.rows () || nc != b.columns ())
    {
      (*current_liboctave_error_handler)
	("two-arg min expecting args of same size");
      return FloatMatrix ();
    }

  EMPTY_RETURN_CHECK (FloatMatrix);

  FloatMatrix result (nr, nc);

  for (octave_idx_type j = 0; j < nc; j++)
    for (octave_idx_type i = 0; i < nr; i++)
      {
	OCTAVE_QUIT;
	result (i, j) = xmin (a (i, j), b (i, j));
      }

  return result;
}

FloatMatrix
max (float d, const FloatMatrix& m)
{
  octave_idx_type nr = m.rows ();
  octave_idx_type nc = m.columns ();

  EMPTY_RETURN_CHECK (FloatMatrix);

  FloatMatrix result (nr, nc);

  for (octave_idx_type j = 0; j < nc; j++)
    for (octave_idx_type i = 0; i < nr; i++)
      {
	OCTAVE_QUIT;
	result (i, j) = xmax (d, m (i, j));
      }

  return result;
}

FloatMatrix
max (const FloatMatrix& m, float d)
{
  octave_idx_type nr = m.rows ();
  octave_idx_type nc = m.columns ();

  EMPTY_RETURN_CHECK (FloatMatrix);

  FloatMatrix result (nr, nc);

  for (octave_idx_type j = 0; j < nc; j++)
    for (octave_idx_type i = 0; i < nr; i++)
      {
	OCTAVE_QUIT;
	result (i, j) = xmax (m (i, j), d);
      }

  return result;
}

FloatMatrix
max (const FloatMatrix& a, const FloatMatrix& b)
{
  octave_idx_type nr = a.rows ();
  octave_idx_type nc = a.columns ();

  if (nr != b.rows () || nc != b.columns ())
    {
      (*current_liboctave_error_handler)
	("two-arg max expecting args of same size");
      return FloatMatrix ();
    }

  EMPTY_RETURN_CHECK (FloatMatrix);

  FloatMatrix result (nr, nc);

  for (octave_idx_type j = 0; j < nc; j++)
    for (octave_idx_type i = 0; i < nr; i++)
      {
	OCTAVE_QUIT;
	result (i, j) = xmax (a (i, j), b (i, j));
      }

  return result;
}

MS_CMP_OPS(FloatMatrix, , float, )
MS_BOOL_OPS(FloatMatrix, float, 0.0)

SM_CMP_OPS(float, , FloatMatrix, )
SM_BOOL_OPS(float, FloatMatrix, 0.0)

MM_CMP_OPS(FloatMatrix, , FloatMatrix, )
MM_BOOL_OPS(FloatMatrix, FloatMatrix, 0.0)

/*
;;; Local Variables: ***
;;; mode: C++ ***
;;; End: ***
*/
