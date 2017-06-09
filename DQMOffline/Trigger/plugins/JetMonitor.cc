#include "DQMOffline/Trigger/plugins/JetMonitor.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "DQM/TrackingMonitor/interface/GetLumi.h"

#include "CommonTools/TriggerUtils/interface/GenericTriggerEventFlag.h"


// Define Phi Bin //
double Jet_MAX_PHI = 3.2;
int Jet_N_PHI = 64;
MEbinning jet_phi_binning_{
  Jet_N_PHI, -Jet_MAX_PHI, Jet_MAX_PHI
};
// Define Eta Bin //
double Jet_MAX_ETA = 5;
int Jet_N_ETA = 50;
MEbinning jet_eta_binning_{
  Jet_N_ETA, -Jet_MAX_ETA, Jet_MAX_ETA
};
// Define HEP17 for PHI 
double Jet_MAX_PHI_HEP17 = -0.52;
double Jet_MIN_PHI_HEP17 = -0.87;
int N_PHI_HEP17 = 7;
const MEbinning phi_binning_hep17_{
  N_PHI_HEP17, Jet_MIN_PHI_HEP17, Jet_MAX_PHI_HEP17
};
// Define HEP18 for PHI 
double Jet_MAX_PHI_HEP18 = -0.17;
double Jet_MIN_PHI_HEP18 = -0.52;
int N_PHI_HEP18 = 7;
const MEbinning phi_binning_hep18_{
  N_PHI_HEP18, Jet_MIN_PHI_HEP18, Jet_MAX_PHI_HEP18
};
// Define HEP17 for ETA 
double Jet_MAX_ETA_HEP17 = 3.0;
double Jet_MIN_ETA_HEP17 = 1.3;
int N_ETA_HEP17 = 6;
const MEbinning eta_binning_hep17_{
  N_ETA_HEP17, Jet_MIN_ETA_HEP17, Jet_MAX_ETA_HEP17
};

const MEbinning eta_binning_hem17_{
  N_ETA_HEP17, -Jet_MAX_ETA_HEP17, Jet_MIN_ETA_HEP17
};


// -----------------------------
//  constructors and destructor
// -----------------------------

JetMonitor::JetMonitor( const edm::ParameterSet& iConfig ):
   metSelection_ ( iConfig.getParameter<std::string>("metSelection") )
  , jetSelection_ ( iConfig.getParameter<std::string>("jetSelection") )
  , calojetSelection_ ( iConfig.getParameter<std::string>("calojetSelection") )
  , eleSelection_ ( iConfig.getParameter<std::string>("eleSelection") )
  , muoSelection_ ( iConfig.getParameter<std::string>("muoSelection") ) 
{
  folderName_            = iConfig.getParameter<std::string>("FolderName"); 
  metToken_              = consumes<reco::PFMETCollection>        (iConfig.getParameter<edm::InputTag>("met")       );
  pfjetToken_            = mayConsume<reco::PFJetCollection>      (iConfig.getParameter<edm::InputTag>("pfjets")    ); 
  calojetToken_          = mayConsume<reco::CaloJetCollection>    (iConfig.getParameter<edm::InputTag>("calojets")    ); 
  eleToken_              = mayConsume<reco::GsfElectronCollection>(iConfig.getParameter<edm::InputTag>("electrons") );
  muoToken_              = mayConsume<reco::MuonCollection>       (iConfig.getParameter<edm::InputTag>("muons")     );     
  met_variable_binning_  = iConfig.getParameter<edm::ParameterSet>("histoPSet").getParameter<std::vector<double> >("jetptBinning");
  met_binning_           = getHistoPSet   (iConfig.getParameter<edm::ParameterSet>("histoPSet").getParameter<edm::ParameterSet>   ("metPSet")    );
  jetptThr_binning_      = getHistoPSet   (iConfig.getParameter<edm::ParameterSet>("histoPSet").getParameter<edm::ParameterSet>   ("jetPtThrPSet")    );
  ls_binning_            = getHistoLSPSet (iConfig.getParameter<edm::ParameterSet>("histoPSet").getParameter<edm::ParameterSet>   ("lsPSet")     );

  num_genTriggerEventFlag_ = new GenericTriggerEventFlag(iConfig.getParameter<edm::ParameterSet>("numGenericTriggerEventPSet"),consumesCollector(), *this);
  den_genTriggerEventFlag_ = new GenericTriggerEventFlag(iConfig.getParameter<edm::ParameterSet>("denGenericTriggerEventPSet"),consumesCollector(), *this);

  njets_      = iConfig.getParameter<int>("njets" );
  nelectrons_ = iConfig.getParameter<int>("nelectrons" );
  nmuons_     = iConfig.getParameter<int>("nmuons" );
  ptcut_      = iConfig.getParameter<double>("ptcut" ); // for HLT Jet 
  isPFJetTrig    = iConfig.getParameter<bool>("ispfjettrg" );
  isCaloJetTrig  = iConfig.getParameter<bool>("iscalojettrg" );

//  jetHEP18_EtaVsPhi_.numerator   = nullptr;
//  jetHEP18_EtaVsPhi_.denominator = nullptr;

  // test a_ME
  AutoNullPtr(a_ME,sizeof(a_ME)/sizeof(a_ME[0]));
  AutoNullPtr(a_ME_HB,sizeof(a_ME_HB)/sizeof(a_ME_HB[0]));
  AutoNullPtr(a_ME_HE,sizeof(a_ME_HE)/sizeof(a_ME_HE[0]));
  AutoNullPtr(a_ME_HF,sizeof(a_ME_HF)/sizeof(a_ME_HF[0]));
  AutoNullPtr(a_ME_HE_p,sizeof(a_ME_HE_p)/sizeof(a_ME_HE_p[0]));
  AutoNullPtr(a_ME_HE_m,sizeof(a_ME_HE_m)/sizeof(a_ME_HE_m[0]));
  AutoNullPtr(a_ME_HEP17,sizeof(a_ME_HEP17)/sizeof(a_ME_HEP17[0]));
  AutoNullPtr(a_ME_HEM17,sizeof(a_ME_HEM17)/sizeof(a_ME_HEM17[0]));
  AutoNullPtr(a_ME_HEP18,sizeof(a_ME_HEP18)/sizeof(a_ME_HEP18[0]));
}

