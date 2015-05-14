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
		QLabel* currentToScaleLabel;
		QLineEdit* currentToScaleEdit;
		QLabel* scalingPercentageLabel;
		QLineEdit* scalingPercentageEdit;
		QLabel* waitTimeLabel;
		QLineEdit* waitTimeEdit;
		QLabel* modelLabel;
		QComboBox* modelComboBox;
		QButtonGroup* buttonGroup;
		QGroupBox* buttonGroupBox;
		QPushButton* addStepButton;
		QPushButton* exitButton;

	protected:
		QVBoxLayout* AddStepDialogLayout;
		QHBoxLayout* layout1;
		QHBoxLayout* layout2;
		QHBoxLayout* layout3;
		QHBoxLayout* layout4;
		QHBoxLayout* layout5;
		QHBoxLayout* layout6;
		QHBoxLayout* buttonGroupLayout;
		QHBoxLayout* buttonGroupBoxLayout;
};

#endif // ADDSTEPDIALOG_H
