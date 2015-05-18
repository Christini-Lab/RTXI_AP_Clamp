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
 * Action Potental Clamp
 * 
 * APC_AddStepDialogUI.cpp, v1.0
 *
 * Author: Francis A. Ortega (2015)
 *
 * Notes in header
 *
 ***/

#include "APC_AddStepDialogUI.h"

#include <QtGui>
#include <iostream>

/*
 *  Constructs a AddStepDialog as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
AddStepDialog::AddStepDialog( QWidget* parent  ) : QDialog( parent ) {
	 QWidget::setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle( "Add Step to Protocol" );
    AddStepDialogLayout = new QVBoxLayout( this );

    stepComboBox = new QComboBox( this );
    stepComboBox->clear();
    stepComboBox->insertItem( 0, tr( "Static Pacing" ) );
    stepComboBox->insertItem( 1, tr( "Start: Record Vm" ) );
    stepComboBox->insertItem( 2, tr( "Stop: Record Vm" ) );
    stepComboBox->insertItem( 4, tr( "Record Average Vm" ) );
    stepComboBox->insertItem( 5, tr( "AP Clamp" ) );
    stepComboBox->insertItem( 6, tr( "Start: Data recorder" ) );
    stepComboBox->insertItem( 7, tr( "Stop: Data recorder" ) );
    stepComboBox->insertItem( 8, tr( "Wait" ) );

    AddStepDialogLayout->addWidget( stepComboBox );

    layout1 = new QHBoxLayout; 
    BCLLabel = new QLabel( "Basic Cycle Length (ms)", this );
    BCLLabel->setAlignment( Qt::AlignCenter );
    layout1->addWidget( BCLLabel );
    BCLEdit = new QLineEdit( "", this );
	 BCLEdit->setValidator( new QDoubleValidator(0, 1000, 2, BCLEdit) );
    layout1->addWidget( BCLEdit );
    AddStepDialogLayout->addLayout( layout1 );

    layout2 = new QHBoxLayout;
    numBeatsLabel = new QLabel( "Number of Beats", this );
    numBeatsLabel->setAlignment( Qt::AlignCenter );
    layout2->addWidget( numBeatsLabel );
    numBeatsEdit = new QLineEdit( "", this );
	 numBeatsEdit->setValidator( new QIntValidator(0, 10000, numBeatsEdit) );
    layout2->addWidget( numBeatsEdit );
    AddStepDialogLayout->addLayout( layout2 );

    layout4 = new QHBoxLayout;
    recordIdxLabel = new QLabel( "Recording Idx", this );
    recordIdxLabel->setAlignment( Qt::AlignCenter );
    layout4->addWidget( recordIdxLabel );
    recordIdxEdit = new QLineEdit( "", this );
	recordIdxEdit->setValidator( new QIntValidator(0, 99, recordIdxEdit) );
    layout4->addWidget( recordIdxEdit );
    AddStepDialogLayout->addLayout( layout4 );

    layout5 = new QHBoxLayout;  
    waitTimeLabel = new QLabel( "Wait Time (ms)", this );
    waitTimeLabel->setAlignment( Qt::AlignCenter );
    layout5->addWidget( waitTimeLabel );
    waitTimeEdit = new QLineEdit( "", this );
	 waitTimeEdit->setValidator( new QDoubleValidator(0, 10000, 2, waitTimeEdit) );
    layout5->addWidget( waitTimeEdit );
    AddStepDialogLayout->addLayout( layout5 );

    buttonGroup = new QButtonGroup( this );
	 buttonGroupBox = new QGroupBox( this );
    buttonGroupBoxLayout = new QHBoxLayout( buttonGroupBox );
    buttonGroupBoxLayout->setAlignment( Qt::AlignTop );

    addStepButton = new QPushButton( "Add Step", buttonGroupBox );
    buttonGroupBoxLayout->addWidget( addStepButton );
	 buttonGroup->addButton(addStepButton);

    exitButton = new QPushButton( "Exit", buttonGroupBox );
    buttonGroupBoxLayout->addWidget( exitButton );
	 buttonGroup->addButton(exitButton);
    AddStepDialogLayout->addWidget( buttonGroupBox );
}

/*
 *  Destroys the object and frees any allocated resources
 */
AddStepDialog::~AddStepDialog() { } // no need to delete child widgets, Qt does it all for us
