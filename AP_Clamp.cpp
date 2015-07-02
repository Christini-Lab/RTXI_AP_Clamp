/*
 * Copyright (C) 2015 Weill Medical College of Cornell University
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*** INTRO
 * Action Potential Clamp
 * 
 * AP_Clamp.cpp, v1.0
 *
 * Author: Francis A. Ortega (2015)
 *
 * Notes in header
 *
 ***/

/* Include */
#include <AP_Clamp.h>
#include <iostream>
#include <math.h>
#include <main_window.h>

#include <QtGui>

//#include "/usr/local/rtxi/plugins/data_recorder/data_recorder.h"
//#include "/home/user/Projects/rtxi/plugins/data_recorder/data_recorder.h"

using namespace std;

namespace {
	class AP_Clamp_SyncEvent : public RT::Event {
		public:
			int callback(void) {
				return 0;
			}
	};
}

// Create Module Instance
extern "C" Plugin::Object *createRTXIPlugin(void) {
    return new AP_Clamp::Module();
}

// Inputs, Outputs, and Parameters
static Workspace::variable_t vars[] = {
    {
        "Input (V or A)", "Input (V or A)", Workspace::INPUT, }, // Voltage or current of target cell, from amplifier, input(0)
    {
        "Output (V or A)", "Output (V or A)", Workspace::OUTPUT, }, //  Current sent to target cell, to internal input, output(0)
    {
        "Digital Output", "Digital Output", Workspace::OUTPUT, },
    // States
    {
        "Time (ms)", "Time Elapsed (ms)", Workspace::STATE, }, 
    {
        "Voltage (mv)", "Membrane voltage of target cell (mv)", Workspace::STATE, }, // Voltage in mV, converted from amplifier input
    {
        "Beat Number", "Number of beats", Workspace::STATE, },
    {
        "APD (ms)", "Action Potential Duration of cell (ms)", Workspace::STATE, },
    // Parameters
    {
        "APD Repolarization %", "APD Repolarization %", Workspace::PARAMETER, },
    {
        "Minimum APD (ms)", "Minimum depolarization duration considered to be an action potential (ms)", Workspace::PARAMETER, },
    {
        "Stim Window (ms)", "Window of time after stimulus that is ignored by APD calculation", Workspace::PARAMETER, }, 
    {
        "Number of Trials", "Number of times the protocol will be repeated", Workspace::PARAMETER, }, 
    {
        "Interval Time (ms)", "Amont of time between each trial", Workspace::PARAMETER, }, 
    {
        "BCL (ms)", "Basic Cycle Length", Workspace::PARAMETER, }, 
    {
        "Stim Mag (nA)", "Amplitude of stimulation pulse (nA)", Workspace::PARAMETER, }, 
    {
        "Stim Length (ms)", "Duration of stimulation pulse (nA", Workspace::PARAMETER, }, 
    {
        "LJP (mv)", "Liquid Junction Potential (mV)", Workspace::PARAMETER, },    
};

// Number of variables in vars
static size_t num_vars = sizeof(vars) / sizeof(Workspace::variable_t);

AP_Clamp::Module::Module(void) : QWidget( MainWindow::getInstance()->centralWidget() ), RT::Thread( 0 ), Workspace::Instance( "Action Potential Clamp", vars, num_vars ) {

    // Build Module GUI
	 QWidget::setAttribute(Qt::WA_DeleteOnClose);
    createGUI();
    initialize(); // Initialize parameters, initialize states, reset model, and update rate
    refreshDisplay();
    show();
} // End constructor

AP_Clamp::Module::~Module(void) {
    delete protocol;
	 setActive(false);
	 AP_Clamp_SyncEvent event;
	 RT::System::getInstance()->postEvent(&event);
} // End destructor