JetMonitor::~JetMonitor()
{
  if (num_genTriggerEventFlag_) delete num_genTriggerEventFlag_;
  if (den_genTriggerEventFlag_) delete den_genTriggerEventFlag_;
}

MEbinning JetMonitor::getHistoPSet(edm::ParameterSet pset)
{
  return MEbinning{
    pset.getParameter<int32_t>("nbins"),
      pset.getParameter<double>("xmin"),
      pset.getParameter<double>("xmax"),
      };
}

MEbinning JetMonitor::getHistoLSPSet(edm::ParameterSet pset)
{
  return MEbinning{
    pset.getParameter<int32_t>("nbins"),
      0.,
      double(pset.getParameter<int32_t>("nbins"))
      };
}

void JetMonitor::setMETitle(JetME& me, std::string titleX, std::string titleY)
{
  me.numerator->setAxisTitle(titleX,1);
  me.numerator->setAxisTitle(titleY,2);
  me.denominator->setAxisTitle(titleX,1);
  me.denominator->setAxisTitle(titleY,2);
}
void JetMonitor::bookME(DQMStore::IBooker &ibooker, JetME& me, std::string& histname, std::string& histtitle, int& nbins, double& min, double& max)
{
  me.numerator   = ibooker.book1D(histname+"_numerator",   histtitle+" (numerator)",   nbins, min, max);
  me.denominator = ibooker.book1D(histname+"_denominator", histtitle+" (denominator)", nbins, min, max);
}
void JetMonitor::bookME(DQMStore::IBooker &ibooker, JetME& me, std::string& histname, std::string& histtitle, std::vector<double> binning)
{
  int nbins = binning.size()-1;
  std::vector<float> fbinning(binning.begin(),binning.end());
  float* arr = &fbinning[0];
  me.numerator   = ibooker.book1D(histname+"_numerator",   histtitle+" (numerator)",   nbins, arr);
  me.denominator = ibooker.book1D(histname+"_denominator", histtitle+" (denominator)", nbins, arr);
}
void JetMonitor::bookME(DQMStore::IBooker &ibooker, JetME& me, std::string& histname, std::string& histtitle, int& nbinsX, double& xmin, double& xmax, double& ymin, double& ymax)
{
  me.numerator   = ibooker.bookProfile(histname+"_numerator",   histtitle+" (numerator)",   nbinsX, xmin, xmax, ymin, ymax);
  me.denominator = ibooker.bookProfile(histname+"_denominator", histtitle+" (denominator)", nbinsX, xmin, xmax, ymin, ymax);
}
void JetMonitor::bookME(DQMStore::IBooker &ibooker, JetME& me, std::string& histname, std::string& histtitle, int& nbinsX, double& xmin, double& xmax, int& nbinsY, double& ymin, double& ymax)
{
  me.numerator   = ibooker.book2D(histname+"_numerator",   histtitle+" (numerator)",   nbinsX, xmin, xmax, nbinsY, ymin, ymax);
  me.denominator = ibooker.book2D(histname+"_denominator", histtitle+" (denominator)", nbinsX, xmin, xmax, nbinsY, ymin, ymax);
}
void JetMonitor::bookME(DQMStore::IBooker &ibooker, JetME& me, std::string& histname, std::string& histtitle, std::vector<double> binningX, std::vector<double> binningY)
{
  int nbinsX = binningX.size()-1;
  std::vector<float> fbinningX(binningX.begin(),binningX.end());
  float* arrX = &fbinningX[0];
  int nbinsY = binningY.size()-1;
  std::vector<float> fbinningY(binningY.begin(),binningY.end());
  float* arrY = &fbinningY[0];

  me.numerator   = ibooker.book2D(histname+"_numerator",   histtitle+" (numerator)",   nbinsX, arrX, nbinsY, arrY);
  me.denominator = ibooker.book2D(histname+"_denominator", histtitle+" (denominator)", nbinsX, arrX, nbinsY, arrY);
}

