# Test JEP bytestream to JetElement/... conversion
# Specify the Converters
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::JetElement>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::JEMHits>" ]
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::JEMEtSums>" ]
ByteStreamAddressProviderSvc = Service( "ByteStreamAddressProviderSvc" )
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::JetElement>/LVL1JetElements" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::JEMHits>/JEMHits" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::JEMEtSums>/JEMEtSums" ]
# DLLs
theApp.Dlls += [ "TrigT1CaloByteStream" ]
# Algs
theApp.TopAlg += [ "JemTester" ]
Tester = Algorithm( "JemTester" )
Tester.JetElementLocation = "LVL1JetElements"
Tester.JEMHitsLocation    = "JEMHits"
Tester.JEMEtSumsLocation  = "JEMEtSums"
Tester.JetElementPrint = 1
Tester.JEMHitsPrint    = 1
Tester.JEMEtSumsPrint  = 1