void AP_Clamp::Module::execute(void) { // Real-Time Execution
    voltage = input(0) * 1e3 - LJP;
    
    switch( executeMode ) {
    case IDLE:
        break;

    case THRESHOLD:
        // Apply stimulus for given number of ms (StimLength) 
        if( time - cycleStartTime <= stimLength ) {
            backToBaseline = false;
            peakVoltageT = Vrest;
            output( 0 ) = stimulusLevel * 1e-9; // stimulsLevel is in nA, convert to A for amplifier
        }
        
        else {
            output( 0 ) = 0;

            if( voltage > peakVoltageT ) // Find peak voltage after stimulus
                peakVoltageT = voltage;

            // If Vm is back to resting membrane potential (within 2 mV; determined when threshold detection button is first pressed) 
            if( voltage-Vrest < 2 ) { // Vrest: voltage at the time threshold test starts
                if ( !backToBaseline ) {
                    responseDuration = time-cycleStartTime;
                    responseTime = time;
                    backToBaseline = true;
                }

                // Calculate time length of voltage response
                if( responseDuration > 50 && peakVoltageT > 10 ) { // If the response was more than 50ms long and peakVoltage is more than 10mV, consider it an action potential
                    stimMag = stimulusLevel*1.5; // Set the current stimulus value as the calculated threshold * 2
                    thresholdOn = false;
                    executeMode = IDLE;
                }
                // If no action potential occurred, and Vm is back to rest 
                else {

                    // If the cell has rested for  200ms since returning to baseline 
                    if( time-responseTime > 200 ) {
                        stimulusLevel += 0.1; // Increase the magnitude of the stimulus and try again 
                        cycleStartTime = time; // Record the time of stimulus application 
                    }   
                }
            }
        }
        time += period;
        break;

    case PACE:
        
        time += period;
        stepTime += 1;
        // If time is greater than BCL, advance the beat
        if ( stepTime - cycleStartTime >= BCLInt ) {
            beatNum++;            
            cycleStartTime = stepTime;
            Vrest = voltage;
            calculateAPD( 1 ); // First step of APD calculation called at each stimulus
        }
        
        // Stimulate cell for stimLength(ms), digital out on for duration of stimulus
        if ( (stepTime - cycleStartTime) < stimLengthInt ) {
            outputCurrent = stimMag * 1e-9; // stimMag in nA, convert to A for amplifier
            digitalOut = 1;
        }
        else {
            outputCurrent = 0;
            digitalOut = 0;
        }

        // Inject Current
        output( 0 ) = outputCurrent;
        output( 1 ) = digitalOut;
        //Calulate APD
        calculateAPD( 2 ); // Second step of APD calculation
        break;

    case PROTOCOL:

        time += period;
        stepTime += 1;

        if (protocolMode == STEPINIT) {
            stepInitDone = false;

            // These steps do not consume a thread loop by themselves
            while (!stepInitDone) {
                // End of protocol
                if (currentStep >= protocolContainer->size()) {// If end of protocol has been reached
                    protocolMode = END;
                    stepInitDone = true;
                }
                else {
                    stepPtr = protocolContainer->at(currentStep);
                    stepType = stepPtr->stepType;

                    // Start data recording
                    if (stepType == ProtocolStep::STARTRECORD) {
                        if( !recording ) { // Record data if dataRecord is toggled
                            Event::Object event(Event::START_RECORDING_EVENT);
                            Event::Manager::getInstance()->postEventRT(&event);
                            recording = true;
                        }
                        currentStep++;
                    }
                    // Stop data recording
                    else if (stepType == ProtocolStep::STOPRECORD) {
                        if(recording == true) {
                            Event::Object event(Event::STOP_RECORDING_EVENT);
                            Event::Manager::getInstance()->postEventRT(&event);
                            recording = false;
                        }
                        currentStep++;
                    }
                    // Start Vm recording init
                    else if (stepType == ProtocolStep::STARTVM) {
                        vmRecording = true;
                        recordingIndex = stepPtr->recordIdx;
                        vmRecordData = &voltageData[recordingIndex];
                        vmRecordData->clear();
                        vmRecordCnt = 0;
                        currentStep++;
                    }
                    // Stop Vm recording init
                    else if (stepType == ProtocolStep::STOPVM) {
                        vmRecording = false;
                        currentStep++;
                    }
                    else {
                        stepTime = 0;
                        cycleStartTime = 0;
                        pBCLInt = stepPtr->BCL / period; // BCL for protocol

                        // Pace, Average, and AP Clamp Init
                        if (stepType == ProtocolStep::PACE ||
                            stepType == ProtocolStep::AVERAGE ||
                            stepType == ProtocolStep::APCLAMP ) {
                            
                            stepEndTime = (( stepPtr->BCL * stepPtr->numBeats ) / period ) - 1; // -1 since time starts at 0, not 1
                            beatNum++;

                            if ( stepType == ProtocolStep::AVERAGE ) {
                                recordingIndex = stepPtr->recordIdx;
                                avgRecordData = &voltageData[recordingIndex];
                                avgRecordData->clear();
                                avgRecordData->resize( stepPtr->BCL / period ); // All elements are set to 0
                                avgCnt = 1; // Keeps track of how many beats have been added
                            }
                            else if ( stepType == ProtocolStep::APCLAMP ) {
                                recordingIndex = stepPtr->recordIdx;
                                apClampData = &voltageData[recordingIndex];
                                apClampCnt = 1;
                            }
                        }
                        // Wait Init
                        else {
                            stepEndTime = ( stepPtr->waitTime / period ) - 1; // -1 since time starts at 0, not 1
                        }
                        
                        protocolMode = EXEC;
                        Vrest = voltage;
                        calculateAPD( 1 );
                        stepInitDone = true;

                        if ( stepType == ProtocolStep::APCLAMP && pBCLInt > apClampData->size() ) {
                            ERROR_MSG("AP_Clamp Error: Not enough data for entire step\n");
                            protocolMode = END;
                        }
                    }                   
                }
                
            } // end while (!stepInitiDone)            
        } // end if (protocolMode == STEPINIT)
   
        if ( protocolMode == EXEC ) { // Execute protocol

            // Static Pacing or Averaging
            if ( stepType == ProtocolStep::PACE || stepType == ProtocolStep::AVERAGE) { // Pace cell at BCL
                if (stepTime - cycleStartTime >= pBCLInt) {
                    beatNum++;
                    cycleStartTime = stepTime;
                    Vrest = voltage;
                    calculateAPD( 1 );
                    if ( stepType == ProtocolStep::AVERAGE )
                        avgCnt++;
                }
                
                // Stimulate cell for stimLength(ms), digital out on for duration for stimulus
                if ( (stepTime - cycleStartTime) < stimLengthInt ) {
                    outputCurrent = stimMag * 1e-9;
                    digitalOut = stepPtr->digitalOut;
                }
                else {
                    outputCurrent = 0;
                    digitalOut = 0;
                }

                if ( stepType == ProtocolStep::AVERAGE ) {
                    if ( avgCnt == stepPtr->numBeats ) // Voltage in mV
                        avgRecordData->at(stepTime - cycleStartTime) =
                            (voltage + avgRecordData->at(stepTime - cycleStartTime)) / avgCnt;
                    else
                        avgRecordData->at(stepTime - cycleStartTime) = voltage + avgRecordData->at(stepTime - cycleStartTime);
                }
                output(0) = outputCurrent;
                output(1) = digitalOut;
                calculateAPD(2);
                
            } // end if(PACE || AVERAGE)

            // Wait
            else if ( stepType == ProtocolStep::WAIT ) { 
                output(0) = 0;
            }
            
            // AP Clamp
            else {
                if (stepTime - cycleStartTime >= pBCLInt) {
                    beatNum++;
                    output(1) = stepPtr->digitalOut;
                    cycleStartTime = stepTime;
                }
                if (stepTime - cycleStartTime > (50 / period) && stepPtr->digitalOut != 0) // Digital out on for 50ms
                    output(1) = 0;
                voltage = apClampData->at(stepTime - cycleStartTime);
                output(0) = (voltage * 1e-3) + (LJP * 1e-3);
            }
            
            if ( vmRecording ) {
                vmRecordData->push_back(voltage); // Voltage in mV
            }
            
            if( stepTime >= stepEndTime ) {
                currentStep++;
                protocolMode = STEPINIT;
            }            
        } // end EXEC

        if( protocolMode == END ) { // End of Protocol: Stop Data recorder and untoggle button
            if(recording == true) {
                Event::Object event(Event::STOP_RECORDING_EVENT);
                Event::Manager::getInstance()->postEventRT(&event);
                recording = false;
            }
            if (currentTrial < numTrials) {
                reset();
                beatNum = 0; // beatNum is changed at beginning of protocol, so it must start at 0 instead of 1
                stepTracker = -1; // Used to highlight the current step in list box, -1 to force first step to be highlighted
                protocolMode = STEPINIT;
                executeMode = PROTOCOL;
                currentTrial++;
            }
            else {
                protocolOn = false;
                executeMode = IDLE;
            }
        } // end END
            
        break;
        
    } // end switch( executeMode )     
} // end execute()