void JetMonitor::bookHistograms(DQMStore::IBooker     & ibooker,
				 edm::Run const        & iRun,
				 edm::EventSetup const & iSetup) 
{  
  
  std::string histname, histtitle;
  std::string hist_obtag = "";
  std::string histtitle_obtag = "";
  std::string currentFolder = folderName_ ;
  ibooker.setCurrentFolder(currentFolder.c_str());

  if (isPFJetTrig) {hist_obtag = "pfjet";          histtitle_obtag =  "PFJet";}
  else if (isCaloJetTrig) {hist_obtag = "calojet"; histtitle_obtag =  "CaloJet"; }
  else {hist_obtag = "pfjet"; histtitle_obtag =  "PFJet"; } //default is pfjet 

  // Test bookMESub
  bookMESub(ibooker,a_ME,sizeof(a_ME)/sizeof(a_ME[0]),hist_obtag,histtitle_obtag,"","");
  bookMESub(ibooker,a_ME_HB,sizeof(a_ME_HB)/sizeof(a_ME_HB[0]),hist_obtag,histtitle_obtag,"HB","(HB)");
  bookMESub(ibooker,a_ME_HE,sizeof(a_ME_HE)/sizeof(a_ME_HE[0]),hist_obtag,histtitle_obtag,"HE","(HE)");
  bookMESub(ibooker,a_ME_HF,sizeof(a_ME_HF)/sizeof(a_ME_HF[0]),hist_obtag,histtitle_obtag,"HF","(HF)");
  bookMESub(ibooker,a_ME_HE_p,sizeof(a_ME_HE_p)/sizeof(a_ME_HE_p[0]),hist_obtag,histtitle_obtag,"HE_p","(HE+)");
  bookMESub(ibooker,a_ME_HE_m,sizeof(a_ME_HE_m)/sizeof(a_ME_HE_m[0]),hist_obtag,histtitle_obtag,"HE_m","(HE-)");
  bookMESub(ibooker,a_ME_HEP17,sizeof(a_ME_HEP17)/sizeof(a_ME_HEP17[0]),hist_obtag,histtitle_obtag,"HEP17","(HEP17)");
  bookMESub(ibooker,a_ME_HEM17,sizeof(a_ME_HEM17)/sizeof(a_ME_HEM17[0]),hist_obtag,histtitle_obtag,"HEM17","(HEM17)");
  bookMESub(ibooker,a_ME_HEP18,sizeof(a_ME_HEP18)/sizeof(a_ME_HEP18[0]),hist_obtag,histtitle_obtag,"HEP18","(HEP18)");

/*  histname = hist_obtag +"EtaVsPhi"; histtitle = histtitle_obtag + " eta Vs phi ";
  bookME(ibooker,jetEtaVsPhi_,histname,histtitle, jet_eta_binning_.nbins, jet_eta_binning_.xmin, jet_eta_binning_.xmax, jet_phi_binning_.nbins, jet_phi_binning_.xmin, jet_phi_binning_.xmax);
  setMETitle(jetEtaVsPhi_,histtitle_obtag + " #eta","#phi");
  v_ME.push_back(jetEtaVsPhi_);*/
  
  // Initialize the GenericTriggerEventFlag
  if ( num_genTriggerEventFlag_ && num_genTriggerEventFlag_->on() ) num_genTriggerEventFlag_->initRun( iRun, iSetup );
  if ( den_genTriggerEventFlag_ && den_genTriggerEventFlag_->on() ) den_genTriggerEventFlag_->initRun( iRun, iSetup );

}

