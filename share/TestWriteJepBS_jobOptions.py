# Test JEP Container to bytestream conversion
include ( "TrigT1CaloByteStream/WriteJepBS_jobOptions.py" )
theApp.TopAlg += [ "JemTester" ]
JemTester = Algorithm( "JemTester" )
JemTester.JetElementLocation = "LVL1JetElements"
