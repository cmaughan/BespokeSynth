/*
  ==============================================================================

    PlaySequencer.cpp
    Created: 12 Dec 2020 11:00:20pm
    Author:  Ryan Challinor

  ==============================================================================
*/

#include "PlaySequencer.h"
#include "OpenFrameworksPort.h"
#include "SynthGlobals.h"
#include "ModularSynth.h"
#include "UIControlMacros.h"

PlaySequencer::PlaySequencer()
   : mInterval(kInterval_16n)
   , mNumMeasures(1)
   , mWrite(false)
   , mNoteRepeat(false)
   , mLinkColumns(false)
   , mWidth(240)
   , mHeight(225)
   , mUseLightVelocity(false)
   , mUseMedVelocity(false)
   , mClearLane(false)
   , mSustain(false)
   , mGrid(nullptr)
   , mVelocityFull(1)
   , mVelocityMed(.5f)
   , mVelocityLight(.25f)
{
   TheTransport->AddListener(this, mInterval, OffsetInfo(0, true), false);
   TheTransport->AddListener(&mNoteOffScheduler, mInterval, OffsetInfo(TheTransport->GetMeasureFraction(mInterval) * .5f, false), false);

   mNoteOffScheduler.mOwner = this;
}

void PlaySequencer::CreateUIControls()
{
   IDrawableModule::CreateUIControls();

   mGridController = new GridController(this, "grid", mWidth - 50, 4);

   float width, height;
   UIBLOCK0();
   DROPDOWN(mIntervalSelector, "interval", (int*)(&mInterval), 50); UIBLOCK_SHIFTRIGHT();
   DROPDOWN(mNumMeasuresSelector, "measures", &mNumMeasures, 50); UIBLOCK_NEWLINE();
   CHECKBOX(mWriteCheckbox, "write", &mWrite);
   CHECKBOX(mNoteRepeatCheckbox, "note repeat", &mNoteRepeat);
   UIBLOCK_SHIFTRIGHT();
   UIBLOCK_SHIFTUP();
   CHECKBOX(mLinkColumnsCheckbox, "link columns", &mLinkColumns);
   ENDUIBLOCK(width, height);
   mGrid = new UIGrid(3, height, mWidth-16, 150, TheTransport->CountInStandardMeasure(mInterval), mLanes.size(), this);
   mHeight = height + 153;
   mGrid->SetFlip(true);
   mGrid->SetGridMode(UIGrid::kMultislider);
   mGrid->SetRestrictDragToRow(true);

   ofRectangle gridRect = mGrid->GetRect(true);
   for (size_t i = 0; i<mLanes.size(); ++i)
   {
      ofVec2f cellPos = mGrid->GetCellPosition(mGrid->GetCols() - 1, i) + mGrid->GetPosition(true);
      mLanes[i].mMuteOrEraseCheckbox = new Checkbox(this, ("mute/delete"+ofToString(i)).c_str(), gridRect.getMaxX() + 3, cellPos.y+1, &mLanes[i].mMuteOrErase);
      mLanes[i].mMuteOrEraseCheckbox->SetDisplayText(false);
      mLanes[i].mMuteOrEraseCheckbox->SetBoxSize(10);
   }

   mIntervalSelector->AddLabel("4n", kInterval_4n);
   mIntervalSelector->AddLabel("4nt", kInterval_4nt);
   mIntervalSelector->AddLabel("8n", kInterval_8n);
   mIntervalSelector->AddLabel("8nt", kInterval_8nt);
   mIntervalSelector->AddLabel("16n", kInterval_16n);
   mIntervalSelector->AddLabel("16nt", kInterval_16nt);
   mIntervalSelector->AddLabel("32n", kInterval_32n);
   mIntervalSelector->AddLabel("64n", kInterval_64n);

   mNumMeasuresSelector->AddLabel("1", 1);
   mNumMeasuresSelector->AddLabel("2", 2);
   mNumMeasuresSelector->AddLabel("4", 4);
   mNumMeasuresSelector->AddLabel("8", 8);
   mNumMeasuresSelector->AddLabel("16", 16);
}

PlaySequencer::~PlaySequencer()
{
   TheTransport->RemoveListener(this);
   TheTransport->RemoveListener(&mNoteOffScheduler);
}

