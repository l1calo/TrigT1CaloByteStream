#ifndef TRIGT1CALOBYTESTREAM_PPMSORTPERMUTATIONS_H
#define TRIGT1CALOBYTESTREAM_PPMSORTPERMUTATIONS_H

#include <stdint.h>
#include <vector>
#include <map>

/** Sort permutations for PPM compressed formats
 *
 *  Note: this is just a possible version not the real thing.
 *  Based on ROD document version 1.06d.
 *
 *  @author Peter Faulkner
 */

class PpmSortPermutations {

 public:
   PpmSortPermutations();
   ~PpmSortPermutations();

   /// Return permutation code for given permutation vector
   int permutationCode(const std::vector<int>& perm);

   /// Return permutation vector for given permutation code
   void permutationVector(int code, std::vector<int>& perm);

   /// Return the total number of permutations for a given number of slices
   int totalPerms(int nslice);

 private:

   /// Generate permutation maps for a given number of slices
   void genPermMaps(int nslice);

   /// Recursive function to generate all permutations of slices
   void permutations(int pos);

   //  Temporaries for permutations generator
   int m_nslices;
   int m_permCount;
   std::vector<int> m_permVector;
   std::map<int, uint32_t>* m_mapP2V;
   std::map<uint32_t, int>* m_mapV2P;

   /// Permutation map: code to vector
   std::map<int, std::map<int, uint32_t>*> m_codeToVec;
   /// Permutation map: vector to code
   std::map<int, std::map<uint32_t, int>*> m_vecToCode;

};

#endif
