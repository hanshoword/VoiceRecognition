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
	unsigned int riff_size; // file length - 8 in bytes (문자열 아래부터 끝까지 byte 단위의 크기)
	char wave_id[4]; // "WAVE" 
	char format_id[4]; // "fmt " 
	unsigned int format_size; // format size (canonical form= 16L, L: long integer)
	unsigned short format_type; // 1 for PCM (almost wavfile : pcm)
	unsigned short ChannelNo; // 1 for mono, 2 for stereo (음성파일의 채널 수)
	unsigned int SamplesPerSec; // 44.1 KHz (샘플링 주파수)
	unsigned int AvgBytesPerSec; // sample rate * block align (1초당 변환에 사용되는 평균 데이터량)
	unsigned short BytesPerSample; // channels * bits/sample/8 (모든채널의 샘플당 bit 수)
	unsigned short BitsPerSample; // 8 for 8 bits, 16 for 16bits (샘플 한개의 bit)
	char data_id[4]; // "data" 
	unsigned int data_size; // length of the data block= (riff_size + 8 - 44)/BytesPerSample 
}WAVE_Header;

WAVE_Header Header;
WAVEFORMATEX WaveFormat; //Wave foramt 구조체

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
	WAVEHDR *pHdr = NULL; //웨이브폼 오디오 버퍼에 대한 정보

	switch (uMsg)
	{
	case WIM_CLOSE: // Recording close
		break;
	case WIM_DATA: //녹음 버퍼가 가득차서 드라이버로 부터 프로그램에게 메세지 전달
		pHdr = (WAVEHDR *)dwParam1; //데이터를 포함하는 버퍼를 식별하는 포인터에 대한 값으로 초기화 

		RecognitionFor1Frame(pHdr);
		if (waveInAddBuffer(hwi, pHdr->lpNext, sizeof(WAVEHDR))) //다음 버퍼를 사용한다고 드라이버에게 전달
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

	InitHeader(); //Wave file header 초기화
	
	WAVEHDR WaveInHeader[BUFFER_COUNT]; //버퍼와 메모리공간을 연결해주는 구조체 (버퍼에 대한 정보를 가짐)
	HWAVEIN hWaveIn;  // Recording handler
	unsigned char *pWaveInBuffer[BUFFER_COUNT]; //Input buffer

	WaveFormat.wFormatTag = WAVE_FORMAT_PCM; //PCM
	WaveFormat.wBitsPerSample = BIT_PER_SAMPLE; //Sampling 단위 : 16 bit 
	WaveFormat.nChannels = MONO; // 녹음방식 : MONO 
	WaveFormat.nSamplesPerSec = SAMPLE_RATE;  //Sampling 주기 : 44100 Hz
	WaveFormat.nBlockAlign = WaveFormat.nChannels * BIT_PER_SAMPLE / BIT; 
	//(채널수 * 샘플당 비트(16bit)) / 8(byte) 한 샘플당 몇 바이트인지 메모리크기를 명시
	WaveFormat.nAvgBytesPerSec = WaveFormat.nBlockAlign * SAMPLE_RATE; // 1초당 변환에 사용되는 평균 데이터량을 명시
	WaveFormat.cbSize = 0; //PCM인 경우 무시

	/*입력버퍼 메모리 할당*/
	for (int i = 0; i < BUFFER_COUNT; i++)
	{
		pWaveInBuffer[i] = (unsigned char*)malloc(BUFFER_SIZE * WaveFormat.nBlockAlign); //실제 데이터가 쓰여지는 버퍼
		ZeroMemory(pWaveInBuffer[i], BUFFER_SIZE * WaveFormat.nBlockAlign);
	}

	printf("Input buffer memory allocation complete\n");

	/*사운드 드라이버와 응용프로그램이 데이터를 주고받을 때 필요한 정보 설정(입력 버퍼)*/
	for (int i = 0; i < BUFFER_COUNT; i++)
	{
		WaveInHeader[i].lpData = (LPSTR)pWaveInBuffer[i]; //Current buffer : 녹음 할 때 사용 할 메모리 공간에 대한 주소
		WaveInHeader[i].dwBufferLength = BUFFER_SIZE * WaveFormat.nBlockAlign;// lpData에 명시된 메모리의 크기
		WaveInHeader[i].dwBytesRecorded = 0;
		WaveInHeader[i].dwUser = 0;
		WaveInHeader[i].dwFlags = 0;
		WaveInHeader[i].dwLoops = 0;
		WaveInHeader[i].lpNext = &WaveInHeader[(i + 1) % BUFFER_COUNT]; //next buffer (ring buffer)
	}

	printf("WaveInheader information settings complete\n");

	/*녹음장치에 대한 핸들을 얻고, 콜백함수를 등록한다*/

	if (waveInOpen(&hWaveIn, WAVE_MAPPER, &WaveFormat, (DWORD_PTR)WaveInProc, NULL, CALLBACK_FUNCTION))
	{
		printf("Failed to open waveform input device.\n");
		//반환 값이 에러라면 에러메세지 출력
		exit(0);
	}

	printf("Recorder handle & Callback function registration complete\n");


	/*녹음 할 준비가 되었음을 드라이버에게 알린다*/
	for (int i = 0; i < BUFFER_COUNT; i++)
	{
		if (waveInPrepareHeader(hWaveIn, &WaveInHeader[i], sizeof(WAVEHDR)))
		{
			printf("Failed to prepareHeader.\n");
			//반환 값이 에러라면 에러메세지 출력
			exit(0);
		}
	}

	printf("Recording preparing\n");

	/*사용할 입력버퍼를 드라이버에게 전달한다*/

	if (waveInAddBuffer(hWaveIn, &WaveInHeader[0], sizeof(WAVEHDR)))
	{
		printf("Failed to read block from device.\n");
		//반환 값이 에러라면 에러메세지 출력
		exit(0);
	}
	printf("Delivery the Input buffer to sound driver\n");


	printf("Recording preparation complete\n\n");


	/*녹음장치를 이용하여 녹음을 시작한다*/
	if (waveInStart(hWaveIn))
	{
		printf("Failed to start Recording.\n");
		//반환 값이 에러라면 에러메세지 출력
		exit(0);
	}

	printf("[Please input voice] : Press any key if you want to exit.\n");

	while (!kbhit());

	/*녹음 버퍼를 정리 : 메모리 해제*/

	for (int i = 0; i < BUFFER_COUNT; i++)
	{
		waveInUnprepareHeader(hWaveIn, &WaveInHeader[i], sizeof(WAVEHDR));
	}
	waveInClose(hWaveIn); // waveInOpen에 의해서 사용되던 장치를 해제

	printf("Closing Sound Recorder\n");

	/*버퍼 메모리 반환*/

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

	if (frame_total_energy > 23000000) //13000000 Speech threshold : 경험적으로 설정
	{
		Header.riff_size += wave->dwBufferLength;
		Header.data_size += wave->dwBufferLength;
		
		speechFlag = 1;

		fseek(fp, 0, SEEK_SET); //wav file pointer를 초기로 이동시킴
		fwrite(&Header, sizeof(Header), 1, fp); //wav file에 header를 쓴다
		fseek(fp, 0, SEEK_END); //file pointer를 파일의 끝으로 이동시킴
		printf("speech data\n");
		
		fwrite(wave->lpData, wave->dwBufferLength, 1, fp); //현재 버퍼에 저장된 값을 파일에 쓴다.
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
	Header.riff_size = 36; //파일 전체 사이즈에서, 위에 'Riff ID' 와 자기자신인 'Chunk Size' 를 제외한 값
	memcpy(&Header.wave_id, "WAVE", 4); //file form
	memcpy(&Header.format_id, "fmt ", 4); //fmt
	Header.format_size = 16; //고정값
	Header.format_type = WAVE_FORMAT_PCM; //고정값
	Header.ChannelNo = MONO; //채널 모노
	Header.SamplesPerSec = SAMPLE_RATE; //44100 hz
	Header.AvgBytesPerSec = SAMPLE_RATE * 2; //sample rate * channelNo
	Header.BytesPerSample = MONO * BIT_PER_SAMPLE / BIT; //Sample Frame 의 크기
	Header.BitsPerSample = BIT_PER_SAMPLE; //Sample 한 개 몇 bit인지 명시
	memcpy(&Header.data_id, "data", 4); //고정값
	Header.data_size = 0; //헤더제외 데이터 크기
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
				printf("Result : 시동\n");
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
					printf("Result : 정지\n");
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
						printf("Result : 상승\n");
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
							printf("Result : 하강\n");
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
								printf("Result : 왼쪽\n");
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
									printf("Result : 오른쪽\n");
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