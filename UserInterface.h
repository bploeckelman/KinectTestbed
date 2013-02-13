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
    sfg::Button::Ptr quitButton;
    sfg::Button::Ptr openButton;
    sfg::ToggleButton::Ptr saveButton;
    sfg::CheckButton::Ptr showColorButton;
    sfg::CheckButton::Ptr showDepthButton;
    sfg::Label::Ptr infoLabel;

public:
    UserInterface();

    void draw(sf::RenderWindow &renderWindow);
    void handleEvent(sf::Event &event);

    void setInfo(const std::string &info) { infoLabel->SetText(sf::String(info)); }

private:
    void setupWidgetHandlers();
    void setupWindowConfiguration();

    // Widget handlers...
    void onQuitButtonClick();
    void onOpenButtonClick();
    void onSaveButtonClick();
    void onShowColorButtonClick();
    void onShowDepthButtonClick();

};

