# Test CP bytestream to CPMTower/... conversion
include ( "TrigT1CaloByteStream/ReadCpBS_jobOptions.py" )
theApp.TopAlg += [ "LVL1BS::CpmTester/CpmTester" ]
