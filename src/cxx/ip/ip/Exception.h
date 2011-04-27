/**
 * @file src/cxx/ip/ip/Exception.h
 * @author <a href="mailto:Laurent.El-Shafey@idiap.ch">Laurent El Shafey</a> 
 *
 * @brief Exceptions used throughout the IP subsystem of Torch
 */

#ifndef TORCH_IP_EXCEPTION_H 
#define TORCH_IP_EXCEPTION_H

#include <cstdlib>
#include "core/Exception.h"

namespace Torch { namespace ip {

  class Exception: public Torch::core::Exception {

    public:
      Exception() throw();
      virtual ~Exception() throw();
      virtual const char* what() const throw();

  };

  class ParamOutOfBoundaryError: public Exception {
    public:
      ParamOutOfBoundaryError(const std::string& paramname, const bool larger, 
        const int value, const int limit) throw();
      virtual ~ParamOutOfBoundaryError() throw();
      virtual const char* what() const throw();

    private:
      std::string m_paramname;
      bool m_larger;
      int m_value;
      int m_limit;
      mutable std::string m_message;
  };

  class UnknownScalingAlgorithm: public Exception {
    public:
      UnknownScalingAlgorithm() throw();
      virtual ~UnknownScalingAlgorithm() throw();
      virtual const char* what() const throw();

    private:
      mutable std::string m_message;
  };

  class UnknownRotatingAlgorithm: public Exception {
    public:
      UnknownRotatingAlgorithm() throw();
      virtual ~UnknownRotatingAlgorithm() throw();
      virtual const char* what() const throw();

    private:
      mutable std::string m_message;
  };
  
  /**
    * This exception is thrown when using a LBP with a non-common number
    * of neighbours ( != 4 && != 8)
   */
  class LBPUnsupportedNNeighbours: public Exception {
    public:
      LBPUnsupportedNNeighbours(const int N)  throw();
      virtual ~LBPUnsupportedNNeighbours() throw();
      virtual const char* what() const throw();

    private:
      int m_n_neighbours;
      mutable std::string m_message;
  };

}}

#endif /* TORCH_IP_EXCEPTION_H */
