/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
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
//_________________________________________________________________________
// Version of AliPHOSv1 which keeps all hits in TreeH
// AddHit, StepManager,and FinishEvent are redefined 
//                  
//*-- Author: Gines MARTINEZ (SUBATECH)


// --- ROOT system ---

#include "TBRIK.h"
#include "TNode.h"
#include "TRandom.h"

// --- Standard library ---

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strstream.h>

// --- AliRoot header files ---

#include "AliPHOSv2.h"
#include "AliPHOSHit.h"
#include "AliPHOSDigit.h"
#include "AliPHOSReconstructioner.h"
#include "AliRun.h"
#include "AliConst.h"

ClassImp(AliPHOSv2)

//____________________________________________________________________________
AliPHOSv2::AliPHOSv2()
{
  // default ctor
  fNTmpHits = 0 ; 
  fTmpHits  = 0 ; 
}

//____________________________________________________________________________
AliPHOSv2::AliPHOSv2(const char *name, const char *title):
  AliPHOSv1(name,title)
{
  // ctor
   fHits= new TClonesArray("AliPHOSHit",1000) ;
}

//____________________________________________________________________________
AliPHOSv2::~AliPHOSv2()
{
  // dtor
  if ( fTmpHits) {
    fTmpHits->Delete() ; 
    delete fTmpHits ;
    fTmpHits = 0 ; 
  }
  
  if ( fEmcRecPoints ) { 
    fEmcRecPoints->Delete() ; 
    delete fEmcRecPoints ; 
    fEmcRecPoints = 0 ; 
  }
  
  if ( fPpsdRecPoints ) { 
    fPpsdRecPoints->Delete() ;
    delete fPpsdRecPoints ;
    fPpsdRecPoints = 0 ; 
  }
 
  if ( fTrackSegments ) {
    fTrackSegments->Delete() ; 
    delete fTrackSegments ;
    fTrackSegments = 0 ; 
  }
}

//____________________________________________________________________________
void AliPHOSv2::AddHit(Int_t shunt, Int_t primary, Int_t tracknumber, Int_t Id, Float_t * hits)
{
  // Add a hit to the hit list.
  // In this version of AliPHOSv1, a PHOS hit is real geant 
  // hits in a single crystal or in a single PPSD gas cell

  TClonesArray &ltmphits = *fHits ;
  AliPHOSHit *newHit ;
  
  //  fHits->Print("");
  
  newHit = new AliPHOSHit(shunt, primary, tracknumber, Id, hits) ;
  
  // We DO want to save in TreeH the raw hits 
  new(ltmphits[fNhits]) AliPHOSHit(*newHit) ;
  fNhits++ ;

  // Please note that the fTmpHits array must survive up to the
  // end of the events, so it does not appear e.g. in ResetHits() (
  // which is called at the end of each primary).  

  delete newHit;

}




//___________________________________________________________________________
void AliPHOSv2::FinishEvent()
{
  // Makes the digits from the sum of summed hit in a single crystal or PPSD gas cell
  // Adds to the energy the electronic noise
  // Keeps digits with energy above fDigitThreshold

  // Save the cumulated hits instead of raw hits (need to create the branch myself)
  // It is put in the Digit Tree because the TreeH is filled after each primary
  // and the TreeD at the end of the event.
  
  Int_t i ;
  Int_t relid[4];
  Int_t j ; 
  TClonesArray &lDigits = *fDigits ;
  AliPHOSHit  * hit ;
  AliPHOSDigit * newdigit ;
  AliPHOSDigit * curdigit ;
  Bool_t deja = kFALSE ; 
  
  for ( i = 0 ; i < fNhits ; i++ ) {
    hit = (AliPHOSHit*)fHits->At(i) ;
    newdigit = new AliPHOSDigit( hit->GetPrimary(), hit->GetId(), Digitize( hit->GetEnergy() ) ) ;
    deja =kFALSE ;
    for ( j = 0 ; j < fNdigits ;  j++) { 
      curdigit = (AliPHOSDigit*) lDigits[j] ;
      if ( *curdigit == *newdigit) {
	*curdigit = *curdigit + *newdigit ; 
	deja = kTRUE ; 
      }
    }
    if ( !deja ) {
      new(lDigits[fNdigits]) AliPHOSDigit(* newdigit) ;
      fNdigits++ ;  
    }
 
    delete newdigit ;    
  } 
  
  // Noise induced by the PIN diode of the PbWO crystals

  Float_t energyandnoise ;
  for ( i = 0 ; i < fNdigits ; i++ ) {
    newdigit =  (AliPHOSDigit * ) fDigits->At(i) ;
    fGeom->AbsToRelNumbering(newdigit->GetId(), relid) ;

    if (relid[1]==0){   // Digits belong to EMC (PbW0_4 crystals)
      energyandnoise = newdigit->GetAmp() + Digitize(gRandom->Gaus(0., fPinElectronicNoise)) ;

      if (energyandnoise < 0 ) 
	energyandnoise = 0 ;

      if ( newdigit->GetAmp() < fDigitThreshold ) // if threshold not surpassed, remove digit from list
	fDigits->RemoveAt(i) ; 
    }
  }
  
  fDigits->Compress() ;  

  fNdigits =  fDigits->GetEntries() ; 
  for (i = 0 ; i < fNdigits ; i++) { 
    newdigit = (AliPHOSDigit *) fDigits->At(i) ; 
    newdigit->SetIndexInList(i) ; 
  }

}

