#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/PluginManager/interface/ModuleDef.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"


#include "DQMServices/Core/interface/DQMEDAnalyzer.h"
#include "DQMServices/Core/interface/DQMStore.h"
#include "DQMServices/Core/interface/MonitorElement.h"

#include "DataFormats/GEMRecHit/interface/GEMRecHit.h"
#include "DataFormats/GEMRecHit/interface/GEMRecHitCollection.h"
#include "DataFormats/GEMDigi/interface/GEMDigiCollection.h"

#include "Geometry/GEMGeometry/interface/GEMGeometry.h"
#include "Geometry/Records/interface/MuonGeometryRecord.h"

#include <string>
#include <iostream>

//----------------------------------------------------------------------------------------------------
 
class GEMDQMStrip: public DQMEDAnalyzer
{
public:
  GEMDQMStrip(const edm::ParameterSet& cfg);
  ~GEMDQMStrip() override {};
  static void fillDescriptions(edm::ConfigurationDescriptions & descriptions);  
protected:
  void dqmBeginRun(edm::Run const &, edm::EventSetup const &) override {};
  void bookHistograms(DQMStore::IBooker &, edm::Run const &, edm::EventSetup const &) override;
  void analyze(edm::Event const& e, edm::EventSetup const& eSetup) override;
  void beginLuminosityBlock(edm::LuminosityBlock const& lumi, edm::EventSetup const& eSetup) override {std::cout<<"Beginning of new luminosity section"<<std::endl;};
  void endLuminosityBlock(edm::LuminosityBlock const& lumi, edm::EventSetup const& eSetup) override {std::cout<<"End of luminosity section"<<std::endl;};
  void endRun(edm::Run const& run, edm::EventSetup const& eSetup) override {};

private:
  edm::EDGetToken tagRecHit_;

  const GEMGeometry* initGeometry(edm::EventSetup const & iSetup);
  int findVFAT(float min_, float max_, float x_, int roll_);
     
  const GEMGeometry* GEMGeometry_; 

  std::vector<GEMChamber> gemChambers_;
  std::unordered_map<UInt_t,  MonitorElement*> StripsFired_vs_eta_;
};

using namespace std;
using namespace edm;

int GEMDQMStrip::findVFAT(float min_, float max_, float x_, int roll_) {
  float step = abs(max_-min_)/3.0;
  if ( x_ < (min(min_,max_)+step) ) { return 8 - roll_;}
  else if ( x_ < (min(min_,max_)+2.0*step) ) { return 16 - roll_;}
  else { return 24 - roll_;}
}

const GEMGeometry* GEMDQMStrip::initGeometry(edm::EventSetup const & iSetup) {
  const GEMGeometry* GEMGeometry_ = nullptr;
  try {
    edm::ESHandle<GEMGeometry> hGeom;
    iSetup.get<MuonGeometryRecord>().get(hGeom);
    GEMGeometry_ = &*hGeom;
  }
  catch( edm::eventsetup::NoProxyException<GEMGeometry>& e) {
    edm::LogError("MuonGEMBaseValidation") << "+++ Error : GEM geometry is unavailable on event loop. +++\n";
    return nullptr;
  }
  return GEMGeometry_;
}

GEMDQMStrip::GEMDQMStrip(const edm::ParameterSet& cfg)
{
  tagRecHit_ = consumes<GEMRecHitCollection>(cfg.getParameter<edm::InputTag>("recHitsInputLabel")); 
}

void GEMDQMStrip::fillDescriptions(edm::ConfigurationDescriptions & descriptions)
{
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("recHitsInputLabel", edm::InputTag("gemRecHits", "")); 
  descriptions.add("GEMDQMStrip", desc);  
}

void GEMDQMStrip::bookHistograms(DQMStore::IBooker &ibooker, edm::Run const &, edm::EventSetup const & iSetup)
{
  GEMGeometry_ = initGeometry(iSetup);
  if ( GEMGeometry_ == nullptr) return ;  

  const std::vector<const GEMSuperChamber*>& superChambers_ = GEMGeometry_->superChambers();   
  for (auto sch : superChambers_){
    int n_lay = sch->nChambers();
    for (int l=0;l<n_lay;l++){
      gemChambers_.push_back(*sch->chamber(l+1));
    }
  }
  ibooker.cd();
  ibooker.setCurrentFolder("GEM/recHit");
  for (auto ch : gemChambers_){
    GEMDetId gid = ch.id();
    string hName_fired = "StripFired_Gemini_"+to_string(gid.chamber())+"_la_"+to_string(gid.layer());
    string hTitle_fired = "StripsFired Gemini chamber : "+to_string(gid.chamber())+", layer : "+to_string(gid.layer());
    StripsFired_vs_eta_[ ch.id() ] = ibooker.book2D(hName_fired, hTitle_fired, 384, 1, 385, 8, 1,9);
  }
}

void GEMDQMStrip::analyze(edm::Event const& event, edm::EventSetup const& eventSetup)
{
  const GEMGeometry* GEMGeometry_  = initGeometry(eventSetup);
  if ( GEMGeometry_ == nullptr) return; 

  edm::Handle<GEMRecHitCollection> gemRecHits;
  event.getByToken( this->tagRecHit_, gemRecHits);
  if (!gemRecHits.isValid()) {
    edm::LogError("GEMDQMStrip") << "GEM recHit is not valid.\n";
    return ;
  }  
  for (auto ch : gemChambers_){
    GEMDetId cId = ch.id();
    for(auto roll : ch.etaPartitions()){
      GEMDetId rId = roll->id();
      const auto& recHitsRange = gemRecHits->get(rId); 
      auto gemRecHit = recHitsRange.first;
      for ( auto hit = gemRecHit; hit != recHitsRange.second; ++hit ) {
	for(int i = hit->firstClusterStrip(); i < (hit->firstClusterStrip() + hit->clusterSize()); i++){
	  StripsFired_vs_eta_[ cId ]->Fill(i, rId.roll());
	}
      }  
    }   
  }
}

DEFINE_FWK_MODULE(GEMDQMStrip);
