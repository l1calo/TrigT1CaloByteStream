# Bytestream to TrigT1Calo objects conversions
include.block("TrigT1CaloByteStream/ReadLVL1CaloBS_jobOptions.py")

# the following include is needed to load correctly the trigger towers on/off, TT/Cells maps
include( "CaloConditions/CaloConditions_jobOptions.py" )
# To setup correctly the LArCablingService when doLAr is off in the top option.
if not rec.doLArg():
    include( "LArConditionsCommon/LArIdMap_comm_jobOptions.py" )
    include( "LArIdCnv/LArIdCnv_joboptions.py" )

from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__PpmByteStreamTool
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__CpByteStreamTool
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__CpmRoiByteStreamTool
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__JepByteStreamTool
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__JepRoiByteStreamTool
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__RodHeaderByteStreamTool
ToolSvc = Service("ToolSvc")
ToolSvc += LVL1BS__PpmByteStreamTool("PpmByteStreamTool")
ToolSvc += LVL1BS__CpByteStreamTool("CpByteStreamTool")
ToolSvc += LVL1BS__CpmRoiByteStreamTool("CpmRoiByteStreamTool")
ToolSvc += LVL1BS__JepByteStreamTool("JepByteStreamTool")
ToolSvc += LVL1BS__JepRoiByteStreamTool("JepRoiByteStreamTool")
ToolSvc += LVL1BS__RodHeaderByteStreamTool("RodHeaderByteStreamTool")

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
