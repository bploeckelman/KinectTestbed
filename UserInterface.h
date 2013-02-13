#pragma once
#include <SFGUI/SFGUI.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderWindow.hpp>


class UserInterface
{
private:
    sfg::SFGUI sfgui;
    sfg::Desktop desktop;
    sfg::Window::Ptr window;

    // Widgets...
    sfg::Box::Ptr box;

    sfg::Label::Ptr infoLabel;

    sfg::Button::Ptr quitButton;
    sfg::Button::Ptr openButton;

    sfg::ToggleButton::Ptr saveButton;

    sfg::CheckButton::Ptr showColorButton;
    sfg::CheckButton::Ptr showDepthButton;
    sfg::CheckButton::Ptr showJointsButton;

    sfg::ProgressBar::Ptr jointFramesProgress;
    sfg::Label::Ptr jointFramesFilename;
    sfg::Label::Ptr jointFrameIndex;

public:
    UserInterface();

    void draw(sf::RenderWindow &renderWindow);
    void handleEvent(sf::Event &event);

    void setInfo    (const std::string &info) { infoLabel->SetText(sf::String(info)); }
    void setFileName(const std::string &name) { jointFramesFilename->SetText(sf::String(name)); }
    void setProgress(float fraction) { jointFramesProgress->SetFraction(fraction); }
    void setIndex(int index) {
        std::stringstream ss;
        ss << "[" << index << "]";
        jointFrameIndex->SetText(sf::String(ss.str()));
    }

private:
    void setupWidgetHandlers();
    void setupWindowConfiguration();

    // Widget handlers...
    void onQuitButtonClick();
    void onOpenButtonClick();
    void onSaveButtonClick();
    void onShowColorButtonClick();
    void onShowDepthButtonClick();
    void onShowJointsButtonClick();
};

