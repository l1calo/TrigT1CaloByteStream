# JEP RoI bytestream to JEMRoI/... conversion
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__JepRoiByteStreamTool
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc += LVL1BS__JepRoiByteStreamTool("JepRoiByteStreamTool")
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::JEMRoI>" ]
ByteStreamCnvSvc.InitCnvs += [ "LVL1::CMMRoI" ]
ByteStreamAddressProviderSvc = Service( "ByteStreamAddressProviderSvc" )
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::JEMRoI>/JEMRoIs" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::JEMRoI>/JEMRoIsRoIB" ]
ByteStreamAddressProviderSvc.TypeNames += [ "LVL1::CMMRoI/CMMRoIs" ]
ByteStreamAddressProviderSvc.TypeNames += [ "LVL1::CMMRoI/CMMRoIsRoIB" ]
