# JEP bytestream to JetElement/... conversion
# Specify the Converters
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::JetElement>" ]
ByteStreamAddressProviderSvc = Service( "ByteStreamAddressProviderSvc" )
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::JetElement>/LVL1JetElements" ]
# DLLs
theApp.Dlls += [ "TrigT1CaloByteStream" ]
# Algs
theApp.TopAlg += [ "JemTester" ]
Tester = Algorithm( "JemTester" )
Tester.JetElementLocation ="LVL1JetElements"
