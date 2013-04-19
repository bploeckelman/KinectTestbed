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
	sfg::Label::Ptr playLabel;

	sfg::Button::Ptr quitButton;
	sfg::Button::Ptr openButton;
	sfg::Button::Ptr closeButton;
	sfg::Button::Ptr layerButton;

	sfg::ToggleButton::Ptr saveButton;
	sfg::ToggleButton::Ptr playButton;

	sfg::CheckButton::Ptr showColorButton;
	sfg::CheckButton::Ptr showDepthButton;
	sfg::CheckButton::Ptr showSkeletonButton;
	sfg::CheckButton::Ptr enableSeatedMode;

	sfg::CheckButton::Ptr showJointsButton;
	sfg::CheckButton::Ptr showInferredButton;
	sfg::CheckButton::Ptr showOrientationButton;
	sfg::CheckButton::Ptr showBonesButton;
	sfg::CheckButton::Ptr showJointPathButton;
	sfg::CheckButton::Ptr enableHandControlButton;

	sfg::ComboBox::Ptr filterJointsCombo;
	sfg::ComboBox::Ptr performancesCombo;

	sfg::ProgressBar::Ptr jointFramesProgress;
	sfg::Label::Ptr jointFramesFilename;
	sfg::Label::Ptr jointFrameIndex;

public:
	UserInterface();

	void draw(sf::RenderWindow &renderWindow);
	void handleEvent(sf::Event &event);

	void setInfo    (const std::string &info) { infoLabel->SetText(sf::String(info)); }
	void setPlayLabel(const std::string &text) { playLabel->SetText(sf::String(text)); }
	void setFileName(const std::string &name) { jointFramesFilename->SetText(sf::String(name)); }
	void setProgress(float fraction) { jointFramesProgress->SetFraction(fraction); }
	void setIndex(int index) {
		std::stringstream ss;
		ss << "[" << index << "]";
		jointFrameIndex->SetText(sf::String(ss.str()));
	}

	void addPerformance(const std::string& name) {
		performancesCombo->AppendItem(name);
	}
	void removePerformance( const std::string& name ) {
		// TODO : handle removing more cleanly... make it a map maybe? with unique id #s instead of vector?
		if (name == "Live") {
			std::cout << "Live performance can't be closed" << std::endl;
			return;
		} 

		for (auto i = performancesCombo->GetStartItemIndex(); i < performancesCombo->GetItemCount(); ++i) {
			if (name == performancesCombo->GetItem(i)) {
				performancesCombo->RemoveItem(i);
				break;
			}
		}
	}

	sfg::ComboBox::Ptr& getPerformancesCombo() { return performancesCombo; }

private:
	void setupWidgetHandlers();
	void setupWindowConfiguration();

	// Widget handlers...
	void onQuitButtonClick();
	void onOpenButtonClick();
	void onSaveButtonClick();
	void onCloseButtonClick();
	void onLayerButtonClick();
	void onPlayButtonClick();
	void onShowColorButtonClick();
	void onShowDepthButtonClick();
	void onShowSkeletonButtonClick();
	void onEnableSeatedModeClick();
	void onShowJointsButtonClick();
	void onShowInferredButtonClick();
	void onShowOrientationButtonClick();
	void onShowBonesButtonClick();
	void onShowJointPathButtonClick();
	void onEnableHandControlButtonClick();
	void onProgressBarMouseMove();
	void onFilterComboSelect();
	void onPerformanceComboSelect();
};

