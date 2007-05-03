#include <cmath>

#include "TrigT1CaloByteStream/ChannelCoordinate.h"
#include "TrigT1CaloByteStream/CpmCrateMappings.h"

namespace LVL1BS {

// Static constants

const int    CpmCrateMappings::s_crates;
const int    CpmCrateMappings::s_modules;
const int    CpmCrateMappings::s_channels;

const int    CpmCrateMappings::s_etaBinsPerRow;

const double CpmCrateMappings::s_etaGran = 0.1;
const double CpmCrateMappings::s_phiGran = M_PI/32.;


CpmCrateMappings::CpmCrateMappings()
{
}

// Return eta, phi mapping for given crate/module/channel.

bool CpmCrateMappings::mapping(const int crate, int module, const int channel,
                                          ChannelCoordinate& coord) const
{
  --module; // CPM modules are numbered 1 to s_modules
  if (crate < 0 || crate >= s_crates || module < 0 || module >= s_modules ||
      channel < 0 || channel >= s_channels) return false;

  // Channels numbered thus:
  //
  //  :    :    :    :    :
  //  |  9 | 11 | 13 | 15 |
  //  |  8 | 10 | 12 | 14 | phi
  //  |  1 |  3 |  5 |  7 |
  //  |  0 |  2 |  4 |  6 |
  //  +----+----+----+----+
  //           eta
  const int etaBin = (channel / 2) % s_etaBinsPerRow;
  const int phiBin = ((channel / 2) / s_etaBinsPerRow) * 2
                                    + channel % 2 - 2;  // allow for overlap

  // End modules only have one column (Is that right?)
  if ((module == 0 && etaBin != s_etaBinsPerRow - 1) ||
      (module == s_modules - 1 && etaBin != 0)) return false;

  const double phiBase = M_PI/2. * double(crate);
  double phi           = phiBase + s_phiGran * (double(phiBin) + 0.5);
  if (phi < 0.) phi += 2.*M_PI;

  const double etaBase = s_etaGran * s_etaBinsPerRow * (module - s_modules/2);
  const double eta     = etaBase + s_etaGran * (double(etaBin) + 0.5);
 
  coord.setEta(eta);
  coord.setPhi(phi);
  coord.setEtaGranularity(s_etaGran);
  coord.setPhiGranularity(s_phiGran);
  return true;
}

int CpmCrateMappings::crates()
{
  return s_crates;
}

int CpmCrateMappings::modules()
{
  return s_modules;
}

int CpmCrateMappings::channels()
{
  return s_channels;
}

} // end namespace
