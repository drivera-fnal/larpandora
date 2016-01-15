/**
 *  @file  larpandora/LArPandoraInterface/LArPandoraHelper.h
 *
 *  @brief helper function for LArPandora producer modules
 *
 */
#ifndef LAR_PANDORA_HELPER_H
#define LAR_PANDORA_HELPER_H

// Framework includes
#include "art/Persistency/Common/Ptr.h" 

// LArSoft includes
#include "lardata/RecoBase/Track.h"
#include "lardata/RecoBase/Cluster.h"
#include "lardata/RecoBase/SpacePoint.h"
#include "lardata/RecoBase/Hit.h"
#include "larreco/RecoAlg/ClusterRecoUtil/ClusterParamsAlgBase.h"

// Pandora includes
#include "Objects/ParticleFlowObject.h"
#include "LArObjects/LArTrackPfo.h"

#include <vector>
#include <set>

namespace lar_pandora
{

class LArPandoraHelper
{
public:
    /**
     *  @brief Build a recob::Cluster object from an input vector of recob::Hit objects
     *
     *  @param id  The id code for the cluster
     *  @param hitVector  The input vector of hits
     *  @param hitList    The input list of isolated hits
     *  @param algo Algorithm set to fill cluster members
     *  
     *  If you don't know which algorithm to pick, StandardClusterParamsAlg is
     *  a good default.
     *  
     *  The hits that are isolated (that is, present in isolatedHits) are not
     *  fed to the cluster parameter algorithms.
     */
    static recob::Cluster BuildCluster(const int id, const std::vector<art::Ptr<recob::Hit>> &hitVector,
        const std::set<art::Ptr<recob::Hit>> &hitList, cluster::ClusterParamsAlgBase& algo);

    /**
     *  @brief Build a recob::Track object from an input Pandora particle flow object
     *
     *  @param id  The id code for the track
     *  @param pPfo  The particle flow object
     */
    static recob::Track BuildTrack(const int id, const pandora::ParticleFlowObject *const pPfo);

    /**
     *  @brief Merge a set of recob::Track objects
     *
     *  @param id  The id code for the tracks
     *  @param trackStateVector  The vector of trajectory points for this track
     */
    static recob::Track BuildTrack(const int id, const lar_content::LArTrackStateVector &trackStateVector);
};

} // namespace lar_pandora

#endif //  LAR_PANDORA_HELPER_H