/**
* File Name: "VoiceRecognition.cpp"
* Description:
*     - This program is Voice recognition according to drone commands.
*  (Drone commands are ascending, descending, forward, backward, letf and right.)
*
* Programmed by Sang-Hun Han (June 5, 2018),
* Last updated: Version 2.0, June 26, 2019 (by Sang-Hun Han).
*
* ============================================================================================
*  Version Control (Explain updates in detail)
* ============================================================================================
*  Name            YYYY/MM/DD    Version  Remarks
*  Sang-Hun Han  2018/05/08		 v1.0	 Voice Recording in wav file.
*  Sang-Hun Han  2018/05/22		 v2.0	 Voice activity detection.
*  Sang-Hun Han  2018/05/29		 v3.0	 Voice recognition in real time.
*  Sang-Hun Han  2018/06/04		 v4.0	 Build a big database to improve recognition rates.
*  Sang-Hun Han  2018/06/04		 v4.0	 Implementation of voice verification module.
* ============================================================================================
*/

#pragma comment(lib, "winmm.lib")
#include<iostream>
#include<stdio.h>
#include<conio.h>
#include <windows.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

using namespace std;

#define WAVE_FORMAT_PCM 1
#define MONO 1
#define STEREO 2
#define SAMPLE_RATE 44100
#define BIT_PER_SAMPLE 16
#define BIT 8
#define BUFFER_COUNT 2
#define BUFFER_SIZE 1024*8

/*VAD variable*/

int speechCnt = 0;
int speechFlag = 0;



/*Wave Header structure*/

typedef struct
{
	char riff_id[4]; // "RIFF format" 
	unsigned int riff_size; // file length - 8 in bytes (���ڿ� �Ʒ����� ������ byte ������ ũ��)
	char wave_id[4]; // "WAVE" 
	char format_id[4]; // "fmt " 
	unsigned int format_size; // format size (canonical form= 16L, L: long integer)
	unsigned short format_type; // 1 for PCM (almost wavfile : pcm)
	unsigned short ChannelNo; // 1 for mono, 2 for stereo (���������� ä�� ��)
	unsigned int SamplesPerSec; // 44.1 KHz (���ø� ���ļ�)
	unsigned int AvgBytesPerSec; // sample rate * block align (1�ʴ� ��ȯ�� ���Ǵ� ��� �����ͷ�)
	unsigned short BytesPerSample; // channels * bits/sample/8 (���ä���� ���ô� bit ��)
	unsigned short BitsPerSample; // 8 for 8 bits, 16 for 16bits (���� �Ѱ��� bit)
	char data_id[4]; // "data" 
	unsigned int data_size; // length of the data block= (riff_size + 8 - 44)/BytesPerSample 
}WAVE_Header;

WAVE_Header Header;
WAVEFORMATEX WaveFormat; //Wave foramt ����ü

FILE *fp;
FILE *fp1;
FILE *fp2;

void InitHeader();
void RecognitionFor1Frame(WAVEHDR* wave);
void PrintVoiceResult(char* buffer, char* src, char* result, char* tok, int data_size);


/**
*  Function Name: WaveInProc
*  Input arguments (condition):
*    1) HWAVEIN hwi : Handle to the waveform-audio device associated with the callback function.
*    2) UINT uMsg : Waveform-audio input message.
*	 3) DWORD dwInstance : User instance data specified with waveInOpen.
*    4) DWORD dwParam1 : Pointer to a WAVEHDR structure that identifies the buffer containing the data. (Message parameter)
*    5) DWORD dwParam2 : Must be 0. (Message parameter)
*  Processing in function (in pseudo code style):
*    1) initialization: WAVEHDR *pHdr <- NULL;
*    2) switch(uMsg)
		{
			case WIM_CLOSE : break;
			case WIM_DATA : RecordingVoice; waveInAddbuffer(Next Buffer)
			case WIM__OPEN : break;
		}
*  Function Return:
*    1) data type: This function does not return a value.
*/