void PlaySequencer::Init()
{
   IDrawableModule::Init();
}

void PlaySequencer::DrawModule()
{
   if (Minimized() || IsVisible() == false)
      return;

   mGridController->Draw();
   mIntervalSelector->Draw();
   mWriteCheckbox->Draw();
   mNoteRepeatCheckbox->Draw();
   mLinkColumnsCheckbox->Draw();
   mNumMeasuresSelector->Draw();
   for (size_t i = 0; i < mLanes.size(); ++i)
      mLanes[i].mMuteOrEraseCheckbox->Draw();

   mGrid->Draw();

   ofPushStyle();
   ofSetColor(255, 0, 0, 50);
   ofFill();
   for (size_t i = 0; i < mLanes.size(); ++i)
   {
      if (mLanes[i].mMuteOrErase)
      {
         ofRectangle gridRect = mGrid->GetRect(true);
         ofVec2f cellPos = mGrid->GetCellPosition(0, i) + mGrid->GetPosition(true);
         ofRect(cellPos.x, cellPos.y+1, gridRect.width, gridRect.height / mGrid->GetRows());
      }
   }
   ofPopStyle();
}

void PlaySequencer::OnClicked(int x, int y, bool right)
{
   IDrawableModule::OnClicked(x, y, right);

   if (right)
      return;

   mGrid->TestClick(x, y, right);
}

void PlaySequencer::MouseReleased()
{
   IDrawableModule::MouseReleased();
   mGrid->MouseReleased();
}

bool PlaySequencer::MouseMoved(float x, float y)
{
   IDrawableModule::MouseMoved(x, y);
   mGrid->NotifyMouseMoved(x, y);
   return false;
}

void PlaySequencer::CheckboxUpdated(Checkbox* checkbox)
{
   for (size_t i = 0; i < mLanes.size(); ++i)
   {
      if (checkbox == mLanes[i].mMuteOrEraseCheckbox)
      {
         if (mLinkColumns)
         {
            for (size_t j = 0; j < mLanes.size(); ++j)
            {
               if (j % 4 == i % 4)
                  mLanes[j].mMuteOrErase = mLanes[i].mMuteOrErase;
            }
         }
      }
   }
}

void PlaySequencer::PlayNote(double time, int pitch, int velocity, int voiceIdx, ModulationParameters modulation)
{
   if (!mEnabled)
      return;

   if (pitch < mLanes.size())
   {
      if (velocity > 0)
      {
         mLanes[pitch].mInputVelocity = velocity;
      }
      else
      {
         if (mNoteRepeat)
            mLanes[pitch].mInputVelocity = 0;
      }
   }
}

void PlaySequencer::OnTimeEvent(double time)
{
   if (!mEnabled)
      return;

   int step = GetStep(time);

   mGrid->SetHighlightCol(time, step);

   for (size_t i = 0; i < mLanes.size(); ++i)
   {
      float gridVal = mGrid->GetVal(step, i);
      int playVelocity = (int)(gridVal * 127);

      if (mLanes[i].mMuteOrErase)
      {
         playVelocity = 0;
         if (mWrite)
            mGrid->SetVal(step, i, 0);
      }

      if (mLanes[i].mInputVelocity > 0)
      {
         float velMult = 1;
         switch (GetVelocityLevel())
         {
         case 1: velMult = mVelocityLight; break;
         case 2: velMult = mVelocityMed; break;
         case 3: velMult = mVelocityFull; break;
         }
         playVelocity = mLanes[i].mInputVelocity * velMult;
         if (mWrite)
            mGrid->SetVal(step, i, playVelocity / 127.0f);
         if (!mNoteRepeat)
            mLanes[i].mInputVelocity = 0;
      }

      if (playVelocity > 0 && mLanes[i].mIsPlaying == false)
      {
         PlayNoteOutput(time, i, playVelocity);
         mLanes[i].mIsPlaying = true;
      }

      if (mSustain)
      {
         if (mLanes[i].mIsPlaying && playVelocity == 0)
         {
            PlayNoteOutput(time, i, 0);
            mLanes[i].mIsPlaying = false;
         }
      }
   }

   UpdateLights();
}