void AP_Clamp::Module::initialize(void){ // Initialize all variables, protocol, and model cell
    protocol = new Protocol();
    protocolContainer = &protocol->protocolContainer; // Pointer to protocol container
        
    // States
    time = 0;
    voltage = 0;
    beatNum = 0;
    APD = 0;
	 executeMode = IDLE;

    // Parameters
    APDRepol = 90;    
    minAPD = 50;
    stimWindow = 4;
    numTrials = 1;
    intervalTime = 1000;
    BCL = 1000;
    stimMag = 4;
    stimLength = 1;
    LJP = 0;
    
    mainWindow->APDRepolEdit->setText( QString::number(APDRepol) );
    mainWindow->minAPDEdit->setText( QString::number(minAPD) );
    mainWindow->stimWindowEdit->setText( QString::number(stimWindow) );
    mainWindow->numTrialEdit->setText( QString::number(numTrials) );
    mainWindow->intervalTimeEdit->setText( QString::number(intervalTime) );
    mainWindow->BCLEdit->setText( QString::number(BCL) );
    mainWindow->stimMagEdit->setText( QString::number(stimMag) );
    mainWindow->stimLengthEdit->setText( QString::number(stimLength) );
    mainWindow->LJPEdit->setText( QString::number(LJP) );
    
    // Flags
    recording = false;
    loadedFile = "";
    protocolOn = false;

    // APD parameters
   upstrokeThreshold = -40;

   // AP Clamp Variables
    voltageData.resize(100);
}

