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
	, closeButton(sfg::Button::Create("Close"))
	, saveButton(sfg::ToggleButton::Create("Save"))
	, showColorButton(sfg::CheckButton::Create("Color"))
	, showDepthButton(sfg::CheckButton::Create("Depth"))
	, showSkeletonButton(sfg::CheckButton::Create("Skeleton"))
	, enableSeatedMode(sfg::CheckButton::Create("Seated Mode"))
	, showJointsButton(sfg::CheckButton::Create("Joints"))
	, showOrientationButton(sfg::CheckButton::Create("Orientation"))
	, showBonesButton(sfg::CheckButton::Create("Bones"))
	, showInferredButton(sfg::CheckButton::Create("Inferred"))
	, showJointPathButton(sfg::CheckButton::Create("Joint Path"))
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
			   quitButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onQuitButtonClick, this);
			   openButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onOpenButtonClick, this);
			   saveButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onSaveButtonClick, this);
			  closeButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onCloseButtonClick, this);
		  showColorButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onShowColorButtonClick, this);
		  showDepthButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onShowDepthButtonClick, this);
	   showSkeletonButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onShowSkeletonButtonClick, this);
		 enableSeatedMode->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onEnableSeatedModeClick, this);
	      showBonesButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onShowBonesButtonClick, this);
	     showJointsButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onShowJointsButtonClick, this);
	   showInferredButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onShowInferredButtonClick, this);
	  showJointPathButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onShowJointPathButtonClick, this);
	showOrientationButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onShowOrientationButtonClick, this);
		 filterJointsCombo->GetSignal(sfg::ComboBox::OnSelect).Connect(&UserInterface::onFilterComboSelect, this);
	  jointFramesProgress->GetSignal(sfg::ProgressBar::OnMouseMove).Connect(&UserInterface::onProgressBarMouseMove, this);
	// TODO: hook up other widget handlers as needed
}

void UserInterface::setupWindowConfiguration()
{
	infoLabel->SetText("Sensor [?] : ?");
	infoLabel->SetLineWrap(true);

	showColorButton->SetActive(true);
	showDepthButton->SetActive(true);
	showSkeletonButton->SetActive(true);
	enableSeatedMode->SetActive(false);

	showJointsButton->SetActive(true);
	showInferredButton->SetActive(false);
	showOrientationButton->SetActive(true);
	showBonesButton->SetActive(true);
	showJointPathButton->SetActive(false);

	jointFramesFilename->SetText(sf::String(""));
	jointFramesFilename->SetLineWrap(true);
	jointFramesFilename->SetZOrder(1);

	jointFrameIndex->SetText(sf::String(""));
	jointFrameIndex->SetLineWrap(true);
	jointFrameIndex->SetZOrder(1);

	jointFramesProgress->SetFraction(0.0);
	jointFramesProgress->SetRequisition(sf::Vector2f(1250, 50));

	filterJointsCombo->AppendItem("No joint filtering");
	filterJointsCombo->AppendItem("Low joint filtering");
	filterJointsCombo->AppendItem("Medium joint filtering");
	filterJointsCombo->AppendItem("High joint filtering");

	sfg::Fixed::Ptr fixed = sfg::Fixed::Create();
	fixed->Put(infoLabel, sf::Vector2f(0,0));
	fixed->Put(quitButton, sf::Vector2f(0  , 20));
	fixed->Put(openButton, sf::Vector2f(50 , 20));
	fixed->Put(closeButton, sf::Vector2f(100, 20));
	fixed->Put(saveButton, sf::Vector2f(150, 20));
	fixed->Put(showColorButton, sf::Vector2f(0, 60));
	fixed->Put(showDepthButton, sf::Vector2f(0, 100));
	fixed->Put(showSkeletonButton, sf::Vector2f(0, 140));
	fixed->Put(jointFramesProgress, sf::Vector2f(0, 650));
	fixed->Put(jointFramesFilename, sf::Vector2f(50, 655));
	fixed->Put(jointFrameIndex, sf::Vector2f(10, 655));
	fixed->Put(enableSeatedMode, sf::Vector2f(0, 180));
	fixed->Put(showJointsButton, sf::Vector2f(0, 220));
	fixed->Put(showInferredButton, sf::Vector2f(0, 260));
	fixed->Put(showOrientationButton, sf::Vector2f(0, 300));
	fixed->Put(showBonesButton, sf::Vector2f(0, 340));
	fixed->Put(showJointPathButton, sf::Vector2f(0, 380));
	fixed->Put(filterJointsCombo, sf::Vector2f(0, 1100));
	box->Pack(fixed);

	window->SetTitle("Kinect Testbed");
	window->SetStyle(sfg::Window::Style::NO_STYLE);
	window->Add(box);
	desktop.Add(window);
}

void UserInterface::onQuitButtonClick() { Application::request().shutdown(); }
void UserInterface::onOpenButtonClick() { Application::request().loadFile(); }
void UserInterface::onCloseButtonClick() { Application::request().closeFile(); }
void UserInterface::onSaveButtonClick() { Application::request().getKinect().toggleSave(); }
void UserInterface::onShowColorButtonClick()       { Application::request().toggleShowColor(); }
void UserInterface::onShowDepthButtonClick()       { Application::request().toggleShowDepth(); }
void UserInterface::onShowSkeletonButtonClick()    { Application::request().toggleShowSkeleton(); }
void UserInterface::onEnableSeatedModeClick()      { Application::request().getKinect().toggleSeatedMode(); }
void UserInterface::onShowJointsButtonClick()      { Application::request().getKinect().getSkeleton().toggleJoints(); }
void UserInterface::onShowInferredButtonClick()    { Application::request().getKinect().getSkeleton().toggleInferred(); }
void UserInterface::onShowOrientationButtonClick() { Application::request().getKinect().getSkeleton().toggleOrientation(); }
void UserInterface::onShowBonesButtonClick()       { Application::request().getKinect().getSkeleton().toggleBones(); }
void UserInterface::onShowJointPathButtonClick()   { Application::request().getKinect().getSkeleton().toggleJointPath(); }


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
		Application::request().setJointFrameIndex(fraction);
	}
}

void UserInterface::onFilterComboSelect()
{
	// TODO: map combo box index to string
	const sf::String& selected(filterJointsCombo->GetSelectedText());
		 if (selected == "Joint filtering - Off")    Application::request().getKinect().getSkeleton().setFilterLevel(Skeleton::OFF);
	else if (selected == "Joint filtering - Low")    Application::request().getKinect().getSkeleton().setFilterLevel(Skeleton::LOW);
	else if (selected == "Joint filtering - Medium") Application::request().getKinect().getSkeleton().setFilterLevel(Skeleton::MEDIUM);
	else if (selected == "Joint filtering - High")   Application::request().getKinect().getSkeleton().setFilterLevel(Skeleton::HIGH);
}
