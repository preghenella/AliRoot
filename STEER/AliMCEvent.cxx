/**************************************************************************
 * Copyright(c) 1998-2007, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

/* $Id$ */

//-------------------------------------------------------------------------
//     Class for Kinematic Events
//     Author: Andreas Morsch, CERN
//-------------------------------------------------------------------------
#include <TArrow.h>
#include <TMarker.h>
#include <TH2F.h>
#include <TTree.h>
#include <TFile.h>
#include <TParticle.h>
#include <TClonesArray.h>
#include <TRefArray.h>
#include <TList.h>
#include <TArrayF.h>

#include "AliLog.h"
#include "AliMCEvent.h"
#include "AliMCVertex.h"
#include "AliStack.h"
#include "AliTrackReference.h"
#include "AliHeader.h"
#include "AliGenEventHeader.h"


Int_t AliMCEvent::fgkBgLabelOffset(10000000);


AliMCEvent::AliMCEvent():
    AliVEvent(),
    fStack(0),
    fMCParticles(0),
    fMCParticleMap(0),
    fHeader(new AliHeader()),
    fTRBuffer(0),
    fTrackReferences(new TClonesArray("AliTrackReference", 1000)),
    fTreeTR(0),
    fTmpTreeTR(0),
    fTmpFileTR(0),
    fNprimaries(-1),
    fNparticles(-1),
    fSubsidiaryEvents(0),
    fPrimaryOffset(0),
    fSecondaryOffset(0),
    fExternal(0),
    fVertex(0)
{
    // Default constructor
}

AliMCEvent::AliMCEvent(const AliMCEvent& mcEvnt) :
    AliVEvent(mcEvnt),
    fStack(mcEvnt.fStack),
    fMCParticles(mcEvnt.fMCParticles),
    fMCParticleMap(mcEvnt.fMCParticleMap),
    fHeader(mcEvnt.fHeader),
    fTRBuffer(mcEvnt.fTRBuffer),
    fTrackReferences(mcEvnt.fTrackReferences),
    fTreeTR(mcEvnt.fTreeTR),
    fTmpTreeTR(mcEvnt.fTmpTreeTR),
    fTmpFileTR(mcEvnt.fTmpFileTR),
    fNprimaries(mcEvnt.fNprimaries),
    fNparticles(mcEvnt.fNparticles),
    fSubsidiaryEvents(0),
    fPrimaryOffset(0),
    fSecondaryOffset(0),
    fExternal(0),
    fVertex(mcEvnt.fVertex)
{ 
// Copy constructor
}


AliMCEvent& AliMCEvent::operator=(const AliMCEvent& mcEvnt)
{
    // assignment operator
    if (this!=&mcEvnt) { 
	AliVEvent::operator=(mcEvnt); 
    }
  
    return *this; 
}

void AliMCEvent::ConnectTreeE (TTree* tree)
{
    // Connect the event header tree
    tree->SetBranchAddress("Header", &fHeader);
}

void AliMCEvent::ConnectTreeK (TTree* tree)
{
    // Connect the kinematics tree to the stack
    if (!fMCParticles) fMCParticles = new TClonesArray("AliMCParticle",1000);
    //
    fStack = fHeader->Stack();
    fStack->ConnectTree(tree);
    //
    // Load the event
    fStack->GetEvent();
    fNparticles = fStack->GetNtrack();
    fNprimaries = fStack->GetNprimary();
    Int_t iev  = fHeader->GetEvent();
    Int_t ievr = fHeader->GetEventNrInRun();
    
    AliInfo(Form("AliMCEvent# %5d %5d: Number of particles: %5d (all) %5d (primaries)\n", 
		 iev, ievr, fNparticles, fNprimaries));
 
    // This is a cache for the TParticles converted to MCParticles on user request
    if (fMCParticleMap) {
	fMCParticleMap->Clear();
	fMCParticles->Delete();
	if (fNparticles>0) fMCParticleMap->Expand(fNparticles);
    }
    else
	fMCParticleMap = new TRefArray(fNparticles);
}

