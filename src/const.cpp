 /******************************************************************************
 *    This program is free software: you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License as published by      *
 *   the Free Software Foundation, either version 3 of the License, or         *
 *   (at your option) any later version.                                       *
 *                                                                             *
 *   This program is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *   GNU General Public License for more details.                              *
 *                                                                             *
 *   You should have received a copy of the GNU General Public License         *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                             *   
 *   Authors:                                                                  *
 *      Carlos Arguelles (University of Wisconsin Madison)                     * 
 *         carguelles@icecube.wisc.edu                                         *
 *      Christopher Weaver (University of Wisconsin Madison)                   * 
 *         chris.weaver@icecube.wisc.edu                                       *
 *      Jordi Salvado (University of Wisconsin Madison)                        *
 *         jsalvado@icecube.wisc.edu                                           *
 ******************************************************************************/

#include <SQuIDS/const.h>

#include <cmath>
#include <complex>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_blas.h>

#include <SQuIDS/SU_inc/dimension.h>

namespace squids{

Const::Const():
th(gsl_matrix_alloc(SQUIDS_MAX_HILBERT_DIM,SQUIDS_MAX_HILBERT_DIM),gsl_matrix_free),
dcp(gsl_matrix_alloc(SQUIDS_MAX_HILBERT_DIM,SQUIDS_MAX_HILBERT_DIM),gsl_matrix_free),
de(gsl_matrix_alloc(SQUIDS_MAX_HILBERT_DIM-1,1),gsl_matrix_free)
{
    /* PHYSICS CONSTANTS
    #===============================================================================
    # MATH
    #===============================================================================
    */
    pi=3.14159265358979;	    // Pi
    piby2=1.57079632679490;         // Pi/2
    sqrt2=1.41421356237310;         // Sqrt[2]
    ln2 = log(2.0);                 // log[2]
    
    /*
    #===============================================================================
    # EARTH 
    #===============================================================================
    */
    earthradius = 6371.0;	    // [km] Earth radius
    /*
    #===============================================================================
    # SUN 
    #===============================================================================
    */
    sunradius = 109.0*earthradius;  // [km] Sun radius 
    
    /*
    #===============================================================================
    # # PHYSICAL CONSTANTS
    #===============================================================================
    */
    GF = 1.16639e-23;	            // [eV^-2] Fermi Constant 
    Na = 6.0221415e+23;		    // [mol cm^-3] Avogadro Number
    sw_sq = 0.2312;                 // [dimensionless] sin(th_weinberg) ^2
    G  = 6.67300e-11;               // [m^3 kg^-1 s^-2]
    alpha = 1.0/137.0;              // [dimensionless] fine-structure constant 
    e_charge = sqrt(4.*pi*alpha);
    
    /*
    #===============================================================================
    # UNIT CONVERSION FACTORS
    #===============================================================================
    */
    // Energy
    EeV = 1.0e18;                   // [eV/EeV]
    PeV = 1.0e15;                   // [eV/PeV]
    TeV = 1.0e12;                   // [eV/TeV]
    GeV = 1.0e9;                    // [eV/GeV]
    MeV = 1.0e6;                    // [eV/MeV]
    keV = 1.0e3;                    // [eV/keV]
    eV  = 1.0;                      // [eV/eV]
    Joule = 1/1.60225e-19;          // [eV/J]
    // Mass
    kg = 5.62e35;                   // [eV/kg]
    gr = 1e-3*kg;                   // [eV/g] 
    // Time
    sec = 1.523e15;                 // [eV^-1/s]
    hour = 3600.0*sec;              // [eV^-1/h]
    day = 24.0*hour;                // [eV^-1/d]
    year = 365.0*day;               // [eV^-1/yr]
    // Distance
    meter = 5.06773093741e6;        // [eV^-1/m]
    cm = 1.0e-2*meter;              // [eV^-1/cm]
    km = 1.0e3*meter;               // [eV^-1/km]
    fermi = 1.0e-15*meter;          // [eV^-1/fm]
    angstrom = 1.0e-10*meter;       // [eV^-1/A]
    AU = 149.60e9*meter;            // [eV^-1/AU]
    ly = 9.4605284e15*meter;        // [eV^-1/ly]
    parsec = 3.08568025e16*meter;   // [eV^-1/parsec]
    kparsec = 1.e3*parsec;          // [eV^-1/kparsec]
    Mparsec = 1.e6*parsec;          // [eV^-1/Mparsec]
    Gparsec = 1.e9*parsec;          // [eV^-1/Gparsec]
    // luminosity
    picobarn = 1.0e-36*pow(cm,2);       // [eV^-2/pb]
    femtobarn = 1.0e-39*pow(cm,2);      // [eV^-2/fb]
    // Presure
    Pascal = Joule/pow(meter,3);        // [eV^4/Pa]
    atm = 101325.0*Pascal;          // [eV^4/atm]
    // Temperature
    Kelvin = 1/1.1604505e4;         // [eV/K]
    // Electromagnetic units
    C = 6.24150965e18*e_charge;
    A = C/sec;
    T = kg/(A*sec*sec);
    // Angle
    degree = pi/180.0;              // [rad/degree]
    
    // initializing matrices
    
  
    for(unsigned int i=0; i<SQUIDS_MAX_HILBERT_DIM-1; i++)
        gsl_matrix_set(de.get(),i,0,0.0);
  
    for(unsigned int i=0; i<SQUIDS_MAX_HILBERT_DIM; i++){
        for(unsigned int j=0; j<SQUIDS_MAX_HILBERT_DIM; j++)
            gsl_matrix_set(th.get(),i,j,0.0);
    }
  
    for(unsigned int i=0; i<SQUIDS_MAX_HILBERT_DIM; i++){
        for(unsigned int j=0; j<SQUIDS_MAX_HILBERT_DIM; j++)
            gsl_matrix_set(dcp.get(),i,j,0.0);
    }
    
    electron = 0;
    muon = 1;
    tau = 2;
    sterile1 = 3;
    sterile2 = 4;
    sterile3 = 5;
    
    tau_mass = 1776.82*MeV;
    tau_lifetime = 2.906e-13*sec;
    
    muon_lifetime = 2.196e-6*sec;
    muon_mass = 105.658*MeV;
    electron_mass = 0.5109*MeV;

    proton_mass = 938.272*MeV;
    neutron_mass = 939.565*MeV;
}

Const::~Const(){}

void Const::SetMixingAngle(unsigned int state1, unsigned int state2, double angle){
    if(state2<=state1)
        throw std::runtime_error("Const::SetMixingAngle: state indices should be ordered and unequal"
                                 " (Got "+std::to_string(state1)+" and "+std::to_string(state2)+")");
    if(state1>=SQUIDS_MAX_HILBERT_DIM-1)
        throw std::runtime_error("Const::SetMixingAngle: First state index must be less than "+std::to_string(SQUIDS_MAX_HILBERT_DIM-1));
    if(state2>=SQUIDS_MAX_HILBERT_DIM)
        throw std::runtime_error("Const::SetMixingAngle: Second mass state index must be less than " SQUIDS_MAX_HILBERT_DIM_STR);
    
    gsl_matrix_set(th.get(),state1,state2,angle);
}

double Const::GetMixingAngle(unsigned int state1, unsigned int state2) const{
    if(state2<=state1)
        throw std::runtime_error("Const::GetMixingAngle: state indices should be ordered and unequal"
                                 " (Got "+std::to_string(state1)+" and "+std::to_string(state2)+")");
    if(state1>=SQUIDS_MAX_HILBERT_DIM-1)
        throw std::runtime_error("Const::GetMixingAngle: First state index must be less than "+std::to_string(SQUIDS_MAX_HILBERT_DIM-1));
    if(state2>=SQUIDS_MAX_HILBERT_DIM)
        throw std::runtime_error("Const::GetMixingAngle: Second mass state index must be less than " SQUIDS_MAX_HILBERT_DIM_STR);
    
    return(gsl_matrix_get(th.get(),state1,state2));
}

void Const::SetEnergyDifference(unsigned int upperState, double diff){
    if(upperState==0)
        throw std::runtime_error("Const::SetSquaredEnergyDifference: Upper state index must be greater than 0");
    if(upperState>=SQUIDS_MAX_HILBERT_DIM)
        throw std::runtime_error("Const::SetSquaredEnergyDifference: Upper state index must be less than " SQUIDS_MAX_HILBERT_DIM_STR);
    
    gsl_matrix_set(de.get(),upperState-1,0,diff);
}

double Const::GetEnergyDifference(unsigned int upperState) const{
    if(upperState==0)
        throw std::runtime_error("Const::GetSquaredEnergyDifference: Upper state index must be greater than 0");
    if(upperState>=SQUIDS_MAX_HILBERT_DIM)
        throw std::runtime_error("Const::GetSquaredEnergyDifference: Upper state index must be less than " SQUIDS_MAX_HILBERT_DIM_STR);
    
    return(gsl_matrix_get(de.get(),upperState-1,0));
}

void Const::SetPhase(unsigned int state1, unsigned int state2, double phase){
    if(state2<=state1)
        throw std::runtime_error("Const::SetPhase: State indices should be ordered and unequal"
                                 " (Got "+std::to_string(state1)+" and "+std::to_string(state2)+")");
    if(state2>=SQUIDS_MAX_HILBERT_DIM)
        throw std::runtime_error("Const::SetPhase: Upper state index must be less than " SQUIDS_MAX_HILBERT_DIM_STR);
    
    gsl_matrix_set(dcp.get(),state1,state2,phase);
}

double Const::GetPhase(unsigned int state1, unsigned int state2) const{
    if(state2<=state1)
        throw std::runtime_error("Const::GetPhase: State indices should be ordered and unequal"
                                 " (Got "+std::to_string(state1)+" and "+std::to_string(state2)+")");
    if(state2>=SQUIDS_MAX_HILBERT_DIM)
        throw std::runtime_error("Const::GetPhase: Upper state index must be less than " SQUIDS_MAX_HILBERT_DIM_STR);
    
    return(gsl_matrix_get(dcp.get(),state1,state2));
}

std::unique_ptr<gsl_matrix_complex,void (*)(gsl_matrix_complex*)> Const::GetTransformationMatrix(size_t dim) const{
  if(dim>SQUIDS_MAX_HILBERT_DIM)
    throw std::runtime_error("Const::GetTransformationMatrix: dimension must be less than " SQUIDS_MAX_HILBERT_DIM_STR);
  
  gsl_matrix_complex* U = gsl_matrix_complex_alloc(dim,dim);
  gsl_matrix_complex* R = gsl_matrix_complex_alloc(dim,dim);
  gsl_matrix_complex* dummy = gsl_matrix_complex_alloc(dim,dim);
  gsl_matrix_complex_set_identity(U);
  gsl_matrix_complex_set_identity(R);
  gsl_matrix_complex_set_zero(dummy);
  
  const auto unit=gsl_complex_rect(1,0);
  const auto zero=gsl_complex_rect(0,0);
  auto to_gsl=[](const std::complex<double>& c)->gsl_complex{
    return(gsl_complex_rect(c.real(),c.imag()));
  };
  
  //construct each subspace rotation and accumulate the product
  for(size_t j=1; j<dim; j++){
    for(size_t i=0; i<j; i++){
      //set up the subspace rotation
      double theta=GetMixingAngle(i,j);
      double delta=GetPhase(i,j);
      double c=cos(theta);
      auto cp=sin(theta)*std::exp(std::complex<double>(0,-delta));
      auto cpc=-std::conj(cp);
      gsl_matrix_complex_set(R,i,i,to_gsl(c));
      gsl_matrix_complex_set(R,i,j,to_gsl(cp));
      gsl_matrix_complex_set(R,j,i,to_gsl(cpc));
      gsl_matrix_complex_set(R,j,j,to_gsl(c));
      
      //multiply this rotation onto the product from the left
      gsl_blas_zgemm(CblasNoTrans,CblasNoTrans,unit,R,U,zero,dummy);
      std::swap(U,dummy);
      
      //clean up the rotation matrix for next iteration
      gsl_matrix_complex_set(R,i,i,unit);
      gsl_matrix_complex_set(R,i,j,zero);
      gsl_matrix_complex_set(R,j,i,zero);
      gsl_matrix_complex_set(R,j,j,unit);
    }
  }
  
  //clean up temporary matrices
  gsl_matrix_complex_free(R);
  gsl_matrix_complex_free(dummy);

  return std::unique_ptr<gsl_matrix_complex,void (*)(gsl_matrix_complex*)>(U,gsl_matrix_complex_free);
}

} //namespace squids