void PlaySequencer::NoteOffScheduler::OnTimeEvent(double time)
{
   if (mOwner->mSustain)
      return;

   for (size_t i = 0; i < mOwner->mLanes.size(); ++i)
   {
      if (mOwner->mLanes[i].mIsPlaying)
      {
         mOwner->PlayNoteOutput(time, i, 0);
         mOwner->mLanes[i].mIsPlaying = false;
      }
   }

   mOwner->UpdateLights(true);
}

int PlaySequencer::GetStep(double time)
{
   int measure = TheTransport->GetMeasure(time) % mNumMeasures;
   int stepsPerMeasure = TheTransport->CountInStandardMeasure(mInterval) * TheTransport->GetTimeSigTop() / TheTransport->GetTimeSigBottom();
   int step = TheTransport->GetQuantized(time, mInterval) + stepsPerMeasure * measure;
   return step;
}

void PlaySequencer::UpdateInterval()
{
   TheTransport->UpdateListener(this, mInterval, OffsetInfo(0, false));
   TheTransport->UpdateListener(&mNoteOffScheduler, mInterval, OffsetInfo(TheTransport->GetMeasureFraction(mInterval) * .5f, false));
   UpdateNumMeasures(mNumMeasures);
}

void PlaySequencer::UpdateNumMeasures(int oldNumMeasures)
{
   int oldSteps = mGrid->GetCols();

   int stepsPerMeasure = TheTransport->CountInStandardMeasure(mInterval) * TheTransport->GetTimeSigTop() / TheTransport->GetTimeSigBottom();
   mGrid->SetGrid(stepsPerMeasure * mNumMeasures, mLanes.size());

   if (mNumMeasures > oldNumMeasures)
   {
      for (int i = 1; i < mNumMeasures / oldNumMeasures; ++i)
      {
         for (int j = 0; j < oldSteps; ++j)
         {
            for (int row = 0; row < mGrid->GetRows(); ++row)
            {
               mGrid->SetVal(j + i * oldSteps, row, mGrid->GetVal(j, row));
            }
         }
      }
   }
}

void PlaySequencer::UpdateLights(bool betweener)
{
   if (mGridController == nullptr)
      return;

   for (int i = 0; i < 4; ++i)
      mGridController->SetLight(i, 0, mWrite ? kGridColor2Bright : kGridColorOff);

   for (int i = 0; i < 3; ++i)
      mGridController->SetLight(i, 1, mNoteRepeat && !betweener ? kGridColor2Bright : kGridColorOff);

   mGridController->SetLight(3, 1, mLinkColumns ? kGridColor2Bright : kGridColorOff);

   mGridController->SetLight(0, 2, mNumMeasures == 1 ? kGridColor3Bright : kGridColorOff);
   mGridController->SetLight(1, 2, mNumMeasures == 2 ? kGridColor3Bright : kGridColorOff);
   mGridController->SetLight(2, 2, mNumMeasures == 4 ? kGridColor3Bright : kGridColorOff);
   mGridController->SetLight(3, 2, mNumMeasures == 8 ? kGridColor3Bright : kGridColorOff);

   mGridController->SetLight(0, 3, GetVelocityLevel() == 1 || GetVelocityLevel() == 3 ? kGridColor2Bright : kGridColorOff);
   mGridController->SetLight(1, 3, GetVelocityLevel() == 2 || GetVelocityLevel() == 3 ? kGridColor2Bright : kGridColorOff);

   mGridController->SetLight(3, 3, mClearLane ? kGridColor1Bright : kGridColorOff);

   int step = GetStep(gTime);
   for (int i = 0; i < 16; ++i)
   {
      int x = (i / 4) % 4 + 4;
      int y = i % 4;
      mGridController->SetLight(x, y, step % 16 == i ? kGridColor3Bright : kGridColorOff);
   }

   for (size_t i = 0; i < mLanes.size(); ++i)
   {
      int x = i % 4 * 2;
      int y = 7 - i / 4;
      mGridController->SetLight(x, y, mLanes[i].mIsPlaying ? kGridColor2Bright : kGridColorOff);
      mGridController->SetLight(x + 1, y, mLanes[i].mMuteOrErase ? kGridColorOff : kGridColor1Bright);
   }
}