void CALLBACK WaveInProc(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	WAVEHDR *pHdr = NULL; //���̺��� ����� ���ۿ� ���� ����

	switch (uMsg)
	{
	case WIM_CLOSE: // Recording close
		break;
	case WIM_DATA: //���� ���۰� �������� ����̹��� ���� ���α׷����� �޼��� ����
		pHdr = (WAVEHDR *)dwParam1; //�����͸� �����ϴ� ���۸� �ĺ��ϴ� �����Ϳ� ���� ������ �ʱ�ȭ 

		RecognitionFor1Frame(pHdr);
		if (waveInAddBuffer(hwi, pHdr->lpNext, sizeof(WAVEHDR))) //���� ���۸� ����Ѵٰ� ����̹����� ����
		{
			printf("error\n");
			exit(1);
		}

		break;
	case WIM_OPEN: //Recording start
		break;
	default:
		break;
	}
}


/**
*  Function Name: main
*  Input arguments (condition):
*	This fuction is main, so does not have parameter.
*  Processing in function (in pseudo code style):
*    1) initialization: 
*				WAVEHDR WaveInHeader; (Buffer option structure)
				HWAVEIN hWaveIn; (handler)
				unsigned char *pWaveInBuffer; (Buffer)
				WaveFormat initialization
*    2) waveInOpen - waveInPrepareHeader - waveInAddBuffer - waveInStart - waveInUnprepareHeader - waveInClose
*  Function Return:
*    1) data type: This function does not return a value.
*/

int main()
{
	fopen_s(&fp, "VAD.wav", "wb");

	InitHeader(); //Wave file header �ʱ�ȭ
	
	WAVEHDR WaveInHeader[BUFFER_COUNT]; //���ۿ� �޸𸮰����� �������ִ� ����ü (���ۿ� ���� ������ ����)
	HWAVEIN hWaveIn;  // Recording handler
	unsigned char *pWaveInBuffer[BUFFER_COUNT]; //Input buffer

	WaveFormat.wFormatTag = WAVE_FORMAT_PCM; //PCM
	WaveFormat.wBitsPerSample = BIT_PER_SAMPLE; //Sampling ���� : 16 bit 
	WaveFormat.nChannels = MONO; // ������� : MONO 
	WaveFormat.nSamplesPerSec = SAMPLE_RATE;  //Sampling �ֱ� : 44100 Hz
	WaveFormat.nBlockAlign = WaveFormat.nChannels * BIT_PER_SAMPLE / BIT; 
	//(ä�μ� * ���ô� ��Ʈ(16bit)) / 8(byte) �� ���ô� �� ����Ʈ���� �޸�ũ�⸦ ���
	WaveFormat.nAvgBytesPerSec = WaveFormat.nBlockAlign * SAMPLE_RATE; // 1�ʴ� ��ȯ�� ���Ǵ� ��� �����ͷ��� ���
	WaveFormat.cbSize = 0; //PCM�� ��� ����

	/*�Է¹��� �޸� �Ҵ�*/
	for (int i = 0; i < BUFFER_COUNT; i++)
	{
		pWaveInBuffer[i] = (unsigned char*)malloc(BUFFER_SIZE * WaveFormat.nBlockAlign); //���� �����Ͱ� �������� ����
		ZeroMemory(pWaveInBuffer[i], BUFFER_SIZE * WaveFormat.nBlockAlign);
	}

	printf("Input buffer memory allocation complete\n");

	/*���� ����̹��� �������α׷��� �����͸� �ְ���� �� �ʿ��� ���� ����(�Է� ����)*/
	for (int i = 0; i < BUFFER_COUNT; i++)
	{
		WaveInHeader[i].lpData = (LPSTR)pWaveInBuffer[i]; //Current buffer : ���� �� �� ��� �� �޸� ������ ���� �ּ�
		WaveInHeader[i].dwBufferLength = BUFFER_SIZE * WaveFormat.nBlockAlign;// lpData�� ��õ� �޸��� ũ��
		WaveInHeader[i].dwBytesRecorded = 0;
		WaveInHeader[i].dwUser = 0;
		WaveInHeader[i].dwFlags = 0;
		WaveInHeader[i].dwLoops = 0;
		WaveInHeader[i].lpNext = &WaveInHeader[(i + 1) % BUFFER_COUNT]; //next buffer (ring buffer)
	}

	printf("WaveInheader information settings complete\n");

	/*������ġ�� ���� �ڵ��� ���, �ݹ��Լ��� ����Ѵ�*/

	if (waveInOpen(&hWaveIn, WAVE_MAPPER, &WaveFormat, (DWORD_PTR)WaveInProc, NULL, CALLBACK_FUNCTION))
	{
		printf("Failed to open waveform input device.\n");
		//��ȯ ���� ������� �����޼��� ���
		exit(0);
	}

	printf("Recorder handle & Callback function registration complete\n");


	/*���� �� �غ� �Ǿ����� ����̹����� �˸���*/
	for (int i = 0; i < BUFFER_COUNT; i++)
	{
		if (waveInPrepareHeader(hWaveIn, &WaveInHeader[i], sizeof(WAVEHDR)))
		{
			printf("Failed to prepareHeader.\n");
			//��ȯ ���� ������� �����޼��� ���
			exit(0);
		}
	}

	printf("Recording preparing\n");

	/*����� �Է¹��۸� ����̹����� �����Ѵ�*/

	if (waveInAddBuffer(hWaveIn, &WaveInHeader[0], sizeof(WAVEHDR)))
	{
		printf("Failed to read block from device.\n");
		//��ȯ ���� ������� �����޼��� ���
		exit(0);
	}
	printf("Delivery the Input buffer to sound driver\n");


	printf("Recording preparation complete\n\n");


	/*������ġ�� �̿��Ͽ� ������ �����Ѵ�*/
	if (waveInStart(hWaveIn))
	{
		printf("Failed to start Recording.\n");
		//��ȯ ���� ������� �����޼��� ���
		exit(0);
	}

	printf("[Please input voice] : Press any key if you want to exit.\n");

	while (!kbhit());

	/*���� ���۸� ���� : �޸� ����*/

	for (int i = 0; i < BUFFER_COUNT; i++)
	{
		waveInUnprepareHeader(hWaveIn, &WaveInHeader[i], sizeof(WAVEHDR));
	}
	waveInClose(hWaveIn); // waveInOpen�� ���ؼ� ���Ǵ� ��ġ�� ����

	printf("Closing Sound Recorder\n");

	/*���� �޸� ��ȯ*/

	for (int i = 0; i < BUFFER_COUNT; i++)
	{
		free(pWaveInBuffer[i]);
	}
	printf("Buffer memory de-allocation\n\n");

	return 0;
}



