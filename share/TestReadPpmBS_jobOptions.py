# Test PPM bytestream to TriggerTower conversion
include ( "TrigT1CaloByteStream/ReadPpmBS_jobOptions.py" )
theApp.TopAlg += [ "PpmTester" ]
PpmTester = Algorithm( "PpmTester" )
PpmTester.TriggerTowerLocation ="LVL1TriggerTowers"
