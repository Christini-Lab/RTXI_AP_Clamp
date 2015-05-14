#ifndef AP_CLAMPUI_H
#define AP_CLAMPUI_H

#include <QtGui>

class AP_ClampUI : public QWidget {
	Q_OBJECT

	public:
		AP_ClampUI( QWidget* parent = 0 /*, const char* name = 0, WFlags fl = 0 */);
		~AP_ClampUI();

		QButtonGroup* protocolButtonGroup;
		QGroupBox* protocolGroup;
		QPushButton* staticPacingButton;
		QPushButton* resetButton;
		QPushButton* startProtocolButton;
		QPushButton* thresholdButton;
		QTabWidget* tabBox;
		QWidget* TabPage;
		QLabel* timeLabel;
		QLineEdit* timeEdit;
		QLabel* beatNumLabel;
		QLineEdit* beatNumEdit;
		QLabel* voltageLabel;
		QLineEdit* voltageEdit;
		QLabel* APDLabel;
		QLineEdit* APDEdit;
		QWidget* TabPage_2;
		QLabel* BCLLabel;
		QLineEdit* BCLEdit;
		QLabel* stimMagLabel;
		QLineEdit* stimMagEdit;
		QLabel* stimLengthLabel;
		QLineEdit* stimLengthEdit;
		QLabel* CmLabel;
		QLineEdit* CmEdit;
		QLabel* LJPLabel;
		QLineEdit* LJPEdit;
		QWidget* tab;
		QLabel* numTrialLabel;
		QLineEdit* numTrialEdit;
		QLabel* intervalTimeLabel;
		QLineEdit* intervalTimeEdit;
		QCheckBox* recordDataCheckBox;
		QGroupBox* modelGroup;
		QComboBox* modelComboBox;
		QWidget* TabPage_3;
		QPushButton* deleteStepButton;
		QPushButton* loadProtocolButton;
		QPushButton* saveProtocolButton;
		QPushButton* addStepButton;
		QPushButton* clearProtocolButton;
		QWidget* tab_2;
		QLabel* APDRepolLabel;
		QLineEdit* APDRepolEdit;
		QLabel* minAPDLabel;
		QLineEdit* minAPDEdit;
		QLabel* stimWindowLabel;
		QLineEdit* stimWindowEdit;
		QListWidget* protocolEditorListBox;

	protected:
		QVBoxLayout* AP_ClampUILayout;
		QGridLayout* protocolButtonGroupLayout;
		QGridLayout* protocolGroupLayout;
		QSpacerItem* spacer1b;
		QSpacerItem* spacer2b;
		QSpacerItem* spacer3b;
		QSpacerItem* spacer4b;
		QSpacerItem* spacer5b;
		QSpacerItem* spacer6b;
		QGridLayout* TabPageLayout;
		QGridLayout* TabPageLayout_2;
		QGridLayout* tabLayout;
		QHBoxLayout* recordDataLayout;
		QSpacerItem* spacer1a;
		QSpacerItem* spacer2a;
		QVBoxLayout* modelGroupLayout;
		QVBoxLayout* TabPageLayout_3;
		QGridLayout* pEditLayout1;
		QHBoxLayout* pEditLayout2;
		QSpacerItem* spacer1;
		QSpacerItem* spacer2;
		QGridLayout* tabLayout_2;
};

#endif // AP_CLAMPUI_H
