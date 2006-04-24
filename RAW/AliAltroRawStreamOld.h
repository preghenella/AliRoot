#ifndef ALIALTRORAWSTREAMOLD_H
#define ALIALTRORAWSTREAMOLD_H
/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

///////////////////////////////////////////////////////////////////////////////
///
/// This is a class for reading raw data digits in Altro format
/// OLD RCU version
/// The RCU trailer is just one 32-bit word
/// The Common Data Header is 7 32-bit words and is skipped during
/// the raw data reading.
/// The raw data payload is little endian.
///
///////////////////////////////////////////////////////////////////////////////

#include <TObject.h>

class AliRawReader;

class AliAltroRawStreamOld: public TObject {
  public :
    AliAltroRawStreamOld(AliRawReader* rawReader);
    virtual ~AliAltroRawStreamOld();

    virtual void             Reset();
    virtual Bool_t           Next();

    inline Int_t GetDDLNumber()  const { return fDDLNumber; }  // Provide current DDL number
    inline Int_t GetPrevDDLNumber() const { return fPrevDDLNumber; }// Provide previous DDL number
    inline Bool_t  IsNewDDLNumber() const {return (fDDLNumber != fPrevDDLNumber);};
    inline Int_t GetHWAddress()  const { return fHWAddress; }  // Provide current hardware address
    inline Int_t GetPrevHWAddress() const { return fPrevHWAddress; }  // Provide previous hardware address
    inline Bool_t  IsNewHWAddress() const {return (fHWAddress != fPrevHWAddress) || IsNewDDLNumber();};
    inline Int_t GetTime()       const { return fTime; }       // Provide index of current time bin
    inline Int_t GetPrevTime()   const { return fPrevTime; }   // Provide index of previous time bin
    inline Bool_t  IsNewTime()   const {return (fTime != fPrevTime) || IsNewHWAddress();};
    inline Int_t GetSignal()     const { return fSignal; }     // Provide signal in ADC counts
    inline Int_t GetTimeLength() const { return fTimeBunch; }  // Provide total length of current time bunch

    void SelectRawData(Int_t detId);                           // Select raw data for specific detector id

    inline void  SetNoAltroMapping(Bool_t flag) { fNoAltroMapping = flag; }  // Specify whenever to use or not the altro mapping

  protected:
    AliAltroRawStreamOld(const AliAltroRawStreamOld& stream);
    AliAltroRawStreamOld& operator = (const AliAltroRawStreamOld& stream);

    Bool_t           fNoAltroMapping;  // temporary flag in case of no altro mapping is provided
    Short_t          fSegmentation[3]; // temporary container for the dummy trailer, to be removed

  private :

    UShort_t         GetNextWord();
    Bool_t           ReadTrailer();
    Bool_t           ReadDummyTrailer();
    void             ReadBunch();
    void             ReadAmplitude();
    Int_t            GetPosition();

    Int_t            fDDLNumber;    // index of current DDL number
    Int_t            fPrevDDLNumber;// index of current DDL number
    Short_t          fHWAddress;    // current hardware address
    Short_t          fPrevHWAddress;// previous hardware address
    Int_t            fTime;         // index of current time bin
    Int_t            fPrevTime;     // index of previous time bin
    Int_t            fSignal;       // signal in ADC counts
    Int_t            fTimeBunch;    // total length of the current time bunch

    AliRawReader*    fRawReader;    // object for reading the raw data


    UChar_t*         fData;         // raw data

    Int_t            fPosition;     // current (10 bit) position in fData
    Int_t            fCount;        // counter of words to be read for current trailer
    Int_t            fBunchLength;  // remaining number of signal bins in the current bunch

    ClassDef(AliAltroRawStreamOld, 0)  // base class for reading Altro raw digits
};

#endif