void AliMCEvent::ConnectTreeTR (TTree* tree)
{
    // Connect the track reference tree
    fTreeTR = tree;
    
    if (fTreeTR->GetBranch("AliRun")) {
	if (fTmpFileTR) {
	    fTmpFileTR->Close();
	    delete fTmpFileTR;
	}
	// This is an old format with one branch per detector not in synch with TreeK
	ReorderAndExpandTreeTR();
    } else {
	// New format 
	fTreeTR->SetBranchAddress("TrackReferences", &fTRBuffer);
    }
}

Int_t AliMCEvent::GetParticleAndTR(Int_t i, TParticle*& particle, TClonesArray*& trefs)
{
    // Retrieve entry i
    if (i < 0 || i >= fNparticles) {
	AliWarning(Form("AliMCEventHandler::GetEntry: Index out of range"));
	particle = 0;
	trefs    = 0;
	return (-1);
    }
    particle = fStack->Particle(i);
    if (fTreeTR) {
	fTreeTR->GetEntry(fStack->TreeKEntry(i));
	trefs    = fTRBuffer;
	return trefs->GetEntries();
    } else {
	trefs = 0;
	return -1;
    }
}


void AliMCEvent::Clean()
{
    // Clean-up before new trees are connected
    delete fStack; fStack = 0;

    // Clear TR
    if (fTRBuffer) {
	fTRBuffer->Delete();
	delete fTRBuffer;
	fTRBuffer = 0;
    }
}

void AliMCEvent::FinishEvent()
{
  // Clean-up after event
  //    
    fStack->Reset(0);
    fMCParticles->Delete();
    fMCParticleMap->Clear();
    if (fTRBuffer)
      fTRBuffer->Delete();
    fTrackReferences->Delete();
    fNparticles = -1;
    fNprimaries = -1;    
    fStack      =  0;
//    fSubsidiaryEvents->Clear();
    fSubsidiaryEvents = 0;
}



void AliMCEvent::DrawCheck(Int_t i, Int_t search)
{
    //
    // Simple event display for debugging
    if (!fTreeTR) {
	AliWarning("No Track Reference information available");
	return;
    } 
    
    if (i > -1 && i < fNparticles) {
	fTreeTR->GetEntry(fStack->TreeKEntry(i));
    } else {
	AliWarning("AliMCEvent::GetEntry: Index out of range");
    }
    
    Int_t nh = fTRBuffer->GetEntries();
    
    
    if (search) {
	while(nh <= search && i < fNparticles - 1) {
	    i++;
	    fTreeTR->GetEntry(fStack->TreeKEntry(i));
	    nh =  fTRBuffer->GetEntries();
	}
	printf("Found Hits at %5d\n", i);
    }
    TParticle* particle = fStack->Particle(i);
    
    TH2F*    h = new TH2F("", "", 100, -500, 500, 100, -500, 500);
    Float_t x0 = particle->Vx();
    Float_t y0 = particle->Vy();

    Float_t x1 = particle->Vx() + particle->Px() * 50.;
    Float_t y1 = particle->Vy() + particle->Py() * 50.;
    
    TArrow*  a = new TArrow(x0, y0, x1, y1, 0.01);
    h->Draw();
    a->SetLineColor(2);
    
    a->Draw();
    
    for (Int_t ih = 0; ih < nh; ih++) {
	AliTrackReference* ref = (AliTrackReference*) fTRBuffer->At(ih);
	TMarker* m = new TMarker(ref->X(), ref->Y(), 20);
	m->Draw();
	m->SetMarkerSize(0.4);
	
    }
}


