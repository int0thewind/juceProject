/*
  ==============================================================================

    This file was auto-generated and contains the startup code for a PIP.

  ==============================================================================
*/

#include <JuceHeader.h>

#include <memory>
#include "MPEIntroductionTutorial.h"

class Application    : public JUCEApplication
{
public:
    //==============================================================================
    Application() = default;

    const String getApplicationName() override       { return "MPEIntroductionTutorial"; }
    const String getApplicationVersion() override    { return "1.0.0"; }

    void initialise (const String&) override {
        this->mainWindow = std::make_unique<MainWindow> ("MPEIntroductionTutorial", new MainComponent(), *this);
    }
    void shutdown() override {
        this->mainWindow = nullptr;
    }

private:
    class MainWindow    : public DocumentWindow
    {
    public:
        MainWindow (const String& name, Component* c, JUCEApplication& a)
            : DocumentWindow (name, Desktop::getInstance().getDefaultLookAndFeel()
                .findColour (ResizableWindow::backgroundColourId),
                DocumentWindow::allButtons),
                app (a)
        {
            this->setUsingNativeTitleBar (true);
            this->setContentOwned (c, true);

           #if JUCE_ANDROID || JUCE_IOS
            setFullScreen (true);
           #else
            this->setResizable (true, false);
            this->setResizeLimits (300, 250, 10000, 10000);
            this->centreWithSize (getWidth(), getHeight());
           #endif

            this->setVisible (true);
        }

        void closeButtonPressed() override
        {
            this->app.systemRequestedQuit();
        }

    private:
        JUCEApplication& app;

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
START_JUCE_APPLICATION (Application)
