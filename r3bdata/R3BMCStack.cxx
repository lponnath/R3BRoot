// -------------------------------------------------------------------------
// -----                       R3BStack source file                    -----
// -----             Created 10/08/04  by D. Bertini / V. Friese       -----
// -------------------------------------------------------------------------
#include "R3BMCStack.h"

#include "FairDetector.h"
#include "FairMCPoint.h"
#include "R3BMCTrack.h"
#include "FairRootManager.h"

#include "TError.h"
#include "TLorentzVector.h"
#include "TParticle.h"
#include "TRefArray.h"

#include <list>
#include <iostream>

using std::cout;
using std::endl;
using std::pair;


// -----   Default constructor   -------------------------------------------
R3BStack::R3BStack(Int_t size) {
  fParticles = new TClonesArray("TParticle", size);
  fTracks    = new TClonesArray("R3BMCTrack", size);
  fCurrentTrack = -1;
  fNPrimaries = fNParticles = fNTracks = 0;
  fIndex = 0;
  fStoreSecondaries = kTRUE;
  fMinPoints        = 0;
  fEnergyCut        = 0.;
  fStoreMothers     = kTRUE;
  fDebug            = kFALSE;
}

// -------------------------------------------------------------------------



// -----   Destructor   ----------------------------------------------------
R3BStack::~R3BStack() {
  if (fParticles) {
    fParticles->Delete();
    delete fParticles;
  }
  if (fTracks) {
    fTracks->Delete();
    delete fTracks;
  }
}
// -------------------------------------------------------------------------

  

// -----   Virtual public method PushTrack   -------------------------------
void R3BStack::PushTrack(Int_t toBeDone, Int_t parentId, Int_t pdgCode,
			 Double_t px, Double_t py, Double_t pz,
			 Double_t e, Double_t vx, Double_t vy, Double_t vz, 
			 Double_t time, Double_t polx, Double_t poly,
			 Double_t polz, TMCProcess proc, Int_t& ntr, 
			 Double_t weight, Int_t is) {

  // --> Get TParticle array
  TClonesArray& partArray = *fParticles;

  // --> Create new TParticle and add it to the TParticle array
  Int_t trackId = fNParticles;
  Int_t nPoints = 0;
  Int_t daughter1Id = -1;
  Int_t daughter2Id = -1;
  TParticle* particle = 
    new(partArray[fNParticles++]) TParticle(pdgCode, trackId, parentId, 
					    nPoints, daughter1Id, 
					    daughter2Id, px, py, pz, e, 
					    vx, vy, vz, time);
  particle->SetPolarisation(polx, poly, polz);
  particle->SetWeight(weight);
  particle->SetUniqueID(proc);

  // --> Increment counter
  if (parentId < 0) fNPrimaries++;

  // --> Set argument variable
  ntr = trackId;

  // --> Push particle on the stack if toBeDone is set
  if (toBeDone == 1) fStack.push(particle);

}
// -------------------------------------------------------------------------

  

// -----   Virtual method PopNextTrack   -----------------------------------
TParticle* R3BStack::PopNextTrack(Int_t& iTrack) {

  // If end of stack: Return empty pointer
  if (fStack.empty()) {
    iTrack = -1;
    return NULL;
  }

  // If not, get next particle from stack
  TParticle* thisParticle = fStack.top();
  fStack.pop();

  if ( !thisParticle) {
    iTrack = 0;
    return NULL;
  }

  fCurrentTrack = thisParticle->GetStatusCode();
  iTrack = fCurrentTrack;

  return thisParticle;

}
// -------------------------------------------------------------------------

  

// -----   Virtual method PopPrimaryForTracking   --------------------------
TParticle* R3BStack::PopPrimaryForTracking(Int_t iPrim) {

  // Get the iPrimth particle from the fStack TClonesArray. This
  // should be a primary (if the index is correct).

  // Test for index
  if (iPrim < 0 || iPrim >= fNPrimaries) {
    cout << "-E- R3BStack: Primary index out of range! " << iPrim << endl;
    Fatal("R3BStack::PopPrimaryForTracking", "Index out of range");
  }

  // Return the iPrim-th TParticle from the fParticle array. This should be
  // a primary.
  TParticle* part = (TParticle*)fParticles->At(iPrim);
  if ( ! (part->GetMother(0) < 0) ) {
    cout << "-E- R3BStack:: Not a primary track! " << iPrim << endl;
    Fatal("R3BStack::PopPrimaryForTracking", "Not a primary track");
  }

  return part;

}
// -------------------------------------------------------------------------