int PlaySequencer::GetVelocityLevel()
{
   if (mUseLightVelocity && !mUseMedVelocity)
      return 1;

   if (mUseMedVelocity && !mUseLightVelocity)
      return 2;

   return 3;
}

void PlaySequencer::OnControllerPageSelected()
{
   UpdateLights();
}

void PlaySequencer::OnGridButton(int x, int y, float velocity, IGridController* grid)
{
   if (grid == mGridController)
   {
      bool press = velocity > 0;
      if (x >= 0 && y >= 0)
      {
         if (press && y == 0 && x < 4)
            mWrite = !mWrite;

         if (press && y == 1 && x < 3)
            mNoteRepeat = !mNoteRepeat;

         if (press && y == 1 && x == 3)
            mLinkColumns = !mLinkColumns;

         if (press && y == 2)
         {
            int newNumMeasures = mNumMeasures;
            if (x == 0)
               newNumMeasures = 1;
            if (x == 1)
               newNumMeasures = 2;
            if (x == 2)
               newNumMeasures = 4;
            if (x == 3)
               newNumMeasures = 8;

            if (newNumMeasures != mNumMeasures)
            {
               int oldNumMeasures = mNumMeasures;
               mNumMeasures = newNumMeasures;
               UpdateNumMeasures(oldNumMeasures);
            }
         }

         if (x == 0 && y == 3)
            mUseLightVelocity = press;
         if (x == 1 && y == 3)
            mUseMedVelocity = press;

         if (x == 3 && y == 3)
            mClearLane = press;

         if (x == 2 && y == 3 && mClearLane)
            mGrid->Clear();

         if (y >= 4)
         {
            int pitch = x / 2 + (7 - y) * 4;
            if (x % 2 == 0)
            {
               if (velocity > 0)
               {
                  mLanes[pitch].mInputVelocity = velocity * 127;
               }
               else
               {
                  if (mNoteRepeat)
                     mLanes[pitch].mInputVelocity = 0;
               }
            }
            else if (x % 2 == 1)
            {
               mLanes[pitch].mMuteOrErase = press;
               if (mLinkColumns)
               {
                  for (size_t i=0; i<mLanes.size(); ++i)
                  {
                     if (i % 4 == pitch % 4)
                        mLanes[i].mMuteOrErase = press;
                  }
               }

               if (press && mClearLane)
               {
                  for (int i = 0; i < mGrid->GetCols(); ++i)
                  {
                     mGrid->SetVal(i, pitch, 0);

                     if (mLinkColumns)
                     {
                        for (size_t j = 0; j < mLanes.size(); ++j)
                        {
                           if (j % 4 == pitch % 4)
                              mGrid->SetVal(i, j, 0);
                        }
                     }
                  }
               }
            }
         }

         UpdateLights();
      }
   }
}

void PlaySequencer::ButtonClicked(ClickButton* button)
{
}

void PlaySequencer::DropdownUpdated(DropdownList* list, int oldVal)
{
   if (list == mIntervalSelector)
      UpdateInterval();
   if (list == mNumMeasuresSelector)
      UpdateNumMeasures(oldVal);
}

void PlaySequencer::IntSliderUpdated(IntSlider* slider, int oldVal)
{
}

void PlaySequencer::LoadLayout(const ofxJSONElement& moduleInfo)
{
   mModuleSaveData.LoadString("target", moduleInfo);
   mModuleSaveData.LoadBool("sustain", moduleInfo, false);
   mModuleSaveData.LoadFloat("velocity_full", moduleInfo, 1, 0, 1, K(isTextField));
   mModuleSaveData.LoadFloat("velocity_med", moduleInfo, .5f, 0, 1, K(isTextField));
   mModuleSaveData.LoadFloat("velocity_light", moduleInfo, .25f, 0, 1, K(isTextField));

   SetUpFromSaveData();
}

void PlaySequencer::SetUpFromSaveData()
{
   SetUpPatchCables(mModuleSaveData.GetString("target"));
   mSustain = mModuleSaveData.GetBool("sustain");
   mVelocityFull = mModuleSaveData.GetFloat("velocity_full");
   mVelocityMed = mModuleSaveData.GetFloat("velocity_med");
   mVelocityLight = mModuleSaveData.GetFloat("velocity_light");
}
