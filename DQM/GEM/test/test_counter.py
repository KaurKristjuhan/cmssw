import FWCore.ParameterSet.Config as cms

process = cms.Process('DQMTEST')

#from Configuration.StandardSequences.Eras import eras
#process = cms.Process('RECO',eras.Run2_2017,eras.run2_GEM_2017)

#process.load('Configuration.StandardSequences.L1Reco_cff')
#process.load('Configuration.StandardSequences.Reconstruction_cff')
#process.load('Configuration.StandardSequences.RecoSim_cff')
#process.load('Configuration.StandardSequences.AlCaRecoStreams_cff')


# import of standard configurations
process.load('Configuration.StandardSequences.Services_cff')
process.load('FWCore.MessageService.MessageLogger_cfi')
#process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
#process.load('Configuration.Geometry.GeometryDB_cff')
process.load('Configuration.Geometry.GeometryExtended2017Reco_cff')
process.load('Configuration.StandardSequences.MagneticField_38T_cff')
process.load('Configuration.StandardSequences.SimL1Emulator_cff')
process.load('Configuration.StandardSequences.EndOfProcess_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
process.load("DQM.Integration.config.FrontierCondition_GT_cfi")

process.MessageLogger = cms.Service("MessageLogger",
  statistics = cms.untracked.vstring(),
  destinations = cms.untracked.vstring('cerr'),
  cerr = cms.untracked.PSet(
      threshold = cms.untracked.string('WARNING')
  )
)



process.load("DQM.Integration.config.environment_cfi")
process.dqmEnv.subSystemFolder = "GEM"
process.dqmEnv.eventInfoFolder = "EventInfo"
process.dqmSaver.path = ""
process.dqmSaver.tag = "GEM"

process.source = cms.Source(
        "NewEventStreamFileReader",
        fileNames = cms.untracked.vstring ('file:FILENAME_TEMPLATE')
    )

process.maxEvents = cms.untracked.PSet(
  input = cms.untracked.int32(-1)
)

#process.load('EventFilter.GEMRawToDigi.GEMSQLiteCabling_cfi')

process.load("EventFilter.GEMRawToDigi.muonGEMDigis_cfi")
process.load('RecoLocalMuon.GEMRecHit.gemRecHits_cfi')
process.load("DQM.GEM.GEMDQM_cff")
process.muonGEMDigis.InputLabel = cms.InputTag('rawDataCollector')

process.muonGEMDigis.useDBEMap = True
process.muonGEMDigis.unPackStatusDigis = True

process.path = cms.Path(
  process.muonGEMDigis *
  process.gemRecHits *
  process.GEMDQMStrip
)

process.end_path = cms.EndPath(
  process.dqmEnv +
  process.dqmSaver
)

process.schedule = cms.Schedule(
  process.path,
  process.end_path
)
