//############################################################################
//### Name:        ShowerDirectionCheater                                  ###
//### Author:      Ed Tyley                                                ###
//### Date:        16.07.19                                                ###
//### Description: Cheating tool using truth for shower direction          ###
//############################################################################

//Framework Includes
#include "art/Utilities/ToolMacros.h"

//LArSoft Includes
#include "larpandora/LArPandoraEventBuilding/LArPandoraShower/Tools/IShowerTool.h"
#include "larpandora/LArPandoraEventBuilding/LArPandoraShower/Algs/LArPandoraShowerCheatingAlg.h"
#include "lardataobj/RecoBase/Cluster.h"

namespace ShowerRecoTools {

  class ShowerDirectionCheater:IShowerTool {

    public:

      ShowerDirectionCheater(const fhicl::ParameterSet& pset);

      //Generic Direction Finder
      int CalculateElement(const art::Ptr<recob::PFParticle>& pfparticle,
          art::Event& Event,
          reco::shower::ShowerElementHolder& ShowerEleHolder
          ) override;

    private:

      TVector3 ShowerPCAVector(std::vector<art::Ptr<recob::SpacePoint> >& spacePoints_pfp, art::FindManyP<recob::Hit>& fmh, TVector3& ShowerCentre);
      double RMSShowerGradient(std::vector<art::Ptr<recob::SpacePoint> >& sps, TVector3& ShowerCentre, TVector3& Direction);
      double CalculateRMS(std::vector<float> perps);

      //Algorithm functions
      shower::LArPandoraShowerCheatingAlg fLArPandoraShowerCheatingAlg;

      //Services
      art::ServiceHandle<art::TFileService> tfs;

      //fcl
      art::InputTag fPFParticleLabel;
      float fNSegments; //Number of segement to split the shower into the perforam the RMSFlip.
      bool fRMSFlip;    //Flip the direction by considering the rms.
      bool fVertexFlip; //Flip the direction by considering the vertex position relative to the center position.

      //TTree Branch variables
      TTree* Tree;
      float vertexDotProduct;
      float rmsGradient;

      std::string fShowerStartPositionInputLabel;
      std::string fTrueParticleInputLabel;
      std::string fShowerDirectionOutputLabel;

  };


  ShowerDirectionCheater::ShowerDirectionCheater(const fhicl::ParameterSet& pset) :
    IShowerTool(pset.get<fhicl::ParameterSet>("BaseTools")),
    fLArPandoraShowerCheatingAlg(pset.get<fhicl::ParameterSet>("LArPandoraShowerCheatingAlg")),
    fPFParticleLabel(pset.get<art::InputTag>("PFParticleLabel")),
    fNSegments(pset.get<float>("NSegments")),
    fRMSFlip(pset.get<bool>("RMSFlip")),
    fVertexFlip(pset.get<bool>("VertexFlip")),
    fShowerStartPositionInputLabel(pset.get<std::string>("ShowerStartPositionInputLabel")),
    fTrueParticleInputLabel(pset.get<std::string>("TrueParticleInputLabel")),
    fShowerDirectionOutputLabel(pset.get<std::string>("ShowerDirectionOutputLabel"))
  {
    if (vertexDotProduct||rmsGradient){
      Tree = tfs->make<TTree>("DebugTreeDirCheater", "DebugTree from shower direction cheater");
      if (fVertexFlip) Tree->Branch("vertexDotProduct",&vertexDotProduct);
      if (fRMSFlip)    Tree->Branch("rmsGradient",&rmsGradient);
    }
  }

