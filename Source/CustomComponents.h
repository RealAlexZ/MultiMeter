
#include <JuceHeader.h>
using namespace juce;


class CustomLook2 : public LookAndFeel_V4
{
public:
    CustomLook2()
    {
        //setColour(ToggleButton::textColourId, Colours::darkgrey);
        //setColour(ToggleButton::tickColourId, Colours::lightgrey);
        //setColour(ToggleButton::tickDisabledColourId, Colours::lightgrey);
    }

    ~CustomLook2() override {}

    void drawButtonBackground(Graphics& g, Button& button,
        const Colour& backgroundColour,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown)
    {
        auto buttonArea = button.getLocalBounds().toFloat();
        float cornerSize = 3.0f; 

        auto buttonColor = button.findColour(TextButton::buttonColourId);

        g.setColour(button.getToggleState()?buttonColor.darker(0.5) : buttonColor);
        g.fillRoundedRectangle(buttonArea, cornerSize);
    }

    Font getComboBoxFont(ComboBox& combobox) override
    {
        auto font = getPopupMenuFont();
        font.setHeight(14);
        return font;
    }

    void drawToggleButton(Graphics& g, ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        Colour buttonColor = button.findColour(TextButton::buttonColourId);
        Colour tickColor = button.findColour(ToggleButton::tickColourId);

        if (button.getToggleState())
            g.setColour(buttonColor);
        else
            g.setColour(buttonColor.darker(0.5f));

        drawButtonBackground(g, button, buttonColor, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

        g.setColour(button.findColour(ToggleButton::textColourId));
        auto buttonArea = button.getLocalBounds();
        g.drawFittedText(button.getButtonText(), buttonArea, Justification::centred, 1);
    }


    Font getTextButtonFont(TextButton&, int buttonHeight)
    {
        return Font("Arial", buttonHeight * 0.8f, Font::plain);
    }


private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomLook2)
};

class CustomLook : public LookAndFeel_V4
{
private:
    Colour baseColour{ 0xffd2d2d2 };
public:
    CustomLook()
    {
        setColour(ComboBox::backgroundColourId, Colour(240, 240, 240));
        setColour(ComboBox::buttonColourId, Colour(210, 210, 210));
        setColour(ComboBox::outlineColourId, Colours::black);
        setColour(ComboBox::textColourId, Colours::grey);
        setColour(PopupMenu::backgroundColourId, Colour(240, 240, 240));
        setColour(PopupMenu::textColourId, Colours::darkgrey);
        setColour(PopupMenu::highlightedBackgroundColourId, Colour(160, 160, 160));
        setColour(Label::textColourId, Colour(45, 45, 45));

        setColour(ToggleButton::textColourId, Colours::darkgrey);
        setColour(ToggleButton::tickColourId, Colours::lightgrey);
        setColour(ToggleButton::tickDisabledColourId, Colours::lightgrey);
    }

    ~CustomLook() override {}

    void drawToggleButton(Graphics& g, ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto buttonArea = button.getLocalBounds();

        if (button.getToggleState())
        {
            g.setColour(baseColour);
        }
        else
        {
            g.setColour(baseColour.darker(0.2f));
        }

        g.fillRect(buttonArea.toFloat());
        g.setColour(button.findColour(ToggleButton::textColourId));     
        g.drawFittedText(button.getButtonText(), buttonArea, Justification::centred, 1);
    }

    void drawPopupMenuBackground(Graphics& g, int width, int height) override
    {
        LookAndFeel_V4::drawPopupMenuBackground(g, width, height);

        g.setColour(Colour(5, 5, 5));
        g.drawRect(0, 0, width, height);
    }

    PopupMenu::Options getOptionsForComboBoxPopupMenu(ComboBox& box, Label& label) override
    {
        PopupMenu::Options option = LookAndFeel_V4::getOptionsForComboBoxPopupMenu(box, label);
        return option.withStandardItemHeight((int)(label.getHeight()));
    };

    void positionComboBoxText(juce::ComboBox& comboBox, juce::Label& label) override
    {
        label.setFont(getComboBoxFont(comboBox));
        label.setJustificationType(juce::Justification::horizontallyCentred);
        label.setBounds(1, 1, comboBox.getWidth() - 2, comboBox.getHeight() - 2);
    }

    void drawPopupMenuItem(Graphics& g, const Rectangle<int>& area,
        const bool isSeparator, const bool isActive,
        const bool isHighlighted, const bool isTicked,
        const bool hasSubMenu, const String& text,
        const String& shortcutKeyText,
        const Drawable* icon, const Colour* textColour) override
    {
        if (isHighlighted)
        {
            g.setColour(findColour(PopupMenu::highlightedBackgroundColourId));
            g.fillRect(area);
        }

        g.setColour(findColour(PopupMenu::textColourId));
        g.setFont(getPopupMenuFont());
        g.drawText(text, area.reduced(4), Justification::centred);

        if (isSeparator)
        {
            g.setColour(findColour(PopupMenu::textColourId));
            g.drawLine(area.getX() + 4, area.getCentreY(), area.getRight() - 4, area.getCentreY());
        }

    }

    Font getTextButtonFont(TextButton&, int buttonHeight)
    {
        return Font("Arial", buttonHeight * 0.8f, Font::plain);
    }


private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomLook)
};


