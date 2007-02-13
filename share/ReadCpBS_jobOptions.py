# CP bytestream to CPMTower/... conversion
# Specify the Converters
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::CPMTower>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::CPMHits>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::CMMCPHits>" ]
ByteStreamAddressProviderSvc = Service( "ByteStreamAddressProviderSvc" )
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::CPMTower>/CPMTowers" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::CPMHits>/CPMHits" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::CMMCPHits>/CMMCPHits" ]
# DLLs
theApp.Dlls += [ "TrigT1CaloByteStream" ]
