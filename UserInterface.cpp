#include "UserInterface.h"

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderWindow.hpp>


UserInterface::UserInterface()
    : sfgui()
    , desktop()
    , window(sfg::Window::Create())
    , button(sfg::Button::Create("Hello"))
{
    setupWidgetHandlers();
    setupWindowConfiguration();
}


void UserInterface::draw(sf::RenderWindow &renderWindow)
{
    desktop.Update(1.f); // TODO: use a timer
    renderWindow.pushGLStates();
    sfgui.Display(renderWindow);
    renderWindow.popGLStates();
}

void UserInterface::handleEvent(sf::Event &event)
{
    desktop.HandleEvent(event);
}

// ----------------------------------------------------------------------------

void UserInterface::setupWidgetHandlers()
{
    button->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onButtonClick, this);
    // TODO: hook up other widget handlers as needed
}

void UserInterface::setupWindowConfiguration()
{
    window->SetTitle("Hello SF-GUI Example");
    window->Add(button);
    desktop.Add(window);
}

void UserInterface::onButtonClick()
{
    if      (button->GetLabel() == "Hello") button->SetLabel("World");
    else if (button->GetLabel() == "World") button->SetLabel("Hello");
}
