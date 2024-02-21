
#pragma once

#include <JuceHeader.h>
using namespace juce;

//==============================================================================
// This class defines a custom LookAndFeel for buttons, including ToggleButtons and TextButtons.
class ButtonsLook : public LookAndFeel_V4
{
public:
    // Constructor
    ButtonsLook() {}

    // Destructor
    ~ButtonsLook() override {}

    // Override the drawButtonBackground method to customize the appearance of buttons.
    void drawButtonBackground(Graphics& g, Button& button,
        const Colour& backgroundColour,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override
    {
        // Get the button area and corner size
        auto buttonArea = button.getLocalBounds().toFloat();
        float cornerSize = 3.0f;

        // Get the button color
        auto buttonColor = button.findColour(TextButton::buttonColourId);

        // Set the color based on the toggle state and fill the rounded rectangle
        g.setColour(button.getToggleState() ? buttonColor.darker(0.5) : buttonColor);
        g.fillRoundedRectangle(buttonArea, cornerSize);
    }

    // Override the getComboBoxFont method to customize the font of ComboBoxes.
    Font getComboBoxFont(ComboBox& combobox) override
    {
        auto font = getPopupMenuFont();
        font.setHeight(14);
        return font;
    }

    // Override the drawToggleButton method to customize the appearance of ToggleButtons.
    void drawToggleButton(Graphics& g, ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        // Get the button color
        Colour buttonColor = button.findColour(TextButton::buttonColourId);

        // Set the color based on the toggle state
        if (button.getToggleState())
            g.setColour(buttonColor);
        else
            g.setColour(buttonColor.darker(0.5f));

        // Draw the button background
        drawButtonBackground(g, button, buttonColor, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

        // Set the text color and draw the button text centered within the button area
        g.setColour(button.findColour(ToggleButton::textColourId));
        auto buttonArea = button.getLocalBounds();
        g.drawFittedText(button.getButtonText(), buttonArea, Justification::centred, 1);
    }

    // Override the getTextButtonFont method to customize the font of TextButtons.
    Font getTextButtonFont(TextButton&, int buttonHeight) override
    {
        return Font("Arial", buttonHeight * 0.8f, Font::plain);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonsLook)
};

//==============================================================================
// This class defines a custom LookAndFeel for ToggleButtons, which changes the text color and background color
class CustomLook : public LookAndFeel_V4
{
public:
    CustomLook()
    {
        // Set the text color for ToggleButtons to dark grey
        setColour(ToggleButton::textColourId, Colours::darkgrey);
    }

    ~CustomLook() override {}

    // Override the drawToggleButton method to customize the appearance of ToggleButtons
    void drawToggleButton(Graphics& g, ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto buttonArea = button.getLocalBounds();

        // Set the background color based on the toggle state
        if (button.getToggleState())
        {
            g.setColour(BACKGROUND_COLOR);
        }
        else
        {
            g.setColour(BACKGROUND_COLOR.darker(0.2f));
        }

        // Fill the button area with the background color
        g.fillRect(buttonArea.toFloat());
        // Set the text color based on the LookAndFeel settings
        g.setColour(button.findColour(ToggleButton::textColourId));
        // Draw the button text centered within the button area.
        g.drawFittedText(button.getButtonText(), buttonArea, Justification::centred, 1);
    }

    // Override the getTextButtonFont method to customize the font of TextButtons
    Font getTextButtonFont(TextButton&, int buttonHeight) override
    {
        return Font("Arial", buttonHeight * 0.8f, Font::plain);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomLook)
};

//==============================================================================
// This class defines a custom ToggleButton subclass called Switch, which changes its text according to its toggle state
class Switch : public ToggleButton
{
private:
    ButtonsLook lookAndFeel;
    String onText, offText;
public:
    // Constructor that takes the on and off text labels
    Switch(const String& on_text, const String& off_text) : onText(on_text), offText(off_text)
    {
        setLookAndFeel(&lookAndFeel);
    }

    ~Switch() override
    {
        setLookAndFeel(nullptr);
    }

    // Override the clicked method to update the button text when clicked
    void clicked() override
    {
        ToggleButton::clicked();
        // Update the button text based on the toggle state
        if (getToggleState())
            setButtonText(onText);
        else
            setButtonText(offText);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Switch)
};

//==============================================================================
// This class represents a chain of toggle buttons used for controls where multiple options need to be toggled
// It allows toggling between 2 or more options.
class ToggleChain : public Component
{
private:
    ButtonsLook lookAndFeel; // Look and feel for buttons
    int selectedId{ 0 }; // Currently selected option ID

public:
    ToggleChain()
    {
        setLookAndFeel(&lookAndFeel); // Set the look and feel
    }

    ~ToggleChain() override
    {
        setLookAndFeel(nullptr); // Reset the look and feel
    }

    // Adds an option with a specified name and listener to the toggle chain
    void addOption(const String& name, Button::Listener& listener)
    {
        auto toggleButton = std::make_unique<ToggleButton>(name); // Create a new toggle button
        toggleButton->addListener(&listener); // Add listener to the toggle button
        toggleButton->setClickingTogglesState(false); // Set clicking to not toggle state
        addAndMakeVisible(toggleButton.get()); // Add button to the UI
        toggleButtons.add(std::move(toggleButton)); // Add button to the array
        updateLayout(); // Update layout
    }

    void resized() override
    {
        updateLayout(); // Update layout on resize
    }

    // Sets the selection to the specified ID
    void setSelection(int id)
    {
        if (id < toggleButtons.size())
        {
            toggleButtons[id]->setToggleState(true, dontSendNotification); // Set toggle state of the specified button
            selectedId = id; // Update selected ID
        }
    }

    // Returns the ID of the currently selected option
    int getSelectedId()
    {
        return selectedId; // Return selected ID
    }

    // Handles button click events
    void buttonClicked(Button* button)
    {
        ToggleButton* toggleButton = dynamic_cast<ToggleButton*>(button); // Cast to ToggleButton

        if (toggleButton != nullptr)
        {
            if (std::find(toggleButtons.begin(), toggleButtons.end(), toggleButton) != toggleButtons.end())
            {
                if (!toggleButton->getToggleState())
                {
                    for (int i = 0; i < toggleButtons.size(); ++i)
                    {
                        if (toggleButtons[i] == toggleButton)
                        {
                            selectedId = i; // Update selected ID
                            toggleButton->setToggleState(true, dontSendNotification); // Toggle the clicked button
                        }
                        else
                        {
                            toggleButtons[i]->setToggleState(false, dontSendNotification); // Untoggle other buttons
                        }
                    }
                }
            }
        }
    }

private:
    // Updates the layout of the toggle buttons
    void updateLayout()
    {
        auto bounds = getLocalBounds();
        int xPos = 0;
        for (auto& tb : toggleButtons)
        {
            tb->setBounds(bounds.removeFromLeft(50).reduced(1, 1).translated(xPos, 0)); // Set button bounds
            xPos += 5; // Adjust X position
        }
    }

    OwnedArray<ToggleButton> toggleButtons; // Array of toggle buttons

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToggleChain)
};

//==============================================================================
// This class represents a menu for switching between three different views: Goniometer, Spectrum Analyzer, and Histogram
class SwitchButton : public Component {
private:
    // ID representing the currently selected switch option
    int switchId{ 1 };

public:
    // Constructor initializes the menu buttons and sets up their appearance and behavior
    SwitchButton() {
        setLookAndFeel(&lookAndFeel);

        // Setting up button labels
        Goniometer.setButtonText("GONIOMETER");
        Spectrum.setButtonText("ANALYZER");
        Histogram.setButtonText("HISTOGRAM");

        // Disabling toggling on click for the buttons
        Goniometer.setClickingTogglesState(false);
        Spectrum.setClickingTogglesState(false);
        Histogram.setClickingTogglesState(false);

        // Adding buttons to the component and making them visible
        addAndMakeVisible(Goniometer);
        addAndMakeVisible(Spectrum);
        addAndMakeVisible(Histogram);

        // Setting Spectrum as the default active button
        Spectrum.setToggleState(true, dontSendNotification);

        // Updating the layout of the buttons
        updateButtonLayout();
    }