void AP_Clamp::Module::reset( void ) {
    period = RT::System::getInstance()->getPeriod()*1e-6; // Grabs RTXI thread period and converts to ms (from ns)
    BCLInt = BCL / period;
    stimLengthInt = stimLength / period;
     
    stepTime = -1;
    time = -period;
    cycleStartTime = 0;
    beatNum = 1;
    Vrest = voltage;
    calculateAPD( 1 );

    // Protocol variables
    currentStep = 0;
}

void AP_Clamp::Module::addStep( void ) {
    int idx = mainWindow->protocolEditorListBox->currentRow();
    if( idx == -1 ) { // Protocol is empty or nothing is selected, add step to end
        if( protocol->addStep( this ) )   // Update protocolEditorListBox if a step was added
            rebuildListBox();
    }
    else // If a step is selected, add step after
        if( protocol->addStep( this, idx ) )   // Update protocolEditorListBox if a step was added
            rebuildListBox();            
}

void AP_Clamp::Module::deleteStep( void ) {
    int idx = mainWindow->protocolEditorListBox->currentRow();
    if( idx == -1 ) // Protocol is empty or nothing is selected, return
        return ;
    
    protocol->deleteStep( this, idx ); // Delete the currently selected step in the list box
    rebuildListBox();
}

void AP_Clamp::Module::saveProtocol( void ) {
    loadedFile = protocol->saveProtocol( this );
}

void AP_Clamp::Module::loadProtocol( void ) {
    loadedFile = protocol->loadProtocol( this );
    rebuildListBox();
}

void AP_Clamp::Module::clearProtocol( void ) {
    protocol->clearProtocol();
    rebuildListBox();
}

void AP_Clamp::Module::toggleThreshold( void ) {
    thresholdOn = mainWindow->thresholdButton->isChecked();
	 
    setActive(false); //breakage maybe...
	 AP_Clamp_SyncEvent event;
	 RT::System::getInstance()->postEvent(&event);

    if( thresholdOn ) { // Start protocol, reinitialize parameters to start values
        executeMode = THRESHOLD;
        reset();
        Vrest = input(0) * 1e3;
        peakVoltageT = Vrest;
        stimulusLevel = 2.0; // na
        responseDuration = 0;
        responseTime = 0;
        setActive( true );
    }
    else { // Stop protocol, only called when pace button is unclicked in the middle of a run
        executeMode = IDLE;
        setActive( false );
    }
}

