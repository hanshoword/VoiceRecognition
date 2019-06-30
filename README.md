## VoiceRecognition

### Objective
* 음성인식을 통한 편의성을 사용자에게 제공합니다.

* 신호처리(음성인식) 분야를 설계함으로써 설계능력을 함양하는데 목적이 있습니다.
 
### Necessity

* 4차 산업혁명이 도래함에 따라 사람과 사물이 통신망으로 연결되어 정보를 교환하는 영상인식 및 음성인식, 인공지능 등의 기술이 주목받고 있습니다. 

* 앞으로 이러한 기술들이 보편화가 된다면 인간의 삶에 큰 영향을 미칠 것입니다. 여기에 주목해 음성인식기술을 개발해, 사용자의 사물 조작 편의성을 높혀 보고자 합니다.

* 기존제품

   * 음성인식의 경우 아마존의 음성인식 인공지능(AI)비서 알렉스와 SKT의 한국어 인공지능(AI) 스피커 누구, KT에서 자체개발한 세계최초 인공지능(AI) TV 기가지니 등이 대표적으로 출시되어있습니다.

* 개선사항

  * 음성인식 : 주변의 대화내용이나 TV소리를 명령어로 착각하여 명령을 수행하거나 음성 인식률이 90%정도 혹은 인식률이 기대에 못 미친다는 문제점이 있습니다. 
     (사례 : 알렉사가 뉴스 앵커의 발언을 명령어로 착각하여 수행) 또한 인식기 내부에 등록된 어휘 중 가장 가까운 것을 출력하기 때문에 오동작 할 수 있다는 문제점이 있습니다. 
     
### Detailed objective

#### Core component and performance metric

* Core component

  * 음성 인식
  
* Performance metric

  * 인식률 90%이상( 제어 명령어 6개 )

* Comment

  * 음성 녹음 및 DB구축,
  
  * 음성 입력 및 음성 구간 검출(VAD) 모듈 구현
  
  * HTK를 이용한 HMM 모델 학습
  
  * 인식 결과 신뢰도 검증 모듈 구현
  
  #### Other objectives (Cost, design, etc)
  
  * DB를 구축 한 후 저수준의 API를 이용하여 음성 녹음을 구현
  
  * Sample Energy로 음성구간 검출(VAD)모듈을 구현
  
  * HTK를 통해 HMM모델을 구현 (녹음된 음성을 트레이닝) 한 후 마이크로 입력되는 음성과 특징을 비교하여 일치한다면 결과를 출력
  
### Constraint
#### HTK 음성인식, VAD, HMM
  * 자신의 목소리가 아닌 타인의 목소리를 입력 했을 때도 인식률이 높을것인 가에 대한 문제가 있습니다.
  
  * 파형이 비슷한 명령어의 경우 오작동이 일어 날 수 있습니다.
  
### Specification
  
* 음성 녹음

  * 저수준 API를 이용하여 음성녹음을 구현

    * Winmm, library, VAD

* VAD (Voice Activity Detection)

  * 음성 Energy 측정을 통하여 VAD (음성구간검출) 모듈을 구현
  
     * Sample 에너지

* HMM 모델

  * 대량의 녹음된 음성과 HTK 명령어를 이용하여 HMM모델을 훈련
  
    *Hcopy, HParse, HLEd, HComV, HERest
    
* HVite 인식 실험

  * 실시간으로 입력되는 음성 특징과 훈련시킨 음성데이터의 특징을 비교 한 뒤 인식결과를 텍스트파일로 출력합니다.
    * HVite
    
* 음성 신뢰도 검증 모듈

  * 음성 신뢰도 검증 모듈은 n-best 결과의 1순위, 2순위 유사성을 판단, 오작동을 어느 정도 방지합니다.
  
    * n-best

### Task assignment

Main component | Implementation approach |
-------------  | ----------------------- |
 음성 DB 구축   |  여러 사용자의 명령어에 대한 음성을 녹음하여 데이터를 모은다. (마이크 이용)
