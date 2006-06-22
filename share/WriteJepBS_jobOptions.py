# JepContainer to JEP (JEM+CMMs) bytestream conversion
theApp.Dlls += [ "TrigT1CaloByteStream" ]
theApp.TopAlg += [ "JepContainerMaker" ]
JepMaker = Algorithm( "JepContainerMaker" )
JepMaker.JetElementLocation ="LVL1JetElements"

ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc.JepByteStreamTool.DataVersion = 1
ByteStreamCnvSvc.JepByteStreamTool.DataFormat = 1
ByteStreamCnvSvc.JepByteStreamTool.SlinksPerCrate = 4
StreamBS = Algorithm( "StreamBS" )
StreamBS.ItemList  += [ "1321678566#*" ]