#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "DataFormats/TrackerCommon/interface/TrackerTopology.h"
#include "Geometry/Records/interface/TrackerTopologyRcd.h"
#include "DataFormats/Math/interface/deltaR.h" // For Delta R
void JetMonitor::analyze(edm::Event const& iEvent, edm::EventSetup const& iSetup)  {
  // Filter out events if Trigger Filtering is requested
  if (den_genTriggerEventFlag_->on() && ! den_genTriggerEventFlag_->accept( iEvent, iSetup) ) return;
  edm::Handle<reco::PFJetCollection> pfjetHandle;
  iEvent.getByToken( pfjetToken_, pfjetHandle );

  edm::Handle<reco::CaloJetCollection> calojetHandle;
  iEvent.getByToken( calojetToken_, calojetHandle );

  int ls = iEvent.id().luminosityBlock();
  v_jetpt.clear();
  v_jeteta.clear();
  v_jetphi.clear();
  std::vector<reco::PFJet> pfjets;
  std::vector<reco::CaloJet> calojets;
  if ( int(pfjetHandle->size()) < njets_ ) return;
  if (isPFJetTrig){
     for ( auto const & jet : *pfjetHandle ) {
        if(!jetSelection_(jet)) continue;
        v_jetpt.push_back(jet.pt()); 
        v_jeteta.push_back(jet.eta()); 
        v_jetphi.push_back(jet.phi()); 
     }
  }
  else if (isCaloJetTrig){
     for ( auto const & jet : *calojetHandle ) {
        if(!calojetSelection_(jet)) continue;
        v_jetpt.push_back(jet.pt()); 
        v_jeteta.push_back(jet.eta()); 
        v_jetphi.push_back(jet.phi()); 
     }
  }
  else {cout << "Error!! Check out the type of JetTrigger!! " << endl;}
//  cout << "v_jetpt.size()? " << v_jetpt.size() << endl;
  if (v_jetpt.size() < 1) {return;}
  double jetpt_ = v_jetpt[0];
  double jeteta_ = v_jeteta[0];
  double jetphi_ = v_jetphi[0];

  FillME(a_ME,jetpt_,jetphi_,jeteta_,ls,"denominator"); 
  if (isBarrel( jeteta_ ) )
  {
     FillME(a_ME_HB,jetpt_,jetphi_,jeteta_,ls,"denominator"); 
  }
  if (isEndCapP( jeteta_ ) )
  {
     FillME(a_ME_HE,jetpt_,jetphi_,jeteta_,ls,"denominator"); 
     FillME(a_ME_HE_p,jetpt_,jetphi_,jeteta_,ls,"denominator"); 
  }
  if (isEndCapM( jeteta_ ) )
  {
     FillME(a_ME_HE,jetpt_,jetphi_,jeteta_,ls,"denominator"); 
     FillME(a_ME_HE_m,jetpt_,jetphi_,jeteta_,ls,"denominator"); 
  }
  if (isForward( jeteta_ ) )
  {
     FillME(a_ME_HF,jetpt_,jetphi_,jeteta_,ls,"denominator"); 
  }
  if (isHEP17( jeteta_, jetphi_ ) )
  {
     FillME(a_ME_HEP17,jetpt_,jetphi_,jeteta_,ls,"denominator"); 
  }
  if (isHEM17( jeteta_, jetphi_ ) )
  {
     FillME(a_ME_HEM17,jetpt_,jetphi_,jeteta_,ls,"denominator"); 
  }
  if (isHEP18( jeteta_, jetphi_ ) )
  {
     FillME(a_ME_HEP18,jetpt_,jetphi_,jeteta_,ls,"denominator"); 
  }
  if (num_genTriggerEventFlag_->on() && ! num_genTriggerEventFlag_->accept( iEvent, iSetup) ) return; // Require Numerator //

  FillME(a_ME,jetpt_,jetphi_,jeteta_,ls,"numerator"); 

  if (isBarrel( jeteta_ ) )
  {
     FillME(a_ME_HB,jetpt_,jetphi_,jeteta_,ls,"numerator"); 
  }
  if (isEndCapP( jeteta_ ) )
  {
     FillME(a_ME_HE,jetpt_,jetphi_,jeteta_,ls,"numerator"); 
     FillME(a_ME_HE_p,jetpt_,jetphi_,jeteta_,ls,"numerator"); 
  }
  if (isEndCapM( jeteta_ ) )
  {
     FillME(a_ME_HE,jetpt_,jetphi_,jeteta_,ls,"numerator"); 
     FillME(a_ME_HE_m,jetpt_,jetphi_,jeteta_,ls,"numerator"); 
  }
  if (isForward( jeteta_ ) )
  {
     FillME(a_ME_HF,jetpt_,jetphi_,jeteta_,ls,"numerator"); 
  }
  if (isHEP17( jeteta_, jetphi_ ) )
  {
     FillME(a_ME_HEP17,jetpt_,jetphi_,jeteta_,ls,"numerator"); 
  }
  if (isHEM17( jeteta_, jetphi_ ) )
  {
     FillME(a_ME_HEM17,jetpt_,jetphi_,jeteta_,ls,"numerator"); 
  }
  if (isHEP18( jeteta_, jetphi_ ) )
  {
     FillME(a_ME_HEP18,jetpt_,jetphi_,jeteta_,ls,"numerator"); 
  }

}

