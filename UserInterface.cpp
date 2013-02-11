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
    , box(sfg::Box::Create(sfg::Box::HORIZONTAL, 0.f))
    , quitButton(sfg::Button::Create("Quit"))
    , openButton(sfg::Button::Create("Open"))
    , saveButton(sfg::Button::Create("Save"))
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
    saveButton->GetSignal(sfg::Button::OnLeftClick)
           .Connect(&UserInterface::onSaveButtonClick, this);
    // TODO: hook up other widget handlers as needed
}

void UserInterface::setupWindowConfiguration()
{
    infoLabel->SetText("Sensor [?] : ?");
    infoLabel->SetLineWrap(true);

    sfg::Fixed::Ptr fixed = sfg::Fixed::Create();
    fixed->Put(infoLabel, sf::Vector2f(0,0));
    fixed->Put(quitButton, sf::Vector2f(0, 25));
    fixed->Put(openButton, sf::Vector2f(50, 25));
    fixed->Put(saveButton, sf::Vector2f(100, 25));
    box->Pack(fixed);

    window->SetTitle("Kinect Testbed");
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
    std::wstring wfilename(Application::request().showFileChooser());
    std::string filename;
    filename.assign(wfilename.begin(), wfilename.end());
    std::cout << "Selected file: " << filename << std::endl;

    // TODO: open file
}

void UserInterface::onSaveButtonClick()
{
    Application::request().toggleSave();
}
