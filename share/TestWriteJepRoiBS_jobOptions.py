# Test JEP RoI Container to bytestream conversion
include ( "TrigT1CaloByteStream/WriteJepRoiBS_jobOptions.py" )
theApp.TopAlg += [ "LVL1BS::JemTester/JemTester" ]
