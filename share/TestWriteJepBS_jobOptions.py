# Test JepContainer to JEP (JEM+CMMs) bytestream conversion
theApp.Dlls += [ "TrigT1CaloByteStream" ]

theApp.TopAlg += [ "JemTester" ]
Tester = Algorithm( "JemTester" )
Tester.JetElementLocation = "LVL1JetElements"
Tester.JEMHitsLocation    = "JEMHits"
Tester.JEMEtSumsLocation  = "JEMEtSums"
Tester.JetElementPrint = 1
Tester.JEMHitsPrint    = 1
Tester.JEMEtSumsPrint  = 1

theApp.TopAlg += [ "JepContainerMaker" ]
JepMaker = Algorithm( "JepContainerMaker" )
JepMaker.JetElementLocation = "LVL1JetElements"
JepMaker.JEMHitsLocation    = "JEMHits"
JepMaker.JEMEtSumsLocation  = "JEMEtSums"

ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc.JepByteStreamTool.DataVersion = 1
ByteStreamCnvSvc.JepByteStreamTool.DataFormat  = 1
ByteStreamCnvSvc.JepByteStreamTool.SlinksPerCrate = 4
StreamBS = Algorithm( "StreamBS" )
StreamBS.ItemList  += [ "1321678566#*" ]
