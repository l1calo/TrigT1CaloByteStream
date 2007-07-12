# CP bytestream to CPMTower/... conversion
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__CpByteStreamTool
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc += LVL1BS__CpByteStreamTool("CpByteStreamTool")
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::CPMTower>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::CPMHits>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::CMMCPHits>" ]
ByteStreamAddressProviderSvc = Service( "ByteStreamAddressProviderSvc" )
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::CPMTower>/CPMTowers" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::CPMHits>/CPMHits" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::CMMCPHits>/CMMCPHits" ]