void JetMonitor::fillHistoPSetDescription(edm::ParameterSetDescription & pset)
{
  pset.add<int>   ( "nbins");
  pset.add<double>( "xmin" );
  pset.add<double>( "xmax" );
}

void JetMonitor::fillHistoLSPSetDescription(edm::ParameterSetDescription & pset)
{
  pset.add<int>   ( "nbins", 2500);
}

void JetMonitor::fillDescriptions(edm::ConfigurationDescriptions & descriptions)
{
  edm::ParameterSetDescription desc;
  desc.add<std::string>  ( "FolderName", "HLT/Jet" );

  desc.add<edm::InputTag>( "met",      edm::InputTag("pfMet") );
  desc.add<edm::InputTag>( "pfjets",     edm::InputTag("ak4PFJetsCHS") );
  desc.add<edm::InputTag>( "calojets",     edm::InputTag("ak4CaloJets") );
  desc.add<edm::InputTag>( "electrons",edm::InputTag("gedGsfElectrons") );
  desc.add<edm::InputTag>( "muons",    edm::InputTag("muons") );
  desc.add<int>("njets",      0);
  desc.add<int>("nelectrons", 0);
  desc.add<int>("nmuons",     0);
  desc.add<double>("ptcut",   0);
  desc.add<std::string>("metSelection", "pt > 0");
  desc.add<std::string>("jetSelection", "pt > 20");
  desc.add<std::string>("calojetSelection", "pt > 20");
  desc.add<std::string>("eleSelection", "pt > 0");
  desc.add<std::string>("muoSelection", "pt > 0");
  desc.add<bool>("ispfjettrg",    true);
  desc.add<bool>("iscalojettrg",  false);
  desc.add<bool>("isjetFrac", false);

  edm::ParameterSetDescription genericTriggerEventPSet;
  genericTriggerEventPSet.add<bool>("andOr");
  genericTriggerEventPSet.add<edm::InputTag>("dcsInputTag", edm::InputTag("scalersRawToDigi") );
  genericTriggerEventPSet.add<std::vector<int> >("dcsPartitions",{});
  genericTriggerEventPSet.add<bool>("andOrDcs", false);
  genericTriggerEventPSet.add<bool>("errorReplyDcs", true);
  genericTriggerEventPSet.add<std::string>("dbLabel","");
  genericTriggerEventPSet.add<bool>("andOrHlt", true);
  genericTriggerEventPSet.add<edm::InputTag>("hltInputTag", edm::InputTag("TriggerResults::HLT") );
  genericTriggerEventPSet.add<std::vector<std::string> >("hltPaths",{});
//  genericTriggerEventPSet.add<std::string>("hltDBKey","");
  genericTriggerEventPSet.add<bool>("errorReplyHlt",false);
  genericTriggerEventPSet.add<unsigned int>("verbosityLevel",1);

  desc.add<edm::ParameterSetDescription>("numGenericTriggerEventPSet", genericTriggerEventPSet);
  desc.add<edm::ParameterSetDescription>("denGenericTriggerEventPSet", genericTriggerEventPSet);

  edm::ParameterSetDescription histoPSet;
  edm::ParameterSetDescription metPSet;
  edm::ParameterSetDescription jetPtThrPSet;
  fillHistoPSetDescription(metPSet);
  fillHistoPSetDescription(jetPtThrPSet);
  histoPSet.add<edm::ParameterSetDescription>("metPSet", metPSet);  
  histoPSet.add<edm::ParameterSetDescription>("jetPtThrPSet", jetPtThrPSet);  
  std::vector<double> bins = {0.,20.,40.,60.,80.,90.,100.,110.,120.,130.,140.,150.,160.,170.,180.,190.,200.,220.,240.,260.,280.,300.,350.,400.,450.,1000.}; // Jet pT Binning
  histoPSet.add<std::vector<double> >("jetptBinning", bins);

  edm::ParameterSetDescription lsPSet;
  fillHistoLSPSetDescription(lsPSet);
  histoPSet.add<edm::ParameterSetDescription>("lsPSet", lsPSet);

  desc.add<edm::ParameterSetDescription>("histoPSet",histoPSet);

  descriptions.add("jetMonitoring", desc);
}

