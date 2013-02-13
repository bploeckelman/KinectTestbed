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
    , saveButton(sfg::ToggleButton::Create("Save"))
    , infoLabel(sfg::Label::Create())
    , showColorButton(sfg::CheckButton::Create("Color"))
    , showDepthButton(sfg::CheckButton::Create("Depth"))
    , showJointsButton(sfg::CheckButton::Create("Joints"))
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
    showColorButton->GetSignal(sfg::Button::OnLeftClick)
           .Connect(&UserInterface::onShowColorButtonClick, this);
    showDepthButton->GetSignal(sfg::Button::OnLeftClick)
           .Connect(&UserInterface::onShowDepthButtonClick, this);
    showJointsButton->GetSignal(sfg::Button::OnLeftClick)
           .Connect(&UserInterface::onShowJointsButtonClick, this);
    // TODO: hook up other widget handlers as needed
}

void UserInterface::setupWindowConfiguration()
{
    showColorButton->SetActive(true);
    showDepthButton->SetActive(true);
    showJointsButton->SetActive(true);
    infoLabel->SetText("Sensor [?] : ?");
    infoLabel->SetLineWrap(true);

    sfg::Fixed::Ptr fixed = sfg::Fixed::Create();
    fixed->Put(infoLabel, sf::Vector2f(0,0));
    fixed->Put(quitButton, sf::Vector2f(0  , 20));
    fixed->Put(openButton, sf::Vector2f(50 , 20));
    fixed->Put(saveButton, sf::Vector2f(100, 20));
    fixed->Put(showColorButton, sf::Vector2f(0, 60));
    fixed->Put(showDepthButton, sf::Vector2f(0, 100));
    fixed->Put(showJointsButton, sf::Vector2f(0, 140));
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
    Application::request().loadFile();
}

void UserInterface::onSaveButtonClick()
{
    Application::request().toggleSave();
}

void UserInterface::onShowColorButtonClick()
{
    Application::request().toggleShowColor();
}

void UserInterface::onShowDepthButtonClick()
{
    Application::request().toggleShowDepth();
}

void UserInterface::onShowJointsButtonClick()
{
    Application::request().toggleShowJoints();
}