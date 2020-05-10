#include "QtTestWindowsAudioStream.h"
#include <thread>


const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

QtTestWindowsAudioStream::QtTestWindowsAudioStream(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	connect(ui.buttonDevice, SIGNAL(clicked()), this, SLOT(DeviceSelected()));
	connect(ui.selectSource, SIGNAL(clicked()), this, SLOT(SelectSource()));
	connect(ui.playButton, SIGNAL(clicked()), this, SLOT(Play()));

	ui.progressBar->setValue(0);

	CoInitialize(nullptr);

	IMMDeviceEnumerator* pEnumerator = nullptr;
	const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
	const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);

	HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
	pDevice = nullptr;
	EDataFlow dataFlow = EDataFlow::eRender;
	DWORD stateMask = DEVICE_STATE_ACTIVE | DEVICE_STATE_DISABLED | DEVICE_STATE_NOTPRESENT | DEVICE_STATE_UNPLUGGED;
	pDevices = nullptr;
	hr = pEnumerator->EnumAudioEndpoints(dataFlow, stateMask, &pDevices);
	UINT numDevices = 0;
	pDevices->GetCount(&numDevices);
	IPropertyStore* pPropertyStore = nullptr;
	PROPVARIANT propVar{ 0 };
	PropVariantInit(&propVar);
	WCHAR name[256];

	for (UINT i = 0; i < numDevices; ++i)
	{
		pDevices->Item(i, &pDevice);
		pDevice->OpenPropertyStore(STGM_READ, &pPropertyStore);
		pPropertyStore->GetValue(PKEY_Device_FriendlyName, &propVar);

		hr = PropVariantToString(propVar, name, ARRAYSIZE(name));

		QString deviceName = QString::number(i);
		deviceName.append("  ");
		deviceName.append(QString::fromWCharArray(name));

		ui.listWidget->addItem(deviceName);
	}

	PropVariantClear(&propVar);
	CoUninitialize();
}

void QtTestWindowsAudioStream::DeviceSelected()
{
	ui.buttonDevice->setText("wow");
}

void QtTestWindowsAudioStream::SelectSource()
{
	fileName = QFileDialog::getOpenFileName(this, tr("Open Sound File"), "D:/Nuendo/Komposition/BeatyBeat/Audio", tr("Wave Files (*.wav)"));
	ui.lineEdit->setText(fileName);
	gotFile = true;
}

void QtTestWindowsAudioStream::Play()
{
	if (!gotFile)
	{
		return;
	}

	QByteArray ba = fileName.toLocal8Bit();
	const char *c_str = ba.data();

	m_sourceFile = sf_open(c_str, SFM_READ, &m_fileInfo);

	m_length = m_fileInfo.frames;

	QList<QListWidgetItem*> selection = ui.listWidget->selectedItems();

	QString deviceName = selection[0]->text();
	QStringList stringList = deviceName.split(" ");
	QString qDeviceIndex = stringList.at(0);
	UINT deviceIndex = qDeviceIndex.toUInt();
	
	IMMDeviceEnumerator *pEnumerator = NULL;
	EDataFlow dataFlow = EDataFlow::eRender;
	DWORD stateMask = DEVICE_STATE_ACTIVE | DEVICE_STATE_DISABLED | DEVICE_STATE_NOTPRESENT | DEVICE_STATE_UNPLUGGED;
	pDevice = nullptr;
	pDevices = nullptr;

	CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void**)&pEnumerator);

	HRESULT hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);

	ReadBuffer();
}