// -----   Virtual public method GetCurrentTrack   -------------------------
TParticle* R3BStack::GetCurrentTrack() const {
  TParticle* currentPart = GetParticle(fCurrentTrack);
  if ( ! currentPart) {
    cout << "-W- R3BStack: Current track not found in stack!" << endl;
    Warning("R3BStack::GetCurrentTrack", "Track not found in stack");
  }
  return currentPart;
}
// -------------------------------------------------------------------------


  
// -----   Public method AddParticle   -------------------------------------
void R3BStack::AddParticle(TParticle* oldPart) {
  TClonesArray& array = *fParticles;
  TParticle* newPart = new(array[fIndex]) TParticle(*oldPart);
  newPart->SetWeight(oldPart->GetWeight());
  newPart->SetUniqueID(oldPart->GetUniqueID());
  fIndex++;
}
// -------------------------------------------------------------------------



// -----   Public method FillTrackArray   ----------------------------------
void R3BStack::FillTrackArray() {

  cout << "-I- R3BStack: Filling MCTrack array..." << endl;

  // --> Reset index map and number of output tracks
  fIndexMap.clear();
  fNTracks = 0;

  //<DB> if no selection than no selection
  /*
  if ( fMinPoints == 0 ) {
     
   for (Int_t iPart=0; iPart<fNParticles; iPart++) {
      R3BMCTrack* track = 
	new( (*fTracks)[fNTracks]) R3BMCTrack(GetParticle(iPart));
      fNTracks++;
    }
   cout << "-I- R3BStack: no select. MCTracks forwarded : " << fNParticles << endl; 
   return;
  }//! fMinPoints
  */
   
  // --> Check tracks for selection criteria
  SelectTracks();

  // --> Loop over fParticles array and copy selected tracks
  for (Int_t iPart=0; iPart<fNParticles; iPart++) {

    fStoreIter = fStoreMap.find(iPart);
    if (fStoreIter == fStoreMap.end() ) {
      cout << "-E- R3BStack: Particle " << iPart 
	   << " not found in storage map!" << endl;
      Fatal("R3BStack::FillTrackArray",
	    "Particle not found in storage map.");
    }
    Bool_t store = (*fStoreIter).second;

    if (store) {
      R3BMCTrack* track = 
	new( (*fTracks)[fNTracks]) R3BMCTrack(GetParticle(iPart));
      fIndexMap[iPart] = fNTracks;
      // --> Set the number of points in the detectors for this track
      for (Int_t iDet=kCAL; iDet<=kTRA; iDet++) {
	pair<Int_t, Int_t> a(iPart, iDet);
	track->SetNPoints(iDet, fPointsMap[a]);
      }
      fNTracks++;
    
    }else{
      if (fDebug) cout << "-D- R3BMCStack IndexMap ---> -2 for iPart: " << iPart << endl;    
      fIndexMap[iPart] = -2;
    }

  }

  // --> Map index for primary mothers
  fIndexMap[-1] = -1;

  // --> Screen output
  Print(0);

}
// -------------------------------------------------------------------------



// -----   Public method UpdateTrackIndex   --------------------------------
void R3BStack::UpdateTrackIndex(TRefArray* detList) {

  if ( fMinPoints == 0 ) return;  
  cout << "-I- R3BStack: Updating track indizes...";
  Int_t nColl = 0;

  // First update mother ID in MCTracks
  for (Int_t i=0; i<fNTracks; i++) {
    R3BMCTrack* track = (R3BMCTrack*)fTracks->At(i);
    Int_t iMotherOld = track->GetMotherID();
    fIndexIter = fIndexMap.find(iMotherOld);
    if (fIndexIter == fIndexMap.end()) {
      cout << "-E- R3BStack: Particle index " << iMotherOld 
	   << " not found in dex map! " << endl;
      Fatal("R3BStack::UpdateTrackIndex",
		"Particle index not found in map");
    }
    track->SetMotherID( (*fIndexIter).second );
  }

  // Now iterate through all active detectors
  TIterator* detIter = detList->MakeIterator();
  detIter->Reset();
  FairDetector* det = NULL;
  while( (det = (FairDetector*)detIter->Next() ) ) {

    // --> Get hit collections from detector
    Int_t iColl = 0;
    TClonesArray* hitArray;
    while ( (hitArray = det->GetCollection(iColl++)) ) {
      nColl++;
      Int_t nPoints = hitArray->GetEntriesFast();
      
      // --> Update track index for all MCPoints in the collection
      for (Int_t iPoint=0; iPoint<nPoints; iPoint++) {
	FairMCPoint* point = (FairMCPoint*)hitArray->At(iPoint);
	Int_t iTrack = point->GetTrackID();

	if (fDebug) cout << "-D- R3BMCStack TrackID Get : " << iTrack << endl;

	fIndexIter = fIndexMap.find(iTrack);
	if (fIndexIter == fIndexMap.end()) {
	  cout << "-E- R3BStack: Particle index " << iTrack 
	       << " not found in index map! " << endl;
	  Fatal("R3BStack::UpdateTrackIndex",
		"Particle index not found in map");
	}
	if (fDebug) cout << "-D- R3BMCStack TrackID Set : " << (*fIndexIter).second << endl; 
	if ( ((*fIndexIter).second ) < 0 ) {
	   point->SetTrackID(iTrack);
	}else{
	   point->SetTrackID((*fIndexIter).second);
	}

      }

    }   // Collections of this detector
  }     // List of active detectors

  cout << "...stack and " << nColl << " collections updated." << endl;

}
// -------------------------------------------------------------------------