class Switch : public ToggleButton
{
private:
    CustomLook2 lookandfeel;
    String onText, offText;
public:
    Switch(const String& on_text, const String& off_text) : onText(on_text), offText(off_text)
    {
        setLookAndFeel(&lookandfeel);
    }

    ~Switch() override
    {
        setLookAndFeel(nullptr);
    }

    void clicked() override
    {
        ToggleButton::clicked();
        if (getToggleState())
            setButtonText(onText);
        else
            setButtonText(offText);
    }

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Switch)
};



class ToggleChain : public Component
{
private:
    CustomLook2 lookandfeel;
    int selectedId{ 0 };
public:
    ToggleChain()
    {
        setLookAndFeel(&lookandfeel);
    }

    ~ToggleChain() override
    {
        setLookAndFeel(nullptr);
    }

    void addOption(const String& name, Button::Listener& listener)
    {
        auto toggleButton = std::make_unique<ToggleButton>(name);
        toggleButton->addListener(&listener);
        toggleButton->setClickingTogglesState(false);
        addAndMakeVisible(toggleButton.get());
        toggleButtons.add(std::move(toggleButton));
        updateLayout();
    }

    void resized() override
    {
        updateLayout();
    }

    void setSelection(int id)
    {
        if (id < toggleButtons.size())
        {
            toggleButtons[id]->setToggleState(true, dontSendNotification);
            selectedId = id;
        }

    }

    int getSelectedId()
    {
        return selectedId;
    }

    void buttonClicked(Button* button)
    {
        ToggleButton* toggleButton = dynamic_cast<ToggleButton*>(button); // Cast to ToggleButton

        if (toggleButton != nullptr)
        {
            // Check if the clicked button is a member of toggleButtons
            if (std::find(toggleButtons.begin(), toggleButtons.end(), toggleButton) != toggleButtons.end())
            {
                if (!toggleButton->getToggleState())
                {
                    for (int i = 0; i < toggleButtons.size(); ++i)
                    {
                        if (toggleButtons[i] == toggleButton)
                        {
                            selectedId = i;
                            toggleButton->setToggleState(true, dontSendNotification);
                        }
                        else
                        {
                            toggleButtons[i]->setToggleState(false, dontSendNotification);
                        }
                    }
                }
            }
        }
    }

private:
    void updateLayout()
    {
        auto bounds = getLocalBounds();
        int xPos = 0;
        for (auto& tb : toggleButtons)
        {
            tb->setBounds(bounds.removeFromLeft(50).reduced(1,1).translated(xPos,0));
            xPos += 5;
        }
    }

    OwnedArray<ToggleButton> toggleButtons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToggleChain)
};


class SwitchButton : public Component
{
private:
    int switchId{1};
public:
    SwitchButton()
    {
        setLookAndFeel(&lookandfeel);

        Gonio.setButtonText("GONIOMETER");
        Spectra.setButtonText("ANALYZER");
        Histogram.setButtonText("HISTOGRAM");

        Gonio.setClickingTogglesState(false);
        Spectra.setClickingTogglesState(false);
        Histogram.setClickingTogglesState(false);

        addAndMakeVisible(Gonio);
        addAndMakeVisible(Spectra);
        addAndMakeVisible(Histogram);

        Spectra.setToggleState(true, dontSendNotification);

        updateButtonLayout();
    }

    ~SwitchButton() override {
        setLookAndFeel(nullptr);
    }

    void addListener(Button::Listener& listener)
    {
        Gonio.addListener(&listener);
        Spectra.addListener(&listener);
        Histogram.addListener(&listener);
    }

    void buttonClicked(Button* button)
    {
        if (button == &Gonio)
        {
            if (!Gonio.getToggleState())
            {
                Gonio.setToggleState(true, dontSendNotification);
                Spectra.setToggleState(false, dontSendNotification);
                Histogram.setToggleState(false, dontSendNotification);
            }
            switchId = 0;
        }
        else if (button == &Spectra)
        {
            if (!Spectra.getToggleState())
            {
                Spectra.setToggleState(true, dontSendNotification);
                Gonio.setToggleState(false, dontSendNotification);
                Histogram.setToggleState(false, dontSendNotification);
            }
            switchId = 1;
        }
        else if (button == &Histogram)
        {
            if (!Histogram.getToggleState())
            {
                Histogram.setToggleState(true, dontSendNotification);
                Gonio.setToggleState(false, dontSendNotification);
                Spectra.setToggleState(false, dontSendNotification);
            }
            switchId = 2;
        }
    }


    int getSwitchID() const
    {
        return switchId;
    }

    void resized() override
    {
        updateButtonLayout();
    }

private:
    ToggleButton Gonio, Spectra, Histogram;

    CustomLook lookandfeel;

    void updateButtonLayout()
    {
        auto buttonWidth = getWidth() / 3;
        auto buttonHeight = getHeight();

        Gonio.setBounds(0, 0, buttonWidth, buttonHeight);
        Spectra.setBounds(buttonWidth, 0, buttonWidth, buttonHeight);
        Histogram.setBounds(2 * buttonWidth, 0, buttonWidth, buttonHeight);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SwitchButton)
};
