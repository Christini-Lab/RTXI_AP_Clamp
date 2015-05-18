#ifndef ADDSTEPDIALOG_H
#define ADDSTEPDIALOG_H

#include <QtGui>

class AddStepDialog : public QDialog {
	Q_OBJECT

	public:
		AddStepDialog( QWidget* parent = 0 );
		~AddStepDialog();

		QComboBox* stepComboBox;
		QLabel* BCLLabel;
		QLineEdit* BCLEdit;
		QLabel* numBeatsLabel;
		QLineEdit* numBeatsEdit;
        QLabel* recordIdxLabel;
		QLineEdit* recordIdxEdit;
		QLabel* waitTimeLabel;
		QLineEdit* waitTimeEdit;
		QButtonGroup* buttonGroup;
		QGroupBox* buttonGroupBox;
		QPushButton* addStepButton;
		QPushButton* exitButton;

	protected:
		QVBoxLayout* AddStepDialogLayout;
		QHBoxLayout* layout1;
		QHBoxLayout* layout2;
		QHBoxLayout* layout4;
		QHBoxLayout* layout5;
		QHBoxLayout* buttonGroupLayout;
		QHBoxLayout* buttonGroupBoxLayout;
};

#endif // ADDSTEPDIALOG_H
