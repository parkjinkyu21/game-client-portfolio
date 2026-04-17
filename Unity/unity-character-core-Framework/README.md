# Unity Character Core Framework

3D 캐릭터 기반 프로젝트를 위한 **Core 프레임워크 포트폴리오 코드 샘플**입니다.  
캐릭터 장착 시스템, 애니메이션 오버레이, 전역 이벤트 버스, 상태 흐름 제어, 커스텀 URP Renderer Feature를 중심으로 구성했습니다.

이 저장소는 범용 패키지 배포 목적보다는, 실제 프로젝트에서 사용한 **구조 설계 방식과 구현 코드**를 보여주기 위한 포트폴리오용 저장소입니다.

> **기술 스택**  
> Unity, C#, URP RenderGraph, Addressables, UniTask, DOTween

---

## 한눈에 보는 핵심 포인트

- **캐릭터 시스템**
  - Addressables + UniTask 기반 비동기 생성 / 장착 / 해제
  - 장착 파츠 렌더링, 숨김 규칙, 본 리매핑 처리
- **애니메이션 시스템**
  - Base + Upper / Lower / Full Overlay 구조
  - Avatar Mask 기반 상하체 분리 애니메이션
  - AnimatorOverrideController 기반 런타임 모션 교체
- **흐름 제어**
  - 전역 Event Bus
  - App 상태 전환용 State Machine
  - 선언형 순차 실행 StepFlow
- **렌더링**
  - URP RenderGraph 기반 ScreenBlend 화면 전환
  - LayerFiltered Renderer Feature 기반 선택 레이어 렌더링

---

## 저장소 성격

이 저장소는 **실행 가능한 완성 패키지 전체 공개본이 아닙니다.**

다음 목적에 초점을 둡니다.

- Unity 프로젝트에서의 코어 구조 설계 방식
- 캐릭터 중심 시스템의 책임 분리
- 비동기 로딩 / 애니메이션 / 렌더링을 포함한 런타임 제어 방식
- 실무형 코드 스타일과 확장 가능한 구조

일부 프로젝트 공용 타입, 샘플 씬, 패키지 설정, 에셋은 공개 범위에서 제외되어 있을 수 있습니다.

---

## 주요 코드 위치

### 캐릭터 시스템
- `Actor/CharacterActor/CharacterActor.cs`
- `Actor/CharacterActor/CharacterAnimator.cs`
- `Actor/CharacterActor/CharacterView.cs`
- `Actor/CharacterActor/CharacterService.cs`

### 이벤트 / 흐름 제어
- `Event/EventController.cs`
- `Flow/AppFlow.cs`
- `Flow/StepFlow.cs`

### 입력 추상화
- `Input/InputController.cs`
- `Input/InputReaderBase.cs`
- `Input/InputHandlerBase.cs`

### 렌더링
- `RenderFeature/ScreenBlend/*`
- `RenderFeature/LayerFiltered/LayerFilteredRenderFeature.cs`

---

## Architecture Overview

```text
App Layer
 └─ 게임 로직 / UI / 씬 관리

Core Layer
 ├─ Actor
 │   ├─ CharacterActor
 │   ├─ CharacterView
 │   ├─ CharacterAnimator
 │   └─ CharacterService
 ├─ Event
 │   └─ EventController
 ├─ Flow
 │   ├─ AppFlow
 │   └─ StepFlow
 ├─ Input
 │   ├─ InputReader
 │   └─ InputHandler
 └─ RenderFeature
     ├─ ScreenBlend
     └─ LayerFiltered
```

---

## 모듈 소개

### 1. Actor — 캐릭터 시스템

캐릭터 생성, 장착, 애니메이션, 렌더링을 담당하는 영역입니다.

구성 예시:

```text
CharacterService
 └─ CharacterActor
     ├─ CharacterView
     │   ├─ CharacterEquipPartsRenderer
     │   └─ CharacterEquipPartsHideRule
     └─ CharacterAnimator
         └─ MotionClipTable
```

핵심 포인트:

- Addressables + UniTask 기반 비동기 캐릭터 로드
- 동일 장비 부위 연속 요청 시 **버전 토큰 + CancellationToken** 으로 이전 요청 무효화
- 장비 교체 시 숨김 규칙 및 메시/머터리얼/본 연결 처리
- 오버레이 애니메이션 구조를 통해 상체 / 하체 / 전신 동작 분리

비동기 장착 처리 예시:

```csharp
private readonly Dictionary<CharacterEquipPart, int> _loadVersions = new();
private readonly Dictionary<CharacterEquipPart, CancellationTokenSource> _loadCts = new();

public virtual void Equip(CharacterEquipPart part, string prefabName)
{
    CancelLoading(part);
    int version = GetNextLoadVersion(part);

    var cts = CancellationTokenSource.CreateLinkedTokenSource(_destroyToken);
    _loadCts[part] = cts;

    EquipAsync(part, prefabName, version, cts.Token).Forget();
}
```

---

### 2. Event — 전역 이벤트 버스

모듈 간 직접 참조를 줄이기 위한 Pub-Sub 구조입니다.

```csharp
EventController.Instance.RegisterEvent<string, int>(EventKeys.UI_OPENED, OnUIOpened);
EventController.Instance.ExecuteEvent(EventKeys.UI_OPENED, "Inventory", 1);
```

구성 요소:

