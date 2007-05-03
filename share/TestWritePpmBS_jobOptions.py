# Test TriggerTower to PPM bytestream conversion
include ( "TrigT1CaloByteStream/WritePpmBS_jobOptions.py" )
theApp.TopAlg += [ "LVL1BS::PpmTester/PpmTester" ]
PpmTester = Algorithm( "PpmTester" )
PpmTester.TriggerTowerLocation ="LVL1TriggerTowers"
