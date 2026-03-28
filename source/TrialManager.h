#pragma once

#ifdef FREECODER_TRIAL

#include <juce_core/juce_core.h>

/** Local-file-based 7-day trial manager.
 *
 *  On first launch, writes the current Unix epoch to
 *  <userAppData>/Ament Audio/FREECODER/.trial_start.
 *  Subsequent launches read that stamp and compute days elapsed.
 *
 *  The plugin remains fully functional after the trial ends — this class
 *  only supplies the information needed to display a purchase reminder.
 */
class TrialManager
{
public:
    static TrialManager& getInstance()
    {
        static TrialManager instance;
        return instance;
    }

    /** Days elapsed since first launch (clamped at 0 if clock drift detected). */
    int  getDaysElapsed()  const noexcept { return daysElapsed; }

    /** Returns true once 7 or more days have passed since first launch. */
    bool isTrialExpired()  const noexcept { return daysElapsed >= 7; }

private:
    TrialManager()
    {
        const auto trialFile = getTrialFile();
        juce::int64 installEpoch = 0;

        if (trialFile.existsAsFile())
        {
            installEpoch = trialFile.loadFileAsString().trim().getLargeIntValue();
        }
        else
        {
            installEpoch = juce::Time::getCurrentTime().toMilliseconds() / 1000LL;
            trialFile.getParentDirectory().createDirectory();
            trialFile.replaceWithText (juce::String (installEpoch));
        }

        const auto nowEpoch       = juce::Time::getCurrentTime().toMilliseconds() / 1000LL;
        const auto secondsElapsed = nowEpoch - installEpoch;
        daysElapsed = (int) juce::jmax (0LL, secondsElapsed / (60LL * 60LL * 24LL));
    }

    static juce::File getTrialFile()
    {
        // macOS  → ~/Library/Application Support/Ament Audio/FREECODER/.trial_start
        // Windows → %APPDATA%/Ament Audio/FREECODER/.trial_start
        // Linux   → ~/Ament Audio/FREECODER/.trial_start
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                          .getChildFile ("Ament Audio")
                          .getChildFile ("FREECODER")
                          .getChildFile (".trial_start");
    }

    int daysElapsed = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrialManager)
};

#endif // FREECODER_TRIAL
