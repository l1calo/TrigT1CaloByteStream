
#include <utility>

#include "TrigT1CaloByteStream/PpmSortPermutations.h"

PpmSortPermutations::PpmSortPermutations()
{
}

PpmSortPermutations::~PpmSortPermutations()
{
  // Delete permutation maps
  std::map<int, std::map<int, uint32_t>*>::iterator pos = m_codeToVec.begin();
  std::map<int, std::map<int, uint32_t>*>::iterator pose = m_codeToVec.end();
  for (; pos != pose; ++pos) delete pos->second;
  std::map<int, std::map<uint32_t, int>*>::iterator vos = m_vecToCode.begin();
  std::map<int, std::map<uint32_t, int>*>::iterator vose = m_vecToCode.end();
  for (; vos != vose; ++vos) delete vos->second;
}

// Generate permutation maps for a given number of slices

void PpmSortPermutations::genPermMaps(int nslice)
{
  if (m_codeToVec.find(nslice) == m_codeToVec.end()) {
    m_nslices = nslice;
    m_permVector.resize(nslice);
    m_permCount = 0;
    m_mapP2V = new std::map<int, uint32_t>;
    m_mapV2P = new std::map<uint32_t, int>;
    permutations(0);
    m_codeToVec.insert(std::make_pair(nslice, m_mapP2V));
    m_vecToCode.insert(std::make_pair(nslice, m_mapV2P));
  }
}

// Recursive function to generate all permutations of slices

void PpmSortPermutations::permutations(int pos)
{
  for (int i = 0; i < m_nslices; ++i) {
    bool found = false;
    for (int j = 0; j < pos; ++j) {
      if (m_permVector[j] == i) {
        found = true;
	break;
      }
    }
    if (found) continue;
    m_permVector[pos] = i;
    if (pos < m_nslices-1) permutations(pos+1);
    else {
      uint32_t ivec = 0;
      for (int n = 0; n < m_nslices; ++n) {
        ivec = ivec*m_nslices + m_permVector[n];
      }
      m_mapP2V->insert(std::make_pair(m_permCount, ivec));
      m_mapV2P->insert(std::make_pair(ivec, m_permCount));
      ++m_permCount;
    }
  }
}

// Return permutation code for given permutation vector

int PpmSortPermutations::permutationCode(const std::vector<int>& perm)
{
  int code = 0;
  int nslice = perm.size();
  if (nslice > 0) {
    genPermMaps(nslice);
    uint32_t ivec = 0;
    std::vector<int>::const_iterator pos = perm.begin();
    for (; pos != perm.end(); ++pos) ivec = ivec*nslice + *pos;
    std::map<int, std::map<uint32_t, int>*>::const_iterator mpos;
    mpos = m_vecToCode.find(nslice);
    if (mpos != m_vecToCode.end()) {
      std::map<uint32_t, int>* vmap = mpos->second;
      std::map<uint32_t, int>::const_iterator vpos = vmap->find(ivec);
      if (vpos != vmap->end()) code = vpos->second;
    }
  }
  return code;
}

// Return permutation vector for given permutation code

void PpmSortPermutations::permutationVector(int code, std::vector<int>& perm)
{
  int nslice = perm.size();
  if (nslice > 0 ) {
    genPermMaps(nslice);
    std::map<int, std::map<int, uint32_t>*>::const_iterator mpos;
    mpos = m_codeToVec.find(nslice);
    if (mpos != m_codeToVec.end()) {
      std::map<int, uint32_t>* vmap = mpos->second;
      std::map<int, uint32_t>::const_iterator vpos = vmap->find(code);
      if (vpos != vmap->end()) {
        uint32_t ivec = vpos->second;
        std::vector<int>::reverse_iterator pos = perm.rbegin();
	for (; pos != perm.rend(); ++pos) {
	  *pos = ivec%nslice;
	  ivec /= nslice;
        }
      }
    }
  }
}

// Return the total number of permutations for a given number of slices

int PpmSortPermutations::totalPerms(int nslice) const
{
  // It's just factorial(nslice)
  int total = 1;
  for (int i = 2; i <= nslice; ++i) total *= i;
  return total;
}