/**
*  Function Name:
*  Input arguments (condition):
*    1) WAVEHDR* wave : Contents about current buffer
*
*  Processing in function (in pseudo code style):
*    1) initialization:
*		int *sample_energy = (int*)malloc(sizeof(int) * wave->dwBufferLength);
*		int frame_total_energy = 0;
*		char *buffer;
*		char *src;
*		char *tok;
*		char *result;
*		int data_size = 0;
*    2) 	for (int i = 0; i < wave->dwBufferLength; i++)
*			{
*				sample_energy[i] = (wave->lpData[i])*(wave->lpData[i]);
*				frame_total_energy += sample_energy[i];
*			} //Sample energy for 1 frame.
*
*    3)		if (frame_total_energy > threadshold)
*			{
*				This is Speech data. 
*			}
*			else
*			{
*				This is Non Speeh data.
*				if (speechCnt % 5 == 0 && speechFlag == 1)
*				{
*					<Real time Voice recognition>
*					...
*					PrintVoiceResult(buffer, src, result, tok, data_size);
*				}
*			}
*
*  Function Return:
*    1) data type: This function does not return a value.
*/

void RecognitionFor1Frame(WAVEHDR* wave)
{
	int *sample_energy = (int*)malloc(sizeof(int) * wave->dwBufferLength);
	int frame_total_energy = 0;
	char *buffer;
	char *src;
	char *tok;
	char *result;
	int data_size = 0;

	for (int i = 0; i < wave->dwBufferLength; i++)
	{
		sample_energy[i] = (wave->lpData[i])*(wave->lpData[i]);
		frame_total_energy += sample_energy[i];
	}
	frame_total_energy = abs(frame_total_energy);

	if (frame_total_energy > 23000000) //13000000 Speech threshold : ���������� ����
	{
		Header.riff_size += wave->dwBufferLength;
		Header.data_size += wave->dwBufferLength;
		
		speechFlag = 1;

		fseek(fp, 0, SEEK_SET); //wav file pointer�� �ʱ�� �̵���Ŵ
		fwrite(&Header, sizeof(Header), 1, fp); //wav file�� header�� ����
		fseek(fp, 0, SEEK_END); //file pointer�� ������ ������ �̵���Ŵ
		printf("speech data\n");
		
		fwrite(wave->lpData, wave->dwBufferLength, 1, fp); //���� ���ۿ� ����� ���� ���Ͽ� ����.
	}
	else
	{
		speechCnt++;
		printf("non speech data\n");

		if (speechCnt % 5 == 0 && speechFlag == 1)
		{
			fclose(fp);
			if (((Header.data_size / 1000) > 30))
			{
				system("Hcopy -C config -S test_list.scp");
				printf("Feature extraction completed\n");
				system("HVite -C config1 -H hmm5/macros -H hmm5/hmmdefs -S test.scp -i recout.mlf -w wdnet -n 6 6 -p -8 -s 16 dict monophones0");
				printf("Complete Speech Recognition\n");

				
				fopen_s(&fp1, "C:/Users/User/source/repos/VoiceRecognition/VoiceRecording/recout.mlf", "rb");
				if (fp1 == NULL)
				{
					printf("File Open Failed\n");
					exit(0);
				}
				fseek(fp1, 0L, SEEK_END);
				data_size = ftell(fp1);
				buffer = (char*)malloc(sizeof(char) * data_size);
				tok = (char*)malloc(sizeof(char) * data_size);
				src = (char*)malloc(sizeof(char) * data_size);
				result = (char*)malloc(sizeof(char) * data_size);
				fseek(fp1, 0, SEEK_SET);
				fread(buffer, data_size, sizeof(char), fp1);

				PrintVoiceResult(buffer, src, result, tok, data_size);

				fclose(fp1);
			}
			fopen_s(&fp, "VAD.wav", "wb");
			if (fp == NULL)
			{
				printf("File Open Failed\n");
				exit(0);
			}
			speechFlag = 0;

			speechCnt = 0;

			Header.riff_size = 36;
			Header.data_size = 0;
		}
	}
}