void AliPHOSv2::StepManager(void)
{
  // Accumulates hits as long as the track stays in a single crystal or PPSD gas Cell

  Int_t          relid[4] ;      // (box, layer, row, column) indices
  Float_t        xyze[4] ;       // position wrt MRS and energy deposited
  TLorentzVector pos ;
  Int_t copy ;

  Int_t tracknumber =  gAlice->CurrentTrack() ; 
  Int_t primary     =  gAlice->GetPrimary( gAlice->CurrentTrack() );

  TString name = fGeom->GetName() ; 
  if ( name == "GPS2" ) { // the CPV is a PPSD
    if( gMC->CurrentVolID(copy) == gMC->VolId("GCEL") ) // We are inside a gas cell 
    {
      gMC->TrackPosition(pos) ;
      xyze[0] = pos[0] ;
      xyze[1] = pos[1] ;
      xyze[2] = pos[2] ;
      xyze[3] = gMC->Edep() ; 

      if ( xyze[3] != 0 ) { // there is deposited energy 
       	gMC->CurrentVolOffID(5, relid[0]) ;  // get the PHOS Module number
       	gMC->CurrentVolOffID(3, relid[1]) ;  // get the Micromegas Module number 
      // 1-> Geom->GetNumberOfModulesPhi() *  fGeom->GetNumberOfModulesZ() upper                         
      //  >  fGeom->GetNumberOfModulesPhi()  *  fGeom->GetNumberOfModulesZ() lower
       	gMC->CurrentVolOffID(1, relid[2]) ;  // get the row number of the cell
        gMC->CurrentVolID(relid[3]) ;        // get the column number 

	// get the absolute Id number

	Int_t absid ; 
       	fGeom->RelToAbsNumbering(relid, absid) ; 

	// add current hit to the hit list      
	AddHit(fIshunt, primary, tracknumber, absid, xyze);

      } // there is deposited energy 
     } // We are inside the gas of the CPV  
   } // GPS2 configuration
  
   if(gMC->CurrentVolID(copy) == gMC->VolId("PXTL") )  //  We are inside a PBWO crystal
     {
       gMC->TrackPosition(pos) ;
       xyze[0] = pos[0] ;
       xyze[1] = pos[1] ;
       xyze[2] = pos[2] ;
       xyze[3] = gMC->Edep() ;

       if ( xyze[3] != 0 ) {
          gMC->CurrentVolOffID(10, relid[0]) ; // get the PHOS module number ;
          relid[1] = 0   ;                    // means PBW04
          gMC->CurrentVolOffID(4, relid[2]) ; // get the row number inside the module
          gMC->CurrentVolOffID(3, relid[3]) ; // get the cell number inside the module

      // get the absolute Id number

          Int_t absid ; 
          fGeom->RelToAbsNumbering(relid, absid) ; 
 
      // add current hit to the hit list

          AddHit(fIshunt, primary,tracknumber, absid, xyze);
    
       } // there is deposited energy
    } // we are inside a PHOS Xtal
}

