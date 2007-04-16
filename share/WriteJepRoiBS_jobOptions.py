# JEP RoI container to bytestream conversion
theApp.Dlls += [ "TrigT1CaloByteStream" ]
StreamBS = Algorithm( "StreamBS" )
StreamBS.ItemList  += [ "1266611723#*" ]
