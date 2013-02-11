#include "UserInterface.h"

#include <SFGUI/SFGUI.hpp>

#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include "Application.h"


UserInterface::UserInterface()
    : sfgui()
    , desktop()
    , window(sfg::Window::Create())
    , box(sfg::Box::Create(sfg::Box::HORIZONTAL, 20.f))
    , openButton(sfg::Button::Create("Open"))
    , quitButton(sfg::Button::Create("Quit"))
    , infoLabel(sfg::Label::Create())
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
    quitButton->GetSignal(sfg::Button::OnLeftClick)
	       .Connect(&UserInterface::onQuitButtonClick, this);
    openButton->GetSignal(sfg::Button::OnLeftClick)
	       .Connect(&UserInterface::onOpenButtonClick, this);
    // TODO: hook up other widget handlers as needed
}

void UserInterface::setupWindowConfiguration()
{
    infoLabel->SetText("Sensors = ??");

    sfg::Fixed::Ptr fixed = sfg::Fixed::Create();
    fixed->Put(quitButton, sf::Vector2f(0,0));
    fixed->Put(openButton, sf::Vector2f(0, 30));
    fixed->Put(infoLabel, sf::Vector2f(40,5));
    box->Pack(fixed);

    window->SetTitle("Hello SF-GUI Example");
    window->SetStyle(sfg::Window::Style::NO_STYLE);
    window->Add(box);
    desktop.Add(window);
}

void UserInterface::onQuitButtonClick()
{
    Application::request().shutdown();    
}

void UserInterface::onOpenButtonClick()
{
    //Application::request().showFileOpenDialog();
    std::wstring filename(Application::request().showFileChooser());
    std::wcout << "Selected file: " << filename << std::endl;
    // TODO: open file
}