void AliMCEvent::ReorderAndExpandTreeTR()
{
//
//  Reorder and expand the track reference tree in order to match the kinematics tree.
//  Copy the information from different branches into one
//
//  TreeTR

    fTmpFileTR = new TFile("TrackRefsTmp.root", "recreate");
    fTmpTreeTR = new TTree("TreeTR", "TrackReferences");
    if (!fTRBuffer)  fTRBuffer = new TClonesArray("AliTrackReference", 100);
    fTmpTreeTR->Branch("TrackReferences", "TClonesArray", &fTRBuffer, 32000, 0);
    

//
//  Activate the used branches only. Otherwisw we get a bad memory leak.
    fTreeTR->SetBranchStatus("*",        0);
    fTreeTR->SetBranchStatus("AliRun.*", 1);
    fTreeTR->SetBranchStatus("ITS.*",    1);
    fTreeTR->SetBranchStatus("TPC.*",    1);
    fTreeTR->SetBranchStatus("TRD.*",    1);
    fTreeTR->SetBranchStatus("TOF.*",    1);
    fTreeTR->SetBranchStatus("FRAME.*",  1);
    fTreeTR->SetBranchStatus("MUON.*",   1);
//
//  Connect the active branches
    TClonesArray* trefs[7];
    for (Int_t i = 0; i < 7; i++) trefs[i] = 0;
    if (fTreeTR){
	// make branch for central track references
	if (fTreeTR->GetBranch("AliRun")) fTreeTR->SetBranchAddress("AliRun", &trefs[0]);
	if (fTreeTR->GetBranch("ITS"))    fTreeTR->SetBranchAddress("ITS",    &trefs[1]);
	if (fTreeTR->GetBranch("TPC"))    fTreeTR->SetBranchAddress("TPC",    &trefs[2]);
	if (fTreeTR->GetBranch("TRD"))    fTreeTR->SetBranchAddress("TRD",    &trefs[3]);
	if (fTreeTR->GetBranch("TOF"))    fTreeTR->SetBranchAddress("TOF",    &trefs[4]);
	if (fTreeTR->GetBranch("FRAME"))  fTreeTR->SetBranchAddress("FRAME",  &trefs[5]);
	if (fTreeTR->GetBranch("MUON"))   fTreeTR->SetBranchAddress("MUON",   &trefs[6]);
    }

    Int_t np = fStack->GetNprimary();
    Int_t nt = fTreeTR->GetEntries();
    
    //
    // Loop over tracks and find the secondaries with the help of the kine tree
    Int_t ifills = 0;
    Int_t it     = 0;
    Int_t itlast = 0;
    TParticle* part;

    for (Int_t ip = np - 1; ip > -1; ip--) {
	part = fStack->Particle(ip);
//	printf("Particle %5d %5d %5d %5d %5d %5d \n", 
//	       ip, part->GetPdgCode(), part->GetFirstMother(), part->GetFirstDaughter(), 
//	       part->GetLastDaughter(), part->TestBit(kTransportBit));

	// Determine range of secondaries produced by this primary during transport	
	Int_t dau1  = part->GetFirstDaughter();
	if (dau1 < np) continue;  // This particle has no secondaries produced during transport
	Int_t dau2  = -1;
	if (dau1 > -1) {
	    Int_t inext = ip - 1;
	    while (dau2 < 0) {
		if (inext >= 0) {
		    part = fStack->Particle(inext);
		    dau2 =  part->GetFirstDaughter();
		    if (dau2 == -1 || dau2 < np) {
			dau2 = -1;
		    } else {
			dau2--;
		    }
		} else {
		    dau2 = fStack->GetNtrack() - 1;
		}
		inext--;
	    } // find upper bound
	}  // dau2 < 0
	

//	printf("Check (1) %5d %5d %5d %5d %5d \n", ip, np, it, dau1, dau2);
//
// Loop over reference hits and find secondary label
// First the tricky part: find the entry in treeTR than contains the hits or
// make sure that no hits exist.
//
	Bool_t hasHits   = kFALSE;
	Bool_t isOutside = kFALSE;

	it = itlast;
	while (!hasHits && !isOutside && it < nt) {
	    fTreeTR->GetEntry(it++);
	    for (Int_t ib = 0; ib < 7; ib++) {
		if (!trefs[ib]) continue;
		Int_t nh = trefs[ib]->GetEntries();
		for (Int_t ih = 0; ih < nh; ih++) {
		    AliTrackReference* tr = (AliTrackReference*) trefs[ib]->At(ih);
		    Int_t label = tr->Label();
		    if (label >= dau1 && label <= dau2) {
			hasHits = kTRUE;
			itlast = it - 1;
			break;
		    }
		    if (label > dau2 || label < ip) {
			isOutside = kTRUE;
			itlast = it - 1;
			break;
		    }
		} // hits
		if (hasHits || isOutside) break;
	    } // branches
	} // entries

	if (!hasHits) {
	    // Write empty entries
	    for (Int_t id = dau1; (id <= dau2); id++) {
		fTmpTreeTR->Fill();
		ifills++;
	    } 
	} else {
	    // Collect all hits
	    fTreeTR->GetEntry(itlast);
	    for (Int_t id = dau1; (id <= dau2) && (dau1 > -1); id++) {
		for (Int_t ib = 0; ib < 7; ib++) {
		    if (!trefs[ib]) continue;
		    Int_t nh = trefs[ib]->GetEntries();
		    for (Int_t ih = 0; ih < nh; ih++) {
			AliTrackReference* tr = (AliTrackReference*) trefs[ib]->At(ih);
			Int_t label = tr->Label();
			// Skip primaries
			if (label == ip) continue;
			if (label > dau2 || label < dau1) 
			    printf("AliMCEventHandler::Track Reference Label out of range !: %5d %5d %5d %5d \n", 
				   itlast, label, dau1, dau2);
			if (label == id) {
			    // secondary found
			    tr->SetDetectorId(ib-1);
			    Int_t nref =  fTRBuffer->GetEntriesFast();
			    TClonesArray &lref = *fTRBuffer;
			    new(lref[nref]) AliTrackReference(*tr);
			}
		    } // hits
		} // branches
		fTmpTreeTR->Fill();
		fTRBuffer->Delete();
		ifills++;
	    } // daughters
	} // has hits
    } // tracks

    //
    // Now loop again and write the primaries
    //
    it = nt - 1;
    for (Int_t ip = 0; ip < np; ip++) {
	Int_t labmax = -1;
	while (labmax < ip && it > -1) {
	    fTreeTR->GetEntry(it--);
	    for (Int_t ib = 0; ib < 7; ib++) {
		if (!trefs[ib]) continue;
		Int_t nh = trefs[ib]->GetEntries();
		// 
		// Loop over reference hits and find primary labels
		for (Int_t ih = 0; ih < nh; ih++) {
		    AliTrackReference* tr = (AliTrackReference*)  trefs[ib]->At(ih);
		    Int_t label = tr->Label();
		    if (label < np && label > labmax) {
			labmax = label;
		    }
		    
		    if (label == ip) {
			tr->SetDetectorId(ib-1);
			Int_t nref = fTRBuffer->GetEntriesFast();
			TClonesArray &lref = *fTRBuffer;
			new(lref[nref]) AliTrackReference(*tr);
		    }
		} // hits
	    } // branches
	} // entries
	it++;
	fTmpTreeTR->Fill();
	fTRBuffer->Delete();
	ifills++;
    } // tracks
    // Check


    // Clean-up
    delete fTreeTR; fTreeTR = 0;
    
    for (Int_t ib = 0; ib < 7; ib++) {
	if (trefs[ib]) {
	    trefs[ib]->Clear();
	    delete trefs[ib];
	    trefs[ib] = 0;
	}
    }

    if (ifills != fStack->GetNtrack()) 
	printf("AliMCEvent:Number of entries in TreeTR (%5d) unequal to TreeK (%5d) \n", 
	       ifills, fStack->GetNtrack());

    fTmpTreeTR->Write();
    fTreeTR = fTmpTreeTR;
}