void AP_Clamp::Module::toggleProtocol( void ) {
    bool protocolOn = mainWindow->startProtocolButton->isChecked();

	 setActive(false);
	 AP_Clamp_SyncEvent event;
	 RT::System::getInstance()->postEvent(&event);
    if( protocolOn ){
        if( protocolContainer->size() <= 0 ) {
				QMessageBox * msgBox = new QMessageBox;
				msgBox->setWindowTitle("Error");
				msgBox->setText("No protocol entered");
				msgBox->setStandardButtons(QMessageBox::Ok);
				msgBox->setDefaultButton(QMessageBox::NoButton);
				msgBox->setWindowModality(Qt::WindowModal);
				msgBox->open();
            protocolOn = false;
				executeMode = IDLE;
        } else {
			  executeMode = IDLE; // Keep on IDLE until update is finished
			  reset();
			  beatNum = 0; // beatNum is changed at beginning of protocol, so it must start at 0 instead of 1
			  stepTracker = -1; // Used to highlight the current step in list box, -1 to force first step to be highlighted
			  protocolMode = STEPINIT; 
			  executeMode = PROTOCOL;
              currentTrial = 1;
			  setActive( true );
		  }
	 } else { // Stop protocol, only called when protocol button is unclicked in the middle of a run
		  if( recording ) { // Stop data recorder if recording
            ::Event::Object event(::Event::STOP_RECORDING_EVENT);
            ::Event::Manager::getInstance()->postEventRT(&event);
				recording = false;
        } 
        executeMode = IDLE;
        setActive( false );
	 }
}

void AP_Clamp::Module::togglePace( void ) {
    paceOn = mainWindow->staticPacingButton->isChecked();

	 setActive(false);
	 AP_Clamp_SyncEvent event;
	 RT::System::getInstance()->postEvent(&event);
    
	 if( paceOn ) { // Start protocol, reinitialize parameters to start values
        reset();
        executeMode = PACE;
        setActive( true );
    }
    else { // Stop protocol, only called when pace button is unclicked in the middle of a run
        if( recording ) { // Stop data recorder if recording
            ::Event::Object event(::Event::STOP_RECORDING_EVENT);
            ::Event::Manager::getInstance()->postEventRT(&event);
				recording = false;
        }        
        executeMode = IDLE;
        setActive( false );
    }
}

/*** Other Functions ***/

void AP_Clamp::Module::Module::calculateAPD(int step){ // Two APDs are calculated based on different criteria
    switch( step ) {
    case 1:
        APDMode = START;
        break;

    case 2:
        switch( APDMode ) { 
        case START:// Find time membrane voltage passes upstroke threshold, start of AP            
            if( voltage >= upstrokeThreshold ) {
                APStart = time;
                peakVoltage = Vrest;
                APDMode = PEAK;
            }
            break;
            
        case PEAK: // Find peak of AP, points within "window" are ignored to eliminate effect of stimulus artifact
            if( (time - APStart) > stimWindow ) { // If we are outside the chosen time window after the AP
                if( peakVoltage < voltage  ) { // Find peak voltage                    
                    peakVoltage = voltage;
                    peakTime = time;
                }
                else if ( (time - peakTime) > 5 ) { // Keep looking for the peak for 5ms to account for noise
                    double APAmp;                    
                    APAmp = peakVoltage - Vrest ; // Amplitude of action potential based on resting membrane and peak voltage
                    // Calculate downstroke threshold based on AP amplitude and desired AP repolarization %
                    downstrokeThreshold = peakVoltage - ( APAmp * (APDRepol / 100.0) );
                    APDMode = DOWN;
                }
            }
            break;
            
        case DOWN: // Find downstroke threshold and calculate APD
            if( voltage <= downstrokeThreshold ) {
                APD = time - APStart;
                APDMode = DONE;
            }
            break;

        default: // DONE: APD has been found, do nothing
            break;
        }
    }
}

