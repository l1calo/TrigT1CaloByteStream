# CP RoI bytestream to CPMRoI conversion
# Specify the Converters
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::CPMRoI>" ]
ByteStreamAddressProviderSvc = Service( "ByteStreamAddressProviderSvc" )
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::CPMRoI>/CPMRoIs" ]
# DLLs
theApp.Dlls += [ "TrigT1CaloByteStream" ]
