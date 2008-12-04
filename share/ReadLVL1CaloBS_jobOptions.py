# Bytestream to TrigT1Calo objects conversions
include.block("TrigT1CaloByteStream/ReadLVL1CaloBS_jobOptions.py")
from AthenaCommon.AppMgr import ServiceMgr
from ByteStreamCnvSvc.ByteStreamCnvSvcConf import ByteStreamCnvSvc
ServiceMgr += ByteStreamCnvSvc()
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__PpmByteStreamTool
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__CpByteStreamTool
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__CpmRoiByteStreamTool
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__JepByteStreamTool
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__JepRoiByteStreamTool
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__RodHeaderByteStreamTool
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc += LVL1BS__PpmByteStreamTool("PpmByteStreamTool")
ByteStreamCnvSvc += LVL1BS__CpByteStreamTool("CpByteStreamTool")
ByteStreamCnvSvc += LVL1BS__CpmRoiByteStreamTool("CpmRoiByteStreamTool")
ByteStreamCnvSvc += LVL1BS__JepByteStreamTool("JepByteStreamTool")
ByteStreamCnvSvc += LVL1BS__JepRoiByteStreamTool("JepRoiByteStreamTool")
ByteStreamCnvSvc += LVL1BS__RodHeaderByteStreamTool("RodHeaderByteStreamTool")
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::TriggerTower>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::CPMTower>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::CPMHits>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::CMMCPHits>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::CPMRoI>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::JetElement>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::JEMHits>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::JEMEtSums>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::CMMJetHits>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::CMMEtSums>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::JEMRoI>" ]
ByteStreamCnvSvc.InitCnvs += [ "LVL1::CMMRoI" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::RODHeader>" ]
ByteStreamAddressProviderSvc = Service( "ByteStreamAddressProviderSvc" )
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::TriggerTower>/TriggerTowers" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::CPMTower>/CPMTowers" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::CPMTower>/CPMTowersOverlap" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::CPMHits>/CPMHits" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::CMMCPHits>/CMMCPHits" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::CPMRoI>/CPMRoIs" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::CPMRoI>/CPMRoIsRoIB" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::JetElement>/JetElements" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::JetElement>/JetElementsOverlap" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::JEMHits>/JEMHits" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::JEMEtSums>/JEMEtSums" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::CMMJetHits>/CMMJetHits" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::CMMEtSums>/CMMEtSums" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::JEMRoI>/JEMRoIs" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::JEMRoI>/JEMRoIsRoIB" ]
ByteStreamAddressProviderSvc.TypeNames += [ "LVL1::CMMRoI/CMMRoIs" ]
ByteStreamAddressProviderSvc.TypeNames += [ "LVL1::CMMRoI/CMMRoIsRoIB" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::RODHeader>/RODHeaders" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::RODHeader>/RODHeadersPP" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::RODHeader>/RODHeadersCP" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::RODHeader>/RODHeadersCPRoI" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::RODHeader>/RODHeadersJEP" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::RODHeader>/RODHeadersJEPRoI" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::RODHeader>/RODHeadersCPRoIB" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::RODHeader>/RODHeadersJEPRoIB" ]
