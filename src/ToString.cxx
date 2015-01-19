#include "ToString.h"
#include <sstream>


namespace {

	template<typename T>
		std::string ToString(const std::vector<T>& vv){
		std::stringstream o;
		std::string delim = "";
		for(auto val: vv){
			o << delim << int(val);
			delim ="|";
		}
		return o.str();
	}
}

std::string LVL1BS::ToString(const xAOD::TriggerTower& tt){
	std::stringstream o;
	o << "xAOD::TriggerTower, coolId=" << std::hex << tt.coolId() << std::dec
	  << ", layer=" << int(tt.layer())
	  << ", eta=" << tt.eta()
	  << ", phi=" << tt.phi()
	  << ", lut_cp=" << ::ToString(tt.lut_cp())
	  << ", lut_jep=" << ::ToString(tt.lut_jep())
	  << ", correction=" << ::ToString(tt.correction())
	  << ", correctionEnabled=" << ::ToString(tt.correctionEnabled())
	  << ", bcidVec=" << ::ToString(tt.bcidVec())
	  << ", adc=" << ::ToString(tt.adc())
	  << ", bcidExt=" << ::ToString(tt.bcidExt())
	  << ", error=" << tt.error()
	  << ", peak=" << int(tt.peak())
	  << ", adcPeak=" << int(tt.adcPeak())
	  << ", cpET=" << int(tt.cpET())
	  << ", jepET=" << int(tt.jepET())
	  ;

	return o.str();
}

std::string LVL1BS::ToString(const xAOD::TriggerTowerContainer& tt){
	std::stringstream o;
	for(xAOD::TriggerTowerContainer::const_iterator iter = tt.begin();
			iter != tt.end(); ++iter){
		o << ToString(**iter) << std::endl;
	}
	return o.str();
}
