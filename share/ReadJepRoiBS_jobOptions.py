# JEP RoI bytestream to JEMRoI/... conversion
# Specify the Converters
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::JEMRoI>" ]
ByteStreamCnvSvc.InitCnvs += [ "LVL1::CMMRoI" ]
ByteStreamAddressProviderSvc = Service( "ByteStreamAddressProviderSvc" )
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::JEMRoI>/JEMRoIs" ]
ByteStreamAddressProviderSvc.TypeNames += [ "LVL1::CMMRoI/CMMRoIs" ]
# DLLs
theApp.Dlls += [ "TrigT1CaloByteStream" ]