  int ShowerDirectionCheater::CalculateElement(const art::Ptr<recob::PFParticle>& pfparticle,
      art::Event& Event,
      reco::shower::ShowerElementHolder& ShowerEleHolder){


    const simb::MCParticle* trueParticle;

    //Get the hits from the shower:
    auto const pfpHandle = Event.getValidHandle<std::vector<recob::PFParticle> >(fPFParticleLabel);

    auto const clockData = art::ServiceHandle<detinfo::DetectorClocksService const>()->DataFor(Event);
    auto const detProp   = art::ServiceHandle<detinfo::DetectorPropertiesService const>()->DataFor(Event, clockData);

    if (ShowerEleHolder.CheckElement(fTrueParticleInputLabel)){
      ShowerEleHolder.GetElement(fTrueParticleInputLabel,trueParticle);
    } else {

      //Could store these in the shower element holder and just calculate once?
      std::map<int,const simb::MCParticle*> trueParticles = fLArPandoraShowerCheatingAlg.GetTrueParticleMap();
      std::map<int,std::vector<int> > showersMothers = fLArPandoraShowerCheatingAlg.GetTrueChain(trueParticles);

      //Get the clusters
      auto const clusHandle = Event.getValidHandle<std::vector<recob::Cluster> >(fPFParticleLabel);
      art::FindManyP<recob::Cluster> fmc(pfpHandle, Event, fPFParticleLabel);
      std::vector<art::Ptr<recob::Cluster> > clusters = fmc.at(pfparticle.key());

      //Get the hit association
      art::FindManyP<recob::Hit> fmhc(clusHandle, Event, fPFParticleLabel);

      std::vector<art::Ptr<recob::Hit> > showerHits;
      for(auto const& cluster: clusters){
        //Get the hits
        std::vector<art::Ptr<recob::Hit> > hits = fmhc.at(cluster.key());
        showerHits.insert(showerHits.end(),hits.begin(),hits.end());
      }

      //Get the true particle from the shower
      std::pair<int,double> ShowerTrackInfo = fLArPandoraShowerCheatingAlg.TrueParticleIDFromTrueChain(clockData,
          showersMothers,showerHits,2);

      if(ShowerTrackInfo.first==-99999) {
        mf::LogError("ShowerDirectionCheater") << "True shower not found, returning";
        return 1;
      }
      trueParticle = trueParticles[ShowerTrackInfo.first];
    }

    TVector3 trueDir = {trueParticle->Px(),trueParticle->Py(),trueParticle->Pz()};
    trueDir = trueDir.Unit(); // TODO: Can probably remove?

    TVector3 trueDirErr = {-999,-999,-999};
    ShowerEleHolder.SetElement(trueDir,trueDirErr,fShowerDirectionOutputLabel);

    if (fRMSFlip || fVertexFlip){
      //Get the SpacePoints and hits
      art::FindManyP<recob::SpacePoint> fmspp(pfpHandle, Event, fPFParticleLabel);

      if (!fmspp.isValid()){
        throw cet::exception("ShowerDirectionCheater") << "Trying to get the spacepoint and failed. Something is not configured correctly. Stopping ";
      }

      auto const spHandle = Event.getValidHandle<std::vector<recob::SpacePoint> >(fPFParticleLabel);
      art::FindManyP<recob::Hit> fmh(spHandle, Event, fPFParticleLabel);
      if(!fmh.isValid()){
        throw cet::exception("ShowerDirectionCheater") << "Spacepoint and hit association not valid. Stopping.";
      }
      std::vector<art::Ptr<recob::SpacePoint> > spacePoints = fmspp.at(pfparticle.key());

      if (spacePoints.empty()) return 1;

      //Get Shower Centre
      float TotalCharge;

      TVector3 ShowerCentre = IShowerTool::GetLArPandoraShowerAlg().ShowerCentre(clockData, detProp, spacePoints, fmh, TotalCharge);

      //Check if we are pointing the correct direction or not, First try the start position
      if(ShowerEleHolder.CheckElement(fShowerStartPositionInputLabel) && fVertexFlip){

        //Get the General direction as the vector between the start position and the centre
        TVector3 StartPositionVec = {-999, -999, -999};
        ShowerEleHolder.GetElement(fShowerStartPositionInputLabel,StartPositionVec);

        TVector3 GeneralDir       = (ShowerCentre - StartPositionVec).Unit();

        //Dot product
        vertexDotProduct = trueDir.Dot(GeneralDir);

        //If the dotproduct is negative the Direction needs Flipping
        if(vertexDotProduct < 0){
          trueDir[0] = - trueDir[0];
          trueDir[1] = - trueDir[1];
          trueDir[2] = - trueDir[2];
        }

        //ShowerEleHolder.SetShowerDirection(trueDir);
      }

      if (fRMSFlip){
        //Otherwise Check against the RMS of the shower. Method adapated from EMShower Thanks Mike.
        rmsGradient = RMSShowerGradient(spacePoints,ShowerCentre,trueDir);
        if(rmsGradient < 0){

          trueDir[0] = - trueDir[0];
          trueDir[1] = - trueDir[1];
          trueDir[2] = - trueDir[2];
        }

        //ShowerEleHolder.SetShowerDirection(trueDir);
      }
      Tree->Fill();
    }

    return 0;
  }


  //Function to calculate the RMS at segements of the shower and calculate the gradient of this. If negative then the direction is pointing the opposite way to the correct one
  double ShowerDirectionCheater::RMSShowerGradient(std::vector<art::Ptr<recob::SpacePoint> >& sps, TVector3& ShowerCentre, TVector3& Direction){

    //Order the spacepoints
    IShowerTool::GetLArPandoraShowerAlg().OrderShowerSpacePoints(sps,ShowerCentre,Direction);

    //Get the length of the shower.
    double minProj =IShowerTool::GetLArPandoraShowerAlg().SpacePointProjection(sps[0],ShowerCentre,Direction);
    double maxProj =IShowerTool::GetLArPandoraShowerAlg().SpacePointProjection(sps[sps.size()-1],ShowerCentre,Direction);

    double length = (maxProj-minProj);
    double segmentsize = length/fNSegments;

    std::map<int, std::vector<float> > len_segment_map;

    //Split the the spacepoints into segments.
    for(auto const& sp: sps){

      //Get the the projected length
      double len = IShowerTool::GetLArPandoraShowerAlg().SpacePointProjection(sp,ShowerCentre,Direction);

      //Get the length to the projection
      double  len_perp = IShowerTool::GetLArPandoraShowerAlg().SpacePointPerpendicular(sp,ShowerCentre,Direction,len);

      int sg_len = round(len/segmentsize);
      //TODO: look at this:
      //int sg_len = round(len/segmentsize+fNSegments/2); //Add to make positive
      len_segment_map[sg_len].push_back(len_perp);
    }

    int counter = 0;
    float sumx  = 0;
    float sumy  = 0;
    float sumx2 = 0;
    float sumxy = 0;

    //Get the rms of the segments and caclulate the gradient.
    for(auto const& segment: len_segment_map){
      if (segment.second.size()<2) continue;
      float RMS = CalculateRMS(segment.second);
      //Calculate the gradient using regression
      sumx  += segment.first;
      sumy  += RMS;
      sumx2 += segment.first * segment.first;
      sumxy += RMS * segment.first;
      ++counter;
    }

    double RMSgradient = (counter*sumxy - sumx*sumy)/(counter*sumx2 - sumx*sumx);
    return RMSgradient;

  }

  double ShowerDirectionCheater::CalculateRMS(std::vector<float> perps){
    int counter = 0;
    double sum  = 0;
    for (const auto &perp : perps){
      sum= perp*perp;
      ++counter;
    }
    double rms = std::sqrt(sum/(counter-1));

    return rms;
  }

}

DEFINE_ART_CLASS_TOOL(ShowerRecoTools::ShowerDirectionCheater)