AliVParticle* AliMCEvent::GetTrack(Int_t i) const
{
    // Get MC Particle i
    //

    if (fExternal) {
	return ((AliVParticle*) (fMCParticles->At(i)));
    }
    
    //
    // Check first if this explicitely accesses the subsidiary event
    
    if (i >= BgLabelOffset()) {
	if (fSubsidiaryEvents) {
	    AliMCEvent* bgEvent = (AliMCEvent*) (fSubsidiaryEvents->At(1));
	    return (bgEvent->GetTrack(i - BgLabelOffset()));
	} else {
	    return 0;
	}
    }
    
    //
    AliMCParticle *mcParticle = 0;
    TParticle     *particle   = 0;
    TClonesArray  *trefs      = 0;
    Int_t          ntref      = 0;
    TRefArray     *rarray     = 0;



    // Out of range check
    if (i < 0 || i >= fNparticles) {
	AliWarning(Form("AliMCEvent::GetEntry: Index out of range"));
	mcParticle = 0;
	return (mcParticle);
    }

    
    if (fSubsidiaryEvents) {
	AliMCEvent*   mc;
	Int_t idx = FindIndexAndEvent(i, mc);
	return (mc->GetTrack(idx));
    } 

    //
    // First check If the MC Particle has been already cached
    if(!fMCParticleMap->At(i)) {
	// Get particle from the stack
	particle   = fStack->Particle(i);
	// Get track references from Tree TR
	if (fTreeTR) {
	    fTreeTR->GetEntry(fStack->TreeKEntry(i));
	    trefs     = fTRBuffer;
	    ntref     = trefs->GetEntriesFast();
	    rarray    = new TRefArray(ntref);
	    Int_t nen = fTrackReferences->GetEntriesFast();
	    for (Int_t j = 0; j < ntref; j++) {
		// Save the track references in a TClonesArray
		AliTrackReference* ref = dynamic_cast<AliTrackReference*>((*fTRBuffer)[j]);
		// Save the pointer in a TRefArray
		new ((*fTrackReferences)[nen]) AliTrackReference(*ref);
		rarray->AddAt((*fTrackReferences)[nen], j);
		nen++;
	    } // loop over track references for entry i
	} // if TreeTR available
	Int_t nentries = fMCParticles->GetEntriesFast();
	new ((*fMCParticles)[nentries]) AliMCParticle(particle, rarray, i);
	mcParticle = dynamic_cast<AliMCParticle*>((*fMCParticles)[nentries]);
	fMCParticleMap->AddAt(mcParticle, i);

	TParticle* part = mcParticle->Particle();
	Int_t imo  = part->GetFirstMother();
	Int_t id1  = part->GetFirstDaughter();
	Int_t id2  = part->GetLastDaughter();
	if (fPrimaryOffset > 0 || fSecondaryOffset > 0) {
	    // Remapping of the mother and daughter indices
	    if (imo < fNprimaries) {
		mcParticle->SetMother(imo + fPrimaryOffset);
	    } else {
		mcParticle->SetMother(imo + fSecondaryOffset - fNprimaries);
	    }

	    if (id1 < fNprimaries) {
		mcParticle->SetFirstDaughter(id1 + fPrimaryOffset);
		mcParticle->SetLastDaughter (id2 + fPrimaryOffset);
	    } else {
		mcParticle->SetFirstDaughter(id1 + fSecondaryOffset - fNprimaries);
		mcParticle->SetLastDaughter (id2 + fSecondaryOffset - fNprimaries);
	    }

	    
	    if (i > fNprimaries) {
		mcParticle->SetLabel(i + fPrimaryOffset);
	    } else {
		mcParticle->SetLabel(i + fSecondaryOffset - fNprimaries);
	    }
	    
	    
	} else {
	    mcParticle->SetFirstDaughter(id1);
	    mcParticle->SetLastDaughter (id2);
	    mcParticle->SetMother       (imo);
	}

    } else {
	mcParticle = dynamic_cast<AliMCParticle*>(fMCParticleMap->At(i));
    }
    
    
    return mcParticle;
}