HMM 모델 학습 구현  |  HTK 명령어((HComV)를 이용해 HMM초기모델을 만들어 준 후, HERest(재추정)을 통해 훈련시켜 HMM모델을 완성한다.
VAD (Voice activity Detection) | 실시간으로 사용자의 음성을 입력받아, Buffer에 저장된 1frame Sample energy를 구한 후 일정 threadshold이상이면 음성으로 판단하여 음성부분을 검출한다.
HVite | 해당 음성의 특징추출을 통해 훈련된 음성의 특징과 비교한다. 
HVite | 특징 비교 후 인식된 결과를 mlf 파일로 출력한다. 
음성 신뢰도 검증모듈 구현 | 출력된 mlf 파일의 n-best결과 유사성을 통해 잘못 추정될 수 있는 가능성을 배제하여 오작동을 어느정도 방지한다.
최종 txt 출력 | txt파일로 최종 결과물을 출력한다.

### Design (Block & Sequence Diagram)

<img src = "https://user-images.githubusercontent.com/47768726/60395501-8960ac00-9b6f-11e9-82aa-3551452918a8.jpg" width = 45% height = 45%></img>
<img src = "https://user-images.githubusercontent.com/47768726/60395542-1ad01e00-9b70-11e9-8eb3-7c968385deff.jpg" width = 45% height = 45%></img>


### Functional block

```
- Speech Data : 여러 사용자의 음성 데이터(명령어)를 모은다.
- VAD & Feature Extraction : 현재 버퍼에 들어있는 한 프레임의 샘플 에너지를 모두 더해서 총 에너지를 계산하고, 일정 임계치를 넘으면 음성으로 판별하고 넘지 않으면 비 음성으로 판별한다. 음성부분을 검출 한 후 Hcopy명령을 통해 특징을 추출한다.

- Acoustic Model Traning (HTK) : HTK 명령어를 통해 HMM초기모델을 만들어준다. HERESET, 재추정을 통해서 HMM모델을 학습시켜준다.

- Acoustic Model (HMM) : 재추정을 통하여 훈련된 HMM모델이 완성된다.


- Start of Speech : 실시간으로 사용자의 음성을 입력받는다. ( Start of Speech)
- VAD : 위와 마찬가지로 샘플에너지를 통해 음성을 검출한다.
- Feature Extraction : 실시간으로 받아온 음성의 특징을 추출한다.


- Calculation of Observation : 실시간 음성과 훈련된 음성의 특징을 비교한다.
- Recognition Results ( Text file) : 음성인식결과를 텍스트 파일로 출력한다.

```

### Source code list


Functional block | Detailed functional block | Prototype of functions therein
---------------- | ---------------- | ---------------- | 
Recognition Module | Start Speech | winmm.lib waveInOpen waveInPrepareHeader waveInAddBuffer waveInStart waveInUnprepareHeader waveInClose
Recognition Module | VAD | void RecognitionFor1Frame(WAVEHDR* wave) void CALLBACK WaveInProc(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
Recognition Module | Recognition Results | void PrintVoiceResult(char* buffer, char* src, char* result, char* tok, int data_size)

### Final Output

<img src = "https://user-images.githubusercontent.com/47768726/60395819-1c034a00-9b74-11e9-87ae-6e074bfdb3dc.png" width = 90% height = 90%></img>

* VAD
 
 * void RecognitionFor1Frame(WAVEHDR* wave);
    ```
    
    초기모델은 sample energy만으로 음성구간을 판별 

    이후 음성구간에서 비 음성 구간으로 바뀌는 동시에 비 음성 프레임이 5번나오면 끝점추출 하여 음성구간 검출
    
    ```
* 결과물 출력 / 음성 검증
 
 * void PrintVoiceResult(char* buffer, char* src, char* result, char* tok, int data_size);
 
  ```
  
  초기모델로 명령어에 따른 숫자만 출력
  
  일정 시간동안만 특정 숫자를 출력하도록 수정
  
  이후 결과물 출력 이전에 검증 모듈을 한번 거친 후 결과물을 출력을 결정함.
  
  ```
  
 
  ### Performance analysis and evaluation
  
  
* 음성DB구축

  *타인음성으로 모델 학습 시 인식률이 떨어져 본인 음성으로 구축  

* 음성녹음 및 VAD

  *  winmm 라이브러리를 통해 wave 파일 녹음가능
  * 샘플에너지를 통한 음성 및 비음성구간 검출가능

* HMM모델

  *음성 DB를 HTK명령을 통해 HMM모델 학습하여 구현

* 신뢰도 검증모듈

   * n-best 유사성을 파악해 한 번 더 검출 하여 오작동 확률을 줄임

* 인식률

  * 조용한 환경에서 본인 목소리로 테스트 시 90%인식률을 보였음.
  * 사람의 음성이 있는 환경과 타인 목소리로 실험 시 인식률이 떨어졌음

### Improvement

  * 음성 데이터베이스를 늘릴수록 인식률 증가를 보였다. 하지만 계속해서 추가하면 오버슈팅이 발생해서 인식률이 오히려 떨어질 것입니다.

  * 잡음이 없는 환경에서 실험할수록 인식률이 증가했다. 푸리에 변환을 통해 특정 잡음 주파수를 제거해준다면 인식률이 더 높아질 것으로 보입니다.

  * 신뢰도 검증모듈을 추가하여 n-best결과의 1순위, 2순위 likelihood 차이를 분석해서 명령어 오작동을 방지할 수 있습니다. 
  
### Conclusion
```
프로젝트를 통해 신기술인 음성인식을 접 할 수 있었습니다. 

차량내부 음성인식이나 스마트TV 같은 경우 특정 잡음제거기술이 필수적이라고 생각되었습니다. 

잡음제거는 FFT와 잡음제거 필터를 구현하면 제거가 가능 할 것 같았습니다. 

시중에 나온 음성인식 제품의 경우 다양한사람들에 대한 음성에도 불구하고 인식률이 90%정도 인 것을 보면, 

수많은 음성 데이터베이스가 구축되어 있을 것이라고 생각되었습니다.

```