/**
*  Function Name: InitHeader
*  Input arguments (condition) : This function does not have parameter.
*  Processing in function (in pseudo code style):
*    1) initialization:
*			memcpy(&Header.riff_id, "RIFF", 4);
*			Header.riff_size = 36;
*			memcpy(&Header.wave_id, "WAVE", 4);
*			memcpy(&Header.format_id, "fmt ", 4);
*			Header.format_size = 16;
*			Header.format_type = WAVE_FORMAT_PCM;
*			Header.ChannelNo = MONO;
*			Header.SamplesPerSec = SAMPLE_RATE;
*			Header.AvgBytesPerSec = SAMPLE_RATE * 2;
*			Header.BytesPerSample = MONO * BIT_PER_SAMPLE / BIT;
*			Header.BitsPerSample = BIT_PER_SAMPLE;
*			memcpy(&Header.data_id, "data", 4);
*			Header.data_size = 0;
*  Function Return:
*    1) data type: This function does not return a value.
*/


void InitHeader()
{
	memcpy(&Header.riff_id, "RIFF", 4); //RIFF format 
	Header.riff_size = 36; //���� ��ü �������, ���� 'Riff ID' �� �ڱ��ڽ��� 'Chunk Size' �� ������ ��
	memcpy(&Header.wave_id, "WAVE", 4); //file form
	memcpy(&Header.format_id, "fmt ", 4); //fmt
	Header.format_size = 16; //������
	Header.format_type = WAVE_FORMAT_PCM; //������
	Header.ChannelNo = MONO; //ä�� ���
	Header.SamplesPerSec = SAMPLE_RATE; //44100 hz
	Header.AvgBytesPerSec = SAMPLE_RATE * 2; //sample rate * channelNo
	Header.BytesPerSample = MONO * BIT_PER_SAMPLE / BIT; //Sample Frame �� ũ��
	Header.BitsPerSample = BIT_PER_SAMPLE; //Sample �� �� �� bit���� ���
	memcpy(&Header.data_id, "data", 4); //������
	Header.data_size = 0; //������� ������ ũ��
}