bool JetMonitor::isBarrel(double eta){
  bool output = false;
  if (fabs(eta)<=1.3) output=true;
  return output;
}

//------------------------------------------------------------------------//
bool JetMonitor::isEndCapM(double eta){
  bool output = false;
  if (fabs(eta) <= 3.0 && fabs(eta) > 1.3 && (eta/fabs(eta) < 0) ) output=true;
  return output;
}
/// For Hcal Endcap Plus Area
bool JetMonitor::isEndCapP(double eta){
  bool output = false;
  //if ( eta<=3.0 && eta >1.3) output=true;
  if (fabs(eta) <= 3.0 && fabs(eta) > 1.3 && (eta/fabs(eta) > 0) ) output=true;
  return output;
}
/// For Hcal Forward Plus Area
bool JetMonitor::isForward(double eta){
  bool output = false;
  if (fabs(eta)>3.0) output=true;
  return output;
}
/// For Hcal HEP17 Area
bool JetMonitor::isHEP17(double eta, double phi){
  bool output = false;
  // phi -0.87 to -0.52 
  if (fabs(eta) <= 3.0 && fabs(eta) > 1.3 && (eta/fabs(eta) > 0) &&
      phi > -0.87 && phi <= -0.52 ) {output=true;}
  return output;
}
/// For Hcal HEM17 Area
bool JetMonitor::isHEM17(double eta, double phi){
  bool output = false;
  if (fabs(eta) <= 3.0 && fabs(eta) > 1.3 && (eta/fabs(eta) < 0) &&
      phi > -0.87 && phi <= -0.52 ) {output=true;}
  return output;
}
/// For Hcal HEP18 Area
bool JetMonitor::isHEP18(double eta, double phi){
  bool output = false;
  // phi -0.87 to -0.52 
  if (fabs(eta) <= 3.0 && fabs(eta) > 1.3 && (eta/fabs(eta) > 0) &&
      phi > -0.52 && phi <= -0.17 ) {output=true;}
  return output;

}
void JetMonitor::AutoNullPtr(JetME* a_me,const int len_){
   for (int i =0; i < len_; ++i)
   {
      a_me[i].denominator = nullptr;
      a_me[i].numerator = nullptr;
   }
}
void JetMonitor::FillME(std::vector<JetME> v_me,double pt_, double phi_, double eta_, int ls_,std::string denu){
   std::vector<JetME> v_;
   v_.clear();
   v_ = v_me;
   std::string DenoOrNume = "";
   DenoOrNume = denu;
/*   cout << "v_me size ? " << v_me.size() << endl;
   cout << "pt : " << pt_ << " eta : "  << eta_ << " phi : " << phi_ << " ls : " << ls_ << endl; 
   for (unsigned int i =0; i < v_me.size(); ++i){
      std::cout << "v_me[" << i<< "] : " << v_me[i].denominator->getName()<< std::endl;
   }*/ 
   if (DenoOrNume == "denominator")
   {
      // index 0 = pt, 1 = ptThreshold , 2 = pt vs ls , 3 = phi, 4 = eta, 5 = eta vs phi 
      v_me[0].denominator->Fill(pt_);// pt
      v_me[1].denominator->Fill(pt_);// jetpT Threshold binning for pt 
      v_me[2].denominator->Fill(ls_,pt_);// pt vs ls
      v_me[3].denominator->Fill(phi_);// phi 
      v_me[4].denominator->Fill(eta_);// eta
      v_me[5].denominator->Fill(eta_,phi_);// eta vs phi
      v_me[6].denominator->Fill(eta_,pt_);// eta vs phi
   }
   else if (DenoOrNume == "numerator")
   {
      v_me[0].numerator->Fill(pt_);// pt 
      v_me[1].numerator->Fill(pt_);// jetpT Threshold binning for pt 
      v_me[2].numerator->Fill(ls_,pt_);// pt vs ls
      v_me[3].numerator->Fill(phi_);// phi
      v_me[4].numerator->Fill(eta_);// eat
      v_me[5].numerator->Fill(eta_,phi_);// eta vs phi
      v_me[6].numerator->Fill(eta_,pt_);// eta vs phi
   }
   else {
   std::cout << "CHECK OUT denu option in FillME !!!"<< std::endl;
   }
}
void JetMonitor::FillME(JetME* a_me,double pt_, double phi_, double eta_, int ls_,std::string denu){
   std::string isDeno = "";
   isDeno = denu;
   std::string DenoOrNume = "";
   DenoOrNume = denu;

   if (DenoOrNume == "denominator")
   {
      // index 0 = pt, 1 = vari.pt , 2 = pt vs ls , 3 = phi, 4 = eta, 5 = eta vs phi , 6 = eta vs pt
      a_me[0].denominator->Fill(pt_);// pt
      a_me[1].denominator->Fill(pt_);// jetpT Threshold binning for pt 
      a_me[2].denominator->Fill(ls_,pt_);// pt vs ls
      a_me[3].denominator->Fill(phi_);// phi 
      a_me[4].denominator->Fill(eta_);// eta
      a_me[5].denominator->Fill(eta_,phi_);// eta vs phi
      a_me[6].denominator->Fill(eta_,pt_);// eta vs pT
   }
   else if (DenoOrNume == "numerator")
   {
      a_me[0].numerator->Fill(pt_);// pt 
      a_me[1].numerator->Fill(pt_);// jetpT Threshold binning for pt 
      a_me[2].numerator->Fill(ls_,pt_);// pt vs ls
      a_me[3].numerator->Fill(phi_);// phi
      a_me[4].numerator->Fill(eta_);// eat
      a_me[5].numerator->Fill(eta_,phi_);// eta vs phi
      a_me[6].numerator->Fill(eta_,pt_);// eta vs pT 
   }
   else {
   std::cout << "CHECK OUT denu option in FillME !!!"<< std::endl;
   }
}
void JetMonitor::bookMESub(DQMStore::IBooker & Ibooker , JetME* a_me,const int len_,std::string h_Name ,std::string h_Title, std::string h_subOptName , std::string h_suOptTitle ){
   std::string hName = h_Name;
   std::string hTitle = h_Title;
   std::string hSubN =""; 
   std::string hSubT =""; 
   hSubT = h_suOptTitle;

   int nbin_phi = jet_phi_binning_.nbins;
   double maxbin_phi = jet_phi_binning_.xmax;
   double minbin_phi = jet_phi_binning_.xmin;

   int nbin_eta = jet_eta_binning_.nbins;
   double maxbin_eta = jet_eta_binning_.xmax;
   double minbin_eta = jet_eta_binning_.xmin;

   if (h_subOptName != ""){
      hSubN = "_"+h_subOptName;
   }

   if (h_subOptName == "HEP17") {
      nbin_phi = phi_binning_hep17_.nbins;
      maxbin_phi = phi_binning_hep17_.xmax;
      minbin_phi = phi_binning_hep17_.xmin;

      nbin_eta = eta_binning_hep17_.nbins;
      maxbin_eta = eta_binning_hep17_.xmax;
      minbin_eta = eta_binning_hep17_.xmin;
   }
   if (h_subOptName == "HEM17") {
      nbin_phi = phi_binning_hep17_.nbins;
      maxbin_phi = phi_binning_hep17_.xmax;
      minbin_phi = phi_binning_hep17_.xmin;

      nbin_eta = eta_binning_hem17_.nbins;
      maxbin_eta = eta_binning_hem17_.xmax;
      minbin_eta = eta_binning_hem17_.xmin;
   }
   if (h_subOptName == "HEP18") {
      nbin_phi = phi_binning_hep18_.nbins;
      maxbin_phi = phi_binning_hep18_.xmax;
      minbin_phi = phi_binning_hep18_.xmin;

      nbin_eta = eta_binning_hep17_.nbins;
      maxbin_eta = eta_binning_hep17_.xmax;
      minbin_eta = eta_binning_hep17_.xmin;
   }
   hName = h_Name+"pT"+hSubN;
   hTitle = h_Title+" pT " + hSubT;
   bookME(Ibooker,a_me[0],hName,hTitle,met_binning_.nbins,met_binning_.xmin, met_binning_.xmax);
   setMETitle(a_me[0], h_Title +" pT [GeV]","events / [GeV]");
   
   hName = h_Name+ "pT_pTThresh" + hSubN;
   hTitle = h_Title+" pT " + hSubT;
   bookME(Ibooker,a_me[1],hName,hTitle,jetptThr_binning_.nbins,jetptThr_binning_.xmin, jetptThr_binning_.xmax);
   setMETitle(a_me[1],h_Title + "pT [GeV]","events / [GeV]");

   hName = h_Name + "pTVsLS" + hSubN; 
   hTitle = h_Title+" vs LS " + hSubT;
   bookME(Ibooker,a_me[2],hName,hTitle,ls_binning_.nbins, ls_binning_.xmin, ls_binning_.xmax,met_binning_.xmin, met_binning_.xmax);
   setMETitle(a_me[2],"LS",h_Title + "pT [GeV]");

   hName = h_Name + "phi" + hSubN;
   hTitle = h_Title+" phi " + hSubT;
//   bookME(Ibooker,a_me[3],hName,h_Title, jet_phi_binning_.nbins, jet_phi_binning_.xmin, jet_phi_binning_.xmax);
   bookME(Ibooker,a_me[3],hName,hTitle, nbin_phi, minbin_phi,maxbin_phi );
   setMETitle(a_me[3],h_Title +" #phi","events / 0.1 rad");
   
   hName = h_Name + "eta"+ hSubN;
   hTitle = h_Title+" eta " + hSubT;
//   bookME(Ibooker,a_me[4],hName,h_Title, jet_eta_binning_.nbins, jet_eta_binning_.xmin, jet_eta_binning_.xmax);
   bookME(Ibooker,a_me[4],hName,hTitle, nbin_eta, minbin_eta, maxbin_eta);
   setMETitle(a_me[4],h_Title + " #eta","events / #eta");
   
   hName = h_Name + "EtaVsPhi"+hSubN;
   hTitle = h_Title+" eta Vs phi " + hSubT;
//   bookME(Ibooker,a_me[5],hName,h_Title, jet_eta_binning_.nbins, jet_eta_binning_.xmin, jet_eta_binning_.xmax, jet_phi_binning_.nbins, jet_phi_binning_.xmin, jet_phi_binning_.xmax);
   bookME(Ibooker,a_me[5],hName,hTitle, nbin_eta, minbin_eta, maxbin_eta, nbin_phi, minbin_phi, maxbin_phi);
   setMETitle(a_me[5],h_Title + " #eta","#phi");
   
   hName = h_Name + "EtaVspT"+hSubN;
   hTitle = h_Title+" eta Vs pT " + hSubT;
//   bookME(Ibooker,a_me[6],hName,h_Title, jet_eta_binning_.nbins, jet_eta_binning_.xmin, jet_eta_binning_.xmax, met_binning_.nbins,met_binning_.xmin, met_binning_.xmax);
   bookME(Ibooker,a_me[6],hName,hTitle, nbin_eta, minbin_eta, maxbin_eta, met_binning_.nbins,met_binning_.xmin, met_binning_.xmax);
   setMETitle(a_me[6],h_Title + " #eta","Leading Jet pT [GeV]");

}
// Define this as a plug-in
#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(JetMonitor);
