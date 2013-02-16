#include "UserInterface.h"

#include <SFGUI/SFGUI.hpp>

#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include "Core/Application.h"


UserInterface::UserInterface()
    : sfgui()
    , desktop()
    , window(sfg::Window::Create())
    , box(sfg::Box::Create(sfg::Box::HORIZONTAL, 0.f))
    , infoLabel(sfg::Label::Create())
    , quitButton(sfg::Button::Create("Quit"))
    , openButton(sfg::Button::Create("Open"))
    , saveButton(sfg::ToggleButton::Create("Save"))
    , showColorButton(sfg::CheckButton::Create("Color"))
    , showDepthButton(sfg::CheckButton::Create("Depth"))
    , showJointsButton(sfg::CheckButton::Create("Joints"))
    , jointFramesProgress(sfg::ProgressBar::Create())
    , jointFramesFilename(sfg::Label::Create())
    , jointFrameIndex(sfg::Label::Create())
    , filterJointsCombo(sfg::ComboBox::Create())
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
    jointFramesProgress->GetSignal(sfg::ProgressBar::OnMouseMove)
           .Connect(&UserInterface::onProgressBarMouseMove, this);
    filterJointsCombo->GetSignal(sfg::ComboBox::OnSelect)
           .Connect(&UserInterface::onFilterComboSelect, this);
    // TODO: hook up other widget handlers as needed
}

void UserInterface::setupWindowConfiguration()
{
    infoLabel->SetText("Sensor [?] : ?");
    infoLabel->SetLineWrap(true);

    showColorButton->SetActive(false);
    showDepthButton->SetActive(false);
    showJointsButton->SetActive(true);

    jointFramesFilename->SetText(sf::String(""));
    jointFramesFilename->SetLineWrap(true);
    jointFramesFilename->SetZOrder(1);

    jointFrameIndex->SetText(sf::String(""));
    jointFrameIndex->SetLineWrap(true);
    jointFrameIndex->SetZOrder(1);

    jointFramesProgress->SetFraction(0.0);
    jointFramesProgress->SetRequisition(sf::Vector2f(1200, 25));

    filterJointsCombo->AppendItem("No joint filtering");
    filterJointsCombo->AppendItem("Low joint filtering");
    filterJointsCombo->AppendItem("Medium joint filtering");
    filterJointsCombo->AppendItem("High joint filtering");

    sfg::Fixed::Ptr fixed = sfg::Fixed::Create();
    fixed->Put(infoLabel, sf::Vector2f(0,0));
    fixed->Put(quitButton, sf::Vector2f(0  , 20));
    fixed->Put(openButton, sf::Vector2f(50 , 20));
    fixed->Put(saveButton, sf::Vector2f(100, 20));
    fixed->Put(showColorButton, sf::Vector2f(0, 60));
    fixed->Put(showDepthButton, sf::Vector2f(0, 100));
    fixed->Put(showJointsButton, sf::Vector2f(0, 140));
    fixed->Put(jointFramesProgress, sf::Vector2f(0, 650));
    fixed->Put(jointFramesFilename, sf::Vector2f(50, 655));
    fixed->Put(jointFrameIndex, sf::Vector2f(10, 655));
    fixed->Put(filterJointsCombo, sf::Vector2f(0, 180));
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

void UserInterface::onProgressBarMouseMove()
{
    // Get mouse position
    if (!sf::Mouse::isButtonPressed(sf::Mouse::Left))
        return;

    // TODO: enable dragging that started in the progress bar but moved out of it after click+holding
    const sf::Vector2i pos = Application::request().getMousePosition();
    const sf::Vector2f bounds(jointFramesProgress->GetAbsolutePosition().x
                            , jointFramesProgress->GetAbsolutePosition().x + jointFramesProgress->GetRequisition().x);
    if (pos.x >= bounds.x && pos.x <= bounds.y) {
        const float fraction = (float) (pos.x - bounds.x) / (bounds.y - bounds.x);
        jointFramesProgress->SetFraction(fraction);
        Application::request().setJointIndex(fraction);
    }
}

void UserInterface::onFilterComboSelect()
{
    // TODO: map combo box index to string
    const sf::String& selected(filterJointsCombo->GetSelectedText());
         if (selected == "Joint filtering - Off")    Application::request().setFilterLevel(OFF);
    else if (selected == "Joint filtering - Low")    Application::request().setFilterLevel(LOW);
    else if (selected == "Joint filtering - Medium") Application::request().setFilterLevel(MEDIUM);
    else if (selected == "Joint filtering - High")   Application::request().setFilterLevel(HIGH);
}
