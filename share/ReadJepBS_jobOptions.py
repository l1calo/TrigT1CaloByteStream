# JEP bytestream to JetElement/... conversion
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__JepByteStreamTool
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc += LVL1BS__JepByteStreamTool("JepByteStreamTool")
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::JetElement>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::JEMHits>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::JEMEtSums>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::CMMJetHits>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::CMMEtSums>" ]
ByteStreamAddressProviderSvc = Service( "ByteStreamAddressProviderSvc" )
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::JetElement>/JetElements" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::JEMHits>/JEMHits" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::JEMEtSums>/JEMEtSums" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::CMMJetHits>/CMMJetHits" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::CMMEtSums>/CMMEtSums" ]
