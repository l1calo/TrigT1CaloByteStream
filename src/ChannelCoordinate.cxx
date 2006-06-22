
#include "TrigT1CaloByteStream/ChannelCoordinate.h"

ChannelCoordinate::ChannelCoordinate() : m_layer(NONE), m_eta(0.0), m_phi(0.0),
                                         m_etaGran(0.0), m_phiGran(0.0)
{
}

ChannelCoordinate::ChannelCoordinate(CaloLayer layer, double eta, double phi,
                                     double etaGran, double phiGran) :
 m_layer(layer), m_eta(eta), m_phi(phi), m_etaGran(etaGran), m_phiGran(phiGran)
{
}

std::string ChannelCoordinate::layerName(CaloLayer layer)
{
  std::string name;
  switch (layer) {
    case ChannelCoordinate::NONE:  name = "NONE";  break;
    case ChannelCoordinate::EM:    name = "EM";    break;
    case ChannelCoordinate::HAD:   name = "HAD";   break;
    case ChannelCoordinate::FCAL2: name = "FCAL2"; break;
    case ChannelCoordinate::FCAL3: name = "FCAL3"; break;
    default:                       name = "????";  break;
  }
  return name;
}