AliGenEventHeader* AliMCEvent::GenEventHeader() const {return (fHeader->GenEventHeader());}


void AliMCEvent::AddSubsidiaryEvent(AliMCEvent* event) 
{
    // Add a subsidiary event to the list; for example merged background event.
    if (!fSubsidiaryEvents) {
	TList* events = new TList();
	events->Add(new AliMCEvent(*this));
	fSubsidiaryEvents = events;
    }
    
    fSubsidiaryEvents->Add(event);
}

Int_t AliMCEvent::FindIndexAndEvent(Int_t oldidx, AliMCEvent*& event) const
{
    // Find the index and event in case of composed events like signal + background
    TIter next(fSubsidiaryEvents);
    next.Reset();
     if (oldidx < fNprimaries) {
	while((event = (AliMCEvent*)next())) {
	    if (oldidx < (event->GetPrimaryOffset() + event->GetNumberOfPrimaries())) break;
	}
	return (oldidx - event->GetPrimaryOffset());
    } else {
	while((event = (AliMCEvent*)next())) {
	    if (oldidx < (event->GetSecondaryOffset() + (event->GetNumberOfTracks() - event->GetNumberOfPrimaries()))) break;
	}
	return (oldidx - event->GetSecondaryOffset() + event->GetNumberOfPrimaries());
    }
}

