//
//  NoteLooper.h
//  modularSynth
//
//  Created by Ryan Challinor on 3/31/13.
//
//

#pragma once

#include <iostream>
#include "NoteEffectBase.h"
#include "Transport.h"
#include "Checkbox.h"
#include "Slider.h"
#include "IDrawableModule.h"
#include "DropdownList.h"
#include "MidiDevice.h"
#include "Scale.h"
#include "Canvas.h"
#include "CanvasElement.h"
#include "ClickButton.h"

class NoteLooper : public IDrawableModule, public NoteEffectBase, public IAudioPoller, public IFloatSliderListener, public IIntSliderListener, public IDropdownListener, public IButtonListener
{
public:
   NoteLooper();
   ~NoteLooper();
   static IDrawableModule* Create() { return new NoteLooper(); }
   
   string GetTitleLabel() override { return "note looper"; }
   void CreateUIControls() override;
   void SetEnabled(bool enabled) override { mEnabled = enabled; }

   //INoteReceiver
   void PlayNote(double time, int pitch, int velocity, int voiceIdx = -1, ModulationParameters modulation = ModulationParameters()) override;

   //IAudioPoller
   void OnTransportAdvanced(float amount) override;
   
   void CheckboxUpdated(Checkbox* checkbox) override;
   //IFloatSliderListener
   void FloatSliderUpdated(FloatSlider* slider, float oldVal) override;
   //IIntSliderListener
   void IntSliderUpdated(IntSlider* slider, int oldVal) override;
   //IDropdownListener
   void DropdownUpdated(DropdownList* list, int oldVal) override;
   //IButtonListener
   void ButtonClicked(ClickButton* button) override;
   
   virtual void LoadLayout(const ofxJSONElement& moduleInfo) override;
   virtual void SetUpFromSaveData() override;
   
private:
   double GetCurPos(double time) const;
   NoteCanvasElement* AddNote(double measurePos, int pitch, int velocity, double length, int voiceIdx = -1, ModulationParameters modulation = ModulationParameters());
   void SetNumMeasures(int numMeasures);
   int GetNewVoice(int voiceIdx);
   
   //IDrawableModule
   void DrawModule() override;
   void GetModuleDimensions(float& width, float& height) override { width=mWidth; height=mHeight; }
   bool Enabled() const override { return mEnabled; }
   
   struct NoteEvent
   {
      bool mValid;
      float mPos;
      int mPitch;
      int mVelocity;
      int mJustPlaced;
      int mAssociatedEvent;   //associated note on/off
   };
   
   struct CurrentNote
   {
      int mPitch;
      int mVelocity;
   };

   float mWidth;
   float mHeight;
   int mMinRow;
   int mMaxRow;
   bool mWrite;
   Checkbox* mWriteCheckbox;
   bool mDeleteOrMute;
   Checkbox* mDeleteOrMuteCheckbox;
   IntSlider* mNumMeasuresSlider;
   int mNumMeasures;
   vector<CanvasElement*> mNoteChecker {128};
   std::array<NoteCanvasElement*, 128> mInputNotes {};
   std::array<NoteCanvasElement*, 128> mCurrentNotes {};
   Canvas* mCanvas;
   ClickButton* mClearButton;
   int mVoiceRoundRobin;

   std::array<ModulationParameters, kNumVoices+1> mVoiceModulations {};
   std::array<int, kNumVoices> mVoiceMap {};
};