void QtTestWindowsAudioStream::ReadBuffer()
{
	REFERENCE_TIME hnsRequestedDuration = 10000000;
	IAudioClient *pAudioClient = nullptr;
	WAVEFORMATEX *pwfx = nullptr;
	UINT32 bufferFrameCount;
	IAudioRenderClient *pRenderClient = nullptr;
	BYTE *pData;
	DWORD flags = 0;
	UINT32 numFramesAvailable;
	UINT32 numFramesPadding;

	HRESULT hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, (void**)&pAudioClient);

	pAudioClient->GetMixFormat(&pwfx);

	pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		0,
		hnsRequestedDuration,
		0,
		pwfx,
		nullptr);

	pAudioClient->GetBufferSize(&bufferFrameCount);

	pAudioClient->GetService(
		IID_IAudioRenderClient,
		(void**)&pRenderClient);

	pRenderClient->GetBuffer(bufferFrameCount, &pData);
	WAVEFORMATEXTENSIBLE* pwfxEx = nullptr;

	if (pwfx->cbSize != 0)
	{
		pwfxEx = (WAVEFORMATEXTENSIBLE*)pwfx;
		// 00000003-0000-0010-8000-00AA00389B71 == WAVE_FORMAT_FLOAT
	}

// This would be the easy way to play arbitrary numbers of file channels into different numbers of out channels
// The same would have to be done for the continuous play loop, with an additional escape for the files end.
//
// we want to support writing up to 2 channels for now
// pData with pwfxEx.subFormat float has 32bit/sample interleaved data
// 
// 	if (pwfx->cbSize != 0)
// 	{
// 		pwfxEx = (WAVEFORMATEXTENSIBLE*)pwfx;
// 		// 00000003-0000-0010-8000-00AA00389B71 == WAVE_FORMAT_FLOAT
// 	}
// 
// 	// we want to support writing up to 2 channels for now
// 	// expecting the output to have at least two channels
// 	// pData with pwfxEx.subFormat float has 32bit/sample interleaved data
// 
// 	float* pDataFloat = (float*)pData;
// 
// 	int numChannelsOut = pwfx->nChannels;
// 	int numChannelsFile = m_fileInfo.channels;
// 	int bufferLength = bufferFrameCount;
// 
// 	if (numChannelsFile == 1)
// 	{
// 		for (int i = 0; i < bufferLength; ++i)
// 		{
// 			sf_read_float(m_sourceFile, &pDataFloat[i * numChannelsOut], 1);
// 
// 			for (int j = 1; j < numChannelsOut; ++j)
// 			{
// 				pDataFloat[(i * numChannelsOut) + j] = 0.0f;
// 			}
// 		}
// 	}
// 	else if (numChannelsFile == 2)
// 	{
// 		if (numChannelsOut == 2)
// 		{
// 			for (int i = 0; i < bufferLength; ++i)
// 			{
// 				sf_read_float(m_sourceFile, &pDataFloat[i * numChannelsOut], 2);
// 			}
// 		}
// 		else // Assuming output has more than two channels
// 		{
// 			for (int i = 0; i < bufferLength; ++i)
// 			{
// 				sf_read_float(m_sourceFile, &pDataFloat[i * numChannelsOut], 2);
// 
// 				for (int j = 2; j < numChannelsOut; ++j)
// 				{
// 					pDataFloat[(i * numChannelsOut) + j] = 0.0f;
// 				}
// 			}
// 		}
// 	}
// 	else
// 	{
// 		if (numChannelsOut == 2)
// 		{
// 			for (int i = 0; i < bufferLength; ++i)
// 			{
// 				sf_read_float(m_sourceFile, &pDataFloat[i * numChannelsOut], 2);
// 				sf_seek(m_sourceFile, numChannelsFile - 2, SEEK_CUR);
// 			}
// 		}
// 		else // Assuming output has more than two channels
// 		{
// 			for (int i = 0; i < bufferLength; ++i)
// 			{
// 				sf_read_float(m_sourceFile, &pDataFloat[i * numChannelsOut], 2);
// 				sf_seek(m_sourceFile, numChannelsFile - 2, SEEK_CUR);
// 
// 				for (int j = 2; j < numChannelsOut; ++j)
// 				{
// 					pDataFloat[(i * numChannelsOut) + j] = 0.0f;
// 				}
// 			}
// 		}
// 	}
// 	

	sf_count_t alreadyRead = 0;
	sf_read_raw(m_sourceFile, pData, bufferFrameCount * (pwfx->wBitsPerSample / 8));
	
	alreadyRead += (bufferFrameCount * (pwfx->wBitsPerSample / 8)) / 8;
	ui.progressBar->setValue(static_cast<int>((static_cast<float>(alreadyRead) / static_cast<float>(m_length)) * 100.0f));

	hr = pRenderClient->ReleaseBuffer(bufferFrameCount, flags);

	hr = pAudioClient->Start();  // Start playing.
	
	REFERENCE_TIME hnsActualDuration;
	hnsActualDuration = (double)REFTIMES_PER_SEC *
		bufferFrameCount / pwfx->nSamplesPerSec;

	// Each loop fills about half of the shared buffer.
	while (flags != AUDCLNT_BUFFERFLAGS_SILENT)
	{
		// Sleep for half the buffer duration.
		Sleep((DWORD)(hnsActualDuration / REFTIMES_PER_MILLISEC / 2));

		// See how much buffer space is available.
		hr = pAudioClient->GetCurrentPadding(&numFramesPadding);

		numFramesAvailable = bufferFrameCount - numFramesPadding;

		// Grab all the available space in the shared buffer.
		hr = pRenderClient->GetBuffer(numFramesAvailable, &pData);

		sf_count_t numBytesToRead = bufferFrameCount * (pwfx->wBitsPerSample / 8);
		if (sf_read_raw(m_sourceFile, pData, numBytesToRead) != numBytesToRead)
		{
			flags = AUDCLNT_BUFFERFLAGS_SILENT;
		}
		else
		{
			alreadyRead += numBytesToRead / 8;
			ui.progressBar->setValue(static_cast<int>((static_cast<float>(alreadyRead) / static_cast<float>(m_length)) * 100.0f));
		}

		hr = pRenderClient->ReleaseBuffer(numFramesAvailable, flags);
	}

	ui.progressBar->setValue(100);

	// Wait for last data in buffer to play before stopping.
	Sleep((DWORD)(hnsActualDuration / REFTIMES_PER_MILLISEC / 2));

	hr = pAudioClient->Stop();  // Stop playing.
	
	CoUninitialize();
}