/**
*  Function Name:
*  Input arguments (condition):
*    1) char* buffer
*    2) char* src
*	 3) char* result
*	 4) char* tok
*	 5) int data_size
*  Processing in function (in pseudo code style):
*    1) initialization:	
*			char *temp = (char*)malloc(sizeof(char) * data_size);
*			char *temp2 = (char*)malloc(sizeof(char) * data_size);
*			int fst, snd = 0;
*    2)	N-Best result (first result - second result -> similar) -> miss recognition
*	if (fst - snd > 20)
*	{
*		print(sidong, jungji, sangseung, hagang, oenjjog, oleunjjog);
*	}
*	else
*	{
*		print(incorrect commnad);
*	}
*  Function Return:
*    1) data type: This function does not return value.
*/


void PrintVoiceResult(char* buffer, char* src, char* result, char* tok, int data_size)
{
	fopen_s(&fp2, "result.txt", "wb");

	char *temp = (char*)malloc(sizeof(char) * data_size);
	char *temp2 = (char*)malloc(sizeof(char) * data_size);
	int fst, snd = 0;
	strcpy(temp, buffer);
	strcpy(temp2, buffer);
	src = "-";
	result = strstr(temp, src);
	tok = strtok(result, "/");
	fst = atoi(tok);
	result = strstr(temp2, src);
	src = "/";
	result = strstr(result, src);
	src = "-";
	result = strstr(result, src);
	tok = strtok(result, "/");
	snd = atoi(tok);

	tok = strtok(buffer, "-");
	src = "sidong";
	result = strstr(buffer, src);

	if (fst - snd > 20)
	{
		if (result != NULL)
		{
			tok = strtok(result, " ");
			if (strcmp(tok, "sidong") == 0)
			{
				printf("Result : �õ�\n");
				fprintf(fp2, "1");
			}
		}
		else
		{
			tok = strtok(buffer, "-");
			src = "jungji";
			result = strstr(buffer, src);
			if (result != NULL)
			{
				tok = strtok(result, " ");
				if (strcmp(tok, "jungji") == 0)
				{
					printf("Result : ����\n");
					fprintf(fp2, "2");

				}
			}
			else
			{
				tok = strtok(buffer, "-");
				src = "sangseung";
				result = strstr(buffer, src);
				if (result != NULL)
				{
					tok = strtok(result, " ");
					if (strcmp(tok, "sangseung") == 0)
					{
						printf("Result : ���\n");
						fprintf(fp2, "3");

					}
				}
				else
				{
					tok = strtok(buffer, "-");
					src = "hagang";
					result = strstr(buffer, src);
					if (result != NULL)
					{
						tok = strtok(result, " ");
						if (strcmp(tok, "hagang") == 0)
						{
							printf("Result : �ϰ�\n");
							fprintf(fp2, "4");
						}
					}
					else
					{
						tok = strtok(buffer, "-");
						src = "oenjjog";
						result = strstr(buffer, src);
						if (result != NULL)
						{
							tok = strtok(result, " ");
							if (strcmp(tok, "oenjjog") == 0)
							{
								printf("Result : ����\n");
								fprintf(fp2, "5");
							}
						}
						else
						{
							tok = strtok(buffer, "-");
							src = "oleunjjog";
							result = strstr(buffer, src);
							if (result != NULL)
							{
								tok = strtok(result, " ");
								if (strcmp(tok, "oleunjjog") == 0)
								{
									printf("Result : ������\n");
									fprintf(fp2, "6");
								}
							}
						}
					}
				}
			}
		}
	}
	else
	{
		printf("|     Incorrect command     |\n");
	}

	fclose(fp2);
}