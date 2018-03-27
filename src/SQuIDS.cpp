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
 *      Jordi Salvado (University of Wisconsin Madison)                        *
 *         jsalvado@icecube.wisc.edu                                           *
 *      Christopher Weaver (University of Wisconsin Madison)                   *
 *         chris.weaver@icecube.wisc.edu                                       *
 ******************************************************************************/

#include <SQuIDS/SQuIDS.h>
#include <cmath>
#include <limits>
#include <algorithm>
#include <iostream>

namespace squids{

///\brief Auxiliary function used for the GSL interface
int RHS(double ,const double*,double*,void*);

SQuIDS::SQuIDS():
CoherentRhoTerms(false),
NonCoherentRhoTerms(false),
OtherRhoTerms(false),
DecoherenceTerms(false),
GammaScalarTerms(false),
OtherScalarTerms(false),
AnyNumerics(false),
is_init(false),
adaptive_step(true),
nsteps(1000),
step(gsl_odeiv2_step_rkf45),
h(std::numeric_limits<double>::epsilon()),
h_min(std::numeric_limits<double>::min()),
h_max(std::numeric_limits<double>::max()),
abs_error(1e-20),
rel_error(1e-20),
last_dstate_ptr(nullptr),
last_estate_ptr(nullptr),
debug(false)
{
  sys.function = &RHS;
  sys.jacobian = NULL;
  sys.dimension = 0;
  sys.params = this;
}

SQuIDS::SQuIDS(unsigned int n, unsigned int ns, unsigned int nrh, unsigned int nsc, double ti):
SQuIDS(){
  ini(n,ns,nrh,nsc,ti);
}

SQuIDS::SQuIDS(SQuIDS&& other):
CoherentRhoTerms(other.CoherentRhoTerms),
NonCoherentRhoTerms(other.NonCoherentRhoTerms),
OtherRhoTerms(other.OtherRhoTerms),
DecoherenceTerms(other.DecoherenceTerms),
GammaScalarTerms(other.GammaScalarTerms),
OtherScalarTerms(other.OtherScalarTerms),
AnyNumerics(other.AnyNumerics),
is_init(other.is_init),
adaptive_step(other.adaptive_step),
x(std::move(other.x)),
t(other.t),
t_ini(other.t_ini),
nsteps(other.nsteps),
size_rho(other.size_rho),
size_state(other.size_state),
system(std::move(other.system)),
step(other.step),
sys(other.sys),
h(other.h),
h_min(other.h_min),
h_max(other.h_max),
abs_error(other.abs_error),
rel_error(other.rel_error),
dstate(std::move(other.dstate)),
nx(other.nx),
nsun(other.nsun),
nrhos(other.nrhos),
nscalars(other.nscalars),
params(std::move(other.params)),
state(std::move(other.state)),
estate(std::move(other.estate)),
last_dstate_ptr(other.last_dstate_ptr),
last_estate_ptr(other.last_estate_ptr),
debug(other.debug)
{
  sys.params=this;
  other.is_init=false; //other is no longer usable, since we stole its contents
}

void SQuIDS::ini(unsigned int n, unsigned int nsu, unsigned int nrh, unsigned int nsc, double ti){
  /*
    Setting the number of energy bins, number of components for the density matrix and
    number of scalar functions
  */

  nx=n;
  nsun=nsu;
  nrhos=nrh;
  nscalars=nsc;
  size_rho=nsun*nsun;
  size_state=(size_rho*nrhos+nscalars);
  /*
    Setting time units and initial time
  */
  t_ini=ti;
  t=ti;

  //Allocate memory for the system
  unsigned int numeqn=nx*size_state;
  system.reset(new double[numeqn]);
  sys.dimension = static_cast<size_t>(numeqn);

  /*
    Initializing the SU algebra object, needed to compute algebraic operations like commutators,
    anticommutators, rotations and evolutions
  */

  //Allocate memory
  x.resize(nx);

  state.reset(new SU_state[nx]);
  estate.reset(new SU_state[nx]);
  dstate.reset(new SU_state[nx]);

  for(unsigned int ei = 0; ei < nx; ei++){
    state[ei].rho.reset(new SU_vector[nrhos]);
    estate[ei].rho.reset(new SU_vector[nrhos]);
    dstate[ei].rho.reset(new SU_vector[nrhos]);
  }

  //initially, estate just points to the same memory as state
  for(unsigned int ei = 0; ei < nx; ei++){
    for(unsigned int i=0;i<nrhos;i++){
      state[ei].rho[i]=SU_vector(nsun,&(system[ei*size_state+i*size_rho]));
      estate[ei].rho[i]=SU_vector(nsun,&(system[ei*size_state+i*size_rho]));
      dstate[ei].rho[i]=SU_vector(nsun,nullptr);
    }
    if(nscalars>0){
      state[ei].scalar=&(system[ei*size_state+nrhos*size_rho]);
      estate[ei].scalar=&(system[ei*size_state+nrhos*size_rho]);
    }
  }
  last_dstate_ptr=nullptr;
  last_estate_ptr=nullptr;

  is_init=true;
};

void SQuIDS::set_system_pointers(double* sp, double* dp){
  //If the memory we're told to use is the same as in the last call,
  //we can skip resetting all of the pointers.
  if(sp!=last_estate_ptr){
    for(unsigned int ei = 0; ei < nx; ei++){
      for(unsigned int i=0;i<nrhos;i++)
        estate[ei].rho[i].SetBackingStore(&(sp[ei*size_state+i*size_rho]));
      estate[ei].scalar=&(sp[ei*size_state+nrhos*size_rho]);
    }
    last_estate_ptr=sp;
  }
  if(dp!=last_dstate_ptr){
    for(unsigned int ei = 0; ei < nx; ei++){
      for(unsigned int i=0;i<nrhos;i++)
        dstate[ei].rho[i].SetBackingStore(&(dp[ei*size_state+i*size_rho]));
      dstate[ei].scalar=&(dp[ei*size_state+nrhos*size_rho]);
    }
    last_dstate_ptr=dp;
  }
}

SQuIDS::~SQuIDS(){}

SQuIDS& SQuIDS::operator=(SQuIDS&& other){
  if(&other==this)
    return(*this);
  
  CoherentRhoTerms=other.CoherentRhoTerms;
  NonCoherentRhoTerms=other.NonCoherentRhoTerms;
  OtherRhoTerms=other.OtherRhoTerms;
  DecoherenceTerms=other.DecoherenceTerms;
  GammaScalarTerms=other.GammaScalarTerms;
  OtherScalarTerms=other.OtherScalarTerms;
  AnyNumerics=other.AnyNumerics;
  is_init=other.is_init;
  adaptive_step=other.adaptive_step;
  x=std::move(other.x);
  t=other.t;
  t_ini=other.t_ini;
  nsteps=other.nsteps;
  size_rho=other.size_rho;
  size_state=other.size_state;
  system=std::move(other.system);
  step=other.step;
  sys=other.sys;
  h=other.h;
  h_min=other.h_min;
  h_max=other.h_max;
  abs_error=other.abs_error;
  rel_error=other.rel_error;
  dstate=std::move(other.dstate);
  nx=other.nx;
  nsun=other.nsun;
  nrhos=other.nrhos;
  nscalars=other.nscalars;
  params=std::move(other.params);
  state=std::move(other.state);
  estate=std::move(other.estate);
  last_dstate_ptr=other.last_dstate_ptr;
  last_estate_ptr=other.last_estate_ptr;
  sys.params=this;
  debug=other.debug;
  other.is_init=false; //other is no longer usable, since we stole its contents
  
  return(*this);
}

/*
  This functions sets the grid of points in where we have a
  density matrix and a set of scalars
 */

void SQuIDS::Set_xrange(double xi, double xf, std::string type){
  if (xi == xf){
    x[0] = xi;
    return;
  }

  if(type=="linear" || type=="Linear" || type=="lin" || type=="Lin"){
    for(unsigned int e1 = 0; e1 < nx; e1++){
      x[e1]=xi+(xf-xi)*static_cast<double>(e1)/static_cast<double>(nx-1);
    }
  }else if(type=="log" || type=="Log"){
    double xmin_log,xmax_log;
    if (xi < 1.0e-10 ){
      throw std::runtime_error("SQUIDS::Set_xrange : Xmin too small for log scale");
    }else{
        xmin_log = log(xi);
    }
    xmax_log = log(xf);

    for(unsigned int e1 = 0; e1 < nx; e1++){
      double X=xmin_log+(xmax_log-xmin_log)*static_cast<double>(e1)/static_cast<double>(nx-1);
      x[e1]=exp(X);
    }
  }else{
    throw std::runtime_error("SQUIDS::Set_xrange : Not well deffined X range");
  }
}

double SQuIDS::GetExpectationValue(SU_vector op, unsigned int nrh, unsigned int i) const{
  SU_vector h0=H0(x[i],nrh);
  return state[i].rho[nrh]*op.Evolve(h0,t-t_ini);
}

double SQuIDS::GetExpectationValue(SU_vector op, unsigned int nrh, unsigned int i, double scale, std::vector<bool>& avr) const {
  SU_vector h0=H0(x[i],nrh);
  std::unique_ptr<double[]> evol_buf(new double[h0.GetEvolveBufferSize()]);
  h0.PrepareEvolve(evol_buf.get(),t-t_ini,scale,avr);
  return state[i].rho[nrh]*op.Evolve(evol_buf.get());
}

SU_vector SQuIDS::GetIntermediateState(unsigned int nrh, double xi) const{
  //find bracketing state entries
  auto xit=std::lower_bound(x.begin(),x.end(),xi);
  if(xit==x.end())
    throw std::runtime_error("SQUIDS::GetExpectationValueD : x value not in the array.");
  if(xit!=x.begin())
    xit--;
  size_t xid=std::distance(x.begin(),xit);
  //linearly interpolate between the two states
  double f2=((xi-x[xid])/(x[xid+1]-x[xid]));
  double f1=1-f2;
  return f1*state[xid].rho[nrh] + f2*state[xid+1].rho[nrh];
}

double SQuIDS::GetExpectationValueD(const SU_vector& op, unsigned int nrh, double xi) const{
#ifdef SQUIDS_THREAD_LOCAL
  static SQUIDS_THREAD_LOCAL expectationValueDBuffer buf(nsun);
#else //slow way, without thread local storage
  expectationValueDBuffer buf(nsun);
#endif
  return(GetExpectationValueD(op,nrh,xi,buf));
}

double SQuIDS::GetExpectationValueD(const SU_vector& op, unsigned int nrh, double xi, double scale, std::vector<bool>& avr) const{
#ifdef SQUIDS_THREAD_LOCAL
  static SQUIDS_THREAD_LOCAL expectationValueDBuffer buf(nsun);
#else //slow way, without thread local storage
  expectationValueDBuffer buf(nsun);
#endif
  return(GetExpectationValueD(op,nrh,xi,buf,scale,avr));
}

double SQuIDS::GetExpectationValueD(const SU_vector& op, unsigned int nrh, double xi,
                                    SQuIDS::expectationValueDBuffer& buf) const{
  //find bracketing state entries
  auto xit=std::lower_bound(x.begin(),x.end(),xi);
  if(xit==x.end())
    throw std::runtime_error("SQUIDS::GetExpectationValueD : x value not in the array.");
  if(xit!=x.begin())
    xit--;
  size_t xid=std::distance(x.begin(),xit);

  //linearly interpolate between the two states
  double f2=((xi-x[xid])/(x[xid+1]-x[xid]));
  double f1=1-f2;
  buf.state =f1*state[xid].rho[nrh];
  buf.state+=f2*state[xid+1].rho[nrh];
  //compute the evolved operator
  buf.op=op.Evolve(H0(xi,nrh),t-t_ini);
  //apply operator to state
  return buf.state*buf.op;
}

double SQuIDS::GetExpectationValueD(const SU_vector& op, unsigned int nrh, double xi,
                                    SQuIDS::expectationValueDBuffer& buf,
                                    double scale, std::vector<bool>& avr) const{
  //find bracketing state entries
  auto xit=std::lower_bound(x.begin(),x.end(),xi);
  if(xit==x.end())
    throw std::runtime_error("SQUIDS::GetExpectationValueD : x value not in the array.");
  if(xit!=x.begin())
    xit--;
  size_t xid=std::distance(x.begin(),xit);

  //linearly interpolate between the two states
  double f2=((xi-x[xid])/(x[xid+1]-x[xid]));
  double f1=1-f2;
  buf.state =f1*state[xid].rho[nrh];
  buf.state+=f2*state[xid+1].rho[nrh];
  //compute the evolved operator
  std::unique_ptr<double[]> evol_buf(new double[H0(xi,nrh).GetEvolveBufferSize()]);
  H0(xi,nrh).PrepareEvolve(evol_buf.get(),t-t_ini,scale,avr);
  buf.op=op.Evolve(evol_buf.get());
  //apply operator to state
  return (buf.op*state[xid].rho[nrh])*f1 + (buf.op*state[xid+1].rho[nrh])*f2;
  //return buf.state*buf.op;
}

void SQuIDS::Set_xrange(const std::vector<double>& xs){
  if(xs.size()!=nx)
    throw std::runtime_error("SQUIDS::Set_xrange : wrong number of x values");
  if(!std::is_sorted(xs.begin(),xs.end()))
    throw std::runtime_error("SQUIDS::Set_xrange : x values must be sorted");
  x=xs;
}

unsigned int SQuIDS::Get_i(double xi) const{
  double xl, xr;
  unsigned int nr=nx-1;
  unsigned int nl=0;

  xl=x[nl];
  xr=x[nr];

  if(xi>xr || xi<xl)
    throw std::runtime_error(" Error SQUIDS::Get_i :  value  out of bounds");

  while((nr-nl)>1){
    if(((nr-nl)%2)!=0){
      if(nr<nx-1)nr++;
      else if(nl>0)nl--;
    }
    if(xi<(xl+(xr-xl)/2)){
      nr=nl+(nr-nl)/2;
      xr=x[nr];
    }else{
      nl=nl+(nr-nl)/2;
      xl=x[nl];
    }
  }
  return nl;
}

void SQuIDS::Set_GSL_step(gsl_odeiv2_step_type const* opt){
  step = opt;
}

void SQuIDS::Set_AdaptiveStep(bool opt){
  adaptive_step=opt;
}
void SQuIDS::Set_CoherentRhoTerms(bool opt){
  CoherentRhoTerms=opt;
  AnyNumerics=(CoherentRhoTerms||NonCoherentRhoTerms||OtherRhoTerms||DecoherenceTerms||GammaScalarTerms||OtherScalarTerms);
}
void SQuIDS::Set_NonCoherentRhoTerms(bool opt){
  NonCoherentRhoTerms=opt;
  AnyNumerics=(CoherentRhoTerms||NonCoherentRhoTerms||OtherRhoTerms||DecoherenceTerms||GammaScalarTerms||OtherScalarTerms);
}
void SQuIDS::Set_OtherRhoTerms(bool opt){
  OtherRhoTerms=opt;
  AnyNumerics=(CoherentRhoTerms||NonCoherentRhoTerms||OtherRhoTerms||DecoherenceTerms||GammaScalarTerms||OtherScalarTerms);
}

void SQuIDS::Set_DecoherenceTerms(bool opt){
  DecoherenceTerms=opt;
  AnyNumerics=(CoherentRhoTerms||NonCoherentRhoTerms||OtherRhoTerms||DecoherenceTerms||GammaScalarTerms||OtherScalarTerms);
}

void SQuIDS::Set_GammaScalarTerms(bool opt){
  GammaScalarTerms=opt;
  AnyNumerics=(CoherentRhoTerms||NonCoherentRhoTerms||OtherRhoTerms||DecoherenceTerms||GammaScalarTerms||OtherScalarTerms);
}
void SQuIDS::Set_OtherScalarTerms(bool opt){
  OtherScalarTerms=opt;
  AnyNumerics=(CoherentRhoTerms||NonCoherentRhoTerms||OtherRhoTerms||DecoherenceTerms||GammaScalarTerms||OtherScalarTerms);
}

void SQuIDS::Set_AnyNumerics(bool opt){
  AnyNumerics=opt;
}

void SQuIDS::Set_h_min(double opt){
  h_min=opt;
  if(h<h_min){
    if(h_max<50.0*h_min){
      h=(h_min+h_max)/2.0;
    }else{
      h=h_min*10.0;
    }
  }
}

void SQuIDS::Set_h_max(double opt){
  h_max=opt;
  if(h>h_max){
    if(h_max<50.0*h_min){
      h=(h_min+h_max)/2.0;
    }else{
      h=h_min*10.0;
    }
  }
}

void SQuIDS::Set_h(double opt){
  h=opt;
}

double SQuIDS::Get_h_min() const{
  return h_min;
}

double SQuIDS::Get_h_max() const{
  return h_max;
}

double SQuIDS::Get_h() const{
  return h;
}

void SQuIDS::Set_rel_error(double opt){
  rel_error=opt;
}

void SQuIDS::Set_abs_error(double opt){
  abs_error=opt;
}

void SQuIDS::Set_NumSteps(unsigned int opt){
  nsteps=opt;
}

double SQuIDS::Get_rel_error() const{
  return rel_error;
}

double SQuIDS::Get_abs_error() const{
  return abs_error;
}

double SQuIDS::Get_NumSteps() const{
  return nsteps;
}

void SQuIDS::Derive(double at){
  t=at;
  PreDerive(at);
  for(unsigned int ei = 0; ei < nx; ei++){
    // Density matrix
    for(unsigned int i = 0; i < nrhos; i++){
      // Coherent interaction
      if(CoherentRhoTerms)
        dstate[ei].rho[i] = iCommutator(estate[ei].rho[i],HI(ei,i,t));
      else
        dstate[ei].rho[i].SetAllComponents(0.);
      // Non coherent interaction
      if(NonCoherentRhoTerms)
        dstate[ei].rho[i] -= ACommutator(GammaRho(ei,i,t),estate[ei].rho[i]);
      // Other possible interaction, for example involving the Scalars or non linear terms in rho.
      if(OtherRhoTerms)
        dstate[ei].rho[i] += InteractionsRho(ei,i,t);
      // Decoherence
      if(DecoherenceTerms) { //TODO Maybe this should be instead labelled `DissipationTerms`?
        dstate[ei].rho[i] -= D_Rho(ei,i,t);
      }

      //Some debug logging TODO remove once have everything working
      if (debug) {
        if (ei == 0) {
          if (i==0) {
            std::cout << "Evolving state : t = " << t << " : rho = " << estate[ei].rho[i] << " : Total prob = " << (estate[ei].rho[i]*estate[ei].rho[i]) <<  std::endl;
          }
        }
      }


    }
    //Scalars
    for(unsigned int is=0;is<nscalars;is++){
      dstate[ei].scalar[is]=0.;
      if(GammaScalarTerms)
        dstate[ei].scalar[is] += -estate[ei].scalar[is]*GammaScalar(ei,is,t);
      if(OtherScalarTerms)
        dstate[ei].scalar[is] += InteractionsScalar(ei,is,t);
    }
  }
}

void SQuIDS::Evolve(double dt){
  if(AnyNumerics){
    int gsl_status = GSL_SUCCESS;

    // initial time
    
    // ODE system error control
    gsl_odeiv2_driver* d = gsl_odeiv2_driver_alloc_y_new(&sys,step,h,abs_error,rel_error);
    gsl_odeiv2_driver_set_hmin(d,h_min);
    gsl_odeiv2_driver_set_hmax(d,h_max);
    gsl_odeiv2_driver_set_nmax(d,0);
    
    double* gsl_sys = system.get();
    
    if(adaptive_step){
      gsl_status = gsl_odeiv2_driver_apply(d, &t, t+dt, gsl_sys);
    }else{
      gsl_status = gsl_odeiv2_driver_apply_fixed_step(d, &t, dt/nsteps , nsteps , gsl_sys);
    }
    
    gsl_odeiv2_driver_free(d);
    if( gsl_status != GSL_SUCCESS ){
      throw std::runtime_error("SQUIDS::Evolve: Error in GSL ODE solver ("
                               +std::string(gsl_strerror(gsl_status))+")");
    }
    
    //after evolving, make estate alias state again
    for(unsigned int ei = 0; ei < nx; ei++){
      for(unsigned int i=0;i<nrhos;i++)
        estate[ei].rho[i].SetBackingStore(&(system[ei*size_state+i*size_rho]));
      if(nscalars>0)
        estate[ei].scalar=&(system[ei*size_state+nrhos*size_rho]);
    }
  }else{
    t+=dt;
    PreDerive(t);
  }
}

int RHS(double t, const double* state_dbl_in, double* state_dbl_out, void* par){
  SQuIDS* dms=static_cast<SQuIDS*>(par);
  dms->set_system_pointers(const_cast<double*>(state_dbl_in),state_dbl_out);
  dms->Derive(t);
  return 0;
}
  
} //namespace squids
