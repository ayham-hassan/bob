/**
 * @file cxx/ip/ip/scale.h
 * @date Mon Mar 14 11:21:29 2011 +0100
 * @author Laurent El Shafey <Laurent.El-Shafey@idiap.ch>
 *
 * @brief This file defines a function to rescale a 2D or 3D array/image.
 *
 * Copyright (C) 2011-2012 Idiap Research Institute, Martigny, Switzerland
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BOB5SPRO_IP_SCALE_H
#define BOB5SPRO_IP_SCALE_H

#include "core/array_assert.h"
#include "core/array_index.h"
#include "core/cast.h"
#include "ip/Exception.h"
#include "ip/common.h"

namespace tca = bob::core::array;

namespace bob {
/**
 * \ingroup libip_api
 * @{
 *
 */
  namespace ip {

    namespace detail {
      /**
        * @brief Function which rescales a 2D blitz::array/image of a given 
        *   type, using bilinear interpolation.
        *   The first dimension is the height (y-axis), whereas the second
        *   one is the width (x-axis).
        * @warning No check is performed on the dst blitz::array/image.
        * @param src The input blitz array
        * @param dst The output blitz array
        */
      template<typename T, bool mask>
      void scaleNoCheck2D_BI(const blitz::Array<T,2>& src, 
        const blitz::Array<bool,2>& src_mask, blitz::Array<double,2>& dst,
        blitz::Array<bool,2>& dst_mask)
      {
        const int height = dst.extent(0);
        const int width = dst.extent(1);

        const double x_ratio = (src.extent(1)-1.) / (width-1.);
        const double y_ratio = (src.extent(0)-1.) / (height-1.);
        for( int y=0; y<height; ++y) {
          double y_src = y_ratio * y;
          double dy2 = y_src - floor(y_src);
          double dy1 = 1. - dy2;
          int y_ind1 = tca::keepInRange( floor(y_src), 0, src.extent(0)-1);
          int y_ind2 = tca::keepInRange( y_ind1+1, 0, src.extent(0)-1);
          for( int x=0; x<width; ++x) {
            double x_src = x_ratio * x;
            double dx2 = x_src - floor(x_src);
            double dx1 = 1. - dx2;
            int x_ind1 = tca::keepInRange( floor(x_src), 0, src.extent(1)-1);
            int x_ind2 = tca::keepInRange( x_ind1+1, 0, src.extent(1)-1);
            double val = dx1*dy1*src(y_ind1, x_ind1)+dx1*dy2*src(y_ind2, x_ind1)
              + dx2*dy1*src(y_ind1, x_ind2 )+dx2*dy2*src(y_ind2, x_ind2 );
            dst(y,x) = val; // TODO Check C-style cast
            if( mask) {
              bool all_in_mask = true;
              for( int ym=y_ind1; ym<=y_ind2; ++ym)
                for( int xm=x_ind1; xm<=x_ind2; ++xm)
                  all_in_mask = all_in_mask && src_mask(ym,xm);
              dst_mask(y,x) = all_in_mask;
            }
          }
        }
      }

    }

    namespace Rescale {
      enum Algorithm {
        NearestNeighbour,
        BilinearInterp
      };
    }

    /**
      * @brief Function which rescales a 2D blitz::array/image of a given type.
      *   The first dimension is the height (y-axis), whereas the second
      *   one is the width (x-axis).
      * @param src The input blitz array
      * @param dst The output blitz array. The new array is resized according
      *   to the dimensions of this dst array.
      * @param alg The algorithm used for rescaling.
      */
    template<typename T>
    void scale(const blitz::Array<T,2>& src, blitz::Array<double,2>& dst, 
      const enum Rescale::Algorithm alg=Rescale::BilinearInterp)
    {
      // Check and resize src if required
      tca::assertZeroBase(src);

      // Check and resize dst if required
      tca::assertZeroBase(dst);

      // Defines output height and width
      const int height = dst.extent(0);
      const int width = dst.extent(1);

      // Check parameters and throw exception if required
      if( height<1 ) {
        throw ParamOutOfBoundaryError("height", false, height, 1);
      }
      if( width<1 ) {
        throw ParamOutOfBoundaryError("width", false, width, 1);
      }
  
      // If src and dst have the same shape, do a simple copy
      if( height==src.extent(0) && width==src.extent(1)) {
        for( int y=0; y<src.extent(0); ++y)
          for( int x=0; x<src.extent(1); ++x)
            dst(y,x) = bob::core::cast<double>(src(y,x));
      }
      // Otherwise, do the rescaling
      else
      {    
        // Rescale the 2D array
        switch(alg)
        {
          case Rescale::BilinearInterp:
            {
              // Rescale using Bilinear Interpolation
              blitz::Array<bool,2> src_mask, dst_mask;
              detail::scaleNoCheck2D_BI<T,false>(src, src_mask, dst, dst_mask);
            }
            break;
          default:
            throw bob::ip::UnknownScalingAlgorithm();
        }
      }
    }

    /**
      * @brief Function which rescales a 2D blitz::array/image of a given type.
      *   The first dimension is the height (y-axis), whereas the second
      *   one is the width (x-axis).
      * @param src The input blitz array
      * @param dst The output blitz array. The new array is resized according
      *   to the dimensions of this dst array.
      * @param alg The algorithm used for rescaling.
      */
    template<typename T>
    void scale(const blitz::Array<T,2>& src, const blitz::Array<bool,2>& src_mask,
      blitz::Array<double,2>& dst, blitz::Array<bool,2>& dst_mask,
      const enum Rescale::Algorithm alg=Rescale::BilinearInterp)
    {
      // Check and resize src if required
      tca::assertZeroBase(src);
      tca::assertZeroBase(src_mask);
      tca::assertSameShape(src, src_mask);

      // Check and resize dst if required
      tca::assertZeroBase(dst);
      tca::assertZeroBase(dst_mask);
      tca::assertSameShape(dst, dst_mask);

      // Defines output height and width
      const int height = dst.extent(0);
      const int width = dst.extent(1);

      // Check parameters and throw exception if required
      if( height<1 ) {
        throw ParamOutOfBoundaryError("height", false, height, 1);
      }
      if( width<1 ) {
        throw ParamOutOfBoundaryError("width", false, width, 1);
      }
  
      // If src and dst have the same shape, do a simple copy
      if( height==src.extent(0) && width==src.extent(1)) {
        for( int y=0; y<src.extent(0); ++y)
          for( int x=0; x<src.extent(1); ++x)
            dst(y,x) = bob::core::cast<double>(src(y,x));
        detail::copyNoCheck(src_mask,dst_mask); 
      }
      // Otherwise, do the rescaling
      else
      {    
        // Rescale the 2D array
        switch(alg)
        {
          case Rescale::BilinearInterp:
            {
              // Rescale using Bilinear Interpolation
              detail::scaleNoCheck2D_BI<T,true>(src, src_mask, dst, dst_mask);
            }
            break;
          default:
            throw bob::ip::UnknownScalingAlgorithm();
        }
      }
    }


	  template <typename T>
	  blitz::Array<T,2> scaleAs(const blitz::Array<T,2>& original, const double scale_factor) 
	  {
		  blitz::TinyVector<int, 2> new_shape = floor(original.shape() * scale_factor + 0.5);
		  return blitz::Array<T,2>(new_shape);
	  }
	  
	  template <typename T>
	  blitz::Array<T,3> scaleAs(const blitz::Array<T,3>& original, const double scale_factor) 
	  {
		  // with 3d Blitz arrays (e.g, color image) we do not want to scale the number of planes :)
		  blitz::TinyVector<int, 3> new_shape = original.shape();
		  new_shape(1) = floor(new_shape(1) * scale_factor + 0.5);
		  new_shape(2) = floor(new_shape(2) * scale_factor + 0.5);
		  
		  return blitz::Array<T,3>(new_shape);
	  }
  }
/**
 * @}
 */
}

#endif /* BOB5SPRO_IP_SCALE_H */