// Rebuilds list box, run after modifying protocol
void AP_Clamp::Module::rebuildListBox( void ) {
    mainWindow->protocolEditorListBox->clear(); // Clear list box

    // Rebuild list box
    for( int i = 0; i < protocolContainer->size(); i++ ) {
        mainWindow->protocolEditorListBox->insertItem( i,  protocol->getStepDescription( i ) );
    }
}
/* Build Module GUI */
void AP_Clamp::Module::createGUI( void ) {

    QMdiSubWindow *subWindow  = new QMdiSubWindow;
    subWindow->setWindowTitle( QString::number( getID() ) + " Action Potential Dynamic Clamp" );
    subWindow->setWindowIcon(QIcon("/usr/local/lib/rtxi/RTXI-widget-icon.png"));
    subWindow->setMinimumSize(300,450);
    MainWindow::getInstance()->createMdi(subWindow); 
    subWindow->setWidget(this);

    mainWindow = new AP_ClampUI(subWindow);
    QVBoxLayout *layout = new QVBoxLayout(this);
	 setLayout(layout);
	 layout->addWidget(mainWindow);

    // Set GUI refresh rate
    QTimer *timer = new QTimer(this);
    timer->start(500);

    // Set validators
    mainWindow->APDRepolEdit->setValidator( new QIntValidator(mainWindow->APDRepolEdit) );
    mainWindow->minAPDEdit->setValidator( new QIntValidator(mainWindow->minAPDEdit) );
    mainWindow->stimWindowEdit->setValidator( new QIntValidator(mainWindow->stimWindowEdit) );
    mainWindow->numTrialEdit->setValidator( new QIntValidator(mainWindow->numTrialEdit) );
    mainWindow->intervalTimeEdit->setValidator( new QIntValidator(mainWindow->intervalTimeEdit) );
    mainWindow->BCLEdit->setValidator( new QIntValidator(mainWindow->BCLEdit) );
    mainWindow->stimMagEdit->setValidator( new QDoubleValidator(mainWindow->stimMagEdit) );
    mainWindow->stimLengthEdit->setValidator( new QDoubleValidator(mainWindow->stimLengthEdit) );
    mainWindow->LJPEdit->setValidator( new QDoubleValidator(mainWindow->LJPEdit) );
    
    // Connect MainWindow elements to slot functions
    QObject::connect( mainWindow->addStepButton, SIGNAL(clicked(void)), this, SLOT( addStep(void)) );
    QObject::connect( mainWindow->deleteStepButton, SIGNAL(clicked(void)), this, SLOT( deleteStep(void)) );
    QObject::connect( mainWindow->saveProtocolButton, SIGNAL(clicked(void)), this, SLOT( saveProtocol(void)) );
    QObject::connect( mainWindow->loadProtocolButton, SIGNAL(clicked(void)), this, SLOT( loadProtocol(void)) );
    QObject::connect( mainWindow->clearProtocolButton, SIGNAL(clicked(void)), this, SLOT( clearProtocol(void)) );
    QObject::connect( mainWindow->startProtocolButton, SIGNAL(toggled(bool)), this, SLOT( toggleProtocol(void)) );
    QObject::connect( mainWindow->thresholdButton, SIGNAL(clicked(void)), this, SLOT( toggleThreshold(void)) );
    QObject::connect( mainWindow->staticPacingButton, SIGNAL(clicked(void)), this, SLOT( togglePace(void)) );
    QObject::connect( mainWindow->resetButton, SIGNAL(clicked(void)), this, SLOT( reset(void)) );
    QObject::connect( mainWindow->APDRepolEdit, SIGNAL(returnPressed(void)), this, SLOT( modify(void)) );
    QObject::connect( mainWindow->minAPDEdit, SIGNAL(returnPressed(void)), this, SLOT( modify(void)) );
    QObject::connect( mainWindow->stimWindowEdit, SIGNAL(returnPressed(void)), this, SLOT( modify(void)) );
    QObject::connect( mainWindow->numTrialEdit, SIGNAL(returnPressed(void)), this, SLOT( modify(void)) );
    QObject::connect( mainWindow->intervalTimeEdit, SIGNAL(returnPressed(void)), this, SLOT( modify(void)) );
    QObject::connect( mainWindow->BCLEdit, SIGNAL(returnPressed(void)), this, SLOT( modify(void)) );
    QObject::connect( mainWindow->stimMagEdit, SIGNAL(returnPressed(void)), this, SLOT( modify(void)) );
    QObject::connect( mainWindow->stimLengthEdit, SIGNAL(returnPressed(void)), this, SLOT( modify(void)) );
    QObject::connect( mainWindow->LJPEdit, SIGNAL(returnPressed(void)), this, SLOT( modify(void)) );
    QObject::connect(timer, SIGNAL(timeout(void)), this, SLOT(refreshDisplay(void)));

    // Connections to allow only one button being toggled at a time
    QObject::connect( mainWindow->thresholdButton, SIGNAL(toggled(bool)), mainWindow->staticPacingButton, SLOT( setDisabled(bool)) );
    QObject::connect( mainWindow->thresholdButton, SIGNAL(toggled(bool)), mainWindow->startProtocolButton, SLOT( setDisabled(bool)) );
    QObject::connect( mainWindow->startProtocolButton, SIGNAL(toggled(bool)), mainWindow->staticPacingButton, SLOT( setDisabled(bool)) );
    QObject::connect( mainWindow->startProtocolButton, SIGNAL(toggled(bool)), mainWindow->thresholdButton, SLOT( setDisabled(bool)) );
    QObject::connect( mainWindow->staticPacingButton, SIGNAL(toggled(bool)), mainWindow->thresholdButton, SLOT( setDisabled(bool)) );
    QObject::connect( mainWindow->staticPacingButton, SIGNAL(toggled(bool)), mainWindow->startProtocolButton, SLOT( setDisabled(bool)) );
                      
    // Connect states to workspace
    setData( Workspace::STATE, 0, &time );
    setData( Workspace::STATE, 1, &voltage );
    setData( Workspace::STATE, 2, &beatNum );
    setData( Workspace::STATE, 3, &APD );

	 subWindow->show();
} // End createGUI()

