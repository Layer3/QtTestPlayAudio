#pragma once

#include <audioclient.h>
#include <audioendpoints.h>
#include <mmdeviceapi.h>
#include <devicetopology.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <propvarutil.h>

#include <QtWidgets/QMainWindow>
#include <QPushButton>
#include <QTextEdit>
#include <QListWidget>
#include <QFile>
#include <QFileDialog>
#include <QUrl>
#include <QAudioFormat>
#include <QAudioDecoder>
#include "sndfile.h"
#include "sndfile.hh"
#include "ui_QtTestWindowsAudioStream.h"

class QtTestWindowsAudioStream : public QMainWindow
{
	Q_OBJECT

public:
	QtTestWindowsAudioStream(QWidget *parent = Q_NULLPTR);

private slots:
	void DeviceSelected();
	void SelectSource();
	void ReadBuffer();
	void Play();

private:
	Ui::QtTestWindowsAudioStreamClass ui;

	IMMDevice* pDevice;
	IMMDeviceCollection* pDevices;
	SNDFILE* m_sourceFile;
	SF_INFO m_fileInfo;
	sf_count_t m_length = 0;
	QString fileName;
	bool gotFile = false;
	bool continuePlayback = true;
};
