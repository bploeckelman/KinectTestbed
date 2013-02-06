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
    sfg::Button::Ptr button;

public:
    UserInterface();

    void draw(sf::RenderWindow &renderWindow);
    void handleEvent(sf::Event &event);

private:
    void setupWidgetHandlers();
    void setupWindowConfiguration();

    // Widget handlers...
    void onButtonClick();

};

