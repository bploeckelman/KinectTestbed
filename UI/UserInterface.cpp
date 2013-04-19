#include "UserInterface.h"

#include <SFGUI/SFGUI.hpp>

#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include "Core/Application.h"
#include "Kinect/Skeleton.h"


UserInterface::UserInterface()
	: sfgui()
	, desktop()
	, window(sfg::Window::Create())
	, box(sfg::Box::Create(sfg::Box::HORIZONTAL, 0.f))
	, infoLabel(sfg::Label::Create())
	, playLabel(sfg::Label::Create())
	, quitButton(sfg::Button::Create("Quit"))
	, openButton(sfg::Button::Create("Open"))
	, closeButton(sfg::Button::Create("Close"))
	, saveButton(sfg::ToggleButton::Create("Save"))
	, layerButton(sfg::Button::Create("Layer"))
	, playButton(sfg::ToggleButton::Create("Play"))
	, showColorButton(sfg::CheckButton::Create("Color"))
	, showDepthButton(sfg::CheckButton::Create("Depth"))
	, showSkeletonButton(sfg::CheckButton::Create("Skeleton"))
	, enableSeatedMode(sfg::CheckButton::Create("Seated Mode"))
	, showJointsButton(sfg::CheckButton::Create("Joints"))
	, showOrientationButton(sfg::CheckButton::Create("Orientation"))
	, showBonesButton(sfg::CheckButton::Create("Bones"))
	, showInferredButton(sfg::CheckButton::Create("Inferred"))
	, showJointPathButton(sfg::CheckButton::Create("Joint Path"))
	, enableHandControlButton(sfg::CheckButton::Create("Hand Controls"))
	, jointFramesProgress(sfg::ProgressBar::Create())
	, jointFramesFilename(sfg::Label::Create())
	, jointFrameIndex(sfg::Label::Create())
	, filterJointsCombo(sfg::ComboBox::Create())
	, performancesCombo(sfg::ComboBox::Create())
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
			   playButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onPlayButtonClick, this);
			  closeButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onCloseButtonClick, this);
			  layerButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onLayerButtonClick, this);
		  showColorButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onShowColorButtonClick, this);
		  showDepthButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onShowDepthButtonClick, this);
	   showSkeletonButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onShowSkeletonButtonClick, this);
		 enableSeatedMode->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onEnableSeatedModeClick, this);
	      showBonesButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onShowBonesButtonClick, this);
	     showJointsButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onShowJointsButtonClick, this);
	   showInferredButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onShowInferredButtonClick, this);
	  showJointPathButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onShowJointPathButtonClick, this);
	showOrientationButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onShowOrientationButtonClick, this);
	enableHandControlButton->GetSignal(sfg::Button::OnLeftClick).Connect(&UserInterface::onEnableHandControlButtonClick, this);
		filterJointsCombo->GetSignal(sfg::ComboBox::OnSelect).Connect(&UserInterface::onFilterComboSelect, this);
		performancesCombo->GetSignal(sfg::ComboBox::OnSelect).Connect(&UserInterface::onPerformanceComboSelect, this);
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
	enableSeatedMode->SetActive(true);

	showJointsButton->SetActive(true);
	showInferredButton->SetActive(false);
	showOrientationButton->SetActive(false);
	showBonesButton->SetActive(true);
	showJointPathButton->SetActive(false);
	enableHandControlButton->SetActive(false);

	jointFramesFilename->SetText(sf::String(""));
	jointFramesFilename->SetLineWrap(true);
	jointFramesFilename->SetZOrder(1);

	jointFrameIndex->SetText(sf::String(""));
	jointFrameIndex->SetLineWrap(true);
	jointFrameIndex->SetZOrder(1);

	jointFramesProgress->SetFraction(0.0);
	jointFramesProgress->SetRequisition(sf::Vector2f(1250, 50));

	playButton->SetActive(false);
	playButton->SetRequisition(sf::Vector2f(60, 40));

	std::stringstream ss;
	ss << "Animation info...";
	playLabel->SetText(ss.str());
	playLabel->SetLineWrap(true);

	filterJointsCombo->AppendItem("No filtering");
	filterJointsCombo->AppendItem("Low filtering");
	filterJointsCombo->AppendItem("Medium filtering");
	filterJointsCombo->AppendItem("High filtering");
	filterJointsCombo->SelectItem(2);

	performancesCombo->AppendItem("Live");
	performancesCombo->SelectItem(0);
	performancesCombo->SetRequisition(sf::Vector2f(300,20));

	sfg::Fixed::Ptr fixed = sfg::Fixed::Create();

	fixed->Put(infoLabel, sf::Vector2f(0,0));

	fixed->Put(quitButton, sf::Vector2f(0  , 20));
	fixed->Put(openButton, sf::Vector2f(50 , 20));
	fixed->Put(closeButton, sf::Vector2f(100, 20));
	fixed->Put(saveButton, sf::Vector2f(150, 20));
	fixed->Put(layerButton, sf::Vector2f(200, 20));

	fixed->Put(showColorButton, sf::Vector2f(0, 60));
	fixed->Put(showDepthButton, sf::Vector2f(0, 100));
	fixed->Put(showSkeletonButton, sf::Vector2f(0, 140));
	fixed->Put(enableSeatedMode, sf::Vector2f(0, 180));
	fixed->Put(showJointsButton, sf::Vector2f(0, 220));
	fixed->Put(showInferredButton, sf::Vector2f(0, 260));
	fixed->Put(showOrientationButton, sf::Vector2f(0, 300));
	fixed->Put(showBonesButton, sf::Vector2f(0, 340));
	fixed->Put(showJointPathButton, sf::Vector2f(0, 380));
	fixed->Put(enableHandControlButton, sf::Vector2f(0, 420));
	fixed->Put(filterJointsCombo, sf::Vector2f(0, 460));
	fixed->Put(performancesCombo, sf::Vector2f(120, 50));

	fixed->Put(playButton, sf::Vector2f(0, 600));
	fixed->Put(playLabel, sf::Vector2f(80, 615));
	fixed->Put(jointFramesProgress, sf::Vector2f(0, 650));
	fixed->Put(jointFramesFilename, sf::Vector2f(50, 655));
	fixed->Put(jointFrameIndex, sf::Vector2f(10, 655));

	box->Pack(fixed);

	window->SetTitle("Kinect Testbed");
	window->SetStyle(sfg::Window::Style::NO_STYLE);
	window->Add(box);
	desktop.Add(window);
}

