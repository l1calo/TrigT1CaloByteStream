
# ==============================================================================
# Change the input file
InputFiles = [
    # "/afs/cern.ch/work/h/hristova/public/M7/248370/data14_cos.00248370.express_express.merge.RAW._lb0163._SFO-ALL._0001.1"
    "/afs/cern.ch/work/j/juraj/public/2015/TileCIS/data15_calib.00249297.calibration_L1CaloEnergyScan.daq.RAW._lb0000._SFO-1._0001.data",
    "/afs/cern.ch/work/j/juraj/public/2015/TileCIS/data15_calib.00249300.calibration_L1CaloEnergyScan.daq.RAW._lb0000._SFO-1._0001.data"
]
# ==============================================================================

from ByteStreamCnvSvc import ReadByteStream


svcMgr.ByteStreamInputSvc.FullFileName = InputFiles
include("TrigT1CaloByteStream/ReadLVL1CaloBSRun2_jobOptions.py")
# svcMgr.MessageSvc.defaultLimit = 5000


class PyTriggerTowerRef(PyAthena.Alg):

    def __init__(self, name='PyTriggerTowerRef', **kw):
        # init base class
        kw['name'] = name
        super(PyTriggerTowerRef, self).__init__(**kw)

    def initialize(self):
        self.event_store = PyAthena.py_svc("StoreGateSvc")
        return PyAthena.StatusCode.Success

    def execute(self):
        tt = self.event_store["xAODTriggerTowers"]
              
        self.setFilterPassed(True)
        return PyAthena.StatusCode.Success

    def finalize(self):
        return PyAthena.StatusCode.Success
    pass

from AthenaCommon.AlgSequence import AlgSequence

topSequence = AlgSequence()
topSequence += PyTriggerTowerRef()


from OutputStreamAthenaPool.MultipleStreamManager import MSMgr

MyFirstXAODStream = MSMgr.NewPoolRootStream( "StreamxAOD", "TileCIS.root" )


MyFirstXAODStream.AddItem(["xAOD::TriggerTowerContainer#xAODTriggerTowers"])
MyFirstXAODStream.AddItem(["xAOD::TriggerTowerAuxContainer#xAODTriggerTowersAux."])


#svcMgr.StoreGateSvc.Dump = True

theApp.EvtMax = 2
# ==============================================================================