// Load from Settings
void AP_Clamp::Module::doLoad(const Settings::Object::State &s) {
    if (s.loadInteger("Maximized"))
        showMaximized();
    else if (s.loadInteger("Minimized"))
        showMinimized();
    
    if (s.loadInteger("W") != NULL) {
        parentWidget()->resize(s.loadInteger("W"), s.loadInteger("H"));
        parentWidget()->move(s.loadInteger("X"), s.loadInteger("Y"));
    }

    loadedFile = QString::fromStdString(s.loadString("Protocol"));
    if( loadedFile != "" ) {        
        protocol->loadProtocol( this, loadedFile );
        rebuildListBox();
    }

    mainWindow->APDRepolEdit->setText( QString::number( s.loadInteger("APD Repol") ) );
    mainWindow->minAPDEdit->setText( QString::number( s.loadInteger("Min APD") ) );
    mainWindow->stimWindowEdit->setText( QString::number( s.loadInteger("Stim Window") ) );
    mainWindow->numTrialEdit->setText( QString::number( s.loadInteger("Num Trials") ) );
    mainWindow->intervalTimeEdit->setText( QString::number( s.loadInteger("Interval Time") ) );
    mainWindow->BCLEdit->setText( QString::number( s.loadInteger("BCL") ) );
    mainWindow->stimMagEdit->setText( QString::number( s.loadInteger("Stim Mag") ) );
    mainWindow->stimLengthEdit->setText( QString::number( s.loadInteger("Stim Length") ) );
    mainWindow->LJPEdit->setText( QString::number( s.loadInteger("LJP") ) );    
    
    modify();
}

// Save Settings
void AP_Clamp::Module::doSave(Settings::Object::State &s) const {
    if (isMaximized())
        s.saveInteger( "Maximized", 1 );
    else if (isMinimized())
        s.saveInteger( "Minimized", 1 );

    QPoint pos = parentWidget()->pos();
    s.saveInteger( "X", pos.x() );
    s.saveInteger( "Y", pos.y() );
    s.saveInteger( "W", parentWidget()->width() );
    s.saveInteger( "H", parentWidget()->height() );
    s.saveString( "Protocol", loadedFile.toStdString() );
    s.saveInteger( "APD Repol", APDRepol );
    s.saveInteger( "Min APD", minAPD );
    s.saveInteger( "Stim Window", stimWindow );
    s.saveInteger( "Num Trials", numTrials);
    s.saveInteger( "Interval Time", intervalTime );
    s.saveInteger( "BCL", BCL );
    s.saveDouble( "Stim Mag", stimMag );
    s.saveDouble( "Stim Length", stimLength );
    s.saveDouble( "LJP", LJP );
}

