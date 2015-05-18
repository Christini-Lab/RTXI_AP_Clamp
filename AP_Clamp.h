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
 * AP_Clamp.h, v1.0
 *
 * Author: Francis A. Ortega (2015)
 *
 ***/

#ifndef AP_CLAMP_H
#define AP_CLAMP_H

#include "include/APC_Protocol.h" // Protocol Library
#include "include/APC_MainWindowUI.h" // Main Window GUI

#include <vector>

#include <rt.h>
#include <settings.h>
#include <workspace.h>
#include <event.h>
#include <plugin.h>

#include <QtGui>

namespace AP_Clamp {
    class Module: public QWidget, public RT::Thread, public Plugin::Object, 
                  public Workspace::Instance, public Event::Handler, public Event::RTHandler {
    
        Q_OBJECT // macro needed if slots are implemented
    
    public:
    
        Module( void );
        ~Module(void);        
        void execute( void );
        void receiveEvent( const ::Event::Object * );
        void receiveEventRT( const ::Event::Object * );
                         
    public slots:
        void modify( void ); // Updates parameters
        void reset( void ); // Calls update(RESET)
        void addStep( void ); // Adds a protocol set
        void deleteStep( void ); // Deletes a protocol step
        void saveProtocol( void ); // Saves protocol into a xml file
        void loadProtocol( void ); // Loads xml protocol file
        void clearProtocol( void ); // Clears protocol
        void toggleProtocol( void ); // Called when protocol button is toggled
        void togglePace( void ); // Called when pace button is toggled
        void toggleThreshold( void ); // Called when threshold button is toggled
        void refreshDisplay( void );

    private:
        // GUI
        AP_ClampUI *mainWindow;
    
        // Flags
        enum executeMode_t { IDLE, THRESHOLD, PACE, PROTOCOL } executeMode;
        enum protocolMode_t { STEPINIT, EXEC, END } protocolMode;
        bool recording; // True if data recording is recording
        bool voltageClamp;
        bool vmRecording;
        bool stepInitDone;
        QString loadedFile;
        bool protocolOn;
        bool thresholdOn;
        bool paceOn;
        int stepTracker;

        // States
        double time; // Time (ms)
        double voltage; // Membrane voltage
        double beatNum; // Beat number
        double APD; // Action potential duration

        // Parameters
        int APDRepol; // APD Repolarization percentage
        int minAPD; // Minimum duration of depolarization that counts as action potential
        int stimWindow; // Window of time after stimulus ignored by APD calculation
        int numTrials; // Number of trials to be run
        int intervalTime; // Time between trials
        int BCL; // Basic cycle length
        double stimMag; // Stimulation magnitude (nA)
        double stimLength; // Stimulation length (ms)
        double LJP; // Liquid junction potential (mV);

        // Protocol Variables
        Protocol *protocol;
        std::vector<ProtocolStepPtr> *protocolContainer; // Protocol container
        ProtocolStepPtr stepPtr; // Pointer to protocol step in protocol ctonainer
        ProtocolStep::stepType_t stepType; // Current step type for current step
        double outputCurrent; // Current output
        int currentStep; // Current step in protocol
        int stepTime; // Time tracker for step
        int stepEndTime; // Time end tracker for step
        int cycleStartTime; // Time tracker for BCL
        double period; // Period based on RTXI thread rate
        int BCLInt; // BCL / period (unitless)
        int pBCLInt; // BCL for protocol
        int stimLengthInt; // stimLength / period (unitless)

        // APD Calculation    
        double upstrokeThreshold; // Upstroke threshold for start of AP
        double downstrokeThreshold; // Downstroke threshold for end of AP
        enum APDMode_t { START, PEAK, DOWN, DONE } APDMode;
        double APStart; // Time the action potential starts   
        double peakVoltage; // Peak of action potential
        double peakTime; // Time of action potential peak
        double Vrest;

        // Threshold Variables
        bool actionPotential;
        bool thresholdStimulate;
        bool backToBaseline;
        double stimulusLevel;
        double startingStimulusLevel;
        double responseTime, initialStimulusTime;
        double diastolicThreshold;
        double responseDuration;
        double peakVoltageT;

        // AP Clamp Variables
        std::vector< std::vector<double> > voltageData;
        std::vector<double> *vmRecordData;
        std::vector<double> *avgRecordData;
        std::vector<double> *apClampData;
        int recordingIndex;
        int vmRecordCnt, avgCnt, apClampCnt;
   
        // Module functions
        void createGUI(); // Construct GUI
        void initialize(); // Initialization
        void rebuildListBox( void ); // Builds protocol list box
        void calculateAPD( int ); // Calulates action potential duration

        friend class ModifyEvent;
        friend class ToggleProtocolEvent;
        friend class TogglePaceEvent;
        friend class ToggleThresholdEvent;
    
        class ModifyEvent : public RT::Event {
        public:
            ModifyEvent( Module *, int, int, int, int, int, int,
                         double, double, double );
            ~ModifyEvent( void ) { };

            int callback( void );

        private:
            Module *module;
            int APDRepolValue;
            int minAPDValue;
            int stimWindowValue;
            int numTrialsValue;
            int intervalTimeValue;
            int BCLValue;
            double stimMagValue;
            double stimLengthValue;
            double LJPValue;

        }; // class ModifyEvent
        
    protected:
        void doLoad( const Settings::Object::State & );
        void doSave( Settings::Object::State & ) const;
                                          
    private slots:

    }; // Class Module
    
}; // namespace AP_Clamp

#endif // AP_CLAMP_H
