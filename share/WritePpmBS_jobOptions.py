# TriggerTower to PPM bytestream conversion
theApp.Dlls += [ "TrigT1CaloByteStream" ]
StreamBS = Algorithm( "StreamBS" )
StreamBS.ItemList  += [ "6207#*" ]
