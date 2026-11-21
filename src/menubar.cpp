#include "menubar.h"

MenuBar::MenuBar(QMenuBar *parent) : QMenuBar(parent)
{
    addFileMenu();
    addToolsMenu();
    addViewMenu();
    addHelpMenu();
}

void MenuBar::addFileMenu()
{
    QMenu *menuFile = addMenu("File");
    
    QAction *actionExit = new QAction(QIcon::fromTheme("application-exit"), "Exit");
    //exitAction->setShortcut(QKeySequence::fromString("Ctrl+Q"));
    actionExit->setStatusTip("Exit Application");
    connect(actionExit, &QAction::triggered, this, &MenuBar::quitApplication);
        
    QAction *actionQuickSave = new QAction(QIcon::fromTheme("document-save"), "QuickSave Instruments");
    //saveAction->setShortcut(QKeySequence::fromString("Ctrl+S"));
    connect(actionQuickSave, &QAction::triggered, this, &MenuBar::quickSave);
    
    QAction *actionQuickLoad = new QAction(QIcon::fromTheme("document-open"), "QuickLoad Instruments");
    connect(actionQuickLoad, &QAction::triggered, this, [this]{ emit signalLoadQuick(); });
    
    QAction *actionSave = new QAction(QIcon::fromTheme("document-save"), "Save Instruments");
    connect(actionSave, &QAction::triggered, this, [this]{ emit signalSave(); });
    
    QAction *actionLoad = new QAction(QIcon::fromTheme("document-open"), "Load Instruments");
    connect(actionLoad, &QAction::triggered, this, [this]{ emit signalLoad(); });
    
    
    
    menuFile->addAction(actionQuickSave);
    menuFile->addAction(actionQuickLoad);
    //menuFile->addSeparator();
    menuFile->addAction(actionSave);
    menuFile->addAction(actionLoad);
    menuFile->addSeparator();
    menuFile->addAction(actionExit);
}
void MenuBar::addToolsMenu()
{
    QMenu *menuTools = addMenu("Tools");
    
    QAction *globalKeyShift = new QAction("Global Key Shift");
    connect(globalKeyShift, &QAction::triggered, this, &MenuBar::globalKeyShift);
    
    QAction *globalKeyMap = new QAction("Keymap Editor...");
    connect(globalKeyMap, &QAction::triggered, this, &MenuBar::signalKeymapEditor);
    
    
    QAction *globalResendMIDISettings = new QAction("Resend all MIDI Settings");
    connect(globalResendMIDISettings, &QAction::triggered, this, &MenuBar::globalResendMIDISettings);
    
    QAction *actionResetAll = new QAction();
    
    QAction *actionResetTab = new QAction();
    
    QAction *actionCopyTab = new QAction();
    
    QAction *actionPasteTab = new QAction();
    
    menuTools->addAction(globalResendMIDISettings);
    menuTools->addAction(globalKeyMap);
    menuTools->addAction(globalKeyShift);
}
void MenuBar::addViewMenu()
{
    QMenu *menuView = addMenu("View");
    
    this->showNetwork = new QAction("Show Network Section");
    this->showNetwork->setCheckable(true);
    this->showNetwork->setChecked(false);
    connect(this->showNetwork, &QAction::toggled, this, &MenuBar::showNetworkActionChanged);
    
    this->showKeys = new QAction("Show Keys Section");
    this->showKeys->setCheckable(true);
    this->showKeys->setChecked(true);
    connect(this->showKeys, &QAction::toggled, this, &MenuBar::showKeysActionChanged);
    
    this->showPitch = new QAction("Show Pitchwheel and Volume Section");
    this->showPitch->setCheckable(true);
    this->showPitch->setChecked(true);
    connect(this->showPitch, &QAction::toggled, this, &MenuBar::showPitchActionChanged);
    
    this->showNotes = new QAction("Show Notes Section");
    this->showNotes->setCheckable(true);
    this->showNotes->setChecked(true);
    connect(this->showNotes, &QAction::toggled, this, &MenuBar::showNotesActionChanged);
    
    this->showPCKeyboard = new QAction("Show PC-Keyboard Section");
    this->showPCKeyboard->setCheckable(true);
    this->showPCKeyboard->setChecked(true);
    connect(this->showPCKeyboard, &QAction::toggled, this, &MenuBar::showPCKeyboardActionChanged);
    
    this->showPianoKeyboard = new QAction("Show Piano-Keyboard Section");
    this->showPianoKeyboard->setCheckable(true);
    this->showPianoKeyboard->setChecked(false);
    connect(this->showPianoKeyboard, &QAction::toggled, this, &MenuBar::showPianoKeyboardActionChanged);
    
    menuView->addAction(showNetwork);
    menuView->addAction(showKeys);
    menuView->addAction(showPitch);
    menuView->addAction(showNotes);
    menuView->addAction(showPCKeyboard);
    menuView->addAction(showPianoKeyboard);
    
    // for all keyboards; MainWindow sets the saved state (setSoundingNotesChecked)
    this->showSoundingNotes = new QAction("Show Sounding Notes on the PC Keyboards");
    this->showSoundingNotes->setCheckable(true);
    this->showSoundingNotes->setChecked(true);
    this->showSoundingNotes->setToolTip("A small line on every key: the octaves (or notes) it sounds as right now");
    connect(this->showSoundingNotes, &QAction::toggled, this, &MenuBar::signalShowSoundingNotes);
    menuView->addSeparator();
    menuView->addAction(this->showSoundingNotes);
}

