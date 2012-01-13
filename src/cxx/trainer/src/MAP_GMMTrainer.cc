/**
 * @file src/cxx/trainer/src/MAP_GMMTrainer.cc
 * @author <a href="mailto:Roy.Wallace@idiap.ch">Roy Wallace</a> 
 * @author <a href="mailto:Francois.Moulin@idiap.ch">Francois Moulin</a>
 * @author <a href="mailto:Laurent.El-Shafey@idiap.ch">Laurent El Shafey</a> 
 *
 * Copyright (C) 2011 Idiap Reasearch Institute, Martigny, Switzerland
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
#include "trainer/MAP_GMMTrainer.h"
#include "trainer/Exception.h"

bob::trainer::MAP_GMMTrainer::MAP_GMMTrainer(double relevance_factor, bool update_means, bool update_variances, 
    bool update_weights, double mean_var_update_responsibilities_threshold): 
  GMMTrainer(update_means, update_variances, update_weights, mean_var_update_responsibilities_threshold), 
  relevance_factor(relevance_factor), m_prior_gmm(NULL), m_T3_alpha(0.), m_T3_adaptation(false) {
  
}

bob::trainer::MAP_GMMTrainer::~MAP_GMMTrainer() {
  
}

void bob::trainer::MAP_GMMTrainer::initialization(bob::machine::GMMMachine& gmm, const bob::io::Arrayset& data) {
  // Allocate memory for the sufficient statistics and initialise
  m_ss.resize(gmm.getNGaussians(),gmm.getNInputs());
  blitz::Array<double,2> ar2(gmm.getNGaussians(),gmm.getNInputs());
  gmm.setWeights(m_prior_gmm->getWeights());
  m_prior_gmm->getMeans(ar2);
  gmm.setMeans(ar2);
  m_prior_gmm->getVariances(ar2);
  gmm.setVariances(ar2);
}

bool bob::trainer::MAP_GMMTrainer::setPriorGMM(bob::machine::GMMMachine *prior_gmm) {
  if (prior_gmm == NULL) return false;
  m_prior_gmm = prior_gmm;
  return true;
}

void bob::trainer::MAP_GMMTrainer::mStep(bob::machine::GMMMachine& gmm, const bob::io::Arrayset& data) {
  // Read options and variables
  double n_gaussians = gmm.getNGaussians();
  double n_inputs = gmm.getNInputs();
  
  // Check that the prior GMM has been specified
  if (m_prior_gmm == NULL) {
    throw NoPriorGMM();
  }

  blitz::firstIndex i;
  blitz::secondIndex j;

  // Calculate the "data-dependent adaptation coefficient", alpha_i
  m_cache_alpha.resize(n_gaussians);
  if( m_T3_adaptation )
    m_cache_alpha = m_T3_alpha;
  else
    m_cache_alpha = m_ss.n(i) / (m_ss.n(i) + relevance_factor);

  // - Update weights if requested
  //   Equation 11 of Reynolds et al., "Speaker Verification Using Adapted Gaussian Mixture Models", Digital Signal Processing, 2000
  if (update_weights) {

    // Calculate the maximum likelihood weights
    m_cache_ml_weights.resize(n_gaussians);
    m_cache_ml_weights = m_ss.n / (int32_t)m_ss.T; //cast req. for linux/32-bits & osx

    // Get the prior weights
    const blitz::Array<double,1>& prior_weights = m_prior_gmm->getWeights();

    // Calculate the new weights
    m_cache_new_weights.resize(n_gaussians);
    m_cache_new_weights = m_cache_alpha * m_cache_ml_weights + (1-m_cache_alpha) * prior_weights;

    // Apply the scale factor, gamma, to ensure the new weights sum to unity 
    double gamma = blitz::sum(m_cache_new_weights);
    m_cache_new_weights /= gamma;

    // Set the new weights
    gmm.setWeights(m_cache_new_weights);
  }

  // Update GMM parameters
  // - Update means if requested
  //   Equation 12 of Reynolds et al., "Speaker Verification Using Adapted Gaussian Mixture Models", Digital Signal Processing, 2000
  if (update_means) {

    // Calculate the maximum likelihood means
    m_cache_ml_means.resize(n_gaussians,n_inputs);
    m_cache_ml_means = m_ss.sumPx(i,j) / m_ss.n(i);

    // Get the prior means
    m_cache_prior_means.resize(n_gaussians,n_inputs);
    m_prior_gmm->getMeans(m_cache_prior_means);

    // Calculate new means
    m_cache_new_means.resize(n_gaussians,n_inputs);
    for (int i = 0; i < n_gaussians; i++) {
      blitz::Array<double,1> means(m_cache_new_means(i, blitz::Range::all()));
      if(m_ss.n(i) < m_mean_var_update_responsibilities_threshold) {
        means = m_cache_prior_means(i, blitz::Range::all());
      }
      else {
        means = m_cache_alpha(i) * m_cache_ml_means(i, blitz::Range::all()) + (1-m_cache_alpha(i)) * m_cache_prior_means(i, blitz::Range::all());
      }
    }

    // Set the new means
    gmm.setMeans(m_cache_new_means);
  }

  // - Update variance if requested
  //   Equation 13 of Reynolds et al., "Speaker Verification Using Adapted Gaussian Mixture Models", Digital Signal Processing, 2000
  if (update_variances) {

    // Get the prior variances
    m_cache_prior_variances.resize(n_gaussians,n_inputs);
    m_prior_gmm->getVariances(m_cache_prior_variances);

    // Calculate E_i(x^2) (equation 10)
    m_cache_Exx.resize(n_gaussians,n_inputs);
    m_cache_Exx = m_ss.sumPxx(i,j) / m_ss.n(i);

    // Get means and prior means
    m_cache_means.resize(n_gaussians,n_inputs);
    gmm.getMeans(m_cache_means);
    m_cache_prior_means.resize(n_gaussians,n_inputs); 
    m_prior_gmm->getMeans(m_cache_prior_means);

    // Calculate new variances (equation 13)
    blitz::Array<double,2> new_variances(n_gaussians,n_inputs);
    for (int i=0; i<n_gaussians; ++i) {
      blitz::Array<double,1> variances(new_variances(i, blitz::Range::all()));
      if(m_ss.n(i) < m_mean_var_update_responsibilities_threshold) {
        variances = (m_cache_prior_variances(i,blitz::Range::all()) + m_cache_prior_means(i,blitz::Range::all())) - blitz::pow2(m_cache_means(i,blitz::Range::all()));
      }
      else {
        variances = m_cache_alpha(i) * m_cache_Exx(i,blitz::Range::all()) + (1-m_cache_alpha(i)) * (m_cache_prior_variances(i,blitz::Range::all()) + m_cache_prior_means(i,blitz::Range::all())) - blitz::pow2(m_cache_means(i,blitz::Range::all()));
      }
    }

    // Set the new variances
    gmm.setVariances(new_variances);
  }
}