void UserInterface::onQuitButtonClick()  { Application::request().shutdown(); }
void UserInterface::onOpenButtonClick()  { Application::request().loadFile(); }
void UserInterface::onCloseButtonClick() { Application::request().closeFile(); }
void UserInterface::onLayerButtonClick() { Application::request().getKinect().startLayering(); }
void UserInterface::onSaveButtonClick()  { Application::request().getKinect().toggleSave(); }
void UserInterface::onShowColorButtonClick()       { Application::request().toggleShowColor(); }
void UserInterface::onShowDepthButtonClick()       { Application::request().toggleShowDepth(); }
void UserInterface::onShowSkeletonButtonClick()    { Application::request().toggleShowSkeleton(); }
void UserInterface::onEnableSeatedModeClick()      { Application::request().getKinect().toggleSeatedMode(); }
void UserInterface::onShowJointsButtonClick()      { Application::request().getKinect().getSkeleton().toggleJoints(); }
void UserInterface::onShowInferredButtonClick()    { Application::request().getKinect().getSkeleton().toggleInferred(); }
void UserInterface::onShowOrientationButtonClick() { Application::request().getKinect().getSkeleton().toggleOrientation(); }
void UserInterface::onShowBonesButtonClick()       { Application::request().getKinect().getSkeleton().toggleBones(); }
void UserInterface::onShowJointPathButtonClick()   { Application::request().getKinect().getSkeleton().toggleJointPath(); }
void UserInterface::onEnableHandControlButtonClick() { Application::request().toggleHandControl(); }

void UserInterface::onPlayButtonClick()  {
	Application::request().toggleAutoPlay();
	const bool isPlaying = Application::request().isAutoPlay();
	playButton->SetLabel(isPlaying ? "Pause" : "Play");
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
		Application::request().setJointFrameIndex(fraction);
	}
}

void UserInterface::onFilterComboSelect()
{
	// TODO: map combo box index to string
	const sf::String& selected(filterJointsCombo->GetSelectedText());
	     if (selected == "Joint filtering - Off")    Application::request().getKinect().getSkeleton().setFilterLevel(OFF);
	else if (selected == "Joint filtering - Low")    Application::request().getKinect().getSkeleton().setFilterLevel(LOW);
	else if (selected == "Joint filtering - Medium") Application::request().getKinect().getSkeleton().setFilterLevel(MEDIUM);
	else if (selected == "Joint filtering - High")   Application::request().getKinect().getSkeleton().setFilterLevel(HIGH);
}

void UserInterface::onPerformanceComboSelect()
{
	const sf::String& selected(performancesCombo->GetSelectedText());
	const unsigned int index = performancesCombo->GetSelectedItem();
	// TODO : map combo box text to vector index of performances 
	std::cout << "Rendering performance '" << selected.toAnsiString() << "', gui index[" << index << "]" << std::endl;
	Application::request().getKinect().getSkeleton().setPerformance(selected);
}