void MenuBar::setSoundingNotesChecked(bool checked)
{
    const QSignalBlocker blocker(this->showSoundingNotes);
    this->showSoundingNotes->setChecked(checked);
}
void MenuBar::addHelpMenu()
{
    QMenu *menuHelp = addMenu("Help");
    
    QAction *actionLayout = new QAction("Keyboard Layout");
    actionLayout->setToolTip("How the notes are arranged: the chromatic button accordion system");
    connect(actionLayout, &QAction::triggered, this, [this]{ emit signalHelpLayout();} );
    
    QAction *actionKeyboards = new QAction("Keyboard Recommendations");
    actionKeyboards->setToolTip("Which computer keyboards work well for live performance");
    connect(actionKeyboards, &QAction::triggered, this, [this]{ emit signalHelpKeyboards();} );
    
    QAction *actionAbout = new QAction("About");
    connect(actionAbout, &QAction::triggered, this, [this]{ emit signalHelpAbout();} );
    
    QAction *actionInterception = new QAction("Windows Lock Driver (Interception)");
    actionInterception->setToolTip("The Interception driver: installation and what to know before a performance");
    connect(actionInterception, &QAction::triggered, this, [this]{ emit signalHelpInterception();} );
    
    QAction *actionMultiplexer = new QAction("MIDI Multiplexer");
    actionMultiplexer->setToolTip("Layers and splits for a MIDI keyboard, controller or sequencer");
    connect(actionMultiplexer, &QAction::triggered, this, [this]{ emit signalHelpMultiplexer();} );
    
    menuHelp->addAction(actionLayout);
    menuHelp->addAction(actionKeyboards);
    menuHelp->addAction(actionInterception);
    menuHelp->addAction(actionMultiplexer);
    menuHelp->addSeparator();
    menuHelp->addAction(actionAbout);
}

void MenuBar::quitApplication()
{
    QApplication::quit();
}

void MenuBar::quickSave()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "QuickSave", "Overwrite QuickSave?", QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        emit signalSaveQuick();
    }
}

void MenuBar::showNetworkActionChanged()
{
    emit signalShowActionChanged(GUIElements::Network, this->showNetwork->isChecked());
}
void MenuBar::showKeysActionChanged()
{
    emit signalShowActionChanged(GUIElements::MIDIKeys, this->showKeys->isChecked());
}
void MenuBar::showPitchActionChanged()
{
    emit signalShowActionChanged(GUIElements::PitchAndVolume, this->showPitch->isChecked());
}
void MenuBar::showNotesActionChanged()
{
    emit signalShowActionChanged(GUIElements::Notes, this->showNotes->isChecked());
}
void MenuBar::showPCKeyboardActionChanged()
{
    emit signalShowActionChanged(GUIElements::KeyboardPC, this->showPCKeyboard->isChecked());
}
void MenuBar::showPianoKeyboardActionChanged()
{
    emit signalShowActionChanged(GUIElements::KeyboardPiano, this->showPianoKeyboard->isChecked());
}

void MenuBar::globalKeyShift()
{
    emit signalGlobalKeyShift();
}

void MenuBar::globalResendMIDISettings()
{
    emit signalResendMIDISettings();
}