void AP_Clamp::Module::modify(void) {
    int APDr = mainWindow->APDRepolEdit->text().toInt();
    int mAPD = mainWindow->minAPDEdit->text().toInt();
    int sw = mainWindow->stimWindowEdit->text().toInt();
    int nt = mainWindow->numTrialEdit->text().toInt();
    int it = mainWindow->intervalTimeEdit->text().toInt();
    int b = mainWindow->BCLEdit->text().toInt();
    double sm = mainWindow->stimMagEdit->text().toDouble();
    double sl = mainWindow->stimLengthEdit->text().toDouble();
    double ljp = mainWindow->LJPEdit->text().toDouble();

    if( APDr == APDRepol && mAPD == minAPD && sw == stimWindow && nt == numTrials && it == intervalTime
        && b == BCL && sm == stimMag && sl == stimLength && ljp == LJP ) // If nothing has changed
        return ;

    // Set parameters
    setValue( 0, APDr );
    setValue( 1, mAPD );
    setValue( 2, sw );
    setValue( 3, nt );
    setValue( 4, it );
    setValue( 5, b );
    setValue( 6, sm );
    setValue( 7, sl );
    setValue( 8, ljp );

    ModifyEvent event( this, APDr, mAPD, sw, nt, it, b, sm, sl, ljp );
    RT::System::getInstance()->postEvent( &event );
}

void AP_Clamp::Module::refreshDisplay(void) {
    mainWindow->timeEdit->setText( QString::number(time) );
    mainWindow->voltageEdit->setText( QString::number(voltage) );
    mainWindow->beatNumEdit->setText( QString::number(beatNum) );
    mainWindow->APDEdit->setText( QString::number(APD) );
    
    if( executeMode == IDLE ) {
        if( mainWindow->startProtocolButton->isChecked() && !protocolOn ) {
            mainWindow->startProtocolButton->setChecked( false );
		  } else if( mainWindow->thresholdButton->isChecked() && !thresholdOn ) {
            mainWindow->thresholdButton->setChecked( false );
            mainWindow->stimMagEdit->setText( QString::number( stimMag ) );
            modify();
		  }
    }
    else if( executeMode == PROTOCOL ) {
        if( stepTracker != currentStep ) {
            stepTracker = currentStep;
            mainWindow->protocolEditorListBox->setCurrentRow( currentStep );
        }        
    }
}

AP_Clamp::Module::ModifyEvent::ModifyEvent(
                                           Module *m, int APDr, int mAPD, int sw, int nt, int it,
                                           int b, double sm, double sl, double ljp ) 
    : module( m ), APDRepolValue( APDr ), 
      minAPDValue( mAPD ), stimWindowValue( sw ),
      numTrialsValue( nt ), intervalTimeValue( it ), 
      BCLValue( b ), stimMagValue( sm ), 
      stimLengthValue( sl ), LJPValue( ljp ) { }

int AP_Clamp::Module::ModifyEvent::callback( void ) {
    module->APDRepol = APDRepolValue;
    module->minAPD = minAPDValue;
    module->numTrials = numTrialsValue;
    module->intervalTime = intervalTimeValue;
    module->BCL = BCLValue;
    module->BCLInt = module->BCL / module->period; // Update BCLInt when BCL is updated
    module->stimMag = stimMagValue;
    module->stimLength = stimLengthValue;
    module->LJP = LJPValue;
    
    return 0;
}

// Event handling
void AP_Clamp::Module::receiveEvent( const ::Event::Object *event ) {
    if( event->getName() == Event::RT_POSTPERIOD_EVENT ) {
        period = RT::System::getInstance()->getPeriod()*1e-6; // Grabs RTXI thread period and converts to ms (from ns)
        BCLInt = BCL / period;
        stimLengthInt = stimLength / period;
    }

    if( event->getName() == Event::START_RECORDING_EVENT ) recording = true;
    if( event->getName() == Event::STOP_RECORDING_EVENT ) recording = false;
}

void AP_Clamp::Module::receiveEventRT( const ::Event::Object *event ) {
    if( event->getName() == Event::RT_POSTPERIOD_EVENT ) {
        period = RT::System::getInstance()->getPeriod()*1e-6; // Grabs RTXI thread period and converts to ms (from ns)
        BCLInt = BCL / period;
        stimLengthInt = stimLength / period;
    }

    if( event->getName() == Event::START_RECORDING_EVENT ) recording = true;
    if( event->getName() == Event::STOP_RECORDING_EVENT ) recording = false;
}