// Wav file headers optionally use a JUNK chunk that has to be skipped, using libsndfile for now
/*
QFile m_WAVFile;
m_WAVFile.setFileName(wavFile);
m_WAVFile.open(QIODevice::ReadWrite);

char* strm;
QVector<double> m_DATA;

qDebug()<<m_WAVFile.read(4);//RIFF
// m_WAVHeader.RIFF = m_WAVFile.read(4).data();

m_WAVFile.read(strm,4);//chunk size
qDebug()<<qFromLittleEndian<quint32>((uchar*)strm) ;

m_WAVFile.read(strm,4);//format
qDebug()<<strm;

m_WAVFile.read(strm,4);//subchunk1 id
qDebug()<<strm;

m_WAVFile.read(strm,4);//subchunk1 size
qDebug()<<qFromLittleEndian<quint32>((uchar*)strm) ;

m_WAVFile.read(strm,2);//audio format
qDebug()<<qFromLittleEndian<quint32>((uchar*)strm) ;

m_WAVFile.read(strm,2);//NumChannels
qDebug()<<qFromLittleEndian<quint32>((uchar*)strm) ;

m_WAVFile.read(strm,4);//Sample rate
qDebug()<<qFromLittleEndian<quint32>((uchar*)strm) ;

m_WAVFile.read(strm,4);//Byte rate
qDebug()<<qFromLittleEndian<quint32>((uchar*)strm) ;

m_WAVFile.read(strm,2);//Block Allign
qDebug()<<qFromLittleEndian<quint32>((uchar*)strm) ;

m_WAVFile.read(strm,2);//BPS
qDebug()<<qFromLittleEndian<quint32>((uchar*)strm) ;

m_WAVFile.read(strm,4);//subchunk2 id
qDebug()<<strm;

m_WAVFile.read(strm,4);//subchunk2 size
qDebug()<<qFromLittleEndian<quint32>((uchar*)strm) ;

while(!m_WAVFile.atEnd())
{
m_WAVFile.read(strm,2);
if(qFromLittleEndian<short>((uchar*)strm))
m_DATA << (qFromLittleEndian<short>((uchar*)strm));
}
*/