// -----   Public method Reset   -------------------------------------------
void R3BStack::Reset() {
  fIndex = 0;
  fCurrentTrack = -1;
  fNPrimaries = fNParticles = fNTracks = 0;
  while (! fStack.empty() ) fStack.pop();
  fParticles->Clear();
  fTracks->Clear();
  fPointsMap.clear();
}
// -------------------------------------------------------------------------



// -----   Public method Register   ----------------------------------------
void R3BStack::Register() {
  FairRootManager::Instance()->Register("MCTrack", "Stack", fTracks,kTRUE);
}
// -------------------------------------------------------------------------



// -----   Public method Print  --------------------------------------------
void R3BStack::Print(Int_t iVerbose) const {
  cout << "-I- R3BStack: Number of primaries        = " 
       << fNPrimaries << endl;
  cout << "              Total number of particles  = " 
       << fNParticles << endl;
  cout << "              Number of tracks in output = "
       << fNTracks << endl;
  if (iVerbose) {
    for (Int_t iTrack=0; iTrack<fNTracks; iTrack++) 
      ((R3BMCTrack*) fTracks->At(iTrack))->Print(iTrack);
  }
}
// -------------------------------------------------------------------------



// -----   Public method AddPoint (for current track)   --------------------
void R3BStack::AddPoint(DetectorId detId) {
  Int_t iDet = detId;
  pair<Int_t, Int_t> a(fCurrentTrack, iDet);
  if ( fPointsMap.find(a) == fPointsMap.end() ) fPointsMap[a] = 1;
  else fPointsMap[a]++;
}
// -------------------------------------------------------------------------



// -----   Public method AddPoint (for arbitrary track)  -------------------
void R3BStack::AddPoint(DetectorId detId, Int_t iTrack) {
  if ( iTrack < 0 ) return;
  Int_t iDet = detId;
  pair<Int_t, Int_t> a(iTrack, iDet);
  if ( fPointsMap.find(a) == fPointsMap.end() ) fPointsMap[a] = 1;
  else fPointsMap[a]++;
}
// -------------------------------------------------------------------------




// -----   Virtual method GetCurrentParentTrackNumber   --------------------
Int_t R3BStack::GetCurrentParentTrackNumber() const {
  TParticle* currentPart = GetCurrentTrack();
  if ( currentPart ) return currentPart->GetFirstMother();
  else               return -1;
}
// -------------------------------------------------------------------------



// -----   Public method GetParticle   -------------------------------------
TParticle* R3BStack::GetParticle(Int_t trackID) const {
  if (trackID < 0 || trackID >= fNParticles) {
    cout << "-E- R3BStack: Particle index " << trackID 
	 << " out of range." << endl;
    Fatal("R3BStack::GetParticle", "Index out of range");
  }
  return (TParticle*)fParticles->At(trackID);
}
// -------------------------------------------------------------------------



// -----   Private method SelectTracks   -----------------------------------
void R3BStack::SelectTracks() {

  // --> Clear storage map
  fStoreMap.clear();

  // --> Check particles in the fParticle array
  for (Int_t i=0; i<fNParticles; i++) {

    TParticle* thisPart = GetParticle(i);
    Bool_t store = kTRUE;

    // --> Get track parameters
    Int_t iMother   = thisPart->GetMother(0);
    TLorentzVector p;
    thisPart->Momentum(p);
    Double_t energy = p.E();
    Double_t mass   = thisPart->GetMass();
    Double_t eKin = energy - mass;
    if(eKin < 0.0) eKin=0.0; // sometimes due to different PDG masses between ROOT and G4!!!!!!
    // --> Calculate number of points
    Int_t nPoints = 0;
    for (Int_t iDet=kCAL; iDet<=kTRA; iDet++) {
      pair<Int_t, Int_t> a(i, iDet);
      if ( fPointsMap.find(a) != fPointsMap.end() )
	nPoints += fPointsMap[a];
    }

    // --> Check for cuts (store primaries in any case)
    if (iMother < 0)            store = kTRUE;
    else {
      if (!fStoreSecondaries)   store = kFALSE;
      if (nPoints < fMinPoints) store = kFALSE;
      if (eKin < fEnergyCut)    store = kFALSE;
    }

    // --> Set storage flag
    fStoreMap[i] = store;

  }

  // --> If flag is set, flag recursively mothers of selected tracks
  if (fStoreMothers) {
    for (Int_t i=0; i<fNParticles; i++) {
      if (fStoreMap[i]) {
	Int_t iMother = GetParticle(i)->GetMother(0);
	while(iMother >= 0) {
	  fStoreMap[iMother] = kTRUE;
	  iMother = GetParticle(iMother)->GetMother(0);
	}
      }
    }
  }

}
// -------------------------------------------------------------------------



ClassImp(R3BStack)
