//
//  Monophonify.cpp
//  modularSynth
//
//  Created by Ryan Challinor on 12/12/12.
//
//

#include "Monophonify.h"
#include "OpenFrameworksPort.h"
#include "ModularSynth.h"
#include "UIControlMacros.h"

Monophonify::Monophonify()
: mLastPlayedPitch(-1)
, mLastVelocity(0)
, mRequireHeldNote(false)
, mGlideTime(0)
, mGlideSlider(nullptr)
{
   for (int i=0; i<128; ++i)
      mHeldNotes[i] = -1;
}

void Monophonify::CreateUIControls()
{
   IDrawableModule::CreateUIControls();
   UIBLOCK0();
   CHECKBOX(mRequireHeldNoteCheckbox, "require held", &mRequireHeldNote);
   FLOATSLIDER(mGlideSlider, "glide", &mGlideTime, 0, 1000);
   ENDUIBLOCK(mWidth, mHeight);
   
   mGlideSlider->SetMode(FloatSlider::kSquare);
}

void Monophonify::DrawModule()
{
   if (Minimized() || IsVisible() == false)
      return;
   
   mRequireHeldNoteCheckbox->Draw();
   mGlideSlider->Draw();
}

void Monophonify::PlayNote(double time, int pitch, int velocity, int voiceIdx, ModulationParameters modulation)
{
   if (!mEnabled)
   {
      PlayNoteOutput(time, pitch, velocity, voiceIdx, modulation);
      return;
   }
   
   if (pitch < 0 || pitch >= 128)
      return;
   
   mPitchBend.AppendTo(modulation.pitchBend);
   modulation.pitchBend = &mPitchBend;

   voiceIdx = 0;
   
   if (velocity > 0)
   {
      mLastVelocity = velocity;
      
      int mostRecentPitch = GetMostRecentPitch();
      
      if (mostRecentPitch != -1)
      {
         mPitchBend.RampValue(time, mostRecentPitch - pitch + mPitchBend.GetIndividualValue(0), 0, mGlideTime);
         PlayNoteOutput(time,mostRecentPitch, 0, -1, modulation);
         PlayNoteOutput(time, pitch, velocity, voiceIdx, modulation);
      }
      else
      {
         if (!mRequireHeldNote && mLastPlayedPitch != -1)
            mPitchBend.RampValue(time, mLastPlayedPitch - pitch + mPitchBend.GetIndividualValue(0), 0, mGlideTime);
         else
            mPitchBend.RampValue(time, 0, 0, 0);

         PlayNoteOutput(time, pitch, velocity, voiceIdx, modulation);
      }
      mLastPlayedPitch = pitch;
      mHeldNotes[pitch] = time;
   }
   else
   {
      bool wasCurrNote = pitch == GetMostRecentPitch();
      
      mHeldNotes[pitch] = -1;
      
      int mostRecentPitch = GetMostRecentPitch();
      
      if (mostRecentPitch != -1)
      {
         if (wasCurrNote)
         {
            mPitchBend.RampValue(time, pitch - mostRecentPitch + mPitchBend.GetIndividualValue(0), 0, mGlideTime);
            PlayNoteOutput(time, mostRecentPitch, mLastVelocity, voiceIdx, modulation);
         }
      }
      else
      {
         PlayNoteOutput(time, pitch, 0, voiceIdx, modulation);
      }
   }
}

int Monophonify::GetMostRecentPitch() const
{
   int mostRecentPitch = -1;
   double mostRecentTime = 0;
   for (int i=0; i<128; ++i)
   {
      if (mHeldNotes[i] > mostRecentTime)
      {
         mostRecentPitch = i;
         mostRecentTime = mHeldNotes[i];
      }
   }

   return mostRecentPitch;
}

void Monophonify::CheckboxUpdated(Checkbox* checkbox)
{
   if (checkbox == mEnabledCheckbox)
   {
      mNoteOutput.Flush(gTime);
      for (int i=0; i<128; ++i)
         mHeldNotes[i] = -1;
   }
}

void Monophonify::FloatSliderUpdated(FloatSlider* slider, float oldVal)
{
}

void Monophonify::LoadLayout(const ofxJSONElement& moduleInfo)
{
   mModuleSaveData.LoadString("target", moduleInfo);

   SetUpFromSaveData();
}

void Monophonify::SetUpFromSaveData()
{
   SetUpPatchCables(mModuleSaveData.GetString("target"));
}