Int_t AliMCEvent::BgLabelToIndex(Int_t label)
{
    // Convert a background label to an absolute index
    if (fSubsidiaryEvents) {
	AliMCEvent* bgEvent = (AliMCEvent*) (fSubsidiaryEvents->At(1));
	label -= BgLabelOffset();
	if (label < bgEvent->GetNumberOfPrimaries()) {
	    label += bgEvent->GetPrimaryOffset();
	} else {
	    label += (bgEvent->GetSecondaryOffset() - fNprimaries);
	}
    }
    return (label);
}


Bool_t AliMCEvent::IsPhysicalPrimary(Int_t i) 
{
//
// Delegate to subevent if necesarry 

    
    if (!fSubsidiaryEvents) {
	return fStack->IsPhysicalPrimary(i);
    } else {
	AliMCEvent* evt = 0;
	Int_t idx = FindIndexAndEvent(i, evt);
	return (evt->IsPhysicalPrimary(idx));
    }
}

void AliMCEvent::InitEvent()
{
//
// Initialize the subsidiary event structure
    if (fSubsidiaryEvents) {
	TIter next(fSubsidiaryEvents);
	AliMCEvent* evt;
	fNprimaries = 0;
	fNparticles = 0;
	
	while((evt = (AliMCEvent*)next())) {
	    fNprimaries += evt->GetNumberOfPrimaries();	
	    fNparticles += evt->GetNumberOfTracks();    
	}
	
	Int_t ioffp = 0;
	Int_t ioffs = fNprimaries;
	next.Reset();
	
	while((evt = (AliMCEvent*)next())) {
	    evt->SetPrimaryOffset(ioffp);
	    evt->SetSecondaryOffset(ioffs);
	    ioffp += evt->GetNumberOfPrimaries();
	    ioffs += (evt->GetNumberOfTracks() - evt->GetNumberOfPrimaries());	    
	}
    }
}
void AliMCEvent::PreReadAll()                              
{
    // Preread the MC information
    Int_t i;
    // secondaries
    for (i = fStack->GetNprimary(); i < fStack->GetNtrack(); i++) 
    {
	GetTrack(i);
    }
    // primaries
    for (i = 0; i < fStack->GetNprimary(); i++) 
    {
	GetTrack(i);
    }
    
    
}


const AliVVertex * AliMCEvent::GetPrimaryVertex() const 
{
    // Create a MCVertex object from the MCHeader information
    TArrayF v;
    GenEventHeader()->PrimaryVertex(v) ;
    if (!fVertex) {
	fVertex = new AliMCVertex(v[0], v[1], v[2]);
    } else {
	((AliMCVertex*) fVertex)->SetPosition(v[0], v[1], v[2]);
    }
    return fVertex;
}


ClassImp(AliMCEvent)
