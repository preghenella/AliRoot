#ifndef ALIMUONCLUSTERSTOREV1_H
#define ALIMUONCLUSTERSTOREV1_H

/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
* See cxx source for full Copyright notice                               */

// $Id$

/// \ingroup rec
/// \class AliMUONClusterStoreV1
/// \brief Implementation of VClusterStore
/// 
// Author Laurent Aphecetche, Subatech

#ifndef ALIMUONVCLUSTERSTORE_H
#  include "AliMUONVClusterStore.h"
#endif

class TObjArray;
class TClonesArray;

class AliMUONClusterStoreV1 : public AliMUONVClusterStore
{
public:
  AliMUONClusterStoreV1();
  AliMUONClusterStoreV1(const AliMUONClusterStoreV1& rhs);
  AliMUONClusterStoreV1& operator=(const AliMUONClusterStoreV1& rhs);  
  virtual ~AliMUONClusterStoreV1();
  
  virtual AliMUONClusterStoreV1* Create() const { return new AliMUONClusterStoreV1; }
  
  using AliMUONVClusterStore::Add;
  
  virtual Bool_t Add(const AliMUONRawCluster& Cluster);

  /// Whether the Connect(TTree&) method is implemented
  virtual Bool_t CanConnect() const { return kTRUE; }
  
  virtual TIterator* CreateIterator() const;

  virtual TIterator* CreateChamberIterator(Int_t firstChamberId, Int_t lastChamberId) const;
  
  virtual Bool_t Connect(TTree& tree, Bool_t alone=kTRUE) const;

  virtual void Clear(Option_t* opt="");
  
  using AliMUONVClusterStore::GetSize;
  
  virtual Int_t GetSize() const;

  virtual AliMUONRawCluster* Remove(AliMUONRawCluster& cluster);

private:

  TClonesArray* ChamberClusters(Int_t chamberId) const;
  TObject** ChamberClustersPtr(Int_t chamberId) const;
  //AliMUONRawCluster* Find(Int_t clusterId, Int_t& index) const;
  
private:
  TObjArray* fClusters; //!< Array of TClonesArray of RawClusters
  
  ClassDef(AliMUONClusterStoreV1,1) // Implementation of VClusterStore
};

#endif