    // Destructor resets the look and feel
    ~SwitchButton() override {
        setLookAndFeel(nullptr);
    }

    // Adds a listener to the menu buttons to handle their clicks
    void addListener(Button::Listener& listener) {
        Goniometer.addListener(&listener);
        Spectrum.addListener(&listener);
        Histogram.addListener(&listener);
    }

    // Handles button clicks by disabling other options and updating the active switch ID
    void buttonClicked(Button* button) {
        if (button == &Goniometer) {
            if (!Goniometer.getToggleState()) {
                Goniometer.setToggleState(true, dontSendNotification);
                Spectrum.setToggleState(false, dontSendNotification);
                Histogram.setToggleState(false, dontSendNotification);
            }
            switchId = 0;
        }
        else if (button == &Spectrum) {
            if (!Spectrum.getToggleState()) {
                Spectrum.setToggleState(true, dontSendNotification);
                Goniometer.setToggleState(false, dontSendNotification);
                Histogram.setToggleState(false, dontSendNotification);
            }
            switchId = 1;
        }
        else if (button == &Histogram) {
            if (!Histogram.getToggleState()) {
                Histogram.setToggleState(true, dontSendNotification);
                Goniometer.setToggleState(false, dontSendNotification);
                Spectrum.setToggleState(false, dontSendNotification);
            }
            switchId = 2;
        }
    }

    // Retrieves the currently selected option ID
    int getSwitchID() const {
        return switchId;
    }

    // Resizes and updates the layout of the menu buttons
    void resized() override {
        updateButtonLayout();
    }

private:
    // Toggle buttons representing the menu options
    ToggleButton Goniometer, Spectrum, Histogram;

    // Custom look and feel for the buttons
    CustomLook lookAndFeel;

    // Updates the layout of the menu buttons based on the component size
    void updateButtonLayout() {
        auto buttonWidth = getWidth() / 3;
        auto buttonHeight = getHeight();

        Goniometer.setBounds(0, 0, buttonWidth, buttonHeight);
        Spectrum.setBounds(buttonWidth, 0, buttonWidth, buttonHeight);
        Histogram.setBounds(2 * buttonWidth, 0, buttonWidth, buttonHeight);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SwitchButton)
};
