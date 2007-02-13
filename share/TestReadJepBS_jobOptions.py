# Test JEP bytestream to JetElement/... conversion
include ( "TrigT1CaloByteStream/ReadJepBS_jobOptions.py" )
theApp.TopAlg += [ "JemTester" ]
JemTester = Algorithm( "JemTester" )
JemTester.JetElementLocation = "LVL1JetElements"