- `EventKey`
  - `readonly struct` 기반 값 객체
- `EventController`
  - Delegate 기반 등록 / 해제 / 실행
  - 중복 등록 방지
- `KeyValueEventSO<T>`
  - ScriptableObject 기반 이벤트 채널

---

### 3. Flow — 상태 전환 및 순차 실행

#### AppFlow
앱 단위 상태 전환을 담당하는 상태 머신입니다.

```csharp
AppFlow.Instance.ChangeState(new LobbyState());
```

- `OnEnter` 중 추가 전환 요청이 와도 **펜딩 처리**로 안전하게 전환

#### StepFlow
단계를 순차적으로 실행하는 선언형 흐름 빌더입니다.

```csharp
new StepFlow()
    .Step("LoadScene", () => SceneManager.LoadSceneAsync("Lobby"))
    .Wait(() => isSceneReady ? StepWaitResult.Success : StepWaitResult.Waiting)
    .Complete(() => Debug.Log("Scene loaded"))
    .OnCompleted(() => Debug.Log("All steps done"))
    .Start();
```

활용 포인트:

- 씬 로딩
- UI 초기화
- 순차 부팅 플로우
- 비동기 작업 정리

---

### 4. Input — 입력 추상화

플랫폼별 입력 차이를 Reader / Handler 구조로 분리했습니다.

```text
InputController
 └─ InputReaderBase<THandler>
     └─ InputHandlerBase
```

```csharp
InputController.Instance.SwitchTo(mobileInputReader);
InputController.Instance.BindHandler(playerInputHandler);
```

핵심 포인트:

- PC / Mobile 입력 구현 교체 용이
- Core 레이어는 입력 의미를 직접 알지 않음
- 전략 패턴 기반 확장 구조

---

### 5. RenderFeature — 커스텀 URP 렌더링

#### ScreenBlend
URP RenderGraph 기반 화면 전환 기능입니다.

구성 예시:

```text
ScreenBlendController
 └─ ScreenBlendRuntimeContextRegistry
     └─ ScreenBlendRuntimeContext
         └─ ScreenBlendPass
             └─ ScreenBlendFeature
```

핵심 포인트:

- 카메라 화면 스냅샷 캡처 후 ZoomOverlay 전환
- `WeakReference` 기반 카메라 추적
- `RTHandle` 기반 리소스 관리
- DOTween 기반 진행값 제어

#### LayerFilteredRenderFeature
특정 레이어만 선택적으로 렌더링하기 위한 Renderer Feature입니다.

- 레이어 분리 렌더링
- 머터리얼 오버라이드
- 깊이 테스트 오버라이드 지원

---

## Technical Highlights

| 영역 | 기술 | 설명 |
|---|---|---|
| 비동기 | Addressables + UniTask | 캐릭터 / 장비 비동기 로드 |
| 비동기 | 버전 토큰 + CancellationToken | 동일 부위 연속 요청 시 이전 요청 무효화 |
| 애니메이션 | Avatar Mask Overlay | 상체 / 하체 / 전신 오버레이 구조 |
| 애니메이션 | AnimatorOverrideController | 런타임 모션 클립 교체 |
| 렌더링 | URP RenderGraph | 화면 스냅샷 기반 ScreenBlend 처리 |
| 렌더링 | RTHandle + Blitter | 렌더 타깃 관리 |
| 메모리 | WeakReference | 파괴된 카메라 참조 자동 정리 |
| 설계 | Service / Strategy / State / Builder | 시스템 책임 분리 및 확장성 확보 |

---

## Design Patterns

- **Service**
  - `CharacterService`가 캐릭터 생성 / 장착의 진입점 역할 수행
- **Strategy**
  - `InputReaderBase`, `IMotionPlayer`를 통해 런타임 구현 교체
- **State**
  - `AppFlow` + `AppStateBase` 기반 앱 상태 전환
- **Builder**
  - `StepFlow`의 메서드 체이닝 기반 순차 흐름 구성
- **Event Bus**
  - `EventController` 기반 발행-구독 통신
- **Registry**
  - `ScreenBlendRuntimeContextRegistry`를 통한 카메라별 상태 관리

---

## 이 저장소에서 보여주고 싶은 점

- Unity 프로젝트에서 코어 레이어를 어떻게 분리하는지
- 캐릭터 시스템을 서비스 / 뷰 / 애니메이션 단위로 어떻게 나누는지
- 비동기 로딩과 애니메이션 충돌을 런타임에서 어떻게 다루는지
- 렌더 기능까지 포함해 구조적으로 확장 가능한 형태를 어떻게 만드는지

---

## 개발 환경

- **Engine:** Unity (URP)
- **Language:** C#
- **Async:** UniTask
- **Resource Loading:** Addressables
- **Tween:** DOTween
- **Rendering:** URP RenderGraph API

> 사용한 Unity / URP 세부 버전은 실제 프로젝트 기준에 따라 달라질 수 있으며, 본 저장소는 구조와 구현 방식 중심의 포트폴리오 샘플입니다.

---

## 라이선스

MIT License를 따릅니다.  
자세한 내용은 [LICENSE](LICENSE) 파일을 참고해주세요.

Unity 제공 패키지(Addressables, URP 등)는 Unity의 정책을 따르며,  
외부 라이브러리(UniTask, DOTween 등)는 각 라이브러리의 라이선스를 따릅니다.
