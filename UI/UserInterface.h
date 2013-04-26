#pragma once
#include <SFGUI/SFGUI.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderWindow.hpp>


class UserInterface
{
private:
	// Toggles
	bool showColor;
	bool showDepth;
	bool showSkeleton;
	bool seatedMode;
	bool handControl;
	bool autoPlay;

	// Label info
	float lastFrameTime;

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

	bool isShowingColor() const;
	bool isShowingDepth() const;
	bool isShowingSkeleton() const;
	bool isSeatedModeEnabled() const;
	bool isHandControlEnabled() const;
	bool isAutoPlayEnabled() const;

	float getLastFrameTime() const;
	void setLastFrameTime(float frameTime);

	void setInfo(const std::string &info);
	void setPlayLabel(const std::string &text);
	void setFileName(const std::string &name);
	void setProgress(float fraction);

	void setIndex(int index);

	void addPerformance(const std::string& name);
	void removePerformance( const std::string& name );

	sfg::ComboBox::Ptr& getPerformancesCombo();

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


inline bool UserInterface::isShowingColor() const { return showColor; }
inline bool UserInterface::isShowingDepth() const { return showDepth; }
inline bool UserInterface::isShowingSkeleton() const { return showSkeleton; }
inline bool UserInterface::isSeatedModeEnabled() const { return seatedMode; }
inline bool UserInterface::isHandControlEnabled() const { return handControl; }
inline bool UserInterface::isAutoPlayEnabled() const { return autoPlay; }

inline float UserInterface::getLastFrameTime() const { return lastFrameTime; }
inline void UserInterface::setLastFrameTime(float frameTime) { lastFrameTime = frameTime; }

inline void UserInterface::setInfo(const std::string &info) { infoLabel->SetText(sf::String(info)); }
inline void UserInterface::setPlayLabel(const std::string &text) { playLabel->SetText(sf::String(text)); }
inline void UserInterface::setFileName(const std::string &name) { jointFramesFilename->SetText(sf::String(name)); }
inline void UserInterface::setProgress(float fraction) { jointFramesProgress->SetFraction(fraction); }

inline sfg::ComboBox::Ptr& UserInterface::getPerformancesCombo() { return performancesCombo; }
