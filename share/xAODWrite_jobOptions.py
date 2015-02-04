
# ==============================================================================
# Change the input file
InputFiles = [
    #"/afs/cern.ch/work/v/vscharf/public/splash/data09_1beam.00140370.physics_MinBias.merge.RAW._lb0002._0001.1",
    "/afs/cern.ch/work/h/hristova/public/M7/248370/data14_cos.00248370.physics_L1Calo.merge.RAW._lb0007._SFO-ALL._0001.1",       
    #"/afs/cern.ch/work/h/hristova/public/M7/248370/data14_cos.00248370.express_express.merge.RAW._lb0163._SFO-ALL._0001.1"
    #"/afs/cern.ch/work/j/juraj/public/2015/TileCIS/data15_calib.00249297.calibration_L1CaloEnergyScan.daq.RAW._lb0000._SFO-1._0001.data",
    #"/afs/cern.ch/work/j/juraj/public/2015/TileCIS/data15_calib.00249300.calibration_L1CaloEnergyScan.daq.RAW._lb0000._SFO-1._0001.data",
    #"/afs/cern.ch/work/j/jfrost/public/R19testing/slice_rerun/data12_8TeV.00204158.express_express.merge.RAW._lb0958._SFO-ALL._0001.1"
 ]
# ==============================================================================

from ByteStreamCnvSvc import ReadByteStream


svcMgr.ByteStreamInputSvc.FullFileName = InputFiles
include("TrigT1CaloByteStream/ReadLVL1CaloBSRun2_jobOptions.py")
#svcMgr.MessageSvc.defaultLimit = 1000000


class PyTriggerTowerRef(PyAthena.Alg):

    def __init__(self, name='PyTriggerTowerRef', **kw):
        # init base class
        kw['name'] = name
        super(PyTriggerTowerRef, self).__init__(**kw)

    def initialize(self):
        self.event_store = PyAthena.py_svc("StoreGateSvc")
        return PyAthena.StatusCode.Success

    def execute(self):
        #tt1 = self.event_store["xAODTriggerTowersAux."]
        tt =  self.event_store["xAODTriggerTowers"]
 
        self.setFilterPassed(True)
        return PyAthena.StatusCode.Success

    def finalize(self):
        return PyAthena.StatusCode.Success
    pass

from AthenaCommon.AlgSequence import AlgSequence

topSequence = AlgSequence()
topSequence += PyTriggerTowerRef()


# from OutputStreamAthenaPool.MultipleStreamManager import MSMgr
   
# MyFirstXAODStream = MSMgr.NewPoolRootStream( "StreamxAOD", "TileCIS.root" )
   
# # MyFirstXAODStream.AddItem(["xAOD::CPMTowerContainer#xAODCPMTowers", "xAOD::CPMTowerAuxContainer#xAODCPMTowersAux."])
# MyFirstXAODStream.AddItem(["xAOD::TriggerTowerContainer#xAODTriggerTowers", "xAOD::TriggerTowerAuxContainer#xAODTriggerTowersAux."])



svcMgr.StoreGateSvc.Dump = True

theApp.EvtMax = 1
# ==============================================================================